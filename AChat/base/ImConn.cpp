#include "ImConn.h"
#include "AsyncLog.h"
ImConn::ImConn():sockfd(-1), busy(false), recvByte(0), lastSendTick(get_tick_count()),
				 lastRecvTick(get_tick_count())
{

}

ImConn::~ImConn()
{

}

int ImConn::Send(void* data, int len)
{
	lastSendTick = get_tick_count();

	if(busy)
	{
		outBuf.writableBytes();
	}

	tcpConn.send(data, len);


	onWriteCompelete();
	return len;
}

void ImConn::onRead()
{

}

static ImConn* FindImConn(connMap* imconn_map, SOCKET handle)
{
	ImConn* pConn = NULL;
	connMap::iterator iter = imconn_map->find(handle);
	if (iter != imconn_map->end())
	{
		pConn = iter->second;
	}

	return pConn;
}

void imconn_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	NOTUSED_ARG(handle);
	NOTUSED_ARG(pParam);

	if (!callback_data)
		return;

	connMap* conn_map = (connMap*)callback_data;
	ImConn* pConn = FindImConn(conn_map, handle);
	if (!pConn)
		return;

	//log("msg=%d, handle=%d ", msg, handle);

	switch (msg)
	{
	case NETLIB_MSG_CONFIRM:
		pConn->onConfirm();
		break;
	case NETLIB_MSG_READ:
		pConn->onRead();
		break;
	case NETLIB_MSG_WRITE:
		pConn->onWrite();
		break;
	case NETLIB_MSG_CLOSE:
		pConn->onClose();
		break;
	default:
		LOGE("!!!imconn_callback error msg: %d ", msg);
		break;
	}

}

//////////////////////////
ImConn::ImConn()
{
	//log("ImConn::ImConn ");

	busy = false;
	sockfd = NETLIB_INVALID_HANDLE;
	recvByte = 0;

	lastSendTick = lastRecvTick = get_tick_count();
}

ImConn::~ImConn()
{
	//log("ImConn::~ImConn, handle=%d ", m_handle);
}

int ImConn::Send(void* data, int len)
{
	lastSendTick = get_tick_count();
//	++g_send_pkt_cnt;

	if (busy)
	{
		outBuf.Write(data, len);
		return len;
	}

	int offset = 0;
	int remain = len;
	while (remain > 0) {
		int send_size = remain;
		if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE) {
			send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
		}

		int ret = tcpConn.send((char*)data + offset , send_size);

		offset += ret;
		remain -= ret;
	}

	if (remain > 0)
	{
		outBuf.Write((char*)data + offset, remain);
		busy = true;
		LOGE("send busy, remain=%d ", outBuf.GetWriteOffset());
	}
    else
    {
        onWriteCompelete();
    }

	return len;
}

void ImConn::onRead()
{

    ImPdu* pPdu = NULL;
	try
    {
		while ( ( pPdu = ImPdu::ReadPdu(inBuf.GetBuffer(), inBuf.GetWriteOffset()) ) )
		{
            uint32_t pdu_len = pPdu->GetLength();

			HandlePdu(pPdu);

			inBuf.Read(NULL, pdu_len);
			delete pPdu;
            pPdu = NULL;
//			++g_recv_pkt_cnt;
		}
	} catch (PduException& ex) {
		LOGE("!!!catch exception, sid=%u, cid=%u, err_code=%u, err_msg=%s, close the connection ",
				ex.GetServiceId(), ex.GetCommandId(), ex.GetErrorCode(), ex.GetErrorMsg());
        if (pPdu) {
            delete pPdu;
            pPdu = NULL;
        }
        OnClose();
	}
}

void ImConn::onWrite()
{
	if (!busy)
		return;

	while (outBuf.GetWriteOffset() > 0) {
		int send_size = outBuf.GetWriteOffset();
		if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE) {
			send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
		}

		int ret = tcpConn.send(outBuf.beginWrite(), send_size);
		if (ret <= 0) {
			ret = 0;
			break;
		}

		outBuf.Read(NULL, ret);
	}

	if (outBuf.GetWriteOffset() == 0) {
		busy = false;
	}

	LOGE("onWrite, remain=%d ", outBuf.GetWriteOffset());
}


