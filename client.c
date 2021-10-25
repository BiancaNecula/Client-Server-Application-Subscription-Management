#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "LinkedList.h"
#include "helpers.h"

struct udp_packet {
        char topic[51];
        char data_type;
        char content[1501];
};


struct client_msg {
	char id[20];
	int subscribe; // 1 pt subscribe, -1 pt unsubscribe, 0 pt exit
	char topic[51];
        int sf;
};

struct pack_to_send {
	struct sockaddr_in udp_addr;
	struct udp_packet udp_pack;
};

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}


int main(int argc, char *argv[])
{
	int sockfd, ret;
	struct sockaddr_in serv_addr;
	char buff[BUFLEN];

	if (argc < 3) {
		usage(argv[0]);
	}
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	FD_SET(0, &read_fds);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	int z = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&z, sizeof(z));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	ret = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	char *tmp;
	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		if(FD_ISSET(0, &tmp_fds)) {
			memset(buff, 0, BUFLEN);
			fgets(buff, BUFLEN-1, stdin);
			tmp = strtok(buff, " \n");
			if (!strcmp(tmp, "subscribe")) {
				struct client_msg msg;
				tmp = strtok(NULL, " \n");
				memcpy(msg.topic, tmp, 51);
				tmp = strtok(NULL, " \n");
                                msg.sf = atoi(tmp);
				msg.subscribe = 1;
				strcpy(msg.id, argv[1]);
				ret = send(sockfd, &msg, sizeof(struct client_msg), 0);
				DIE(ret < 0, "send");
					
			}
			else if (!strcmp(tmp, "unsubscribe")) {
				struct client_msg msg;
                                tmp = strtok(NULL, " \n");
                                memcpy(msg.topic, tmp, 51);
                                tmp = strtok(NULL, " \n");
                                msg.sf = atoi(tmp);
                                msg.subscribe = -1;
                                strcpy(msg.id, argv[1]);
                                ret = send(sockfd, &msg, sizeof(struct client_msg), 0);
				DIE(ret < 0, "send");

			}
			else if (!strcmp(tmp, "exit")) {
				struct client_msg msg;
                                msg.subscribe = 0;
                                strcpy(msg.id, argv[1]);
                                ret = send(sockfd, &msg, sizeof(struct client_msg), 0);
                                DIE(ret < 0, "send");
				break;
			}
			else {
				printf("Comanda gresita!\n");
			}
		}
		else {
			struct pack_to_send rec;
			ret = recv(sockfd, &rec, sizeof(struct pack_to_send), 0);
			DIE(ret < 0, "recv");
			printf("%s:%i - %s - ", inet_ntoa(rec.udp_addr.sin_addr), ntohs(rec.udp_addr.sin_port), rec.udp_pack.topic);
			switch(rec.udp_pack.data_type){
        			case '0':
					printf("INT - ");
					uint32_t *n = (uint32_t*)(rec.udp_pack.content + 1);
					if(rec.udp_pack.content[0] == 1) {
						printf("-%u\n", ntohl(*n));
					}
					else
					{
						printf("%u\n", ntohl(*n));
					}
					break;
				case '1':
					printf("SHORT REAL - ");
					uint16_t *n2 = (uint16_t*)rec.udp_pack.content;
					double a = (double)(ntohs(*n2)) / 100;
					printf("-%.3f\n", a);
					break;
				case '2':
					printf("FLOAT - ");
					uint32_t *n3 = (uint32_t*)(rec.udp_pack.content + 1);
					uint8_t *powr = (uint8_t*)(rec.udp_pack.content + 1 + sizeof(uint32_t));
					float b = (ntohl(*n3)) * pow(10, (-1)*(*powr));
                                        if(rec.udp_pack.content[0] == 1) {
                                                printf("-%.3f\n", b);
                                        }
                                        else
                                        {
                                                printf("%.3f\n", b);
                                        }
                                        break;

				case '3':
					printf("STRING - ");
					printf("%s\n", rec.udp_pack.content);
					break;
				default:
					break;	

		
			}
		}
	}

	return 0;


}
