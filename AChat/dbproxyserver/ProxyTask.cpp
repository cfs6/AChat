#include "ProxyTask.h"
#include "ProxyConn.h"
#include "AsyncLog.h"
ProxyTask::ProxyTask(uint32_t conn_UUid, pduHandlerFunc pdu_handler, CImPdu* pPdu):
					connUUid(conn_UUid), pduHandler(pdu_handler)\
{
	pdu.reset(new CImPdu());
}

ProxyTask::~ProxyTask()
{

}


void ProxyTask::run()
{
	if(!pdu)
	{
		LOGE("run proxytask fail! pdu==nullptr ");
		// tell CProxyConn to close connection with m_conn_uuid
		CProxyConn::AddResponsePdu(m_conn_uuid, NULL);
	}
	else
	{
		if(pduHandler)
		{
			pduHandler(pdu, connUUid);
		}
	}
}


