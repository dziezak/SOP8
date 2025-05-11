#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define BUF_SIZE 64

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <SERVER_IP> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number.\n");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) die("socket");

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
        die("inet_pton");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        die("connect");

    printf("[Client] Connected to server at %s:%d\n", server_ip, port);
    printf("[Client] Enter commands like 'p07' or 'g12' and press Enter.\n");

    fd_set readfds;
    char buf[BUF_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        int ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ready < 0) die("select");

        // User input
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (!fgets(buf, sizeof(buf), stdin)) {
                printf("[Client] EOF or error on stdin.\n");
                break;
            }

            size_t len = strlen(buf);
            if (len > 0 && buf[len - 1] != '\n') {
                fprintf(stderr, "[Client] Input too long. Max %d chars.\n", BUF_SIZE - 2);
                // Flush stdin
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                continue;
            }

            if (write(sockfd, buf, len) != len) {
                perror("write");
                break;
            }
        }

        // Server message
        if (FD_ISSET(sockfd, &readfds)) {
            ssize_t n = read(sockfd, buf, sizeof(buf) - 1);
            if (n <= 0) {
                printf("[Client] Disconnected from server.\n");
                break;
            }
            buf[n] = '\0';
            printf("[Client] Update from server: %s", buf);
        }
    }

    close(sockfd);
    return 0;
}
