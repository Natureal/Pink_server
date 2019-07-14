#ifndef PINK_HTTP_CONN_H
#define PINK_HTTP_CONN_H

#include <iostream>
//#include "pink_conn_pool.h"
#include "tools/IPC_tool.h" 
#include "tools/socket_tool.h"
#include "tools/pink_epoll_tool.h"
#include "pink_http_machine.h"
#include "pink_conn_timer.h"

using std::shared_ptr;

// http 连接体类，负责建立/关闭http连接，读入缓冲区数据/输出数据到缓冲区
class pink_http_conn{
public:
	pink_http_conn(){}
	~pink_http_conn(){}

	// 初始化新接受的连接
	void init(int sockfd, const sockaddr_in &addr);
	// 初始化监听fd，包装成一个 http_conn，其实只需要其 sockfd 属性
	void init_listen(int sockfd);
	// 关闭连接
	void close_conn();
	// 处理客户请求
	void process(int flag);
	// 非阻塞读
	bool read();
	// 非阻塞写
	bool write();
	// 获取 fd
	int get_fd();

	enum OP_TYPE{ READ = 0, WRITE };

private:
	// 空参初始化连接
	void init();

public:
	static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
	static const int WRITE_BUFFER_SIZE = 1024;  // 写缓冲区的大小
	static int epollfd;               // 所有socket上的事件都被注册到同一个epoll内核事件表中
	static int user_count;            // 统计用户数量
	conn_timer *timer;
	bool timeout;

private:
	int sockfd;                       // 该HTTP连接的socket
	sockaddr_in address;			  // 对方的socket地址
	
	char read_buf[READ_BUFFER_SIZE];  // 读缓冲区
	int read_idx;                     // 已经读入的客户数据最后一个字节的下一个位置

	char write_buf[WRITE_BUFFER_SIZE];// 写缓冲区
	int write_idx;                    // 写缓冲区中待发送的字节

	// 采用 writev() 聚集写（gather write），收集内存中分散的若干缓冲区，写至文件的连续区域中
	/*
	   struct iovec{
			ptr_t iov_base; // starting address
			size_t iov_len; // length in bytes
	   }

	*/
	struct iovec iv[2];
	int iv_count;                      // 表示被写内存块的数量

	pink_http_machine machine;
};

#endif


