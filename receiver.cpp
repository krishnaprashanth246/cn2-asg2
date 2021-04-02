#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <thread>
#include <mutex>

using namespace std;

#define FILEPATH "received_file"
#define MAXBUFSIZE 1400
#define MAXSEND 20

typedef struct pkt
{
    uint32_t ts;       /* time stamp */
    uint32_t dataSize; /* datasize , normally it will be 1460*/
    uint32_t seqNum;   /* store sequence # */
    int retransmit;
    char buf[MAXBUFSIZE]; /* Data buffer */
} pkt;

struct stat statbuf;
struct timespec start, stop;
struct sockaddr_in serv_addr;

char *nodes;
char *data;

int itr_ptr; /* Iterating Pointer */
int datarecv = 0;
int sockfd;
int filesize;
int totalPacketsArrived = 0, totalSeq = 0;
double time_taken;

thread thrd[1];

mutex lk;

void error(const char *msg)
{
    cerr << msg;
    exit(1);
}

char *openFILE(char *str)
{
    int fp;
    int pagesize;
    char *data;

    fp = open(str, O_RDONLY);
    if (fp < 0)
        error("Error: while opening the file\n");

    if (fstat(fp, &statbuf) < 0)
        error("ERROR: while file status\n");

    /* mmap-ing the file */

    if ((data = (char *)mmap((caddr_t)0, statbuf.st_size, PROT_READ, MAP_SHARED, fp, 0)) == MAP_FAILED)
        error("ERROR : mmap \n");

    return data;
}

int iterateArrayAgain()
{
    int i;
    if (nodes == NULL)
        return -1;
    for (i = itr_ptr; i <= totalSeq; i++)
    {
        if (nodes[i] == 0)
        {
            if (i == totalSeq)
                itr_ptr = 0;
            else
                itr_ptr = (i + 1);
            return i;
        }
    }
    itr_ptr = 0;
    return -1;
}

void checkme(void *arg)
{
    int requestIndex;
    int n = 0;
    printf("enter check me server\n");
    for (; true;)
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
        if (requestIndex >= 0 && requestIndex <= totalSeq)
        {
            if ((n = sendto(sockfd, &requestIndex, sizeof(int), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
                error("sendto\n");
        }
    }
}

bool deleteNode(char **arrayNode, int seqNum)
{

    if (seqNum >= 0 && seqNum <= totalSeq)
    {
        if (nodes[seqNum] != 0)
        {
            return false;
        }
        else
        {
            nodes[seqNum] = 1;
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    int portno, i = 0;
    socklen_t fromlen;
    int n;
    FILE *fp;
    int errn = 0;
    int last = -1, seq = 0;
    pkt recvDG;
    int ack = -1;

    int datasize = 0;

    if (argc != 2)
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
    // filesize = 104857600;
    totalSeq = ceil(filesize / MAXBUFSIZE);
    cout << "Seqtotal::" << totalSeq << " filesize = " << filesize << '\n';

    fp = fopen(FILEPATH, "w+");

    thrd[0] = thread(checkme, (void *)0);

    nodes = (char *)calloc(totalSeq, sizeof(char));

    if (clock_gettime(CLOCK_REALTIME, &start) == -1)
    {
        error("clock get time");
    }

    for (; true;)
    {

        n = recvfrom(sockfd, &recvDG, sizeof(pkt), 0, (struct sockaddr *)&serv_addr, &fromlen);

        if (n == sizeof(int))
        {
            continue;
        }
        else if (n < 0)
        {
            error("recvfrom");
        }

        std::unique_lock<std::mutex> lck1(lk);

        if (deleteNode(&nodes, recvDG.seqNum))
        {

            fseek(fp, MAXBUFSIZE * recvDG.seqNum, SEEK_SET);
            fwrite(&recvDG.buf, recvDG.dataSize, 1, fp);
            fflush(fp);
            totalPacketsArrived++;
        }

        lck1.unlock();
        if (totalPacketsArrived == totalSeq + 1)
        {
            cout << "Complete File received!" << endl;
            i = 0;
            while (i < MAXSEND)
            {
                n = sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                i++;
            }

            break;
        }
    }

    if (clock_gettime(CLOCK_REALTIME, &stop) == -1)
    {
        error("clock get time stop");
    }

    thrd[0].join();
    fclose(fp);

    time_taken = (stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / 1e9;
    cout << "Time taken to complete " << time_taken << endl;

    munmap(data, statbuf.st_size);

    close(sockfd);
    return 0;
}
