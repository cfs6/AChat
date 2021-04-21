#ifndef MSGCONN_H_
#define MSGCONN_H_

#include "ImConn.h"

#define KICK_FROM_ROUTE_SERVER 		1
#define MAX_ONLINE_FRIEND_CNT		100	//通知好友状态通知的最多个数

typedef struct {
	uint32_t msg_id;
	uint32_t from_id;
	uint64_t timestamp;
} msg_ack_t;

class ImUser;

class MsgConn : public ImConn
{
public:
	MsgConn();
	virtual ~MsgConn();

    string GetLoginName() { return m_login_name; }
    uint32_t GetUserId() { return m_user_id; }
    void SetUserId(uint32_t user_id) { m_user_id = user_id; }
    uint32_t GetHandle() { return sockfd; }
    uint16_t GetPduVersion() { return m_pdu_version; }
    uint32_t GetClientType() { return m_client_type; }
    uint32_t GetClientTypeFlag();
    void SetOpen() { m_bOpen = true; }
    bool IsOpen() { return m_bOpen; }
    void SetKickOff() { m_bKickOff = true; }
    bool IsKickOff() { return m_bKickOff; }
    void SetOnlineStatus(uint32_t status) { m_online_status = status; }
    uint32_t GetOnlineStatus() { return m_online_status; }
    
    void onConnected();
	TcpConnection getConn()
	{
		if(tcpConn == nullptr)
		{
			return nullptr;
		}
		return tcpConn;
	}
    void SendUserStatusUpdate(uint32_t user_status);

	virtual void Close(bool kick_user = false);

	virtual void OnConnect(net_handle_t handle);
	virtual void OnClose();
	virtual inline void OnTimer(uint64_t curr_tick);

	virtual void HandlePdu(ImPdu* pPdu);

	void AddToSendList(uint32_t msg_id, uint32_t from_id);
	void DelFromSendList(uint32_t msg_id, uint32_t from_id);
private:
    void _HandleHeartBeat(ImPdu* pPdu);
	void _HandleLoginRequest(ImPdu* pPdu);
    void _HandleLoginOutRequest(ImPdu* pPdu);
    void _HandleClientRecentContactSessionRequest(ImPdu* pPdu);
	void _HandleClientMsgData(ImPdu* pPdu);
	void _HandleClientMsgDataAck(ImPdu* pPdu);
	void _HandleClientTimeRequest(ImPdu* pPdu);
    void _HandleClientGetMsgListRequest(ImPdu* pPdu);
    void _HandleClientGetMsgByMsgIdRequest(ImPdu* pPdu);
	void _HandleClientUnreadMsgCntRequest(ImPdu* pPdu);
	void _HandleClientMsgReadAck(ImPdu* pPdu);
    void _HandleClientGetLatestMsgIDReq(ImPdu* pPdu);
	void _HandleClientP2PCmdMsg(ImPdu* pPdu);
	void _HandleClientUserInfoRequest(ImPdu* pPdu);
	void _HandleClientUsersStatusRequest(ImPdu* pPdu);
	void _HandleClientRemoveSessionRequest(ImPdu* pPdu);
	void _HandleClientAllUserRequest(ImPdu* pPdu);
    void _HandleChangeAvatarRequest(ImPdu* pPdu);
    void _HandleChangeSignInfoRequest(ImPdu* pPdu);

    void _HandleClientDeviceToken(ImPdu* pPdu);
    void _HandleKickPCClient(ImPdu* pPdu);
    void _HandleClientDepartmentRequest(ImPdu* pPdu);
    void _SendFriendStatusNotify(uint32_t status);
    void _HandlePushShieldRequest(ImPdu* pPdu);
    void _HandleQueryPushShieldRequest(ImPdu* pPdu);
private:
    string          m_login_name;
    uint32_t        m_user_id;
    bool			m_bOpen;
    bool            m_bKickOff;
    uint64_t		m_login_time;
    uint32_t		m_last_seq_no;
    uint16_t		m_pdu_version;
    string 			m_client_version;
    list<msg_ack_t>	m_send_msg_list;
    uint32_t		m_msg_cnt_per_sec;
    uint32_t        m_client_type;        //客户端登录方式
    uint32_t        m_online_status;      //在线状态 1-online, 2-off-line, 3-leave
    TimerId    	    m_checkOnlineTimerId;
};

void init_msg_conn();

#endif /* MSGCONN_H_ */
