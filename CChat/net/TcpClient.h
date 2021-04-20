
#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include <string>
#include <mutex>
#include "TcpConnection.h"

namespace net
{
	class EventLoop;
	class Connector;
	typedef std::shared_ptr<Connector> ConnectorPtr;

	class TcpClient
	{
	public:
		TcpClient(EventLoop* loop, const InetAddress& inetAddr, const std::string& nameArg);
		~TcpClient();       // force out-line dtor, for scoped_ptr members.

		void connect();
		void disConnect();
		void stop();

		TcpConnectionPtr getConnection()const
		{
			std::unique_lock<std::mutex> lock(mutex);
			return connection;
		}

		EventLoop* getLoop()const { return loop; }
		bool isRetry();
		void enableRetry();

		const std::string& getName()const { return name; }
        /// Set connection callback.
        /// Not thread safe.
		void setConnectionCallback(const ConnectionCallback& cb)
		{
			connectionCallback = cb;
		}
        /// Set message callback.
        /// Not thread safe.
		void setMessageCallback(const MessageCallback& cb)
		{
			messageCallback = cb;
		}
        /// Set write complete callback.
        /// Not thread safe.
		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{
			writeCompleteCallback = cb;
		}

	private:
		/// Not thread safe, but in loop
		void newConnection(int sockfd);
		/// Not thread safe, but in loop
		void removeConnection(const TcpConnectionPtr& conn);
	private:
		EventLoop*                         loop;
		ConnectorPtr                       connector;   // avoid revealing Connector
		const std::string                  name;
		ConnectionCallback                 connectionCallback;
		MessageCallback                    messageCallback;
		WriteCompleteCallback              writeCompleteCallback;
		bool                               retry;       // atomic
		bool                               connected;     // atomic

        // always in loop thread
        int                                nextConnId;
        mutable std::mutex                 mutex;
        TcpConnectionPtr                   connection; // @GuardedBy mutex_

	};
}

#endif /* TCPCLIENT_H_ */
