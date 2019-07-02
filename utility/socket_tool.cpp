#include "socket_tool.h"


int set_nonblocking(const int fd){
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

int bind_and_listen(const int port){
	// 检查端口号范围
	if(port <= 1024 || port >= 65535){
		port = 7777;
	}
	
	// 创建TCP监听socket
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if(listenfd < 0){
		perror("creating listenfd failed");
	}

	// 启用地址重用，虽然可能会收到非期望数据
	int enable = 1;
	if(setsocketopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		perror("setsocketopt failed");
	}
	
	// 将监听socket绑定到socketaddr
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		perror("bind failed");
	}

	if(listen(listenfd, LISTENQUEUE_LEN) < 0){
		perror("listen failed");
	}

	return listenfd;
}

