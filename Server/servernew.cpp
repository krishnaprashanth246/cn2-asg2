#include <bits/stdc++.h>
// #include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// #include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <thread>
#include <mutex>

using namespace std;

#define FILEPATH "CS3543_100MB"
#define MAXBUFSIZE 1400
#define MAXSEND 20

mutex lk;

typedef struct pkt
{
    uint32_t ts;          /* time stamp */
    uint32_t dataSize;    /* datasize , normally it will be 1460*/
    uint32_t seqNum;      /* store sequence # */
    int retransmit;
    char buf[MAXBUFSIZE]; /* Data buffer */
} pkt;

char *allNodes;
int iteratingptr;
char *data;
struct stat statbuf;

struct timespec start, stop;
double totaltime;

int datarecv = 0;
int sockfd;
thread thrd[1];
int filesize;
struct sockaddr_in serv_addr;

int totalPacketsArrived = 0, totalSeq = 0;


void error(const char *msg)
{
    cerr << msg;
    exit(1);
}

void startSendingBack(void *arg)
{
    int totalseq, lefbyte, lastseq, size_p, n = 0;
    pkt sendDG;
    int packetmiss, errn;
    socklen_t servlen;

    servlen = sizeof(serv_addr);

    cout<<"enter startSendingBack client"<<endl;
    //fflush(stdout);
    // sleep(10);
    for(;true;)
    {

        n = recvfrom(sockfd, &packetmiss, sizeof(int), 0, (struct sockaddr *)&serv_addr, &servlen);
        //    printf("packetmiss:: %d\n",packetmiss);
        if (n < 0)
        {
            error("rcv from\n");
        }
        if (packetmiss < 0)
        {
            cout<<"data recv fully"<<endl;
            //pthread_exit(0);
            return;
        }
        totalseq = filesize / MAXBUFSIZE;
        lefbyte = filesize % MAXBUFSIZE;
        //
        size_p = MAXBUFSIZE;
        if (lefbyte != 0 && totalseq == packetmiss)
        {
            size_p = lefbyte;
        }
        std::unique_lock<std::mutex> lck(lk);

        memcpy(sendDG.buf, &data[packetmiss * MAXBUFSIZE], size_p);
        lck.unlock();

        sendDG.seqNum = packetmiss;
        sendDG.dataSize = size_p;
        sendDG.retransmit = 1;

        n = sendto(sockfd, &sendDG, sizeof(pkt), 0, (struct sockaddr *)&serv_addr, servlen);
        if (n < 0)
            error("sendto");
    }
}

char *openFILE(char *str)
{
    int fp;
    int pagesize;
    char *data;

    fp = open(str, O_RDONLY);
    if (fp < 0)
    {
        error("Error: while opening the file\n");
    }

    /* get the status of the file */
    if (fstat(fp, &statbuf) < 0)
    {
        error("ERROR: while file status\n");
    }

    /* mmap it , data is a pointer which is mapping the whole file*/
    data = (char*) mmap((caddr_t)0, statbuf.st_size, PROT_READ, MAP_SHARED, fp, 0);

    if (data == MAP_FAILED)
    {
        error("ERROR : mmap \n");
    }

    return data;
}

int iterateArrayAgain()
{
    int i;
    if (allNodes == NULL)
    {
        return -1;
    }
    for (i = iteratingptr; i <= totalSeq; i++)
    {
        if (allNodes[i] == 0)
        {
            if (i == totalSeq)
                iteratingptr = 0;
            else
                iteratingptr = (i + 1);
            return i;
        }
    }
    iteratingptr = 0;
    return -1;
}

void checkme(void *arg)
{
    int n = 0; // , i = 0;

    //  LinkedList *requestNodePointer = anchor;
    //int ack = -1;
    int requestIndex;
    printf("enter check me server\n");
    for(;true;)
    {
        if (totalPacketsArrived == totalSeq + 1)
        {
            printf("all data received!!\n");
            return;
        }
        usleep(100);
        std::unique_lock<std::mutex> lck(lk);
        requestIndex = iterateArrayAgain();
        lck.unlock();
        if (requestIndex < 0)
        {
            // sleep(1);
        }
        else if (requestIndex >= 0 && requestIndex <= totalSeq)
        {
            n = sendto(sockfd, &requestIndex, sizeof(int), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

            if (n < 0)
                error("sendto\n");
        }
    }
}

int deleteNodesFromArray(char **arrayNode, int seqNum)
{

    if (seqNum >= 0 && seqNum <= totalSeq)
    {
        if (allNodes[seqNum] == 0)
        {
            allNodes[seqNum] = 1;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int portno, i = 0;
    socklen_t fromlen;
    int n;
    FILE *fp;
    int errn = 0;
    int last = -1, seq = 0;
    //int diff=0;

    pkt recvDG;
    int ack = -1;

    int datasize = 0;
    // anchor=NULL;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    long int sndsize = 50000000;

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&sndsize, (long int)sizeof(sndsize)) == -1)
    {
        printf("error with setsocket");
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&sndsize, (long int)sizeof(sndsize)) == -1)
    {
        printf("error with setsocket");
    }

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    fromlen = sizeof(struct sockaddr_in);
    /* prerequisite */
    n = recvfrom(sockfd, &filesize, sizeof(filesize), 0, (struct sockaddr *)&serv_addr, &fromlen);
    totalSeq = ceil(filesize * 1.0 / MAXBUFSIZE);
    cout << "Seqtotal::" << totalSeq << " filesize = " << filesize << '\n';

    fp = fopen(FILEPATH, "w+");

    thrd[0] = thread(checkme, (void*)0);
    // if ((errn = pthread_create(&thrd[0], 0, checkme, (void *)0)))
    // {
    //     fprintf(stderr, "pthread_create[0] %s\n", strerror(errn));
    //     pthread_exit(0);
    // }
    

    allNodes = (char *)calloc(totalSeq, sizeof(char));

    /*  last = totalSeq;
     for (i=last;i>=0;i--)
     addNodes(&anchor,i);*/

    if (clock_gettime(CLOCK_REALTIME, &start) == -1)
    {
        error("clock get time");
    }

    for(;true;)
    {
        //fflush(stdout);
        //printNodes(&anchor);

        n = recvfrom(sockfd, &recvDG, sizeof(pkt), 0, (struct sockaddr *)&serv_addr, &fromlen);

        if (n == sizeof(int))
        {
            continue;
        }
        if (n < 0)
            error("recvfrom");

        std::unique_lock<std::mutex> lck1(lk);
        if (deleteNodesFromArray(&allNodes, recvDG.seqNum))
        {

            fseek(fp, MAXBUFSIZE * recvDG.seqNum, SEEK_SET);
            fwrite(&recvDG.buf, recvDG.dataSize, 1, fp);
            fflush(fp);
            totalPacketsArrived++;
        }
        else
        {
        }
        lck1.unlock();
        if (totalPacketsArrived == totalSeq + 1)
        {
            printf("we got the whole file!! wow\n");
            for (i = 0; i < MAXSEND; i++)
            {
                printf("sending ack\n");
                n = sendto(sockfd, &ack, sizeof(int), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if (n < 0)
                    error("sendto");
            }

            break;
        }
    }
    
    if(clock_gettime(CLOCK_REALTIME,&stop) == -1){
        error("clock get time stop");
    }

    //totaltime = (stop.tv_sec - start.tv_sec)+(double)(stop.tv_nsec - start.tv_nsec)/1e9;

    //printf("Time taken to complete : %f sec",totaltime);

    thrd[0].join();

    fclose(fp);
    totaltime = (stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / 1e9;

    printf("Time taken to complete : %f sec", totaltime);

    munmap(data, statbuf.st_size);

    close(sockfd);
    return 0;
}
