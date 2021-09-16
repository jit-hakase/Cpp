#ifndef UTIL_CONNECTION_H
#define UTIL_CONNECTION_H

#include "log_std.h"

#include <mutex>
#include <queue>
#include <condition_variable>

struct ConnectionInfo {
    std::string host{};
    uint16_t port = 0;
    std::string user{};
    std::string passwd{};
    std::string dbname{};
};

template <typename T>
class ConnectionPool {
public:
    bool init(ConnectionInfo *info, uint32_t size) {
        std::lock_guard<std::mutex> lg(m_mtx);
        for (int idx = 0; idx < size; ++idx) {
            auto conn = new T;
            if (conn->init(info)) {
                m_conns.push(conn);
            }
        }
        TRC("Connection: init connection num[%lu] success", m_conns.size());
        return !m_conns.empty();
    }

    void exit() {
        std::lock_guard<std::mutex> lg(m_mtx);
        while (!m_conns.empty()) {
            auto dba = m_conns.front();
            dba->exit();
            m_conns.pop();
        }
    }

    T* acquire() {
        std::unique_lock<std::mutex> ul(m_mtx);
        for (;;) {
            if (!m_conns.empty()) {
                auto dba = m_conns.front();
                m_conns.pop();
                ul.unlock();
                return dba;
            }
            WRN("Connection: wait to acquire...");
            m_cv.wait(ul);
        }
    }

    void release(T *dba) {
        {
            std::lock_guard<std::mutex> lg(m_mtx);
            m_conns.push(dba);
        }
        m_cv.notify_one();
    }

private:
    std::queue<T*> m_conns;
    std::mutex m_mtx;
    std::condition_variable m_cv;
};

template <typename T>
class ConnectionAdapter {
public:
    explicit ConnectionAdapter() {
        m_pool = &Singleton<ConnectionPool<T>>::instance();
        m_dba = m_pool->acquire();
    }

    ~ConnectionAdapter() {
        m_pool->release(m_dba);
    }

    T& set_sql(const std::string &sql) {
        return m_dba->set_sql(sql);
    }

private:
    ConnectionPool<T> *m_pool = nullptr;
    T *m_dba = nullptr;
};

#endif  // UTIL_CONNECTION_H
