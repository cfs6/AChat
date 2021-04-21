#ifndef LOGINCONN_H_
#define LOGINCONN_H_

#include "ImConn.h"

enum {
	LOGIN_CONN_TYPE_CLIENT = 1,
	LOGIN_CONN_TYPE_MSG_SERV
};

typedef struct  {
    string		ip_addr1;	// 电信IP
    string		ip_addr2;	// 网通IP
    uint16_t	port;
    uint32_t	max_conn_cnt;
    uint32_t	cur_conn_cnt;
    string 		hostname;	// 消息服务器的主机名
} msg_serv_info_t;


class LoginConn : public ImConn
{
public:
	LoginConn();
	virtual ~LoginConn();

	TcpConnection getConn()
	{
		if(tcpConn == nullptr)
		{
			return nullptr;
		}
		return tcpConn;
	}

	void

	void send();//todo
	void OnConnect2(net_handle_t handle, int conn_type);
	void Close(/*const TcpConnection& tcpConn*/);
	void OnClose();

	void login_conn_timer_callback();
	void loginConnCheck();
	void checkTimeout(uint64_t curr_tick);


	virtual void HandlePdu(ImPdu* pPdu);
private:
	void _HandleMsgServInfo(ImPdu* pPdu);
	void _HandleUserCntUpdate(ImPdu* pPdu);
	void _HandleMsgServRequest(ImPdu* pPdu);

private:
	int	m_conn_type;
	TimerId      m_checkOnlineTimerId;

	static connMap g_client_conn_map;
	static connMap g_msg_serv_conn_map;
};

//void init_login_conn();

#endif /* LOGINCONN_H_ */
