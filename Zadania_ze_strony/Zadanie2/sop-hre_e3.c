#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 20
#define BUF_SIZE 128
#define MAX_ELECTORS 7

const char* elector_names[] = {
    "INVALID", "Moguncja", "Trewir", "Kolonia", "Czechy",
    "Palatynat", "Saksonia", "Brandenburgia"
};

typedef struct {
    int fd;
    int identified;     // 0 - nie zidentyfikowany, 1 - tak
    int elector_id;     // 1..7 lub 0
} client_t;

client_t clients[FD_SETSIZE];  // fd-indexed

int elector_fd[MAX_ELECTORS + 1] = {0}; // elektor_id -> fd
int elector_vote[MAX_ELECTORS + 1] = {0}; // elektor_id -> vote (1–3)

void ERR(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) ERR("fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        ERR("fcntl F_SETFL");
}

void remove_client(int fd, int epoll_fd) {
    if (clients[fd].identified) {
        int e = clients[fd].elector_id;
        if (elector_fd[e] == fd) {
            elector_fd[e] = 0;
            elector_vote[e] = 0;
        }
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    memset(&clients[fd], 0, sizeof(client_t));
    printf("[Server] Disconnected client FD %d\n", fd);
}

void handle_client_msg(int fd, int epoll_fd) {
    char buf[BUF_SIZE];
    ssize_t len = recv(fd, buf, BUF_SIZE - 1, 0);
    if (len <= 0) {
        remove_client(fd, epoll_fd);
        return;
    }

    buf[len] = '\0';

    for (int i = 0; i < len; ++i) {
        char ch = buf[i];
        if (!isdigit(ch)) continue;

        if (!clients[fd].identified) {
            int id = ch - '0';
            if (id < 1 || id > 7) {
                printf("[Server] Invalid elector ID from FD %d\n", fd);
                remove_client(fd, epoll_fd);
                return;
            }

            if (elector_fd[id] != 0) {
                printf("[Server] Elector %d already connected.\n", id);
                remove_client(fd, epoll_fd);
                return;
            }

            clients[fd].identified = 1;
            clients[fd].elector_id = id;
            elector_fd[id] = fd;

            char msg[128];
            snprintf(msg, sizeof(msg), "Welcome, elector of %s!\n", elector_names[id]);
            send(fd, msg, strlen(msg), 0);
            printf("[Server] Elector %d connected: %s (FD %d)\n", id, elector_names[id], fd);
        } else {
            int vote = ch - '0';
            if (vote >= 1 && vote <= 3) {
                int e = clients[fd].elector_id;
                elector_vote[e] = vote;
                printf("[Server] Elector %s voted for candidate %d\n", elector_names[e], vote);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) ERR("socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        ERR("bind");

    if (listen(server_fd, SOMAXCONN) < 0)
        ERR("listen");

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) ERR("epoll_create1");

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    printf("[Server] Listening on port %d...\n", port);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            ERR("epoll_wait");
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                set_nonblocking(client_fd);
                clients[client_fd].fd = client_fd;

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                printf("[Server] Client connected: FD %d\n", client_fd);
            } else {
                handle_client_msg(fd, epoll_fd);
            }
        }
    }

    close(server_fd);
    return 0;
}
