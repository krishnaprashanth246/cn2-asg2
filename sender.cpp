#include <bits/stdc++.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <mutex>

using namespace std;
// #define filename "../../CS3543_100MB"
#define MAXSEND 10
#define PAYLOAD 1400

mutex lk;

char *data;
char *nodes;
int iteratingptr;
int filesize = 0;

struct sockaddr_in serv_addr;
struct stat statbuf;

//off_t filesize;
int totalPacketsArrived = 0, totalSeq = 0;

struct timespec start, stop;
double totaltime;

typedef struct pkt
{
    int retransmit;
    uint32_t ts;       /* time stamp */
    uint32_t seqNum;   /* store sequence # */
    uint32_t dataSize; /* datasize , normally it will be 1460*/
    char buf[PAYLOAD]; /* Data buffer */
} pkt;

thread thrd[1];
int sockfd;

void error(const char *msg)
{
    cerr << msg;
    exit(0);
}

int iterateArrayAgain()
{
    int i = iteratingptr;
    if (nodes == NULL)
    {
        return -1;
    }
    while (i <= totalSeq)
    {
        if (nodes[i] == 0)
        {
            if (i != totalSeq)
                iteratingptr = i + 1;
            else
                iteratingptr = 0;
        }
        i++;
    }
    iteratingptr = 0;
    return -1;
}

void receiveMe(void *arg)
{
    int n = 0; 
    int requestIndex;
    for (; true;)
    {
        if (totalPacketsArrived - 1 == totalSeq)
        {
            cout << "File received!" << endl;
            return;
        }
        usleep(100);

        unique_lock<mutex> lck(lk);
        requestIndex = iterateArrayAgain();
        lk.unlock();

        if (requestIndex <= totalSeq)
        {
            if (requestIndex >= 0)
            {
                n = sendto(sockfd, &requestIndex, sizeof(int), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if (!(n >= 0))
                    error("error occured in sendto\n");
            }
        }
    }
}

char *openFILE(char *str)
{
    char *data;
    int fp, pagesize;

    // fp = open(str, O_RDONLY);
    if ((fp = open(str, O_RDONLY)) < 0)
        error("Error: while opening the file\n");

    /* get the status of the file */
    if (fstat(fp, &statbuf) < 0)
        error("ERROR: while file status\n");

    /* mmap it , data is a pointer which is mapping the whole file*/
    if ((data = (char *)mmap((caddr_t)0, statbuf.st_size, PROT_READ, MAP_SHARED, fp, 0)) == MAP_FAILED)
        error("ERROR : mmap \n");

    return data;
}

void checkme(void *arg)
{
    pkt sendDG, recvDG;
    int totalseq, lefbyte, lastseq, size_p, n = 0;
    socklen_t servlen;
    int packetmiss, errn;
    servlen = sizeof(serv_addr);

    for (; true;)
    {
        if ((n = recvfrom(sockfd, &packetmiss, sizeof(int), 0, (struct sockaddr *)&serv_addr, &servlen)) < 0)
            error("error occured in  recvfrom\n");
        if (packetmiss < 0)
        {
            cout << "complete file sent" << endl;
            return;
        }
        lefbyte = filesize % PAYLOAD;
        totalseq = filesize / PAYLOAD;

        size_p = PAYLOAD;
        if (totalseq == packetmiss && lefbyte != 0)
            size_p = lefbyte;
        
        unique_lock<mutex> lck(lk);
        memcpy(sendDG.buf, &data[packetmiss * PAYLOAD], size_p);
        lck.unlock();

        sendDG.dataSize = size_p;
        sendDG.retransmit = 1;
        sendDG.seqNum = packetmiss;

        if ((n = sendto(sockfd, &sendDG, sizeof(pkt), 0, (struct sockaddr *)&serv_addr, servlen)) < 0)
            error("sendto");
    }
}

int main(int argc, char *argv[])
{

    FILE *fp = NULL;
    pkt recvDG;

    int errn = -1;
    int seq = 0, datasize = 0, ack = -1;
    int portno, n;
    int i, j;

    unsigned char buffer[PAYLOAD];
    unsigned char *chptr = NULL;

    socklen_t fromlen;

    struct stat st;
    struct hostent *server;

    if (argc != 4)
    {
        cerr << "usage:- " << argv[0] << " <hostname> <port> <filename>\n";
        exit(0);
    }
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    portno = atoi(argv[2]);
    if (sockfd < 0)
        error("ERROR opening socket\n");

    if ((server = gethostbyname(argv[1])) == NULL)
    {
        cerr << ("ERROR, no such host\n");
        exit(0);
    }

    long int sndsize = 50000000;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);


    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&sndsize, (long int)sizeof(sndsize)) == -1)
        cout << "error with setsocket" << endl;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&sndsize, (long int)sizeof(sndsize)) == -1)
        cout << "error with setsocket" << endl;

    if ((chptr = (unsigned char *)memset(buffer, '\0', PAYLOAD)) == NULL)
        error("error occured in memset:");

    if ((fp = fopen(argv[3], "r")) == NULL)
        error("error occured in fopen:");

    if (stat(argv[3], &st) == 0)
    {
        filesize = st.st_size;
        cout << "The size of this file is " << filesize << endl;
    }

    i = 0;
    while (i < MAXSEND)
    {
        n = sendto(sockfd, &filesize, sizeof(filesize), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        i++;
    }
    // cout << "filesize sent\n";

    data = openFILE(argv[3]);
    datasize = statbuf.st_size;


    thrd[0] = thread(checkme, (void*)0);
    if (clock_gettime(CLOCK_REALTIME, &start) == -1)
        error("error occured in clock get time");

    while (datasize > 0)
    {
        pkt sendDG;
        int chunk, share;
        memset(&sendDG, 0, sizeof(pkt));

        chunk = PAYLOAD;
        share = datasize;

        if (share - chunk >= 0)
            share = share - chunk;
        else
            chunk = share;

        unique_lock<mutex> lck(lk);
        memcpy(sendDG.buf, &data[seq * PAYLOAD], chunk);
        lck.unlock();

        sendDG.seqNum = seq;
        sendDG.retransmit = 0;
        sendDG.dataSize = chunk;
        usleep(100);
        sendto(sockfd, &sendDG, sizeof(sendDG), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        datasize -= chunk;
        seq++;
    }

    thrd[0].join();

    if(clock_gettime(CLOCK_REALTIME,&stop) == -1)
        error("error occured in  clock get time stop");
    munmap(data, statbuf.st_size);

    totaltime = (double)(stop.tv_sec - start.tv_sec) ;
    totaltime += ((double)(stop.tv_nsec - start.tv_nsec) / 1e9);

    cout << "Time taken to complete : " << totaltime << "secs\n";
    cout << "speed = " << ((double)filesize)/(totaltime*1024*1024) << " MBps\n";
    fclose(fp);
    close(sockfd);
    return 0;
}