#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/stat.h>

#define BUF_LEN 8192

void udpserver(char *localport, char *serverip, char *serverport);

void usage(char *execname);

void sighandler(int signal);

extern char *optarg;
extern int optind, opterr, optopt;


int main(int argc, char *argv[]) {
    int c;
    char *options = "l:s:p:hv";
    char *argval[] = {NULL, NULL, NULL};


    /*
     * Parse arguments and their values.
     */
    while ((c = getopt(argc, argv, options)) != -1) {
        switch (c) {
            case 'l':
                argval[0] = optarg;
                break;
            case 's':
                argval[1] = optarg;
                break;
            case 'p':
                argval[2] = optarg;
                break;
            case 'h':
            case 'v':
                usage(argv[0]);
                return EXIT_SUCCESS;
            case '?':
                printf("Unknown switch: %c. Ignoring.\n", optopt);
                break;
            default:
                continue;
        }
    }

    /*
     * Check that argument values are set.
     */
    if (argval[0] == NULL) {
        printf("Missing local udp port for listening.\n");
        return EXIT_FAILURE;
    }
    if (argval[1] == NULL) {
        printf("Missing server ip address.\n");
        return EXIT_FAILURE;
    }
    if (argval[2] == NULL) {
        printf("Missing server udp port number.\n");
        return EXIT_FAILURE;
    }

    /*
     * Start as service.
     */
    pid_t pid;
    pid = fork();
    /* Error in fork. */
    if (pid < 0) {
        return EXIT_FAILURE;
    }
    /* Terminate parent process. */
    if (pid > 0) {
        return EXIT_SUCCESS;
    }
    /* Child process becomes session leader. */
    if (setsid() < 0) {
        return EXIT_FAILURE;
    }

    /* Set new file permission */
    umask(0);

    /* Close file std. descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Install sig. handler for kill command (default signal is TERM) */
    signal(SIGTERM, sighandler);

    /* Open syslog logger */
    openlog(argv[0], LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Service started.");

    /* Run main logic of udp proxy service. */
    udpserver(argval[0], argval[1], argval[2]);
    return EXIT_SUCCESS;
}


void sighandler(int sig) {
    if (sig == SIGTERM) {
        syslog(LOG_NOTICE, "Service terminated.");
        closelog();
        exit(EXIT_SUCCESS);
    }
}


/**
 * Terminate this application with custom message.
 * @param msg message to print
 */
void die(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * Start udp proxy service. Local port is udp port value on which proxy receive datagram.
 * All arguments are passed as string and are converted into integers.
 *
 * @param localport a string that contains valid udp port value
 * @param serverip a string that contains valid server ip address
 * @param serverport a string that contains valid server port
 */
void udpserver(char *localport, char *serverip, char *serverport) {
    struct sockaddr_in si_me;
    int serverSocket;

    size_t recv_len;
    socklen_t slen = sizeof(si_me);

    char datagram[BUF_LEN];

    /*
     * Create a server UDP socket.
     */
    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        die("Socket problem: UDP not supported.");
    }

    /*
     * Zero out the structure.
     */
    memset((char *) &si_me, 0, sizeof(si_me));

    /*
     * Bind to local udp port and all ip
     * interfaces.
     */
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(atoi(localport));
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    /*
     * Bind socket to port.
     */
    if ((bind(serverSocket, (struct sockaddr *) &si_me, sizeof(si_me))) == -1) {
        die("Bind error. Check that port is available.");
    }


    //设置加入多播组(接收端)
    //结构体的定义
//    struct ip_mreq
//    {
//        struct in_addr imr_multiaddr; //多播组的IP地址          //组播组的IP地址。
//        struct in_addr imr_interface; //加入的客服端主机IP地址   //本地某一网络设备接口的IP地址。
//    }
//    struct ip_mreq my_mreq;
//    memset(&my_mreq,0, sizeof(my_mreq));
//    my_mreq.imr_multiaddr.s_addr = htonl(atoi("224.0.0.1"));
//    my_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
//    if(-1 == setsockopt(serverSocket,IPPROTO_IP,IP_ADD_MEMBERSHIP,&my_mreq, sizeof(my_mreq))){
//        die("setsockopt error");
//    }

    /*
     * UDP proxy socket address.
     */
    struct sockaddr_in si_proxy;
    int proxySocket;
    if ((proxySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        die("Socket problem: UDP not supported.");
    }

    memset((char *) &si_proxy, 0, sizeof(si_proxy));
    si_proxy.sin_family = AF_INET;
    si_proxy.sin_port = htons(atoi(serverport));

    //inet_aton是一个计算机函数，功能是将一个字符串IP地址转换为一个32位的网络序列IP地址。
    //如果这个函数成功，函数的返回值非零，如果输入地址不正确则会返回零。使用这个函数并没有错误码存放在errno中，所以它的值会被忽略。
    if (inet_aton(serverip, &si_proxy.sin_addr) == 0) {
        die("Call inet_aton() failed for given server address.");
    }

    /*
     * Keep listening for data.
     */
    while (1) {
        struct sockaddr_in si_other;
        syslog(LOG_NOTICE, "Waiting for data ...");
        /*
         * Try to receive some data, this is a blocking call.
         */
        if ((recv_len = recvfrom(serverSocket, datagram, BUF_LEN, 0, (struct sockaddr *) &si_other, &slen)) == -1) {
            syslog(LOG_NOTICE, "Function recvfrom() problem.");
        }
        /*
        * Print details of the client data received.
        */
        syslog(LOG_NOTICE, "Received packet from: %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

        /*
         * Forward datagram to remote server.
         */
        syslog(LOG_NOTICE, "Forwarding packet to: %s:%d\n", inet_ntoa(si_proxy.sin_addr), ntohs(si_proxy.sin_port));
        if (sendto(proxySocket, datagram, recv_len, 0, (struct sockaddr *) &si_proxy, slen) == -1) {
            syslog(LOG_NOTICE, "Function sendto() problem.");
        }


        /*
         * Receive a reply from server on proxy socket.
         */
        if ((recv_len = recvfrom(proxySocket, datagram, BUF_LEN, 0, (struct sockaddr *) &si_proxy, &slen)) == -1) {
            syslog(LOG_NOTICE, "Function recvfrom() problem.");
        }
        /*
         * Print details of the client data received.
         */
        syslog(LOG_NOTICE, "Received packet from: %s:%d\n", inet_ntoa(si_proxy.sin_addr), ntohs(si_proxy.sin_port));


        /*
         * Send back reply to client.
         */
        syslog(LOG_NOTICE, "Forwarding packet to: %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        if (sendto(serverSocket, datagram, recv_len, 0, (struct sockaddr *) &si_other, slen) == -1) {
            syslog(LOG_NOTICE, "Function sendto() problem.");
        }
    }
    close(serverSocket);
    close(proxySocket);
}


/**
 * Print application usage and command line example.
 * @param execname binary executable name, could be
 * provided as argv[0]
 */
void usage(char *execname) {
    printf("Usage:\n");
    printf(" -l [local udp port to listen]\n");
    printf(" -s [server ip address]\n");
    printf(" -p [remote server udp port]\n");
    printf("Example:\n");
    printf("%s -l 1234 -s 127.0.0.1 -p 1234\n", execname);
    printf("\n");
}