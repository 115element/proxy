
#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h> // sockaddr_in
#include <netinet/ip.h>
#include <arpa/inet.h>  // inet_addr
#include <errno.h>
#include <string.h>
#include <stdlib.h>     // atoi
#include <unistd.h>
#include <fcntl.h>      // fcntl

#define MAX_CONN_NUM 1024
#define BUF_SIZE 1024

#define exitif(s, err_str) do { \
    if(s) { \
        if(listenfd != -1) { \
            close(listenfd); \
            listenfd = -1; \
        } \
        if(epfd != -1) { \
            close(epfd); \
            epfd = -1; \
        } \
        printf("%s: %s(code:%d)\n", err_str, strerror(errno), errno); \
        exit(0); \
    } \
} while(0);


typedef struct _user_data {
    int             fd;
    uint32_t        events;
    char            send_buf[BUF_SIZE];
    char            recv_buf[BUF_SIZE];
} user_data;


int set_fd_nonblock(int fd) {
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("usage: %s ip port\n", argv[0]);
        return -1;
    }

    int ret;
    int listenfd = -1;
    int epfd = -1;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    exitif(-1 == ret, "socket");

    ret = set_fd_nonblock(listenfd);
    exitif(-1 == ret, "set_fd_nonblock");

    struct sockaddr_in local_addr;
    int addr_len = sizeof(struct sockaddr_in);
    memset(&local_addr, 0, addr_len);
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(argv[1]);
    local_addr.sin_port = htons(atoi(argv[2]));
    ret = bind(listenfd, (struct sockaddr *)&local_addr, addr_len);
    exitif(-1 == ret, "bind");

    ret = listen(listenfd, 5);
    exitif(-1 == ret, "listen");

    epfd = epoll_create(1);
    // 关注事件
    struct epoll_event watch_event;
    watch_event.events = EPOLLIN;
    watch_event.data.fd = listenfd;
    // 这里用的是fd对应的epoll_event，而不是整个epoll_event数组
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &watch_event);
    exitif(-1 == ret, "epoll_ctl");

    // 就绪事件列表
    struct epoll_event ready_events[MAX_CONN_NUM];
    while (1) {
        // events是触发的events
        int nready = epoll_wait(epfd, ready_events, MAX_CONN_NUM, 3000);
        if (nready < 0) {
            if (EINTR == errno) {
                continue;
            }
            exitif(-1 == nready , "epoll_wait");
        }
        // 就绪事件对应于events数组的下标[0, nready - 1]
        int i;
        for (i = 0; i < nready; ++i){
            if (listenfd == ready_events[i].data.fd) {
                struct sockaddr_in peer_addr;
                memset(&peer_addr, 0, addr_len);
                int peer_addr_len;
                int clientfd = accept(listenfd, (struct sockaddr *)&peer_addr, (socklen_t *)&peer_addr_len);
                exitif(-1 == clientfd, "accept");

                ret = set_fd_nonblock(clientfd);
                exitif(-1 == ret, "set_fd_nonblock");

                user_data* ud = malloc(sizeof(user_data));
                ud->events = EPOLLIN;
                ud->fd = clientfd;
                watch_event.events = EPOLLIN;
                watch_event.data.ptr = ud;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &watch_event);
                exitif(-1 == ret, "epoll_ctl");

                // 忘了加换行符，导致没有打印出来
                printf("new connection from fd: %d, address:%s\n", clientfd, inet_ntoa(peer_addr.sin_addr));
            }
            else {
                user_data* ud = ready_events[i].data.ptr;
                int clientfd = ud->fd;

                // 这里用if-else还是多个if？
                if (EPOLLIN & ready_events[i].events) {
                    char* recv_buf = ud->recv_buf;
                    int n = recv(clientfd, recv_buf, BUF_SIZE, 0);
                    if (n < 0) {
                        printf("recv errno: %s\n", strerror(errno));
                        return -1;
                    }
                    else if (n == 0) {
                        printf("client1 closed, fd: %d\n", clientfd);
                        // 这里第四个参数传什么？
                        // client退出时需要将fd从event中移除，否则会一直触发EPOLLIN
                        ret = epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, NULL);
                        exitif(-1 == ret, "epoll_ctl");
                        // 先移出event，再close
                        close(clientfd);
                        free(ud);
                    }
                    else {
                        // 有结束符就会换行，不需要再加换行符了
                        recv_buf[n] = '\0';
                        printf("recv from fd %d, msg: %s", clientfd, recv_buf);
                        ud->events |= EPOLLOUT;
                        strcpy(ud->send_buf, ud->recv_buf);
                        watch_event.events = ud->events;
                        watch_event.data.ptr = ud;
                        ret = epoll_ctl(epfd, EPOLL_CTL_MOD, clientfd, &watch_event);
                        exitif(-1 == ret, "epoll_ctl");
                    }
                }
                if (EPOLLOUT & ready_events[i].events) {
                    int send_len = strlen(ud->send_buf);
                    int nsend = send(clientfd, ud->send_buf, send_len, 0);
                    if (nsend < 0) {
                        printf("send errno: %s\n", strerror(errno));
                        return -1;
                    }
                    else if (nsend == 0) {
                        printf("send, client1 closed, fd: %d\n", clientfd);
                        ret = epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, NULL);
                        exitif(-1 == ret, "epoll_ctl");
                        close(clientfd);
                        free(ud);
                    }
                    else {
                        ud->events = EPOLLIN;
                        watch_event.events = ud->events;
                        watch_event.data.ptr = ud;

                        ret = epoll_ctl(epfd, EPOLL_CTL_MOD, clientfd, &watch_event);
                        printf("send to fd %d, msg:%s\n", clientfd, ud->send_buf);
                    }
                }
            }
        }
    }

    close(epfd);
    close(listenfd);
    return 0;
}