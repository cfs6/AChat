
#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include <map>
#include <atomic>
#include <memory>

namespace net
{
	class Acceptor;
	class EventLoop;
	class EventLoopThreadPool;

	// TCP server, supports single-threaded and thread-pool models.
	// This is an interface class, so don't expose too much details.
	class TcpServer
	{
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;
		enum Option
		{
			kNoReusePort,
			kReusePort
		};

		TcpServer(EventLoop* loop, const InetAddress& listenAddr,
				  const std::string& nameArg, Option option = kReusePort );
		~TcpServer();

		const std::string& getHostport()const { return hostport; }
		const std::string& getName()const { return name; }
		EventLoop* getLoop()const{ return loop; }

		/// Set the number of threads for handling input.
		///
		/// Always accepts new connection in loop's thread.
		/// Must be called before @c start
		/// @param numThreads
		/// - 0 means all I/O in loop's thread, no thread will created.
		///   this is the default value.
		/// - 1 means all I/O in another thread.
		/// - N means a thread pool with N threads, new connections
		///   are assigned on a round-robin basis.
		//void setThreadNum(int numThreads);
		void setThreadInitCallback(const ThreadInitCallback cb){ threadInitCallback = cb; }
		/// valid after calling start()
		//std::shared_ptr<EventLoopThreadPool> threadPool()
		//{ return threadPool_; }

		/// Starts the server if it's not listenning.
		///
		/// It's harmless to call it multiple times.
		/// Thread safe.
		void start(int workerThreadCount = 4);
		void stop();

		/// Set connection callback.
		/// Not thread safe.
		void setConnectionCallback(const ConnectionCallback& cb)
		{ connectionCallback = cb; }

		/// Set message callback.
		/// Not thread safe.
		void setMessageCallback(const MessageCallback& cb)
		{ messageCallback = cb; }

		/// Set write complete callback.
		/// Not thread safe.
		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{ writeCompleteCallback = cb; }

		void removeConnection(const TcpConnectionPtr& conn);
	private:
		/// Not thread safe, but in loop
		void newConnection(int sockfd, const InetAddress& peerAddr);
		/// Thread safe.
		/// Not thread safe, but in loop
		void removeConnectionInLoop(const TcpConnectionPtr& conn);

	private:
		typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

		EventLoop*                              loop;                   // the acceptor loop
		const std::string                       hostport;
		const std::string                       name;
		std::unique_ptr<Acceptor>               acceptor;               // avoid revealing Acceptor
		std::unique_ptr<EventLoopThreadPool>    eventLoopThreadPool;
		ConnectionCallback                      connectionCallback;
		MessageCallback                         messageCallback;
		WriteCompleteCallback                   writeCompleteCallback;
		ThreadInitCallback                      threadInitCallback;
		std::atomic<int>                        started;
		int                                     nextConnId;             // always in loop thread
		ConnectionMap                           connections;
	};

}




#endif /* TCPSERVER_H_ */
