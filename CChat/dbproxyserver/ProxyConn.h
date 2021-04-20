
#ifndef PROXYCONN_H_
#define PROXYCONN_H_

#include <curl/curl.h>
#include "../base/util.h"
#include "ImConn.h"
#include "TcpConn.h"

typedef struct {
	uint32_t	connuuid;
	ImPdu*		pPdu;
} ResponsePdu;

class ProxyConn : ImConn
{
public:
//	virtual void onConnect();
	virtual void onRead();
	virtual void onTimer(uint64_t currTick);

	ProxyConn(const std::weak_ptr<TcpConnection>& pConn);
	~ProxyConn();

	ProxyConn(const ProxyConn&) = delete;
	ProxyConn& operator=(const ProxyConn&) = delete;

    std::shared_ptr<TcpConnection> getConnectionPtr()
    {
        if (tcpConn.expired())
            return NULL;

        return tcpConn.lock();
    }

//    void init();

    void send();
    //TODO: 传conn指针？
    void onRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer,
    			Timestamp receivTime);
private:
//    void process();
	void HandlePduBuf(const std::shared_ptr<TcpConnection>& conn, uchar_t* pdu_buf, uint32_t pdu_len);
	bool process(const std::shared_ptr<TcpConnection>& conn, const char* inbuf, size_t length);
	static void AddResponsePdu(uint32_t conn_uuid, ImPdu* pPdu);	// 工作线程调用
	static void SendResponsePduList();	// 主线程调用
    //todo
private:
//	static uint32_t                uuidAllocator;

    int32_t           			   connId;            //session id
    int32_t           			   packageSeq;        //当前Session数据包序列号
	std::mutex                     connMutex;
	static list<ResponsePdu*>      responsePduList;

//	std::weak_ptr<TcpConnection>   tmpConn;
//	unique_ptr<HandlerMap>         handlerMap;
	static HandlerMap*             handlerMap;
};


#endif /* PROXYCONN_H_ */
