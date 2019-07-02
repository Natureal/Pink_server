#ifndef PINK_EPOLL_H
#define PINK_EPOLL_H

#include <sys/epoll.h>

const int MAX_EVENT_NUMBER = 10000;

int pink_epoll_create(const int size);
void pink_epoll_addfd(int epollfd, int fd, int events, bool one_shot);
void pink_epoll_modfd(int epollfd, int fd, int events);
void pink_epoll_removefd(int epollfd, int fd);
int pink_epoll_wait(int epollfd, epoll_event *events, int max_event_number, int timeout = -1);
void pink_epoll_handle(int epollfd, int listenfd, epoll_event *events, int event_number, threadpool *pool);



#endif