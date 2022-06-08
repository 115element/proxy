#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>

#define TCP_POROTO "tcp"

int proxy_port;
struct sockaddr_in hostaddr;
extern int errno;
char *sys_myerrlist[];
void parse_args(int argc,char **argv);
void daemonize(int servfd);
void do_proxy(int usersockfd);
void reap_status(void);
void errorout(char *msg);
typedef void signalfun(int);


int main(int argc, char *argv[]) {
    sys_myerrlist[110] = "TIMEOUT";
    sys_myerrlist[111] = "REFUSED";
    sys_myerrlist[101] = "UNREACH";
    unsigned int clilen;
    int childpid;
    int sockfd,newsockfd;
    struct sockaddr_in  servaddr,cliaddr;
    parse_args(argc,argv);

    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = proxy_port;

    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        fputs("failed to create server socket\r\n",stderr);
        exit(1);
    }

    if(bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        fputs("faild to bind server socket to specified port\r\n",stderr);
        exit(1);
    }

    listen(sockfd,5);

    daemonize(sockfd);

    while (1){
        clilen = sizeof(cliaddr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
        if(newsockfd < 0 && errno == EINTR){
            continue;
        } else if(newsockfd < 0){
            errorout("failed to accept connection");
        }

        if((childpid = fork()) == 0) {
            close(sockfd);
            do_proxy(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }

    return 0;
}


void parse_args(int argc,char **argv){
    int j;
    struct hostent *hostp;
    struct servent *servp;
    unsigned long inaddr;

    struct {
        char proxy_port[16];
        char isolated_host[64];
        char service_name[32];
    } pargs;

    if(argc < 4){
        printf("usage:%s <proxy-port> <host> <service-name>|<port-number> \r\n",argv[0]);
        exit(0);
    }

    strcpy(pargs.proxy_port,argv[1]);
    strcpy(pargs.isolated_host,argv[2]);
    strcpy(pargs.service_name,argv[3]);
    for (j = 0; j < strlen(pargs.proxy_port); ++j) {
        if(!isdigit(*(pargs.proxy_port+j))){
            break;
        }
        if(j == strlen(pargs.proxy_port)){
            proxy_port = htons(atoi(pargs.proxy_port));
        } else{
            printf("invalid proxy port");
            exit(0);
        }
    }

    bzero(&hostaddr, sizeof(hostaddr));
    hostaddr.sin_family = AF_INET;
    if((inaddr = inet_addr(pargs.isolated_host)) != INADDR_NONE){
        bcopy(&inaddr,&hostaddr.sin_addr, sizeof(inaddr));
    } else if((hostp = gethostbyname(pargs.isolated_host))!=NULL){
        bcopy(hostp->h_addr,&hostaddr.sin_addr,hostp->h_length);
    } else {
        printf("unknown host");
        exit(1);
    }
}

void errorout(char * msg){
    FILE *console;
    console = fopen("/dev/console","a");
    fprintf(console,"proxyd:%s\r\n",msg);
    fclose(console);
    exit(1);
}


void do_proxy(int usersockfd){
    int isosockfd;
    fd_set rdfdset;
    int connstat;
    unsigned int iolen;
    char buf[2048];

    if((isosockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        errorout("failed to create socket to host");
    }

    connstat = connect(isosockfd, (const struct sockaddr *) &hostaddr, sizeof(hostaddr));
    switch (connstat) {
        case 0:
            break;
        case ETIMEDOUT:
        case ECONNREFUSED:
        case ENETUNREACH:
            strcpy(buf,sys_myerrlist[errno]);
            strcat(buf,"\r\n");
            write(usersockfd,buf, strlen(buf));
            close(usersockfd);
            exit(1);
            break;
        default:
            errorout("failed to connect to host");
    }


    while (1){
        FD_ZERO(&rdfdset);
        FD_SET(usersockfd,&rdfdset);
        FD_SET(isosockfd,&rdfdset);
        if(select(FD_SETSIZE,&rdfdset,NULL,NULL,NULL) < 0){
            errorout("select failed");
        }

        if(FD_ISSET(usersockfd,&rdfdset)){
            if((iolen = read(usersockfd,buf, sizeof(buf)))<0) {
                break;
            }
            write(isosockfd,buf,iolen);
        }

        if(FD_ISSET(isosockfd,&rdfdset)){
            if((iolen = read(isosockfd,buf, sizeof(buf))) < 0 ){
                break;
            }
            write(usersockfd,buf,iolen);
        }
    }

    close(usersockfd);
    close(isosockfd);
}

void daemonize(int servfd){
    int childpid,fd,fdtablesize;
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    if((childpid = fork()) < 0){
        fputs("failed to fork first child\r\n",stderr);
        exit(1);
    } else if(childpid > 0){
        exit(0);
    }

    if(setpgid(0,getpid()) < 0){
        fputs("failed to become process group leader\r\n",stderr);
        exit(1);
    }

    if((fd = open("/dev/tty",O_RDWR)) > 0 ){
        ioctl(fd,TIOCNOTTY,NULL);
        close(fd);
    }

    for (fd = 0,fdtablesize=getdtablesize(); fd<fdtablesize ; fd++) {
          if(fd != servfd) {
              close(fd);
          }
    }


    chdir("/");
    umask(0);
    signal(SIGCLD,(signalfun *)reap_status);
}

void reap_status(){
    int pid;
    int status;
    while ((pid = wait3(&status,WNOHANG,NULL)) >0){
        printf("child %d terminated \n",pid);
    }
}
