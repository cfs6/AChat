#ifndef DBCLIENT_H_
#define DBCLIENT_H_

#include "../net/TcpClient.h"
#include <memory>
#include <list>
#include <map>
#include <mutex>
#include <atomic>
#include <string>
#include "../net/TcpConnection.h"

//using namespace std;
using namespace net;
class DBServConn;
class DBClient
{
public:
	DBClient() = default;
    ~DBClient() = default;

    DBClient(const DBClient& rhs) = delete;
    DBClient& operator =(const DBClient& rhs) = delete;

    bool init(vector<pair<const char*,short>> ipPortVec, vector<EventLoop*> loopVec,
    		vector<string> nameArg);
    void uninit();

    static DBClient* getInstance();

private:
    void onConnected(shared_ptr<TcpConnection> conn);
    void onDisconnected(const std::shared_ptr<TcpConnection>& conn);


private:
//    std::unique_ptr<TcpClient>                   DBClient;
//    std::shared_ptr<DBServConn>  				   DBConn;
    std::list<std::unique_ptr<TcpClient>>          tcpClientList;
    std::list<std::shared_ptr<DBServConn>>         DBConnList;
    std::atomic_int                                dbConnId{};
    std::mutex                                     connMutex;         //多线程之间保护m_sessions
    static DBClient*                               DBClientInstance;

};



#endif /* DBCLIENT_H_ */
