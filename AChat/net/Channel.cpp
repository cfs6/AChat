/*
 * Channel.cpp
 *
 *  Created on: 2021年2月10日
 *      Author: cfs
 */

#include "Channel.h"
//#include "Poller.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#include "base/AsyncLog.h"
using namespace net;
static const int Channel::kNoneEvent = 0;
static const int Channel::kReadEvent = 0;
static const int Channel::kWriteEvent = 0;

Channel::Channel(EventLoop* loop_, int fd_):loop(loop_), fd(fd_), events(0),
										    revents(0), index(-1),
										    logHup(true), tied(false)
{
}

Channel::~Channel()
{

}
void Channel::setReadCallback(const ReadEventCallback& cb)
{
	readCallback = cb;
}
void Channel::setWriteCallback(const EventCallback& cb)
{
	writeCallback = cb;
}
void Channel::setCloseCallback(const EventCallback& cb)
{
	closeCallback = cb;
}
void Channel::setErrorCallback(const EventCallback& cb)
{
	errorCallback = cb;
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
	tie_ = obj;
	tied = true;
}

bool Channel::enableReading()
{
	events |= kReadEvent;
	return update();
}

bool Channel::disableReading()
{
	events &= ~kReadEvent;
	return update();
}

bool Channel::enableWriting()
{
	events |= kWriteEvent;
	return update();
}

bool Channel::disableWriting()
{
	events &= ~kWriteEvent;
	return update();
}
bool Channel::disableAll()
{
	events = kNoneEvent;
	return update();
}
bool Channel::update()
{
	return loop->updateChannel(this);
}

void Channel::remove()
{
	if(!isNoneEvent())
		return;
	loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
	std::shared_ptr<void> guard;
	if(tied)
	{
		guard = tie_.lock();
		if(guard)
		{
			handleEventWithGuard(receiveTime);
		}
	}
	else
	{
		handleEventWithGuard(receiveTime);
	}
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	LOGD(reventsToString().c_str());
	if((revents & EPOLLHUP) && !(revents & EPOLLIN))
	{
		if(logHup)
		{
			LOGW("Channel::handle_event() XPOLLHUP");
		}
		if(closeCallback)
			closeCallback();
	}

	/*if(revents & EPOLLNVAL)
	{
		LOGW("Channel::handle_event() XPOLLNVAL");
	}*/

	if(revents &  EPOLLERR)
	{
		if(errorCallback)
			errorCallback();
	}
	if(revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
	{
		if(readCallback)
			readCallback(receiveTime);
	}
	if(revents & EPOLLOUT)
	{
		if(writeCallback)
			writeCallback();
	}

}

string Channel::reventsToString()const
{
	std::ostringstream oss;
	oss<<fd<<": ";
	if(revents & EPOLLIN)
	{
		oss<<"IN ";
	}
	if(revents & EPOLLPRI)
	{
		oss<<"PRI ";
	}
	if(revents & EPOLLOUT)
	{
		oss<<"OUT ";
	}
	if(revents & EPOLLHUP)
	{
		oss<<"HUP ";
	}
	if(revents & EPOLLRDHUP)
	{
		oss<<"RDHUP ";
	}
	if(revents & EPOLLERR)
	{
		oss<<"ERR ";
	}
	/*if(revents & EPOLLNVAL)
	{

	}*/

}












