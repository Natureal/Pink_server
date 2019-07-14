#include <iostream>
#include <vector>
#include <signal.h>
#include "pink_epoll.h"

const char *conf_file = "pink_conf.conf";
conf_t conf;

int main(int argc, char *argv[]){
	if(read_conf(conf_file, conf) < 0){
		perror("Read conf file failed");
		return 1;
	}

	// 忽略 SIGPIPE 信号：向没有读端的管道写数据，比如对方关闭连接后还写数据
	add_signal(SIGPIPE, SIG_IGN);

	// 创建epoll fd ==============================================
	int epollfd;
	if((epollfd = pink_epoll_create(5)) < 0)
		return 1;
	pink_http_conn::epollfd = epollfd;


	// 初始化非阻塞监听socket ======================================
	int listenfd;
	if((listenfd = bind_and_listen(conf.port)) < 0)
		return 1;
	set_nonblocking(listenfd);

	// 开启I/O复用 ================================================
	int ret = epoll_run(epollfd, listenfd);

	close(epollfd);
	close(listenfd);

	return ret;
}

