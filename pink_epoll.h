#ifndef PINK_EPOLL_H
#define PINK_EPOLL_H
#include "pink_threadpool.h"
#include "pink_http_conn.h"
#include "tools/pink_epoll_tool.h"

using std::unique_ptr;

void epoll_et(int epollfd, int listenfd, unique_ptr<threadpool<pink_http_conn> > &t_pool);

void epoll_lt(int epollfd, int listenfd, unique_ptr<threadpool<pink_http_conn> > &t_pool);


#endif