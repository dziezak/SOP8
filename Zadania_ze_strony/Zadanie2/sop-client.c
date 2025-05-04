#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s <host> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *host = argv[1];
    int port = atoi(argv[2]);

    // 1. Tworzenie gniazda
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // 2. Uzyskanie adresu IP hosta
    struct hostent *server = gethostbyname(host);
    if (!server) {
        fprintf(stderr, "Brak takiego hosta: %s\n", host);
        return EXIT_FAILURE;
    }

    // 3. Konfiguracja adresu serwera
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    // 4. Nawiązywanie połączenia
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return EXIT_FAILURE;
    }

    printf("Połączono z serwerem %s:%d\n", host, port);

    close(sockfd);
    return EXIT_SUCCESS;
}
