#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SIZE 64

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "Uzycie %s <host> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = getpid();
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "%d", pid);
    printf("PID=%s\n", buffer);

    struct sockaddr_in server_addr;
    struct hostent *server = gethostbyname(argv[1]);
    if(!server){
        fprintf(stderr, "Nie znaleziono hosta %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[2]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("connect");
        return EXIT_FAILURE;
    }

    if(write(sockfd, buffer, strlen(buffer)) < 0){
        perror("write");
        return EXIT_FAILURE;
    }

    int16_t result;
    if(read(sockfd, &result, sizeof(result)) < 0){
        perror("read");
        return EXIT_FAILURE;
    }

    printf("SUM=%d\n", ntohs(result));  // Zakładamy, że serwer wysyła w porządku sieciowym

    close(sockfd);
    return 0;
}