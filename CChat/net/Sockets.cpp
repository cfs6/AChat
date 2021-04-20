#include "Sockets.h"
#include <stdio.h>
#include <string.h>


#include "../base/AsyncLog.h"
#include "InetAddress.h"
#include "Endian.h"
#include "Callbacks.h"

using namespace net;

Socket::~Socket()
{
	sockets::close(sockfd);
}

bool Socket::getTcpInfo(struct tcp_info* tcpi) const
{
	socklen_t len = sizeof(*tcpi);
//  memZero(tcpi, len);
	memset(tcpi, 0, len);
	return ::getsockopt(sockfd, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char* buf, int len)const
{
	struct tcp_info tcpi;
	bool ok = getTcpInfo(&tcpi);
	if (ok)
	{
		snprintf(buf, len, "unrecovered=%u "
				"rto=%u ato=%u snd_mss=%u rcv_mss=%u "
				"lost=%u retrans=%u rtt=%u rttvar=%u "
				"sshthresh=%u cwnd=%u total_retrans=%u",
				tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
				tcpi.tcpi_rto,          // Retransmit timeout in usec
				tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
				tcpi.tcpi_snd_mss, tcpi.tcpi_rcv_mss,
				tcpi.tcpi_lost,         // Lost packets
				tcpi.tcpi_retrans,      // Retransmitted packets out
				tcpi.tcpi_rtt,          // Smoothed round trip time in usec
				tcpi.tcpi_rttvar,       // Medium deviation
				tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
				tcpi.tcpi_total_retrans); // Total retransmits for entire connection
	}
	return ok;
}

void Socket::bindAddress(const InetAddress& addr)
{
    sockets::bindOrDie(sockfd, addr.getSockAddrInet());
}

void Socket::listen()
{
    sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peerAddr)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	int connfd = sockets::accept(sockfd, &addr);
	if(connfd>=0)
	{
		peerAddr->setSockAddrInet(addr);
	}
	return connfd;
}

void Socket::shutdownWrite()
{
    sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on)
{
	int opt = on ? 1 : 0;
	::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof(opt)));
	//todo
}

void Socket::setReuseAddr(bool on)
{
    sockets::setReuseAddr(sockfd, on);
}

void Socket::setReusePort(bool on)
{
    sockets::setReusePort(sockfd, on);
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr)
{
    return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in* addr)
{
    return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr)
{
    return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

struct sockaddr_in* sockets::sockaddr_in_cast(struct sockaddr* addr)
{
    return static_cast<struct sockaddr_in*>(implicit_cast<void*>(addr));
}

SOCKET sockets::createOrDie()
{
	SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if(sockfd<0)
	{
		LOGF("sockets::createNonblockingOrDie");
	}
	return sockfd;
}

SOCKET sockets::createNonblockingOrDie()
{
    SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOGF("sockets::createNonblockingOrDie");
    }
    setNonBlockAndCloseOnExec(sockfd);
    return sockfd;
}

void sockets::setNonBlockAndCloseOnExec(SOCKET sockfd)
{
	int flags = ::fcntl(sockfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	int ret = ::fcntl(sockfd, F_SETFL, flags);
	//todo check
	// close-on-exec
	flags = ::fcntl(sockfd, F_GETFD, 0);
	flags |= FD_CLOEXEC;
	ret = ::fcntl(sockfd, F_SETFD, flags);
	//todo check
	(void) ret;
}

void sockets::bindOrDie(SOCKET sockfd, const struct sockaddr_in& addr)
{
	int ret = ::bind(sockfd, sockaddr_cast(&addr), static_cast<socklen_t>(sizeof(addr)));
	if(ret < 0)
	{
		LOGF("sockets::bindOrDie");
	}
}

void sockets::listenOrDie(SOCKET sockfd)
{
	int ret = ::listen(sockfd, SOMAXCONN);
	if(ret < 0)
	{
		LOGF("sockets::listenOrDie");
	}
}

SOCKET sockets::accept(SOCKET sockfd, struct sockaddr_in* addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof(*addr));

	SOCKET connfd = accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

	if(connfd < 0)
	{
        int savedErrno = errno;
        LOGSYSE("Socket::accept");
        switch (savedErrno)
        {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO: // ???
        case EPERM:
        case EMFILE: // per-process lmit of open file desctiptor ???
            // expected errors
            errno = savedErrno;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            // unexpected errors
            LOGF("unexpected error of ::accept %d", savedErrno);
            break;
        default:
            LOGF("unknown error of ::accept %d", savedErrno);
            break;
        }
	}
	return connfd;
}

void sockets::setReuseAddr(SOCKET sockfd, bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
	//todo check
}

void sockets::setReusePort(SOCKET sockfd, bool on)
{
	int optval = on ? 1:0;
	::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(optval)));
	//todo check
}

SOCKET sockets::connect(SOCKET sockfd, const struct sockaddr_in& addr)
{
    return ::connect(sockfd, sockaddr_cast(&addr), static_cast<socklen_t>(sizeof addr));
}

int32_t sockets::read(SOCKET sockfd, void* buf, int32_t count)
{
    return ::read(sockfd, buf, count);
}

/*ssize_t sockets::readv(SOCKET sockfd, const struct iovec* iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}*/

int32_t sockets::write(SOCKET sockfd, const void* buf, int32_t count)
{
    return ::write(sockfd, buf, count);
}

void sockets::close(SOCKET sockfd)
{
    if (::close(sockfd) < 0)
    {
        LOGSYSE("sockets::close, fd=%d, errno=%d, errorinfo=%s", sockfd, errno, strerror(errno));
    }
}

void sockets::shutdownWrite(SOCKET sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOGSYSE("sockets::shutdownWrite");
    }
}

void sockets::toIpPort(char* buf, size_t size, const struct sockaddr_in& addr)
{
	::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(sizeof(addr)));
	size_t end = ::strlen(buf);
	uint16_t port = sockets::networkToHost16(addr.sin_port);
	snprintf(buf + end, size - end, ":%u", port);
}

void sockets::toIp(char* buf, size_t size, const struct sockaddr_in& addr)
{
	if(size>=sizeof(struct sockaddr_in))
	{
		return;
	}
	::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(sizeof(addr)));
}

void sockets::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htobe16(port);
	if(::inet_pton(AF_INET, ip, &addr->sin_addr)<=0)
	{
		LOGSYSE("sockets::fromIpPort");
	}
}

int sockets::getSocketError(SOCKET sockfd)
{
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
	if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen)<0)
	{
		return errno;
	}
	return optval;
}

struct sockaddr_in sockets::getLocalAddr(SOCKET sockfd)
{
	struct sockaddr_in localaddr = {0};
	memset(&localaddr, 0, sizeof(localaddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
	if(::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen)<0)
	{
		LOGSYSE("sockets::getLocalAddr");
	}
	return localaddr;
}

struct sockaddr_in sockets::getPeerAddr(SOCKET sockfd)
{
	struct sockaddr_in peeraddr = {0};
	memset(&peeraddr, 0, sizeof(peeraddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
	if(::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen)<0)
	{
		LOGSYSE("sockets::getPeerAddr");
	}
	return peeraddr;
}

bool sockets::isSelfConnect(SOCKET sockfd)
{
	struct sockaddr_in localaddr = getLocalAddr(sockfd);
	struct sockaddr_in peeraddr = getPeerAddr(sockfd);
	return localaddr.sin_port == peeraddr.sin_port && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}



