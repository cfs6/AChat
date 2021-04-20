
#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_
#include<vector>
#include<memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "base/Timestamp.h"
#include "Callbacks.h"
#include "TimerId.h"
#include "TimerQueue.h"
#include"noncopyable.h"

namespace net
{
	class Channel;
	class Poller;

	/// Reactor, at most one per thread.
	/// This is an interface class, so don't expose too much details.
	class EventLoop : public noncopyable
	{
	public:
		typedef std::function<void()> Functor;

		EventLoop();
		~EventLoop();

		/// Loops forever.
		/// Must be called in the same thread as creation of the object.
		void loop();
		/// Quits loop.
		/// This is not 100% thread safe, if you call through a raw pointer,
		/// better to call through shared_ptr<EventLoop> for 100% safety.
		void doQuit();

		void asssertInLoopThread();
		bool isInLoopThread();

		/// Time when poll returns, usually means data arrival.
		Timestamp getPollReturnTime() const { return pollReturnTime; }

		int64_t getIteration() const { return iteration; }

		/// Runs callback immediately in the loop thread.
		/// It wakes up the loop, and run the cb.
		/// If in the same loop thread, cb is run within the function.
		/// Safe to call from other threads.
		void runInLoop(const Functor& cb);
		/// Queues callback in the loop thread.
		/// Runs after finish pooling.
		/// Safe to call from other threads.
		void queueInLoop(const Functor& cb);

        // timers时间单位均是微秒
        /// Runs callback at 'time'.
        /// Safe to call from other threads.
        TimerId runAt(const Timestamp& time, const TimerCallback& cb);

        /// Runs callback after @c delay seconds.
        /// Safe to call from other threads.
        TimerId runAfter(int64_t delay, const TimerCallback& cb);

        /// Runs callback every @c interval seconds.
        /// Safe to call from other threads.
        TimerId runEvery(int64_t interval, const TimerCallback& cb);

        /// Cancels the timer.
        /// Safe to call from other threads.
        void cancel(TimerId timerId, bool off);

        void remove(TimerId timerId);

        TimerId runAt(const Timestamp& time, TimerCallback&& cb);
        TimerId runAfter(int64_t delay, TimerCallback&& cb);
        TimerId runEvery(int64_t interval, TimerCallback&& cb);

        void setFrameFunctor(const Functor& cb);

        // internal usage
		bool updateChannel(Channel* channel);
		void removeChannel(Channel* channel);
		bool hasChannel(Channel* channel);

		void assertInLoopThread()
		{
			if (!isInLoopThread())
			{
				abortNotInLoopThread();
			}
		}

		bool isInLoopThread()const
		{
			return threadId == std::this_thread::get_id();
		}

        bool isEventHandling()const
        {
        	return eventHandling;
        }

        const std::thread::id getThreadId()const
        {
        	return threadId;
        }

	private:
		bool createWakeupFd();
		bool wakeup();
		void abortNotInLoopThread();
		bool handleRead(); // waked up handler
		void doPendingFunctors();

		void printActiveChannels() const; // DEBUG
	private:
		typedef std::vector<Channel>             channelList;

		bool                                     looping;
		bool                                     quit;
		bool                                     eventHandling;
		bool                                     callingPendingFunctors;
		const std::thread::id                    threadId;
		Timestamp                                pollReturnTime;
		std::unique_ptr<Poller>                  poller;
		std::unique_ptr<TimerQueue>              timerQueue;
		int64_t                                  iteration;
//		const pid_t threadId;

		SOCKET                                   wakeupFd;

		// unlike in TimerQueue, which is an internal class,
		// we don't expose Channel to client.
		std::unique_ptr<Channel>                 wakeupChannels;

		// scratch variables
		channelList                              activeChannels;
		Channel*                                 currentActiveChannel;

		std::mutex                               mutex;
		std::vector<Functor>                     pendingFunctors;   //Guarded by mutex
		Functor                                  frameFunctor;
	};

}



#endif /* EVENTLOOP_H_ */
