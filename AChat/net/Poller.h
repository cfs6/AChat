/*
 * Poller.h
 *
 *  Created on: 2021年2月15日
 *      Author: cfs
 */

#ifndef POLLER_H_
#define POLLER_H_
#include <vector>
#include "base/Timestamp.h"

namespace net
{
	class Channel;
	class Poller
	{
	public:
		Poller();
		virtual ~Poller();

	public:
		typedef std::vector<Channel*>        channelList;

		virtual Timestamp poll(channelList activeChannels, int timeout);
		virtual bool updateChannel(Channel* channel) = 0;
		virtual void removeChannel(Channel* channel) = 0;
		virtual bool hasChannel(Channel* channel)const = 0;
	};
}




#endif /* POLLER_H_ */
