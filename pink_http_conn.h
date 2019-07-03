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
#include <assert.h>    // 包含assert宏
#include <sys/stat.h>  // 包含stat()
#include <string.h>    // C语言的字符串库
#include <pthread.h>   // 提供线程支持
#include <stdio.h>     // C语言标准I/O
#include <stdlib.h>    // standard library，包含malloc(),calloc(),free(),atoi(),exit(),etc
#include <sys/mman.h>  // memory management
#include <stdarg.h>    // standard arguments，为了让函数能够接受可变参数 ...
#include <errno.h>     // 定义了通过错误码来回报错误信息的宏
#include <sys/uio.h>   // writev
#include "tools/IPC_tool.h" 
#include "pink_epoll.h"

class pink_http_conn{
public:
	static const int FILENAME_LEN = 1024;        // 文件名最大长度
	static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
	static const int WRITE_BUFFER_SIZE = 1024;  // 写缓冲区的大小

	// HTTP 的请求方法
	enum METHOD{ GET=0, POST, HEAD, PUT, DELETE, TRACE,
					OPTIONS, CONNECT, PATCH };

	// 解析客户请求时，主状态机所处的状态
	enum CHECK_STATE{ CHECK_STATE_REQUESTLINE = 0,
				  CHECK_STATE_HEADER,
				  CHECK_STATE_CONTENT };

	// 处理HTTP请求的可能结果
	enum HTTP_CODE{ NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, 
				FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};

	// 行的读取状态
	enum LINE_STATUS{ LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
	pink_http_conn(){}
	~pink_http_conn(){}

public:
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
	// 解析HTTP请求
	HTTP_CODE process_read();
	// 填充HTTP应答
	bool process_write(HTTP_CODE ret);

	// 下面这一组函数被process_read调用以分析HTTP请求
	HTTP_CODE parse_request_line(char *text);
	HTTP_CODE parse_headers(char *text);
	HTTP_CODE parse_content(char *text);
	HTTP_CODE do_request();
	char* get_line(){ return read_buf + start_line; }
	LINE_STATUS parse_line();
	
	// 下面这一组函数被process_write调用以填充HTTP应答
	void unmap();
	bool add_response(const char *format, ...);
	bool add_content(const char *content);
	bool add_status_line(int status, const char *title);
	bool add_headers(int content_length);
	bool add_content_length(int content_length);
	bool add_linger();
	bool add_blank_line();

public:
	static int epollfd;               // 所有socket上的事件都被注册到同一个epoll内核事件表中
	static int user_count;            // 统计用户数量

private:
	int sockfd;                       // 该HTTP连接的socket
	sockaddr_in address;			  // 对方的socket地址
	
	char read_buf[READ_BUFFER_SIZE];  // 读缓冲区
	int read_idx;                     // 已经读入的客户数据最后一个字节的下一个位置
	int checked_idx;                  // 当前正在分析的字符在缓冲区的位置
	int start_line;                   // 需要被解析的行的起始位置
	char write_buf[WRITE_BUFFER_SIZE];// 写缓冲区
	int write_idx;                    // 写缓冲区中待发送的字节
	
	CHECK_STATE check_state;          // 主状态机的当前状态
	METHOD method;                    // 请求方法

	char real_file[FILENAME_LEN];     // 客户请求的文件的完整路径，内容为：doc_root(网站根目录） + m_url
	char *url;                        // 客户请求的目标文件的文件名
	char *version;                    // HTTP协议版本号，这里仅支持HTTP/1.1
	char *host;                       // 主机名
	int content_length;               // HTTP请求的消息体长度
	bool linger;                      // HTTP请求是否要求保持连接
	
	char *file_address;               // 客户请求的目标文件被mmap到内存中的起始位置

	
	struct stat file_stat;            // 目标文件的状态，通过它判断文件是否存在，是否为目录，是否可读，文件大小等
	// 采用 writev() 聚集写，收集内存中分散的若干缓冲区，写至文件的连续区域中
	/*
	   struct iovec{
			ptr_t iov_base; // starting address
			size_t iov_len; // length in bytes
	   }

	*/
	struct iovec iv[2];
	int iv_count;                      // 表示被写内存块的数量
};

#endif


