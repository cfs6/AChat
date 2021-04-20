#include "TcpClient.h"

#include <stdio.h>  // snprintf
#include <functional>

#include "../base/AsyncLog.h"
#include "../base/Platform.h"
#include "EventLoop.h"
#include "Sockets.h"
#include "Connector.h"
using namespace net;

namespace net
{
	namespace detail
	{
		void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
		{
			loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
		}
	}

}

TcpClient::TcpClient(EventLoop* loop_, const InetAddress& inetAddr, const std::string& nameArg)
					:loop(loop_), connector(new Connector(loop, inetAddr)), name(nameArg),
					 connectionCallback(defaultConnectionCallback),
					 messageCallback(defaultMessageCallback),
					 retry(false), connected(true), nextConnId(1)
{
	connector->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
    // FIXME setConnectFailedCallback
    LOGD("TcpClient::TcpClient[%s] - connector 0x%x", name.c_str(), connector.get());
}

TcpClient::~TcpClient()
{
	LOGD("TcpClient::~TcpClient[%s] - connector 0x%x", name.c_str(), connector.get());
	TcpConnectionPtr conn;
	bool unique = false;
	{
		std::unique_lock<std::mutex> lock(mutex);
		unique = connection.unique();
		conn = connection;
	}
	if(conn)
	{
		if(loop!=conn->getLoop())
		{
			return;
		}
		// FIXME: not 100% safe, if we are in different thread
		CloseCallback cb = std::bind(&detail::removeConnection, this, std::placeholders::_1);
		loop->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
		if(unique)
		{
			conn->forceClose();
		}
	}
	else
	{
		connector->stop();
        // FIXME: HACK
        // loop_->runAfter(1, boost::bind(&detail::removeConnector, connector_));
	}
}

void TcpClient::connect()
{
	// FIXME: check state
	LOGD("TcpClient::connect[%s] - connecting to %s", name.c_str(), connector->serverAddress().toIpPort().c_str());
	connected = true;
	connector->start();
}

void TcpClient::disConnect()
{
	connected = false;
	{
		std::unique_lock<std::mutex> lock(mutext);
		if(connection)
		{
			connection->shutdown();
		}
	}
}

void TcpClient::stop()
{
	connected = false;
	connector->stop();
}

void TcpClient::newConnection(int sockfd)
{
	loop->assertInLoopThread();
	InetAddress peerAddr(sockets::getPeerAddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId);
	++nextConnId;
	std::string connName = name + buf;

	InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(loop, connName, sockfd, localAddr, peerAddr));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe

    {
    	std::unique_lock<std::mutex> lock(mutex);
    	connection = conn;
    }

    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	loop->assertInLoopThread();
	if(loop != conn->getLoop())
	{
		return;
	}

	{
		std::unique_lock<std::mutex> lock(mutex);
		if(connection != conn)
		{
			LOGSYSE("connection != conn");
			return;
		}
		connection.reset();
	}

	loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));

	if(retry && connected)
	{
        LOGD( "TcpClient::connect[%s] - Reconnecting to %s", name.c_str(), connector->serverAddress().toIpPort().c_str());
        connector->restart();
	}
}








