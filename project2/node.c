/*node*/
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
extern void clearRoutingTable(int rt[5][3]);

void *transmitfuctionC(void *value);
void *transmitfuctionD(void *value);

//configuration file:
//Node ID, hostname, control port, data port, nb1, nb2, nb3, nb4
static int config[5][9] = {
    {1, 4, 5000, 5001, -1, 2, -1, 4, 5},
    {2, 4, 5002, 5003, 1, -1, 3, -1, -1},
    {3, 1, 5000, 5001, -1, 2, -1, -1, 5},
    {4, 2, 5002, 5003, 1, -1, -1, -1, 5},
    {5, 3, 5004, 5005, 1, -1, 3, 4, -1}
};

static int bias = 3;

static int countR = 0;
static int countS = 0;

static int routingTable[5][3] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};
static int routingTableRow = 5;
static int routingTableCol = 3;

static int packetID = 0;

static int god_host = 7;
static int god_port = 5000;

int main(int argc, char *argv[]){

    //input type: ./node id
    int sockC;
    int sockD;
    pthread_t threadC;
    pthread_t threadD;

    if(argc != 2){
        fprintf(stderr, "usage : %s <servhost> <servport>\n", argv[0]);
        exit(1);
    }

    // read in from run param
    int nodeId = atoi(argv[1]);

    // read params from config table
    char hostnum = config[nodeId-1][1];
    int portC = config[nodeId-1][2];
    int portD = config[nodeId-1][3];

    //initialize routing table
    routingTable[nodeId-1][0] = nodeId;
    routingTable[nodeId-1][1] = nodeId;

    //change type, fit creatNode input params type
    char hostAddr[27];
    sprintf(hostAddr,"%s%d%s", "remote0" ,hostnum, ".cs.binghamton.edu");

    sockC = createNode(hostAddr, portC);
    sockD = createNode(hostAddr, portD);

    if(sockC == -1){
        perror("Error\n");
        exit(1);
    }
    if(sockD == -1){
        perror("Error\n");
        exit(1);
    }
    int paramsC[2] = {sockC, nodeId};
    int paramsD[2] = {sockD, nodeId};

    pthread_create(&threadC, NULL, transmitfuctionC, paramsC);
    pthread_create(&threadD, NULL, transmitfuctionD, paramsD);

    while(1);
    //pthread_join(threadC, NULL);
    //pthread_join(threadD, NULL);

}


void *transmitfuctionC(void *value){
    int sock = ((int*)value)[0];
    int nid = ((int*)value)[1];
    fd_set rfds;
    struct timeval tv;
    int counter = 1;

    //send part
    struct sockaddr_in sendAddr;
    sendAddr.sin_family = AF_INET;

    FD_ZERO (&rfds);
    FD_SET (sock, &rfds);
    struct sockaddr_in remaddr;
    socklen_t addrlen = sizeof(remaddr);

    while(1){
        FD_ZERO(&rfds);
        FD_SET (sock, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        select(sock+1, &rfds, NULL, NULL, &tv);

        int recvArr[routingTableRow][routingTableCol];
        for(int i = 0 ; i<routingTableRow; i++){
            for(int j = 0; j<routingTableCol; j++){
                recvArr[i][j] = 0;
            }
        }
        if(counter%10!=0 && FD_ISSET(sock, &rfds)){

            recvfrom(sock, recvArr, sizeof(recvArr), 0, (struct sockaddr *)&remaddr, &addrlen);
            for(int i=0; i<routingTableRow; i++){
                int dest = recvArr[i][0];
                int nhop = recvArr[i][1];
                int dist = recvArr[i][2];
                if(dest > 0 && routingTable[dest-1][0] == 0){ //debug
                    routingTable[dest-1][0] = dest;
                    routingTable[dest-1][1] = nhop;
                    routingTable[dest-1][2] = dist;
                }else if(dest > 0 && routingTable[dest-1][0] == nid){ //debug
                    //do nothing
                }else if(dest > 0 && routingTable[dest-1][0]==dest && routingTable[dest-1][1]==nhop){ //debug
                    routingTable[dest-1][2] = dist;
                }else if(dest > 0 && routingTable[dest-1][0]==dest && routingTable[dest-1][1]!=nhop){ //debug
                    if(routingTable[dest-1][2] > dist){
                        routingTable[dest-1][2] = dist;
                        routingTable[dest-1][1] = nhop;
                    }//else do nothing
                }
            }
            countR++;
            counter++;
            //printf("countR >> %d\n", countR);
        }else{
            counter = 1;
            int tempId;
            for(int i=4; i<=8; i++){
                int tempId = config[nid-1][i];
                if(tempId != -1 ){
                    int tempHostnum = config[tempId-1][1];
                    int tempPort = config[tempId-1][2];

                    struct sockaddr_in testaddr;
                    socklen_t testaddrlen = sizeof(testaddr);
                    sendHelper(tempHostnum, tempPort, &testaddr);

                    int senderArr[routingTableRow][routingTableCol];
                    for(int i = 0 ; i<routingTableRow; i++){
                        for(int j = 0; j<routingTableCol; j++){
                            senderArr[i][j] = 0;
                        }
                    }
                    for(int i=0; i<routingTableRow; i++){
                        if(routingTable[i][0]!=0){
                            for(int j=0; j<routingTableCol; j++){
                                if(j==1){
                                    senderArr[i][j] = nid;
                                }else if(j==0){
                                    senderArr[i][j] = routingTable[i][j];
                                }else if(j==2){
                                    senderArr[i][j] = routingTable[i][j] + 1;
                                }
                            }
                        }
                    }
                    sendto(sock, senderArr, sizeof(senderArr), 0, (struct sockaddr *)&testaddr, testaddrlen);
                }
            }
            countS++;
            //printf("countS >> %d\n", countS);
        }
    }
}

void *transmitfuctionD(void *value){
    int sock = ((int*)value)[0];
    int nid = ((int*)value)[1];
    fd_set rfds;
    struct timeval tv;

    //send part
    struct sockaddr_in sendAddr;
    sendAddr.sin_family = AF_INET;

    FD_ZERO (&rfds);
    FD_SET (sock, &rfds);
    struct sockaddr_in remaddr;
    socklen_t addrlen = sizeof(remaddr);
    int base_ttl = 0;
    int routetraceS = -1;
    int routetraceD = -1;

    while(1){
        FD_ZERO(&rfds);
        FD_SET (sock, &rfds);
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        select(sock+1, &rfds, NULL, NULL, &tv);
        // type : Source node id : Destination node id : Packet id : TTL
        int packet[5];
        int packetSize = sizeof(packet);
        int reclen = 0;
        if(FD_ISSET(sock, &rfds)){
            reclen = recvfrom(sock, packet, packetSize, 0, (struct sockaddr *)&remaddr, &addrlen);
            int type = packet[0];
            int s_id = packet[1];
            int d_id = packet[2];
            int p_id = packet[3];
            int ttl  = packet[4];
            //printf("Rec packet\n");
            //printf("type >>> %d\n", type);
            //printf("s_id >>> %d\n", s_id);
            //printf("d_id >>> %d\n", d_id);
            //printf("p_id >>> %d\n", p_id);
            //printf("ttl  >>> %d\n", ttl );
            //printf("nid  >>> %d\n", nid );

            struct sockaddr_in testaddr;
            socklen_t testaddrlen = sizeof(testaddr);
            int tempHostnum = -1;
            int tempPort = -1;
            int nextHop = -1;
            int flag = 0;

            if(type == 1){
                // send the package to right node
                if(d_id == nid){ // may at start node or dest node
                    //if(ttl > 0){ // at start node
                    if(nid == routetraceS){ // at start node
                        if(s_id == routetraceD){ // feedback from dest node
                            printf("============= End: %d\n", routetraceD);
                        }else{ // feedback from intermedia node
                            printf("============= Intermedia: %d\n", s_id);
                            flag = 1;
                            base_ttl++;
                            packet[1] = routetraceS;
                            packet[2] = routetraceD;
                            packet[4] = base_ttl;
                            nextHop = routingTable[packet[2]-1][1];
                            tempHostnum = config[nextHop-1][1];
                            tempPort = config[nextHop-1][3];
                        }
                    //}else if(ttl == 0){ // at dest node
                    }else if(nid == routetraceD){ // at dest node
                        flag = 1;
                        packet[1] = nid;
                        packet[2] = routetraceS;
                        packet[4] = 15;
                        nextHop = routingTable[packet[2]-1][1];
                        tempHostnum = config[nextHop-1][1];
                        tempPort = config[nextHop-1][3];
                    }
                }else{
                    flag = 1;
                    if(ttl == 0){
                        // send back to src node
                        packet[1] = nid;
                        packet[2] = routetraceS;
                        packet[4] = 15; //default TTL
                    }else{
                        // send forword to next hop
                        //packet[1] = routetraceS;
                        //packet[2] = routetraceD;
                        packet[4] = ttl - 1;
                    }
                    nextHop = routingTable[packet[2]-1][1];
                    tempHostnum = config[nextHop-1][1];
                    tempPort = config[nextHop-1][3];
                }
            }else if(type == 10){
                // create a trace route
                routetraceS = packet[1];
                routetraceD = packet[2];
                if(s_id == nid){
                    printf("============= Start: %d\n", nid);
                    flag = 1;
                    //routetraceS = packet[1];
                    //routetraceD = packet[2];
                    //printf("routeTraceS >> %d\n", routetraceS);
                    //printf("routeTraceD >> %d\n", routetraceD);
                    base_ttl = 0;
                    packet[0] = 1;
                    packet[3] = packetID;
                    packet[4] = base_ttl;
                    nextHop = routingTable[routetraceD-1][1];
                    tempHostnum = config[nextHop-1][1];
                    tempPort = config[nextHop-1][3];
                    packetID += 1;
                }
            }else if(type == 2){
                // add new link
                // in this case:
                // packet[1] packet[2] are node numbers.
                config[packet[1] - 1][packet[2] + bias] = packet[2];
                config[packet[2] - 1][packet[1] + bias] = packet[1];
                printf("Add new link!\n");
                clearRoutingTable(routingTable);
                routingTable[nid-1][0] = nid;
                routingTable[nid-1][1] = nid;
            }else if(type == 3){
                // remove an existing link
                config[packet[1] - 1][packet[2] + bias] = -1;
                config[packet[2] - 1][packet[1] + bias] = -1;
                printf("remove a link!\n");
                clearRoutingTable(routingTable);
                routingTable[nid-1][0] = nid;
                routingTable[nid-1][1] = nid;
            }else if(type == 4){
                // display routing table
                printf("\n");
                displayRoutingTable(routingTableRow, routingTableCol, routingTable);
            }
            if(flag == 1){
            //send code
                //printf("before sendHelper tempHostnum >> %d\n", tempHostnum);
                //printf("before sendHelper tempPort >> %d\n", tempPort);
                sendHelper(tempHostnum, tempPort, &testaddr);
                sendto(sock, packet, packetSize, 0, (struct sockaddr *)&testaddr, testaddrlen);
            }
        }
    }
}
