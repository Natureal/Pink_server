#include "pink_epoll.h"

struct epoll_event *events;

int pink_epoll_create(const int size){
	int epoll_fd = epoll_create(size);
	if(epoll_fd == -1){
		perror("create epollfd faild");
		return -1;
	}
	events = new struct epoll_event[MAX_EVENT_NUMBER];
	return epoll_fd;
}

int pink_epoll_addfd(int epollfd, int fd, int events, bool one_shot){
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) == -1){
		perror("add fd in epoll failed");
		return -1;
	}
}

int pink_epoll_modfd(int epollfd, int fd, int events){
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	if(epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event) == -1){
		perror("modify fd in epoll failed");
		return -1;
	}
}

int pink_epoll_removefd(int epollfd, int fd){
	std::cout << "remove fd: " << fd << std::endl;
	if(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0) == -1){
		perror("remove fd from epoll failed");
		return -1;
	}
}

int pink_epoll_wait(int epollfd, struct epoll_event *events, int max_event_number, int timeout){
	int event_number = epoll_wait(epollfd, events, max_event_number, timeout);
	return event_number;
}
