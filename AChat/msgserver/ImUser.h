#ifndef IMUSER_H_
#define IMUSER_H_

#include "ImConn.h"
#include "public_define.h"
#define MAX_ONLINE_FRIEND_CNT		100	//通知好友状态通知的最多个数

class MsgConn;

class ImUser
{
public:
    ImUser(string user_name);
    ~ImUser();
    
    void SetUserId(uint32_t user_id) { m_user_id = user_id; }
    uint32_t GetUserId() { return m_user_id; }
    string GetLoginName() { return m_login_name; }
    void SetNickName(string nick_name) { m_nick_name = nick_name; }
    string GetNickName() { return m_nick_name; }
    bool IsValidate() { return m_bValidate; }
    void SetValidated() { m_bValidate = true; }
    uint32_t GetPCLoginStatus() { return m_pc_login_status; }
    void SetPCLoginStatus(uint32_t pc_login_status) { m_pc_login_status = pc_login_status; }
    
    
    user_conn_t GetUserConn();
    
    bool IsMsgConnEmpty() { return m_conn_map.empty(); }
    void AddMsgConn(uint32_t handle, MsgConn* pMsgConn) { m_conn_map[handle] = pMsgConn; }
    void DelMsgConn(uint32_t handle) { m_conn_map.erase(handle); }
    MsgConn* GetMsgConn(uint32_t handle);
    void ValidateMsgConn(uint32_t handle, MsgConn* pMsgConn);
    
    void AddUnValidateMsgConn(MsgConn* pMsgConn) { m_unvalidate_conn_set.insert(pMsgConn); }
    void DelUnValidateMsgConn(MsgConn* pMsgConn) { m_unvalidate_conn_set.erase(pMsgConn); }
    MsgConn* GetUnValidateMsgConn(uint32_t handle);
    
    map<uint32_t, MsgConn*>& GetMsgConnMap() { return m_conn_map; }

    void BroadcastPdu(ImPdu* pPdu, MsgConn* pFromConn = NULL);
    void BroadcastPduWithOutMobile(ImPdu* pPdu, MsgConn* pFromConn = NULL);
    void BroadcastPduToMobile(ImPdu* pPdu, MsgConn* pFromConn = NULL);
    void BroadcastClientMsgData(ImPdu* pPdu, uint32_t msg_id, MsgConn* pFromConn = NULL, uint32_t from_id = 0);
    void BroadcastData(void* buff, uint32_t len, MsgConn* pFromConn = NULL);
        
    void HandleKickUser(MsgConn* pConn, uint32_t reason);
    
    bool KickOutSameClientType(uint32_t client_type, uint32_t reason, MsgConn* pFromConn = NULL);
    
    uint32_t GetClientTypeFlag();
private:
    uint32_t		m_user_id;
    string			m_login_name;
    string          m_nick_name;
    bool 			m_user_updated;
    uint32_t        m_pc_login_status;  // pc client login状态，1: on 0: off
    
    bool 			m_bValidate;
    
    map<uint32_t /* handle */, MsgConn*>	m_conn_map;
    set<MsgConn*> m_unvalidate_conn_set;
};

typedef map<uint32_t /* user_id */, ImUser*> ImUserMap_t;
typedef map<string /* 登录名 */, ImUser*> ImUserMapByName_t;

class ImUserManager
{
public:
    ImUserManager() {}
    ~ImUserManager();
    
    static ImUserManager* GetInstance();
    ImUser* GetImUserById(uint32_t user_id);
    ImUser* GetImUserByLoginName(string login_name);
    
    MsgConn* GetMsgConnByHandle(uint32_t user_id, uint32_t handle);
    bool AddImUserByLoginName(string login_name, ImUser* pUser);
    void RemoveImUserByLoginName(string login_name);
    
    bool AddImUserById(uint32_t user_id, ImUser* pUser);
    void RemoveImUserById(uint32_t user_id);
    
    void RemoveImUser(ImUser* pUser);
    
    void RemoveAll();
    void GetOnlineUserInfo(list<user_stat_t>* online_user_info);
    void GetUserConnCnt(list<user_conn_t>* user_conn_list, uint32_t& total_conn_cnt);
    
    void BroadcastPdu(ImPdu* pdu, uint32_t client_type_flag);
private:
    ImUserMap_t m_im_user_map;
    ImUserMapByName_t m_im_user_map_by_name;
};

void get_online_user_info(list<user_stat_t>* online_user_info);


#endif /* IMUSER_H_ */
