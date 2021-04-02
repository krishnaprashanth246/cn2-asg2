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
using namespace std;
static void print_error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

/*----------------------------------------Main loop-----------------------------------------------*/

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		cout << "Client: Usage --> ./[" << argv[0] << "] [IP Address] [Port Number]\n"; //Should have the IP of the server
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in send_addr, from_addr;
	struct stat st;
	struct packet_t packet, ack_pkt;
	struct timeval t_out = {0, 0};

	char cmd_send[50];
	char flname[20];
	char cmd[10];
	char ack_send[4] = "ACK";

	ssize_t numRead = 0, length = 0;
	off_t f_size = 0;
	long int ack_num = 0;
	int cfd, ack_recv = 0;

	FILE *fptr;

	/*Clear all the data buffer and structure*/
	memset(ack_send, 0, sizeof(ack_send));
	memset(&send_addr, 0, sizeof(send_addr));
	memset(&from_addr, 0, sizeof(from_addr));

	/*Populate send_addr structure with IP address and Port*/
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(atoi(argv[2]));
	send_addr.sin_addr.s_addr = inet_addr(argv[1]);

	if ((cfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		print_error("CLient: socket");

	while (true)
	{

		memset(cmd_send, 0, sizeof(cmd_send));
		memset(cmd, 0, sizeof(cmd));
		memset(flname, 0, sizeof(flname));

		cout << "\n Menu \n Enter any of the following commands \n 1.) get [file_name] \n 2.) put [file_name] \n 3.) delete [file_name] \n 4.) ls \n 5.) exit \n";
		scanf(" %[^\n]%*c", cmd_send);

		//printf("----> %s\n", cmd_send);

		sscanf(cmd_send, "%s %s", cmd, flname); //parse the user input into command and filename

		if (sendto(cfd, cmd_send, sizeof(cmd_send), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)) == -1)
		{
			print_error("Client: send");
		}

		/*----------------------------------------------------------------------"get case"-------------------------------------------------------------------------*/

		if ((strcmp(cmd, "get") == 0) && (flname[0] != '\0'))
		{

			long int total_packet = 0;
			long int bytes_rec = 0, i = 1;

			t_out.tv_sec = 2;
			setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); //Enable the timeout option if client does not respond

			recvfrom(cfd, &(total_packet), sizeof(total_packet), 0, (struct sockaddr *)&from_addr, (socklen_t *)&length); //Get the total number of packet to recieve

			t_out.tv_sec = 0;
			setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); //Disable the timeout option

			if (total_packet > 0)
			{
				sendto(cfd, &(total_packet), sizeof(total_packet), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
				printf("----> %ld\n", total_packet);

				fptr = fopen(flname, "wb"); //open the file in write mode

				/*Recieve all the packets and send the acknowledgement sequentially*/
				while (i <= total_packet)
				{
					memset(&packet, 0, sizeof(packet));
					recvfrom(cfd, &(packet), sizeof(packet), 0, (struct sockaddr *)&from_addr, (socklen_t *)&length); //Recieve the packet
					memset(&ack_pkt, 0, sizeof(ack_pkt));
					ack_pkt.ID = 0;
					ack_pkt.length = 0;
					ack_pkt.ack_flag = 1;
					ack_pkt.ack_no = packet.ID;
					sendto(cfd, &(ack_pkt), sizeof(ack_pkt), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)); //Send the ack

					/*Drop the repeated packet*/
					if (packet.ID != i)
						i--;
					else
					{
						fwrite(packet.data, 1, packet.length, fptr); /*Write the recieved data to the file*/
						// printf("packet.ID ---> %ld	packet.length ---> %ld\n", packet.ID, packet.length);
						bytes_rec += packet.length;
					}

					if (i == total_packet)
					{
						printf("File recieved\n");
					}
					i++;
				}
				printf("Total bytes recieved ---> %ld\n", bytes_rec);
				fclose(fptr);
			}
			else
			{
				printf("File is empty\n");
			}
		}

		/*----------------------------------------------------------------------"exit case"-------------------------------------------------------------------------*/

		else if (strcmp(cmd, "exit") == 0)
		{

			exit(EXIT_SUCCESS);
		}

		/*--------------------------------------------------------------------"Invalid case"-------------------------------------------------------------------------*/

		else
		{
			printf("--------Invalid Command!----------\n");
		}
	}

	close(cfd);

	exit(EXIT_SUCCESS);
}
