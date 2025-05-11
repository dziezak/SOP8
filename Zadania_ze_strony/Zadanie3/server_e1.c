#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUF_SIZE 4
#define BACKLOG 1

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
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

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) die("socket");

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        die("bind");

    if (listen(server_fd, BACKLOG) < 0)
        die("listen");

    printf("[Server] Listening on port %d...\n", port);

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) die("accept");

    char buf[BUF_SIZE + 1] = {0};
    ssize_t total = 0;
    while (total < BUF_SIZE) {
        ssize_t n = read(client_fd, buf + total, BUF_SIZE - total);
        if (n <= 0) die("read");
        total += n;
    }

    printf("Received 4 bytes: ");
    for (int i = 0; i < BUF_SIZE; i++) {
        printf("%02X ", (unsigned char)buf[i]);
    }
    printf("\n");

    close(client_fd);
    close(server_fd);
    return 0;
}
