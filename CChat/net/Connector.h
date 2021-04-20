
#ifndef CONNECTOR_H_
#define CONNECTOR_H_
#pragma once

#include "InetAddress.h"
#include <functional>
#include <memory>

namespace net
{
	class Channel;
	class EventLoop;

	class Connector : public std::enable_shared_from_this<Connector>
	{
	public:
		typedef std::function<void(int sockfd)> NewConnectionCallback;
		Connector(EventLoop* loop_, InetAddress& serverAddr_);
		~Connector();

		void setNewConnectionCallback(const NewConnectionCallback& cb)
		{ newConnectionCallback = cb; }

		void start();  // can be called in any thread
		void restart();  // must be called in loop thread
		void stop();  // can be called in any thread

		const InetAddress& serverAddress() const { return serverAddr; }


	private:
		enum States{ kDisconnected, kConnecting, kConnected };
		static const int kMaxRetryDelayMs = 30*1000;
		static const int kInitRetryDelayMs = 500;

		void setState(States s) { state = s; }
		void startInLoop();
		void stopInLoop();
		void connect();
		void connecting(int sockfd);
		void handleWrite();
		void handleError();
		void retry(int sockfd);
		int removeAndResetChannel();
		void resetChannel();

	private:
		EventLoop*                         loop;
		InetAddress                        serverAddr;
		//atomic
		bool                               connected;
		States                             state;
		std::unique_ptr<Channel>           channel;
		NewConnectionCallback              newConnectionCallback;
		int                                retryDelayMs;

	};
}




#endif /* CONNECTOR_H_ */
