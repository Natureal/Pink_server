#include "pink_http_conn.h"

// 定义HTTP相应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n"; 
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get the file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";
const char *doc_root = "/var/www/html";

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::close_conn(bool read_close){
	if(read_close && (m_sockfd != -1)){
		pink_epoll_removefd(m_epollfd, m_sockfd);
		m_sockfd = -1;
		m_user_count--;
	}
}

void http_conn::init(int sockfd, const sockaddr_in &addr){
	m_sockfd = sockfd;
	m_address = addr;
	// 如下两行可以避免TIME_WAIT，仅用于测试，实际中应该注释掉
	//int reuse = 1;
	//etsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	pink_epoll_addfd(m_epollfd, sockfd, (EPOLLIN | EPOLLET | EPOLLRDHUP), true);
	m_user_count++;
	
	this->init();
}

void http_conn::init(){
	m_check_state = CHECK_STATE_REQUESTLINE;
	m_linger = false; // 不保持HTTP连接
	
	m_method = GET;
	m_url = 0;
	m_version = 0;
	m_content_length = 0;
	m_host = 0; // 主机名
	m_start_line = 0; // 正在解析的行的起始位置
	m_checked_idx = 0;
	m_read_idx = 0;
	m_write_idx = 0;
	memset(m_read_buf, '\0', READ_BUFFER_SIZE);
	memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
	memset(m_real_file, '\0', FILENAME_LEN);
}

// 从状态机
http_conn::LINE_STATUS http_conn::parse_line(){
	char temp;
	for(; m_checked_idx < m_read_idx; ++m_checked_idx){
		temp = m_read_buf[m_checked_idx];
		if(temp == '\r'){
			if((m_checked_idx + 1) == m_read_idx){
				return LINE_OPEN;
			}
			else if(m_read_buf[m_checked_idx + 1] == '\n'){
				m_read_buf[m_checked_idx++] = '\0';
				m_read_buf[m_checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if(temp == '\n'){
			if((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r')){
				m_read_buf[m_checked_idx - 1] = '\0';
				m_read_buf[m_checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}

// 循环读取客户数据，直到无数据或者对方关系连接
bool http_conn::read(){
	if(m_read_idx >= READ_BUFFER_SIZE){
		return false;
	}

	int bytes_read = 0;
	while(true){
		bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
		if(bytes_read == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				break;
			}
			return false;
		}
		else if(bytes_read == 0){
			return false; // 关闭连接了
		}
		m_read_idx += bytes_read;
	}
	return true;
}

// 解析HTTP请求行，获取请求方法，目标URL，以及HTTP版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text){
	// strpbrk(): string pointer break
	// 找到任意字符出现的第一个位置
	m_url = strpbrk(text, " \t");
	if(!m_url){
		return BAD_REQUEST;
	}
	*m_url++ = '\0';

	char *method = text;
	if(strcasecmp(method, "GET") == 0){
		m_method = GET;
	}
	else{
		// 目前只处理GET
		return BAD_REQUEST;
	}

	m_url += strspn(m_url, " \t");
	m_version = strpbrk(m_url, " \t");
	if(!m_version){
		return BAD_REQUEST;
	}
	*m_version++ = '\0';
	m_version += strspn(m_version, " \t");
	if(strcasecmp(m_version, "HTTP/1.1") != 0){
		return BAD_REQUEST;
	}
	if(strncasecmp(m_url, "http://", 7) == 0){
		m_url += 7;
		m_url = strchr(m_url, '/');
	}

	if(!m_url || m_url[0] != '/'){
		return BAD_REQUEST;
	}

	m_check_state = CHECK_STATE_HEADER;
	
	// 尚未完成
	return NO_REQUEST;
}

// 解析HTTP请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text){
	// 遇到空行，解析完毕
	if(text[0] == '\0'){
		// 如果该HTTP请求还有消息体，则还需要读取 m_content_length 字节的消息体
		// 转到 CHECK_STATE_CONTENT 状态
		if(m_content_length != 0){
			m_check_state = CHECK_STATE_CONTENT;
			// 未完成
			return NO_REQUEST;
		}
		// 完整
		return GET_REQUEST;
	}
	// 处理connection头部字段
	else if(strncasecmp(text, "Connection:", 11) == 0){
		text += 11;
		text += strspn(text, " \t");
		if(strcasecmp(text, "keep-alive") == 0){
			m_linger = true;
		}
	}
	// 处理Content-Length头部字段
	else if(strncasecmp(text, "Content-Length", 15) == 0){
		text += 15;
		text += strspn(text, " \t");
		m_content_length = atol(text);
	}
	// 处理Host头部字段
	else if(strncasecmp(text, "Host:", 5) == 0){
		text += 5;
		text += strspn(text, " \t");
		m_host = text;
	}
	else{
		printf("oops! unknow header %s\n", text);
	}
	
	// 未完成
	return NO_REQUEST;
}

// 这里并没有真正解析HTTP请求的消息体，只是判断其是否被完整地读入了
http_conn::HTTP_CODE http_conn::parse_content(char *text){
	if(m_read_idx >= (m_content_length + m_checked_idx)){
		text[m_content_length] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

// 主状态机
// 调用 parse_line(), parse_request_line(), parse_headers(), parse_content()
http_conn::HTTP_CODE http_conn::process_read(){
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;
	char *text = 0;

	while(((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
			|| ((line_status = parse_line()) == LINE_OK)){
		text = get_line();
		m_start_line = m_checked_idx;
		printf("got 1 http line: %s\n", text);

		switch(m_check_state){
			case CHECK_STATE_REQUESTLINE:{
				ret = parse_request_line(text);
				if(ret == BAD_REQUEST){
					return BAD_REQUEST;
				}
				break;
			}
			case CHECK_STATE_HEADER:{
				ret = parse_headers(text);
				if(ret == BAD_REQUEST){
					return BAD_REQUEST;
				}
				else if(ret == GET_REQUEST){
					return do_request();
				}
				break;
			}
			case CHECK_STATE_CONTENT:{
				ret = parse_content(text);
				if(ret == GET_REQUEST){
					return do_request();
				}
				line_status = LINE_OPEN;
				break;
			}
			default:{
				return INTERNAL_ERROR;
			}
		}
	}

	return NO_REQUEST;
}

// 当得到一个完整，正确的HTTP请求时，我们就开始分析目标文件。
// 如果目标存在，对所有用户可读，且不是目录，就用mmap将其映射到m_file_address处。
http_conn::HTTP_CODE http_conn::do_request(){
	strcpy(m_real_file, doc_root);
	int len = strlen(doc_root);
	strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
	if(stat(m_real_file, &m_file_stat) < 0){
		return NO_RESOURCE;
	}
	
	// S_IROTH：其他用户具有可读取权限
	if(!(m_file_stat.st_mode & S_IROTH)){
		return FORBIDDEN_REQUEST;
	}

	if(S_ISDIR(m_file_stat.st_mode)){
		return BAD_REQUEST;
	}

	int fd = open(m_real_file, O_RDONLY);
	m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ,
									MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;
}

void http_conn::unmap(){
	if(m_file_address){
		munmap(m_file_address, m_file_stat.st_size);
		m_file_address = 0;
	}
}

// 写HTTP响应
bool http_conn::write(){
	int temp = 0;
	int bytes_have_sent = 0;
	int bytes_to_send = m_write_idx;
	
	// 没东西写
	if(bytes_to_send == 0){
		modfd(m_epollfd, m_sockfd, EPOLLIN);
		init();
		return true;
	}

	while(1){
		temp = writev(m_sockfd, m_iv, m_iv_count);
		if(temp <= -1){
			// TCP 写缓冲区没有空间，等待下一轮 EPOLLOUT 事件
			if(errno == EAGAIN){
				modfd(m_epollfd, m_sockfd, EPOLLOUT);
				return true;
			}
			unmap();
			return false;
		}

		bytes_to_send -= temp; // 这是 index 意义上的
		bytes_have_sent += temp; // index 意义上
		if(bytes_to_send <= bytes_have_sent){
			// 发送HTTP响应成功，根据HTTP请求中的Connection字段决定是否关闭连接
			unmap();
			if(m_linger){
				init();
				modfd(m_epollfd, m_sockfd, EPOLLIN);
				return true;
			}
			else{
				modfd(m_epollfd, m_sockfd, EPOLLIN);
				return false;
			}
		}
	}
}

// 往写缓冲区中写入待发送的数据
bool http_conn::add_response(const char *format, ...){
	if(m_write_idx >= WRITE_BUFFER_SIZE){
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	// 以format格式把arg_list里的参数输出到字符串中
	int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
	if(len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)){
		return false;
	}
	m_write_idx += len;
	va_end(arg_list);
	return true;
}

bool http_conn::add_status_line(int status, const char *title){
	return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len){
	add_content_length(content_len);
	add_linger();
	add_blank_line();
}

bool http_conn::add_content_length(int content_len){
	return add_response("Content-Length: %d\r\n", content_len);
}

bool http_conn::add_linger(){
	return add_response("Connection: %s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line(){
	return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char *content){
	return add_response("%s", content);
}

// 根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool http_conn::process_write(HTTP_CODE ret){
	switch(ret){
		case INTERNAL_ERROR:{
			add_status_line(500, error_500_title);
			add_headers(strlen(error_500_form));
			if(!add_content(error_500_form)){
				return false;
			}
			break;
		}
		case BAD_REQUEST:{
			add_status_line(400, error_400_title);
			add_headers(strlen(error_400_form));
			if(!add_content(error_400_form)){
				return false;
			}
			break;
		}
		case NO_RESOURCE:{
			add_status_line(404, error_404_title);
			add_headers(strlen(error_404_form));
			if(!add_content(error_404_form)){
				return false;
			}
			break;
		}
		case FORBIDDEN_REQUEST:{
			add_status_line(403, error_403_title);
			add_headers(strlen(error_403_form));
			if(!add_content(error_403_form)){
				return false;
			}
			break;
		}
		case FILE_REQUEST:{
			add_status_line(200, ok_200_title);
			if(m_file_stat.st_size != 0){
				add_headers(m_file_stat.st_size);
				m_iv[0].iov_base = m_write_buf;
				m_iv[0].iov_len = m_write_idx;
				m_iv[1].iov_base = m_file_address;
				m_iv[1].iov_len = m_file_stat.st_size;
				m_iv_count = 2;
				return true;
			}
			else{
				const char *ok_string = "<html><body></body></html>";
				add_headers(strlen(ok_string));
				if(!add_content(ok_string)){
					return false;
				}
			}
		}
		default:{
			return false;
		}
	}
	
	m_iv[0].iov_base = m_write_buf;
	m_iv[0].iov_len = m_write_idx;
	m_iv_count = 1;
	return true;
}
												
// 由线程池中的工作线程调用，是处理HTTP请求的入口函数
void http_conn::process(){
	HTTP_CODE read_ret = process_read();
	if(read_ret == NO_REQUEST){
		// 请求还不完整，通知epoll，再读
		modfd(m_epollfd, m_sockfd, EPOLLIN);
		return;
	}

	bool write_ret = process_write(read_ret);
	if(!write_ret){
		close_conn();
	}
	// 准备好写的数据了，通知epoll，写出
	modfd(m_epollfd, m_sockfd, EPOLLOUT);
}


	









