#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>

#include "daemonize.hpp"

static ::pid_t registration_id;
static ::pid_t balancer_id;

void sigint(int signo) {
    kill(registration_id, SIGINT);
    kill(balancer_id, SIGINT);
    exit(1);
}

int main() {
    mrpc::daemonize("intermediate_main");

    if (::mkfifo("/var/mrpc_fifo", O_RDWR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR) < 0) {
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

    for( ; ;) {
        pause();
    }
}