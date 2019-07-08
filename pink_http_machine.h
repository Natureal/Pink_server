#ifndef PINK_HTTP_MACHINE_H
#define PINK_HTTP_MACHINE_H

#include <assert.h>    // 包含assert宏
#include <sys/stat.h>  // 包含stat()

class pink_http_machine{
public:
	pink_http_machine(){}
	~pink_http_machine(){}
	init();

private:
	// 空参构造
	init(char *read_buf, char *write_buf);

	// 解析HTTP请求
	HTTP_CODE process_read();
	// 填充HTTP应答
	bool process_write(HTTP_CODE ret);

	// 下面这一组函数被process_read调用以分析HTTP请求
	HTTP_CODE parse_request_line(char *text);
	HTTP_CODE parse_headers(char *text);
	HTTP_CODE parse_content(char *text);
	HTTP_CODE do_request();
	char* get_line();
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

private:

	METHOD method;                    // 请求方法
	CHECK_STATE check_state;          // 主状态机的当前状态

	char *m_read_buf;                   // 读缓冲区指针，映射到 http_conn 的读缓冲区
	char *m_write_buf;				  // 同理

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