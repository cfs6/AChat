
#ifndef PLATFORM_H_
#define PLATFORM_H_

#pragma once

#include <stdint.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif


typedef int SOCKET;

#define SOCKET_ERROR -1

#define closesocket(s) close(s)


#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

#include <unistd.h>
#include <stdint.h>
#include <endian.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>
#include <errno.h>
#include <dirent.h>


#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <sys/syscall.h>

//for ubuntu readv not found
#ifdef __UBUNTU
#include <sys/uio.h>
#endif


#define  XPOLLIN         POLLIN
#define  XPOLLPRI        POLLPRI
#define  XPOLLOUT        POLLOUT
#define  XPOLLERR        POLLERR
#define  XPOLLHUP        POLLHUP
#define  XPOLLNVAL       POLLNVAL
#define  XPOLLRDHUP      POLLRDHUP

#define  XEPOLL_CTL_ADD  EPOLL_CTL_ADD
#define  XEPOLL_CTL_DEL  EPOLL_CTL_DEL
#define  XEPOLL_CTL_MOD  EPOLL_CTL_MOD

//Linux��û������������������֮
#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)

#endif




#endif /* PLATFORM_H_ */
