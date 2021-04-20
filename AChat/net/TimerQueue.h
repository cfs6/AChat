/*
 * TimerQueue.h
 *
 *  Created on: 2021年2月13日
 *      Author: cfs
 */

#ifndef TIMERQUEUE_H_
#define TIMERQUEUE_H_

#include<set>
#include<vector>
#include "base/Timestamp.h"
#include "Channel.h"
#include "Callbacks.h"

namespace net
{
	class EventLoop;
	class Timer;
	class TimerId;
	class TimerQueue
	{
	public:
		TimerQueue(EventLoop* loop);
		~TimerQueue();

		// Schedules the callback to be run at given time,
		// repeats if @c interval > 0.0.
		// Must be thread safe. Usually be called from other threads.
		TimerId addTimer(const TimerCallback& cb, Timestamp when, int64_t interval/*, int64_t repeatCount*/);

		TimerId addTimer(TimerCallback&& cb, Timestamp when, int64_t interval);

		void removeTimer(TimerId timerId);

		void cancel(TimerId timerId, bool off);

		// called when timerfd alarms
		void doTimer();
	private:
		TimerQueue(const TimerQueue& rhs) = delete;
		TimerQueue& operator=(const TimerQueue& rhs) = delete;

		typedef std::pair<Timestamp, Timer*>      Entry;
		typedef std::set<Entry>                   TimerSet;
		typedef std::pair<Timer*, int64_t>        ActiveTimer;
		typedef std::set<ActiveTimer>             ActiveTimerSet;

		void addTimerInLoop(Timer* timer);
		void removeTimerInLoop(TimerId timerId);
		void cancelTimerInLoop(TimerId timerId, bool off);


		void insert(Timer* timer);

	private:
		EventLoop*                   loop;
		TimerSet                     timers;
	};
}



#endif /* TIMERQUEUE_H_ */
