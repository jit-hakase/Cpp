#ifndef UTILS_LINKEDLIST_H
#define UTILS_LINKEDLIST_H

#include <cstdint>

template <typename T>
struct LinkedNode {
    T elem;
    struct LinkedNode *next;
    struct LinkedNode *prev;
};

template <typename T>
class LinkedList {
public:
    void init() {
        m_head = m_tail = nullptr;
        m_size = 0;
    }

    void exit() {
        auto last = m_head;
        while (last) {
            auto node = last->next;
            free(last);
            last = node;
        }
    }

    void put_tail(const T &elem) {
        auto node = (LinkedNode<T>*) malloc(sizeof(LinkedNode<T>));
        node->elem = elem;
        node->next = nullptr;

        if (m_size > 0) {
            m_tail->next = node;
            node->prev = m_tail;
            m_tail = node;
        } else {
            node->prev = nullptr;
            m_tail = m_head = node;
        }
        ++m_size;
    }

    void put_head(const T &elem) {
        auto node = (LinkedNode<T>*) malloc(sizeof(LinkedNode<T>));
        node->elem = elem;
        node->prev = nullptr;

        if (m_size > 0) {
            m_head->prev = node;
            node->next = m_head;
            m_head = node;
        } else {
            node->next = nullptr;
            m_tail = m_head = node;
        }
        ++m_size;
    }

    void del_head() {
        --m_size;
        if (m_size > 0) {
            auto node = m_head->next;
            free(m_head);
            node->prev = nullptr;
            m_head = node;
        } else {
            free(m_head);
            m_head = m_tail = nullptr;
        }
    }

    void del_tail() {
        --m_size;
        if (m_size > 0) {
            auto node = m_tail->prev;
            free(m_tail);
            node->next = nullptr;
            m_tail = node;
        } else {
            free(m_tail);
            m_head = m_tail = nullptr;
        }
    }

    struct LinkedNode<T> *prev(struct LinkedNode<T> *node) const {
        return node->prev;
    }

    struct LinkedNode<T> *next(struct LinkedNode<T> *node) const {
        return node->next;
    }

    struct LinkedNode<T> *head() const {
        return m_head;
    }

    struct LinkedNode<T> *tail() const {
        return m_tail;
    }

    bool empty() const {
        return m_size == 0;
    }

    uint32_t size() const {
        return m_size;
    }

private:
    uint32_t m_size;
    struct LinkedNode<T> *m_head;
    struct LinkedNode<T> *m_tail;
};

#endif //UTILS_LINKEDLIST_H
