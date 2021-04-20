#include "SessionService.h"

#include <list>
#include <vector>

#include "../ProxyConn.h"
#include "../DBPool.h"
#include "../CachePool.h"
#include "SessionModel.h"
#include "UserModel.h"
#include "GroupModel.h"
#include "IM.Buddy.pb.h"
#include "AsyncLog.h"
using namespace std;

namespace DB_PROXY {
    /**
     *  获取最近会话接口
     *
     *  @param pPdu      收到的packet包指针
     *  @param conn_uuid 该包过来的socket 描述符
     */
    void getRecentSession(ImPdu* pPdu, uint32_t conn_uuid)
    {

        IM::Buddy::IMRecentContactSessionReq msg;
        IM::Buddy::IMRecentContactSessionRsp msgResp;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            ImPdu* pPduResp = new ImPdu;

            uint32_t nUserId = msg.user_id();
            uint32_t nLastTime = msg.latest_update_time();

            //获取最近联系人列表
            list<IM::BaseDefine::ContactSessionInfo> lsContactList;
            SessionModel::getInstance()->getRecentSession(nUserId, nLastTime, lsContactList);
            msgResp.set_user_id(nUserId);
            for(auto it=lsContactList.begin(); it!=lsContactList.end(); ++it)
            {
                IM::BaseDefine::ContactSessionInfo* pContact = msgResp.add_contact_session_list();
    //            *pContact = *it;
                pContact->set_session_id(it->session_id());
                pContact->set_session_type(it->session_type());
                pContact->set_session_status(it->session_status());
                pContact->set_updated_time(it->updated_time());
                pContact->set_latest_msg_id(it->latest_msg_id());
                pContact->set_latest_msg_data(it->latest_msg_data());
                pContact->set_latest_msg_type(it->latest_msg_type());
                pContact->set_latest_msg_from_user_id(it->latest_msg_from_user_id());
            }

            LOGE("userId=%u, last_time=%u, count=%u", nUserId, nLastTime, msgResp.contact_session_list_size());

            msgResp.set_attach_data(msg.attach_data());
            pPduResp->SetPBMsg(&msgResp);
            pPduResp->SetSeqNum(pPdu->GetSeqNum());
            pPduResp->SetServiceId(IM::BaseDefine::SID_BUDDY_LIST);
            pPduResp->SetCommandId(IM::BaseDefine::CID_BUDDY_LIST_RECENT_CONTACT_SESSION_RESPONSE);
            CProxyConn::AddResponsePdu(conn_uuid, pPduResp);
        }
        else
        {
            LOGE("parse pb failed");
        }
    }

    /**
     *  删除会话接口
     *
     *  @param pPdu      收到的packet包指针
     *  @param conn_uuid 该包过来的socket 描述符
     */
    void deleteRecentSession(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Buddy::IMRemoveSessionReq msg;
        IM::Buddy::IMRemoveSessionRsp msgResp;

        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            ImPdu* pPduResp = new ImPdu;

            uint32_t nUserId = msg.user_id();
            uint32_t nPeerId = msg.session_id();
            IM::BaseDefine::SessionType nType = msg.session_type();
            if(IM::BaseDefine::SessionType_IsValid(nType))
            {
                bool bRet = false;
                uint32_t nSessionId = SessionModel::getInstance()->getSessionId(nUserId, nPeerId, nType, false);
                if (nSessionId != INVALID_VALUE) {
                    bRet = SessionModel::getInstance()->removeSession(nSessionId);
                    // if remove session success, we need to clear the unread msg count
                    if (bRet)
                    {
                        UserModel::getInstance()->clearUserCounter(nUserId, nPeerId, nType);
                    }
                }
                LOGE("userId=%d, peerId=%d, result=%s", nUserId, nPeerId, bRet?"success":"failed");

                msgResp.set_attach_data(msg.attach_data());
                msgResp.set_user_id(nUserId);
                msgResp.set_session_id(nPeerId);
                msgResp.set_session_type(nType);
                msgResp.set_result_code(bRet?0:1);
                pPduResp->SetPBMsg(&msgResp);
                pPduResp->SetSeqNum(pPdu->GetSeqNum());
                pPduResp->SetServiceId(IM::BaseDefine::SID_BUDDY_LIST);
                pPduResp->SetCommandId(IM::BaseDefine::CID_BUDDY_LIST_REMOVE_SESSION_RES);
                CProxyConn::AddResponsePdu(conn_uuid, pPduResp);
            }
            else
            {
                LOGE("invalied session_type. userId=%u, peerId=%u, seseionType=%u", nUserId, nPeerId, nType);
            }
        }
        else{
            LOGE("parse pb failed");
        }
    }

};



