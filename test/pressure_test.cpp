#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

static const char* request_keep_alive = "GET http://localhost/index.html HTTP/1.1\r\nConnection: keep-alive\r\n\r\nxxxxxxxxxxxxxx";
static const char* request_close = "GET http://localhost/index.html HTTP/1.1\r\nConnection: close\r\n\r\nxxxxxxxxxxxxxx";
const int MAX_EVENT_NUMBER = 10000;

int set_nonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void epoll_addfd(int epoll_fd, int fd, const int events){
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	set_nonblocking(fd);
}

bool write_nbytes(int sockfd, const char *buffer, int len){
	int bytes_write = 0;
	printf("write out %d bytes to socket fd: %d\n", len, sockfd);
	while(true){
		bytes_write = send(sockfd, buffer, len, 0);
		if(bytes_write == -1){
			perror("Write failed");
			return false;
		}
		else if(bytes_write == 0){
			return false;
		}
		len -= bytes_write;
		buffer = buffer + bytes_write;
		if(len <= 0){
			printf("write succeeded\n");
			return true;
		}
	}
}

bool read_once(int sockfd, char* buffer, int len){
	int bytes_read = 0;
	memset(buffer, '\0', len);
	bytes_read = recv(sockfd, buffer, len, 0);
	if(bytes_read == -1){
		return false;	
	}
	else if(bytes_read == 0){
		return false;
	}
	printf("read in %d bytes from socket %d with content: \n %s\n", bytes_read,
															sockfd, buffer);
	return true;
}

void start_conn(int epoll_fd, int num, const char *ip, int port){
	int ret = 0;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	// from ipv4/6 to 网络字节序整数地址
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);
	
	for(int i = 0; i < num; ++i){
		//sleep(1);
		int sockfd = socket(PF_INET, SOCK_STREAM, 0);
		if(sockfd < 0){
			continue;
		}
		printf("create %dth sock and connect\n", i + 1);
		ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
		if(ret == 0){
			epoll_addfd(epoll_fd, sockfd, EPOLLOUT | EPOLLET | EPOLLERR);
		}
		else{
			printf("connect failed.\n");
		}
	}
}

void close_conn(int epoll_fd, int sockfd){
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, 0);
	close(sockfd);
}

int main(int argc, char* argv[]){
	assert(argc == 4);
	const char *ip = argv[1];
	int port = atoi(argv[2]);
	int num_conn = atoi(argv[3]);

	int epoll_fd = epoll_create(100);

	start_conn(epoll_fd, num_conn, ip, port);

	epoll_event events[MAX_EVENT_NUMBER];
	char buffer[2048];
	int timeout = 2000;
	int ret;

	while(true){
		int event_number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, timeout);
		for(int i = 0; i < event_number; ++i){
			int sockfd = events[i].data.fd;
			if(events[i].events & EPOLLIN){
				ret = read_once(sockfd, buffer, 2048);
				if(ret == 0){
					close_conn(epoll_fd, sockfd);
				}
				struct epoll_event event;
				event.events = EPOLLOUT | EPOLLET | EPOLLERR;
				event.data.fd = sockfd;
				epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &event);
			}
			else if(events[i].events & EPOLLOUT){
				ret = write_nbytes(sockfd, request_close, strlen(request_close));
				if(ret == 0){
					close_conn(epoll_fd, sockfd);
				}
				struct epoll_event event;
				event.events = EPOLLIN | EPOLLET | EPOLLERR;
				event.data.fd = sockfd;
				epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &event);
			}
			else if(events[i].events & EPOLLERR){
				printf("epoll error\n");
				close_conn(epoll_fd, sockfd);
			}
		}
	}

	return 0;
}

