#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

#define SA struct sockaddr
#define MAXLINE 20
#define PORT 8080

void dig_echo(int sockfd, SA *pcliaddr, socklen_t clilen) {
    int n;
    socklen_t len;
    char mesg[MAXLINE];
    for (;;) {
        len = clilen;
        n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
        sendto(sockfd, mesg, n, 0, pcliaddr, len);
    }
}

//typedef struct AA bb;

struct AA {
    struct AA *prev;
    struct AA *next;
};

struct AA mm;

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
    dig_echo(sockfd, (SA *) &cliaddr, sizeof(cliaddr));
    return 0;
}


