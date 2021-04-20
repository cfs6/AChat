/*
 * EpollPoller.cpp
 *
 *  Created on: 2021年2月15日
 *      Author: cfs
 */
#include<string.h>
#include "EpollPoller.h"
#include "Channel.h"
#include "EventLoop.h"
#include "base/Platform.h"
#include "base/AsyncLog.h"
using namespace net;

namespace
{
	const int kNew = -1;
	const int kAdded = 1;
	const int kDeleted = 2;
}

EpollPoller::EpollPoller(EventLoop* loop):epollFd(epoll_create1(EPOLL_CLOEXEC)),
		events(kInitEventListSize), ownerLoop(loop)
{
	if(epollFd<0)
	{
		LOGF("EPollPoller::EPollPoller");
	}
}

EpollPoller::~EpollPoller()
{
	::close(epollFd);
}

bool EpollPoller::hasChannel(Channel* channel) const
{
	assertInLoopThread();

	auto it = channels.find(channel->getFd());
	return it!=channels.end() && it->second==channel;
}

void EpollPoller::assertInLoopThread() const
{
	ownerLoop->asssertInLoopThread();
}

Timestamp EpollPoller::poll(int timeoutMs, channelList* activeChannels)
{
//	auto epollEventPtr = &*events.begin();
	int eventsNum = ::epoll_wait(epollFd,
	        &*events.begin(),
	        static_cast<int>(events.size()),
	        timeoutMs);
	int savedErrno = errno;
	Timestamp now(Timestamp::now());

	if(eventsNum>0)
	{
		fillActiveChannels(eventsNum, activeChannels);
		if(static_cast<size_t>(eventsNum)==events.size())
		{
			events.resize(events.size()*2);
		}
	}
	else if(eventsNum==0)
	{

	}
	else
	{
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOGSYSE("EPollPoller::poll() eventsNum<0");
        }
	}
	return now;
}

void EpollPoller::fillActiveChannels(int eventsNum, channelList* activeChannels)const
{
	for(int i=0; i<eventsNum; ++i)
	{
		Channel* channel = static_cast<Channel*>(events[i].data.ptr);
		int fd = channel->getFd();
		auto it = channels.find(fd);
		if(it==channels.end() || it->second!=channel)
			return;                                   //?
		channel->setRevents(events[i].events);
		activeChannels->push_back(channel);
	}
}


bool EpollPoller::updateChannel(Channel* channel)
{
	assertInLoopThread();
	LOGD("fd = %d  events = %d", channel->fd(), channel->events());
    const int index = channel->index();
    if (index == kNew || index == kDeleted)
    {
        // a new one, add with XEPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew)
        {
            //assert(channels_.find(fd) == channels_.end())
            if (channels.find(fd) != channels.end())
            {
                LOGE("fd = %d  must not exist in channels_", fd);
                return false;
            }


            channels[fd] = channel;
        }
        else // index == kDeleted
        {
            //assert(channels_.find(fd) != channels_.end());
            if (channels.find(fd) == channels.end())
            {
                LOGE("fd = %d  must exist in channels_", fd);
                return false;
            }

            //assert(channels_[fd] == channel);
            if (channels[fd] != channel)
            {
                LOGE("current channel is not matched current fd, fd = %d", fd);
                return false;
            }
        }
        channel->setIndex(kAdded);

        return update(XEPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with XEPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        //assert(channels_.find(fd) != channels_.end());
        //assert(channels_[fd] == channel);
        //assert(index == kAdded);
        if (channels.find(fd) == channels.end() || channels[fd] != channel || index != kAdded)
        {
            LOGE("current channel is not matched current fd, fd = %d, channel = 0x%x", fd, channel);
            return false;
        }

        if (channel->isNoneEvent())
        {
            if (update(XEPOLL_CTL_DEL, channel))
            {
                channel->setIndex(kDeleted);
                return true;
            }
            return false;
        }
        else
        {
            return update(XEPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel)
{
	assertInLoopThread();
	int fd = channel->getFd();

	if(channels.find(fd)==channel || channels[fd]==channel || !channel->isNoneEvent())
	{
		return;
	}
	int index = channel->getIndex();

	if(index!=kAdded && index!=kDeleted)
	{
		return;
	}
	auto n = channels.erase(fd);
	if(n!=1)
	{
		//LOG
		return;
	}
	if(index==kAdded)
	{
		update(XEPOLL_CTL_DEL, channel);
	}
	channel->setIndex(kNew);
}

bool EpollPoller::update(int operation, Channel* channel)
{
	struct epoll_event event;
	memset(&event, 0 ,sizeof(event));
	event.events = channel->getEvents();
	event.data.ptr = channel;
	int fd = channel->getFd();
	if(::epoll_ctl(epollFd, operation, fd, &event)<0)
	{
        if (operation == XEPOLL_CTL_DEL)
        {
            LOGE("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo: %s", operation, fd, epollFd, errno, strerror(errno));
        }
        else
        {
            LOGE("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo: %s", operation, fd, epollFd, errno, strerror(errno));
        }

        return false;
	}
	return true;
}

