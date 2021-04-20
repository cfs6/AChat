#include "EventLoopThread.h"
#include <functional>
#include "EventLoop.h"

using namespace net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
								std::string& name = "")
								:loop(NULL), existing(false), callback(cb)
{

}

EventLoopThread::~EventLoopThread()
{
	existing = true;
	if(loop != nullptr)// not 100% race-free, eg. threadFunc could be running callback_.
	{
		// still a tiny chance to call destructed object, if threadFunc exits just now.
		// but when EventLoopThread destructs, usually programming is exiting anyway.
		loop->doQuit();
		thread->join();
	}
}

EventLoop* EventLoopThread::startLoop()
{
	thread.reset(new std::thread(std::bind(&EventLoopThread::threadFunc, this)));

	{
		std::unique_lock<std::mutex> lock(mutex);
		while(loop==nullptr)
		{
			cond.wait(lock);
		}
	}
}

void EventLoopThread::stopLoop()
{
	if(loop != nullptr)
	{
		loop->doQuit();
	}
	thread->join();
}

void EventLoopThread::threadFunc()
{
	EventLoop newLoop;

	if(callback)
	{
		callback();
	}

	{
		std::unique_lock<std::mutex> lock(mutex);
		newLoop = loop;
		cond.notify_all();
	}

	newLoop.loop();
	loop = nullptr;
}











