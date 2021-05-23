#ifndef COUNTDOWNLATCH_H_
#define COUNTDOWNLATCH_H_
/*CountDownLatch的主要作用是确保Thread中传进去的func真的启动了以后
 外层的start才返回*/
//#include <mutex>
#include "Condition.h"
#include "MutexLock.h"
#include "noncopyable.h"

class CountDownLatch : noncopyable
{
public:
	explicit CountDownLatch(int count_);
	void wait();
	void countDown();

private:
	mutable MutexLock mutex;
	Condition condition;
	int count;
};






#endif /* COUNTDOWNLATCH_H_ */
