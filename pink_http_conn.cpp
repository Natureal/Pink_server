#include "pink_http_conn.h"

int pink_http_conn::user_count = 0;
int pink_http_conn::epollfd = -1;

int pink_http_conn::get_fd(){
	return sockfd;
}

void pink_http_conn::close_conn(){
	timer->cancel();
	if(sockfd != -1){
		pink_epoll_removefd(epollfd, sockfd);
		sockfd = -1;
		user_count--;
	}
	delete this;
}

void pink_http_conn::init_listen(int sockfd){
	this->sockfd = sockfd;
}

void pink_http_conn::init(int sockfd, const sockaddr_in &addr){
	this->sockfd = sockfd;
	this->address = addr;
	// 如下两行可以避免TIME_WAIT，仅用于测试，实际中应该注释掉
	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	pink_epoll_add_connfd(epollfd, sockfd, this, (EPOLLIN | EPOLLET), true);
	set_nonblocking(sockfd);

	user_count++;
	
	this->init();
}

void pink_http_conn::init(){

	timeout = false;
	read_idx = 0;
	write_idx = 0;

	memset(read_buf, '\0', READ_BUFFER_SIZE);
	memset(write_buf, '\0', WRITE_BUFFER_SIZE);

	machine.init(read_buf, write_buf, &read_idx, &write_idx, 
							READ_BUFFER_SIZE, WRITE_BUFFER_SIZE);
}

// 由线程池中的工作线程调用，是处理HTTP请求的入口函数
void pink_http_conn::process(int flag){

	if(flag == READ){
		if(!read()){
			close_conn();
			return;
		}
		pink_http_machine::HTTP_CODE process_read_ret = machine.process_read();
		if(process_read_ret == pink_http_machine::NOT_COMPLETED){
			// 请求还不完整，通知epoll，再读
			pink_epoll_modfd(epollfd, sockfd, this, (EPOLLIN | EPOLLET), true);
			return;
		}
		bool process_write_ret = machine.process_write(process_read_ret, iv, iv_count);
		if(!process_write_ret){
			close_conn();
			return;
		}
		// 处理完请求直接写
		if(!write()){
			close_conn();
			return;
		}
	}
	else if(flag == WRITE){
		if(!write()){
			close_conn();
			return;
		}
	}
}

// 循环读取客户数据，直到无数据或者对方关系连接
bool pink_http_conn::read(){
	if(read_idx >= READ_BUFFER_SIZE){
		return false;
	}

	int bytes_read = 0;
	while(true){
		bytes_read = recv(sockfd, read_buf + read_idx, 
							READ_BUFFER_SIZE - read_idx, 0);
		//cout << "read: " << bytes_read << endl;
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
		pink_epoll_modfd(epollfd, sockfd, this, (EPOLLIN | EPOLLET), true);
		init();
		return true;
	}

	while(1){
		temp = writev(sockfd, iv, iv_count);

		// writev 出错，返回 -1
		if(temp <= -1){
			// TCP 写缓冲区没有空间，等待下一轮 EPOLLOUT 事件
			// EAGAIN: Resource temporarily unavailable
			if(errno == EAGAIN){
				pink_epoll_modfd(epollfd, sockfd, this, (EPOLLOUT | EPOLLET), true);
				return true;
			}
			// 其他错误原因，就不返回了
			machine.unmap();
			return false;
		}

		bytes_have_sent += temp; // index 意义上
		//std::cout << "bytes_have_sent: " << bytes_have_sent << endl;

		if(bytes_have_sent >= bytes_to_send){
			// 发送HTTP响应成功，根据HTTP请求中的Connection字段决定是否关闭连接
			machine.unmap();
			if(machine.get_linger()){
				init();
				pink_epoll_modfd(epollfd, sockfd, this, (EPOLLIN | EPOLLET), true);
				return true;
			}
			else{
				return false;
			}
		}
	}
}

