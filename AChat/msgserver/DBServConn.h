
#ifndef DBSERVCONN_H_
#define DBSERVCONN_H_

#include "ImConn.h"
#include "ServInfo.h"
//#include "RouteServConn.h"

class DBServConn : public ImConn
{
public:
	DBServConn(const std::shared_ptr<TcpConnection>& conn, int connid);
	virtual ~DBServConn();

	bool IsOpen() { return m_bOpen; }

	void Connect(const char* server_ip, uint16_t server_port, uint32_t serv_idx);
	virtual void Close();


    std::shared_ptr<TcpConnection> getConnectionPtr()
    {
        if (dbServConn.expired())
            return NULL;

        return dbServConn.lock();
    }
	void onConnect();
	virtual void OnConfirm();
	virtual void OnClose();
	virtual void OnTimer(uint64_t curr_tick);

	virtual void HandlePdu(ImPdu* pPdu);
private:
	void handleValidateResponse(ImPdu* pPdu);
    void handleRecentSessionResponse(ImPdu* pPdu);
    void handleAllUserResponse(ImPdu* pPdu);
    void handleGetMsgListResponse(ImPdu* pPdu);
    void handleGetMsgByIdResponse(ImPdu* pPdu);
    void handleMsgData(ImPdu* pPdu);
	void handleUnreadMsgCountResponse(ImPdu* pPdu);
    void handleGetLatestMsgIDRsp(ImPdu* pPdu);
	void handleDBWriteResponse(ImPdu* pPdu);
	void handleUsersInfoResponse(ImPdu* pPdu);
	void handleStopReceivePacket(ImPdu* pPdu);
	void handleRemoveSessionResponse(ImPdu* pPdu);
	void handleChangeAvatarResponse(ImPdu* pPdu);
    void handleChangeSignInfoResponse(ImPdu* pPdu);
    void handleSetDeviceTokenResponse(ImPdu* pPdu);
    void handleGetDeviceTokenResponse(ImPdu* pPdu);
    void handleDepartmentResponse(ImPdu* pPdu);
    
    void handlePushShieldResponse(ImPdu* pPdu);
    void handleQueryPushShieldResponse(ImPdu* pPdu);
private:
	bool 		m_bOpen;
//	uint32_t	m_serv_idx;

	std::atomic_int                 dbconnId;
	std::weak_ptr<TcpConnection>    dbServConn;
//	TcpConnection*    dbServConn;
};

void init_db_serv_conn(serv_info_t* server_list, uint32_t server_count, uint32_t concur_conn_cnt);
DBServConn* get_db_serv_conn_for_login();
DBServConn* get_db_serv_conn();

#endif /* DBSERVCONN_H_ */
