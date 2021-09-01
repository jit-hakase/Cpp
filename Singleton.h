#ifndef UTILS_SINGLETON_H
#define UTILS_SINGLETON_H

template <typename T>
class Singleton {
public:
    static T& instance() {
        static T obj;
        return obj;
    }
};

#endif
