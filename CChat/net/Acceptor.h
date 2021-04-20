
#ifndef ACCEPTOR_H_
#define ACCEPTOR_H_
#pragma once
#include<functional>

#include"Channel.h"
#include"Sockets.h"

namespace net
{
	class EventLoop;
	class InetAddress;

	//Acceptor of inconing TCP connections
	class Acceptor
	{
	public:
		typedef function<void(int sockfd, const InetAddress)>         NewConnectionCallBack;

		Acceptor(EventLoop* loop_, const InetAddress& listenAddr, bool reuseport);
		~Acceptor();

		void setNewConnectionCallBack(NewConnectionCallBack newConnectionCallBack_)
		{
			NewConnectionCallBack = newConnectionCallBack_;
		}

		bool isListenning()const{ return listenning; }
		void listen();

	private:
		void handleRead();

	private:
		EventLoop*                     loop;
		Socket                         acceptSocket;
		Channel                        acceptChannel;
		NewConnectionCallBack          newConnectionCallBack;
		bool                           listenning;

		int                            idleFd;
	};
}


#endif /* ACCEPTOR_H_ */
