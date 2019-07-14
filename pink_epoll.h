#ifndef PINK_EPOLL_H
#define PINK_EPOLL_H

#include <netinet/in.h>
#include "pink_thread_pool.h"
#include "pink_http_conn.h"

#include <iostream>
#include <netinet/in.h>
#include <time.h>
#include "pink_conn_timer.h"

using std::unique_ptr;
using std::shared_ptr;

int epoll_run(int epollfd, int listenfd);

void epoll_et(int epollfd, int listenfd);

void epoll_lt(int epollfd, int listenfd);

// 定时器时间堆
class pink_time_heap{
public:
	pink_time_heap() = default;
	pink_time_heap(int init_cap = 1024);
	~pink_time_heap();
	void push(conn_timer* t);
	void cancel(conn_timer *t);
	conn_timer* top() const;
	void pop();
	void tick();
	bool empty() const;

private:
	void swap_down(int pos);
	void resize();

private:
	conn_timer** array;
	int cap;
	int size;
};


#endif