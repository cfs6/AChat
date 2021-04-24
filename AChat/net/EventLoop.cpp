#include"EventLoop.h"
#include<sstream>
#include<string.h>
#include<iostream>
#include "base/AsyncLog.h"
#include "Channel.h"
#include "Sockets.h"
#include "InetAddress.h"
using namespace net;

thread_local EventLoop* loopInThisThread = 0;
const int kPollTimeMs = 1;

EventLoop::EventLoop():looping(false), quit(false), eventHandling(false),callingPendingFunctors(false),
		threadId(std::this_thread::get_id()), timerQueue(new TimerQueue(this)), iteration(0L),
		currentActiveChannel(NULL)
{
	createWakeupFd();
	wakeupChannels.reset(new Channel(this, wakeupFd));
	poller.reset(new Poller(this));

	if(loopInThisThread)
	{
		LOGF("Another EventLoop  exists in this thread ");
	}
	else
	{
		loopInThisThread = this;
	}
	wakeupChannels->setReadCallback(std::bind(&EventLoop::handleRead, this));

	wakeupChannels.get()->enableReading();
}

EventLoop::~EventLoop()
{
	assertInLoopThread();
	LOGD("EventLoop 0x%x destructs.", this);

	wakeupChannels->disableAll();
	wakeupChannels->remove();

	sockets::close(wakeupFd);

}

bool EventLoop::createWakeupFd()
{
	wakeupFd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
	if(wakeupFd < 0)
	{
		LOGF("Unable to create wakeup eventfd, EventLoop: 0x%x", this);
		return false;
	}
	return true;
}



void EventLoop::loop()
{
	assertInLoopThread();
	looping = true;
	quit = false; // FIXME: what if someone calls quit() before loop() ?
	LOGD("EventLoop 0x%x start looping", this);

	while(!quit)
	{
		timerQueue->doTimer();

		activeChannels.clear();
		pollReturnTime = poller->poll(activeChannels ,kPollTimeMs);
		printActiveChannels();

		++iteration;
		//todo sort channel by priority

		eventHandling = true;
		for(const auto& it : activeChannels)
		{
			currentActiveChannel = it;
			currentActiveChannel->handleEvent(pollReturnTime);
		}

		currentActiveChannel = nullptr;
		eventHandling = false;
		doPendingFunctors();

		if(frameFunctor)
		{
			frameFunctor();
		}
	}

	LOGD("EventLoop 0x%x stop looping", this);
	looping = false;

	std::ostringstream oss;
	oss<<std::this_thread::get_id();
	std::string stid = oss.str();
	LOGI("Exiting loop, EventLoop object: 0x%x , threadID: %s", this, stid.c_str());

}

void EventLoop::quit()
{
	quit = true;
	if(!isInLoopThread())
	{
		wakeup();
	}
}

void EventLoop::runInLoop(const Functor& cb)
{
	if(isInLoopThread())
	{
		cb();
	}
	else
	{
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(const Functor& cb)
{
	{
		std::unique_lock<std::mutex> lock(mutex);
		pendingFunctors.push_back(cb);
	}
	if(!isInLoopThread() || callingPendingFunctors)
	{
		wakeup();
	}
}

void EventLoop::setFrameFunctor(const Functor& cb)
{
	frameFunctor = cb;
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
	return timerQueue->addTimer(cb, time, 0, 1);
}

TimerId EventLoop::runAfter(int64_t delay, const TimerCallback& cb)
{
	Timestamp time(addTime(Timestamp::now(), delay));
	return runAt(time, cb);
}

TimerId EventLoop::runEvery(int64_t interval, const TimerCallback& cb)
{
	Timestamp time(addTime(Timestamp::now(), interval));
	return timerQueue->addTimer(cb, time, interval, -1);
}

TimerId EventLoop::runAt(const Timestamp& time, TimerCallback&& cb)
{
    return timerQueue->addTimer(std::move(cb), time, 0, 1);
}

TimerId EventLoop::runAfter(int64_t delay, TimerCallback&& cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(int64_t interval, TimerCallback&& cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue->addTimer(std::move(cb), time, interval, -1);
}

void EventLoop::cancel(TimerId timerId, bool off)
{
	return timerQueue->cancel(timerId, off);
}

void EventLoop::remove(TimerId timerId)
{
	return timerQueue->removeTimer(timerId);
}

bool EventLoop::updateChannel(Channel* channel)
{
	if(channel->ownerLoop() != this)
	{
		LOGE("EventLoop::updateChannel: ownerLoop() != this");
		return false;
	}
	assertInLoopThread();
	return poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
	if(channel->ownerLoop() != this)
	{
		LOGE("EventLoop::removeChannel: ownerLoop() != this");
		return;
	}
	assertInLoopThread();
	if(eventHandling)
	{
		if(currentActiveChannel == channel ||
				std::find(activeChannels.begin(), activeChannels.end(), channel)
		== activeChannels.end())
		{

		}
		else
		{
			LOGE("EventLoop::removeChannel currentActiveChannel != channel && "
					"std::find(activeChannels.begin(), activeChannels.end(), channel) != activeChannels.end()");
		}
	}
	LOGD("Remove channel, channel = 0x%x, fd = %d", channel, channel->fd());
	poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
	if(channel->ownerLoop() != this)
	{
		LOGE("EventLoop::hasChannel: ownerLoop() != this");
	}
    assertInLoopThread();
    return poller->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
	std::stringstream ss;
	ss<<"threadId = "<<threadId<<" this_thread::get_id() = "<<std::this_thread::get_id();
	LOGF("EventLoop::abortNotInLoopThread - EventLoop %s", ss.str().c_str());
}

bool EventLoop::wakeup()
{
	uint64_t one = 1;
	int32_t ret = sockets::write(wakeupFd, &one, sizeof(one));

	if(ret != sizeof(one))
	{
		int error = errno;
		LOGSYSE("EventLoop::wakeup() writes %d  bytes instead of 8, fd: %d, error: %d, errorinfo: %s", ret, wakeupFd, error, strerror(error));
		return false;
	}
	return true;
}

bool EventLoop::handleRead()
{
	uint64_t one = 1;
	int32_t ret = sockets::read(wakeupFd, &one, sizeof(one));
	if(ret != sizeof(one))
	{
        int error = errno;
        LOGSYSE("EventLoop::wakeup() read %d  bytes instead of 8, fd: %d, error: %d, errorinfo: %s", ret, wakeupFd, error, strerror(error));
        return false;
	}
	return true;
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors = true;

	{
		std::unique_lock<std::mutex> lock(mutex);
		functors.swap(pendingFunctors);
	}

	for(size_t i=0; i<functors.size(); ++i)
	{
		functors[i]();
	}
	callingPendingFunctors = false;
}

void EventLoop::printActiveChannels() const
{
	for(const Channel* channel : activeChannels)
	{
		LOGD("{%s}", channel->reventsToString().c_str());
	}
}










