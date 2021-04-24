
#ifndef MsgdispatchServer_H_
#define MsgdispatchServer_H_

#include <memory>
#include <mutex>
#include <list>
#include "../net/EventLoop.h"
#include "../net/TcpServer.h"

using namespace net;


class MsgConn;

class MsgdispatchServer final
{
public:
    MsgdispatchServer() = default;
    ~MsgdispatchServer() = default;

    MsgdispatchServer(const MsgdispatchServer& rhs) = delete;
    MsgdispatchServer& operator =(const MsgdispatchServer& rhs) = delete;

    static MsgdispatchServer* getInstance();
public:
    bool init(const char* ip, short port, EventLoop* loop);
    void uninit();

    void onConnected(std::shared_ptr<TcpConnection> conn);
    void onDisconnected(const std::shared_ptr<TcpConnection>& conn);

private:
    std::unique_ptr<TcpServer>                     tcpServer;
    TcpServer									   tmpServer;
    std::list<std::shared_ptr<MsgConn>>            MsgConnList;
    std::mutex                                     connMutex;

    static MsgdispatchServer*                      MsgdispatchServerInstance;
};



#endif /* MsgdispatchServer_H_ */
