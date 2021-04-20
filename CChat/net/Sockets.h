
#ifndef SOCKETS_H_
#define SOCKETS_H_
#pragma once

#include<stdint.h>

struct tcp_info;

namespace net
{
	class InetAddress;

	class Socket
	{
	public:
		explicit Socket(int sockfd_):sockfd(sockfd_){}
		~Socket();

		SOCKET getFd()const{ return sockfd; }

		bool getTcpInfo(struct tcp_info*) const;
		bool getTcpInfoString(char* buf, int len)const;

		void bindAddress(const InetAddress& loackAddr);

		void listen();

		int accept(InetAddress* peerAddr);

		void shutdownWrite();
		// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
		void setTcpNoDelay(bool on);
		// Enable/disable SO_REUSEADDR
		void setReuseAddr(bool on);
		// Enable/disable SO_REUSEPORT
		void setReusePort(bool on);
		// Enable/disable SO_KEEPALIVE
		void setKeepAlive(bool on);

	private:
		const SOCKET sockfd;
	};

	namespace sockets
	{
		///
		/// Creates a socket file descriptor,
		/// abort if any error.
		SOCKET createOrDie();
		SOCKET createNonblockingOrDie();

		void setNonBlockAndCloseOnExec(SOCKET sockfd);

		void setReuseAddr(SOCKET sockfd, bool on);
		void setReusePort(SOCKET sockfd, bool on);

		SOCKET connect(SOCKET sockfd, const struct sockaddr_in& addr);
		void bindOrDie(SOCKET sockfd, const struct sockaddr_in& addr);
		void listenOrDie(SOCKET sockfd);
		SOCKET accept(SOCKET sockfd, struct sockaddr_in* addr);
		int32_t read(SOCKET sockfd, void *buf, int32_t count);

		int32_t write(SOCKET sockfd, const void *buf, int32_t count);
		void close(SOCKET sockfd);
		void shutdownWrite(SOCKET sockfd);

		//将addr 转换成字符串Ip:Port形式
		void toIpPort(char* buf, size_t size, const struct sockaddr_in& addr);
		//将addr转换成字符串Ip
		void toIp(char* buf, size_t size, const struct sockaddr_in& addr);
		//将IpPort转换为网络地址
		void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);

		int getSocketError(SOCKET sockfd);

		const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
		struct sockaddr* sockaddr_cast(struct sockaddr_in* addr);
		const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
		struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr);

		//获取本端socket地址
		struct sockaddr_in getLocalAddr(SOCKET sockfd);
		//获取远端socket地址
		struct sockaddr_in getPeerAddr(SOCKET sockfd);
		bool isSelfConnect(SOCKET sockfd);
	}
}



#endif /* SOCKETS_H_ */
