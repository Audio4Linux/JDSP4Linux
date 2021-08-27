#ifndef SAFECALL_H
#define SAFECALL_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if defined(__GNUC__)
# if defined(__GNUC_PATCHLEVEL__)
#  define __GNUC_VERSION__ (__GNUC__ * 10000 \
                            + __GNUC_MINOR__ * 100 \
                            + __GNUC_PATCHLEVEL__)
# else
#  define __GNUC_VERSION__ (__GNUC__ * 10000 \
                            + __GNUC_MINOR__ * 100)
# endif
#endif

static int safe_write(int fd, const char *buf, size_t len)
{
    while (write(fd, buf, len) == -1 && errno == EINTR) {
        ;
    }
    return len;
}

int safe_printf(int fd, const char *fmt, ...)
{
    int chars = 0;
    const int MAXDIGITS = 32;
    char buf[MAXDIGITS];
    va_list ap;

    va_start(ap, fmt);
    while (*fmt) {
        const char *p = strchr(fmt, '%');
        size_t len = p ? (size_t)(p - fmt) : strlen(fmt);
        chars += safe_write(fd, fmt, len);
        if (p) {
            int width = -1;
            ++p;
            while (*p >= '0' && *p <= '9') {
                width *= (width < 0) ? 0 : 10;
                width += (*p - '0');
                len++;
                p++;
            }
            switch (*p) {
            case 's': {
                const char *s = va_arg(ap, char *);
                chars += safe_write(fd, s, strlen(s));
                len += 2;
                break;
            }
            case 'x': {
                unsigned int n = va_arg(ap, unsigned int);
                int i = MAXDIGITS;
                buf[--i] = 0;
                do {
                    unsigned int digit = (n & 0xf);
                    n >>= 4;
                    buf[--i] = (digit > 9) ? (digit - 10 + 'a') : (digit + '0');
                } while (n || width > MAXDIGITS - i - 1);
                chars += safe_write(fd, buf + i, MAXDIGITS - i - 1);
                len += 2;
                break;
            }
            case 'u': {
                unsigned int n = va_arg(ap, unsigned int);
                int i = MAXDIGITS;
                buf[--i] = 0;
                do {
                    int digit = n % 10;
                    n /= 10;
                    buf[--i] = digit + '0';
                } while (n);
                chars += safe_write(fd, buf + i, MAXDIGITS - i - 1);
                len += 2;
                break;
            }
            default:
                chars += safe_write(fd, p - 1, 1);
                len += 1;
                break;
            }
        }
        fmt += len;
    }
    va_end(ap);
    return chars;
}
static int safe_open_wo_fd(const char * filename){
    int fd = -1;
    do
        fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC | O_SYNC, 0600);
    while (fd == -1 && errno == EINTR);
    return fd;
}
static int safe_open_ro_fd(const char * filename){
    int fd = -1;
    do
        fd = open(filename, O_RDONLY | O_CLOEXEC, 0600);
    while (fd == -1 && errno == EINTR);
    return fd;
}
#endif // SAFECALL_H
