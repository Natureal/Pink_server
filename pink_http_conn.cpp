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

	read_idx = 0;
	write_idx = 0;
	memset(read_buf, '\0', READ_BUFFER_SIZE);
	memset(write_buf, '\0', WRITE_BUFFER_SIZE);

	machine.init(read_buf, write_buf, &read_idx, &write_idx, 
							READ_BUFFER_SIZE, WRITE_BUFFER_SIZE);
}

// 由线程池中的工作线程调用，是处理HTTP请求的入口函数
void pink_http_conn::process(){
	pink_http_machine::HTTP_CODE read_ret = machine.process_read();
	if(read_ret == pink_http_machine::NOT_COMPLETED){
		// 请求还不完整，通知epoll，再读
		pink_epoll_modfd(epollfd, sockfd, EPOLLIN);
		return;
	}

	bool write_ret = machine.process_write(read_ret, iv, iv_count);
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
	int bytes_to_send = iv[0].iov_len + iv[1].iov_len;
	
	// 没东西写
	if(bytes_to_send == 0){
		pink_epoll_modfd(epollfd, sockfd, EPOLLIN);
		init();
		return true;
	}

	while(1){
		temp = writev(sockfd, iv, iv_count);
		std::cout << "HERE!!!!!!!!!!!!!!!!" << std::endl;
		std::cout << "bytes_to_send: " << bytes_to_send << std::endl;
		std::cout << "temp: " << temp << std::endl;
		std::cout << write_buf << std::endl;

		// writev 出错，返回 -1
		if(temp <= -1){
			// TCP 写缓冲区没有空间，等待下一轮 EPOLLOUT 事件
			// EAGAIN: Resource temporarily unavailable
			if(errno == EAGAIN){
				pink_epoll_modfd(epollfd, sockfd, EPOLLOUT);
				return true;
			}
			// 其他错误原因，就不返回了
			machine.unmap();
			return false;
		}

		std::cout << "start writing" << std::endl;

		bytes_have_sent += temp; // index 意义上
		if(bytes_have_sent >= bytes_to_send){
			// 发送HTTP响应成功，根据HTTP请求中的Connection字段决定是否关闭连接
			machine.unmap();
			std::cout << "linger: " << machine.get_linger() << std::endl;
			if(machine.get_linger()){
				init();
				pink_epoll_modfd(epollfd, sockfd, EPOLLIN);
				return true;
			}
			else{
				//pink_epoll_modfd(epollfd, sockfd, EPOLLIN);
				return false;
			}
		}
	}
}

