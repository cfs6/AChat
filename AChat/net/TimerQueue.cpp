#include "TimerQueue.h"

#include<functional>
#include"EventLoop.h"
#include"Timer.h"
#include"TimerId.h"

using namespace net;

TimerQueue::TimerQueue(EventLoop* loop_):loop(loop_), timers()
{
}

TimerQueue::~TimerQueue()
{
	for(TimerSet::iterator it = timers.begin(); it != timers.end(); ++it)
	{
		delete it->second;
	}
}

TimerId TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, int64_t interval/*, int64_t repeatCount*/)
{
	Timer* timer = new Timer(cb, when, interval);
	loop->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->getSequence());
}

TimerId TimerQueue::addTimer(TimerCallback&& cb, Timestamp when, int64_t interval)
{
	Timer* timer = new Timer(std::move(cb), when, interval);
	loop->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->getSequence());
}

void TimerQueue::removeTimer(TimerId timerId)
{
	loop->runInLoop(std::bind(&TimerQueue::removeTimerInLoop, this, timerId));
}

void TimerQueue::cancel(TimerId timerId, bool off)
{
	loop->runInLoop(std::bind(&TimerQueue::cancelTimerInLoop, this, timerId));
}

void TimerQueue::doTimer()
{
	loop->assertInLoopThread();
	Timestamp now(Timestamp::now());
	for(auto iter = timers.begin(); iter!=timers.end();)
	{
		if(iter->second->getExpiration() <= now)
		{
			iter->second->run();
			if(iter->second->getRepeatCount()==0)
			{
				iter = timers.erase(iter);
			}
			else
			{
				++iter;
			}
		}
		else
		{
			break;
		}
	}


}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	loop->assertInLoopThread();
	insert(timer);
}

void TimerQueue::removeTimerInLoop(TimerId timerId)
{
	loop->assertInLoopThread();
	Timer* timer = timerId.timer;
	for(auto iter = timers.begin(); iter != timers.end(); ++iter)
	{
		if(iter == timer)
		{
			timers.erase(iter);
			break;
		}
	}
}

void TimerQueue::cancelTimerInLoop(TimerId timerId, bool off)
{
	loop->assertInLoopThread();
	Timer* timer = timerId.timer;
	for(auto iter = timers.begin(); iter != timers.end(); ++iter)
	{
		if(iter == timer)
		{
			iter->second->cancel(off);
			break;
		}
	}
}

void TimerQueue::insert(Timer* timer)
{
	loop->assertInLoopThread();
	Timestamp when = timer->getExpiration();
	timers.insert(Entry(when, timer));
}








