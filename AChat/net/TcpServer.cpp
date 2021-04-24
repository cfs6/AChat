#include "TcpServer.h"

#include <stdio.h>  // snprintf
#include <functional>

#include "../base/Platform.h"
#include "../base/AsyncLog.h"
#include "../base/Singleton.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Sockets.h"

using namespace net;

TcpServer::TcpServer(EventLoop* loop_, const InetAddress& listenAddr,
		  const std::string& nameArg, Option option = kReusePort ):
				  loop(loop_), hostport(listenAddr.toIpPort()),
				  name(nameArg), acceptor(new Acceptor(loop, listenAddr, option=kReusePort)),
				  connectionCallback(defaultConnectionCallback),
				  messageCallback(defaultMessageCallback),
				  started(0), nextConnId(1)
{
	acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    loop->assertInLoopThread();
    LOGD("TcpServer::~TcpServer [%s] destructing", name.c_str());

    stop();
}

void TcpServer::start(int workerThreadCount = 4)
{
	if(started == 0)
	{
		eventLoopThreadPool.reset(new EventLoopThreadPool());
		eventLoopThreadPool->init(loop, workerThreadCount);
		eventLoopThreadPool->start();

		loop->runInLoop(std::bind(&Acceptor::listen, acceptor));
		started = 1;
	}
	else
	{
		LOGD("started != 0 ");
	}
}

void TcpServer::stop()
{
	if(started == 0)
	{
		LOGD("started == 0 already start");
	}

	for(ConnectionMap::iterator it = connections.begin(); it != connections.end(); ++it)
	{
		TcpConnectionPtr conn = it->second;
		it->second.reset();
		conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
		conn.reset();
	}
	eventLoopThreadPool->stop();
	started = 0;
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	loop->assertInLoopThread();
	EventLoop* ioLoop = eventLoopThreadPool->getNextLoop();
	char buf[32];
	snprintf(buf, sizeof(buf), ":%s#%d", hostport.c_str(), nextConnId);
	++nextConnId;
	string connName = name + buf;

	LOGD("TcpServer::newConnection [%s] - new connection [%s] from %s", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

	InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections[connName] = conn;
    conn->setConnectionCallback(connectionCallback);
    conn->setMessageCallback(messageCallback);
    conn->setWriteCompleteCallback(writeCompleteCallback);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe
    //该线程分离完io事件后，立即调用TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    // FIXME: unsafe
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop->assertInLoopThread();
	LOGD("TcpServer::removeConnectionInLoop [%s] - connection %s", name_.c_str(), conn->name().c_str());
	size_t n = connections.erase(conn->getName());

    if (n != 1)
    {
        //出现这种情况，是TcpConneaction对象在创建过程中，对方就断开连接了。
        LOGD("TcpServer::removeConnectionInLoop [%s] - connection %s, connection does not exist.", name_.c_str(), conn->name().c_str());
        return;
    }

	EventLoop* ioLoop = conn->getLoop();
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

