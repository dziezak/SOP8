#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define ERR(source) (perror(source), exit(EXIT_FAILURE))

int sock;

void *receiver_thread(void *arg) {
    char buf[BUF_SIZE];
    while (1) {
        ssize_t count = read(sock, buf, sizeof(buf) - 1);
        if (count <= 0) {
            printf("Rozłączono z serwerem.\n");
            exit(EXIT_SUCCESS);
        }
        buf[count] = '\0';
        printf(">> %s", buf);
    }
    return NULL;
}

int main() {
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("127.0.0.1", "12345", &hints, &res) != 0) ERR("getaddrinfo");

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) ERR("socket");

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) ERR("connect");
    freeaddrinfo(res);

    pthread_t tid;
    pthread_create(&tid, NULL, receiver_thread, NULL);

    char input[BUF_SIZE];
    while (fgets(input, sizeof(input), stdin)) {
        if (write(sock, input, strlen(input)) < 0) {
            perror("write");
            break;
        }
    }

    close(sock);
    return 0;
}
