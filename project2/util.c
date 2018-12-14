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

int createNode(char *initHost, ushort initPort){
    int sd;
    char *host;
    //ushort port;
    struct sockaddr_in sa;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    sa.sin_family = AF_INET;
    sa.sin_port=htons(initPort);
    //sa.sin_addr.s_addr = htonl(initHost);
    struct hostent *h = gethostbyname(initHost);
    memcpy(&sa.sin_addr.s_addr, h->h_addr, h->h_length);
    int testBind = bind(sd, (struct sockaddr *) &sa, sizeof(sa));
    printf("Bind >> %d\n", testBind);
    if(testBind == -1) printf("error in bind\n");

    //test: get host name
    char hostname[128];
    gethostname(hostname, sizeof(hostname));
    printf("hostname >> %s\n", hostname);

    //test: get port name
    socklen_t len = sizeof(sa);
    ushort testPort;
    if(getsockname(sd, (struct sockaddr *)&sa, &len)==-1)
        printf("read sock name error\n");
    else
        testPort = ntohs(sa.sin_port);
    printf("port >> %d\n", testPort);

    printf("return sd >> %d\n", sd);
    return (sd);
}

void sendHelper(int hostnum, int port, struct sockaddr_in* pans){

    //bzero(&ans, sizeof(ans));
    pans->sin_family = AF_INET;//same
    char hostAddr[27];
    sprintf(hostAddr,"%s%d%s", "remote0" ,hostnum, ".cs.binghamton.edu");
    struct hostent *h = gethostbyname(hostAddr);
    memcpy(&pans->sin_addr.s_addr, h->h_addr, h->h_length);
    pans->sin_port = htons(port);//different
}

void displayRoutingTable(int row, int col, int table[row][col]){
    printf("===============Routing Table===============\n");
    printf("destination    next hop     distance \n");
    for(int i=0; i<row; i++){
        printf("%-10d   %-10d   %-10d\n", table[i][0], table[i][1], table[i][2]);
        printf("-----------------------------------------------\n");
    }
    printf("\n");
}

void clearRoutingTable(int rt[5][3]){
    for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
            rt[i][j] = 0;
        }
    }

}
