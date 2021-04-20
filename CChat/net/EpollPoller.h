/*
 * EpollPoller.h
 *
 *  Created on: 2021年2月15日
 *      Author: cfs
 */

#ifndef EPOLLPOLLER_H_
#define EPOLLPOLLER_H_
#pragma once
#include <vector>
#include <map>
#include "Poller.h"
#include "base/Timestamp.h"
struct epoll_event;
namespace net
{

	class EventLoop;

	class EpollPoller : public Poller
	{
	public:
		EpollPoller(EventLoop* loop);
		~EpollPoller();
		virtual Timestamp poll(int timeoutMs, channelList* activeChannels);
		virtual bool updateChannel(Channel* channel);
		virtual void removeChannel(Channel* channel);

		virtual bool hasChannel(Channel* channel) const;

		void assertInLoopThread() const;
	private:
		static const int kInitEventListSize = 16;
		void fillActiveChannels(int numEvents, channelList* activeChannels)const;
		bool update(int operation, Channel* channel);
	private:
		typedef std::vector<struct epoll_event>   eventList;
		typedef std::map<int, Channel*>           channelMap;

		int                                       epollFd;
		eventList                                 events;

		channelMap								  channels;
		EventLoop*                                ownerLoop;
	};
}




#endif /* EPOLLPOLLER_H_ */
