#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define HOSTNAME "www.baidu.com"

int main(int argc, char *argv[]) {
    struct hostent *hptr;
    char **pptr;
    char str[32] = {0};

    if ((hptr = gethostbyname(HOSTNAME)) == NULL) {
        printf("gethostbyname error: %s\n", HOSTNAME);
        return 0;
    }
    printf("official hostname:%s\n", hptr->h_name); ///主机规范名

    for (pptr = hptr->h_aliases; *pptr != NULL; pptr++) { ///将主机别名打印出来
        printf("alias: %s\n", *pptr);
    }

    switch (hptr->h_addrtype) { //根据地址类型，将地址打印出来
        case AF_INET:
        case AF_INET6:
            pptr = hptr->h_addr_list;
            for (; *pptr != NULL; pptr++) { //将得到的所有地址打印出来
                //inet_ntop: 将网络字节序的二进制转换为文本字符串的格式
                printf("address: %s\n", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
                printf("first address: %s\n", inet_ntop(hptr->h_addrtype, hptr->h_addr_list, str, sizeof(str)));
            }
            break;
        default:
            printf("unknown address type\n");
            break;
    }
    return 0;
}
