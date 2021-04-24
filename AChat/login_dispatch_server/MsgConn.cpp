#include "MsgConn.h"
#include <sstream>
#include <string.h>
#include <vector>
#include "../net/EventLoopThread.h"
#include "../base/AsyncLog.h"
#include "../base/Singleton.h"
#include "../utils/StringUtil.h"
#include "../utils/URLEncodeUtil.h"
#include "UserManager.h"
#include "ImConn.h"
using namespace IM::BaseDefine;
#define MAX_URL_LENGTH 2048
#define MAX_NO_PACKAGE_INTERVAL  30

static map<uint32_t, msg_serv_info*> 	 msgServerInfos;
static int                         		 totalOnlineUserCnt;
MsgConn::MsgConn(std::shared_ptr<TcpConnection>& conn) : tmpConn(conn)
{

}

void MsgConn::onRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receivTime)
{
    //LOGI << "Recv a http request from " << conn->peerAddress().toIpPort();

    string inbuf;
    inbuf.append(pBuffer->peek(), pBuffer->readableBytes());
    if (inbuf.length() <= 4)
        return;



    conn->runEvery(15000000, std::bind(&MsgConn::checkHeartbeat,this, conn));











    if (!process(conn, url, param))
    {
        LOGE("handle http request error, from: %s, request: %s", conn->peerAddress().toIpPort().c_str(), pBuffer->retrieveAllAsString().c_str());
    }

    conn->forceClose();
}

void MsgConn::send(const char* data, size_t length)
{
    if (!tmpConn.expired())
    {
        std::shared_ptr<TcpConnection> conn = tmpConn.lock();
        conn->send(data, length);
    }
}

bool MsgConn::process(const std::shared_ptr<TcpConnection>& conn, const std::string& url, const std::string& param)
{
    if (url.empty())
        return false;

    if (url == "/register.do")
    {
        onRegisterResponse(param, conn);
    }
    else if (url == "/login.do")
    {
        onLoginResponse(param, conn);
    }
    else if (url == "/getfriendlist.do")
    {

    }
    else if (url == "/getgroupmembers.do")
    {

    }
    else
        return false;


    return true;
}

void MsgConn::makeupResponse(const std::string& input, std::string& output)
{
    std::ostringstream os;
    //TODO
    output = os.str();
}


void MsgConn::Close()
{
	tmpConn->forceClose();
	LOGD("MsgConn::Close()");
}

void MsgConn::HandlePdu(ImPdu* pPdu)
{
	switch (pPdu->GetCommandId())
	{
        case CID_OTHER_HEARTBEAT:
            break;
        case CID_OTHER_MSG_SERV_INFO:
        	handleMsgServInfo(pPdu);
            break;
        case CID_OTHER_USER_CNT_UPDATE:
        	handleUserCntUpdate(pPdu);
            break;

        default:
            LOGE("wrong msg, cmd id=%d ", pPdu->GetCommandId());
            break;
	}
}

//TODO 发心跳包 / 检测心跳超时？
void MsgConn::checkHeartbeat(const std::shared_ptr<TcpConnection>& conn)
{
	if(!conn)
	{
		LOGE("current conn expire, check heartbeat fail");
		return;
	}
	uint64_t currTick = get_tick_count();
	if(currTick - lastSendTick > MAX_NO_PACKAGE_INTERVAL)
	{
        IM::Other::IMHeartBeat msg;
        ImPdu pdu;
        pdu.SetPBMsg(&msg);
        pdu.SetServiceId(SID_OTHER);
        pdu.SetCommandId(CID_OTHER_HEARTBEAT);
		sendPdu(&pdu);
	}
	if (currTick - lastRecvTick > SERVER_TIMEOUT)
	{
		LOGE("connection to MsgServer timeout ");
		Close();
	}
}


void MsgConn::handleMsgServInfo(ImPdu* pPdu)
{
	msg_serv_info* pMsgServInfo = new msg_serv_info;
    IM::Server::IMMsgServInfo msg;
    msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

    pMsgServInfo->msgServerId = msg.serverid();//TODO
	pMsgServInfo->ip_addr1 = msg.ip1();
	pMsgServInfo->ip_addr2 = msg.ip2();
	pMsgServInfo->port = msg.port();
	pMsgServInfo->max_conn_cnt = msg.max_conn_cnt();
	pMsgServInfo->cur_conn_cnt = msg.cur_conn_cnt();
	pMsgServInfo->hostname = msg.host_name();

	msgServerInfos.insert(make_pair(msgServerId,pMsgServInfo));
	totalOnlineUserCnt += pMsgServInfo->cur_conn_cnt;

	LOGD("MsgServInfo, ip_addr1=%s, ip_addr2=%s, port=%d, max_conn_cnt=%d, cur_conn_cnt=%d, "\
		"hostname: %s. ",
		pMsgServInfo->ip_addr1.c_str(), pMsgServInfo->ip_addr2.c_str(), pMsgServInfo->port,pMsgServInfo->max_conn_cnt,
		pMsgServInfo->cur_conn_cnt, pMsgServInfo->hostname.c_str());
}


void MsgConn::handleUserCntUpdate(ImPdu* pPdu)
{
//	map<uint32_t, msg_serv_info_t*>::iterator it = g_msg_serv_info.find(m_handle);
	//TODO 协议需要更新
	uint32_t msgServerid = getMsgServerId();
	auto it = msgServerInfos.find(msgServerid);
	if (it != msgServerInfos.end())
	{
		msg_serv_info* pMsgServInfo = it->second;
        IM::Server::IMUserCntUpdate msg;
        msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

		uint32_t action = msg.user_action();
		if (action == USER_CNT_INC)
		{
			pMsgServInfo->cur_conn_cnt++;
			totalOnlineUserCnt++;
		}
		else
		{
			pMsgServInfo->cur_conn_cnt--;
			totalOnlineUserCnt--;
		}

		LOGE("%s:%d, cur_cnt=%u, total_cnt=%u ", pMsgServInfo->hostname.c_str(),
            pMsgServInfo->port, pMsgServInfo->cur_conn_cnt, totalOnlineUserCnt);
	}
}
