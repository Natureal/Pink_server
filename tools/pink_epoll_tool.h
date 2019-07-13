#ifndef PINK_EPOLL_TOOL_H
#define PINK_EPOLL_TOOL_H

#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <memory>

const int MAX_EVENT_NUMBER = 100000;

int pink_epoll_create(const int size);
int pink_epoll_addfd(int epollfd, int fd, void *conn, int event_code, bool one_shot);
int pink_epoll_modfd(int epollfd, int fd, void *conn, int event_code, bool one_shot);
int pink_epoll_removefd(int epollfd, int fd);
int pink_epoll_wait(int epollfd, struct epoll_event *events, int max_event_number, int timeout = -1);

#endif