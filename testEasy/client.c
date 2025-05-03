#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define PORT "12345"
#define BUF_SIZE 256

int main() {
    int sock;
    struct addrinfo hints = {}, *res;
    char buffer[BUF_SIZE];

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("127.0.0.1", PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("[Klient] Połączono z serwerem.\n");
    printf("[Klient] Podaj wiadomość (Ctrl+D aby zakończyć):\n");

    while (fgets(buffer, BUF_SIZE, stdin)) {
        buffer[strlen(buffer)-1] = '\0';
        if (write(sock, buffer, strlen(buffer)) < 0) {
            perror("write");
            break;
        }
    }

    close(sock);
    freeaddrinfo(res);
    return 0;
}
