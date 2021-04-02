#include <bits/stdc++.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace std;
#define FILENAME "CS3543_100MB"
#define FILEPATH_C "textfile_back.txt"
#define MAXSEND 20
#define PAYLOAD 1400

pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;

char *data;
char *allNodes;
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
    uint32_t ts;       /* time stamp */
    uint32_t dataSize; /* datasize , normally it will be 1460*/
    uint32_t seqNum;   /* store sequence # */
    int retransmit;
    char buf[PAYLOAD]; /* Data buffer */
} pkt;

pthread_t thrd[1];
int sockfd;

void error(const char *msg)
{
    cerr << msg;
    exit(0);
}

int iterateArrayAgain()
{
    int i = iteratingptr;
    if (allNodes == NULL)
    {
        return -1;
    }
    while (i <= totalSeq)
    {
        if (allNodes[i] == 0)
        {
            if (i != totalSeq)
                iteratingptr = i + 1;
            else
                iteratingptr = 0;
            return i;
        }
        i++;
    }
    iteratingptr = 0;
    return -1;
}

void *receiveMe(void *arg)
{
    int n = 0; // , i = 0;
               //   LinkedList *requestNodePointer = anchor;
               //int ack = -1;
    int requestIndex;
    cout << "enter receiveMe server" << endl;
    for (; true;)
    {
        if (totalPacketsArrived - 1 == totalSeq)
        {
            cout << "all data recved!" << endl;
            pthread_exit(0);
        }
        usleep(100);
        pthread_mutex_lock(&lck);
        requestIndex = iterateArrayAgain();
        pthread_mutex_unlock(&lck);
        if (requestIndex < 0)
        {
            // sleep(1);
        }
        else if (requestIndex >= 0)
        {
            if (requestIndex <= totalSeq)
            {
                n = sendto(sockfd, &requestIndex, sizeof(int), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if (n < 0)
                    error("sendto\n");
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
    {
        error("Error: while opening the file\n");
    }

    /* get the status of the file */
    if (fstat(fp, &statbuf) < 0)
    {
        error("ERROR: while file status\n");
    }

    /* mmap it , data is a pointer which is mapping the whole file*/
    data = (char *)mmap((caddr_t)0, statbuf.st_size, PROT_READ, MAP_SHARED, fp, 0);

    if (data == MAP_FAILED)
    {
        error("ERROR : mmap \n");
    }

    return data;
}
int deleteNodesFromArray(char **arrayNode, int seqNum)
{

    if (seqNum >= 0)
    {
        if(seqNum <= totalSeq)
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
    }
    return 0;
}
void *checkme(void *arg)
{
    int totalseq, lefbyte, lastseq, size_p, n = 0;
    pkt sendDG, recvDG;
    int packetmiss, errn;
    socklen_t servlen;
    servlen = sizeof(serv_addr);

    cout << "enter check me client" << endl;
    //fflush(stdout);
    // sleep(10);
    for (; true;)
    {
        n = recvfrom(sockfd, &packetmiss, sizeof(int), 0, (struct sockaddr *)&serv_addr, &servlen);
        //    printf("packetmiss:: %d\n",packetmiss);
        if (n < 0)
        {
            error("rcv from\n");
        }
        if (packetmiss < 0)
        {
            cout << "data recv fully" << endl;
            pthread_exit(0);
        }
        totalseq = filesize / PAYLOAD;
        lefbyte = filesize % PAYLOAD;
        //
        size_p = PAYLOAD;
        if (lefbyte != 0 && totalseq == packetmiss)
        {
            size_p = lefbyte;
        }
        pthread_mutex_lock(&lck);

        memcpy(sendDG.buf, &data[packetmiss * PAYLOAD], size_p);
        pthread_mutex_unlock(&lck);

        sendDG.seqNum = packetmiss;
        sendDG.dataSize = size_p;
        sendDG.retransmit = 1;

        n = sendto(sockfd, &sendDG, sizeof(pkt), 0, (struct sockaddr *)&serv_addr, servlen);
        if (n < 0)
            error("sendto");
    }
}

int main(int argc, char *argv[])
{
    // int count = 0;
    // int cnt = 0;
    // int rdbytes = 0;
    int errn = -1;
    unsigned char buffer[PAYLOAD];
    unsigned char *chptr = NULL;
    struct stat st;
    int seq = 0, datasize = 0, ack = -1; //, filesize =0;
    socklen_t fromlen;
    // int ack = -1;
    pkt recvDG;

    //char *data;

    FILE *fp = NULL;

    int portno, n;
    struct hostent *server;
    int i, j;

    if (argc != 3)
    {
        cerr << "usage:- " << argv[0] << " <hostname> <port>\n";
        exit(0);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    portno = atoi(argv[2]);
    if (sockfd < 0)
        error("ERROR opening socket\n");
    // server = gethostbyname(argv[1]);
    if ((server = gethostbyname(argv[1])) == NULL)
    {
        cerr << ("ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    long int sndsize = 50000000;

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&sndsize, (long int)sizeof(sndsize)) == -1)
    {
        cout << "error with setsocket" << endl;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&sndsize, (long int)sizeof(sndsize)) == -1)
    {
        cout << "error with setsocket" << endl;
    }

    if ((chptr = (unsigned char *)memset(buffer, '\0', PAYLOAD)) == NULL)
    {
        error("memset:");
    }

    if ((fp = fopen(FILENAME, "r")) == NULL)
    {
        error("fopen:");
    }

    if (stat(FILENAME, &st) == 0)
    {
        filesize = st.st_size;
        // printf("The size of this file is %d.\n", filesize);
        cout << "The size of this file is " << filesize << endl;
    }

    for (i = 0; i < 10; i++)
    {
        n = sendto(sockfd, &filesize, sizeof(filesize), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        cout << filesize << '\n';
        if (n < 0)
            error("sendto");
    }
    pthread_mutex_lock(&lck);

    data = openFILE(FILENAME);
    datasize = statbuf.st_size;

    pthread_mutex_unlock(&lck);

    if ((errn = pthread_create(&thrd[0], 0, checkme, (void *)0)))
    {
        fprintf(stderr, "pthread_create[0] %s\n", strerror(errn));
        pthread_exit(0);
    }

    if (clock_gettime(CLOCK_REALTIME, &start) == -1)
    {
        error("clock get time");
    }

    //   while (fgets(buffer ,PAYLOAD , fp) != NULL ) {
    while (datasize > 0)
    {
        pkt sendDG;
        int chunk, share;
        memset(&sendDG, 0, sizeof(pkt));

        chunk = PAYLOAD;
        share = datasize;

        if (share - chunk >= 0)
        {
            share = share - chunk;
        }
        else
        {
            chunk = share;
        }
        pthread_mutex_lock(&lck);

        memcpy(sendDG.buf, &data[seq * PAYLOAD], chunk);

        pthread_mutex_unlock(&lck);

        sendDG.seqNum = seq;
        sendDG.dataSize = chunk;
        sendDG.retransmit = 0;
        // printf("SeqNum= %d\n",sendDG.seqNum);
        usleep(100);
        sendto(sockfd, &sendDG, sizeof(sendDG), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        seq++;
        datasize -= chunk;
    }
    //printf("join");

    pthread_join(thrd[0], 0);

        if(clock_gettime(CLOCK_REALTIME,&stop) == -1){
        error("clock get time stop");
    }
    /*
    totaltime = (stop.tv_sec - start.tv_sec)+(double)(stop.tv_nsec - start.tv_nsec)/1e9;
    
    printf("Time taken to complete : %f sec",totaltime);
    */
    //fclose(fp);
    /* unmap the data 
    */
    munmap(data, statbuf.st_size);

    totaltime = (double)(stop.tv_sec - start.tv_sec) + ((double)(stop.tv_nsec - start.tv_nsec) / 1e9);

    printf("Time taken to complete : %lf sec", totaltime);

    fclose(fp);
    close(sockfd);
    return 0;
}