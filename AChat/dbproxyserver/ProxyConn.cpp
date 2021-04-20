#include "ProxyConn.h"
#include "ProxyTask.h"
#include "HandlerMap.h"
#include "atomic.h"
#include "IM.Other.pb.h"
#include "IM.BaseDefine.pb.h"
#include "IM.Server.pb.h"
#include "ThreadPool.h"
#include "SyncCenter.h"
#include "ImPduBase.h"
#include "AsyncLog.h"
#include "HandlerMap.h"
#include "Buffer.h"
list<ResponsePdu*> ProxyConn::responsePduList;
static userMap uuid_conn_map;
ProxyConn::ProxyConn(const std::weak_ptr<TcpConnection>& pConn):
					 connId(0), packageSeq(0)
{

}

ProxyConn::~ProxyConn()
{

}

ProxyConn* get_proxy_conn_by_uuid(uint32_t uuid)
{
	ProxyConn* pConn = NULL;
	userMap::iterator it = uuid_conn_map.find(uuid);
	if (it != uuid_conn_map.end())
	{
		pConn = (ProxyConn *)it->second;
	}

	return pConn;
}

void ProxyConn::onRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer,
    			Timestamp receivTime)
{
    while (true)
    {
/*        //不够一个包头大小
        if (pBuffer->readableBytes() < (size_t)sizeof(file_msg_header))
        {
            //LOGI << "buffer is not enough for a package header, pBuffer->readableBytes()=" << pBuffer->readableBytes() << ", sizeof(msg)=" << sizeof(file_msg);
            return;
        }

        //不够一个整包大小
        file_msg_header header;
        memcpy(&header, pBuffer->peek(), sizeof(file_msg_header));

        //包头有错误，立即关闭连接
        if (header.packagesize <= 0 || header.packagesize > MAX_PACKAGE_SIZE)
        {
            //客户端发非法数据包，服务器主动关闭之
            LOGE("Illegal package header size: %lld, close TcpConnection, client: %s", header.packagesize, conn->peerAddress().toIpPort().c_str());
            LOG_DEBUG_BIN((unsigned char*)&header, sizeof(header));
            conn->forceClose();
            return;
        }


        if (pBuffer->readableBytes() < (size_t)header.packagesize + sizeof(file_msg_header))
            return;*/

        //
        uint32_t pdu_len = 0;
        while ( ImPdu::IsPduAvailable((uchar_t*)pBuffer, pBuffer->writableBytes(), pdu_len) )
        {
            HandlePduBuf(conn, (uchar_t*)pBuffer, pdu_len);
//            pBuffer.Read(NULL, pdu_len);//TODO ?
        }


//        pBuffer->retrieve(sizeof(file_msg_header));
//        std::string inbuf;
//        inbuf.append(pBuffer->peek(), header.packagesize);
//        pBuffer->retrieve(header.packagesize);
//        if (!process(conn, inbuf.c_str(), inbuf.length()))
//        {
//            LOGE("Process error, close TcpConnection, client: %s", conn->peerAddress().toIpPort().c_str());
//            conn->forceClose();
//        }
    }// end while-loop
}

void ProxyConn::HandlePduBuf(const std::shared_ptr<TcpConnection>& conn, uchar_t* pdu_buf, uint32_t pdu_len)
{
    ImPdu* pPdu = NULL;
    pPdu = ImPdu::ReadPdu(pdu_buf, pdu_len);
    if (pPdu->GetCommandId() == IM::BaseDefine::CID_OTHER_HEARTBEAT) {
        return;
    }

    pduHandlerFunc handler = handlerMap->getHandler(pPdu->GetCommandId());
    if (handler)
    {
//        Task* pTask = new ProxyTask(connId, handler, pPdu);
//        threadPool.AddTask(pTask); //TODO

    	//TODO 需要重新加/改 线程池以处理这些连接任务
    	if (!pPdu)
    	{
    		// tell CProxyConn to close connection with m_conn_uuid
    		AddResponsePdu(connId, NULL);
    	}
    	else
    	{
    		if (handler) {
    			handler(pPdu, connId);
    		}
    	}

/*        if (!process(conn, (const char*)pPdu->GetBuffer(), pPdu->GetLength()))
        {
            LOGE("Process error, close TcpConnection, client: %s", conn->peerAddress().toIpPort().c_str());
            conn->forceClose();
        }*/
    }
    else
    {
        LOGE("no handler for packet type: %d", pPdu->GetCommandId());
    }
}

/*bool ProxyConn::process(const std::shared_ptr<TcpConnection>& conn, const char* inbuf, size_t length)
{
	if (!m_pPdu) {
		// tell CProxyConn to close connection with m_conn_uuid
		CProxyConn::AddResponsePdu(m_conn_uuid, NULL);
	} else {
		if (m_pdu_handler) {
			m_pdu_handler(m_pPdu, m_conn_uuid);
		}
	}
}*/

void ProxyConn::AddResponsePdu(uint32_t conn_uuid, ImPdu* pPdu)
{
	ResponsePdu* pResp = new ResponsePdu;
	pResp->connuuid = conn_uuid;
	pResp->pPdu = pPdu;

	{
		unique_lock<mutex> lock(connMutex);
		responsePduList.push_back(pResp);
	}
}



void ProxyConn::SendResponsePduList()
{
	while (!responsePduList.empty())
	{
		ResponsePdu* pResp = responsePduList.front();

		{
			unique_lock<std::mutex> lock(connMutex);
			responsePduList.pop_front();
		}

		ProxyConn* pConn = get_proxy_conn_by_uuid(pResp->connuuid);
		if (pConn)
		{
			if (pResp->pPdu)
			{
				pConn->sendPdu(pResp->pPdu);
			}
			else
			{
				LOGE("close connection uuid=%d by parse pdu error\b", pResp->connuuid);
//				pConn->close();
			}
		}

		if (pResp->pPdu)
			delete pResp->pPdu;
		delete pResp;

	}
}



