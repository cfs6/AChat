/*
 * Thread.cpp
 *
 *  Created on: 2021年1月24日
 *      Author: cfs
 */
#include "Thread.h"
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>

#include <linux/unistd.h>
#include <sys/prctl.h>
#include <stdint.h>
#include <memory>
#include <sys/syscall.h>
#include <stdio.h>
#include "CurrentThread.h"
#include "AsyncLog.h"
#include <assert.h>
using namespace std;
namespace CurrentThread
{
__thread int cachedTid = 0;
__thread char tidString[32];
__thread int tidStringLength = 6;
__thread const char* threadName = "default";
}

pid_t gettid()
{
	return static_cast<pid_t>(::syscall(SYS_gettid));
}

void CurrentThread::cacheTid()
{
	if(cachedTid == 0)
	{
		cachedTid = gettid();
		tidStringLength =
				snprintf(tidString, sizeof(tidString), "%5d", cachedTid);
	}
}

struct ThreadData
{
	typedef Thread::ThreadFunc threadFunc;
	threadFunc func;
	string name;
	pid_t* tid;
	CountDownLatch* latch;

	ThreadData(const threadFunc& func_, const string& name_, pid_t tid_, CountDownLatch* latch_)
			:func(func_),name(name_),tid(tid_),latch(latch_){}
	void runInThread()
	{
		*tid = CurrentThread::tid();
		tid = nullptr;
		latch->countDown();
		latch = NULL;

		CurrentThread::threadName = name.empty()? "Thread" : name.c_str();
		prctl(PR_SET_NAME, CurrentThread::threadName);

		func();
		CurrentThread::threadName = "finished";
	}
};


void* startThread(void* obj)
{
	ThreadData* data = static_cast<ThreadData*>(obj);
	data->runInThread();
	delete data;
	return NULL;
}

Thread::Thread(ThreadFunc func, const string& n)
  : started(false),
    joined(false),
    pthreadId(0),
    tid(0),
    tfunc(std::move(func)),
    name(n),
    latch(1)
{
  setDefaultName();
}

Thread::~Thread()
{
	if (started && !joined)
	{
		pthread_detach (pthreadId_);
	}
}

void Thread::setDefaultName()
{
  int num = numCreated.incrementAndGet();
  if (name.empty())
  {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread%d", num);
    name = buf;
  }
}

void Thread::start()
{
//  assert(!started_);
  if(started)
  {
	  LOGE("Thread already started!");
  }
  started = true;
  // FIXME: move(func_)
  detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_, &latch_);
  if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
  {
    started = false;
    delete data; // or no delete?
    LOGSYSE("Failed in pthread_create");
  }
  else
  {
    latch.wait();
    assert(tid > 0);
  }
}

int Thread::join()
{
  assert(started);
  assert(!joined);
  joined = true;
  return pthread_join(pthreadId, NULL);
}


