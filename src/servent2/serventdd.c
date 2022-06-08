#include <stdio.h>
#include <netdb.h>

int main() {
    struct servent *serv = NULL;
    serv = getservbyport(5888, "tcp");
    if (serv != NULL) {
        printf("servent2 name: %s\n", serv->s_name);
        if (*(serv->s_aliases) != NULL) {
            printf("alias name: %s\n", *(serv->s_aliases));
        }
        printf("port number: %d\n", serv->s_port);
        printf("proto to use: %s\n", serv->s_proto);
    }
    return 0;
}