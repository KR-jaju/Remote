#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_clients
{
	int id;
	char msg[1024];
}	t_clients;

t_clients clients[1024];
fd_set read_fds, write_fds, active_fds;
int max_fd = 0, next_id = 0;
char write_buffer[120000], read_buffer[120000];

void print_error(char *msg)
{
	if (msg)
		write(2, msg, strlen(msg));
	else
		write (2, "Fatal error", strlen("Fatal error"));
	write(2, "\n", 1);
	exit(1);
}

void send_all(int except_fd)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (FD_ISSET(fd, &write_fds) && fd != except_fd)
		{
			if (send(fd, write_buffer, strlen(write_buffer), 0) < 0)
				print_error(NULL);
		}
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
		print_error("Wrong number of arguments");
	
	int sockfd, connfd;
	socklen_t len;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		print_error(NULL);

	max_fd = sockfd;
	
	FD_ZERO(&active_fds);
	FD_SET(sockfd, &active_fds);
	bzero(&clients, sizeof(clients));
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(argv[1]));

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) 
		print_error(NULL);
	if (listen(sockfd, 10) < 0)
		print_error(NULL);

	while(1)
	{
		read_fds = active_fds;
		write_fds = active_fds;

		if (select(max_fd + 1, &read_fds, &write_fds, 0, 0) < 0)
			// print_error(NULL);
			continue;  // 끝나지 않고 continue
		
		for (int fd = 0; fd <= max_fd; fd++)
		{
			// if (FD_ISSET(fd, &active_fds))
			if (FD_ISSET(fd, &read_fds))  //read fd 확인하기
			{
				if (fd == sockfd)
				{
					connfd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
					if (connfd < 0)
						continue;
					FD_SET(connfd, &active_fds);
					if (connfd > max_fd)
						max_fd = connfd;
					clients[connfd].id = next_id;
					next_id++;
					sprintf(write_buffer, "server: client %d just arrived\n", clients[connfd].id);
					send_all(connfd);
					break;
				}
				else
				{
					ssize_t recv_len = recv(fd, read_buffer, sizeof(read_buffer), 0);
					if (recv_len <= 0)
					{
						sprintf(write_buffer, "server: client %d just left\n", clients[fd].id);
						send_all(fd);

						FD_CLR(fd, &active_fds);
						close(fd);

						break ;
					}
					else
					{
						for (int i = 0, j = strlen(clients[fd].msg); i < recv_len; i++, j++)
						{
							clients[fd].msg[j] = read_buffer[i];
							if (clients[fd].msg[j] == '\n')
							{
								clients[fd].msg[j] = '\0';
								sprintf(write_buffer, "client %d: %s\n", clients[fd].id, clients[fd].msg);
								send_all(fd);
								// bzero(&clients[fd].msg, sizeof(clients[fd].msg));  
								bzero(&clients[fd].msg, strlen(clients[fd].msg));  //strlen으로 길이 설정
								j = -1;
							}
						}
						break ;
					}
				}
			}
		}
	}
	return (0);
}
