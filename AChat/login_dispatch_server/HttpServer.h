#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <memory>
#include <mutex>
#include <list>
#include "../net/EventLoop.h"
#include "../net/TcpServer.h"

using namespace net;

//class EventLoop;
//class TcpConnection;
//class TcpServer;
//class EventLoopThreadPool;

class HttpConn;

class HttpServer final
{
public:
    HttpServer() = default;
    ~HttpServer() = default;

    HttpServer(const HttpServer& rhs) = delete;
    HttpServer& operator =(const HttpServer& rhs) = delete;

    static HttpServer* getInstance();
public:
    bool init(const char* ip, short port, EventLoop* loop);
    void uninit();

    void onConnected(std::shared_ptr<TcpConnection> conn);
    void onDisconnected(const std::shared_ptr<TcpConnection>& conn);

private:
    std::unique_ptr<TcpServer>                     tcpServer;
    std::list<std::shared_ptr<HttpConn>>           httpConnList;
    std::mutex                                     connMutex;

    static HttpServer*                             httpInstance;
};


#endif //!__HTTP_SERVER_H__
