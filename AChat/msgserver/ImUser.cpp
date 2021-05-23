#include "ImUser.h"
#include "MsgConn.h"
//#include "RouteServConn.h"
#include "IM.Server.pb.h"
#include "IM.Login.pb.h"
#include "AsyncLog.h"
using namespace ::IM::BaseDefine;

ImUser::ImUser(string user_name)
{
    //log("ImUser, userId=%u\n", user_id);
    m_login_name = user_name;
    m_bValidate = false;
    m_user_id = 0;
    m_user_updated = false;
    m_pc_login_status = IM::BaseDefine::USER_STATUS_OFFLINE;
}

ImUser::~ImUser()
{
    //log("~ImUser, userId=%u\n", m_user_id);
}

MsgConn* ImUser::GetUnValidateMsgConn(uint32_t handle)
{
    for (set<MsgConn*>::iterator it = m_unvalidate_conn_set.begin(); it != m_unvalidate_conn_set.end(); it++)
    {
        MsgConn* pConn = *it;
        if (pConn->GetHandle() == handle) {
            return pConn;
        }
    }
    
    return NULL;
}

MsgConn* ImUser::GetMsgConn(uint32_t handle)
{
    MsgConn* pMsgConn = NULL;
    map<uint32_t, MsgConn*>::iterator it = m_conn_map.find(handle);
    if (it != m_conn_map.end()) {
        pMsgConn = it->second;
    }
    return pMsgConn;
}

void ImUser::ValidateMsgConn(uint32_t handle, MsgConn* pMsgConn)
{
    AddMsgConn(handle, pMsgConn);
    DelUnValidateMsgConn(pMsgConn);
}


user_conn_t ImUser::GetUserConn()
{
    uint32_t conn_cnt = 0;
    for (map<uint32_t, MsgConn*>::iterator it = m_conn_map.begin(); it != m_conn_map.end(); it++)
    {
        MsgConn* pConn = it->second;
        if (pConn->IsOpen()) {
            conn_cnt++;
        }
    }
    
    user_conn_t user_cnt = {m_user_id, conn_cnt};
    return user_cnt;
}

void ImUser::BroadcastPdu(ImPdu* pPdu, MsgConn* pFromConn)
{
    for (map<uint32_t, MsgConn*>::iterator it = m_conn_map.begin(); it != m_conn_map.end(); it++)
    {
        MsgConn* pConn = it->second;
        if (pConn != pFromConn) {
            pConn->sendPdu(pPdu);
        }
    }
}

void ImUser::BroadcastPduWithOutMobile(ImPdu *pPdu, MsgConn* pFromConn)
{
    for (map<uint32_t, MsgConn*>::iterator it = m_conn_map.begin(); it != m_conn_map.end(); it++)
    {
        MsgConn* pConn = it->second;
        if (pConn != pFromConn && CHECK_CLIENT_TYPE_PC(pConn->GetClientType())) {
            pConn->sendPdu(pPdu);
        }
    }
}

void ImUser::BroadcastPduToMobile(ImPdu* pPdu, MsgConn* pFromConn)
{
    for (map<uint32_t, MsgConn*>::iterator it = m_conn_map.begin(); it != m_conn_map.end(); it++)
    {
        MsgConn* pConn = it->second;
        if (pConn != pFromConn && CHECK_CLIENT_TYPE_MOBILE(pConn->GetClientType())) {
            pConn->sendPdu(pPdu);
        }
    }
}


void ImUser::BroadcastClientMsgData(ImPdu* pPdu, uint32_t msg_id, MsgConn* pFromConn, uint32_t from_id)
{
    for (map<uint32_t, MsgConn*>::iterator it = m_conn_map.begin(); it != m_conn_map.end(); it++)
    {
        MsgConn* pConn = it->second;
        if (pConn != pFromConn) {
            pConn->sendPdu(pPdu);
            pConn->AddToSendList(msg_id, from_id);
        }
    }
}

void ImUser::BroadcastData(void *buff, uint32_t len, MsgConn* pFromConn)
{
    if(!buff)
        return;
    for (map<uint32_t, MsgConn*>::iterator it = m_conn_map.begin(); it != m_conn_map.end(); it++)
    {
        MsgConn* pConn = it->second;
        
        if(pConn == NULL)
            continue;
        
        if (pConn != pFromConn) {
            pConn->Send(buff, len);
        }
    }
}

void ImUser::HandleKickUser(MsgConn* pConn, uint32_t reason)
{
    map<uint32_t, MsgConn*>::iterator it = m_conn_map.find(pConn->GetHandle());
    if (it != m_conn_map.end()) {
        MsgConn* pConn = it->second;
        if(pConn) {
            LOGE("kick service user, user_id=%u.", m_user_id);
            IM::Login::IMKickUser msg;
            msg.set_user_id(m_user_id);
            msg.set_kick_reason((::IM::BaseDefine::KickReasonType)reason);
            ImPdu pdu;
            pdu.SetPBMsg(&msg);
            pdu.SetServiceId(SID_LOGIN);
            pdu.SetCommandId(CID_LOGIN_KICK_USER);
            pConn->sendPdu(&pdu);
            pConn->SetKickOff();
            //pConn->Close();
        }
    }
}

bool ImUser::KickOutSameClientType(uint32_t client_type, uint32_t reason, MsgConn* pFromConn)
{
    for (map<uint32_t, MsgConn*>::iterator it = m_conn_map.begin(); it != m_conn_map.end(); it++)
    {
        MsgConn* pMsgConn = it->second;
        
        if ((((pMsgConn->GetClientType() ^ client_type) >> 4) == 0) && (pMsgConn != pFromConn)) {
            HandleKickUser(pMsgConn, reason);
            break;
        }
    }
    return true;
}

uint32_t ImUser::GetClientTypeFlag()
{
    uint32_t client_type_flag = 0x00;
    map<uint32_t, MsgConn*>::iterator it = m_conn_map.begin();
    for (; it != m_conn_map.end(); it++)
    {
        MsgConn* pConn = it->second;
        uint32_t client_type = pConn->GetClientType();
        if (CHECK_CLIENT_TYPE_PC(client_type))
        {
            client_type_flag |= CLIENT_TYPE_FLAG_PC;
        }
        else if (CHECK_CLIENT_TYPE_MOBILE(client_type))
        {
            client_type_flag |= CLIENT_TYPE_FLAG_MOBILE;
        }
    }
    return client_type_flag;
}


ImUserManager::~ImUserManager()
{
    RemoveAll();
}

ImUserManager* ImUserManager::GetInstance()
{
    static ImUserManager s_manager;
    return &s_manager;
}


ImUser* ImUserManager::GetImUserByLoginName(string login_name)
{
    ImUser* pUser = NULL;
    ImUserMapByName_t::iterator it = m_im_user_map_by_name.find(login_name);
    if (it != m_im_user_map_by_name.end()) {
        pUser = it->second;
    }
    return pUser;
}

ImUser* ImUserManager::GetImUserById(uint32_t user_id)
{
    ImUser* pUser = NULL;
    ImUserMap_t::iterator it = m_im_user_map.find(user_id);
    if (it != m_im_user_map.end()) {
        pUser = it->second;
    }
    return pUser;
}

MsgConn* ImUserManager::GetMsgConnByHandle(uint32_t user_id, uint32_t handle)
{
    MsgConn* pMsgConn = NULL;
    ImUser* pImUser = GetImUserById(user_id);
    if (pImUser) {
        pMsgConn = pImUser->GetMsgConn(handle);
    }
    return pMsgConn;
}

bool ImUserManager::AddImUserByLoginName(string login_name, ImUser *pUser)
{
    bool bRet = false;
    if (GetImUserByLoginName(login_name) == NULL) {
        m_im_user_map_by_name[login_name] = pUser;
        bRet = true;
    }
    return bRet;
}

void ImUserManager::RemoveImUserByLoginName(string login_name)
{
    m_im_user_map_by_name.erase(login_name);
}

bool ImUserManager::AddImUserById(uint32_t user_id, ImUser *pUser)
{
    bool bRet = false;
    if (GetImUserById(user_id) == NULL) {
        m_im_user_map[user_id] = pUser;
        bRet = true;
    }
    return bRet;
}

void ImUserManager::RemoveImUserById(uint32_t user_id)
{
    m_im_user_map.erase(user_id);
}

void ImUserManager::RemoveImUser(ImUser *pUser)
{
    if (pUser != NULL) {
        RemoveImUserById(pUser->GetUserId());
        RemoveImUserByLoginName(pUser->GetLoginName());
        delete pUser;
        pUser = NULL;
    }
}

void ImUserManager::RemoveAll()
{
    for (ImUserMapByName_t::iterator it = m_im_user_map_by_name.begin(); it != m_im_user_map_by_name.end();
         it++)
    {
        ImUser* pUser = it->second;
        if (pUser != NULL) {
            delete pUser;
            pUser = NULL;
        }
    }
    m_im_user_map_by_name.clear();
    m_im_user_map.clear();
}

void ImUserManager::GetOnlineUserInfo(list<user_stat_t>* online_user_info)
{
    user_stat_t status;
    ImUser* pImUser = NULL;
    for (ImUserMap_t::iterator it = m_im_user_map.begin(); it != m_im_user_map.end(); it++) {
        pImUser = (ImUser*)it->second;
        if (pImUser->IsValidate()) {
            map<uint32_t, MsgConn*>& ConnMap = pImUser->GetMsgConnMap();
            for (map<uint32_t, MsgConn*>::iterator it = ConnMap.begin(); it != ConnMap.end(); it++)
            {
                MsgConn* pConn = it->second;
                if (pConn->IsOpen())
                {
                    status.user_id = pImUser->GetUserId();
                    status.client_type = pConn->GetClientType();
                    status.status = pConn->GetOnlineStatus();
                    online_user_info->push_back(status);
                }
            }
        }
    }
}

void ImUserManager::GetUserConnCnt(list<user_conn_t>* user_conn_list, uint32_t& total_conn_cnt)
{
    total_conn_cnt = 0;
    ImUser* pImUser = NULL;
    for (ImUserMap_t::iterator it = m_im_user_map.begin(); it != m_im_user_map.end(); it++)
    {
        pImUser = (ImUser*)it->second;
        if (pImUser->IsValidate())
        {
            user_conn_t user_conn_cnt = pImUser->GetUserConn();
            user_conn_list->push_back(user_conn_cnt);
            total_conn_cnt += user_conn_cnt.conn_cnt;
        }
    }
}

void ImUserManager::BroadcastPdu(ImPdu* pdu, uint32_t client_type_flag)
{
    ImUser* pImUser = NULL;
    for (ImUserMap_t::iterator it = m_im_user_map.begin(); it != m_im_user_map.end(); it++)
    {
        pImUser = (ImUser*)it->second;
        if (pImUser->IsValidate())
        {
            switch (client_type_flag) {
                case CLIENT_TYPE_FLAG_PC:
                    pImUser->BroadcastPduWithOutMobile(pdu);
                    break;
                case CLIENT_TYPE_FLAG_MOBILE:
                    pImUser->BroadcastPduToMobile(pdu);
                    break;
                case CLIENT_TYPE_FLAG_BOTH:
                    pImUser->BroadcastPdu(pdu);
                    break;
                default:
                    break;
            }
        }
    }
}

