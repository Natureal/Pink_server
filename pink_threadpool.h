#ifndef PINK_THREADPOOL_H
#define PINK_THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"

// 半同步/半反应堆形式

// 线程池类
template<typename T>
class threadpool{
public:
	threadpool(int thread_number = 8, int max_requests = 10000);
	~threadpool();
	bool append(T* request);

private:
	// 不断从工作队列中取出任务并执行
	// worker 作为 pthread_create 的第三个参数，必须为静态函数
	static void *worker(void *arg);
	void run();

private:
	int m_thread_number; // 线程池中的线程数
	int m_max_requests; // 允许的最大请求数
	pthread_t *m_threads; // 进程池数组
	std::list<T*> m_workqueue; // 请求队列
	locker m_queuelocker; // 保护请求队列的互斥锁
	sem m_queuestat; // 是否有任务需要处理
	bool m_stop; // 是否结束线程
};

#endif