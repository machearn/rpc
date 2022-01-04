#ifndef NETWORK_H_
#define NETWORK_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <string>

namespace mrpc {
constexpr const size_t RECV_BUF_SIZE = 4096;
void client(const std::string& data, const std::string& ip, uint32_t port) {
    auto remote_socket = std::make_unique<::sockaddr_in>();
    ::inet_pton(AF_INET, ip.c_str(), &(remote_socket->sin_addr));
    remote_socket->sin_family = AF_INET;
    remote_socket->sin_port = htonl(port);

    auto socket_fp = ::socket(PF_INET, SOCK_STREAM, 0);
    assert(socket_fp);

    if (::connect(socket_fp, (::sockaddr*) remote_socket.get(), sizeof(*remote_socket)) < 0) {
        std::cout << "Failed connect to server\n";
    } else {
        ::send(socket_fp, data.c_str(), data.size(), 0);
    }
}

int server(std::string& data, const std::string& ip, uint32_t port) {
    auto host_socket = std::make_unique<::sockaddr_in>();
    ::inet_pton(AF_INET, ip.c_str(), &(host_socket->sin_addr));
    host_socket->sin_family = AF_INET;
    host_socket->sin_port = htonl(port);

    auto socket_fp = ::socket(PF_INET, SOCK_STREAM, 0);
    assert(socket_fp);

    int ret = ::bind(socket_fp, (sockaddr*) host_socket.get(), sizeof(*host_socket));
    assert(ret != -1);
    ret = ::listen(socket_fp, 5);
    assert(ret != -1);

    std::unique_ptr<::sockaddr_in> remote_socket(new ::sockaddr_in());
    ::socklen_t remote_socket_len = sizeof(*remote_socket);

    int connfd = ::accept(socket_fp, (::sockaddr*) remote_socket.get(), &remote_socket_len);

    if (connfd < 0) {
        std::cout << "Failed to build listening connection\n";
        return -1;
    } else {
        char buf[RECV_BUF_SIZE];
        std::memset(buf, '\0', RECV_BUF_SIZE);

        ::recv(connfd, buf, RECV_BUF_SIZE, 0);
        data = std::string(buf);
        return 0;
    }
}
}// namespace mrpc

#endif// NETWORK_H_
