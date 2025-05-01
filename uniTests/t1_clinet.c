#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


struct packet {
    int operand1;
    int operand2;
    int result;
    char operator;
    int status;
};

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Użycie: %s <adres> <port> <a> <b> <operator>\n", argv[0]);
        exit(1);
    }

    char *host = argv[1];
    int port = atoi(argv[2]);
    struct packet pkt = {
        .operand1 = atoi(argv[3]),
        .operand2 = atoi(argv[4]),
        .operator = argv[5][0]
    };

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {0};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); // konwertuje port do formatu sieciowego
    inet_pton(AF_INET, host, &serv_addr.sin_addr); // konwertuje adres IP z tekstu do formy bitowej


    connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    //Nawiazujemy polaczenie TCP z serwerem 
    write(sockfd, &pkt, sizeof(pkt));
    // wysylamy wiadomosc
    read(sockfd, &pkt, sizeof(pkt));
    // czekamy na odpowiedz

    if (pkt.status == 1)
        printf("Wynik: %d\n", pkt.result);
    else
        printf("Błąd obliczeń (np. dzielenie przez zero lub zły operator)\n");

    close(sockfd);
    return 0;
}
