
#ifndef INETADDRESS_H_
#define INETADDRESS_H_
#include <string>
#include "base/Platform.h"

namespace net
{
	class InetAddress
	{
	public:
		// Constructs an endpoint with given port number.
		// Mostly used in TcpServer listening.
		explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);
		// Constructs an endpoint with given ip and port.
		// @c ip should be "1.2.3.4"
		InetAddress(const std::string& ip, uint16_t port);
		// Constructs an endpoint with given struct @c sockaddr_in
		// Mostly used when accepting new connections
		InetAddress(const struct sockaddr_in& addr_): addr(addr_){}

		std::string toIp()const;
		std::string toIpPort()const;
		uint16_t toPort()const;

		// default copy/assignment are Okay

		const struct sockaddr_in& getSockAddrInet()const{ return addr;}
		void setSockAddrInet(const struct sockaddr_in& addr_) { addr = addr_; }

		uint32_t ipNetEndian() const{ return addr.sin_addr.s_addr; }
		uint16_t portNetEndian()const{ return addr.sin_port; }

		// resolve hostname to IP address, not changing port or sin_family
		// return true on success.
		// thread safe
		static bool resolve(const std::string& hostname, InetAddress* result);

	private:
		struct sockaddr_in       addr;
	};
}


#endif /* INETADDRESS_H_ */
