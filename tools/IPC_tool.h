#ifndef IPC_TOOL_H
#define IPC_TOOL_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 信号量
// 依靠头文件定义 sem_init, sem_destroy, sem_wait, sem_post 函数
class sem{
public:
	sem(){
		if(sem_init(&s, 0, 0) != 0){
			throw std::exception();
		}
	}
	~sem(){
		sem_destroy(&s);
	}
	bool wait(){
		return sem_wait(&s) == 0;
	}
	bool post(){
		return sem_post(&s) == 0;
	}

private:
	sem_t s;
};

// 互斥锁
// 依靠头文件定义 pthread_mutex_init, pthread_mutex_destroy, pthread_mutex_lock/unlock
class mutex{
public:
	mutex(){
		if(pthread_mutex_init(&m, NULL) != 0){
			throw std::exception();
		}

	}
	~mutex(){
		pthread_mutex_destroy(&m);
	}
	bool lock(){
		return pthread_mutex_lock(&m) == 0;
	}
	bool unlock(){
		return pthread_mutex_unlock(&m) == 0;
	}

private:
	pthread_mutex_t m;
};


// 条件变量
class cond{
public:
	cond(){
		if(pthread_mutex_init(&m, NULL) != 0){
			throw std::exception();
		}
		if(pthread_cond_init(&c, NULL) != 0){
			// 如果构造函数出问题，应该立即释放已经成功分配的资源
			pthread_mutex_destroy(&m);
			throw std::exception();
		}
	}
	~cond(){
		pthread_mutex_destroy(&m);
		pthread_cond_destroy(&c);
	}
	bool wait(){
		int ret = 0;
		pthread_mutex_lock(&m);
		ret = pthread_cond_wait(&c, &m);
		pthread_mutex_unlock(&m);
		return ret == 0;
	}
	bool signal(){
		return pthread_cond_signal(&c) == 0;
	}

private:
	pthread_mutex_t m;
	pthread_cond_t c;
};


#endif
