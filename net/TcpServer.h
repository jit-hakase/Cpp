//
// Created by longx on 6/22/21.
//

#ifndef UTILS_TCPSERVER_H
#define UTILS_TCPSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

class TcpServer {
public:
    explicit TcpServer() = default;
    ~TcpServer() = default;

    bool init(uint16_t port, bool nonblock=false) {
        m_nonblock_mode = nonblock;
        int sock_type = SOCK_STREAM | SOCK_CLOEXEC;
        if (m_nonblock_mode) {
            sock_type |= SOCK_NONBLOCK;
        }
        m_sock_fd = socket(PF_INET, sock_type, IPPROTO_TCP);
        if (-1 == m_sock_fd) return false;

        const int on = 1;
        auto ret = setsockopt(m_sock_fd, SOL_TCP, TCP_NODELAY, &on, sizeof(on));
        if (-1 == ret) return false;
        ret = setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (-1 == ret) return false;

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        ret = bind(m_sock_fd, (const struct sockaddr*) &addr, sizeof(addr));
        if (-1 == ret) return false;
        ret = listen(m_sock_fd, SOMAXCONN);
        return -1 != ret;
    }

    int accept() const {
        if (m_nonblock_mode) {
            return ::accept4(m_sock_fd, nullptr, nullptr, SOCK_NONBLOCK);
        } else {
            return ::accept(m_sock_fd, nullptr, nullptr);
        }
    }

    int get_sock_fd() const {
        return m_sock_fd;
    }

private:
    int m_sock_fd;
    bool m_nonblock_mode;
};

#endif//UTILS_TCPSERVER_H
