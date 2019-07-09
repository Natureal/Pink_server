#include <iostream>
#include <vector>
#include <signal.h>
#include "tools/basic_tool.h"
#include "tools/socket_tool.h"
#include "pink_http_conn.h"
#include "pink_threadpool.h"
#include "pink_epoll.h"

const char *conf_file = "pink_conf.conf";
conf_t conf;

const int PRE_FD = 10000; // 预分配的FD数量
const int MAX_FD = 65535;

extern epoll_event *events;


int main(int argc, char *argv[]){

	if(read_conf(conf_file, conf) < 0){
		perror("Read conf file failed");
		return 1;
	}

	// 忽略 SIGPIPE 信号：向没有读端的管道写数据，比如对方关闭连接后还写数据
	add_signal(SIGPIPE, SIG_IGN);

	// 初始化非阻塞监听socket
	int listenfd;
	if((listenfd = bind_and_listen(conf.port)) < 0)
		return 1;

	//std::cout << "listenfd: " << listenfd << std::endl; // for debug

	set_nonblocking(listenfd);

	// 创建epoll fd，并添加监听socket
	int epollfd;
	if((epollfd = pink_epoll_create(5)) < 0)
		return 1;

	//std::cout << "epollfd: " << epollfd << std::endl; for debug

	if(pink_epoll_addfd(epollfd, listenfd, (EPOLLIN | EPOLLET | EPOLLRDHUP), true) < 0)
		return 1;

	// 创建线程池
	threadpool<pink_http_conn> *t_pool = nullptr;
	try{
		t_pool = new threadpool<pink_http_conn >;
	}
	catch( ... ){
		perror("create threadpool failed");
		return 1;
	}

	// 预分配PRE_FD的http connection
	pink_http_conn temp;
	std::vector<pink_http_conn > *users = new std::vector<pink_http_conn>(PRE_FD, temp);
	pink_http_conn::epollfd = epollfd;

	// 运行服务器
	while(true){

		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

		for(int i = 0; i < event_number; ++i){
			int fd = events[i].data.fd;
			// 监听fd上有事件
			if(fd == listenfd){
				sockaddr_in client_addr;
				int connfd = accept_conn(listenfd, client_addr);
				if(connfd < 0)
					continue;
				if(connfd >= MAX_FD){
					send_error(connfd, "Internal server busy");
					continue;
				}
				if(connfd >= users->size()){
					users->reserve(users->size() + 1000);
				}
				(*users)[connfd].init(connfd, client_addr);

				std::cout << "accpeted a connection as: " << connfd << std::endl; // for debug
			}
			// 发生异常
			else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
				std::cout << "exception event from: " << fd << ", events: " << events[i].events << std::endl; // for debug
				if(events[i].events & EPOLLRDHUP)
					std::cout << "EPOLLRDHUP, ";
				if(events[i].events & EPOLLHUP)
					std::cout << "EPOLLHUP, ";
				if(events[i].events & EPOLLERR)
					std::cout << "EPOLLERR";
				std::cout << std::endl;

				(*users)[fd].close_conn();
			}
			else if(events[i].events & (EPOLLIN)){
				std::cout << "read event from: " << fd << std::endl; // for debug

				if((*users)[fd].read()){
					t_pool->append(&((*users)[fd]));
					std::cout << "append pthread success" << std::endl;
				}
				else{
					(*users)[fd].close_conn();
				}
			}
			else if(events[i].events & (EPOLLOUT)){
				std::cout << "write event from: " << fd << std::endl; // for debug

				if(!(*users)[fd].write()){
					(*users)[fd].close_conn();
				}
			}
		}
	}

	close(epollfd);
	close(listenfd);
	std::vector<pink_http_conn >().swap(*users);
	users = nullptr;

	delete t_pool;

	return 0;
}



