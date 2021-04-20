#include "MessageService.h"

#include "../ProxyConn.h"
#include "../CachePool.h"
#include "../DBPool.h"
#include "MessageModel.h"
#include "GroupMessageModel.h"
#include "Common.h"
#include "GroupModel.h"
#include "ImPduBase.h"
#include "IM.Message.pb.h"
#include "SessionModel.h"
#include "RelationModel.h"
#include "AsyncLog.h"
namespace DB_PROXY {

    void getMessage(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Message::IMGetMsgListReq msg;
  if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            uint32_t nUserId = msg.user_id();
            uint32_t nPeerId = msg.session_id();
            uint32_t nMsgId = msg.msg_id_begin();
            uint32_t nMsgCnt = msg.msg_cnt();
            IM::BaseDefine::SessionType nSessionType = msg.session_type();
            if(IM::BaseDefine::SessionType_IsValid(nSessionType))
            {
                ImPdu* pPduResp = new ImPdu;
                IM::Message::IMGetMsgListRsp msgResp;

                list<IM::BaseDefine::MsgInfo> lsMsg;

                if(nSessionType == IM::BaseDefine::SESSION_TYPE_SINGLE)//获取个人消息
                {
                    MessageModel::getInstance()->getMessage(nUserId, nPeerId, nMsgId, nMsgCnt, lsMsg);
                }
                else if(nSessionType == IM::BaseDefine::SESSION_TYPE_GROUP)//获取群消息
                {
                    if(GroupModel::getInstance()->isInGroup(nUserId, nPeerId))
                    {
                        GroupMessageModel::getInstance()->getMessage(nUserId, nPeerId, nMsgId, nMsgCnt, lsMsg);
                    }
                }

                msgResp.set_user_id(nUserId);
                msgResp.set_session_id(nPeerId);
                msgResp.set_msg_id_begin(nMsgId);
                msgResp.set_session_type(nSessionType);
                for(auto it=lsMsg.begin(); it!=lsMsg.end();++it)
                {
                    IM::BaseDefine::MsgInfo* pMsg = msgResp.add_msg_list();
        //            *pMsg = *it;
                    pMsg->set_msg_id(it->msg_id());
                    pMsg->set_from_session_id(it->from_session_id());
                    pMsg->set_create_time(it->create_time());
                    pMsg->set_msg_type(it->msg_type());
                    pMsg->set_msg_data(it->msg_data());
//                    LOGE("userId=%u, peerId=%u, msgId=%u", nUserId, nPeerId, it->msg_id());
                }

                LOGE("userId=%u, peerId=%u, msgId=%u, msgCnt=%u, count=%u", nUserId, nPeerId, nMsgId, nMsgCnt, msgResp.msg_list_size());
                msgResp.set_attach_data(msg.attach_data());
                pPduResp->SetPBMsg(&msgResp);
                pPduResp->SetSeqNum(pPdu->GetSeqNum());
                pPduResp->SetServiceId(IM::BaseDefine::SID_MSG);
                pPduResp->SetCommandId(IM::BaseDefine::CID_MSG_LIST_RESPONSE);
                ProxyConn::AddResponsePdu(conn_uuid, pPduResp);
            }
            else
            {
                LOGE("invalid sessionType. userId=%u, peerId=%u, msgId=%u, msgCnt=%u, sessionType=%u",
                    nUserId, nPeerId, nMsgId, nMsgCnt, nSessionType);
            }
        }
        else
        {
            LOGE("parse pb failed");
        }
    }

    void sendMessage(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Message::IMMsgData msg;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            uint32_t nFromId = msg.from_user_id();
            uint32_t nToId = msg.to_session_id();
            uint32_t nCreateTime = msg.create_time();
            IM::BaseDefine::MsgType nMsgType = msg.msg_type();
            uint32_t nMsgLen = msg.msg_data().length();

            uint32_t nNow = (uint32_t)time(NULL);
            if (IM::BaseDefine::MsgType_IsValid(nMsgType))
            {
                if(nMsgLen != 0)
                {
                    ImPdu* pPduResp = new ImPdu;

                    uint32_t nMsgId = INVALID_VALUE;
                    uint32_t nSessionId = INVALID_VALUE;
                    uint32_t nPeerSessionId = INVALID_VALUE;

                    MessageModel* pMsgModel = MessageModel::getInstance();
                    GroupMessageModel* pGroupMsgModel = GroupMessageModel::getInstance();
                    if(nMsgType == IM::BaseDefine::MSG_TYPE_GROUP_TEXT) {
                        GroupModel* pGroupModel = GroupModel::getInstance();
                        if (pGroupModel->isValidateGroupId(nToId) && pGroupModel->isInGroup(nFromId, nToId))
                        {
                            nSessionId = SessionModel::getInstance()->getSessionId(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_GROUP, false);
                            if (INVALID_VALUE == nSessionId) {
                                nSessionId = SessionModel::getInstance()->addSession(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_GROUP);
                            }
                            if(nSessionId != INVALID_VALUE)
                            {
                                nMsgId = pGroupMsgModel->getMsgId(nToId);
                                if (nMsgId != INVALID_VALUE) {
                                    pGroupMsgModel->sendMessage(nFromId, nToId, nMsgType, nCreateTime, nMsgId, (string&)msg.msg_data());
                                    SessionModel::getInstance()->updateSession(nSessionId, nNow);
                                }
                            }
                        }
                        else
                        {
                            LOGE("invalid groupId. fromId=%u, groupId=%u", nFromId, nToId);
                            delete pPduResp;
                            return;
                        }
                    } else if (nMsgType == IM::BaseDefine::MSG_TYPE_GROUP_AUDIO) {
                        GroupModel* pGroupModel = GroupModel::getInstance();
                        if (pGroupModel->isValidateGroupId(nToId)&& pGroupModel->isInGroup(nFromId, nToId))
                        {
                            nSessionId = SessionModel::getInstance()->getSessionId(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_GROUP, false);
                            if (INVALID_VALUE == nSessionId) {
                                nSessionId = SessionModel::getInstance()->addSession(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_GROUP);
                            }
                            if(nSessionId != INVALID_VALUE)
                            {
                                nMsgId = pGroupMsgModel->getMsgId(nToId);
                                if(nMsgId != INVALID_VALUE)
                                {
                                    pGroupMsgModel->sendAudioMessage(nFromId, nToId, nMsgType, nCreateTime, nMsgId, msg.msg_data().c_str(), nMsgLen);
                                    SessionModel::getInstance()->updateSession(nSessionId, nNow);
                                }
                            }
                        }
                        else
                        {
                            LOGE("invalid groupId. fromId=%u, groupId=%u", nFromId, nToId);
                            delete pPduResp;
                            return;
                        }
                    } else if(nMsgType== IM::BaseDefine::MSG_TYPE_SINGLE_TEXT) {
                        if (nFromId != nToId) {
                            nSessionId = SessionModel::getInstance()->getSessionId(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_SINGLE, false);
                            if (INVALID_VALUE == nSessionId) {
                                nSessionId = SessionModel::getInstance()->addSession(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_SINGLE);
                            }
                            nPeerSessionId = SessionModel::getInstance()->getSessionId(nToId, nFromId, IM::BaseDefine::SESSION_TYPE_SINGLE, false);
                            if(INVALID_VALUE ==  nPeerSessionId)
                            {
                                nSessionId = SessionModel::getInstance()->addSession(nToId, nFromId, IM::BaseDefine::SESSION_TYPE_SINGLE);
                            }
                            uint32_t nRelateId = CRelationModel::getInstance()->getRelationId(nFromId, nToId, true);
                            if(nSessionId != INVALID_VALUE && nRelateId != INVALID_VALUE)
                            {
                                nMsgId = pMsgModel->getMsgId(nRelateId);
                                if(nMsgId != INVALID_VALUE)
                                {
                                    pMsgModel->sendMessage(nRelateId, nFromId, nToId, nMsgType, nCreateTime, nMsgId, (string&)msg.msg_data());
                                    SessionModel::getInstance()->updateSession(nSessionId, nNow);
                                    SessionModel::getInstance()->updateSession(nPeerSessionId, nNow);
                                }
                                else
                                {
                                    LOGE("msgId is invalid. fromId=%u, toId=%u, nRelateId=%u, nSessionId=%u, nMsgType=%u", nFromId, nToId, nRelateId, nSessionId, nMsgType);
                                }
                            }
                            else{
                                LOGE("sessionId or relateId is invalid. fromId=%u, toId=%u, nRelateId=%u, nSessionId=%u, nMsgType=%u", nFromId, nToId, nRelateId, nSessionId, nMsgType);
                            }
                        }
                        else
                        {
                            LOGE("send msg to self. fromId=%u, toId=%u, msgType=%u", nFromId, nToId, nMsgType);
                        }

                    } else if(nMsgType == IM::BaseDefine::MSG_TYPE_SINGLE_AUDIO) {

                        if(nFromId != nToId)
                        {
                            nSessionId = SessionModel::getInstance()->getSessionId(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_SINGLE, false);
                            if (INVALID_VALUE == nSessionId) {
                                nSessionId = SessionModel::getInstance()->addSession(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_SINGLE);
                            }
                            nPeerSessionId = SessionModel::getInstance()->getSessionId(nToId, nFromId, IM::BaseDefine::SESSION_TYPE_SINGLE, false);
                            if(INVALID_VALUE ==  nPeerSessionId)
                            {
                                nSessionId = SessionModel::getInstance()->addSession(nToId, nFromId, IM::BaseDefine::SESSION_TYPE_SINGLE);
                            }
                            uint32_t nRelateId = CRelationModel::getInstance()->getRelationId(nFromId, nToId, true);
                            if(nSessionId != INVALID_VALUE && nRelateId != INVALID_VALUE)
                            {
                                nMsgId = pMsgModel->getMsgId(nRelateId);
                                if(nMsgId != INVALID_VALUE) {
                                    pMsgModel->sendAudioMessage(nRelateId, nFromId, nToId, nMsgType, nCreateTime, nMsgId, msg.msg_data().c_str(), nMsgLen);
                                    SessionModel::getInstance()->updateSession(nSessionId, nNow);
                                    SessionModel::getInstance()->updateSession(nPeerSessionId, nNow);
                                }
                                else {
                                    LOGE("msgId is invalid. fromId=%u, toId=%u, nRelateId=%u, nSessionId=%u, nMsgType=%u", nFromId, nToId, nRelateId, nSessionId, nMsgType);
                                }
                            }
                            else {
                                LOGE("sessionId or relateId is invalid. fromId=%u, toId=%u, nRelateId=%u, nSessionId=%u, nMsgType=%u", nFromId, nToId, nRelateId, nSessionId, nMsgType);
                            }
                        }
                        else
                        {
                            LOGE("send msg to self. fromId=%u, toId=%u, msgType=%u", nFromId, nToId, nMsgType);
                        }
                    }

                    LOGE("fromId=%u, toId=%u, type=%u, msgId=%u, sessionId=%u", nFromId, nToId, nMsgType, nMsgId, nSessionId);

                    msg.set_msg_id(nMsgId);
                    pPduResp->SetPBMsg(&msg);
                    pPduResp->SetSeqNum(pPdu->GetSeqNum());
                    pPduResp->SetServiceId(IM::BaseDefine::SID_MSG);
                    pPduResp->SetCommandId(IM::BaseDefine::CID_MSG_DATA);
                    ProxyConn::AddResponsePdu(conn_uuid, pPduResp);
                }
                else
                {
                    LOGE("msgLen error. fromId=%u, toId=%u, msgType=%u", nFromId, nToId, nMsgType);
                }
            }
            else
            {
                LOGE("invalid msgType.fromId=%u, toId=%u, msgType=%u", nFromId, nToId, nMsgType);
            }
        }
        else
        {
            LOGE("parse pb failed");
        }
    }

    void getMessageById(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Message::IMGetMsgByIdReq msg;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            uint32_t nUserId = msg.user_id();
            IM::BaseDefine::SessionType nType = msg.session_type();
            uint32_t nPeerId = msg.session_id();
            list<uint32_t> lsMsgId;
            uint32_t nCnt = msg.msg_id_list_size();
            for(uint32_t i=0; i<nCnt; ++i)
            {
                lsMsgId.push_back(msg.msg_id_list(i));
            }
            if (IM::BaseDefine::SessionType_IsValid(nType))
            {
                ImPdu* pPduResp = new ImPdu;
                IM::Message::IMGetMsgByIdRsp msgResp;

                list<IM::BaseDefine::MsgInfo> lsMsg;
                if(IM::BaseDefine::SESSION_TYPE_SINGLE == nType)
                {
                    MessageModel::getInstance()->getMsgByMsgId(nUserId, nPeerId, lsMsgId, lsMsg);
                }
                else if(IM::BaseDefine::SESSION_TYPE_GROUP)
                {
                    GroupMessageModel::getInstance()->getMsgByMsgId(nUserId, nPeerId, lsMsgId, lsMsg);
                }
                msgResp.set_user_id(nUserId);
                msgResp.set_session_id(nPeerId);
                msgResp.set_session_type(nType);
                for(auto it=lsMsg.begin(); it!=lsMsg.end(); ++it)
                {
                    IM::BaseDefine::MsgInfo* pMsg = msgResp.add_msg_list();
                    pMsg->set_msg_id(it->msg_id());
                    pMsg->set_from_session_id(it->from_session_id());
                    pMsg->set_create_time(it->create_time());
                    pMsg->set_msg_type(it->msg_type());
                    pMsg->set_msg_data(it->msg_data());
                }
                LOGE("userId=%u, peerId=%u, sessionType=%u, reqMsgCnt=%u, resMsgCnt=%u", nUserId, nPeerId, nType, msg.msg_id_list_size(), msgResp.msg_list_size());
                msgResp.set_attach_data(msg.attach_data());
                pPduResp->SetPBMsg(&msgResp);
                pPduResp->SetSeqNum(pPdu->GetSeqNum());
                pPduResp->SetServiceId(IM::BaseDefine::SID_MSG);
                pPduResp->SetCommandId(IM::BaseDefine::CID_MSG_GET_BY_MSG_ID_RES);
                ProxyConn::AddResponsePdu(conn_uuid, pPduResp);
            }
            else
            {
                LOGE("invalid sessionType. fromId=%u, toId=%u, sessionType=%u, msgCnt=%u", nUserId, nPeerId, nType, nCnt);
            }
        }
        else
        {
            LOGE("parse pb failed");
        }
    }

    void getLatestMsgId(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Message::IMGetLatestMsgIdReq msg;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            uint32_t nUserId = msg.user_id();
            IM::BaseDefine::SessionType nType = msg.session_type();
            uint32_t nPeerId = msg.session_id();
            if (IM::BaseDefine::SessionType_IsValid(nType)) {
                ImPdu* pPduResp = new ImPdu;
                IM::Message::IMGetLatestMsgIdRsp msgResp;
                msgResp.set_user_id(nUserId);
                msgResp.set_session_type(nType);
                msgResp.set_session_id(nPeerId);
                uint32_t nMsgId = INVALID_VALUE;
                if(IM::BaseDefine::SESSION_TYPE_SINGLE == nType)
                {
                    string strMsg;
                    IM::BaseDefine::MsgType nMsgType;
                    MessageModel::getInstance()->getLastMsg(nUserId, nPeerId, nMsgId, strMsg, nMsgType, 1);
                }
                else
                {
                    string strMsg;
                    IM::BaseDefine::MsgType nMsgType;
                    uint32_t nFromId = INVALID_VALUE;
                    GroupMessageModel::getInstance()->getLastMsg(nPeerId, nMsgId, strMsg, nMsgType, nFromId);
                }
                msgResp.set_latest_msg_id(nMsgId);
                LOGE("userId=%u, peerId=%u, sessionType=%u, msgId=%u", nUserId, nPeerId, nType,nMsgId);
                msgResp.set_attach_data(msg.attach_data());
                pPduResp->SetPBMsg(&msgResp);
                pPduResp->SetSeqNum(pPdu->GetSeqNum());
                pPduResp->SetServiceId(IM::BaseDefine::SID_MSG);
                pPduResp->SetCommandId(IM::BaseDefine::CID_MSG_GET_LATEST_MSG_ID_RSP);
                ProxyConn::AddResponsePdu(conn_uuid, pPduResp);

            }
            else
            {
                LOGE("invalid sessionType. userId=%u, peerId=%u, sessionType=%u", nUserId, nPeerId, nType);
            }
        }
        else
        {
            LOGE("parse pb failed");
        }
    }

    void getUnreadMsgCounter(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Message::IMUnreadMsgCntReq msg;
        IM::Message::IMUnreadMsgCntRsp msgResp;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            ImPdu* pPduResp = new ImPdu;

            uint32_t nUserId = msg.user_id();

            list<IM::BaseDefine::UnreadInfo> lsUnreadCount;
            uint32_t nTotalCnt = 0;

            MessageModel::getInstance()->getUnreadMsgCount(nUserId, nTotalCnt, lsUnreadCount);
            GroupMessageModel::getInstance()->getUnreadMsgCount(nUserId, nTotalCnt, lsUnreadCount);
            msgResp.set_user_id(nUserId);
            msgResp.set_total_cnt(nTotalCnt);
            for(auto it= lsUnreadCount.begin(); it!=lsUnreadCount.end(); ++it)
            {
                IM::BaseDefine::UnreadInfo* pInfo = msgResp.add_unreadinfo_list();
    //            *pInfo = *it;
                pInfo->set_session_id(it->session_id());
                pInfo->set_session_type(it->session_type());
                pInfo->set_unread_cnt(it->unread_cnt());
                pInfo->set_latest_msg_id(it->latest_msg_id());
                pInfo->set_latest_msg_data(it->latest_msg_data());
                pInfo->set_latest_msg_type(it->latest_msg_type());
                pInfo->set_latest_msg_from_user_id(it->latest_msg_from_user_id());
            }


            LOGE("userId=%d, unreadCnt=%u, totalCount=%u", nUserId, msgResp.unreadinfo_list_size(), nTotalCnt);
            msgResp.set_attach_data(msg.attach_data());
            pPduResp->SetPBMsg(&msgResp);
            pPduResp->SetSeqNum(pPdu->GetSeqNum());
            pPduResp->SetServiceId(IM::BaseDefine::SID_MSG);
            pPduResp->SetCommandId(IM::BaseDefine::CID_MSG_UNREAD_CNT_RESPONSE);
            ProxyConn::AddResponsePdu(conn_uuid, pPduResp);
        }
        else
        {
            LOGE("parse pb failed");
        }
    }

    void clearUnreadMsgCounter(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Message::IMMsgDataReadAck msg;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            uint32_t nUserId = msg.user_id();
            uint32_t nFromId = msg.session_id();
            IM::BaseDefine::SessionType nSessionType = msg.session_type();
            UserModel::getInstance()->clearUserCounter(nUserId, nFromId, nSessionType);
            LOGE("userId=%u, peerId=%u, type=%u", nFromId, nUserId, nSessionType);
        }
        else
        {
            LOGE("parse pb failed");
        }
    }

    void setDevicesToken(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Login::IMDeviceTokenReq msg;
        IM::Login::IMDeviceTokenRsp msgResp;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            uint32_t nUserId = msg.user_id();
            string strToken = msg.device_token();
            ImPdu* pPduResp = new ImPdu;
            CacheManager* pCacheManager = CacheManager::getInstance();
            CacheConn* pCacheConn = pCacheManager->getCacheConn("token");
            if (pCacheConn)
            {
                IM::BaseDefine::ClientType nClientType = msg.client_type();
                string strValue;
                if(IM::BaseDefine::CLIENT_TYPE_IOS == nClientType)
                {
                    strValue = "ios:"+strToken;
                }
                else if(IM::BaseDefine::CLIENT_TYPE_ANDROID == nClientType)
                {
                    strValue = "android:"+strToken;
                }
                else
                {
                    strValue = strToken;
                }

                string strOldValue = pCacheConn->get("device_"+int2string(nUserId));

                if(!strOldValue.empty())
                {
                    size_t nPos = strOldValue.find(":");
                    if(nPos!=string::npos)
                    {
                        string strOldToken = strOldValue.substr(nPos + 1);
                        string strReply = pCacheConn->get("device_"+strOldToken);
                        if (!strReply.empty()) {
                            string strNewValue("");
                            pCacheConn->set("device_"+strOldToken, strNewValue);
                        }
                    }
                }

                pCacheConn->set("device_"+int2string(nUserId), strValue);
                string strNewValue = int2string(nUserId);
                pCacheConn->set("device_"+strToken, strNewValue);

                LOGE("setDeviceToken. userId=%u, deviceToken=%s", nUserId, strToken.c_str());
                pCacheManager->relCacheConn(pCacheConn);
            }
            else
            {
                LOGE("no cache connection for token");
            }

            LOGE("setDeviceToken. userId=%u, deviceToken=%s", nUserId, strToken.c_str());
            msgResp.set_attach_data(msg.attach_data());
            msgResp.set_user_id(nUserId);
            pPduResp->SetPBMsg(&msgResp);
            pPduResp->SetSeqNum(pPdu->GetSeqNum());
            pPduResp->SetServiceId(IM::BaseDefine::SID_LOGIN);
            pPduResp->SetCommandId(IM::BaseDefine::CID_LOGIN_RES_DEVICETOKEN);
            ProxyConn::AddResponsePdu(conn_uuid, pPduResp);
        }
        else
        {
            LOGE("parse pb failed");
        }
    }


    void getDevicesToken(ImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Server::IMGetDeviceTokenReq msg;
        IM::Server::IMGetDeviceTokenRsp msgResp;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            CacheManager* pCacheManager = CacheManager::getInstance();
            CacheConn* pCacheConn = pCacheManager->getCacheConn("token");
            ImPdu* pPduResp = new ImPdu;
            uint32_t nCnt = msg.user_id_size();

            // 对于ios，不推送
            // 对于android，由客户端处理
            bool is_check_shield_status = false;
            time_t now = time(NULL);
            struct tm* _tm = localtime(&now);
            if (_tm->tm_hour >= 22 || _tm->tm_hour <=7 ) {
                    is_check_shield_status = true;
                }
            if (pCacheConn)
            {
                vector<string> vecTokens;
                for (uint32_t i=0; i<nCnt; ++i) {
                    string strKey = "device_"+int2string(msg.user_id(i));
                    vecTokens.push_back(strKey);
                }
                map<string, string> mapTokens;
                bool bRet = pCacheConn->mget(vecTokens, mapTokens);
                pCacheManager->relCacheConn(pCacheConn);

                if(bRet)
                {
                    for (auto it=mapTokens.begin(); it!=mapTokens.end(); ++it) {
                        string strKey = it->first;
                        size_t nPos = strKey.find("device_");
                        if( nPos != string::npos)
                        {
                            string strUserId = strKey.substr(nPos + strlen("device_"));
                            uint32_t nUserId = string2int(strUserId);
                            string strValue = it->second;
                            nPos = strValue.find(":");
                            if(nPos!=string::npos)
                            {
                                string strType = strValue.substr(0, nPos);
                                string strToken = strValue.substr(nPos + 1);
                                IM::BaseDefine::ClientType nClientType = IM::BaseDefine::ClientType(0);
                                if(strType == "ios")
                                {
                                    // 过滤出已经设置勿打扰并且为晚上22：00～07：00
                                    uint32_t shield_status = 0;
                                    if (is_check_shield_status) {
                                        UserModel::getInstance()->getPushShield(nUserId, &shield_status);
                                    }

                                    if (shield_status == 1) {
                                        // 对IOS处理
                                        continue;
                                    } else {
                                        nClientType = IM::BaseDefine::CLIENT_TYPE_IOS;
                                    }

                                    // nClientType = IM::BaseDefine::CLIENT_TYPE_IOS;
                                    // end
                                }
                                else if(strType == "android")
                                {
                                    nClientType = IM::BaseDefine::CLIENT_TYPE_ANDROID;
                                }
                                if(IM::BaseDefine::ClientType_IsValid(nClientType))
                                {
                                    IM::BaseDefine::UserTokenInfo* pToken = msgResp.add_user_token_info();
                                    pToken->set_user_id(nUserId);
                                    pToken->set_token(strToken);
                                    pToken->set_user_type(nClientType);
                                    uint32_t nTotalCnt = 0;
                                    MessageModel::getInstance()->getUnReadCntAll(nUserId, nTotalCnt);
                                    GroupMessageModel::getInstance()->getUnReadCntAll(nUserId, nTotalCnt);
                                    pToken->set_push_count(nTotalCnt);
                                    pToken->set_push_type(1);
                                }
                                else
                                {
                                    LOGE("invalid clientType.clientType=%u", nClientType);
                                }
                            }
                            else
                            {
                                LOGE("invalid value. value=%s", strValue.c_str());
                            }

                        }
                        else
                        {
                            LOGE("invalid key.key=%s", strKey.c_str());
                        }
                    }
                }
                else
                {
                    LOGE("mget failed!");
                }
            }
            else
            {
                LOGE("no cache connection for token");
            }

            LOGE("req devices token.reqCnt=%u, resCnt=%u", nCnt, msgResp.user_token_info_size());

            msgResp.set_attach_data(msg.attach_data());
            pPduResp->SetPBMsg(&msgResp);
            pPduResp->SetSeqNum(pPdu->GetSeqNum());
            pPduResp->SetServiceId(IM::BaseDefine::SID_OTHER);
            pPduResp->SetCommandId(IM::BaseDefine::CID_OTHER_GET_DEVICE_TOKEN_RSP);
            ProxyConn::AddResponsePdu(conn_uuid, pPduResp);
        }
        else
        {
            LOGE("parse pb failed");
        }
    }
};


