#include "include/balancer.hpp"
#include "daemonize.hpp"

static mrpc::fs::path fifo_path{};
static int fifo_fd;

// handle signal from register
void sigusr1(int signo) {
    if ((fifo_fd = open(fifo_path.c_str(), O_RDONLY)) < 0) {
        syslog(LOG_ERR, "open FIFO error");
        exit(errno);
    }
    std::string func_name{};
    char buf[1024]{};
    while (::read(fifo_fd, buf, 1023) > 0) {
        func_name.append(buf);
    }
    ::close(fifo_fd);

    mrpc::Balancer::instance().requestData(func_name);
}

int main(int argc, char** argv) {
    mrpc::daemonize("balancer");
    auto register_pid = static_cast<::pid_t>(std::strtol(argv[1], nullptr, 10));
    fifo_path = "/var/mrpc_fifo";

    if ((fifo_fd = ::open(fifo_path.c_str(), O_WRONLY)) < 0) {
        ::syslog(LOG_ERR, "open FIFO error");
        exit(errno);
    }
    ::pid_t pid = ::getpid();
    ::write(fifo_fd, &pid, sizeof(::pid_t));
    ::close(fifo_fd);

    mrpc::Balancer::instance().initialize(register_pid, fifo_path);

    if (mrpc::signal(SIGUSR1, sigusr1) == SIG_ERR) {
        syslog(LOG_ERR, "set signal error");
        exit(errno);
    }

    for( ; ;) {
        pause();
    }
}