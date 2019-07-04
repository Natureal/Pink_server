#include "pink_threadpool.h"


// 构造函数
template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):
	thread_number(thread_number), max_requests(max_requests),
	thread_stop(false), threads(NULL)
{
	if((thread_number <= 0) || (max_requests <= 0))
		goto error;
	
	// 初始化线程数组 unsigned long int[thread_number]
	threads = std::make_shared<pthread_t>(new pthread_t[thread_number]);
	if(!threads)
		goto error;

	// 创建 thread_number 个线程，并将它们都设置为脱离线程
	for(int i = 0; i < thread_number; ++i){
		printf("create the %dth thread\n", i);
		// this 为 worker 函数的传入参数
		if(pthread_create(threads.get() + i, NULL, worker, this) != 0){
			delete[] threads;
			goto error;
		}
		// 将所有进程都标记为脱离模式
		if(pthread_detach(*(threads.get() + i))){
			delete[] threads;
			goto error;
		}
	}

error:
	perror("create threadpool failed");
	throw std::exception();
}

// 析构函数
template<typename T>
threadpool<T>::~threadpool(){
	delete[] threads;
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