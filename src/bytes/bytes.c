#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    union {
        short s;
        char c[sizeof(short)];
    } un;

    un.s = 0x0102;
    if (sizeof(short) == 2) {
        if (un.c[0] == 1 && un.c[1] == 2) {
            printf("big-endian\n");
        } else if (un.c[0] == 2 && un.c[1] == 1) {
            printf("little-endian\n");
        } else {
            printf("unknown\n");
        }
    } else {
        printf("sizeof(short) == %lu\n", sizeof(short));
    }
    exit(0);
    return 0;
}

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

int inet_pton1(int family, const char *strptr, void *addrptr) {
    if (family == AF_INET) {
        struct in_addr in_val;
        if (inet_aton(strptr, &in_val)) {
            memcpy(addrptr, &in_val, sizeof(struct in_addr));
            return 1;
        }
        return 0;
    }
    errno = EAFNOSUPPORT;
    return -1;
}

const char *inet_ntop1(int family, const void *addrptr, char *strptr, size_t len) {
    const u_char *p = (const u_char *) addrptr;
    if (family == AF_INET) {
        char temp[INET_ADDRSTRLEN];
        snprintf(temp, sizeof(temp), "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        if (strlen(temp) >= len) {
            errno = ENOSPC;
            return NULL;
        }
        strcpy(strptr, temp);
        return strptr;
    }
    errno = EAFNOSUPPORT;
    return NULL;
}

#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>

// R "n" bytes from a descriptor
size_t readn(int fd, void *vptr, size_t n) {
    size_t nleft;
    size_t nread;
    char *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;  // and call read() again
            } else {
                return -1;
            }
        } else if (nread == 0) {
            break;  // EOF
        }

        nleft -= nread;
        ptr += nread;
    }
    return n - nleft;
}

// Write "n" bytes to a descriptor
size_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    size_t nwritten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0; //and call write() again
            } else {
                return -1;
            }
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}


size_t readline(int fd, void *vptr, size_t maxlen) {
    size_t n, rc;
    char c, *ptr;
    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        again:
        if ((rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n') {
                break;
            } else if (rc == 0) {
                if (n == 1) {
                    return 0; //EOF , no data read
                } else {
                    break; //EOF , some data was read
                }
            }
        } else {
            if (errno == EINTR) {
                goto again;
            }
            return -1; // error,errno set by read()
        }
    }
    *ptr = 0; // null terminate like fgets()
    return n;
}

#define MAXLINE 2048

static size_t my_read(int fd, char *ptr) {
    static unsigned int read_cnt = 0;
    static char *read_ptr;
    static char read_buf[MAXLINE];
    if (read_cnt <= 0) {
        again:
        if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
            if (errno == EINTR) {
                goto again;
            }
            return -1;
        } else if (read_cnt == 0) {
            return 0;
        }
        read_ptr = read_buf;
    }
    read_cnt--;
    *ptr = *read_ptr++;
    return 1;
}

#ifdef S_IF_SOCK
#error S_IF_SOCK not defined
#endif

int isfdtype(int fd, int fdtype) {
    struct stat buf;
    if (fstat(fd, &buf) < 0) {
        return -1;
    }

    if ((buf.st_mode & S_IFMT) == fdtype) {
        return 1;
    } else {
        return 0;
    }
}

#define MAX 100

void str_cli(FILE *fp, int sockfd) {
    char sendline[MAX], recvline[MAX];
    while (fgets(sendline, MAX, fp) != NULL) {
        writen(sockfd, sendline, strlen(sendline));
        if (readline(sockfd, recvline, MAX) == 0) {
            puts("server terminated");
            exit(1);
        }
        fputs(recvline, stdout);
    }
}


struct args {
    long arg1;
    long arg2;
};
struct result {
    long sum;
};

void str_cli1(FILE *fp, int sockfd) {
    char sendline[MAXLINE];
    struct args args;
    struct result result;
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        if (scanf(sendline, "%ld%ld", &args.arg1, &args.arg2) != 2) {
            printf("invalid input:%s", sendline);
            continue;
        }
        writen(sockfd, &args, sizeof(args));
        if (readn(sockfd, &result, sizeof(result)) == 0) {
            exit(1);
        }
        printf("%ld\n", result.sum);
    }
}

int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

#include <math.h>

void str_cli2(FILE *fp, int sockfd) {
    fd_set rset;
    char sendline[MAXLINE], recvline[MAXLINE];
    FD_ZERO(&rset);
    for (;;) {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        select(FD_SETSIZE, &rset, NULL, NULL, NULL);
    }
}

#include <limits.h>
#include <poll.h>

#define OPEN_MAX 200

int main1(int argc, char *argv[]) {
    int i, maxi, listenfd, concfd, sockfd;
    int nready;
    size_t n;
    char line[MAXLINE];
    socklen_t clilen;
    struct pollfd client[OPEN_MAX];
    struct sockaddr_in cliaddr, servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(200);
    bind(listenfd, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    listen(listenfd,5);
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    for (int j = 0; j < OPEN_MAX; ++j) {
        client[j].fd = -1;
    }
    maxi = 0;
    for (;;){
    }
}

