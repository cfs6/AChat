#include "MsgServer.h"

#include "AsyncLog.h"
#include "MsgConn.h"

bool MsgServer::init(const char* ip, short port, EventLoop* loop)
{
	InetAddress addr(ip, port);
//	handlerMap = HandlerMap::getInstance();
	//threadpool?
	MsgServer.reset(new TcpServer(loop, addr, "MsgServer", TcpServer::kReusePort));
	MsgServer->setConnectionCallback(std::bind(&MsgServer::onConnected, this, std::placeholders::_1));
	MsgServer->start(2);
}

void MsgServer::uninit()
{
	if(MsgServer)
		MsgServer->stop();
}

void MsgServer::onConnected(std::shared_ptr<TcpConnection> conn)
{
	if(conn->connected)
	{
        LOGI("login connected: %s", conn->peerAddress().toIpPort().c_str());
        std::shared_ptr<MsgConn> spSession(new MsgConn(conn));
        conn->setMessageCallback(std::bind(&MsgConn::onRead, spSession.get(),
        		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        conn->setConnectionCallback(std::bind(&MsgConn::onConnected, this));//TODO

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

void MsgServer::onDisconnected(const std::shared_ptr<TcpConnection>& conn)
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




