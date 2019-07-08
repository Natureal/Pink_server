#include "pink_http_conn.h"

int pink_http_conn::user_count = 0;
int pink_http_conn::epollfd = -1;

void pink_http_conn::close_conn(bool read_close){
	if(read_close && (sockfd != -1)){
		pink_epoll_removefd(epollfd, sockfd);
		sockfd = -1;
		user_count--;
	}
}

void pink_http_conn::init(int sockfd, const sockaddr_in &addr){
	this->sockfd = sockfd;
	this->address = addr;
	// 如下两行可以避免TIME_WAIT，仅用于测试，实际中应该注释掉
	//int reuse = 1;
	//etsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	//std::cout << "epoll fd: " << epollfd << " , sockfd: " << sockfd << std::endl; // for debug

	pink_epoll_addfd(epollfd, sockfd, (EPOLLIN | EPOLLET | EPOLLRDHUP), true);
	set_nonblocking(sockfd);

	user_count++;
	
	this->init();
}

void pink_http_conn::init(){

	linger = false; // 不保持HTTP连接
	
	checked_idx = 0;
	read_idx = 0;
	write_idx = 0;
	memset(read_buf, '\0', READ_BUFFER_SIZE);
	memset(write_buf, '\0', WRITE_BUFFER_SIZE);
	memset(real_file, '\0', FILENAME_LEN);

	machine.init(read_buf, write_buf);

}

// 由线程池中的工作线程调用，是处理HTTP请求的入口函数
void pink_http_conn::process(){
	HTTP_CODE read_ret = machine.process_read();
	if(read_ret == NO_REQUEST){
		// 请求还不完整，通知epoll，再读
		pink_epoll_modfd(epollfd, sockfd, EPOLLIN);
		return;
	}

	bool write_ret = machine.process_write(read_ret);
	if(!write_ret){
		close_conn();
	}
	// 准备好写的数据了，通知epoll，写出
	pink_epoll_modfd(epollfd, sockfd, EPOLLOUT);
}


// 循环读取客户数据，直到无数据或者对方关系连接
bool pink_http_conn::read(){
	if(read_idx >= READ_BUFFER_SIZE){
		return false;
	}

	int bytes_read = 0;
	while(true){
		bytes_read = recv(sockfd, read_buf + read_idx, READ_BUFFER_SIZE - read_idx, 0);
		if(bytes_read == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				break;
			}
			return false;
		}
		else if(bytes_read == 0){
			return false; // 关闭连接了
		}
		read_idx += bytes_read;
	}
	return true;
}

// 写HTTP响应
bool pink_http_conn::write(){
	int temp = 0;
	int bytes_have_sent = 0;
	int bytes_to_send = write_idx;
	
	// 没东西写
	if(bytes_to_send == 0){
		pink_epoll_modfd(epollfd, sockfd, EPOLLIN);
		init();
		return true;
	}

	while(1){
		temp = writev(sockfd, iv, iv_count);
		if(temp <= -1){
			// TCP 写缓冲区没有空间，等待下一轮 EPOLLOUT 事件
			if(errno == EAGAIN){
				pink_epoll_modfd(epollfd, sockfd, EPOLLOUT);
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
			if(linger){
				init();
				pink_epoll_modfd(epollfd, sockfd, EPOLLIN);
				return true;
			}
			else{
				pink_epoll_modfd(epollfd, sockfd, EPOLLIN);
				return false;
			}
		}
	}
}


void pink_http_conn::unmap(){
	if(file_address){
		munmap(file_address, file_stat.st_size);
		file_address = 0;
	}
}

// 往写缓冲区中写入待发送的数据
bool pink_http_conn::add_response(const char *format, ...){
	if(write_idx >= WRITE_BUFFER_SIZE){
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	// 以format格式把arg_list里的参数输出到字符串中
	int len = vsnprintf(write_buf + write_idx, WRITE_BUFFER_SIZE - 1 - write_idx, format, arg_list);
	if(len >= (WRITE_BUFFER_SIZE - 1 - write_idx)){
		return false;
	}
	write_idx += len;
	va_end(arg_list);
	return true;
}

bool pink_http_conn::add_status_line(int status, const char *title){
	return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool pink_http_conn::add_headers(int content_len){
	add_content_length(content_len);
	add_linger();
	add_blank_line();
}

bool pink_http_conn::add_content_length(int content_len){
	return add_response("Content-Length: %d\r\n", content_len);
}

bool pink_http_conn::add_linger(){
	return add_response("Connection: %s\r\n", (linger == true) ? "keep-alive" : "close");
}

bool pink_http_conn::add_blank_line(){
	return add_response("%s", "\r\n");
}

bool pink_http_conn::add_content(const char *content){
	return add_response("%s", content);
}

// 根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool pink_http_conn::process_write(HTTP_CODE ret){
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
			if(file_stat.st_size != 0){
				add_headers(file_stat.st_size);
				iv[0].iov_base = write_buf;
				iv[0].iov_len = write_idx;
				iv[1].iov_base = file_address;
				iv[1].iov_len = file_stat.st_size;
				iv_count = 2;
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
	
	iv[0].iov_base = write_buf;
	iv[0].iov_len = write_idx;
	iv_count = 1;
	return true;
}
												


