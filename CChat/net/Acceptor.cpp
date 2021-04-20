
#include "Acceptor.h"

#include "../base/Platform.h"
#include "../base/AsyncLog.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Sockets.h"

using namespace net;

Acceptor::Acceptor(EventLoop* loop_, const InetAddress& listenAddr, bool reuseport):
		loop(loop_), acceptSocket(sockets::createOrDie()),
		acceptChannel(loop_, acceptSocket.getFd()), listenning(false)
{
    idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);

    acceptSocket.setReuseAddr(true);
    acceptSocket.setReusePort(reuseport);
    acceptSocket.bindAddress(listenAddr);
    acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
	acceptChannel.disableAll();
	acceptChannel.remove();

	::close(idleFd);
}

void Acceptor::listen()
{
	loop->assertInLoopThread();
	InetAddress peerAddr;

	int connfd = acceptSocket.accept(&peerAddr);
	if(connfd >= 0)
	{
		std::string hostport = peerAddr.toIpPort();
		LOGD("Accepts of %s", hostport.c_str());

		if(newConnectionCallBack)
		{
			newConnectionCallBack(connfd, peerAddr);
		}
		else
		{
			LOGE("!newConnectionCallBack");
			sockets::close(connfd);
		}
	}
	else
	{
        LOGSYSE("in Acceptor::handleRead");

#ifndef WIN32
        /*
        The special problem of accept()ing when you can't

        Many implementations of the POSIX accept function (for example, found in post-2004 Linux)
        have the peculiar behaviour of not removing a connection from the pending queue in all error cases.

        For example, larger servers often run out of file descriptors (because of resource limits),
        causing accept to fail with ENFILE but not rejecting the connection, leading to libev signalling
        readiness on the next iteration again (the connection still exists after all), and typically
        causing the program to loop at 100% CPU usage.

        Unfortunately, the set of errors that cause this issue differs between operating systems,
        there is usually little the app can do to remedy the situation, and no known thread-safe
        method of removing the connection to cope with overload is known (to me).

        One of the easiest ways to handle this situation is to just ignore it - when the program encounters
        an overload, it will just loop until the situation is over. While this is a form of busy waiting,
        no OS offers an event-based way to handle this situation, so it's the best one can do.

        A better way to handle the situation is to log any errors other than EAGAIN and EWOULDBLOCK,
        making sure not to flood the log with such messages, and continue as usual, which at least gives
        the user an idea of what could be wrong ("raise the ulimit!"). For extra points one could
        stop the ev_io watcher on the listening fd "for a while", which reduces CPU usage.

        If your program is single-threaded, then you could also keep a dummy file descriptor for overload
        situations (e.g. by opening /dev/null), and when you run into ENFILE or EMFILE, close it,
        run accept, close that fd, and create a new dummy fd. This will gracefully refuse clients under
        typical overload conditions.

        The last way to handle it is to simply log the error and exit, as is often done with malloc
        failures, but this results in an easy opportunity for a DoS attack.
        */
        if (errno == EMFILE)
        {
            ::close(idleFd);
            idleFd = ::accept(acceptSocket.getFd(), NULL, NULL);
            ::close(idleFd);
            idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
#endif
	}
}

