#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/tcp.h>
#include "LinkedList.h"
#include "helpers.h"

struct client_msg {
	char id[20];
	int subscribe; // 1 pt subscribe, -1 pt unsubscribe, 0 pt exit
	char topic[51];
        int sf;
};


struct udp_packet {
	char topic[51];
	char data_type;
	char content[1501];	
};

struct pack_to_send {
        struct sockaddr_in udp_addr;
        struct udp_packet udp_pack;
	int exit_msg;
};


struct topic_map {
	char topic[51];
	int sf;
};

struct client {
	char id[20];
	struct LinkedList *sub_topics;
	struct LinkedList *tosend;
	int socket;
};

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int search(struct LinkedList *clients, char *buffer) {
	struct Node *node;
        node = clients->head;
        while (node != NULL) {
        	if ((((struct client *)(node->data))->socket > 0) && !strcmp(((struct client *)(node->data))->id, buffer)) {
			return ((struct client *)(node->data))->socket;
		}
		node = node->next;
	}
	return -1;

}


void receive_udp_pack(struct LinkedList *clients, struct sockaddr_in *received, int ret, struct udp_packet *recv_pack){
	struct pack_to_send package;
	memset(&package, 0, sizeof(struct pack_to_send));
	memcpy(&package.udp_pack, recv_pack, sizeof(struct udp_packet));
	memcpy(&package.udp_addr, received, sizeof(struct sockaddr_in));
	package.exit_msg = 0;

	struct Node *node;
	if (clients == NULL) {
        	return;
        }
	node = clients->head;
	while (node != NULL) {
		if (((struct client *)(node->data))->socket > 0) {
        		if ((((struct client *)(node->data))->sub_topics != NULL) && (get_size(((struct client *)(node->data))->sub_topics) > 0)) {
				struct Node *node2;
				node2 = ((struct client *)(node->data))->sub_topics->head;
				while (node2 != NULL) {
					if (!strncmp(((struct topic_map *)(node2->data))->topic, recv_pack->topic, strlen(recv_pack->topic))) {
						ret = send(((struct client *)(node->data))->socket, &package, sizeof(struct pack_to_send), 0);
						DIE(ret < 0, "send");
					}
					node2 = node2->next;
				}	
			}
        	}
		else { // salvam mesajul
			if ((((struct client *)(node->data))->sub_topics != NULL) && (get_size(((struct client *)(node->data))->sub_topics) > 0)) {
                                struct Node *node2;
                                node2 = ((struct client *)(node->data))->sub_topics->head;
                                while (node2 != NULL) {
					if (((struct topic_map *)(node2->data))->sf == 1) {
                                        	if (!strncmp(((struct topic_map *)(node2->data))->topic, recv_pack->topic, strlen(recv_pack->topic))) {
                                                	if (((struct client *)(node->data))->tosend == NULL) {
								((struct client *)(node->data))->tosend = malloc(sizeof(struct LinkedList));
                        		                        init_list(((struct client *)(node->data))->tosend);
								add_nth_node(((struct client *)(node->data))->tosend, 0, &package);
							}
							else {
								add_nth_node(((struct client *)(node->data))->tosend, ((struct client *)(node->data))->tosend->size, &package);
							}
                                        	}
					}
					node2 = node2->next;
                                }
                        }

		}
		node = node->next;
	}

}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd_tcp, sockfd_udp, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr, serv_addr_udp;
	int i, ret;
	socklen_t clilen;
	struct LinkedList *clients = NULL;

	if (argc < 2) {
		usage(argv[0]);
	}


	fd_set read_fds;	
	fd_set tmp_fds;		
	int fdmax;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	// Socket TCP
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
        DIE(sockfd_tcp < 0, "socket");
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int a = 1;
	setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, &a, sizeof(int));
	ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	FD_SET(0, &read_fds);
	FD_SET(sockfd_tcp, &read_fds);

	// Socket UDP
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
        DIE(sockfd_udp < 0, "socket");
	serv_addr_udp.sin_family = AF_INET;
        serv_addr_udp.sin_port = htons(portno);
        serv_addr_udp.sin_addr.s_addr = INADDR_ANY;

        ret = bind(sockfd_udp, (struct sockaddr *) &serv_addr_udp, sizeof(struct sockaddr));
        DIE(ret < 0, "bind");

	FD_SET(sockfd_udp, &read_fds);

	if (sockfd_udp > sockfd_tcp) {
		fdmax = sockfd_udp;
	}
	else {
		fdmax = sockfd_tcp;
	}

	while(1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		

		if (FD_ISSET(0, &tmp_fds)) {
			memset(buffer, 0 , BUFLEN);
			scanf("%s", buffer);
			if(!strcmp(buffer, "exit")) {
				for(int j = 5; j <= fdmax; j++) {
					if (j != sockfd_tcp && j != 0 && j != sockfd_udp) {
						struct pack_to_send package;
					        memset(&package, 0, sizeof(struct pack_to_send));
						package.exit_msg = 1;
                        			send(j, &package, sizeof(struct pack_to_send), 0);
                        			close(j);
						FD_CLR(j, &read_fds);
                    			}
				}
				if (clients != NULL) {
					struct Node *node;
                                	node = clients->head;
                                	while (node != NULL) {
						free_list(&(((struct client *)(node->data))->sub_topics));
						free_list(&(((struct client *)(node->data))->tosend));
						node = node->next;
					}
					free_list(&clients);
				}
				close(sockfd_tcp);
				close(sockfd_udp);
				FD_CLR(sockfd_tcp, &read_fds);
				FD_CLR(sockfd_udp, &read_fds);
				break;
			}
		}	
		if (FD_ISSET(sockfd_udp, &tmp_fds)) {
			struct udp_packet pck;
			memset(&pck, 0, sizeof(struct udp_packet));
                        struct sockaddr_in received;
			memset(&received, 0, sizeof(struct sockaddr_in));
                        socklen_t len = sizeof(struct sockaddr);
                        ret = recvfrom(sockfd_udp, &pck, sizeof(struct udp_packet), 0, (struct sockaddr *) &received, &len);
                        DIE(ret < 0, "recvfrom");
                        receive_udp_pack(clients, &received, ret, &pck);
		}
	        else {

		for (i = 1; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp) {
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd_tcp, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					char newbuffer[20]; //id
					memset(newbuffer, 0, sizeof(newbuffer));
					ret = recv(newsockfd, newbuffer, 20, 0);
					DIE(ret < 0, "recv");
					
					printf("New client %s connected from %s:%d.\n", newbuffer, inet_ntoa(cli_addr.sin_addr), htons(cli_addr.sin_port));
					if (clients == NULL) {
						// doar s-a conectat
						clients = malloc(sizeof(struct LinkedList));
    						init_list(clients);
						struct client cl;
						memset(&cl, 0, sizeof(struct client));
						strcpy(cl.id, newbuffer);
						cl.socket = newsockfd;
						add_nth_node(clients, 0, &cl);

					}
					else if (search(clients, newbuffer) == -1) { // s-a conectat un client nou
						struct client cl;
						memset(&cl, 0, sizeof(struct client));
						strcpy(cl.id, newbuffer);
						cl.socket = newsockfd;
						add_nth_node(clients, clients->size, &cl);
					}
					else if (search(clients, newbuffer) != -1) { // s-a conectat un client care a fost conectat si ulterior
						// trebuie sa ii trimitem mesajele din topicurile la care a dat subscribe
						struct Node *node;
        					node = clients->head;
        					while (node != NULL) {
                					if ((((struct client *)(node->data))->socket == -1) && !strcmp(((struct client *)(node->data))->id, newbuffer)) {
                        					if ((((struct client *)(node->data))->tosend != NULL) && (get_size(((struct client *)(node->data))->tosend) > 0)) {
                                					struct Node *node2;
                                					node2 = ((struct client *)(node->data))->tosend->head;
                                					while (node2 != NULL) {
										ret = send(((struct client *)(node->data))->socket, (struct pack_to_send *)node2->data, sizeof(struct pack_to_send), 0);
				       
				       						DIE(ret < 0, "send");
										node2 = node2->next;	
                                       					}
									free_list(&(((struct client *)(node->data))->tosend));
								}
							}
                                        		node = node->next;
                                		}						
					}  	
				}
				else {  // daca este unul din clienti
					struct client_msg msg;
					memset(&msg, 0, sizeof(struct client_msg));
					ret = recv(i, &msg, sizeof(struct client_msg), 0);
					DIE(ret < 0, "socket");
					
					if (ret == 0) {
						struct Node *node;
                                                node = clients->head;
                                                while (node != NULL) {
                                                        if (((struct client *)(node->data))->socket == i) {
                                                                printf("Client %s disconnected.\n", msg.id);
                                                                ((struct client *)(node->data))->socket = -1;
                                                                close(i);
                                                                FD_CLR(i, &read_fds);
                                                                break;
                                                        }
                                                        node = node->next;
                                                }

					}
					
					if (msg.subscribe == 1) { // subscribe
						int ok = 0;
						if (search(clients, msg.id) == -1) {
							continue; //nu este un id bun
						}
						struct Node *node;
                                                node = clients->head;
                                                while (node != NULL) {
                                                        if ((((struct client *)(node->data))->socket > 0) && !strcmp(((struct client *)(node->data))->id, msg.id)) {
                                                                if ((((struct client *)(node->data))->sub_topics != NULL) && (get_size(((struct client *)(node->data))->sub_topics) > 0)) {
                                                                        struct Node *node2;
                                                                        node2 = ((struct client *)(node->data))->sub_topics->head;
                                                                        while (node2 != NULL) {
                                                                                if(!strcmp(msg.topic, ((struct topic_map *)(node2->data))->topic)) {
											ok = 1;
											break;
										}
                                                                                node2 = node2->next;
                                                                        }
                                                                }
								else {
									((struct client *)(node->data))->sub_topics = malloc(sizeof(struct LinkedList));
                 			                                init_list(((struct client *)(node->data))->sub_topics);
								}
								if (ok == 1) {
									// clientul a dat deja subscribe la topicul respectiv
									break;
								}
								struct topic_map map;
								memset(&map, 0, sizeof(struct topic_map));
								strcpy(map.topic, msg.topic);
							        map.sf = msg.sf;	
								add_nth_node(((struct client *)(node->data))->sub_topics, ((struct client *)(node->data))->sub_topics->size, &map);
								break;

                                                        }
                                                        node = node->next;
                                                }
						
					}
					else if (msg.subscribe == -1) { // unsubscribe
						if (search(clients, msg.id) == -1) {
                                                        continue; //nu este un id bun
                                                }
						struct Node *node;
                                                node = clients->head;
						int g = 0;
                                                while (node != NULL) {
                                                        if ((((struct client *)(node->data))->socket > 0) && !strcmp(((struct client *)(node->data))->id, msg.id)) {
                                                                if ((((struct client *)(node->data))->sub_topics != NULL) && (get_size(((struct client *)(node->data))->sub_topics) > 0)) {
                                                                        struct Node *node2;
                                                                        node2 = ((struct client *)(node->data))->sub_topics->head;
                                                                        while (node2 != NULL) {
                                                                                if(!strcmp(msg.topic, ((struct topic_map *)(node2->data))->topic)) {
                                                                                        struct Node *node_removed = remove_nth_node(((struct client *)(node->data))->sub_topics, g);
											free(node_removed);
											g++;
											break;
                                                                                }
                                                                                node2 = node2->next;
                                                                        }
                                                                }
                                                                break;

                                                        }
							g = 0;
                                                        node = node->next;
                                                }

	
					}
					else if (msg.subscribe == 0) { // exit
						if (search(clients, msg.id) == -1) {
                                                        continue; //nu este un id bun
                                                } 
                                                struct Node *node;
                                                node = clients->head;
                                                while (node != NULL) {
                                                        if (!strcmp(((struct client *)(node->data))->id, msg.id)) {
								printf("Client %s disconnected.\n", msg.id);
								((struct client *)(node->data))->socket = -1;
								close(i);
								FD_CLR(i, &read_fds);
								break;
							}
							node = node->next;
						}

					}
				
				} 
			} 
			}
		}
	}

	return 0;
}



