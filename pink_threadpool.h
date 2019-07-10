#ifndef PINK_THREADPOOL_H
#define PINK_THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include <memory>
#include "tools/IPC_tool.h"

// 半同步/半反应堆形式

// 线程池类
template<typename T>
class threadpool{
public:
	threadpool(int thread_number = 8, int max_requests = 10000);
	~threadpool();
	bool append(T *request);

private:
	// 不断从工作队列中取出任务并执行
	// worker 作为 pthread_create 的第三个参数，必须为静态函数
	static void *worker(void *arg);
	std::shared_ptr<pthread_t> make_shared_array(size_t size);
	void run();

private:
	int thread_number; // 线程池中的线程数
	int max_requests; // 允许的最大请求数

	// 这里使用智能指针纯属练习，实际上没有必要
	std::shared_ptr<pthread_t> threads; // 进程池数组

	std::list<T*> work_queue; // 请求队列
	mutex queue_locker; // 保护请求队列的互斥锁
	sem queue_stat; // 是否有任务需要处理
	bool thread_stop; // 是否结束线程
};


template<typename T>
std::shared_ptr<pthread_t> threadpool<T>::make_shared_array(size_t size){
    return std::shared_ptr<pthread_t> (new pthread_t[size], std::default_delete <pthread_t[]> ());
}

// 构造函数
template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):
	thread_number(thread_number), max_requests(max_requests),
	thread_stop(false), threads(NULL)
{
	if((thread_number <= 0) || (max_requests <= 0))
		throw std::exception();
	
	// 初始化线程数组 unsigned long int[thread_number]
	threads = make_shared_array(thread_number);

	if(!threads)
		throw std::exception();

	// 创建 thread_number 个线程，并将它们都设置为脱离线程
	for(int i = 0; i < thread_number; ++i){
		printf("create the %dth thread\n", i);
		// this 为 worker 函数的传入参数
		if(pthread_create(threads.get() + i, NULL, worker, this) != 0){
			throw std::exception();
		}
		// 将所有进程都标记为脱离模式
		if(pthread_detach(*(threads.get() + i))){
			throw std::exception();
		}
	}
}

// 析构函数
template<typename T>
threadpool<T>::~threadpool(){
	thread_stop = true;
}

// 往请求队列里添加任务
template<typename T>
bool threadpool<T>::append(T *request){
	// 操作工作队列时一定要加锁，因为它被所有线程共享
	if(!queue_locker.lock()) return false;

	if(work_queue.size() > max_requests){
		queue_locker.unlock();
		return false;
	}
	work_queue.push_back(request);
	if(!queue_locker.unlock()) return false;
	if(!queue_stat.post()) return false;
	return true;
}

// 工作线程运行的函数
template<typename T>
void *threadpool<T>::worker(void *arg){
	threadpool *pool = (threadpool *)arg;
	pool->run();  // 静态函数调用成员函数以使用成员变量
	return pool;
}

template<typename T>
void threadpool<T>::run(){
	// 线程循环（阻塞）等待工作队列来任务
	while(!thread_stop){
		queue_stat.wait(); // 等待任务处理的信号
		queue_locker.lock(); // 锁住工作队列
		if(work_queue.empty()){
			queue_locker.unlock();
			continue;
		}
		// 取出工作队列中的元素
		T *request = work_queue.front();
		work_queue.pop_front();
		queue_locker.unlock();
		if(!request){
			continue;
		}
		request->process();
	}
}


#endif