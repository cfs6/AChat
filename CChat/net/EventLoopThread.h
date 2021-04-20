
#ifndef EVENTLOOPTHREAD_H_
#define EVENTLOOPTHREAD_H_
#pragma once

#include<mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <functional>

namespace net
{
	class EventLoop;

	class EventLoopThread
	{
	public:
		typedef std::function<void()> ThreadInitCallback;

		EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), std::string& name = "");
		~EventLoopThread();

		EventLoop* startLoop();
		void stopLoop();

	private:
		EventLoopThread(const EventLoopThread&) = delete;
		EventLoopThread operator=(const EventLoopThread&) = delete;

		void threadFunc();

		EventLoop*                          loop;
		bool                                existing;
		std::unique_ptr<std::thread>        thread;
		std::mutex                          mutex;
		std::condition_variable             cond;
		ThreadInitCallback                  callback;

	};
}




#endif /* EVENTLOOPTHREAD_H_ */
