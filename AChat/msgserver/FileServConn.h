#ifndef __FileServConn__
#define __FileServConn__

#include <iostream>

#include "ImConn.h"
#include "ServInfo.h"
#include "BaseSocket.h"
#include "IM.BaseDefine.pb.h"
class CFileServConn : public ImConn
{
public:
	CFileServConn();
	virtual ~CFileServConn();
    
	bool IsOpen() { return m_bOpen; }
    
	void Connect(const char* server_ip, uint16_t server_port, uint32_t serv_idx);
	virtual void Close();
    
	virtual void OnConfirm();
	virtual void OnClose();
	virtual void OnTimer(uint64_t curr_tick);
    
	virtual void HandlePdu(ImPdu* pPdu);

    const list<IM::BaseDefine::IpAddr>* GetFileServerIPList() { return &m_ip_list; }
    
private:
    void _HandleFileMsgTransRsp(ImPdu* pPdu);
    void _HandleFileServerIPRsp(ImPdu* pPdu);
    
private:
	bool 		m_bOpen;
	uint32_t	m_serv_idx;
    uint64_t	m_connect_time;
    list<IM::BaseDefine::IpAddr> m_ip_list;
};

void init_file_serv_conn(serv_info_t* server_list, uint32_t server_count);
bool is_file_server_available();
CFileServConn* get_random_file_serv_conn();
#endif /* defined(__FileServConn__) */
