#ifndef CURRENTTHREAD_H_
#define CURRENTTHREAD_H_
#pragma once
#include<stdint.h>

namespace CurrentThread
{
	extern __thread int cachedTid;
	extern __thread char tidString[32];
	extern __thread int tidStringLength;
	extern __thread const char* threadName;

	void cacheTid();
	inline int tid()
	{
		if(__builtin_expect(cachedTid==0,0))
		{
			cacheTid();
		}
		return cachedTid;
	}

	inline const char* getTidString()
	{
		return tidString;
	}

	inline int getTidStringLength()
	{
		return tidStringLength;
	}

	inline const char* name()
	{
		return threadName;
	}
}

#endif /* CURRENTTHREAD_H_ */
