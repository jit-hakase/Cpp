//
// Created by longx on 6/22/21.
//

#ifndef UTILS_EVENTLOOP_H
#define UTILS_EVENTLOOP_H

#include "Buffer.h"
#include "TcpServer.h"
#include "Timer.h"

#include <map>
#include <sys/epoll.h>
#include <functional>

#include <unistd.h>
#include "log_std.h"

enum {
    EPOLL_STATUS_READING,
    EPOLL_STATUS_WRITING
};

class EventLoop {
public:
    bool init(uint16_t port, int epsz) {
        m_epee = (struct epoll_event*) calloc(
                epsz + 1, sizeof(struct epoll_event));
        m_epsz = epsz;
        m_ts.init(port, true);

        m_epfd = epoll_create(1024);
        if (-1 == m_epfd) return false;
        m_svr_fd = m_ts.get_sock_fd();
        return add_event(m_svr_fd);
    }

    void on_connect(std::function<void(int)> &&init_func) {
        m_init_func = init_func;
    }

    void on_message(std::function<void(int, Buffer*)> &&recv_func) {
        m_recv_func = recv_func;
    }

    void on_disconnect(std::function<void(int)> &&exit_func) {
        m_exit_func = exit_func;
    }

    void loop() {
        constexpr uint16_t max_read_buf_size = 512u;
        uint8_t tmp_buf[max_read_buf_size];
        auto rdy_num = epoll_wait(m_epfd, m_epee, m_epsz, 0);
        if (-1 == rdy_num) {
            SYS("epoll_wait return errno[%d]", errno);
        }
        for (auto idx = 0; idx < rdy_num; ++idx) {
            auto fd = m_epee[idx].data.fd;
            auto evt = m_epee[idx].events;
            if ((evt & (EPOLLERR | EPOLLHUP)) && !(evt & EPOLLIN)) {
                SYS("epoll_wait event error fd[%d] evt[%d] errno[%d]", fd, evt, errno);
                on_close(fd);
                continue;
            }

            if (evt & EPOLLIN) {
                // Acceptor
                if (fd == m_svr_fd) {
                    on_accept();
                    continue;
                }
                // Data
                auto n = recv(fd, tmp_buf, max_read_buf_size, MSG_DONTWAIT);
                if (n == 0) {
                    on_close(fd);
                    continue;
                } else if (n < 0) {
                    if (errno == EAGAIN) {
                        SYS("recv empty message error[%d]", errno);
                        continue;
                    }
                    on_close(fd);
                    continue;
                } else {
                    recv_epollin(fd, tmp_buf, n);
                }
            }
            // EPOLLOUT
            if (evt & EPOLLOUT) {
                send_epollout(fd);
            }
        }
    }

    void broadcast(std::function<void(int)> &&func) {
        for (auto &&e : m_info_map) {
            func(e.first);
        }
    }

    bool send_data(int fd, void *buf, uint32_t size) {
        auto info = &m_info_map[fd];
        auto wbuf = info->wbuf;

        if (info->stat == EPOLL_STATUS_WRITING) {
            if (wbuf->remain() < size) {
                SYS("send overflow fd[%d]", fd);
                on_close(fd);
                return false;
            } else {
                wbuf->put((uint8_t*) buf, size);
                return true;
            }
        } else if (info->stat == EPOLL_STATUS_READING) {
            auto ret = ::send(fd, buf, size, MSG_DONTWAIT);
            if (ret == -1) {
                if (errno == EAGAIN) {
                    if (wbuf->remain() < size) {
                        SYS("no avail resource for fd[%d]", fd);
                        on_close(fd);
                        return false;
                    } else {
                        wbuf->put((uint8_t*) buf, size);
                        if (r2w_event(fd)) {
                            info->stat = EPOLL_STATUS_WRITING;
                            return true;
                        } else {
                            on_close(fd);
                            return false;
                        }
                    }
                } else {
                    if (errno != ECONNRESET) {
                        SYS("fd[%d] send occur error", fd);
                    }
                    on_close(fd);
                    return false;
                }
            } else if (ret == size) {
                return true;
            } else if (ret != size) {
                wbuf->put((uint8_t*)buf + ret, size - ret);
                return true;
            } else {
                SYS("known errno[%d]", errno);
                return false;
            }
        } else {
            SYS("status[%d] error", info->stat);
            return false;
        }
    }

    void force_close_connection(int fd) {
        on_close(fd);
    }

private:
    void init_info(int fd) {
        const auto max_buf_size = 65535u;
        event_loop_info_t info{};
        info.rbuf = new Buffer(max_buf_size);
        info.wbuf = new Buffer(max_buf_size);
        info.stat = EPOLL_STATUS_READING;
        m_info_map[fd] = info;
    }

    void exit_info(int fd) {
        delete m_info_map[fd].rbuf;
        delete m_info_map[fd].wbuf;
        m_info_map.erase(fd);
    }

    void on_accept() {
        auto clt_fd = m_ts.accept();
        if (clt_fd != -1) {
            if (add_event(clt_fd)) {
                init_info(clt_fd);
                m_init_func(clt_fd);
            }
        } else {
            SYS("accept errno[%d]", errno);
        }
    }

    void on_close(int fd) {
        m_exit_func(fd);
        (void) del_event(fd);
        exit_info(fd);
        close(fd);
    }

    bool add_event(int fd) const {
        struct epoll_event ee{};
        ee.data.fd = fd;
        ee.events = EPOLLIN;
        auto ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ee);
        if (-1 == ret) {
            SYS("epoll_ctl add error fd[%d] errno[%d]", fd, errno);
            return false;
        }
        return true;
    }

    bool del_event(int fd) const {
        struct epoll_event ee{};
        ee.data.fd = fd;
        ee.events = 0;
        auto ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &ee);
        if (-1 == ret) {
            SYS("epoll_ctl del error fd[%d] errno[%d]", fd, errno);
            return false;
        }
        return true;
    }

    void recv_epollin(int fd, uint8_t *tmp_buf, uint32_t size) {
        auto buf = m_info_map[fd].rbuf;
        if (buf->remain() >= size) {
            buf->put((uint8_t*)tmp_buf, size);
            m_recv_func(fd, m_info_map[fd].rbuf);
        } else {
            SYS("read buf has no enough space fd[%d] errno[%d]", fd, errno);
            on_close(fd);
        }
    }

    void send_epollout(int fd) {
        auto info = &m_info_map[fd];
        auto wbuf = info->wbuf;
        auto size = wbuf->avail();
        uint8_t tmp_buf[size];
        wbuf->get((uint8_t*) tmp_buf, size);

        auto ret = ::send(fd, tmp_buf, size, MSG_DONTWAIT);
        if (ret == -1) {
            if (errno == EAGAIN) {
                wbuf->put((uint8_t*) tmp_buf, size);
            } else {
                SYS("fd[%d] send occur error (epollout) errno[%d]", fd, errno);
                on_close(fd);
            }
        } else if (ret != size) {
            wbuf->put(tmp_buf + ret, size - ret);
        } else if (ret == size) {
            if (w2r_event(fd)) {
                info->stat = EPOLL_STATUS_READING;
            } else {
                on_close(fd);
            }
        }
    }

    bool r2w_event(int fd) const {
        struct epoll_event ee{};
        ee.data.fd = fd;
        ee.events = EPOLLIN | EPOLLOUT;
        auto ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ee);
        if (-1 == ret) {
            SYS("epoll_ctl r2w error fd[%d] errno[%d]", fd, errno);
            return false;
        }
        return true;
    }

    bool w2r_event(int fd) const {
        struct epoll_event ee{};
        ee.data.fd = fd;
        ee.events = EPOLLIN;
        auto ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ee);
        if (-1 == ret) {
            SYS("epoll_ctl w2r error fd[%d] errno[%d]", fd, errno);
            return false;
        }
        return true;
    }

    typedef struct {
        Buffer *rbuf;
        Buffer *wbuf;
        int stat;
    } event_loop_info_t;

    int m_epfd = -1;
    int m_epsz = 0;
    struct epoll_event *m_epee = nullptr;

    int m_svr_fd = 0;
    TcpServer m_ts{};

    std::function<void(int)> m_init_func = nullptr;
    std::function<void(int)> m_exit_func = nullptr;
    std::function<void(int, Buffer*)> m_recv_func = nullptr;

    std::map<int, event_loop_info_t> m_info_map{};
};

#endif //UTILS_EVENTLOOP_H
