#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#define PORT 1500
#define BACKLOG 5

int main(void) {
    int sockfd,new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size;

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        printf("socket failed:%d",errno);
        return -1;
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(my_addr.sin_zero),8);
    if(bind(sockfd,(struct sockaddr *)&my_addr, sizeof(struct sockaddr))<0){
        printf("bind error");
        return -1;
    }
    listen(sockfd,BACKLOG);

    while (1){
        sin_size = sizeof(struct sockaddr_in);
        new_fd = accept(sockfd,(struct sockaddr *)&their_addr,&sin_size);
        if(new_fd == -1){
            printf("receive failed");
        } else{
            printf("receive success");
            send(new_fd,"hello world!",12,0);
        }
    }

    return 0;
}