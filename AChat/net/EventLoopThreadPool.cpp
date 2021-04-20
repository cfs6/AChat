#include "EventLoopThreadPool.h"
#include <stdio.h>
#include <assert.h>
#include <sstream>
#include <string>
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Callbacks.h"
#include "base/AsyncLog.h"

using namespace net;

EventLoopThreadPool::EventLoopThreadPool():
		             baseLoop(NULL),
		             started(false),
		             numThreads(0),
		             next(0)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{
	// Don't delete loop, it's stack variable
}

void EventLoopThreadPool::init(EventLoop* baseLoop_, int numThreads_)
{
	baseLoop = baseLoop_;
	numThreads = numThreads_;
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	if(baseLoop == nullptr)
	{
		LOGE("baseLoop == nullptr !");
		return;
	}
	if(isStarted())
	{
		LOGE("isStarted()");
		return;
	}

	baseLoop->assertInLoopThread();
	started = true;

	for(int i = 0; i < numThreads; ++i)
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "%s%d", name.c_str(), i);

		std::unique_ptr<EventLoopThread> t(new EventLoopThread(cb, buf));

		loops.push_back(t->startLoop());
		threads.push_back(std::move(t));
	}

	if(numThreads == 0 && cb)
	{
		cb(baseLoop);
	}
}

void EventLoopThreadPool::stop()
{
	for(auto& iter : threads)
	{
		iter->stopLoop();
	}
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
	baseLoop->assertInLoopThread();

	if(!isStarted())
	{
		LOGE("!isStarted()");
		return nullptr;
	}
	EventLoop* loop = baseLoop;

	if(!loops.empty())
	{
		loop = loops[next];
		next++;
		if(next >= loops.size())
		{
			next = 0;
		}
	}
	return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
    baseLoop->assertInLoopThread();
    EventLoop* loop = baseLoop;

    if (!loops.empty())
    {
        loop = loops[hashCode % loops.size()];
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    baseLoop->assertInLoopThread();
    if (loops.empty())
    {
        return std::vector<EventLoop*>(1, baseLoop);
    }
    else
    {
        return loops;
    }
}

const std::string EventLoopThreadPool::info() const
{
    std::stringstream ss;
    ss << "print threads id info " << endl;
    for (size_t i = 0; i < loops_.size(); i++)
    {
        ss << i << ": id = " << loops_[i]->getThreadID() << endl;
    }
    return ss.str();
}



