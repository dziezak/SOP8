#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 10
#define BUF_SIZE 1024
#define WELCOME_MSG "Welcome, elector!\n"

void ERR(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) ERR("fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        ERR("fcntl F_SETFL");
    return fd;
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
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
        ERR("epoll_ctl: listen_sock");

    printf("[Server] Listening on port %d...\n", port);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            ERR("epoll_wait");
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                // New connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                set_nonblocking(client_fd);

                ev.events = EPOLLIN | EPOLLET;  // Edge-triggered
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                    ERR("epoll_ctl: client");

                send(client_fd, WELCOME_MSG, strlen(WELCOME_MSG), 0);
                printf("[Server] Client connected: FD %d\n", client_fd);
            } else {
                // Client message
                char buf[BUF_SIZE];
                ssize_t len = recv(events[i].data.fd, buf, sizeof(buf) - 1, 0);
                if (len <= 0) {
                    if (len == 0) {
                        printf("[Server] Client disconnected: FD %d\n", events[i].data.fd);
                    } else {
                        perror("recv");
                    }
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                } else {
                    buf[len] = '\0';
                    printf("[Server][FD %d] Message: %s", events[i].data.fd, buf);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
