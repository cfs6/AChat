/*
 * Channel.h
 *
 *  Created on: 2021年1月24日
 *      Author: cfs
 */

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include<sys/epoll.h>
#include<functional>
#include "Types.h"
#include "base/Timestamp.h"

namespace net
{
	class EventLoop;

	class Channel
	{
	public:
		typedef std::function<void()> EventCallback;
		typedef std::function<void(Timestamp)> ReadEventCallback;

		Channel(EventLoop* loop, int fd);
		~Channel();

		void handleEvent(Timestamp receiveTime);
		void setReadCallback(const ReadEventCallback& cb);
		void setWriteCallback(const EventCallback& cb);
		void setCloseCallback(const EventCallback& cb);
		void setErrorCallback(const EventCallback& cb);

		/// Tie this channel to the owner object managed by shared_ptr,
		/// prevent the owner object being destroyed in handleEvent.
		void tie(const std::shared_ptr<void>&);

		int getFd()const{return fd;}
		int getEvents()const{return events;}
		void setRevents(int revt){ revents = revt;}
		void addRevents(int revt){ revents |= revt;}

		bool isNoneEvent()const { return events == kNoneEvent; }

		bool enableReading();
		bool disableReading();
		bool enableWriting();
		bool disableWriting();
		bool disableAll();

		bool isWriting()const {return events == kWriteEvent; }
		//for poller
		int getIndex()const {return index; }
		void setIndex(int idx){index = idx;}

		//for debug()
		string reventsToString()const;

		void doNotLogHup(){ logHup = false;}

		EventLoop* ownerLoop(){ return loop; }
		void remove();

	private:
		bool update();
		void handleEventWithGuard(Timestamp receiveTime);

		static const int       kNoneEvent;
		static const int       kReadEvent;
		static const int       kWriteEvent;

		EventLoop*             loop;
		const int              fd;
		U32 	               events;
		U32                    revents;
	//    U32                    lastEvents;
		int                    index;
		bool				   logHup;

		std::weak_ptr<void>    tie_;
		bool                   tied;

		ReadEventCallback      readCallback;
		EventCallback          writeCallback;
		EventCallback          closeCallback;
		EventCallback          errorCallback;
	};
}




#endif /* CHANNEL_H_ */
