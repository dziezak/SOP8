#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define PORT 12345

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

volatile sig_atomic_t server_running = 1;

void sigint_handler(int sig) {
    server_running = 0;
    printf("\n[Serwer] Przechwycono SIGINT, zamykanie serwera.\n");
}

int make_tcp_socket(void) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) ERR("socket");
    return sock;
}

int bind_tcp_socket(uint16_t port, int backlog_size) {
    struct sockaddr_in addr;
    int socketfd, t = 1;
    socketfd = make_tcp_socket();
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("setsockopt");
    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("bind");
    if (listen(socketfd, backlog_size) < 0)
        ERR("listen");
    return socketfd;
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

int add_new_client(int sfd) {
    int nfd;
    if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        ERR("accept");
    }
    return nfd;
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

void run_server() {
    int server_fd = bind_tcp_socket(PORT, SOMAXCONN); // gniazdo
    int epoll_fd = epoll_create1(0); // mechanizm monitorowaniu wielu fd
    if (epoll_fd == -1) ERR("epoll_create1");

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
        ERR("epoll_ctl");

    signal(SIGINT, sigint_handler);
    printf("[Serwer] Nasłuchiwanie na porcie %d...\n", PORT);

    while (server_running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue; // Obsługa sygnału
            ERR("epoll_wait");
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                int client_fd = add_new_client(server_fd);
                if (client_fd >= 0) {
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                        ERR("epoll_ctl: client_fd");
                    printf("[Serwer] Nowe połączenie: fd=%d\n", client_fd);
                }
            } else {
                char buffer[1024];
                ssize_t bytes = bulk_read(events[i].data.fd, buffer, sizeof(buffer) - 1);
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    printf("[Serwer] Odebrano: %s", buffer);
                    bulk_write(events[i].data.fd, buffer, bytes); // Echo
                } else {
                    printf("[Serwer] Zamknięcie połączenia: fd=%d\n", events[i].data.fd);
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                }
            }
        }
    }
    close(server_fd);
    close(epoll_fd);
}

void run_client() {
    int retries = 5;
    int sock;
    while (retries--) {
        sock = connect_tcp_socket("127.0.0.1", "12345");
        if (sock >= 0) break;
        sleep(1);
    }
    if (sock < 0) ERR("Nie udało się połączyć z serwerem");

    char msg[1024], buf[1024];
    while (1) {
        printf("[Klient] Wpisz wiadomość ('exit' aby zakończyć): ");
        if (!fgets(msg, sizeof(msg), stdin)) break;

        if (strcmp(msg, "exit\n") == 0) break;

        if (bulk_write(sock, msg, strlen(msg)) < 0) {
            fprintf(stderr, "[Klient] Błąd zapisu.\n");
            break;
        }

        ssize_t len = bulk_read(sock, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            printf("[Klient] Odpowiedź serwera: %s", buf);
        } else {
            fprintf(stderr, "[Klient] Serwer się rozłączył.\n");
            break;
        }
    }

    close(sock);
}

int main() {
    pid_t pid = fork();
    if (pid == -1) {
        ERR("fork");
    }
    if (pid == 0) {
        sleep(1); // Daj serwerowi czas na start
        run_client();
    } else {
        run_server();
    }
    return 0;
}

