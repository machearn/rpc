#ifndef RPC_SOCKET_HPP
#define RPC_SOCKET_HPP

#include <unistd.h>
#include <sys/socket.h>

#include <memory>
#include <algorithm>
#include <string>

namespace mrpc {
constexpr const size_t RECV_BUF_SIZE = 1024;

class Socket {
public:
    Socket() : socket_(-1), backlog_(0) {}

    Socket(int domain, int type, int protocol, int backlog) : backlog_(backlog) {
        socket_ = ::socket(domain, type, protocol);
    }

    ~Socket() {
        ::close(socket_);
    }

    int get_socket() const;

    bool valid() const;

    int connect(std::unique_ptr<const sockaddr> remote_addr, socklen_t remote_addr_len) const;

    int bind(std::unique_ptr<const sockaddr> host_addr, socklen_t host_addr_len) const;

    int listen() const;

    int accept(std::unique_ptr<sockaddr> remote_addr, socklen_t* remote_addr_len) const;

    ssize_t send_length(size_t len, int flags) const;

    static ssize_t recv_length(int connfd, size_t* len, int flags);

    ssize_t sendn(const void* data, size_t nbytes, int flags) const;

    static ssize_t recvn(int connfd, void* data, size_t nbytes, int flags);

    int close();

private:
    int socket_;
    int backlog_;
};

int Socket::get_socket() const {
    return socket_;
}

bool Socket::valid() const {
    return socket_ >= 0;
}

int Socket::connect(const std::unique_ptr<const sockaddr> remote_addr,
                    socklen_t remote_addr_len) const {
    return ::connect(socket_, remote_addr.get(), remote_addr_len);
}

int Socket::bind(const std::unique_ptr<const sockaddr> host_addr, socklen_t host_addr_len) const {
    return ::bind(socket_, host_addr.get(), host_addr_len);
}

int Socket::listen() const {
    return ::listen(socket_, backlog_);
}

int Socket::accept(std::unique_ptr<sockaddr> remote_addr, socklen_t* remote_addr_len) const {
    return ::accept(socket_, remote_addr.get(), remote_addr_len);
}

ssize_t Socket::send_length(size_t len, int flags) const {
    return ::send(socket_, &len, sizeof(size_t), 0);
}

ssize_t Socket::recv_length(int connfd, size_t* len, int flags) {
    return ::recv(connfd, len, sizeof(size_t), 0);
}

ssize_t Socket::sendn(const void* data, size_t nbytes, int flags) const {
    size_t left_bytes = nbytes;
    const char* ptr = (char*)data;

    ssize_t sended = 0;
    while (left_bytes > 0) {
        if ((sended = ::send(socket_, ptr, RECV_BUF_SIZE, flags)) <= 0) return -1;
        left_bytes -= sended;
        ptr += sended;
    }
    return nbytes;
}

ssize_t Socket::recvn(int connfd, void* data, size_t nbytes, int flags) {
    size_t left_bytes = nbytes;
    char* ptr = (char*)data;

    ssize_t recved = 0;
    while (left_bytes > 0) {
        if ((recved = ::recv(connfd, ptr, RECV_BUF_SIZE, flags)) < 0) return -1;
        if (recved == 0) break;
        left_bytes -= recved;
        ptr += recved;
    }

    ::close(connfd);
    return nbytes-left_bytes;
}

int Socket::close() {
    int ret = ::close(socket_);
    socket_ = -1;
    return ret;
}
}// namespace mrpc

#endif //RPC_SOCKET_HPP
