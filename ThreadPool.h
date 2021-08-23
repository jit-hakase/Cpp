#ifndef UTILS_THREADPOOL_H
#define UTILS_THREADPOOL_H

#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    void init(uint32_t size) {
        m_size = size;
        for (int idx = 0; idx < size; ++idx) {
            m_thrds.emplace_back(std::thread(std::bind(&ThreadPool::exec_task, this)));
        }
    }

    void exit() {
        {
            std::lock_guard<std::mutex> lg(m_mtx);
            m_is_stop = true;
        }

        for (auto &&thrd : m_thrds) {
            if (thrd.joinable()) {
                thrd.join();
            }
        }
    }

    void add_task(std::function<void()> &&task) {
        if (m_size == 0) {
            task();
            return;
        }

        {
            std::lock_guard<std::mutex> lg(m_mtx);
            m_tasks.emplace(task);
        }
        m_cond.notify_one();
    }

    void exec_task() {
        std::unique_lock<std::mutex> ul(m_mtx);
        for (;;) {
            if (!m_tasks.empty()) {
                auto task = m_tasks.front();
                m_tasks.pop();
                ul.unlock();
                task();
                ul.lock();
            } else if (m_is_stop) {
                break;
            } else {
                m_cond.wait(ul);
            }
        }
    }

private:
    bool m_is_stop = false;
    uint32_t m_size = 0;
    std::mutex m_mtx;
    std::queue<std::function<void()>> m_tasks;
    std::vector<std::thread> m_thrds;
    std::condition_variable m_cond;
};

#endif //UTILS_THREADPOOL_H
