#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <filesystem>

#include "signal.hpp"

static ::pid_t registration_id;
static ::pid_t balancer_id;

void sigint(int signo) {
    kill(registration_id, SIGINT);
    kill(balancer_id, SIGINT);
    exit(1);
}

//todo: make it be daemon process
int main() {
    if (signal(SIGINT, sigint) == SIG_ERR) {
        std::cerr << "set signal error" << std::endl;
        return -1;
    }
    std::filesystem::path cur = std::filesystem::current_path();
    std::filesystem::path registration_path = cur / "registration";
    std::filesystem::path balancer_path = cur / "balancer";

    registration_id = ::fork();
    if (registration_id < 0) {
        std::cerr << "fork error" << std::endl;
        return -1;
    } else if (registration_id == 0) {
        if(::execl(registration_path.c_str(), "registration") < 0) {
            std::cerr << "exec error" << std::endl;
            return -1;
        }
    } else {
        ::sleep(10);
    }

    balancer_id = ::fork();
    if (balancer_id < 0) {
        std::cerr << "fork error" << std::endl;
        return -1;
    } else if (balancer_id == 0) {
        if (::execl(balancer_path.c_str(), "balancer", std::to_string(registration_id).c_str(), (char*)0) < 0) {
            std::cerr << "exec error" << std::endl;
            return -1;
        }
    } else {
        ::sleep(10);
    }

    if (::mkfifo("/tmp/fifo", O_RDWR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR) < 0) {
        std::cerr << "FIFO exist" << std::endl;
    }

    for( ; ;) {
        pause();
    }
    return 0;
}