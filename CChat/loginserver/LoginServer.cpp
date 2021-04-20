#include "LoginServer.h"
#include "AsyncLog.h"
#include "LoginConn.h"


bool LoginServer::init(const char* ip, short port, EventLoop* loop)
{
	InetAddress addr(ip, port);
//	handlerMap = HandlerMap::getInstance();
	//threadpool?
	loginServer.reset(new TcpServer(loop, addr, "LoginServer", TcpServer::kReusePort));
	loginServer->setConnectionCallback(std::bind(&LoginServer::onConnected, this, std::placeholders::_1));
	loginServer->start(2);
}

void LoginServer::uninit()
{
	if(loginServer)
		loginServer->stop();
}

void LoginServer::onConnected(std::shared_ptr<TcpConnection> conn)
{
	if(conn->connected)
	{
        LOGI("login connected: %s", conn->peerAddress().toIpPort().c_str());
        std::shared_ptr<LoginConn> spSession(new LoginConn(conn));
        conn->setMessageCallback(std::bind(&LoginConn::onRead, spSession.get(),
        		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        conn->setConnectionCallback(std::bind(&LoginConn::loginConnCheck, this));

        {
			std::lock_guard<std::mutex> guard(connMutex);
			loginConnList.push_back(spSession);
        }
	}
	else
	{
		onDisconnected(conn);
	}
}

void LoginServer::onDisconnected(const std::shared_ptr<TcpConnection>& conn)
{
    //TODO
    std::lock_guard<std::mutex> guard(connMutex);
    for (auto iter = loginConnList.begin(); iter != loginConnList.end(); ++iter)
    {
        if ((*iter)->getConnectionPtr() == NULL)
        {
            LOGE("connection is NULL");
            break;
        }

        //用户下线
        loginConnList.erase(iter);
        //bUserOffline = true;
        LOGI("client disconnected: %s", conn->peerAddress().toIpPort().c_str());
        break;
    }
}


