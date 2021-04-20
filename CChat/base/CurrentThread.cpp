/*
 * CurrentThread.cpp
 *
 *  Created on: 2021年1月24日
 *      Author: cfs
 */
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include "CurrentThread.h"
#include <linux/unistd.h>
#include <sys/prctl.h>
#include <stdint.h>
#include <memory>
#include <sys/syscall.h>

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
	if(cachedTid==0)
	{
		cachedTid = gettid();
		tidStringLength = snprintf(tidString, sizeof(tidString), "%5d", cachedTid);
	}
}

class ThreadData
{

};









