#include <iostream>
#include "utility/basic_tool.h"
#include "utility/socket_tool.h"
#include "pink_epoll.h"

const char *conf_file = "pink.conf";

int main(int argc, char *argv[]){

	conf_t conf;
	import_conf(conf_file, &conf);
	
	// 忽略 SIGPIPE 信号：向没有读端的管道写数据，比如对方关闭连接后还写数据
	add_signal(SIGPIPE, SIG_IGN);

	// 初始化非阻塞监听socket
	int listenfd = bind_and_listen(conf.port);
	set_nonblocking(listenfd);

	// 创建epoll fd，并添加监听socket
	int epollfd = pink_epoll_create(5);
	extern epoll_event *events;

	pink_epoll_addfd(epollfd, listenfd, (EPOLLIN | EPOLLET | EPOLLRDHUP), true);

	// 创建线程池
	
	threadpool<http_conn> *threadpool = NULL;
	try{
		threadpool = new threadpool<http_conn>;
	}
	catch( ... ){
		cout << "create threadpool failed" << endl;
		return 1;
	}
	

	// 运行服务器
	while(true){

		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

		for(int i = 0; i < event_number; ++i){
		int fd = events[i].data.fd;
		// 监听fd上有事件
		if(fd == listenfd){
			int connfd = accept_conn(listenfd);
			if(connfd < 0){
				continue;
			}

			
		}
		// 发生异常
		else if(events[i].events & (EPOLLRDHUP || EPOLLHUP || EPOLLERR)){
			
		}
		else if(events[i].events & (EPOLLIN)){

		}
		else if(events[i])
	}
}



