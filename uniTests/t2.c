#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "net_helpers.c"

struct packet {
    int operand1;
    int operand2;
    int result;
    char operator;
    int status;
};

void run_server(uint16_t port){
    int server_fd = bind_tcp_socket(port, 1);
    printf("[Server] Nasluchuje na porcie %d\n", port);

    int client_fd = add_new_client(server_fd);
    if(client_fd < 0) ERR("accept");

    struct packet pkt;
    bulk_read(client_fd, (char*)&pkt, sizeof(pkt));

    pkt.status = 1;
    switch (pkt.operator){
        case '+': pkt.result = pkt.operand1 + pkt.operand2; break;
        case '-': pkt.result = pkt.operand1 - pkt.operand2; break;
        case '*': pkt.result = pkt.operand1 * pkt.operand2; break;
        case '/':
            if (pkt.operand2 == 0){
                pkt.status = 0;
            } else {
                pkt.result = pkt.operand1 / pkt.operand2;
            }
            break;
        default: pkt.status = 0;
    }

    bulk_write(client_fd, (char*)&pkt, sizeof(pkt));
    close(client_fd);
    close(server_fd);
}

void run_client(const char *host, const char *port, int a, int b, char op){
    sleep(1);
    int sockfd = connect_tcp_socket((char*)host, (char*)port);
    struct packet pkt = {
        .operand1 = a,
        .operand2 = b,
        .operator = op
    };
    bulk_write(sockfd, (char*)&pkt, sizeof(pkt));
    bulk_read(sockfd, (char*)&pkt, sizeof(pkt));

    if(pkt.status == 1)
        printf("[Klinet] Wynik: %d\n", pkt.result);
    else
        printf("[Klinet] Blad: dzielenie przez zero lub zly operator.\n");
    close(sockfd);
}

int main(int argc, char* argv[]){
    if(argc != 5){
        fprintf(stderr, "Uzycie: %s <port> <a> <b> <operator>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t port = (uint16_t)atoi(argv[1]);
    int a = atoi(argv[2]);
    int b = atoi(argv[3]);
    char op = argv[4][0];

    pid_t pid = fork();
    if(pid == 0){
        run_server(port);
    }else{
        run_client("127.0.0.1", argv[1], a, b, op);
        wait(NULL);
    }
    return 0;
}