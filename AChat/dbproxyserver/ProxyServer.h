#ifndef PROXYSERVER_H_
#define PROXYSERVER_H_
#include "ProxyConn.h"


class ProxyServer final
{
public:
    ProxyServer() = default;
    ~ProxyServer() = default;

    ProxyServer(const ProxyServer& rhs) = delete;
    ProxyServer& operator =(const ProxyServer& rhs) = delete;

    bool init(const char* ip, short port, EventLoop* loop);
    void uninit();

    static ProxyServer* getInstance();

private:
    //新连接到来调用或连接断开，所以需要通过conn->connected()来判断，一般只在主loop里面调用
    void onConnected(std::shared_ptr<TcpConnection> conn);
    //连接断开
    void onDisconnected(const std::shared_ptr<TcpConnection>& conn);


private:
    std::unique_ptr<TcpServer>                     proxyServer;
    std::list<std::shared_ptr<ProxyConn>>          proxyConnList;
    std::mutex                                     connMutex; //多线程之间保护m_sessions
    static ProxyServer*                            proxyServerInstance;
//    static HandlerMap*                             handlerMap;
};


#endif /* PROXYSERVER_H_ */


