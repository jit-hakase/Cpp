#ifndef SPSCQueue_H
#define SPSCQueue_H

#include <cstring>
#include <cstdint>
#include <atomic>

template <typename T, int SIZE, int MASK = SIZE - 1>
class alignas(128) SPSCQueue {
public:
    static_assert(SIZE && !(SIZE & (SIZE - 1)), "SIZE");
    void put(T *d) {
        uint_fast64_t seq;
        re:
        seq = tail;
        if (seq < (head.load(std::memory_order_acquire) + SIZE)) {
            buf[seq & MASK] = *d;
            tail.store(tail + 1, std::memory_order_release);
        } else {
            goto re;
        }
    }

    void get(T *d) {
        uint_fast64_t seq;
        re:
        seq = head;
        if (seq < tail.load(std::memory_order_acquire)) {
            *d = buf[seq & MASK];
            head.store(head + 1, std::memory_order_release);
        } else {
            goto re;
        }
    }

private:
    alignas(128) std::atomic<uint64_t> head;
    alignas(128) std::atomic<uint64_t> tail;
    alignas(128) T buf[SIZE];
};

#endif
