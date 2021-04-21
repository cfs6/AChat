
#ifndef IMCONN_H_
#define IMCONN_H_

#include "util.h"
#include "ImPduBase.h"
#include "net/Buffer.h"
#include "net/TcpConnection.h"
#include <string>
#include <hash_map>

#define SERVER_HEARTBEAT_INTERVAL	5000
#define SERVER_TIMEOUT				30000
#define CLIENT_HEARTBEAT_INTERVAL	30000
#define CLIENT_TIMEOUT				120000
#define MOBILE_CLIENT_TIMEOUT       60000 * 5
#define READ_BUF_SIZE	2048
#define MAX_SOCKET_BUF_SIZE         128*1024
#define NETLIB_MAX_SOCKET_BUF_SIZE		(128 * 1024)
using namespace net;

class ImConn /*: public RefObject*/
{
public:
	ImConn();
	virtual ~ImConn();

	bool isBusy(){return busy; }
	int  sendPdu(ImPdu* pPdu){ return Send(pPdu->GetBuffer(), pPdu->GetLength()); }
	int Send(void* data, int len);

	virtual void onConnect(SOCKET handle) { sockfd = handle; }
	virtual void onConfirm() {}
	virtual void onRead();
	virtual void onWrite();
	virtual void onClose() {}
	virtual void onTimer(uint64_t curr_tick) {}
    virtual void onWriteCompelete() {};

	virtual void handlePdu(ImPdu* pPdu) {}


protected:
	SOCKET               sockfd;
	bool                 busy;
	std::string          peerIp;
	uint16_t             peerPort;

	net::Buffer          inBuf;
	net::Buffer          outBuf;

	bool                 policyConn;
	uint32_t             recvByte;
	uint64_t             lastSendTick;
	uint64_t             lastRecvTick;
	uint64_t             lastAllUserTick;

	TcpConnection        tcpConn;
};

typedef hash_map<SOCKET, ImConn*>  connMap;
typedef hash_map<uint32_t, ImConn*> userMap;





#endif /* IMCONN_H_ */
