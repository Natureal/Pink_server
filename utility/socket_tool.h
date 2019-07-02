#ifndef SOCKET_TOOL_H
#define SOCKET_TOOL_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

const int LISTENQUEUE_LEN = 1024;

int set_nonblocking(const int fd);
int bind_and_listen(const int port);
void accept_conn(

#endif
