#ifndef PINK_EPOLL_H
#define PINK_EPOLL_H

#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>

const int MAX_EVENT_NUMBER = 10000;

int pink_epoll_create(const int size);
int pink_epoll_addfd(int epollfd, int fd, int events, bool one_shot);
int pink_epoll_modfd(int epollfd, int fd, int events);
int pink_epoll_removefd(int epollfd, int fd);
int pink_epoll_wait(int epollfd, epoll_event *events, int max_event_number, int timeout = -1);

#endif