/*
 * Thread.h
 *
 *  Created on: 2021年1月24日
 *      Author: cfs
 */

#ifndef THREAD_H_
#define THREAD_H_

#pragma once
#include <pthread.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include "noncopyable.h"
#include "CountDownLatch.h"

class Thread : public noncopyable
{
public:
	typedef std::function<void()> ThreadFunc;
	explicit Thread(const ThreadFunc, const std::string& name = std::string());
	~Thread();

	void start();
	int join();
	bool isStarted()const{ return started; }
	pid_t getTid()const{ return tid; }
	const std::string& getName(){ return name; }
private:
	void setDefaultName();

	bool                joined;
	bool                started;
	pthread_t           pthreadId;
	pid_t               tid;
	ThreadFunc          tfunc;
	std::string         name;
	CountDownLatch      latch;
	std::atomic<int>    numCreated;

};






#endif /* THREAD_H_ */
