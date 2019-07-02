#include <iostream>
#include "utility/basic_tool.h"
#include "utility/socket_tool.h"
#include "pink_epoll.h"

const char *conf_file = "pink.conf";
conf_t conf;

int main(int argc, char *argv[]){

	import_conf(conf_file, &conf;
	
	// 忽略 SIGPIPE 信号：向没有读端的管道写数据，比如对方关闭连接后还写数据
	add_signal(SIGPIPE, SIG_IGN);

	int listenfd = bind_and_listen(conf.port);
	set_nonblocking(listenfd);



	while(true){

		int event_number = pink_epoll_wait()

		for(int i = 0; i < event_number; ++i){
		int fd = events[i].data.fd;
		// 监听fd上有事件
		if(fd == listenfd){
			int connfd = accept_conn();
			if(connfd < 0){
				cout << "accept connection failed" << endl;
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



