#include <iostream>
#include <vector>
#include <signal.h>
#include "tools/basic_tool.h"
#include "tools/socket_tool.h"
#include "pink_http_conn.h"
#include "pink_connpool.h"
#include "pink_threadpool.h"
#include "pink_epoll.h"

using std::vector;
using std::cout;
using std::endl;
using std::unique_ptr;

const char *conf_file = "pink_conf.conf";
conf_t conf;

const int PRE_CONN = 10000; // 预分配的FD数量
const int MAX_CONN = 10000;

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

	set_nonblocking(listenfd);

	// 创建epoll fd，并添加监听socket
	int epollfd;
	if((epollfd = pink_epoll_create(5)) < 0)
		return 1;
	pink_http_conn::epollfd = epollfd;

	//cout << "epollfd: " << epollfd << endl; for debug

	if(pink_epoll_addfd(epollfd, listenfd, (EPOLLIN), false) < 0)
		return 1;

	// 创建线程池
	unique_ptr<threadpool<pink_http_conn> > t_pool = nullptr;
	try{
		t_pool.reset(new threadpool<pink_http_conn>(conf.max_thread_number, 10000));
	}
	catch( ... ){
		perror("create threadpool failed");
		return 1;
	}

	// 创建连接池
	unique_ptr<pink_connpool<pink_http_conn> > c_pool = nullptr;
	try{
		c_pool.reset(new pink_connpool<pink_http_conn>(PRE_CONN, MAX_CONN));
	}
	catch( ... ){
		perror("create connection pool failed");
		return 1;
	}

	// 运行服务器
	while(true){
		//cout << ".......waiting.........." << endl;
		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if(event_number < 0 && errno != EINTR){
			perror("epoll failed:");
			break;
		}

		for(int i = 0; i < event_number; ++i){
			int fd = events[i].data.fd;
			// 监听fd上有事件
			if(fd == listenfd){
				sockaddr_in client_addr;
				int connfd = accept_conn(listenfd, client_addr);
				if(connfd < 0)
					continue;
				// 连接池已经满了
				int conn_id = c_pool->append_conn(connfd, client_addr);
				if(conn_id < 0){
					send_error(connfd, "Internal server busy");
					continue;
				}
				//cout << "Accepted: " << connfd << " , conn_id: " << conn_id << endl;
			}
			// 发生异常
			else if(events[i].events & (EPOLLHUP | EPOLLERR)){
				/*cout << "exception event from: " << fd << ", events: " << events[i].events << endl; // for debug
				if(events[i].events & EPOLLHUP)
					cout << "EPOLLHUP, ";
				if(events[i].events & EPOLLERR)
					cout << "EPOLLERR";
				cout << endl;
				*/

				c_pool->close_conn(fd);
			}
			else if(events[i].events & (EPOLLIN)){
				//cout << "read event from: " << fd << endl; // for debug
				if(fd == -1){
					continue;
				}

				if(c_pool->read(fd)){
					t_pool->append(&(c_pool->get_conn(fd)));
					//cout << "append pthread success for fd: " << fd << endl;
				}
				else{
					//cout << "nothing read" << endl;
					c_pool->close_conn(fd);
				}
			}
			else if(events[i].events & (EPOLLOUT)){
				//cout << "write event from: " << fd << endl; // for debug
				if(fd == -1){
					//cout << "fd has closed: " << fd << endl;
					continue;
				}

				if(!c_pool->write(fd)){
					c_pool->close_conn(fd);
				}
			}
		}
	}

	close(epollfd);
	close(listenfd);

	return 0;
}



