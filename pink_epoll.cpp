#include "pink_epoll.h"

extern conf_t conf;
extern struct epoll_event *events;
bool stop_server = false;

unique_ptr<pink_threadpool<pink_http_conn> > t_pool = nullptr;
unique_ptr<pink_time_heap> time_heap = nullptr;

int epoll_run(int epollfd, int listenfd){
	// 创建线程池 =================================================
	try{
		t_pool.reset(new pink_threadpool<pink_http_conn>(conf.max_thread_number, 10000));
	}catch( ... ){
		perror("create threadpool failed");
		return 1;
	}
	// 创建时间堆
	try{
		time_heap.reset(new pink_time_heap(2048));
	}catch( ... ){
		perror("create time heap failed");
		return 1;
	}

	if(conf.epoll_et)
		epoll_et(epollfd, listenfd);
	else
		epoll_lt(epollfd, listenfd);
	return 0;
}

void epoll_et(int epollfd, int listenfd){
	// ET 模式，带 EPOLLONESHOT
	cout << "Using Epoll ET mode for listenfd" << endl;
	pink_http_conn *listen_conn = new pink_http_conn;
	listen_conn->init_listen(listenfd);
	pink_epoll_add_connfd(epollfd, listenfd, listen_conn, (EPOLLIN | EPOLLET), true);

	int timeout = conf.timeslot;
	time_t next_timeout = time(NULL) + timeout;

	while(!stop_server){
		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, timeout * 1000);
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
					// 设置定时器
					conn_timer* new_timer = new conn_timer(conf.conn_timeout,
															&(new_conn->timeout));
					new_conn->timer = new_timer;
					// 将定时器添加到时间堆中
					time_heap->push(new_timer);
					//cout << "Accepted: " << connfd << endl;
					pink_epoll_modfd(epollfd, listenfd, listen_conn, (EPOLLIN | EPOLLET), true);
				}
			}
			else if(events[i].events & (EPOLLIN)){
				//cout << "read event from: " << fd << endl; // for debug
				cur_conn->timer->reset(conf.conn_timeout);
				if(t_pool->append(cur_conn, pink_http_conn::READ) == false)
					cur_conn->close_conn();
			}
			else if(events[i].events & (EPOLLOUT)){
				//cout << "write event to: " << fd << endl; // for debug
				if(t_pool->append(cur_conn, pink_http_conn::WRITE) == false)
					cur_conn->close_conn();
			}
			else{
				// 预留给其他 EPOLL 事件的接口, 包括 EPOLLHUP | EPOLLERR
				cur_conn->close_conn();
			}
		}

		// 超时时间到
		if(event_number == 0){
			time_heap->tick();
			next_timeout = time(NULL) + timeout;
			continue;
		}

		// 判断是否超时
		if(next_timeout <= time(NULL)){
			time_heap->tick();
			next_timeout = time(NULL) + timeout;
		}
	}

	delete listen_conn;
}

void epoll_lt(int epollfd, int listenfd){
	// LT 模式，沒有 EPOLLONESHOT
	cout << "Using Epoll LT mode for listenfd" << endl;
	pink_http_conn *listen_conn = new pink_http_conn;
	listen_conn->init_listen(listenfd);
	pink_epoll_add_connfd(epollfd, listenfd, listen_conn, (EPOLLIN), false);

	int timeout = conf.timeslot;
	time_t next_timeout = time(NULL) + timeout;

	while(!stop_server){
		int event_number = pink_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, timeout * 1000);
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
				// 设置定时器
				conn_timer* new_timer = new conn_timer(conf.conn_timeout,
															&(new_conn->timeout));
				new_conn->timer = new_timer;
				// 将定时器添加到时间堆中
				time_heap->push(new_timer);
				//cout << "Accepted: " << connfd << " , conn_id: " << conn_id << endl;
			}
			else if(events[i].events & (EPOLLIN)){
				//cout << "read event from: " << fd << endl; // for debug
				if(t_pool->append(cur_conn, pink_http_conn::READ) == false)
					cur_conn->close_conn();
			}
			else if(events[i].events & (EPOLLOUT)){
				//cout << "write event to: " << fd << endl; // for debug
				if(t_pool->append(cur_conn, pink_http_conn::WRITE) == false)
					cur_conn->close_conn();
			}
			else{
				// 预留给其他 EPOLL 事件的接口, 包括 EPOLLHUP | EPOLLERR
				cur_conn->close_conn();
			}
		}

		// 超时时间到
		if(event_number == 0){
			time_heap->tick();
			next_timeout = time(NULL) + timeout;
			continue;
		}

		// 判断是否超时
		if(next_timeout <= time(NULL)){
			time_heap->tick();
			next_timeout = time(NULL) + timeout;
		}
	}

	delete listen_conn;
}


// =============================================================================
// =============================================================================
// 时间堆实现
pink_time_heap::pink_time_heap(int init_cap) : cap(init_cap)
{
	size = 0;
	array = new conn_timer*[cap];
	if(array == nullptr)
		throw std::exception();
	for(int i = 0; i < cap; ++i)
		array[i] = nullptr;
}

pink_time_heap::~pink_time_heap(){
	for(int i = 0; i < size; ++i) delete array[i];
	delete[] array;
}

void pink_time_heap::push(conn_timer* t){
	if(t == nullptr)
		return;
	if(size >= cap) resize();
	int pos = size++;
	int fa = 0;
	for(; pos > 0; pos = fa){
		fa = (pos - 1) / 2;
		if(array[fa]->expire <= t->expire)
			break;
		array[pos] = array[fa];
	}
	array[pos] = t;
}

void pink_time_heap::cancel(conn_timer* t){
	if(t == nullptr)
		return;
	// 懒惰删除
	t->cancel();
}

conn_timer* pink_time_heap::top() const{
	if(empty())
		return nullptr;
	return array[0];
}

void pink_time_heap::pop(){
	if(empty())
		return;
	if(array[0]){
		delete array[0]; // 删除其所指向的conn_timer，而不会删除指针本身
		array[0] = array[--size];
		swap_down(0);
	}
}

void pink_time_heap::tick(){
	time_t cur_time = time(NULL);
	while(!empty()){
		conn_timer* tmp = top();
		if(tmp == nullptr || tmp->expire > cur_time)
			break;
		if(!tmp->canceled){
			tmp->cb_func();
		}
		pop();
	}
}

bool pink_time_heap::empty() const{
	return size == 0;
}

void pink_time_heap::swap_down(int pos){
	conn_timer* temp = array[pos];
	int child;
	while((child = pos * 2 + 1) < size){
		if(child < size - 1 && 
			array[child + 1]->expire < array[child]->expire){
			child++;
		}
		if(array[child]->expire < temp->expire){
			array[pos] = array[child];
			pos = child;
		}
		else{
			break;
		}
	}
	array[pos] = temp;
}

void pink_time_heap::resize(){
	conn_timer** tmp = new conn_timer*[2 * cap];
	if(tmp == nullptr)
		throw std::exception();
	for(int i = 0; i < 2 * cap; ++i)
		tmp[i] = nullptr;	
	cap *= 2;
	for(int i = 0; i < size; ++i) tmp[i] = array[i];
	delete[] array;
	array = tmp;
}
