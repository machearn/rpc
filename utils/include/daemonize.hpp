#ifndef RPC_DAEMONIZE_HPP
#define RPC_DAEMONIZE_HPP

#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include <iostream>

#include "signal.hpp"

namespace mrpc {
void daemonize(std::string cmd) {
    ::pid_t pid;
    struct rlimit rl;

    umask(0);
    if (::getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        std::cerr << cmd << ": can't get file limit" << std::endl;
        exit(errno);
    }

    if ((pid = fork()) < 0) {
        std::cerr << cmd << ": can't fork" << std::endl;
        exit(errno);
    }
    else if (pid > 0)
        exit(0);
    ::setsid();

    if (mrpc::signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        std::cerr << cmd << ": can't ignore SIGHUB" << std::endl;
        exit(errno);
    }
    if ((pid = fork()) < 0) {
        std::cerr << cmd << ": can't fork" << std::endl;
        exit(errno);
    }
    else if (pid > 0)
        exit(0);

    if (::chdir("/") < 0) {
        std::cerr << cmd << ": can't change to root directory" << std::endl;
        exit(errno);
    }

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (int i = 0; i < rl.rlim_max; i++)
        ::close(i);

    int fd0 = ::open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);

    ::openlog(cmd.c_str(), LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}
}// namespace mrpc

#endif //RPC_DAEMONIZE_HPP
