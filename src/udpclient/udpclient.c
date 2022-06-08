#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#define BROADCAST_PORT 8081

int main() {

    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[2000], client_message[2000];
    unsigned int server_struct_length = sizeof(server_addr);

    //Clean buffers;
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));

    //Create socket
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_desc < 0) {
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");


    //发送广播第一步：设置发送广播
    int on = 1;
    if(setsockopt(socket_desc,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on)) < 0) {
        printf("Error set broadcast\n");
        exit(1);
    }


    //Get input from the user;
    printf("Enter message:");
    scanf("%s", client_message);


    //发送广播第二步：建立广播地址
    struct sockaddr_in broadcastaddr;
    broadcastaddr.sin_family = AF_INET;
    broadcastaddr.sin_addr.s_addr = inet_addr("192.168.1.255"); //本机IP对应的广播地址，
    // 例如：当前主机IP为：192.168.0.146，那么他的广播地址就是：192.168.0.255
    broadcastaddr.sin_port = htons(BROADCAST_PORT);


    ///发送广播第三步：向广播地址发送数据
    //Send the message to server;
    if (sendto(socket_desc, client_message, strlen(client_message), 0,
               (struct sockaddr *) &broadcastaddr, sizeof(broadcastaddr)) < 0) {
        printf("Unable to send message\n");
        return -1;
    }

    //Receive the server`s response:
    if(recvfrom(socket_desc,server_message, sizeof(server_message),0,
                (struct sockaddr *)&server_addr,&server_struct_length) < 0){
        printf("Error while receiving server`s msg\n");
        return -1;
    }
    printf("Server`s response: %s\n",server_message);

    //Close the socket
    close(socket_desc);

    return 0;
}