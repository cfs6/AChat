/*
 * Timer.cpp
 *
 *  Created on: 2021年2月13日
 *      Author: cfs
 */
#include "Timer.h"
#include <memory>

using namespace net;
std::atomic<int64_t> Timer::numCreated;

Timer::Timer(const TimerCallback& timecb, const Timestamp when, int64_t interval_, int64_t reapeatCount_ = -1):
	callback(timecb), expiration(when), interval(interval_), repeatCount(reapeatCount_),
	sequence(++numCreated), canceled(false)
{

}

Timer::Timer(const TimerCallback&& timecb, const Timestamp when, int64_t interval_):
			callback(std::move(timecb)), expiration(when), interval(interval_), repeatCount(-1),
			sequence(++numCreated), canceled(false)
{

}


void Timer::run()
{
	if(canceled)
		return;
	callback();
	if(repeatCount!=-1)
	{
		repeatCount--;
		if(repeatCount==0)
			return;
	}
	expiration += interval;
}






