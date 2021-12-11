#ifndef NETWORK_H_
#define NETWORK_H_

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <memory>

namespace mrpc {
    constexpr const size_t RECV_BUF_SIZE = 4096;
    void transit(const std::string& data, const std::string& ip, uint32_t port) {
        std::unique_ptr<::sockaddr_in> server_socket(new ::sockaddr_in());
        ::inet_pton(AF_INET, ip.c_str(), &(server_socket->sin_addr));
        server_socket->sin_family = AF_INET;
        server_socket->sin_port = htonl(port);

        auto socket_fp = ::socket(PF_INET, SOCK_STREAM, 0);
        assert(socket_fp);

        if (::connect(socket_fp, (::sockaddr*)server_socket.get(), sizeof(*server_socket)) < 0) {
            std::cout << "Failed connect to server\n";
        } else {
            ::send(socket_fp, data.c_str(), data.size(), 0);
        }
    }

    int waiting(const std::string& ip, uint32_t port, std::string& data) {
        std::unique_ptr<::sockaddr_in> listen_socket(new ::sockaddr_in());
        ::inet_pton(AF_INET, ip.c_str(), &(listen_socket->sin_addr));
        listen_socket->sin_family = AF_INET;
        listen_socket->sin_port = htonl(port);

        auto socket_fp = ::socket(PF_INET, SOCK_STREAM, 0);
        assert(socket_fp);

        int ret = bind(socket_fp, (sockaddr*)listen_socket.get(), sizeof(*listen_socket));
        assert(ret != -1);
        ret = listen(socket_fp, 5);
        assert(ret != -1);

        std::unique_ptr<::sockaddr_in> server_socket(new ::sockaddr_in());
        socklen_t server_socket_len = sizeof(*server_socket);

        int connfd = ::accept(socket_fp, (sockaddr*)server_socket.get(), &server_socket_len);

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
}

#endif // NETWORK_H_
