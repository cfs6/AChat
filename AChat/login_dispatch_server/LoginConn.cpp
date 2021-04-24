#include "LoginConn.h"
#include "IM.Server.pb.h"
#include "IM.Other.pb.h"
#include "IM.Login.pb.h"
#include "public_define.h"
#include "AsyncLog.h"
#include "net/TcpConnection.h"
using namespace net;
using namespace IM::BaseDefine;
static connMap g_client_conn_map;
static connMap g_msg_serv_conn_map;
static uint32_t g_total_online_user_cnt = 0;	  // 并发在线总人数
//map<uint32_t, msg_serv_info_t*> g_msg_serv_info;
//set<msg_serv_info_t> msgServerInfos;
void LoginConn::login_conn_timer_callback()
{
	uint64_t cur_time = get_tick_count();
	for (connMap::iterator it = g_client_conn_map.begin(); it != g_client_conn_map.end(); )
	{
		connMap::iterator it_old = it;
		it++;

		LoginConn* pConn = (LoginConn*)it_old->second;
		pConn->checkTimeout(cur_time);
	}

	for (connMap::iterator it = g_msg_serv_conn_map.begin(); it != g_msg_serv_conn_map.end(); )
	{
		connMap::iterator it_old = it;
		it++;

		LoginConn* pConn = (LoginConn*)it_old->second;
		pConn->checkTimeout(cur_time);
	}
}
//
void LoginConn::loginConnCheck()
{
	TcpConnection conn = getConn();
    if (conn)
    {
        m_checkOnlineTimerId = conn.getLoop()->runEvery(1000, std::bind(&LoginConn::login_conn_timer_callback, this));
    }
}

//void init_login_conn()
//{
//	netlib_register_timer(login_conn_timer_callback, NULL, 1000);
//}

LoginConn::LoginConn()
{
}

LoginConn::~LoginConn()
{

}

void LoginConn::Close(/*const TcpConnection& tcpConn*/)
{
	if (sockfd != NETLIB_INVALID_HANDLE)
	{
//		netlib_close(m_handle);
		tcpConn.forceClose();
		if (m_conn_type == LOGIN_CONN_TYPE_CLIENT)
		{
			g_client_conn_map.erase(sockfd);
		}
		else
		{
			g_msg_serv_conn_map.erase(sockfd);

			// remove all user count from this message server
			map<uint32_t, msg_serv_info_t*>::iterator it = g_msg_serv_info.find(sockfd);
			if (it != g_msg_serv_info.end())
			{
				msg_serv_info_t* pMsgServInfo = it->second;

				g_total_online_user_cnt -= pMsgServInfo->cur_conn_cnt;
				LOGD("onclose from MsgServer: %s:%u ", pMsgServInfo->hostname.c_str(), pMsgServInfo->port);
				delete pMsgServInfo;
				g_msg_serv_info.erase(it);
			}
		}
	}

//	ReleaseRef();
}

void LoginConn::OnConnect2(SOCKET handle, int conn_type)
{
	sockfd = handle;
	m_conn_type = conn_type;
	connMap* conn_map = &g_msg_serv_conn_map;
	if (conn_type == LOGIN_CONN_TYPE_CLIENT)
	{
		conn_map = &g_client_conn_map;
	}

	conn_map->insert(make_pair(handle, this));

	tcpConn.setConnectionCallback(imconn_callback);

//	netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void*)imconn_callback);
//	netlib_option(handle, NETLIB_OPT_SET_CALLBACK_DATA, (void*)conn_map);
}

void LoginConn::OnClose()
{
	Close(tcpConn);
}

void LoginConn::checkTimeout(uint64_t curr_tick)
{
	if (m_conn_type == LOGIN_CONN_TYPE_CLIENT)
	{
		if (curr_tick > lastRecvTick + CLIENT_TIMEOUT)
		{
			Close(tcpConn);
		}
	}
	else
	{
		if (curr_tick > lastSendTick + SERVER_HEARTBEAT_INTERVAL)
		{
            IM::Other::IMHeartBeat msg;
            ImPdu pdu;
            pdu.SetPBMsg(&msg);
            pdu.SetServiceId(SID_OTHER);
            pdu.SetCommandId(CID_OTHER_HEARTBEAT);
			sendPdu(&pdu);
		}

		if (curr_tick > lastRecvTick + SERVER_TIMEOUT)
		{
			LOGE("connection to MsgServer timeout ");
			Close(tcpConn);
		}
	}
}

void LoginConn::HandlePdu(ImPdu* pPdu)
{
	switch (pPdu->GetCommandId()) {
        case CID_OTHER_HEARTBEAT:
            break;
        case CID_OTHER_MSG_SERV_INFO:
            _HandleMsgServInfo(pPdu);
            break;
        case CID_OTHER_USER_CNT_UPDATE:
            _HandleUserCntUpdate(pPdu);
            break;
//        case CID_LOGIN_REQ_MSGSERVER:
//            _HandleMsgServRequest(pPdu);
//            break;

        default:
            LOGE("wrong msg, cmd id=%d ", pPdu->GetCommandId());
            break;
	}
}

void LoginConn::_HandleMsgServInfo(ImPdu* pPdu)
{
	msg_serv_info_t* pMsgServInfo = new msg_serv_info_t;
    IM::Server::IMMsgServInfo msg;
    msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());
    
	pMsgServInfo->ip_addr1 = msg.ip1();
	pMsgServInfo->ip_addr2 = msg.ip2();
	pMsgServInfo->port = msg.port();
	pMsgServInfo->max_conn_cnt = msg.max_conn_cnt();
	pMsgServInfo->cur_conn_cnt = msg.cur_conn_cnt();
	pMsgServInfo->hostname = msg.host_name();
	g_msg_serv_info.insert(make_pair(m_handle, pMsgServInfo));

	g_total_online_user_cnt += pMsgServInfo->cur_conn_cnt;

	LOGD("MsgServInfo, ip_addr1=%s, ip_addr2=%s, port=%d, max_conn_cnt=%d, cur_conn_cnt=%d, "\
		"hostname: %s. ",
		pMsgServInfo->ip_addr1.c_str(), pMsgServInfo->ip_addr2.c_str(), pMsgServInfo->port,pMsgServInfo->max_conn_cnt,
		pMsgServInfo->cur_conn_cnt, pMsgServInfo->hostname.c_str());
}

void LoginConn::_HandleUserCntUpdate(ImPdu* pPdu)
{
	map<uint32_t, msg_serv_info_t*>::iterator it = g_msg_serv_info.find(sockfd);
	if (it != g_msg_serv_info.end()) {
		msg_serv_info_t* pMsgServInfo = it->second;
        IM::Server::IMUserCntUpdate msg;
        msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

		uint32_t action = msg.user_action();
		if (action == USER_CNT_INC)
		{
			pMsgServInfo->cur_conn_cnt++;
			g_total_online_user_cnt++;
		}
		else
		{
			pMsgServInfo->cur_conn_cnt--;
			g_total_online_user_cnt--;
		}

		LOGD("%s:%d, cur_cnt=%u, total_cnt=%u ", pMsgServInfo->hostname.c_str(),
            pMsgServInfo->port, pMsgServInfo->cur_conn_cnt, g_total_online_user_cnt);
	}
}

void LoginConn::_HandleMsgServRequest(ImPdu* pPdu)
{
    IM::Login::IMMsgServReq msg;
    msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

	LOGD("HandleMsgServReq. ");

	// no MessageServer available
	if (g_msg_serv_info.size() == 0)
	{
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_NO_MSG_SERVER);
        ImPdu pdu;
        pdu.SetPBMsg(&msg);
        pdu.SetServiceId(SID_LOGIN);
        pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
        pdu.SetSeqNum(pPdu->GetSeqNum());
        SendPdu(&pdu);
        Close();
		return;
	}

	// return a message server with minimum concurrent connection count
	msg_serv_info_t* pMsgServInfo;
	uint32_t min_user_cnt = (uint32_t)-1;
	map<uint32_t, msg_serv_info_t*>::iterator it_min_conn = g_msg_serv_info.end(),it;

	for (it = g_msg_serv_info.begin() ; it != g_msg_serv_info.end(); it++) {
		pMsgServInfo = it->second;
		if ( (pMsgServInfo->cur_conn_cnt < pMsgServInfo->max_conn_cnt) &&
			 (pMsgServInfo->cur_conn_cnt < min_user_cnt))
        {
			it_min_conn = it;
			min_user_cnt = pMsgServInfo->cur_conn_cnt;
		}
	}

	if (it_min_conn == g_msg_serv_info.end()) {
		log("All TCP MsgServer are full ");
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_MSG_SERVER_FULL);
        ImPdu pdu;
        pdu.SetPBMsg(&msg);
        pdu.SetServiceId(SID_LOGIN);
        pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
        pdu.SetSeqNum(pPdu->GetSeqNum());
        SendPdu(&pdu);
	}
    else
    {
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_NONE);
        msg.set_prior_ip(it_min_conn->second->ip_addr1);
        msg.set_backip_ip(it_min_conn->second->ip_addr2);
        msg.set_port(it_min_conn->second->port);
        ImPdu pdu;
        pdu.SetPBMsg(&msg);
        pdu.SetServiceId(SID_LOGIN);
        pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
        pdu.SetSeqNum(pPdu->GetSeqNum());
        SendPdu(&pdu);
    }

	Close();	// after send MsgServResponse, active close the connection
}
