#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count_):mutex(),condition(mutex),count(count_){}

void CountDownLatch::wait()
{
	MutexLockGuard lock(mutex);
	while(count>0)
	{
		condition.wait();
	}
}

void CountDownLatch::countDown()
{
	MutexLockGuard lock(mutex);
	--count;
	if(count==0)
	{
		condition.notifyAll();
	}
}
