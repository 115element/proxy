

#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 100

int main(int argc,char *argv[]){

    int sockfd,n;
    char recvline[MAX_LINE+1];
    struct sockaddr_in servaddr;

    if(argc != 2) {
        puts("a.out <IPADDRESS>");
        exit(1);
    }

    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        puts("socket error");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13);
    if(inet_pton(AF_INET,argv[1],&servaddr.sin_addr) < 0) {
        puts("inet pton error");
        exit(1);
    }

    if(connect(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        puts("connect error");
        exit(1);
    }

    while ((n = read(sockfd,recvline,MAX_LINE)) > 0 ){
        recvline[n] = 0; // null terminate zifu jiewei
        if(fputs(recvline,stdout) == EOF) {
            puts("fputs error");
            exit(1);
        }
    }

    if(n < 0) {
        puts("read error");
        exit(1);
    }

    return 0;
}