/*--------------------------------------------------------------------*/
/* conference server */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

extern char * recvtext(int sd);
extern int sendtext(int sd, char *msg);

extern int startserver();
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
int fd_isset(int fd, fd_set *fsp) {
    return FD_ISSET(fd, fsp);
}
/* main routine */
int main(int argc, char *argv[]) {
    char * msg = 0; // test
    //int csd = 0;
    int servsock; /* server socket descriptor */

    fd_set livesdset; /* set of live client sockets */
    fd_set bufferSet;
    int livesdmax; /* largest live client socket descriptor */
    struct sockaddr_in sa;

    /* check usage */
    if (argc != 1) {
        fprintf(stderr, "usage : %s\n", argv[0]);
        exit(1);
    }

    /* get ready to receive requests */
    servsock = startserver();
    livesdmax = servsock;
    if (servsock == -1) {
        perror("Error on starting server: ");
        exit(1);
    }

    /*
     FILL HERE:
     init the set of live clients
     */
    FD_ZERO (&livesdset);
    FD_SET (servsock, &livesdset);
    FD_SET (servsock, &bufferSet);

    struct timeval tv;
    int sret = 0;

    /* receive requests and process them */
    while (1) {
        int frsock; /* loop variable */
        FD_ZERO (&livesdset);
        //FD_SET (servsock, &livesdset);
        memcpy(&livesdset, &bufferSet, sizeof(livesdset));
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        /*
         FILL HERE
         wait using select() for
         messages from existing clients and
         connect requests from new clients
         */

        sret = select(livesdmax+1, &livesdset, NULL, NULL, &tv);
        /* look for messages from live clients */
        for (frsock = 3; frsock <= livesdmax; frsock++) {
            /* skip the listen socket */
            /* this case is covered separately */
            if (frsock == servsock)
                continue;

            // FILL HERE: message from client 'frsock'
            if (FD_ISSET(frsock, &livesdset)) {
                char * clienthost; /* host name of the client */
                ushort clientport; /* port number of the client */

                /*
                 FILL HERE:
                 figure out client's host name and port
                 using getpeername() and gethostbyaddr()
                 */
                socklen_t len = sizeof(sa);// I am not sure about sa....just try

                if(getpeername(frsock,(struct sockaddr *)&sa, &len)==-1)
                    perror("getpeername error");
                else
                    clientport = ntohs(sa.sin_port);

                struct hostent* temp;
                unsigned long ipAddr = sa.sin_addr.s_addr;
                temp = gethostbyaddr((const char*)&ipAddr, 4, AF_INET);
                clienthost = temp->h_name;


                /* read the message */
                msg = recvtext(frsock);
                if (!msg) {
                    /* disconnect from client */
                    printf("admin: disconnect from '%s(%hu)'\n", clienthost,
                            clientport);

                    /*
                     FILL HERE:
                     remove this guy from the set of live clients
                     */
                    FD_CLR(frsock, &livesdset);
                    FD_CLR(frsock, &bufferSet);

                    /* close the socket */
                    close(frsock);
                } else {
                    /*
                     FILL HERE
                     send the message to all live clients
                     except the one that sent the message
                     */

                    for(int i = 4; i<= livesdmax; i++){
                        if(i == frsock || i == servsock)
                            continue;
                        sendtext(i, msg);
                    }



                    /* display the message */
                    printf("%s(%hu): %s", clienthost, clientport, msg);

                    /* free the message */
                    free(msg);
                }
            }
        }

        /* look for connect requests */
        // FILL HERE: connect request from a new client
        if (FD_ISSET(servsock, &livesdset)){
            /*
             FILL HERE:
             accept a new connection request
             */
            int salen = sizeof(sa);
            int csd = accept(servsock, (struct sockaddr *)&sa, &salen);


            /* if accept is fine? */
            if (csd != -1) {

                char * clienthost; /* host name of the client */
                ushort clientport; /* port number of the client */

                /*
                 FILL HERE:
                 figure out client's host name and port
                 using gethostbyaddr() and without using getpeername().
                 */

                struct hostent* res;
                unsigned long ipAddr = sa.sin_addr.s_addr;
                res = gethostbyaddr((const char*)&ipAddr, 4, AF_INET);

                clienthost = res->h_name;
                clientport = ntohs(sa.sin_port);


                printf("admin: connect from '%s' at '%hu'\n", clienthost,
                        clientport);

                /*
                 FILL HERE:
                 add this guy to set of live clients
                 */
                livesdmax = csd;
                FD_SET (csd, &livesdset);
                FD_SET (csd, &bufferSet);


            } else {
                perror("accept");
                exit(0);
            }
        }
    }
    return 0;
}
/*--------------------------------------------------------------------*/
