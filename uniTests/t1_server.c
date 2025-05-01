#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct packet {
    int operand1;
    int operand2;
    int result;
    char operator;
    int status;
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(sockfd, 1);

    printf("Serwer nasłuchuje na porcie %d...\n", port);
    int client_fd = accept(sockfd, NULL, NULL);

    struct packet pkt;
    read(client_fd, &pkt, sizeof(pkt));

    pkt.status = 1;
    switch(pkt.operator) {
        case '+': pkt.result = pkt.operand1 + pkt.operand2; break;
        case '-': pkt.result = pkt.operand1 - pkt.operand2; break;
        case '*': pkt.result = pkt.operand1 * pkt.operand2; break;
        case '/':
            if (pkt.operand2 == 0) {
                pkt.status = 0;
            } else {
                pkt.result = pkt.operand1 / pkt.operand2;
            }
            break;
        default:
            pkt.status = 0;
    }

    write(client_fd, &pkt, sizeof(pkt));
    close(client_fd);
    close(sockfd);
    return 0;
}
