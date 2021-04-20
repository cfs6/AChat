
#ifndef TCPCONNECTION_H_
#define TCPCONNECTION_H_
#pragma once

#include <memory>

#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;
namespace net
{
	class EventLoop;
	class Socket;
	class Channel;

	/// TCP connection, for both client and server usage.
	/// This is an interface class, so don't expose too much details.
	class TcpConnection
	{
	public:
		TcpConnection(EventLoop* loop,
			            const string& name,
			            int sockfd,
			            const InetAddress& localAddr,
			            const InetAddress& peerAddr);
		~TcpConnection();

		EventLoop* getLoop() const { return loop; }
		const string& getName() const { return name; }
		const InetAddress& localAddress() const { return localAddr; }
		const InetAddress& peerAddress() const { return peerAddr; }
		bool connected() const { return state == kConnected; }
		// return true if success.
		bool getTcpInfo(struct tcp_info*) const;
		string getTcpInfoString() const;

		void send(const void* message, int len);
		void send(const string& message);
		void send(Buffer* message);  // this one will swap data
		void shutdown(); // NOT thread safe, no simultaneous calling
		// void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
		void forceClose();

		void setTcpNoDelay(bool on);

		void setConnectionCallback(const ConnectionCallback& cb)
		{
			connectionCallback = cb;
		}

		void setMessageCallback(const MessageCallback& cb)
		{
			messageCallback = cb;
		}

		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{
			writeCompleteCallback = cb;
		}

		void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark_)
		{
			highWaterMarkCallback = cb;
			highWaterMark = highWaterMark_;
		}

		/// Advanced interface
		Buffer* getInputBuffer()
		{
			return &inputBuffer;
		}

		Buffer* getOutputBuffer()
		{
			return &outputBuffer;
		}

		/// Internal use only.
		void setCloseCallback(const CloseCallback& cb)
		{
			closeCallback = cb;
		}

		// called when TcpServer accepts a new connection
		void connectEstablished();   // should be called only once
		// called when TcpServer has removed me from its map
		void connectDestroyed();  // should be called only once

	private:
		enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
		void handleRead(Timestamp receiveTime);
		void handleWrite();
		void handleClose();
		void handleError();
		// void sendInLoop(string&& message);
		void sendInLoop(const string& message);
		void sendInLoop(const void* message, size_t len);
		void shutdownInLoop();
		// void shutdownAndForceCloseInLoop(double seconds);
		void forceCloseInLoop();
		void setState(StateE s) { state = s; }
		const char* stateToString() const;

	private:
		EventLoop*                  loop;
		const string                name;
		StateE                      state;
		// we don't expose those classes to client.
		std::unique_ptr<Socket>     socket;
		std::unique_ptr<Channel>    channel;
		const InetAddress           localAddr;
		const InetAddress           peerAddr;
		ConnectionCallback          connectionCallback;
		MessageCallback             messageCallback;
		WriteCompleteCallback       writeCompleteCallback;

		HighWaterMarkCallback       highWaterMarkCallback;

		CloseCallback               closeCallback;
		size_t                      highWaterMark;
		Buffer                      inputBuffer;
		Buffer                      outputBuffer;
	};
}


#endif /* TCPCONNECTION_H_ */
