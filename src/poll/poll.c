#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

int main(){
    int fd = socket(AF_INET,SOCK_STREAM,0);
    int reuse_port = 1;

    setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,&reuse_port, sizeof(int));

    //
    struct sockaddr_in bind_address;
    memset(&bind_address,0, sizeof(struct sockaddr_in));
    bind_address.sin_addr.s_addr = inet_addr("0.0.0.0");
    bind_address.sin_family = AF_INET;
    bind_address.sin_port = htons(8888);

    int code = bind(fd, (const struct sockaddr *) &bind_address, sizeof(struct sockaddr_in));
    if(code == -1){
        perror("bind error");
        close(fd);
        return -1;
    }

    code = listen(fd,1024);
    if(code == -1){
        perror("listen error");
        close(fd);
        return -1;
    }

    struct pollfd fds_array[1024];
    for (int i = 0; i < 1024; i++) {
        fds_array[i].fd = -1;
    }

    fds_array[0].fd = fd;
    fds_array[0].events = POLLIN;

    int max_index = 0;

    while (1) {
        int receive_num = poll(fds_array,max_index+1,-1);
        if(receive_num == -1){
            perror("poll error");
            break;
        } else if(receive_num == 0){
            continue;
        } else{
            if(fds_array[0].revents & POLLIN) {
                printf("accept\n");
                int cfd = accept(fd,NULL,NULL);
                if(cfd == -1){
                    perror("accept error");
                    continue;
                } else {
                    int find_index = -1;
                    for (int i = 0; i < ARRAY_SIZE(fds_array); ++i) {
                        if(fds_array[i].fd == -1) {
                            fds_array[i].fd = cfd;
                            fds_array[i].events = POLLIN;
                            find_index = i;
                            break;
                        }
                    }
                    if(find_index == -1){
                        printf("client1 connect too many,refuse!");
                        continue;
                    }
                    if(find_index > max_index) {
                        max_index = find_index;
                    }
                }
            } else {
                for (int i = 0; i < max_index; ++i) {
                    if(fds_array[i].fd == -1){
                        continue;
                    }
                    if(fds_array[i].revents & POLLIN) {
                        char buf[1024];
                        size_t read_num = read(fds_array[i].fd,buf,1024);
                        if(read_num < 0) {
                            perror("read error");
                            close(fds_array[i].fd);
                            fds_array[i].fd = -1;
                            continue;
                        } else if(read_num == 0){
                            printf("close \n");
                            close(fds_array[i].fd);
                            fds_array[i].fd = -1;
                            continue;
                        } else {
                            write(fds_array[i].fd,buf,read_num);
                        }
                    }
                }
            }
        }
    }
}