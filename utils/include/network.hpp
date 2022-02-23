#ifndef NETWORK_H_
#define NETWORK_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>

#include <iostream>
#include <memory>
#include <string>

// todo: RAII to manage socket
namespace mrpc {
constexpr const size_t RECV_BUF_SIZE = 1024;

int send(const std::string& data, const std::string& ip, uint16_t port) {
    auto remote_addr = std::make_unique<::sockaddr_in>();
    ::inet_pton(AF_INET, ip.c_str(), &(remote_addr->sin_addr));
    remote_addr->sin_family = AF_INET;
    remote_addr->sin_port = htons(port);

    auto host_socket = ::socket(PF_INET, SOCK_STREAM, 0);
    if (host_socket < 0) return -1;

    if (::connect(host_socket, (::sockaddr*) remote_addr.get(), sizeof(*remote_addr)) < 0)
        return -1;
    ::send(host_socket, data.c_str(), data.size(), 0);

    ::close(host_socket);
    return 0;
}

int send_receive(const std::string& request, std::string& response, const std::string& ip,
                 uint16_t port) {
    auto remote_addr = std::make_unique<::sockaddr_in>();
    ::inet_pton(AF_INET, ip.c_str(), &(remote_addr->sin_addr));
    remote_addr->sin_family = AF_INET;
    remote_addr->sin_port = htons(port);

    auto host_socket = ::socket(PF_INET, SOCK_STREAM, 0);
    if (host_socket < 0) return -1;

    if (::connect(host_socket, (::sockaddr*) remote_addr.get(), sizeof(*remote_addr)) < 0)
        return -1;
    ::send(host_socket, request.c_str(), request.size(), 0);

    auto host_addr = std::make_unique<::sockaddr_in>();
    host_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    host_addr->sin_family = AF_INET;
    host_addr->sin_port = htons(0);

    if (::bind(host_socket, reinterpret_cast<::sockaddr*>(host_addr.get()), sizeof(*host_addr)) < 0)
        return -1;
    if (::listen(host_socket, 10) < 0) return -1;

    auto response_addr = std::make_unique<::sockaddr_in>();
    ::socklen_t response_addr_len = 0;
    int connfd;
    if ((connfd = ::accept(host_socket, reinterpret_cast<sockaddr*>(remote_addr.get()),
                           &response_addr_len)) < 0)
        return -1;

    char buf[RECV_BUF_SIZE];
    memset(buf, 0, RECV_BUF_SIZE);

    ssize_t recv_size;
    while ((recv_size = ::recv(connfd, buf, RECV_BUF_SIZE - 1, 0)) > 0) {
        if (recv_size < RECV_BUF_SIZE - 1) buf[recv_size] = 0;
        response.append(buf);
    }
    return 0;
}

int establish(const std::string& ip, ::in_port_t port) {
    auto host_addr = std::make_unique<::sockaddr_in>();
    ::inet_pton(AF_INET, ip.c_str(), &(host_addr->sin_addr));
    host_addr->sin_family = AF_INET;
    host_addr->sin_port = htons(port);

    auto host_socket = ::socket(PF_INET, SOCK_STREAM, 0);
    if (host_socket < 0) return -1;

    if (::bind(host_socket, reinterpret_cast<::sockaddr*>(host_addr.get()), sizeof(*host_addr)) < 0)
        return -1;
    if (::listen(host_socket, 10) < 0) return -1;

    return host_socket;
}

int receive(std::string& data, int local_socket, ::sockaddr* remote_addr,
            ::socklen_t* remote_addr_len) {
    int connfd;
    if ((connfd = ::accept(local_socket, remote_addr, remote_addr_len)) < 0) return -1;

    char buf[RECV_BUF_SIZE];
    memset(buf, 0, RECV_BUF_SIZE);

    ssize_t recv_size;
    while ((recv_size = ::recv(connfd, buf, RECV_BUF_SIZE - 1, 0)) > 0) {
        if (recv_size < RECV_BUF_SIZE - 1) buf[recv_size] = 0;
        data.append(buf);
    }

    return 0;
}
}// namespace mrpc

#endif// NETWORK_H_
