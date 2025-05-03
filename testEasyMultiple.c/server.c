#define _GNU_SOURCE
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 12345
#define MAX_EVENTS 10
#define MAX_CLIENTS 100
#define BUF_SIZE 1024
#define ERR(source) (perror(source), exit(EXIT_FAILURE))

int clients[MAX_CLIENTS];
int client_count = 0;

void broadcast(int sender_fd, char *msg, size_t len) {
    for (int i = 0; i < client_count; i++) {
        int fd = clients[i];
        if (fd != sender_fd) {
            if (write(fd, msg, len) < 0) {
                perror("broadcast write");
            }
        }
    }
}

void remove_client(int fd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == fd) {
            clients[i] = clients[--client_count];
            break;
        }
    }
    close(fd);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) ERR("socket");

    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("bind");

    if (listen(server_fd, SOMAXCONN) < 0)
        ERR("listen");

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) ERR("epoll_create");

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
        ERR("epoll_ctl");

    printf("[Server] Listening on port %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) ERR("epoll_wait");

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd < 0) perror("accept");
                else {
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                    clients[client_count++] = client_fd;
                    printf("[Server] New client fd=%d\n", client_fd);
                }
            } else {
                char buf[BUF_SIZE];
                ssize_t count = read(events[i].data.fd, buf, sizeof(buf));
                if (count <= 0) {
                    printf("[Server] Client disconnected fd=%d\n", events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    remove_client(events[i].data.fd);
                } else {
                    buf[count] = '\0';
                    printf("[Server] Message from fd=%d: %s", events[i].data.fd, buf);
                    broadcast(events[i].data.fd, buf, count);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
