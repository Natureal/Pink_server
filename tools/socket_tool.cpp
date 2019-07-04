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
	if(listenfd < 0)
		goto error;

	// 启用地址重用，虽然可能会收到非期望数据
	int enable = 1;
	if(setsocketopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		goto error;
	
	// 将监听socket绑定到socketaddr
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		goto error;

	if(listen(listenfd, LISTENQUEUE_LEN) < 0)
		goto error;

	return listenfd;

error:
	perror("bind and listen failed");
	return -1;
}

int accept_conn(const int listenfd, sockaddr_in &client_addr){
	bzero(&client_addr, sizeof(client_addr));
	socklen_t client_addrlen = sizeof(client_addr);
	int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);
	if(connfd < 0){
		perror("accept connection failed");
		return -1;
	}
	return connfd;
}

void send_error(const int connfd, const char *info){
	cout << string(info) << endl;
	send(connfd, info, strlen(info), 0);
	close(connfd);
}