/* functions to connect proxy */
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

/*--------------------------------------------------------------------*/
/* prepare proxy to accept requests
   returns file descriptor of socket
   returns -1 on error
*/

int startProxy() {
    int sd;
    char * proxyHost;
    ushort proxyPort;
    struct sockaddr_in sa;
    //create a TCP socket using socket()
    sd = socket(AF_INET, SOCK_STREAM, 0);
    //printf("Proxy sd >> %d\n", sd);
    //bind the socket to some port using bind()
    //let the system choose a port
    sa.sin_family = AF_INET;
    sa.sin_port = htons(0);//If you don’t care about the port
    sa.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY == 0
    int testBind = bind(sd, (struct sockaddr *) &sa, sizeof(sa));
    //printf("Bind return >> %d\n", testBind);
    if(testBind == -1){
        printf("error in bind");
    };

    /* we are ready to receive connections */
    //int backlog: length of the pending connection queue
    int listenResult = listen(sd, 5);

    /*
     figure out the full host name (servhost)
     use gethostname() and gethostbyname()
     full host name is remote**.cs.binghamton.edu
    */
    char hostname[128];
    proxyHost = hostname;
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
        proxyPort = ntohs(sa.sin_port);

    /* ready to accept requests */
    printf("admin: started proxy on '%s.cs.binghamton.edu' at '%hu'\n", proxyHost, proxyPort);
    //free(servhost);
    return (sd);
}

int hooktoserver(char *servhost, ushort servport, char *IP) {
    int sd;
    ushort clientport; /* port assigned to this client */
    sd = socket(AF_INET,SOCK_STREAM,0);
    /*
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
    figure out the port assigned to this client
    use getsockname()
    */
    socklen_t len = sizeof(sa);
    if (getsockname(sd, (struct sockaddr *)&sa, &len) == -1){
        printf("Error on port assigned!\n");
        perror("getsockname error");
    }else
        clientport = ntohs(sa.sin_port);

    /* get server address*/
    struct sockaddr_in s;
    socklen_t sLen = sizeof(s);
    getpeername(sd, (struct sockaddr*) &s, &sLen);
    char* temp = inet_ntoa(s.sin_addr);
    strncpy(IP, temp, strlen(temp));
    //printf("server address: %s\n", IP);

    /* succesful. return socket descriptor */
    //printf("admin: connected to server on '%s' at '%hu' thru '%hu'\n", servhost, servport, clientport);
    fflush(stdout);
    return (sd);
}

int readsimple(int sd, char *buf, int n) {
    read(sd, buf, n);
    return(1);
}

char *recvsimple(int sd){
    char *msg;
    int len = 8192;
    msg = (char *) malloc(len);
    if (!readsimple(sd, msg, len)) {
        free(msg);
        return (NULL);
    }
    return (msg);
}


int readn(int sd, void *buf, int n) {
    int toberead;
    void * ptr;
    toberead = n;
    ptr = buf;

    while(toberead>0){
        int byteread = 0;
        //byteread = read(sd, ptr, toberead);

        byteread = recv(sd, ptr, toberead,0);
        if(byteread<=0){
            if(byteread == -1)
                perror("read");
            return (0);
        }
        toberead -= byteread;
        ptr += byteread;
    }
    //read(sd, ptr, 8192);
    recv(sd, ptr, 8192 ,0);
    return (1);
}

void *recvtext(int sd, int size) {
    void *msg;
    long len;
    if(size == 0){
        len = 8000;
    }else
        len = (long)size;

    //printf("before malloc\n");
    msg = malloc(len+500);
    //printf("after malloc\n");
    if (!msg) {
        fprintf(stderr, "error : unable to malloc\n");
        return (NULL);
    }

    if (!readn(sd, msg, len)) {
        free(msg);
        return (NULL);
    }

    /* done reading */
    return (msg);
}

int sendtext(int sd, void *msg, int n) {
    long len;
    if(n == 0){
        len = strlen(msg);
    }else{
        len = n;
    }
    /* write lent */
    //len = htonl(len);
    /* write message text */
    //len = ntohl(len);
    if (len > 0)
        write(sd, msg, len);
    return (1);
}

int hostnameToIP(char * hostname, char* ip){
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if((he=gethostbyname(hostname))==NULL){
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++){
        strcpy(ip, inet_ntoa(*addr_list[i]));
        return 0;
    }
    return 1;
}
