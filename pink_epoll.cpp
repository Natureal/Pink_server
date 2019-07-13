#include "pink_epoll.h"

extern struct epoll_event *events;

void epoll_et(int epollfd, int listenfd, unique_ptr<threadpool<pink_http_conn> > &t_pool){

	// ET 模式，带 EPOLLONESHOT
	cout << "Using Epoll ET mode for listenfd" << endl;
	pink_http_conn *listen_conn = new pink_http_conn;
	listen_conn->init_listen(listenfd);
	pink_epoll_addfd(epollfd, listenfd, listen_conn, (EPOLLIN | EPOLLET), true);

	while(true){
		//cout << ".......waiting.........." << endl;
		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if(event_number < 0 && errno != EINTR){
			perror("epoll failed:");
			break;
		}
		for(int i = 0; i < event_number; ++i){
			pink_http_conn *cur_conn = (pink_http_conn*)events[i].data.ptr;
			int fd = cur_conn->get_fd();
			if(fd == listenfd){
				while(true){
					sockaddr_in client_addr;
					int connfd = accept_conn(listenfd, client_addr);
					if(connfd < 0){
						//cout << "Break" << endl;
						break;
					}
					pink_http_conn *new_conn = new pink_http_conn;
					new_conn->init(connfd, client_addr);
					//cout << "Accepted: " << connfd << endl;
					pink_epoll_modfd(epollfd, listenfd, listen_conn, (EPOLLIN | EPOLLET), true);
				}
			}
			else if(events[i].events & (EPOLLHUP | EPOLLERR)){
				//cout << "exception event from: " << fd << ", events: " << events[i].events << endl; // for debug
				cur_conn->close_conn();
			}
			else if(events[i].events & (EPOLLIN)){
				//cout << "read event from: " << fd << endl; // for debug
				if(fd == -1){
					continue;
				}
				if(cur_conn->read()){
					t_pool->append(cur_conn);
					//cout << "append pthread success for fd: " << fd << endl;
				}
				else{
					cur_conn->close_conn();
				}
			}
			else if(events[i].events & (EPOLLOUT)){
				//cout << "write event from: " << fd << endl; // for debug
				if(fd == -1){
					continue;
				}
				if(!cur_conn->write()){
					cur_conn->close_conn();
				}
			}
		}
	}

	delete listen_conn;

}

void epoll_lt(int epollfd, int listenfd, unique_ptr<threadpool<pink_http_conn> > &t_pool){

	// LT 模式，沒有 EPOLLONESHOT
	cout << "Using Epoll LT mode for listenfd" << endl;
	pink_http_conn *listen_conn = new pink_http_conn;
	listen_conn->init_listen(listenfd);
	pink_epoll_addfd(epollfd, listenfd, listen_conn, (EPOLLIN), false);

	while(true){
		//cout << ".......waiting.........." << endl;
		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if(event_number < 0 && errno != EINTR){
			perror("epoll failed:");
			break;
		}
		for(int i = 0; i < event_number; ++i){
			pink_http_conn *cur_conn = (pink_http_conn*)events[i].data.ptr;
			int fd = cur_conn->get_fd();
			if(fd == listenfd){
				sockaddr_in client_addr;
				int connfd = accept_conn(listenfd, client_addr);
				if(connfd < 0)
					continue;
				pink_http_conn *new_conn = new pink_http_conn;
				new_conn->init(connfd, client_addr);
				//cout << "Accepted: " << connfd << " , conn_id: " << conn_id << endl;
			}
			else if(events[i].events & (EPOLLHUP | EPOLLERR)){
				//cout << "exception event from: " << fd << ", events: " << events[i].events << endl; // for debug
				cur_conn->close_conn();
			}
			else if(events[i].events & (EPOLLIN)){
				//cout << "read event from: " << fd << endl; // for debug
				if(fd == -1){
					continue;
				}
				if(cur_conn->read()){
					t_pool->append(cur_conn);
					//cout << "append pthread success for fd: " << fd << endl;
				}
				else{
					cur_conn->close_conn();
				}
			}
			else if(events[i].events & (EPOLLOUT)){
				//cout << "write event from: " << fd << endl; // for debug
				if(fd == -1){
					continue;
				}
				if(!cur_conn->write()){
					cur_conn->close_conn();
				}
			}
		}
	}
}