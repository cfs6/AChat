#ifndef MSGCONN_H_
#define MSGCONN_H_

#include "../net/Buffer.h"
#include "../base/Timestamp.h"
#include "../net/TcpConnection.h"
//#include "TcpSession.h"
#include <memory>

using namespace net;

typedef struct  {
	uint32_t    msgServerId;
    string		ip_addr1;	// 电信IP
    string		ip_addr2;	// 网通IP
    uint16_t	port;
    uint32_t	max_conn_cnt;
    uint32_t	cur_conn_cnt;
    string 		hostname;	// 消息服务器的主机名
} msg_serv_info;

class MsgConn
{
public:
    MsgConn(std::shared_ptr<TcpConnection>& conn);
    ~MsgConn() = default;
    MsgConn(const MsgConn& rhs) = delete;
    MsgConn& operator =(const MsgConn& rhs) = delete;

public:
    void onRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receivTime);

    std::shared_ptr<TcpConnection> getConnectionPtr()
    {
        if (tmpConn.expired())
            return NULL;

        return tmpConn.lock();
    }

    void send(const char* data, size_t length);


    int32_t getMsgServerId()
    {
        return msgServerInfo.msgServerId;
    }

    std::string getMsgServername()
    {
        return msgServerInfo.hostname;
    }

private:
    bool process(const std::shared_ptr<TcpConnection>& conn, const std::string& url, const std::string& param);
    void makeupResponse(const std::string& input, std::string& output);

    void HandlePdu(ImPdu* pPdu);
    void Close();
    void checkHeartbeat(const std::shared_ptr<TcpConnection>& conn);

    void handleMsgServInfo(ImPdu* pPdu);
    void handleUserCntUpdate(ImPdu* pPdu);
    void handleMsgServRequest(ImPdu* pPdu);
private:
    std::weak_ptr<TcpConnection>      		 tmpConn;
    static int                        		 totalOnlineUserCnt;

    msg_serv_info                     		 msgServerInfo;
    static map<uint32_t, msg_serv_info*> 	 msgServerInfos;

    time_t                          		 lastPackageTime;
    TimerId                                  checkOnlineTimerId;
};



#endif /* MSGCONN_H_ */
