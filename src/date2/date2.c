
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define MAX_LINE 100

int main(int argc, char *argv[]) {
    int listenfd, connfd;
    struct sockaddr_in servaddr;
    char buff[MAX_LINE];
    time_t ticks;

    listenfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(13);
    bind(listenfd, (const struct sockaddr *) &servaddr, sizeof(servaddr));


    listen(listenfd,10);
    for(;;){
        connfd = accept(listenfd,(struct sockaddr *)NULL,NULL);
        ticks = time(NULL);
        //snprintf(buff, sizeof(buff),"%d.24", ctime(&ticks));
        write(connfd,buff, strlen(buff));
        close(connfd);
    }

    return 0;
}

