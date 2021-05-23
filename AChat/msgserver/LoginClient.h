
#ifndef LOGINCLIENT_H_
#define LOGINCLIENT_H_

#include "../net/TcpClient.h"
#include <memory>
#include <list>
#include <map>
#include <mutex>
#include <atomic>
#include "../net/TcpConnection.h"

using namespace std;
using namespace net;
class MsgConn;
class LoginClient
{
public:
	LoginClient() = default;
    ~LoginClient() = default;

    LoginClient(const LoginClient& rhs) = delete;
    LoginClient& operator =(const LoginClient& rhs) = delete;

    bool init(const char* ip, short port, EventLoop* loop);
    void uninit();

    static LoginClient* getInstance();

private:
    void onConnected(shared_ptr<TcpConnection> conn);
    //连接断开
    void onDisconnected(const std::shared_ptr<TcpConnection>& conn);


private:
    std::unique_ptr<TcpClient>                     loginClient;
//    std::list<std::shared_ptr<MsgConn>>            MsgConnList;
    std::shared_ptr<LoginServConn>                 loginServConn;
    std::mutex                                     connMutex;         //多线程之间保护m_sessions
    static LoginClient*                            loginClientInstance;

};



#endif /* LOGINCLIENT_H_ */
