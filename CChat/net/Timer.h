/*
 * Timer.h
 *
 *  Created on: 2021年2月13日
 *      Author: cfs
 */

#ifndef TIMER_H_
#define TIMER_H_
#pragma once

#include<atomic>
#include<stdint.h>
#include"base/Timestamp.h"
namespace net
{
	class Timer
	{
	public:
		Timer(const TimerCallback& timecb, const Timestamp when, int64_t interval, int64_t reapeatCount = -1);

		//右值TimerCallback, repeatCount=-1
		Timer(const TimerCallback&& timecb, const Timestamp when, int64_t interval);

		void run();

		bool isCanceled()
		{
			return canceled;
		}
		void cancel(bool off)
		{
			canceled = off;
		}

		Timestamp getExpiration()
		{
			return expiration;
		}

		Timestamp getRepeatCount()
		{
			return repeatCount;
		}

		int64_t getSequence()
		{
			return sequence;
		}

		static int64_t getNumCreated()
		{
			return numCreated;
		}

		int64_t getInterval()
		{
			return interval;
		}

	private:
		Timer(const Timer&) = delete;
		Timer& operator=(const Timer&) = delete;

	private:
		const TimerCallback            callback;
		Timestamp                      expiration;
		const int64_t                  interval;
		int64_t                        repeatCount;             //重复次数，-1表示一直重复
		const int64_t                  sequence;
		bool                           canceled;
		static std::atomic<int64_t>    numCreated;

	};
}



#endif /* TIMER_H_ */
