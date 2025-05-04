// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>

#define PORT 2000
#define BUF_SIZE 64

int sum_of_digits(const char *pid_str) {
    int sum = 0;
    for (int i = 0; pid_str[i]; i++) {
        if (pid_str[i] >= '0' && pid_str[i] <= '9')
            sum += pid_str[i] - '0';
    }
    return sum;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        char buf[BUF_SIZE];
        ssize_t len = read(client_fd, buf, BUF_SIZE - 1);
        if (len > 0) {
            buf[len] = '\0';
            printf("[Server] Received PID: %s\n", buf);

            int sum = sum_of_digits(buf);
            int16_t sum16 = (int16_t)sum;
            write(client_fd, &sum16, sizeof(sum16));
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
