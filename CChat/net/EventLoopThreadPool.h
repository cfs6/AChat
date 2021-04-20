
#ifndef EVENTLOOPTHREADPOOL_H_
#define EVENTLOOPTHREADPOOL_H_

#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace net
{
	class EventLoop;
	class EventLoopThread;

	class EventLoopThreadPool
	{
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;
		EventLoopThreadPool();
		~EventLoopThreadPool();

		void init(EventLoop* baseLoop, int numThread);
		void start(const ThreadInitCallback& cb);

		void stop();

		// valid after calling start()
		// round-robin
		EventLoop* getNextLoop();

		// with the same hash code, it will always return the same EventLoop
		EventLoop* getLoopForHash(size_t hashCode);

		std::vector<EventLoop*> getAllLoops();

		bool isStarted()const { return started; }

		const std::string getName(){ return name; }

		const std::string info()const;

	private:
		EventLoop*                                          baseLoop;
		std::vector<std::unique_ptr<EventLoopThread>>       threads;
		std::vector<EventLoop*>                             loops;
		std::string                                         name;
		bool                                                started;
		int                                                 numThreads;
		int                                                 next;
	};
}







#endif /* EVENTLOOPTHREADPOOL_H_ */
