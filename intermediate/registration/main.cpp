#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/shm.h>

#include <filesystem>

#include "nlohmann/json.hpp"
#include "include/register.hpp"
#include "daemonize.hpp"
#include "socket.hpp"
#include "misc.hpp"

static std::filesystem::path fifo_path{};
static int fifo_fd;

void sigusr1(int signo) {
    if ((fifo_fd = ::open(fifo_path.c_str(), O_RDONLY)) < 0) {
        ::syslog(LOG_ERR, "open FIFO error");
        std::exit(errno);
    }
    std::string func_name{};
    char buf[1024];
    while (::read(fifo_fd, buf, 1024) > 0) {
        func_name.append(buf);
    }
    ::close(fifo_fd);

    auto result = mrpc::Register::instance().query(func_name);
    nlohmann::json json;
    for (auto& element: result) {
        json.push_back({
                               {"func_name", func_name},
                               {"ip",        std::get<0>(element)},
                               {"port",      std::get<1>(element)}
                       });
    }
    std::string json_str = json.dump();

    if ((fifo_fd = ::open(fifo_path.c_str(), O_RDONLY)) < 0) {
        ::syslog(LOG_ERR, "open FIFO error");
        std::exit(errno);
    }

    ::write(fifo_fd, json_str.c_str(), json_str.size());
    ::close(fifo_fd);
}

void updateBalancer(::pid_t balancer_id, const std::string& func_name) {
    kill(balancer_id, SIGUSR1);
    if ((fifo_fd = ::open(fifo_path.c_str(), O_WRONLY)) < 0) {
        ::syslog(LOG_ERR, "open FIFO error");
        exit(errno);
    }
    ::write(fifo_fd, func_name.c_str(), func_name.size());
    ::close(fifo_fd);
}

int main(int argc, char** argv) {
    mrpc::daemonize("registration");
    fifo_path = "/var/mrpc_fifo";

    if ((fifo_fd = ::open(fifo_path.c_str(), O_WRONLY)) < 0) {
        ::syslog(LOG_ERR, "open FIFO error");
        std::exit(errno);
    }
    ::pid_t pid = ::getpid();
    ::write(fifo_fd, &pid, sizeof(::pid_t));
    ::close(fifo_fd);

    ::pid_t balancer_id;
    if ((fifo_fd = ::open("/var/mrpc_fifo", O_RDONLY)) < 0) {
        ::syslog(LOG_ERR, "open FIFO error");
        exit(errno);
    }
    ::read(fifo_fd, &balancer_id, sizeof(::pid_t));
    ::close(fifo_fd);


    if (mrpc::signal(SIGUSR1, sigusr1) == SIG_ERR) {
        ::syslog(LOG_ERR, "set signal error");
        exit(errno);
    }
    mrpc::configure(std::move(mongocxx::uri{}));

    auto* threshold = new mrpc::Counter{};
    ::key_t key;
    int shm_id;

    if ((key = ::ftok("/dev/null", 0x35)) < 0) {
        ::syslog(LOG_ERR, "create IPC identifier error");
        std::exit(errno);
    }

    if ((shm_id = shmget(key, sizeof(mrpc::Counter), 0666 | IPC_CREAT)) < 0) {
        ::syslog(LOG_ERR, "create shared memory error");
        std::exit(errno);
    }

    std::string ip{"127.0.0.1"};
    ::in_port_t port = 10080;

    auto host_socket = std::make_unique<mrpc::Socket>(PF_INET, SOCK_STREAM, 0, 10);
    if (!host_socket->valid()) {
        ::syslog(LOG_ERR, "create socket error");
        std::exit(errno);
    }

    auto host_addr = std::make_unique<::sockaddr_in>();
    host_addr->sin_family = AF_INET;
    ::inet_pton(AF_INET, ip.c_str(), &host_addr->sin_addr);
    host_addr->sin_port = htons(port);

    host_socket->bind(std::unique_ptr<const ::sockaddr>((::sockaddr*) host_addr.get()),
                      sizeof(::sockaddr_in));
    host_socket->listen();

    auto remote_addr = std::make_unique<::sockaddr_in>();
    ::socklen_t remote_addr_len = sizeof(sockaddr_in);

    int connfd;

    for (;;) {
        connfd = host_socket->accept(std::unique_ptr<::sockaddr>((::sockaddr*) remote_addr.get()),
                                     &remote_addr_len);
        if (connfd < 0) {
            ::syslog(LOG_ERR, "connection error");
            std::exit(errno);
        }

        // todo: multi-process to multi-thread
        pid_t child_pid = 0;
        if ((child_pid = ::fork()) < 0) {
            ::syslog(LOG_ERR, "fork error");
            std::exit(errno);
        } else if (child_pid == 0) {
            threshold = (mrpc::Counter*) ::shmat(shm_id, nullptr, 0);
            if (reinterpret_cast<int64_t>(threshold) < 0) {
                ::syslog(LOG_ERR, "attach shared memory error");
                std::exit(errno);
            }
            std::size_t total_size = 0;
            host_socket->close();
            host_socket->recv_length(connfd, &total_size, 0);

            std::string message(total_size, 0);
            host_socket->recvn(connfd, message, 0);

            nlohmann::json package(message);
            mrpc::OP op = package["op"];

            switch (op) {
                case mrpc::OP::DROP: {
                    mrpc::Register::instance().drop(nlohmann::to_string(package["data"]));
                    updateBalancer(balancer_id, package["data"]["func_name"].get<std::string>());
                    threshold->lock();
                    threshold->reset();
                    threshold->unlock();
                    break;
                }
                case mrpc::OP::INSERT: {
                    mrpc::Register::instance().add(nlohmann::to_string(package["data"]));
                    threshold->lock();
                    threshold->decrement();
                    threshold->unlock();
                    if (threshold->get() <= 0) {
                        updateBalancer(balancer_id,
                                       package["data"]["func_name"].get<std::string>());
                        threshold->lock();
                        threshold->reset();
                        threshold->unlock();
                    }
                    break;
                }
                case mrpc::OP::UPDATE: {
                    mrpc::Register::instance().update(
                            nlohmann::to_string(package["data"]["origin"]),
                            nlohmann::to_string(package["data"]["replace"]));
                    updateBalancer(balancer_id,
                                   package["data"]["origin"]["func_name"].get<std::string>());
                    threshold->lock();
                    threshold->reset();
                    threshold->unlock();
                    break;
                }
                default:
                    ::syslog(LOG_WARNING, "invalid operation");
            }
            ::close(connfd);
            ::shmdt(threshold);
            std::exit(0);
        }
        ::close(connfd);
    }
}