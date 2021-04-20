#include "TcpConnection.h"

#include <functional>
#include <thread>
#include <sstream>
#include <errno.h>
#include "../base/Platform.h"
#include "../base/AsyncLog.h"
#include "Sockets.h"
#include "EventLoop.h"
#include "Channel.h"
#include <memory>
#include "base/AsyncLog.h"
#include "Callbacks.h"
using namespace net;

void net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOGD("%s -> is %s",
        conn.localAddress().toIpPort().c_str(),
        conn.peerAddress().toIpPort().c_str(),
        (conn.connected() ? "UP" : "DOWN"));
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void net::defaultMessageCallback(const TcpConnectionPtr&, Buffer* buf, Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop, const string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop(loop),
    name(nameArg),
    state(kConnecting),
    socket(new Socket(sockfd)),
    channel(new Channel(loop, sockfd)),
    localAddr(localAddr),
    peerAddr(peerAddr),
    highWaterMark(64 * 1024 * 1024)
{
    channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOGD("TcpConnection::ctor[%s] at 0x%x fd=%d", name.c_str(), this, sockfd);
    socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOGD("TcpConnection::dtor[%s] at 0x%x fd=%d state=%s",
         name.c_str(), this, channel->getFd(),stateToString());
    //assert(state == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
	return socket->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket->getTcpInfoString(buf, sizeof buf);
    return buf;
}

void TcpConnection::send(const void* message, int len)
{
	if(state == kConnected)
	{
		if(loop-> isInLoopThread())
		{
			sendInLoop(message, len);
		}
		else
		{
			string message(static_cast<const char*>(message, len));
			loop->runInLoop(
					std::bind(static_cast<void (TcpConnection::*)(const string&)>(&TcpConnection::sendInLoop),
							this, message));  // FIXME)
		}
	}
}

void TcpConnection::send(const string & message)
{
    if (state == kConnected)
    {
        if (loop->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop->runInLoop(
                std::bind(static_cast<void (TcpConnection::*)(const string&)>(&TcpConnection::sendInLoop),
                    this,     // FIXME
                    message));
            //std::forward<string>(message)));
        }
    }
}

void TcpConnection::send(Buffer* buf)
{
	if(state == kConnected)
	{
		if(loop->isInLoopThread())
		{
			sendInLoop(buf->peek(), buf->readableBytes());
			buf->retrieveAll();
		}
		else
		{
			loop->runInLoop(std::bind(static_cast<void (TcpConnection::*)(const string&)>(&TcpConnection::sendInLoop),
	                this,     // FIXME
	                buf->retrieveAllAsString()));
		}
	}
}

void TcpConnection::sendInLoop(const string & message)
{
    sendInLoop(message.c_str(), message.size());
}

void TcpConnection::sendInLoop(const void* message, size_t len)
{
	loop->assertInLoopThread();
	int32_t nwrote = 0;
	size_t remaining = len;
	bool faultError = false;
	if(state == kDisconnected)
	{
		LOGW("disconnected, give up writing!");
	}

	if(!channel->isWriting() && outputBuffer.readableBytes() == 0)
	{
		nwrote = sockets::write(channel->getFd(), message, len);

		if(nwrote >= 0)
		{
			remaining = len - nwrote;
			if(remaining == 0 && writeCompleteCallback)
			{
				loop->queueInLoop(std::bind(writeCompleteCallback, shared_from_this()));
			}
		}
		//send()相关方法调用的write()出错时
		else  // nwrote < 0
		{
			nwrote = 0;
			if(errno != EWOULDBLOCK)
			{
				LOGSYSE("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
			}
		}
	}
	if(remaining > len)
	{
		LOGSYSE("TcpConnection::sendInLoop remaining > len");
		return;
	}
	if(!faultError && remaining > 0)
	{
		size_t oldLen = outputBuffer.readableBytes();
		if(oldLen + remaining >= highWaterMark
				&& oldLen < highWaterMark
				&& highWaterMarkCallback)
		{
			loop->queueInLoop(std::bind(highWaterMarkCallback, shared_from_this(), oldLen + remaining));
		}
		outputBuffer.append(static_cast<const char*>(message) + nwrote, remaining);
		if(!channel->isWriting())
		{
			channel->enableWriting();
		}
	}
}

void TcpConnection::shutdown()
{
	if(state == kConnected)
	{
		setState(kDisconnecting);
		loop->runInLoop(std::bind(&TcpConnection::closeCallback, this));
	}
}

void TcpConnection::shutdownInLoop()
{
	loop->assertInLoopThread();
	if(!channel->isWriting())
	{
		socket->shutdownWrite();
	}
}

void TcpConnection::shutdownInLoop()
{
	loop->assertInLoopThread();
    if (!channel_->isWriting())
    {
        // we are not writing
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose()
{
    // FIXME: use compare and swap
    if (state == kConnected || state == kDisconnecting)
    {
        setState(kDisconnecting);
        loop->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, this));//todo shard_from_this ?
    }
}


void TcpConnection::forceCloseInLoop()
{
    loop->assertInLoopThread();
    if (state == kConnected || state == kDisconnecting)
    {
        // as if we received 0 byte in handleRead();
        handleClose();
    }
}

const char* TcpConnection::stateToString() const
{
    switch (state)
    {
    case kDisconnected:
        return "kDisconnected";
    case kConnecting:
        return "kConnecting";
    case kConnected:
        return "kConnected";
    case kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
    loop->assertInLoopThread();
    if (state != kConnecting)
    {
        //一定不能走这个分支
        return;
    }

    setState(kConnected);
    channel->tie(shared_from_this());

    //假如正在执行这行代码时，对端关闭了连接
    if (!channel->enableReading())
    {
        LOGE("enableReading failed.");
        //setState(kDisconnected);
        handleClose();
        return;
    }

    //connectionCallback_指向void XXServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
    connectionCallback(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
	loop->isInLoopThread();
	if(state == kConnected)
	{
		setState(kDisconnected);
		channel->disableAll();
		connectionCallback(shared_from_this());
	}
	channel->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
	loop->assertInLoopThread();
	int savedErrno = 0;
	int32_t ret = inputBuffer.readFd(channel->getFd(), &savedErrno);
	if(ret > 0)
	{
		messageCallback(shared_from_this(), &inputBuffer, receiveTime);
	}
	else if(ret == 0)
	{
		handleClose();
	}
	else
	{
		errno = savedErrno;
		LOGSYSE("TcpConnection::handleRead");
		handleError();
	}
}

void TcpConnection::handleWrite()
{
	loop->assertInLoopThread();
	if(channel->isWriting())
	{
		int32_t n = sockets::write(channel->getFd(), outputBuffer.peek(), outputBuffer.readableBytes());
		if(n > 0)
		{
			outputBuffer.retrieve(n);
			if(outputBuffer.readableBytes() == 0)
			{
				channel->disableWriting();
				if(writeCompleteCallback)
				{
					loop->queueInLoop(std::bind(writeCompleteCallback, shared_from_this()));
				}
				if(state == kDisconnecting)
				{
					shutdownInLoop();
				}
			}
		}
		else
		{
			LOGSYS("TcpConnection::handleWrite");
			handleClose();
		}
	}
	else
	{
	    LOGD("Connection fd = %d is down, no more writing, %d");
	}
}

void TcpConnection::handleClose()
{
    //在Linux上当一个链接出了问题，会同时触发handleError和handleClose
    //为了避免重复关闭链接，这里判断下当前状态
    //已经关闭了，直接返回
    if (state == kDisconnected)
        return;

    loop->assertInLoopThread();
    LOGD("fd = %d  state = %s", channel->getFd(), stateToString());

    //assert(state == kConnected || state == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);
    channel->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback(guardThis);

    closeCallback(guardThis);

    //只处理业务上的关闭，真正的socket fd在TcpConnection析构函数中关闭
    //if (socket_)
    //{
    //    sockets::close(socket_->fd());
    //}
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel->getFd());
    LOGE("TcpConnection::%s handleError [%d] - SO_ERROR = %s" , name.c_str(), err , strerror(err));

    //调用handleClose()关闭连接，回收Channel和fd
    handleClose();
}












