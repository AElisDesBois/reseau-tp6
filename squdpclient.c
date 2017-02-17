#include <errno.h>
#include <err.h>
#include <sysexits.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libgen.h>

#include "sqserv.h"

#undef LEGACY

char *prog_name;

void usage() {
    printf("usage: %s [-p <port>] <hostname>\n", prog_name);
    printf("\tremote host name as argument\n");
    printf("\t-p <port> specify alternate port\n");
}

int main(int argc, char* argv[]) {

    long port = DEFAULT_PORT;
    //struct sockaddr_in sin;
#ifdef LEGACY
    struct hostent *hp;
#else
    struct addrinfo hints, *ai;
    char service[15];
    char ipstr[INET6_ADDRSTRLEN];
    int error;
#endif
    int fd;
    //size_t  data_len, header_len;
    //ssize_t len;
    //char *data;
    char *host;
    long ch;

    /* get options and arguments */
    prog_name = strdup(basename(argv[0]));
    while ((ch = getopt(argc, argv, "?hp:")) != -1) {
        switch (ch) {
            case 'p': port = strtol(optarg, (char **)NULL, 10);break;
            case 'h':
            case '?':
            default: usage(); return 0;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1) {
        usage();
        return EX_USAGE;
    }

    host = argv[0];

    printf("sending to %s:%d\n", host, (int)port);

    /* create a socket, no bind needed, let the system choose */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    printf("%d\n",fd );
    /* gethostbyname for information, getaddrinfo is prefered */
#ifdef LEGACY
    if((hp = gethostbyname(host)) == NULL) {
        errx(EX_NOHOST, "(gethostbyname) cannot resolve %s: %s", host, hstrerror(h_errno));
    }
    printf("gethostbyname: resolved name '%s' to IP address %s\n", hp->h_name, inet_ntoa(*(struct in_addr *)hp->h_addr));
#else
    /* getaddrinfo is the prefered method today */
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_ADDRCONFIG | AI_CANONNAME | AI_NUMERICSERV; // AI_PASSIVE
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    snprintf(service, sizeof(service), "%ld", port);
    if ((error = getaddrinfo(host, service, &hints, &ai)) != 0) {
        errx(EX_NOHOST, "(getaddrinfo) cannot resolve %s: %s", host, gai_strerror(error));
    }
#endif

    if(ai == NULL) {
        printf("ai = null");
        return -1;
    }
    inet_ntop(ai->ai_family, &(((struct sockaddr_in *)ai->ai_addr)->sin_addr), ipstr, sizeof(ipstr));
    printf("getaddrinfo:   resolved name '%s' to IP address %s\n", ai->ai_canonname, ipstr);
    printf("connecting...\n");
   
    int len = connect(fd,(struct sockaddr *)(ai->ai_addr), sizeof(struct sockaddr_in));
    if(len < 0){
        perror("connected failed: ");  
        return -1;
    } 

    printf("connected => len = %d\n", len);

    /* send data */
    int data_len = 10;
    char* data = (char *) malloc(data_len);
    data = "5";

    int ai_len = sizeof(struct sockaddr_in);
//    int len = write(fd, data, data_len);
//    len = sendto(fd, data, data_len, 0, (struct sockaddr *)ai, ai_len);
    len = send(fd, data, data_len, 0);
    
    if(len < 0){
        perror("sendto failed: ");  
        return -1;
    } 
    printf("len apres envoie = %d\n", (int)len);

    /* receive data */

    //len = read(fd, data, data_len);
    len = recvfrom(fd, data, data_len, 0, (struct sockaddr *)ai,(socklen_t*) &ai_len);
//    len = recv(fd, data, data_len, 0);

    if(len < 0){
        perror("read failed: ");  
        return -1;
    } 

    printf("len apres lecture = %d\n", (int)len);
    printf("resultat = %s\n", data);


    /* cleanup */

    free(data);
    close(fd);

    return EX_OK;
}
