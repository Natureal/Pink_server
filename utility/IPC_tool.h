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
		if(sem_init(&sem, 0, 0) != 0){
			throw std::exception();
		}
	}
	~sem(){
		sem_destroy(&sem);
	}
	bool wait(){
		return sem_wait(&sem) == 0;
	}
	bool post(){
		return sem_post(&sem) == 0;
	}

private:
	sem_t sem;
};

// 互斥锁
// 依靠头文件定义 pthread_mutex_init, pthread_mutex_destroy, pthread_mutex_lock/unlock
class mutex{
public:
	mutex(){
		if(pthread_mutex_init(&mutex, NULL) != 0){
			throw std::exception();
		}

	}
	~mutex(){
		pthread_mutex_destroy(&mutex);
	}
	bool lock(){
		return pthread_mutex_lock(&mutex) == 0;
	}
	bool unlock(){
		return pthread_mutex_unlock(&mutex) == 0;
	}

private:
	pthread_mutex_t mutex;
};


// 条件变量
class cond{
public:
	cond(){
		if(pthread_mutex_init(&mutex, NULL) != 0){
			throw std::exception();
		}
		if(pthread_cond_init(&cond, NULL) != 0){
			// 如果构造函数出问题，应该立即释放已经成功分配的资源
			pthread_mutex_destroy(&mutex);
			throw std::exception();
		}
	}
	~cond(){
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&cond);
	}
	bool wait(){
		int ret = 0;
		pthread_mutex_lock(&mutex);
		ret = pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);
		return ret == 0;
	}
	bool signal(){
		return pthread_cond_signal(&cond) == 0;
	}

private:
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};


#endif
