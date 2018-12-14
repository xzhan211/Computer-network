/*--------------------------------------------------------------------*/
/* functions to connect clients and server */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

#include <stdlib.h>

#define MAXNAMELEN 256
/*--------------------------------------------------------------------*/
/*----------------------------------------------------------------*/
/* prepare server to accept requests
 returns file descriptor of socket
 returns -1 on error
 */
int startserver() {
    int sd; /* socket descriptor */

    char * servhost; /* full name of this host */
    ushort servport; /* port assigned to this server */
    struct sockaddr_in sa;
    /*
     FILL HERE
     create a TCP socket using socket()
     */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    //printf("In startserver sd = %d\n", sd);
    /*
     FILL HERE
     bind the socket to some port using bind()
     let the system choose a port
     */
    sa.sin_family = AF_INET;
    sa.sin_port = htons(0);
    sa.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY == 0
    int testBind = bind(sd, (struct sockaddr *) &sa, sizeof(sa));
    //printf("testBind >> %d\n", testBind);
    if(testBind == -1){
       printf("error in bind");
    };

    /* we are ready to receive connections */
    int listenResult = listen(sd, 5);
    int salen = sizeof(sa);

    /*
     FILL HERE
     figure out the full host name (servhost)
     use gethostname() and gethostbyname()
     full host name is remote**.cs.binghamton.edu
     */
    char hostname[128];
    servhost = hostname;
    gethostname(hostname, sizeof(hostname));


    /*
     FILL HERE
     figure out the port assigned to this server (servport)
     use getsockname()
     */
    socklen_t len = sizeof(sa);
    if (getsockname(sd, (struct sockaddr *)&sa, &len) == -1)
            perror("getsockname");
    else
            servport = ntohs(sa.sin_port);


    /* ready to accept requests */
    printf("admin: started server on '%s.cs.binghamton.edu' at '%hu'\n", servhost, servport);
    //free(servhost);
    return (sd);
}
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/
/*
 establishes connection with the server
 returns file descriptor of socket
 returns -1 on error
 */
int hooktoserver(char *servhost, ushort servport) {
    int sd; /* socket descriptor */

    ushort clientport; /* port assigned to this client */

    /*
     FILL HERE
     create a TCP socket using socket()
     */
    sd = socket(AF_INET,SOCK_STREAM,0);


    /*
     FILL HERE
     connect to the server on 'servhost' at 'servport'
     use gethostbyname() and connect()
     */
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port=htons(servport);//short
    struct hostent *h = gethostbyname(servhost);
    memcpy(&sa.sin_addr.s_addr, h->h_addr, h->h_length);
    if(connect(sd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        printf("Error on connect!\n");
        exit(1);
    }


    /*
     FILL HERE
     figure out the port assigned to this client
     use getsockname()
     */
    /* get client port*/
    socklen_t len = sizeof(sa);
    if (getsockname(sd, (struct sockaddr *)&sa, &len) == -1){
        printf("Error on port assigned!\n");
        perror("getsockname error");
    }else
        clientport = ntohs(sa.sin_port);


    /*not good enough*/
    /* get client address*/
    //char myIP[16];
    //inet_ntop(AF_INET, &sa.sin_addr, myIP, sizeof(myIP));
    //printf("Client ip address: %s\n", myIP);

    /* get server address*/
    struct sockaddr_in s;
    socklen_t sLen = sizeof(s);
    getpeername(sd, (struct sockaddr*) &s, &sLen);
    printf("server address: %s\n", inet_ntoa(s.sin_addr));

    /* succesful. return socket descriptor */
    printf("admin: connected to server on '%s' at '%hu' thru '%hu'\n", servhost,
            servport, clientport);
    printf(">");
    fflush(stdout);
    return (sd);
}
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/
int readn(int sd, char *buf, int n) {
    int toberead;
    char * ptr;

    toberead = n;
    ptr = buf;
    while (toberead > 0) {
        int byteread;

        byteread = read(sd, ptr, toberead);
        if (byteread <= 0) {
            if (byteread == -1)
                perror("read");
            return (0);
        }

        toberead -= byteread;
        ptr += byteread;
    }
    return (1);
}

char *recvtext(int sd) {
    char *msg;
    long len;

    /* read the message length */
    if (!readn(sd, (char *) &len, sizeof(len))) {
        return (NULL);
    }
    len = ntohl(len);

    /* allocate space for message text */
    msg = NULL;
    if (len > 0) {
        msg = (char *) malloc(len);
        if (!msg) {
            fprintf(stderr, "error : unable to malloc\n");
            return (NULL);
        }

        /* read the message text */
        if (!readn(sd, msg, len)) {
            free(msg);
            return (NULL);
        }
    }

    /* done reading */
    return (msg);
}

int sendtext(int sd, char *msg) {
    long len;

    /* write lent */
    len = (msg ? strlen(msg) + 1 : 0);
    len = htonl(len);
    write(sd, (char *) &len, sizeof(len));

    /* write message text */
    len = ntohl(len);
    if (len > 0)
        write(sd, msg, len);
    return (1);
}
/*----------------------------------------------------------------*/
