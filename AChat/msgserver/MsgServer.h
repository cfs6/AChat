#ifndef MSGSERVER_H_
#define MSGSERVER_H_

#include "../net/TcpServer.h"
#include <memory>
#include <list>
#include <map>
#include <mutex>
#include <atomic>
#include "../net/TcpConnection.h"

using namespace std;
using namespace net;
class MsgConn;
class MsgServer
{
public:
	MsgServer() = default;
    ~MsgServer() = default;

    MsgServer(const MsgServer& rhs) = delete;
    MsgServer& operator =(const MsgServer& rhs) = delete;

    bool init(const char* ip, short port, EventLoop* loop);
    void uninit();

    static MsgServer* getInstance();

private:
    //新连接到来调用或连接断开，所以需要通过conn->connected()来判断，一般只在主loop里面调用
    void onConnected(shared_ptr<TcpConnection> conn);
    //连接断开
    void onDisconnected(const std::shared_ptr<TcpConnection>& conn);


private:
    std::unique_ptr<TcpServer>                     MsgServer;
    std::list<std::shared_ptr<MsgConn>>            MsgConnList;
    std::mutex                                     connMutex;         //多线程之间保护m_sessions
    static MsgServer*                              MsgServerInstance;

};

#endif /* MSGSERVER_H_ */
