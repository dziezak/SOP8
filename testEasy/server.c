#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define PORT 12345
#define BUF_SIZE 256

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buffer[BUF_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 1) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Czekam na polaczenie na porcie %d...\n", PORT);
    client_fd = accept(server_fd, NULL, NULL);
    if(client_fd < 0){
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Polaczono z klientem\n");

    while(1){
        ssize_t len = read(clinet_fd, buffer, BUF_SIZE -1);
        if(len <= 0){
            printf("[Server] Klient sie rozlaczyl.\n");
            break;
        }
        buffer[len] = '\0';
        printf("[Server] Odebrano: %s\n");
    }
    close(client_fd);
    close(server_fd);
    return 0;
}

