#ifndef SOCKET_TOOL_H
#define SOCKET_TOOL_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

const int LISTENQUEUE_LEN = 1024;

int set_nonblocking(const int fd);
void add_signal(const int sig, void (handler)(int), bool restart = true);
int socket_bind_listen(const int port);


#endif
