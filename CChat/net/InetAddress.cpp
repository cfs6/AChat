#include "InetAddress.h"

#include<string>
#include<base/AsyncLog.h>
#include "Endian.h"
#include "Sockets.h"

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

using namespace net;

InetAddress::InetAddress(uint16_t port = 0, bool loopbackOnly = false)
{
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	in_addr_t ip = loopbackOnly?kInaddrLoopback:kInaddrAny;
	addr.sin_addr.s_addr = sockets::hostToNetwork32(ip);
	addr.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port)
{
	memset(&addr, 0, sizeof(addr));
	sockets::fromIpPort(ip.c_str(), port, &addr);
}

std::string InetAddress::toIpPort()const
{
	char buf[32];
	sockets::toIpPort(buf, sizeof(buf), addr);
	return buf;
}

std::string InetAddress::toIp()const
{
	char buf[32];
	sockets::toIp(buf, sizeof(buf), addr);
	return buf;
}

uint16_t InetAddress::toPort()const
{
	return sockets::networkToHost16(addr.sin_port);
}

static thread_local char resolveBuffer[64 * 1024];


bool InetAddress::resolve(const std::string& hostname, InetAddress* out)
{
	//assert(out != NULL);
	if(out==NULL)
	{
		LOGE("resolve hostname out==NULL!");
	}
	struct hostent hent;
	struct hostent* he = NULL;
	int herrno = 0;
	memset(&hent, 0, sizeof(hent));

	int ret = gethostbyname_r(hostname.c_str(), &hent, resolveBuffer, sizeof resolveBuffer, &he, &herrno);
	if (ret == 0 && he != NULL)
	{
		//assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
		if(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t))
		{
			LOGE("he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t)");
		}
		out->addr.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	}

	if (ret)
	{
		LOGSYSE("InetAddress::resolve");
	}

	return false;
}







