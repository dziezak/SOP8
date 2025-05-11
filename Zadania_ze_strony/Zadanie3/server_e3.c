#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#define MAX_EVENTS 10
#define MAX_CLIENTS 4
#define BUF_SIZE 4

#define ERR(source) (perror(source), exit(EXIT_FAILURE))

int client_fds[MAX_CLIENTS];
int client_count = 0;
char cities[21]; // miasta sa 1-20

void add_client(int fd){
    if(client_count < MAX_CLIENTS){
        client_fds[client_count++] = fd;
    }
}

bool is_full(){
    return client_count >= MAX_CLIENTS;
}

void broadcast(int sender_fd, const char *msg, size_t len){
    for(int i=0; i<client_count; i++){
        int fd = client_fds[i];
        if(fd != sender_fd)
            if(write(fd, msg, len) < 0)
                perror("broadcast write");
    }
}

void initialize_cities(){
    for(int i=1; i<=20; i++){
        cities[i] = 'G';
    }
}

int parse_city_number(const char *buf){
    if(strlen(buf) < 3) return -1;
    int num = (buf[1]-'0') * 10 + (buf[2]-'0');
    if(num < 1 || num > 20) return -1;
    return num;
}

void handle_command(int client_fd, const char *buf, ssize_t len) {
    if (len < 4 || buf[3] != '\n') {
        //printf("[Server] Ignoring malformed input from fd=%d\n", client_fd);
        return;
    }

    char side = buf[0]; // 'g' or 'p'
    int city = parse_city_number(buf);
    if (city == -1) {
        printf("[Server] Invalid city number in message: %.*s", (int)len, buf);
        return;
    }

    char new_owner = (side == 'g') ? 'G' : 'P';
    if (cities[city] != new_owner) {
        cities[city] = new_owner;
        printf("[Server] City %d is now controlled by %s\n", city, new_owner == 'G' ? "Greeks" : "Persians");
        broadcast(client_fd, buf, len);
    } else {
        printf("[Server] No change for city %d\n", city);
    }
}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number.\n");
        exit(EXIT_FAILURE);
    }

    initialize_cities();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) ERR("socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        ERR("bind");

    if (listen(server_fd, SOMAXCONN) < 0)
        ERR("listen");

    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) ERR("epoll_create1");

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
        ERR("epoll_ctl");

    printf("[Server] Listening on port %d...\n", port);

    while(1){
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if(nfds < 0) ERR("epoll_wait");
        for(int i=0; i<nfds; i++){
            int fd = events[i].data.fd;

            if(fd == server_fd){
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd < 0){
                    perror("accept");
                    continue;
                }
                if(is_full()){
                    printf("[Server] Max clients reached. Rejecting client.\n");
                    close(client_fd);
                }else{
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0){
                        ERR("epoll_ctl: client_fd");
                        close(client_fd);
                        continue;
                    }
                    add_client(client_fd);
                    printf("[Server] New client connected. FD=%d\n", client_fd);
                }
            }else{
                char buf[BUF_SIZE];
                ssize_t bytes_read = read(fd, buf, BUF_SIZE);
                if(bytes_read = BUF_SIZE)
                    handle_command(fd, buf, bytes_read);
            }
        }
    }
    close(server_fd);
    return 0;
}