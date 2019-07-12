#include "pink_epoll.h"

extern struct epoll_event *events;

void epoll_et(int epollfd, int listenfd, unique_ptr<threadpool<pink_http_conn> > &t_pool,
							unique_ptr<pink_connpool<pink_http_conn> > &c_pool){

	// ET 模式，带 EPOLLONESHOT
	cout << "Using Epoll ET mode" << endl;
	pink_epoll_addfd(epollfd, listenfd, (EPOLLIN | EPOLLET), true);

	while(true){
		//cout << ".......waiting.........." << endl;
		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if(event_number < 0 && errno != EINTR){
			perror("epoll failed:");
			break;
		}
		for(int i = 0; i < event_number; ++i){
			int fd = events[i].data.fd;
			if(fd == listenfd){
				while(true){
					sockaddr_in client_addr;
					int connfd = accept_conn(listenfd, client_addr);
					if(connfd < 0){
						//cout << "Break" << endl;
						break;
					}
					// 连接池已经满了
					int conn_id = c_pool->append_conn(connfd, client_addr);
					if(conn_id < 0){
						send_error(connfd, "Internal server busy");
						break;
					}
					//cout << "Accepted: " << connfd << " , conn_id: " << conn_id << endl;
					pink_epoll_modfd(epollfd, listenfd, (EPOLLIN | EPOLLET), true);
				}
			}
			else if(events[i].events & (EPOLLHUP | EPOLLERR)){
				//cout << "exception event from: " << fd << ", events: " << events[i].events << endl; // for debug
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
					c_pool->close_conn(fd);
				}
			}
			else if(events[i].events & (EPOLLOUT)){
				//cout << "write event from: " << fd << endl; // for debug
				if(fd == -1){
					continue;
				}
				if(!c_pool->write(fd)){
					c_pool->close_conn(fd);
				}
			}
		}
	}
}

void epoll_lt(int epollfd, int listenfd, unique_ptr<threadpool<pink_http_conn> > &t_pool,
							unique_ptr<pink_connpool<pink_http_conn> > &c_pool){

	// LT 模式，沒有 EPOLLONESHOT
	cout << "Using Epoll LT mode" << endl;
	pink_epoll_addfd(epollfd, listenfd, (EPOLLIN), false);

	while(true){
		//cout << ".......waiting.........." << endl;
		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if(event_number < 0 && errno != EINTR){
			perror("epoll failed:");
			break;
		}
		for(int i = 0; i < event_number; ++i){
			int fd = events[i].data.fd;
			if(fd == listenfd){
				sockaddr_in client_addr;
				int connfd = accept_conn(listenfd, client_addr);
				if(connfd < 0)
					break;
				// 连接池已经满了
				int conn_id = c_pool->append_conn(connfd, client_addr);
				if(conn_id < 0){
					send_error(connfd, "Internal server busy");
					break;
				}
				//cout << "Accepted: " << connfd << " , conn_id: " << conn_id << endl;
			}
			else if(events[i].events & (EPOLLHUP | EPOLLERR)){
				//cout << "exception event from: " << fd << ", events: " << events[i].events << endl; // for debug
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
					c_pool->close_conn(fd);
				}
			}
			else if(events[i].events & (EPOLLOUT)){
				//cout << "write event from: " << fd << endl; // for debug
				if(fd == -1){
					continue;
				}
				if(!c_pool->write(fd)){
					c_pool->close_conn(fd);
				}
			}
		}
	}
}