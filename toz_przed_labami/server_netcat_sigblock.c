#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#define MAX_EVENTS 10
#define BUF_SIZE 1024

#define ERR(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) ERR("fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        ERR("fcntl F_SETFL O_NONBLOCK");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    // Ignorujemy SIGPIPE (lepiej niż ginąć)
    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) ERR("socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd); // tryb nieblokujący

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        ERR("bind");

    if (listen(server_fd, SOMAXCONN) < 0)
        ERR("listen");

    int epfd = epoll_create1(0);
    if (epfd < 0) ERR("epoll_create1");

    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.fd = server_fd
    };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
        ERR("epoll_ctl");

    printf("[Server] Listening on port %d...\n", port);

    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds < 0) ERR("epoll_wait");

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }

                set_nonblocking(client_fd);

                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
                printf("[Server] New client connected: fd=%d\n", client_fd);

            } else {
                char buf[BUF_SIZE];
                ssize_t count = read(fd, buf, sizeof(buf) - 1);

                if (count <= 0) {
                    printf("[Server] Client disconnected: fd=%d\n", fd);
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                } else {
                    buf[count] = '\0';
                    printf("[Client %d] %s", fd, buf);

                    // Echo message back (optional)
                    ssize_t sent = send(fd, buf, count, MSG_NOSIGNAL);
                    if (sent < 0) {
                        if (errno == EPIPE) {
                            printf("[Server] EPIPE while sending to client %d (broken pipe)\n", fd);
                            close(fd);
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            perror("send");
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
