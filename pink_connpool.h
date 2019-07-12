#ifndef PINK_CONNPOOL_H
#define PINK_CONNPOOL_H

#include <unordered_map>
#include <queue>
#include <memory>
#include <vector>
#include "tools/socket_tool.h"

using std::unique_ptr;
using std::default_delete;
using std::priority_queue;
using std::vector;

template<typename T>
class pink_connpool{
public:
	pink_connpool(int conn_number = 10000, int max_conn_number = 65535);
	int append_conn(int fd, const sockaddr_in &addr);
	int get_id(int fd){ return fd_conn_map[fd]; }
	bool close_conn(int fd);
	bool read(int fd);
	bool write(int fd);
	T& get_conn(int fd);
	size_t get_size();
	bool full();

private:
	size_t used;
	size_t size;
	size_t max_size;
	// 连接池数组
	unique_ptr<T[]> conn_array;
	// 用于存放 fd -> index 的数组
	unique_ptr<int[]> fd_conn_map;
	// 用于存放可用 index 的小根堆
	priority_queue<int, vector<int>, std::greater<int> > avai_idx;
};

template<typename T>
pink_connpool<T>::pink_connpool(int conn_number, int max_conn_number):
		size(conn_number), max_size(max_conn_number), conn_array(new T[size]), 
		fd_conn_map(new int[max_conn_number])
{
	if(!conn_array){
		throw std::exception();
	}

	if(!fd_conn_map){
		throw std::exception();
	}

	memset(fd_conn_map.get(), -1, sizeof(fd_conn_map.get()));

	while(!avai_idx.empty()) avai_idx.pop();
	for(int i = 0; i < conn_number; ++i){
		avai_idx.push(i);
	}
}

template<typename T>
bool pink_connpool<T>::close_conn(int fd){
	conn_array[fd_conn_map[fd]].close_conn();
	avai_idx.push(fd_conn_map[fd]);
	fd_conn_map[fd] = -1;
}

template<typename T>
int pink_connpool<T>::append_conn(int fd, const sockaddr_in &addr){
	if(avai_idx.empty()){
		return -1;
	}

	int cur_idx = avai_idx.top();
	avai_idx.pop();

	fd_conn_map[fd] = cur_idx;

	conn_array[cur_idx].init(fd, addr);

	return cur_idx;
}

template<typename T>
bool pink_connpool<T>::read(int fd){
	return conn_array[fd_conn_map[fd]].read();
}

template<typename T>
bool pink_connpool<T>::write(int fd){
	return conn_array[fd_conn_map[fd]].write();
}

template<typename T>
T& pink_connpool<T>::get_conn(int fd){
	return conn_array[fd_conn_map[fd]];
}

template<typename T>
size_t pink_connpool<T>::get_size(){
	return size;
}

template<typename T>
bool pink_connpool<T>::full(){
	return size >= max_size;
}


#endif