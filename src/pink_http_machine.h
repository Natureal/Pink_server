#ifndef PINK_HTTP_MACHINE_H
#define PINK_HTTP_MACHINE_H

#include <assert.h>    // 包含assert宏
#include <sys/stat.h>  // 包含stat()
#include <sys/mman.h>  // memory management
#include <sys/types.h> // 提供基础数据类型，如pid_t,size_t,time_t,clock_t,etc
#include <stdarg.h>    // standard arguments，为了让函数能够接受可变参数 ...
#include <errno.h>     // 定义了通过错误码来回报错误信息的宏
#include <unistd.h>    // 提供对POSIX API的访问功能，提供对系统调用的封装，如fork,pipe,read,write
#include <signal.h>    // 定义信号处理部分

#include <sys/epoll.h> // 提供epoll多路复用I/O
#include <fcntl.h>     // 包含open,fcntl,fclose,etc
#include <sys/socket.h>// 包含套接字的定义
#include <netinet/in.h>// 包含socketaddr_in,htons的定义
#include <arpa/inet.h> // 包含htons(),htonl(),inet_pton(),struct linger的定义
#include <string.h>    // C语言的字符串库
#include <pthread.h>   // 提供线程支持
#include <stdio.h>     // C语言标准I/O
#include <stdlib.h>    // standard library，包含malloc(),calloc(),free(),atoi(),exit(),etc
#include <sys/uio.h>   // writev

#include <string>
#include <unordered_map>
#include <map>
#include "tools/basic_tool.h"

using std::string;
using std::unordered_map;


class pink_http_machine{
public:
	// HTTP 的请求方法
	enum METHOD{ GET=0, POST, HEAD, PUT, DELETE, TRACE,
					OPTIONS, CONNECT, PATCH, UNKNOWN_METHOD };

	// 解析客户请求时，主状态机所处的状态
	enum CHECK_STATE{ CHECK_STATE_REQUESTLINE = 0,
				  CHECK_STATE_HEADER,
				  CHECK_STATE_CONTENT };

	// 处理HTTP请求的可能结果
	enum HTTP_CODE{ NOT_COMPLETED, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, 
				FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};

	// 行的读取状态
	enum LINE_STATUS{ LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
	pink_http_machine(){}
	~pink_http_machine(){}
	void init(char *read_buf, char *write_buf, int *read_idx, int *write_idx,
								const int read_buf_size, const int write_buf_size);
	// 解析HTTP请求
	HTTP_CODE process_read();
	// 填充HTTP应答
	bool process_write(HTTP_CODE ret, struct iovec *iv, int &iv_count);
	// 解映射工具函数
	void unmap();
	// 获得 linger
	bool get_linger();

private:

	// 下面这一组函数被process_read调用以分析HTTP请求
	HTTP_CODE parse_request_line(char *text);
	METHOD parse_request_method(char *text);
	HTTP_CODE parse_headers(char *text);
	HTTP_CODE parse_content(char *text);
	HTTP_CODE do_request();
	LINE_STATUS parse_line();
	
	// 下面这一组函数被process_write调用以填充HTTP应答
	bool add_response(const char *format, ...);
	bool add_content(const char *content);
	bool add_status_line(int status, const char *title);
	bool add_headers(int content_length);
	bool add_content_length(int content_length);
	bool add_content_type(const char *content_type);
	bool add_linger();
	bool add_blank_line();

public:
	// 静态常量成员，可在类内初始化
	static constexpr int FILENAME_LEN = 1024;       // 文件名最大长度

	static unordered_map<string, string> mime_type;
	static unordered_map<string, string> mime_map_init();

private:

	// 读取/写入 相关信息
	CHECK_STATE check_state;          // 主状态机的当前状态
	int checked_idx;                  // 下一个要分析的字符在缓冲区的位置
	int start_of_line;                   // 需要被解析的行的起始位置
	char *m_read_buf;                 // 读缓冲区指针，映射到 http_conn 的读缓冲区
	char *m_write_buf;				  // 同理
	int *m_read_idx;			      // 映射到 http_conn 的 read_idx
	int *m_write_idx;				  // 同理
	int m_read_buf_size;
	int m_write_buf_size;

	// 请求行相关信息
	METHOD method;                    // 请求方法
	char *url;                        // 客户请求的目标文件的文件名
	char *version;                    // HTTP协议版本号，这里仅支持HTTP/1.1

	// 头部相关信息
	bool linger;                      // HTTP请求是否要求保持连接
	char *accept;
	char *accept_charset;
	char *accept_encoding;
	char *accept_language;
	char *cookie;
	int content_length;               // HTTP请求的消息体长度
	char *host;                       // 主机名
	char *date;						  // 提供报文创建时间
	char *client_ip;
	char *client_email;				  // 提供客户端 email 地址
	char *referer;
	char *user_agent;				  // 提供发起请求的应用程序名称
	
	char request_file[FILENAME_LEN];     // 客户请求的文件的完整路径，内容为：doc_root(网站根目录） + m_url
	char *file_address;               // 客户请求的目标文件被mmap到内存中的起始位置

	// 文件信息
	struct stat file_stat;            // 目标文件的状态，通过它判断文件是否存在，是否为目录，是否可读，文件大小等

};

#endif