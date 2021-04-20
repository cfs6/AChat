/*
 * Condition.h
 *
 *  Created on: 2021年1月24日
 *      Author: cfs
 */

#ifndef CONDITION_H_
#define CONDITION_H_

#pragma once
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <cstdint>
#include <mutex>
#include "MutexLock.h"
#include "noncopyable.h"

class Condition : noncopyable
{
public:
	explicit Condition(MutexLock& mutex_) : mutex(mutex_)
	{
		pthread_cond_init(&cond, NULL);
	}
	~Condition(){pthread_cond_destroy(&cond);}

	void wait()
	{
		pthread_cond_wait(&cond, mutex.get());
	}

	void notify()
	{
		pthread_cond_signal(&cond);
	}
	void notifyAll()
	{
		pthread_cond_broadcast(&cond);
	}
	bool waitForSecond(int seconds)
	{
		struct timespec abstime;
		clock_gettime(CLOCK_REALTIME, &abstime);
		abstime.tv_sec +=static_cast<time_t>(seconds);
		return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);
	}


private:
	MutexLock& mutex;
	pthread_cond_t cond;
};






#endif /* CONDITION_H_ */
