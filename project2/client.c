/*client*/
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
#include <pthread.h>

extern int createNode(char *initHost, ushort initPort);
extern void sendHelper(int hostnum, int port, struct sockaddr_in* ans);
extern void displayRoutingTable(int row, int col, int table[row][col]);

void *transmitfuctionC(void *value);
void *transmitfuctionD(void *value);

//Node ID, hostname, control port, data port
static int config_god[5][4] = {
    {1, 4, 5000, 5001},
    {2, 4, 5002, 5003},
    {3, 1, 5000, 5001},
    {4, 2, 5002, 5003},
    {5, 3, 5004, 5005}
};
//client node: Node ID, hostname, data port
static int god_host = 7;
static int god_port = 5000;

static int routingTableRow = 5;
static int routingTableCol = 3;

int main(int argc, char *argv[]){
    int sockGod;
    if(argc != 4){
    fprintf(stderr, "usage : %s <servhost> <servport>\n", argv[0]);
        exit(1);
    }

    int inputType = atoi(argv[1]);
    int inputSour = atoi(argv[2]);
    int inputDest = atoi(argv[3]);

    //change type, fit creatNode input params type
    char hostAddr[27];
    sprintf(hostAddr,"%s%d%s", "remote0" , god_host,".cs.binghamton.edu");
    sockGod = createNode(hostAddr, god_port);
    if(sockGod == -1){
        perror("Error\n");
        exit(1);
    }

    /*-------------------------------------*/
    fd_set rfds;
    struct timeval tv;

    struct sockaddr_in sendAddr;
    sendAddr.sin_family = AF_INET;
    FD_ZERO (&rfds);
    FD_SET (sockGod, &rfds);
    struct sockaddr_in remaddr;
    socklen_t addrlen = sizeof(remaddr);

    //while(1){
        FD_ZERO(&rfds);
        FD_SET (sockGod, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        select(sockGod+1, &rfds, NULL, NULL, &tv);

        int recvArr[routingTableRow][routingTableCol];
        for(int i = 0 ; i<routingTableRow; i++){
            for(int j = 0; j<routingTableCol; j++){
                recvArr[i][j] = 0;
            }
        }

        if(FD_ISSET(sockGod, &rfds)){
            recvfrom(sockGod, recvArr, sizeof(recvArr), 0, (struct sockaddr *)&remaddr, &addrlen);
            printf("Receive routing table\n");
            displayRoutingTable(routingTableRow, routingTableCol, recvArr);
        }else{
            int packet[5];
            int packetSize = sizeof(packet);
            packet[0] = inputType;
            packet[1] = inputSour;
            packet[2] = inputDest;
            packet[3] = 100;
            packet[4] = 100;

            for(int i=0; i<5; i++){
                int tempHostnum = config_god[i][1];
                int tempPort = config_god[i][3];
                struct sockaddr_in testaddr;
                socklen_t testaddrlen = sizeof(testaddr);
                sendHelper(tempHostnum, tempPort, &testaddr);
                sendto(sockGod, packet, packetSize, 0, (struct sockaddr *)&testaddr, testaddrlen);
            }
        }
    //}
}
