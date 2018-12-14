/*--------------------------------------------------------------------*/
/* proxy server */

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

extern int startProxy();
extern int hooktoserver(char *servhost, ushort servport, char *IP);
extern void * recvtext(int sd, int size);
extern int sendtext(int sd, void *msg, int n);
extern void * recvsimple(int sd);
//extern int hostnameToIP(char * hostname, char* ip);
/*--------------------------------------------------------------------*/

int fd_isset(int fd, fd_set *fsp) {
    return FD_ISSET(fd, fsp);
}

void getFullPath(char * str, char * res){
    char * a = strstr(str, "http");
    int b = strcspn(a, " ");
    strncpy(res, a, b);
    res[b] = '\0';
}
void getHostPath(char * str, char* res){
    char * a = strstr(str, "Host: ");
    int b = strcspn(a, "\n");
    char path[b-1];// I don't know why b is one larger than what I expect...
    //so from here, any place where use b should be minus one more....werid...
    char realPath[b-6-1];
    strncpy(path, a, b);
    for(int i = 0; i<b-6-1; i++){
        realPath[i] = path[i+6];
    }
    strncpy(res, realPath, b-6-1);
    res[b-6-1] = '\0';
}

void  getSubPath(char * fullPath, char * hostPath, char * res){
    int len = strlen(hostPath);
    for(int i = 0; i<len; i++){
        fullPath[i] = 'a';
    }
    char * path = strstr(fullPath, "/");
    int diffL = strlen(fullPath) - len;
    strncpy(res, path, diffL);
    res[diffL] = '\0';
}

void getToGet(char * str, char * res, char * host){
    res[0] = 'G';
    res[1] = 'E';
    res[2] = 'T';
    res[3] = ' ';
    int hostlen = strlen(host);
    int totallen = strlen(str);
    for(int i = 4; i+hostlen+7<totallen; i++){
        *(res+i) = *(str+i+hostlen+7);
    }
    res[totallen] = '\0';
}

void getToHead(char * str, char * res, char * host){
    res[0] = 'H';
    res[1] = 'E';
    res[2] = 'A';
    res[3] = 'D';
    res[4] = ' ';
    int hostlen = strlen(host);
    int totallen = strlen(str);
    for(int i = 4; i+hostlen+7<totallen; i++){
        *(res+i+1) = *(str+i+hostlen+7);
    }
    res[totallen] = '\0';
}

int getLength(char * str){
    char * a = strstr(str, "Content-Length:");
    char * b = strstr(a, " ");
    int c = strcspn(b, "\n");
    char number[100];
    for(int i = 1; i<c; i++){
        number[i-1] = *(b+i);
    }
    return atoi(number);
}

int getPort(char * str){
    int port;
    char number[10];
    char * a = strstr(str, ":");
    if(!a){
        port = 80;
    }else{
        int c = strcspn(a, "\n");
        for(int i = 1; i<c; i++){
            number[i-1] = *(a+i);
        }
        port = atoi(number);
    }
    return (port);
}


int main(int argc, char *argv[]) {
    char * msg = 0;
    int proxySock; // server socket descriptor

    fd_set livesdset; // set of live client sockets
    fd_set bufferSet;
    int livesdmax; // largest live client socket descriptor
    struct sockaddr_in sa;
    clock_t t;

    //using for cashe
    struct Contents{
        char url[200];
        char IP[100];
        int size;
        void * doc;
    } contentArr[5000];

    int arrHead = 0;
    int arrTail = 0;
    int begin = 0;
    int end = 999;

    /* check usage */
    if (argc != 2) {
        printf("Please input the size of cache\n");
        exit(1);
    }
    int cacheSize = atoi(argv[1]);
    int restSize = cacheSize;

    printf("cache size: %d\n", cacheSize);

    /* get ready to receive requests */
    proxySock = startProxy();
    livesdmax = proxySock;
    if (proxySock == -1) {
        perror("Error on starting server: ");
        exit(1);
    }

    /*
     FILL HERE:
     init the set of live clients
    */
    FD_ZERO (&livesdset);
    FD_SET (proxySock, &livesdset);
    FD_SET (proxySock, &bufferSet);

    struct timeval tv;
    //int sret = 0;

    /* receive requests and process them */
    while(1){
        char * cacheStatus = "CACHE_MISS";
        FD_ZERO (&livesdset);
        FD_SET (proxySock, &livesdset);
        //memcpy(&livesdset, &bufferSet, sizeof(livesdset));//left 2 right
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        select(proxySock+1, &livesdset, NULL, NULL, &tv);
        if (FD_ISSET(proxySock, &livesdset)) {
            //printf("connect OK\n");
            t = clock();
            int salen = sizeof(sa);
            // csd is client sd
            int csd = accept(proxySock, (struct sockaddr *)&sa, &salen);
            //printf("csd >> %d\n", csd);
            // csd should add in select set
            char *msg;
            msg = recvsimple(csd);// from client (brower)
            //printf("out >>>>>>>>>>>>>>\n%s\n", msg);
            char totalPath[200] = {'\0'};
            char totalPathWithoutPort[200] = {'\0'};
            char host[200] = {'\0'};
            char subPath[200] = {'\0'};
            char newGet[1000] = {'\0'};
            char clientIP[100] = {'\0'};

            getFullPath(msg, totalPath);
            //printf("totalPath >> %s\n", totalPath);
            getHostPath(msg, host);
            //printf("host >> %s\n", host); // no need
            getSubPath(totalPath, host, subPath);
            //printf("subPath >> %s\n", subPath);

            getToGet(msg, newGet, host);
            //printf("newGet >>\n%s\n", newGet);

            int port = getPort(host);
            //printf("port >> %d\n", port);

            //------hook to server-------
            int hookSock;
            fd_set hookRfds;
            int hookRetval;

            /* get hooked on to the server */
            //make host with out port number
            char * ptr = strstr(host, ":");
            if(ptr){
                int pos = strcspn(host, ":");
                *(host+pos) = '\0';
            }
            //printf("host >> %s\n", host);
            strncpy(totalPathWithoutPort, host, strlen(host));
            strncpy(totalPathWithoutPort+strlen(host), subPath, strlen(subPath));
            //printf("totalPathWithoutPort >> %s\n", totalPathWithoutPort);


            /*check exist in cache*/
            int docLen = 0;
            void * docFinal = NULL;
            for(int i=arrHead; i<arrTail; i++){
                //printf("in loop >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %d\n", i);
                //printf("array left >> %s\n", contentArr[i].url);
                //printf("new right >> %s\n", totalPathWithoutPort);
                if(strcmp(contentArr[i].url, totalPathWithoutPort) == 0){
                    //printf("XXXX\n");
                    docFinal = contentArr[i].doc;
                    docLen = contentArr[i].size;
                    strncpy(clientIP,contentArr[i].IP, strlen(contentArr[i].IP));
                    cacheStatus = "CACHE_HIT";
                    break;
                }
            }
            //printf("BBB\n");
            if(!docFinal){
                hookSock = hooktoserver(host, port, clientIP);
                if (hookSock == -1) {
                    perror("Error: ");
                    exit(1);
                }
                //---------------------------

                char * headFromHttp;
                //printf("sendtext Head feedback(1: ok) >> %d\n",sendtext(hookSock, newGet, 0));
                sendtext(hookSock, newGet, 0);
                headFromHttp = recvsimple(hookSock);
                docLen =  getLength(headFromHttp);
                close(hookSock);
                hookSock = hooktoserver(host, port, clientIP);
                //printf("sendtext Get feedback(1: ok) >> %d\n",sendtext(hookSock, newGet, 0));
                sendtext(hookSock, newGet, 0);
                docFinal = recvtext(hookSock, docLen);
                //printf("close socket to http >> %d\n",close(hookSock));
                close(hookSock);

                /*store this new doc in cache*/
                while(restSize - docLen < 0){
                    //printf("restSize >> %d\n", restSize);
                    restSize += contentArr[arrHead].size;
                    //printf("before\n");
                    for(int i = 0; i< strlen(contentArr[arrHead].url); i++){
                        *(contentArr[arrHead].url+i) = '\0';
                    }
                    for(int i = 0; i< strlen(contentArr[arrHead].IP); i++){
                        *(contentArr[arrHead].IP+i) = '\0';
                    }
                    contentArr[arrHead].size = 0;
                    //printf("mid\n");
                    //free(contentArr[arrHead].doc);
                    //printf("after\n");
                    arrHead++;
                }

                if(restSize - docLen >= 0){
                    restSize -= docLen;
                    printf("rest cache size >> %d\n", restSize);
                    strncpy(contentArr[arrTail].url, totalPathWithoutPort, strlen(totalPathWithoutPort));
                    strncpy(contentArr[arrTail].IP, clientIP, strlen(clientIP));
                    contentArr[arrTail].size = docLen;
                    contentArr[arrTail].doc = docFinal;
                    arrTail++;
                }
            }

            //--------send back to user-----
            //printf("sendtext feedback to client(1:ok)>> %d\n",sendtext(csd, docFinal, docLen+8192));
            //printf("close socket to client>> %d\n",close(csd));
            sendtext(csd, docFinal, docLen+8192);
            close(csd);
            t = clock() - t;


            //standard output
            char reqURL[200];
            double time = ((double)t*1000)/CLOCKS_PER_SEC;
            getFullPath(msg, reqURL);
            printf("\n%s|%s|%s|%d|%.3lf\n\n", clientIP, reqURL, cacheStatus, docLen, time);

        }else{
            printf("running...\n");
        }
    }
}
