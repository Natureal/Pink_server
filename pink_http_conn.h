#ifndef PINK_HTTP_CONN_H
#define PINK_HTTP_CONN_H

#include <iostream>
#include "tools/IPC_tool.h" 
#include "tools/socket_tool.h"
#include "pink_epoll.h"
#include "pink_http_machine.h"

// http 连接体类，负责建立/关闭http连接，读入缓冲区数据/输出数据到缓冲区
class pink_http_conn{
public:
	pink_http_conn(){}
	~pink_http_conn(){}

	// 初始化新接受的连接
	void init(int sockfd, const sockaddr_in &addr);
	// 关闭连接
	void close_conn(bool read_close = true);
	// 处理客户请求
	void process();
	// 非阻塞读
	bool read();
	// 非阻塞写
	bool write();

private:
	// 空参初始化连接
	void init();

public:
	static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
	static const int WRITE_BUFFER_SIZE = 1024;  // 写缓冲区的大小
	static int epollfd;               // 所有socket上的事件都被注册到同一个epoll内核事件表中
	static int user_count;            // 统计用户数量

	// 处理HTTP请求的可能结果
	enum HTTP_CODE{ NOT_COMPLETED, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, 
				FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};

private:
	int sockfd;                       // 该HTTP连接的socket
	sockaddr_in address;			  // 对方的socket地址
	
	char read_buf[READ_BUFFER_SIZE];  // 读缓冲区
	int read_idx;                     // 已经读入的客户数据最后一个字节的下一个位置

	char write_buf[WRITE_BUFFER_SIZE];// 写缓冲区
	int write_idx;                    // 写缓冲区中待发送的字节

	pink_http_machine machine;

};

#endif


