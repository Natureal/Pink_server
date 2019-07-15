#ifndef PINK_CONN_POOL_H
#define PINK_CONN_POOL_H

#include <list>
#include <exception>
#include "tools/IPC_tool.h"

// 线程安全的连接池

template<typename T>
class pink_conn_pool{
public:
	pink_conn_pool(int init_cur_number, int init_max_number);
	~pink_conn_pool();
	T* get_conn(); // 从连接池获取一个连接的指针
	void return_conn(T* conn); // 放回一个连接

private:
	int used;
	int cur_number;
	int max_number;
	list<T*> conn_lst; // 连接体链表
	mutex lst_locker;
};

template<typename T>
pink_conn_pool<T>::pink_conn_pool(int init_cur_number, int init_max_number):
			cur_number(init_cur_number), max_number(init_max_number)
{
	if(cur_number <= 0)
		throw std::exception();
	used = 0;

	lst_locker.lock();
	for(int i = 0; i < cur_number; ++i){
		T* tmp = new T;
		if(tmp == nullptr)
			delete this;
		conn_lst.push_back(tmp);
	}
	lst_locker.unlock();
}

template<typename T>
pink_conn_pool<T>::~pink_conn_pool(){
	while(!conn_lst.empty()){
		T* tmp = conn_lst.front();
		delete tmp;
		conn_lst.pop_front();
	}
}

template<typename T>
T* pink_conn_pool<T>::get_conn(){
	lst_locker.lock();
	if(conn_lst.empty()){
		if(cur_number < max_number){
			T* one_more_conn = new T;
			if(one_more_conn != nullptr){
				++cur_number;
				++used;
			}
			lst_locker.unlock();
			return one_more_conn;
		}
		lst_locker.unlock();
		return nullptr;
	}
	T* tmp = conn_lst.front();
	conn_lst.pop_front();
	++used;
	lst_locker.unlock();
	return tmp;
}

template<typename T>
void pink_conn_pool<T>::return_conn(T* conn){
	lst_locker.lock();
	conn_lst.push_back(conn);
	--used;
	lst_locker.unlock();
}

#endif