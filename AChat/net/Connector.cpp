#include "Connector.h"

#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include <errno.h>
#include <sstream>
#include <iostream>
#include <string.h> //for strerror
#include "../base/AsyncLog.h"
#include "../base/Platform.h"
#include "Sockets.h"

using namespace net;

Connector::Connector(EventLoop* loop_, InetAddress& serverAddr_)
	:loop(loop_),
	 serverAddr(serverAddr_),
	 connected(false),
	 state(kDisconnected),
	 retryDelayMs(kInitRetryDelayMs)
{

}

Connector::~Connector()
{

}

void Connector::start()
{
	connected = true;
	loop->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
	loop->assertInLoopThread();
	if(state != kDisconnected)
	{
		LOGD("state != kDisconnected");
		return;
	}
	if(connected)
	{
		connect();
	}
	else
	{
		LOGD("Connector::startInLoop already connected!");
	}
}

void Connector::stop()
{
	connected = false;
	loop->queueInLoop(std::bind(&Connector::stopInLoop, shared_from_this()));// FIXME: unsafe
	// FIXME: cancel timer
}

void Connector::stopInLoop()
{
	loop->assertInLoopThread();
	if(state == kConnecting)
	{
		setState(kDisconnected);
		int sockfd = removeAndResetChannel();
		retry(sockfd);
	}
}

void Connector::connect()
{
	int sockfd = sockets::createOrDie();
	int ret = sockets::connect(sockfd, serverAddr.getSockAddrInet());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOGSYSE("connect error in Connector::startInLoop, %d ", savedErrno);
        sockets::close(sockfd);
        break;

    default:
        LOGSYSE("Unexpected error in Connector::startInLoop, %d ", savedErrno);
        sockets::close(sockfd);
        // connectErrorCallback_();
        break;
    }
}

void Connector::restart()
{
	loop->assertInLoopThread();
	setState(kDisconnected);
	retryDelayMs = kInitRetryDelayMs;
	connected = true;
	startInLoop();
}

void Connector::connecting(int sockfd)
{
	setState(kConnecting);
	channel->reset(new Channel(loop, sockfd));
	channel->setWriteCallback(std::bind(&Connector::handleWrite, this));
	channel->setErrorCallback(std::bind(&Connector::handleError, this));

	channel->enableWriting();
}

int Connector::removeAndResetChannel()
{
	channel->disableAll();
	channel->remove();
	int sockfd = channel->getFd();
	// Can't reset channel_ here, because we are inside Channel::handleEvent
	loop->queueInLoop(std::bind(&Connector::resetChannel), shared_from_this());
	return sockfd;
}

void Connector::resetChannel()
{
	channel.reset();
}

void Connector::handleWrite()
{
	LOGD("Connector::handleWrite %d", state);

	if(state == kConnecting)
	{
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		if(err)
		{
			LOGW("Connector::handleWrite - SO_ERROR = %d %s", err, strerror(err));
			retry(sockfd);
		}
		else if(sockets::isSelfConnect(sockfd))
		{
            LOGW("Connector::handleWrite - Self connect");
            retry(sockfd);
		}
		else
		{
			setState(kConnected);
			if(connected)
			{
				newConnectionCallback(sockfd);
			}
			else
			{
				sockets::close(sockfd);
			}
		}
	}
	else
	{
		if(state != kDisconnected)
		{
			LOGSYSE("state_ != kDisconnected");
		}
	}
}

void Connector::handleError()
{
    LOGE("Connector::handleError state=%d", state);
    if (state == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOGD("SO_ERROR = %d %s", err, strerror(err));
        LOGE("Connector::handleError state=%d", state);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd)
{
	sockets::close(sockfd);
	setState(kDisconnected);
	if(connected)
	{
		LOGI("Connector::retry - Retry connecting to %s in %d  milliseconds.",
				serverAddr.toIpPort().c_str(), retryDelayMs);
		loop->runAfter(retryDelayMs/1000.0,
                std::bind(&Connector::startInLoop, shared_from_this()));
		retryDelayMs = std::min(retryDelayMs * 2, kMaxRetryDelayMs);
	}
    else
    {
        LOGD("do not connect");
    }
}












