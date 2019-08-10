#ifndef SOCKET_TOOL_H
#define SOCKET_TOOL_H
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

const int LISTENQUEUE_LEN = 4096;

int set_nonblocking(const int fd);
int bind_and_listen(int port);
int accept_conn(const int listenfd, sockaddr_in &client_addr);
void send_error(const int connfd, const char *info);

#endif
