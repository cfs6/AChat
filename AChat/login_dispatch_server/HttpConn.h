#ifndef __HTTP_CONN_H__
#define __HTTP_CONN_H__

#include "util.h"
#include "HttpParserWrapper.h"
#include <string>
using namespace net;
#define HTTP_CONN_TIMEOUT			60000

#define READ_BUF_SIZE	2048
#define HTTP_RESPONSE_HTML          "HTTP/1.1 200 OK\r\n"\
                                    "Connection:close\r\n"\
                                    "Content-Length:%d\r\n"\
                                    "Content-Type:text/html;charset=utf-8\r\n\r\n%s"
#define HTTP_RESPONSE_HTML_MAX      1024

enum {
    CONN_STATE_IDLE,
    CONN_STATE_CONNECTED,
    CONN_STATE_OPEN,
    CONN_STATE_CLOSED,
};

class HttpConn
{
public:
	HttpConn();
	virtual ~HttpConn();

	TcpConnection getConn()
	{
		if(tcpConn == nullptr)
		{
			return nullptr;
		}
		return tcpConn;
	}

	uint32_t GetConnHandle() { return connHandle; }
	char* GetPeerIP() { return (char*)peerIp.c_str(); }

	int Send(void* data, int len);

    void Close();
    void OnConnect(SOCKET handle);
    void OnRead();
    void OnWrite();
    void OnClose();
    void OnTimer(uint64_t curr_tick);
    void OnWriteComlete();

    void http_conn_timer_callback();
    void httpConnCheck();
private:
    void _HandleMsgServRequest(std::string& url, std::string& post_data);

protected:
	SOCKET	           sockfd;
	uint32_t	  	   connHandle;
	bool		  	   busy;

    uint32_t           state;
	std::string		   peerIp;
	uint16_t		   peerPort;
	Buffer	           inBuf;
	Buffer	           outBuf;

	uint64_t		   lastSendTick;
	uint64_t		   lastRecvTick;

	TimerId            m_checkOnlineTimerId;
	TcpConnection      tcpConn;
    CHttpParserWrapper m_cHttpParser;
};

typedef hash_map<uint32_t, HttpConn*> HttpConnMap_t;

HttpConn* FindHttpConnByHandle(uint32_t handle);
//void init_http_conn();

#endif
