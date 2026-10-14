#ifndef NETWORK_LABS_IO_HELPERS_H
#define NETWORK_LABS_IO_HELPERS_H
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

// TCP 是位元組串流，一次 read/write 不保證完成整個欄位。
inline bool read_exact(int fd, void* data, size_t length) {
    char* p = static_cast<char*>(data);
    while (length > 0) {
        const ssize_t n = read(fd, p, length);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        p += n;
        length -= static_cast<size_t>(n);
    }
    return true;
}
inline bool write_all(int fd, const void* data, size_t length) {
    const char* p = static_cast<const char*>(data);
    while (length > 0) {
        const ssize_t n = write(fd, p, length);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        p += n;
        length -= static_cast<size_t>(n);
    }
    return true;
}
#endif
