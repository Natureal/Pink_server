#ifndef PINK_EPOLL_H
#define PINK_EPOLL_H
#include "pink_threadpool.h"
#include "pink_connpool.h"
#include "pink_http_conn.h"
#include "tools/pink_epoll_tool.h"

void epoll_et(int epollfd, int listenfd, unique_ptr<threadpool<pink_http_conn> > &t_pool,
							unique_ptr<pink_connpool<pink_http_conn> > &c_pool);

void epoll_lt(int epollfd, int listenfd, unique_ptr<threadpool<pink_http_conn> > &t_pool,
							unique_ptr<pink_connpool<pink_http_conn> > &c_pool);



#endif