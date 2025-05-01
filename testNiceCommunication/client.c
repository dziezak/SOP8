#define _GNU_SOURCE
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT "12345"
#define ERR(source) (perror(source), fprintf(stderr,"%s:%d\n",__FILE__,__LINE__), exit(EXIT_FAILURE))

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)             \
    (__extension__({                               \
        long int __result;                         \
        do                                         \
            __result = (long int)(expression);     \
        while (__result == -1L && errno == EINTR); \
        __result;                                  \
    }))
#endif

int make_tcp_socket(void) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) ERR("socket");
    return sock;
}

int connect_tcp_socket(char *name, char *port) {
    struct addrinfo hints = {}, *result;
    hints.ai_family = AF_INET;
    int ret = getaddrinfo(name, port, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }
    int socketfd = make_tcp_socket();
    if (connect(socketfd, result->ai_addr, result->ai_addrlen) < 0)
        ERR("connect");
    freeaddrinfo(result);
    return socketfd;
}

ssize_t bulk_read(int fd, char *buf, size_t count) {
    int c;
    size_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0) return c;
        if (c == 0) return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count) {
    int c;
    size_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0) return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

int main() {
    int sock = connect_tcp_socket("127.0.0.1", PORT);
    char msg[] = "Cześć serwerze!";
    bulk_write(sock, msg, strlen(msg));
    char buf[128];
    ssize_t len = bulk_read(sock, buf, sizeof(buf) - 1);
    if (len >= 0) {
        buf[len] = '\0';
        printf("[Klient] Odpowiedź serwera: %s\n", buf);
    }
    close(sock);
    return 0;
}
