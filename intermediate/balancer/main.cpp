#include "include/balancer.hpp"
#include "signal.hpp"

mrpc::fs::path fifo_path{};
int fifo_fd;

// handle signal from register
void sigusr1(int signo) {
    if ((fifo_fd = open(fifo_path.c_str(), O_RDONLY)) < 0) {
        std::cerr << "Failed open FIFO" << std::endl;
        std::cerr << "errno: " << errno << std::endl;
    }
    std::string command{};
    char buf[1024];
    while (::read(fifo_fd, buf, 1024) > 0) {
        command.append(buf);
    }
    ::close(fifo_fd);

    std::string op = command.substr(0, 2);
    std::string func_name = command.substr(3);

    // Normal Update
    if (op == "NU") {
        if (mrpc::Balancer::instance().isAvailable(func_name)) {
            char ret = 0;
            if ((fifo_fd = open(fifo_path.c_str(), O_WRONLY)) < 0) {
                std::cerr << "Failed open FIFO" << std::endl;
                std::cerr << "errno: " << errno << std::endl;
            }
            ::write(fifo_fd, &ret, 1);
            ::close(fifo_fd);
        }
    }
    // Force Update
    else if (op == "FU") {
        mrpc::Balancer::instance().requestData(func_name);
    }
}

// handle signal from parent process
void sigusr2(int signo) {
    if ((fifo_fd = open(fifo_path.c_str(), O_RDONLY)) < 0) {
        std::cerr << "Failed open FIFO" << std::endl;
        std::cerr << "errno: " << errno << std::endl;
    }
    std::string func_name{};
    char buf[1024];
    while (::read(fifo_fd, buf, 1024) > 0) {
        func_name.append(buf);
    }
    ::close(fifo_fd);

    std::string op = func_name.substr(0, 2);

    auto ret = mrpc::Balancer::instance().epoll(func_name);

    if ((fifo_fd = open(fifo_path.c_str(), O_WRONLY)) < 0) {
        std::cerr << "Failed open FIFO" << std::endl;
        std::cerr << "errno: " << errno << std::endl;
    }
    auto ip = std::get<0>(ret);
    auto port = std::to_string(std::get<1>(ret));
    ::write(fifo_fd, ip.c_str(), ip.size());
    ::write(fifo_fd, ":", 1);
    ::write(fifo_fd, port.c_str(), port.size());
    ::close(fifo_fd);
}

int main(int argc, char** argv) {
    auto register_pid = static_cast<::pid_t>(std::strtol(argv[1], nullptr, 10));
    fifo_path = argv[2];

    mrpc::Balancer::instance().initialize(register_pid, fifo_path);

    if (mrpc::signal(SIGUSR1, sigusr1) == SIG_ERR) {
        std::cerr << "set signal error" << std::endl;
    }

    if (mrpc::signal(SIGUSR2, sigusr2) == SIG_ERR) {
        std::cerr << "set signal error" << std::endl;
    }

    for( ; ;) {
        pause();
    }
}