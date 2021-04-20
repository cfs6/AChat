/*
 * MutexLock.h
 *
 *  Created on: 2021年1月24日
 *      Author: cfs
 */

#ifndef MUTEXLOCK_H_
#define MUTEXLOCK_H_
#pragma once
#include<pthread.h>
#include <cstdint>
#include "noncopyable.h"

class MutexLock : noncopyable
{
public:
	MutexLock()
	{
		pthread_mutex_init(&mutex, nullptr);
	}
	~MutexLock()
	{
		pthread_mutex_lock(&mutex);
		pthread_mutex_destroy(&mutex);
	}
	void lock()
	{
		pthread_mutex_lock(&mutex);
	}
	void unlock()
	{
		pthread_mutex_unlock(&mutex);
	}
	pthread_mutex_t *get() {return &mutex;}
private:
	pthread_mutex_t mutex;

private:
	friend class Condition;

};

class MutexLockGuard :noncopyable
{
public:
	explicit MutexLockGuard(MutexLock& mutex_):mutex(mutex_)
	{
		mutex.lock();
	}
	~MutexLockGuard(){mutex.unlock();}

private:
	MutexLock &mutex;
};




#endif /* MUTEXLOCK_H_ */
