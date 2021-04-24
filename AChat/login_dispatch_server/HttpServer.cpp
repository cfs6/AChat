#include "HttpServer.h"
#include "../net/InetAddress.h"
#include "../base/AsyncLog.h"
#include "../base/Singleton.h"
//#include "../net/eventloop.h"
#include "../net/EventLoopThread.h"
#include "../net/EventLoopThreadPool.h"
#include "HttpConn.h"
#include "HttpServer.h"

bool HttpServer::init(const char* ip, short port, EventLoop* loop)
{
    InetAddress addr(ip, port);
    tcpServer.reset(new TcpServer(loop, addr, "ZYL-MYHTTPSERVER", TcpServer::kReusePort));
    tcpServer->setConnectionCallback(std::bind(&HttpServer::onConnected, this, std::placeholders::_1));
    tcpServer->start();

    return true;
}

void HttpServer::uninit()
{
    if (tcpServer)
        tcpServer->stop();
}

void HttpServer::onConnected(std::shared_ptr<TcpConnection> conn)
{
    if (conn->connected())
    {
        std::shared_ptr<HttpConn> spSession(new HttpConn(conn));
        conn->setMessageCallback(std::bind(&HttpConn::onRead, spSession.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        {
            std::lock_guard<std::mutex> guard(connMutex);
            httpConnList.push_back(spSession);
        }
    }
    else
    {
        onDisconnected(conn);
    }
}

void HttpServer::onDisconnected(const std::shared_ptr<TcpConnection>& conn)
{
    std::lock_guard<std::mutex> guard(connMutex);
    for (auto iter = httpConnList.begin(); iter != httpConnList.end(); ++iter)
    {
        if ((*iter)->getConnectionPtr() == NULL)
        {
            LOGE("connection is NULL");
            break;
        }

        if ((*iter)->getConnectionPtr() == conn)
        {
            httpConnList.erase(iter);
            LOGI("monitor client disconnected: %s", conn->peerAddress().toIpPort().c_str());
            break;
        }
    }
}
