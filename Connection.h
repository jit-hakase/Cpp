#ifndef UTIL_CONNECTION_H
#define UTIL_CONNECTION_H

#include "Singleton.h"
#include "log_std.h"

struct ConnectionInfo {
    std::string host;
    uint16_t port = 0;
    std::string user;
    std::string passwd;
    std::string dbname;
};

template <typename T>
class ConnectionPool {
public:
    bool init(ConnectionInfo *info, uint32_t size) {
        std::lock_guard<std::mutex> lg(m_dba_mtx);
        for (int idx = 0; idx < size; ++idx) {
            auto conn = new T;
            if (conn->init(info)) {
                m_dbas.push(conn);
            }
        }
        SQL("ConnectionPool: init connection num[%lu] success", m_dbas.size());
        return !m_dbas.empty();
    }

    void exit() {
        std::lock_guard<std::mutex> lg(m_dba_mtx);
        while (!m_dbas.empty()) {
            auto dba = m_dbas.front();
            dba->exit();
            m_dbas.pop();
        }
    }

    T* acquire() {
        re:
        {
            std::lock_guard<std::mutex> lg(m_dba_mtx);
            if (!m_dbas.empty()) {
                auto dba = m_dbas.front();
                m_dbas.pop();
                return dba;
            }
        }
        SQL("MySQLConnectionPool: adapter get failed");
        sleep(2);
        goto re;
    }

    void release(T *dba) {
        std::lock_guard<std::mutex> lg(m_dba_mtx);
        m_dbas.push(dba);
    }

private:
    std::queue<T*> m_dbas;
    std::mutex m_dba_mtx;
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
