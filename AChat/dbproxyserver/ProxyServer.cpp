#include "ProxyServer.h"
#include "net/InetAddress.h"
#include "AsyncLog.h"

//HandlerMap* ProxyServer::handlerMap = nullptr;

bool ProxyServer::init(const char* ip, short port, EventLoop* loop)
{
	InetAddress addr(ip, port);
//	handlerMap = HandlerMap::getInstance();
	//threadpool?
	proxyServer.reset(new TcpServer(loop, addr, "ProxyServer", TcpServer::kReusePort));
	proxyServer->setConnectionCallback(std::bind(&ProxyServer::onConnected, this, std::placeholders::_1));
	proxyServer->start(2);
}

void ProxyServer::uninit()
{
	if(proxyServer)
		proxyServer->stop();
}

ProxyServer* ProxyServer::getInstance()
{
	if (!proxyServerInstance) {
		proxyServerInstance = new ProxyServer();
	}

	return proxyServerInstance;
}

void ProxyServer::onConnected(std::shared_ptr<TcpConnection> conn)
{
	if(conn->connected)
	{
        LOGI("proxy connected: %s", conn->peerAddress().toIpPort().c_str());
        std::shared_ptr<ProxyConn> spSession(new ProxyConn(conn));
        conn->setMessageCallback(std::bind(&ProxyConn::onRead, spSession.get(),
        		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        std::lock_guard<std::mutex> guard(connMutex);
        proxyConnList.push_back(spSession);
	}
	else
	{
		onDisconnected(conn);
	}
}

void ProxyServer::onDisconnected(const std::shared_ptr<TcpConnection>& conn)
{
    //TODO: 这样的代码逻辑太混乱，需要优化
    std::lock_guard<std::mutex> guard(connMutex);
    for (auto iter = proxyConnList.begin(); iter != proxyConnList.end(); ++iter)
    {
        if ((*iter)->getConnectionPtr() == NULL)
        {
            LOGE("connection is NULL");
            break;
        }

        //用户下线
        proxyConnList.erase(iter);
        //bUserOffline = true;
        LOGI("client disconnected: %s", conn->peerAddress().toIpPort().c_str());
        break;
    }
}
