#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>

#define PACKET_SIZE (1024) //Max buffer size of the data in a packet
/*A packet with unique id, length and data*/
struct packet_t
{
    long int ID;
    long int ack_no;
    long int length;
    char data[PACKET_SIZE];
    bool ack_flag;
};
using namespace std;
/**
---------------------------------------------------------------------------------------------------
print_error
--------------------------------------------------------------------------------------------------
* This function prints the error message to console
*
* 	@\param msg	User message to print
*
* 	@\return	None
*
*/ 
static void print_error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}
/*-------------------------------------------Main loop-----------------------------------------*/

int main(int argc, char **argv)
{
    /*check for appropriate commandline arguments*/
    if (argc != 2)
    {
        cout << "Usage --> ./[" << argv[0] << "] [Port Number]\n"; //Should have a port number > 5000
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr, client_addr;
    struct stat st;
    struct packet_t packet;
    struct timeval t_out = {0, 0};

    char msg_recv[PACKET_SIZE];
    char filename_recv[20];
    char cmd_recv[10];

    ssize_t numRead, length;
    off_t f_size;
    long int ack_num = 0; //Recieve packet acknowledgement
    int ack_send = 0;
    int sfd;

    FILE *fptr;

    /*Clear the server structure - 'server_addr' and populate it with port and IP address*/
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    int temp = atoi(argv[1]);
    server_addr.sin_port = htons(temp);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        print_error("Server: socket");

    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        print_error("Server: bind");

    while(true)
    {
        cout << "Server: Waiting for client to connect\n";

        memset(msg_recv, 0, sizeof(msg_recv));
        memset(cmd_recv, 0, sizeof(cmd_recv));
        memset(filename_recv, 0, sizeof(filename_recv));

        length = sizeof(client_addr);

        if ((numRead = recvfrom(sfd, msg_recv, PACKET_SIZE, 0, (struct sockaddr *)&client_addr, (socklen_t *)&length)) == -1)
            print_error("Server: recieve");

        //print_msg("Server: Recieved %ld bytes from %s\n", numRead, client_addr.sin_addr.s_addr);
        cout << "Server: The recieved message --->" << msg_recv << '\n';

        sscanf(msg_recv, "%s %s", cmd_recv, filename_recv);

        /*----------------------------------------------------------------------"get case"-------------------------------------------------------------------------*/

        if ((strcmp(cmd_recv, "get") == 0) && (filename_recv[0] != '\0'))
        {

            cout << "Server: Get called with file name --> " << filename_recv <<"\n";

            if (access(filename_recv, F_OK) == 0)
            { //Check if file exist

                int total_packet = 0, resend_packet = 0, drop_packet = 0, t_out_flag = 0;

                stat(filename_recv, &st);
                f_size = st.st_size; //Size of the file

                t_out.tv_sec = 2;
                t_out.tv_usec = 0;
                setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); //Set timeout option for recvfrom

                fptr = fopen(filename_recv, "rb"); //open the file to be sent

                total_packet = ((f_size + PACKET_SIZE - 1) / PACKET_SIZE);

                cout << "Total number of packets ---> " <<total_packet << "\n";

                length = sizeof(client_addr);

                sendto(sfd, &(total_packet), sizeof(total_packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)); //Send number of packets (to be transmitted) to reciever
                recvfrom(sfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&client_addr, (socklen_t *)&length);

                while (ack_num != total_packet) //Check for the acknowledgement
                {
                    /*keep Retrying until the ack matches*/
                    sendto(sfd, &(total_packet), sizeof(total_packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                    recvfrom(sfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&client_addr, (socklen_t *)&length);

                    resend_packet++;

                    /*Enable timeout flag even if it fails after 20 tries*/
                    if (resend_packet == 20)
                    {
                        t_out_flag = 1;
                        break;
                    }
                }

                /*transmit data packets sequentially followed by an acknowledgement matching*/

                long int i = 1;
                while (i <= total_packet)
                {
                    struct packet_t ack_pkt;
                    memset(&packet, 0, sizeof(packet));
                    memset(&ack_pkt, 0, sizeof(ack_pkt));
                    ack_num = 0;
                    packet.ID = i;
                    packet.length = fread(packet.data, 1, PACKET_SIZE, fptr);
                    packet.ack_no = 0;
                    packet.ack_flag = 0;                    

                    sendto(sfd, &(packet), sizeof(packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));            //send the packet
                    recvfrom(sfd, &(ack_pkt), sizeof(ack_pkt), 0, (struct sockaddr *)&client_addr, (socklen_t *)&length); //Recieve the acknowledgement

                    while (ack_pkt.ack_no != packet.ID) //Check for ack
                    {
                        /*keep retrying until the ack matches*/
                        sendto(sfd, &(packet), sizeof(packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                        recvfrom(sfd, &(ack_pkt), sizeof(ack_pkt), 0, (struct sockaddr *)&client_addr, (socklen_t *)&length);
                        cout << "packet ---> " << packet.ID << "dropped, " << ++drop_packet <<"times\n";

                        resend_packet++;

                        // cout << "packet ---> " << packet.ID << "dropped, " << drop_packet <<"times\n";

                        /*Enable the timeout flag even if it fails after 200 tries*/
                        if (resend_packet == 200)
                        {
                            t_out_flag = 1;
                            break;
                        }
                    }

                    resend_packet = 0;
                    drop_packet = 0;

                    /*File transfer fails if timeout occurs*/
                    if (t_out_flag == 1)
                    {
                        cout << "File not sent\n";
                        break;
                    }

                    // printf("packet ----> %ld	Ack ----> %ld \n", i, ack_num);

                    if (total_packet == ack_num)
                        cout << "File sent\n";
                    i++;
                }
                fclose(fptr);

                t_out.tv_sec = 0;
                t_out.tv_usec = 0;
                setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); //Disable the timeout option
            }
            else
            {
                cout << "Invalid Filename\n";
            }
        }
        /*--------------------------------------------------------------------"exit case"----------------------------------------------------------------------------*/

        else if (strcmp(cmd_recv, "exit") == 0)
        {
            close(sfd); //close the server on exit call
            exit(EXIT_SUCCESS);
        }

        /*--------------------------------------------------------------------"Invalid case"-------------------------------------------------------------------------*/

        else
        {
            cout << "Server: Unkown command. Please try again\n";
        }
    }

    close(sfd);
    exit(EXIT_SUCCESS);
}
