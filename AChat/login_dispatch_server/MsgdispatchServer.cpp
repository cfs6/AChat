#include "MsgdispatchServer.h"

#include "../net/InetAddress.h"
#include "../base/AsyncLog.h"
#include "../base/Singleton.h"
//#include "../net/eventloop.h"
#include "../net/EventLoopThread.h"
#include "../net/EventLoopThreadPool.h"
#include "MsgConn.h"
#include "MsgdispatchServer.h"

bool MsgdispatchServer::init(const char* ip, short port, EventLoop* loop)
{
    InetAddress addr(ip, port);
    tcpServer.reset(new TcpServer(loop, addr, "MsgdispatchServer", TcpServer::kReusePort));
    tcpServer->setConnectionCallback(std::bind(&MsgdispatchServer::onConnected, this, std::placeholders::_1));
    tcpServer->start();

    return true;
}

void MsgdispatchServer::uninit()
{
    if (tcpServer)
        tcpServer->stop();
}

void MsgdispatchServer::onConnected(std::shared_ptr<TcpConnection> conn)
{
    if (conn->connected())
    {
        std::shared_ptr<MsgConn> spSession(new MsgConn(conn));
        conn->setMessageCallback(std::bind(&MsgConn::onRead, spSession.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        conn->setConnectionCallback(std::bind(&MsgConn::onConnected, this));
        {
            std::lock_guard<std::mutex> guard(connMutex);
            MsgConnList.push_back(spSession);
        }
//        conn->runEvery(15000000, std::bind(&MsgConn::checkHeartbeat,this, conn));
    }
    else
    {
        onDisconnected(conn);
    }

}

void MsgdispatchServer::onDisconnected(const std::shared_ptr<TcpConnection>& conn)
{
    std::lock_guard<std::mutex> guard(connMutex);
    for (auto iter = MsgConnList.begin(); iter != MsgConnList.end(); ++iter)
    {
        if ((*iter)->getConnectionPtr() == NULL)
        {
            LOGE("connection is NULL");
            break;
        }

        if ((*iter)->getConnectionPtr() == conn)
        {
            MsgConnList.erase(iter);
            LOGI("monitor client disconnected: %s", conn->peerAddress().toIpPort().c_str());
            break;
        }
    }
}


