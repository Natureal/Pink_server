#include <iostream>
#include <vector>
#include <signal.h>
#include "pink_epoll.h"
#include "tools/pink_epoll_tool.h"

using std::vector;
using std::cout;
using std::endl;
using std::unique_ptr;

const char *conf_file = "pink_conf.conf";
conf_t conf;

const int PRE_CONN = 10000; // 预分配的FD数量
const int MAX_CONN = 10000;

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
	set_nonblocking(listenfd);

	// 创建epoll fd，并添加监听socket
	int epollfd;
	if((epollfd = pink_epoll_create(5)) < 0)
		return 1;
	pink_http_conn::epollfd = epollfd;

	// 创建线程池
	unique_ptr<threadpool<pink_http_conn> > t_pool = nullptr;
	try{
		t_pool.reset(new threadpool<pink_http_conn>(conf.max_thread_number, 10000));
	}
	catch( ... ){
		perror("create threadpool failed");
		return 1;
	}

	// 开启I/O复用
	epoll_et(epollfd, listenfd, t_pool);
	//epoll_lt(epollfd, listenfd, t_pool, c_pool);

	close(epollfd);
	close(listenfd);

	return 0;
}

