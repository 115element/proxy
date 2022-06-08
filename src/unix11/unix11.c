
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#define SA struct sockaddr
#define MAXLINE 20
#define PORT 8080

void dig_cli(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen) {
    unsigned int n;
    char sendline[MAXLINE],recvline[MAXLINE+1];
    while (fgets(sendline,MAXLINE,fp) != NULL){
        sendto(sockfd,sendline, strlen(sendline),0,pservaddr,servlen);
        n = recvfrom(sockfd,recvline,MAXLINE,0,NULL,NULL);
        recvline[n] = 0;
        fputs(recvline,stdout);
    }
}


int main(int argc,char *argv[]) {

    int sockfd;
    struct sockaddr_in servaddr;
    if(argc != 2){
        exit(1);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    inet_pton(AF_INET,argv[1],&servaddr.sin_addr);
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    dig_cli(stdin,sockfd,(SA *)&servaddr, sizeof(servaddr));
    exit(0);
    return 0;
}

