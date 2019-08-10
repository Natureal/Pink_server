#ifndef PINK_CONN_TIMER_H
#define PINK_CONN_TIMER_H

#include <time.h>

// 连接的定时器，用于处理非活动连接
class conn_timer{
public:
	conn_timer() = default;
	conn_timer(int delay, bool *init_timeout){
		expire = time(NULL) + delay;
		canceled = false;
		conn_timeout = init_timeout;
	}
	void cancel(){ canceled = true; }
	void cb_func(){ *conn_timeout = true; }
	void reset(int delay){
		expire = time(NULL) + delay;
	}

public:
	time_t expire;
	bool canceled;
	bool *conn_timeout;
};

#endif