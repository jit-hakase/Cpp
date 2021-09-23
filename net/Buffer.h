#ifndef UTILS_BUFFER_H
#define UTILS_BUFFER_H

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>

class Buffer {
public:
    explicit Buffer(uint16_t size) :
        m_size(size),
        m_get_idx(0), m_put_idx(0),
        m_buf((uint8_t*) malloc(size)) {}

    ~Buffer() {
        free(m_buf);
    }

    uint16_t avail() const {
        return m_put_idx - m_get_idx;
    }

    uint16_t remain() const {
        return (uint16_t) (m_size - m_put_idx);
    }

    void shrink() {
        if (m_get_idx == m_put_idx) {
            m_get_idx = m_put_idx = 0;
        } else {
            auto ava = avail();
            memmove(m_buf, m_buf + m_get_idx, ava);
            m_get_idx = 0;
            m_put_idx = ava;
        }
    }

    // must call avail before peek, get, put.
    void peek(void *buf, uint16_t size) const {
        memcpy(buf, m_buf + m_get_idx, size);
    }

    void get(uint8_t *buf, uint16_t size) {
        assert(avail() >= size);
        memmove(buf, m_buf + m_get_idx, size);
        m_get_idx += size;
        if (m_get_idx == m_put_idx) {
            m_get_idx = m_put_idx = 0;
        }
    }

    void put(uint8_t *buf, uint16_t size) {
        assert(remain() >= size);
        memcpy(m_buf + m_put_idx, buf, size);
        m_put_idx += size;
    }

private:
    uint8_t *m_buf;
    uint16_t m_size;
    uint16_t m_get_idx;
    uint16_t m_put_idx;
};

#endif //UTILS_BUFFER_H
