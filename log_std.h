#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <mutex>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

__attribute__((format(printf, 4, 5)))
inline void LOG(const char *lvl, const char *file, int line, const char *fmt, ...);

inline void LOG(const char *lvl, const char *file, int line, const char *fmt, ...) {
    static __thread long int tid = 0;
    static std::mutex log_mtx;
    static struct tm time_ds;
    static char msg_buf[1024];
    static time_t cur_sec = 0;
    static struct timeval tv;

    std::lock_guard<std::mutex> lg(log_mtx);
    gettimeofday(&tv, nullptr);
    if (tid == 0) {
        tid = syscall(__NR_gettid);
    }
    if (tv.tv_sec != cur_sec) {
        localtime_r(&cur_sec, &time_ds);
    }
    snprintf(msg_buf, 1024,
             "[%04d-%02d-%02d %02d:%02d:%02d.%06lu][%20s:%04d][%06lu][%3s]: ",
             time_ds.tm_year + 1900, time_ds.tm_mon + 1, time_ds.tm_mday,
             time_ds.tm_hour, time_ds.tm_min, time_ds.tm_sec, tv.tv_usec,
             basename(file), line, tid, lvl);

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf + 70, 954, fmt, args);
    va_end(args);
    printf("%s\n", msg_buf);
}

#define INF(fmt, ...) LOG("INF", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define TRC(fmt, ...) LOG("TRC", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define WRN(fmt, ...) LOG("WRN", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) LOG("ERR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define DIE(fmt, ...) LOG("DIE", __FILE__, __LINE__, fmt, ##__VA_ARGS__), exit(errno)
