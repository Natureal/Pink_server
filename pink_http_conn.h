#ifndef PINK_HTTP_CONN_H
#define PINK_HTTP_CONN_H

#include <unistd.h>    // 提供对POSIX API的访问功能，提供对系统调用的封装，如fork,pipe,read,write
#include <signal.h>    // 定义信号处理部分
#include <sys/types.h> // 提供基础数据类型，如pid_t,size_t,time_t,clock_t,etc
#include <sys/epoll.h> // 提供epoll多路复用I/O
#include <fcntl.h>     // 包含open,fcntl,fclose,etc
#include <sys/socket.h>// 包含套接字的定义
#include <netinet/in.h>// 包含socketaddr_in,htons的定义
#include <arpa/inet.h> // 包含htons(),htonl(),inet_pton(),struct linger的定义
#include <string.h>    // C语言的字符串库
#include <pthread.h>   // 提供线程支持
#include <stdio.h>     // C语言标准I/O
#include <stdlib.h>    // standard library，包含malloc(),calloc(),free(),atoi(),exit(),etc
#include <sys/mman.h>  // memory management
#include <stdarg.h>    // standard arguments，为了让函数能够接受可变参数 ...
#include <errno.h>     // 定义了通过错误码来回报错误信息的宏
#include <sys/uio.h>   // writev
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
	static const int FILENAME_LEN = 1024;       // 文件名最大长度
	static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
	static const int WRITE_BUFFER_SIZE = 1024;  // 写缓冲区的大小
	static int epollfd;               // 所有socket上的事件都被注册到同一个epoll内核事件表中
	static int user_count;            // 统计用户数量
	
	// 处理HTTP请求的可能结果
	enum HTTP_CODE{ NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, 
				FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};

private:
	int sockfd;                       // 该HTTP连接的socket
	sockaddr_in address;			  // 对方的socket地址
	
	char read_buf[READ_BUFFER_SIZE];  // 读缓冲区
	int read_idx;                     // 已经读入的客户数据最后一个字节的下一个位置

	int checked_idx;                  // 当前正在分析的字符在缓冲区的位置
	int start_line;                   // 需要被解析的行的起始位置

	char write_buf[WRITE_BUFFER_SIZE];// 写缓冲区
	int write_idx;                    // 写缓冲区中待发送的字节

	pink_http_machine machine;

};

#endif


