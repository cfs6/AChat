#include "ImConn.h"

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

	tcpCon.send(data, len);
	//todo  发送失败处理？

	onWriteCompelete();
	return len;
}

void ImConn::onRead()
{

}




