#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#define DEST_PORT 1500
#define DEST_IP "127.0.0.1"
#define MAX_DATA 100

int main(void) {

    int sockfd,new_fd;
    struct sockaddr_in dest_addr;
    char buf[MAX_DATA];

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        printf("socket failed: %d",errno);
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(DEST_IP);
    bzero(&(dest_addr.sin_zero),8);
    if(connect(sockfd,(struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1){
        printf("connect failed:%d",errno);
    } else{
        printf("connect success \n");
        recv(sockfd,buf,MAX_DATA,0);
        printf("received:%s",buf);
    }
    close(sockfd);
    return 0;
}