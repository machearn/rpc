#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/msg.h>

#include <filesystem>

#include "nlohmann/json.hpp"

#include "daemonize.hpp"
#include "socket.hpp"
#include "misc.hpp"

static bool hold = true;

static ::pid_t registration_id;
static ::pid_t balancer_id;

static std::shared_ptr<::sockaddr_in> service_addr;
static int msqid;

// add operation to delete message queue
void sigint(int signo) {
    kill(registration_id, SIGINT);
    kill(balancer_id, SIGINT);
    if (::msgctl(msqid, IPC_RMID, nullptr) < 0) {
        ::syslog(LOG_ERR, "remove message queue %d failed", msqid);
        exit(errno);
    }
    exit(1);
}

void sigusr1(int signo) {
    int fifo_fd;
    if ((fifo_fd = open("/var/balancer_fifo", O_RDONLY)) < 0) {
        syslog(LOG_ERR, "open FIFO error");
        exit(errno);
    }
    service_addr = std::make_unique<::sockaddr_in>();
    ::in_port_t port;
    ::read(fifo_fd, &(service_addr->sin_addr), sizeof(service_addr->sin_addr));
    ::read(fifo_fd, &port, sizeof(::in_port_t));
    service_addr->sin_port = htons(port);
    ::close(fifo_fd);

    hold = false;
}

// add operation in main process to create message queue
int main() {
    mrpc::daemonize("intermediate_main");

    ::key_t key = ftok("/dev/null", 0x40);
    if ((msqid = ::msgget(key, 0666|IPC_CREAT)) < 0) {
        ::syslog(LOG_ERR, "create message queue failed");
        exit(errno);
    }

    if (::mkfifo("/var/mrpc_fifo", O_RDWR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR) < 0) {
        ::syslog(LOG_ALERT, "FIFO exist");
    }

    if (::mkfifo("/var/balancer_fifo", O_RDWR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR) < 0) {
        ::syslog(LOG_ALERT, "FIFO exist");
    }

    if (signal(SIGINT, sigint) == SIG_ERR) {
        ::syslog(LOG_ERR, "set signal error");
        exit(errno);
    }
    std::filesystem::path cur = std::filesystem::current_path();
    std::filesystem::path registration_path = cur / "registration";
    std::filesystem::path balancer_path = cur / "balancer";

    int pid;
    pid = ::fork();
    if (pid < 0) {
        ::syslog(LOG_ERR, "fork registration error");
        exit(errno);
    } else if (pid == 0) {
        if(::execl(registration_path.c_str(), "registration") < 0) {
            ::syslog(LOG_ERR, "execute registration error");
            exit(errno);
        }
    }

    int fifo_fd;
    if ((fifo_fd = ::open("/var/mrpc_fifo", O_RDONLY)) < 0) {
        ::syslog(LOG_ERR, "open FIFO error");
        exit(errno);
    }
    ::read(fifo_fd, &registration_id, sizeof(::pid_t));
    ::close(fifo_fd);

    pid = ::fork();
    if (pid < 0) {
        ::syslog(LOG_ERR, "fork balancer error");
        exit(errno);
    } else if (pid == 0) {
        if (::execl(balancer_path.c_str(), "balancer", std::to_string(registration_id).c_str(), NULL) < 0) {
            ::syslog(LOG_ERR, "execute balancer error");
            exit(errno);
        }
    }

    std::string ip{"127.0.0.1"};
    ::in_port_t port{10088};

    auto host_socket = std::make_unique<mrpc::Socket>(PF_INET, SOCK_STREAM, 0, 10);
    if (!host_socket->valid()) {
        ::syslog(LOG_ERR, "create socket error");
        std::exit(errno);
    }

    auto host_addr = std::make_shared<::sockaddr_in>();
    host_addr->sin_family = AF_INET;
    ::inet_pton(AF_INET, ip.c_str(), &host_addr->sin_addr);
    host_addr->sin_port = htons(port);

    host_socket->bind(reinterpret_cast<const sockaddr*>(host_addr.get()), sizeof(::sockaddr_in));
    host_socket->listen();

    auto remote_addr = std::make_shared<::sockaddr_in>();
    ::socklen_t remote_addr_len = sizeof(sockaddr_in);

    int connfd;

    for( ; ;) {
        connfd = host_socket->accept(reinterpret_cast<sockaddr*>(remote_addr.get()), &remote_addr_len);
        if (connfd < 0) {
            ::syslog(LOG_ERR, "connection error");
            std::exit(errno);
        }

        ::pid_t child_pid = ::fork();
        if (child_pid < 0) {
            ::syslog(LOG_ERR, "fork error");
            std::exit(errno);
        } else if (child_pid == 0) {
            if (signal(SIGINT, sigusr1) == SIG_ERR) {
                ::syslog(LOG_ERR, "set signal error");
                exit(errno);
            }

            std::size_t total_size = 0;
            host_socket->close();
            host_socket->recv_length(connfd, &total_size, 0);

            std::string data(total_size, 0);
            host_socket->recvn(connfd, data, 0);

            nlohmann::json meta = nlohmann::json::parse(data);
            ::pid_t this_pid = ::getpid();
            auto func_name = meta["func_name"].get<std::string>();

            mrpc::Message message{};
            ::memcpy(&message.pid, &this_pid, sizeof(::pid_t));
            ::memcpy(message.buffer, func_name.c_str(), func_name.size());
            if (::msgsnd(0, &message, 512+sizeof(::pid_t), 0) < 0) {
                ::syslog(LOG_ERR, "add message failed");
                host_socket->return_length(connfd, -1, 0);
                exit(errno);
            }
            while (hold);

            // send meta to server
            auto service_socket = std::make_unique<mrpc::Socket>(PF_INET, SOCK_STREAM, 0, 10);
            if (!service_socket->valid()) {
                ::syslog(LOG_ERR, "create server socket failed");
                exit(errno);
            }

            service_socket->connect(reinterpret_cast<const sockaddr*>(service_addr.get()), sizeof(::sockaddr_in));
            auto args = nlohmann::to_string(meta["args"]);

            service_socket->send_length(args.size(), 0);
            service_socket->sendn(args, 0);

            std::size_t return_len{};
            service_socket->recv_length(service_socket->get_socket(), &return_len, 0);
            std::string return_str(return_len, 0);
            service_socket->recvn(service_socket->get_socket(), return_str, 0);

            service_socket->send_length(return_len, 0);
            service_socket->sendn(return_str, 0);

            ::close(connfd);
            exit(0);
        } else {
            ::close(connfd);
        }
    }
}