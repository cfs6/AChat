
#include <map>
#include <set>

#include "../DBPool.h"
#include "../CachePool.h"
#include "MessageModel.h"
#include "AudioModel.h"
#include "SessionModel.h"
#include "RelationModel.h"
#include "AsyncLog.h"
using namespace std;

MessageModel* MessageModel::m_pInstance = NULL;
extern string strAudioEnc;

MessageModel::MessageModel()
{

}

MessageModel::~MessageModel()
{

}

MessageModel* MessageModel::getInstance()
{
	if (!m_pInstance) {
		m_pInstance = new MessageModel();
	}

	return m_pInstance;
}

void MessageModel::getMessage(uint32_t nUserId, uint32_t nPeerId, uint32_t nMsgId,
                               uint32_t nMsgCnt, list<IM::BaseDefine::MsgInfo>& lsMsg)
{
    uint32_t nRelateId = RelationModel::getInstance()->getRelationId(nUserId, nPeerId, false);
	if (nRelateId != INVALID_VALUE)
    {
        DBManager* pDBManager = DBManager::getInstance();
        DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
        if (pDBConn)
        {
            string strTableName = "IMMessage_" + int2string(nRelateId % 8);
            string strSql;
            if (nMsgId == 0) {
                strSql = "select * from " + strTableName + " force index (idx_relateId_status_created) where relateId= " + int2string(nRelateId) + " and status = 0 order by created desc, id desc limit " + int2string(nMsgCnt);
            }
            else
            {
                strSql = "select * from " + strTableName + " force index (idx_relateId_status_created) where relateId= " + int2string(nRelateId) + " and status = 0 and msgId <=" + int2string(nMsgId)+ " order by created desc, id desc limit " + int2string(nMsgCnt);
            }
            ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
            if (pResultSet)
            {
                while (pResultSet->next())
                {
                    IM::BaseDefine::MsgInfo cMsg;
                    cMsg.set_msg_id(pResultSet->getInt("msgId"));
                    cMsg.set_from_session_id(pResultSet->getInt("fromId"));
                    cMsg.set_create_time(pResultSet->getInt("created"));
                    IM::BaseDefine::MsgType nMsgType = IM::BaseDefine::MsgType(pResultSet->getInt("type"));
                    if(IM::BaseDefine::MsgType_IsValid(nMsgType))
                    {
                        cMsg.set_msg_type(nMsgType);
                        cMsg.set_msg_data(pResultSet->getString("content"));
                        lsMsg.push_back(cMsg);
                    }
                    else
                    {
                        LOGE("invalid msgType. userId=%u, peerId=%u, msgId=%u, msgCnt=%u, msgType=%u", nUserId, nPeerId, nMsgId, nMsgCnt, nMsgType);
                    }
                }
                delete pResultSet;
            }
            else
            {
                LOGE("no result set: %s", strSql.c_str());
            }
            pDBManager->relDBConn(pDBConn);
            if (!lsMsg.empty())
            {
                AudioModel::getInstance()->readAudios(lsMsg);
            }
        }
        else
        {
            LOGE("no db connection for AChat_slave");
        }
	}
    else
    {
        LOGE("no relation between %lu and %lu", nUserId, nPeerId);
    }
}

bool MessageModel::sendMessage(uint32_t nRelateId, uint32_t nFromId, uint32_t nToId, IM::BaseDefine::MsgType nMsgType, uint32_t nCreateTime, uint32_t nMsgId, string& strMsgContent)
{
    bool bRet =false;
    if (nFromId == 0 || nToId == 0) {
        LOGE("invalied userId.%u->%u", nFromId, nToId);
        return bRet;
    }

	DBManager* pDBManager = DBManager::getInstance();
	DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
	if (pDBConn)
    {
        string strTableName = "IMMessage_" + int2string(nRelateId % 8);
        string strSql = "insert into " + strTableName + " (`relateId`, `fromId`, `toId`, `msgId`, `content`, `status`, `type`, `created`, `updated`) values(?, ?, ?, ?, ?, ?, ?, ?, ?)";
        // 必须在释放连接前delete CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
        PrepareStatement* pStmt = new PrepareStatement();
        if (pStmt->init(pDBConn->getMysql(), strSql))
        {
            uint32_t nStatus = 0;
            uint32_t nType = nMsgType;
            uint32_t index = 0;
            pStmt->setParam(index++, nRelateId);
            pStmt->setParam(index++, nFromId);
            pStmt->setParam(index++, nToId);
            pStmt->setParam(index++, nMsgId);
            pStmt->setParam(index++, strMsgContent);
            pStmt->setParam(index++, nStatus);
            pStmt->setParam(index++, nType);
            pStmt->setParam(index++, nCreateTime);
            pStmt->setParam(index++, nCreateTime);
            bRet = pStmt->executeUpdate();
        }
        delete pStmt;
        pDBManager->relDBConn(pDBConn);
        if (bRet)
        {
            uint32_t nNow = (uint32_t) time(NULL);
            incMsgCount(nFromId, nToId);
        }
        else
        {
            LOGE("insert message failed: %s", strSql.c_str());
        }
	}
    else
    {
        LOGE("no db connection for AChat_master");
    }
	return bRet;
}

bool MessageModel::sendAudioMessage(uint32_t nRelateId, uint32_t nFromId, uint32_t nToId, IM::BaseDefine::MsgType nMsgType, uint32_t nCreateTime, uint32_t nMsgId, const char* pMsgContent, uint32_t nMsgLen)
{
	if (nMsgLen <= 4)
	{
		return false;
	}

	AudioModel* pAudioModel = AudioModel::getInstance();
	int nAudioId = pAudioModel->saveAudioInfo(nFromId, nToId, nCreateTime, pMsgContent, nMsgLen);

	bool bRet = true;
	if (nAudioId != -1)
	{
		string strMsg = int2string(nAudioId);
		bRet = sendMessage(nRelateId, nFromId, nToId, nMsgType, nCreateTime, nMsgId, strMsg);
	}
	else
	{
		bRet = false;
	}

	return bRet;
}

void MessageModel::incMsgCount(uint32_t nFromId, uint32_t nToId)
{
	CacheManager* pCacheManager = CacheManager::getInstance();
	// increase message count
	CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
	if (pCacheConn) {
		pCacheConn->hincrBy("unread_" + int2string(nToId), int2string(nFromId), 1);
		pCacheManager->relCacheConn(pCacheConn);
	} else {
		LOGE("no cache connection to increase unread count: %d->%d", nFromId, nToId);
	}
}

void MessageModel::getUnreadMsgCount(uint32_t nUserId, uint32_t &nTotalCnt, list<IM::BaseDefine::UnreadInfo>& lsUnreadCount)
{
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn)
    {
        map<string, string> mapUnread;
        string strKey = "unread_" + int2string(nUserId);
        bool bRet = pCacheConn->hgetAll(strKey, mapUnread);
        pCacheManager->relCacheConn(pCacheConn);
        if(bRet)
        {
            IM::BaseDefine::UnreadInfo cUnreadInfo;
            for (auto it = mapUnread.begin(); it != mapUnread.end(); it++) {
                cUnreadInfo.set_session_id(atoi(it->first.c_str()));
                cUnreadInfo.set_unread_cnt(atoi(it->second.c_str()));
                cUnreadInfo.set_session_type(IM::BaseDefine::SESSION_TYPE_SINGLE);
                uint32_t nMsgId = 0;
                string strMsgData;
                IM::BaseDefine::MsgType nMsgType;
                getLastMsg(cUnreadInfo.session_id(), nUserId, nMsgId, strMsgData, nMsgType);
                if(IM::BaseDefine::MsgType_IsValid(nMsgType))
                {
                    cUnreadInfo.set_latest_msg_id(nMsgId);
                    cUnreadInfo.set_latest_msg_data(strMsgData);
                    cUnreadInfo.set_latest_msg_type(nMsgType);
                    cUnreadInfo.set_latest_msg_from_user_id(cUnreadInfo.session_id());
                    lsUnreadCount.push_back(cUnreadInfo);
                    nTotalCnt += cUnreadInfo.unread_cnt();
                }
                else
                {
                    LOGE("invalid msgType. userId=%u, peerId=%u, msgType=%u", nUserId, cUnreadInfo.session_id(), nMsgType);
                }
            }
        }
        else
        {
            LOGE("hgetall %s failed!", strKey.c_str());
        }
    }
    else
    {
        LOGE("no cache connection for unread");
    }
}

uint32_t MessageModel::getMsgId(uint32_t nRelateId)
{
    uint32_t nMsgId = 0;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if(pCacheConn)
    {
        string strKey = "msg_id_" + int2string(nRelateId);
        nMsgId = pCacheConn->incrBy(strKey, 1);
        pCacheManager->relCacheConn(pCacheConn);
    }
    return nMsgId;
}

/**
 *  @param nStatus    0获取未被删除的，1获取所有的，默认获取未被删除的
 */
void MessageModel::getLastMsg(uint32_t nFromId, uint32_t nToId, uint32_t& nMsgId, string& strMsgData, IM::BaseDefine::MsgType& nMsgType, uint32_t nStatus)
{
    uint32_t nRelateId = RelationModel::getInstance()->getRelationId(nFromId, nToId, false);
    
    if (nRelateId != INVALID_VALUE)
    {

        DBManager* pDBManager = DBManager::getInstance();
        DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
        if (pDBConn)
        {
            string strTableName = "IMMessage_" + int2string(nRelateId % 8);
            string strSql = "select msgId,type,content from " + strTableName + " force index (idx_relateId_status_created) where relateId= " + int2string(nRelateId) + " and status = 0 order by created desc, id desc limit 1";
            ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
            if (pResultSet)
            {
                while (pResultSet->next())
                {
                    nMsgId = pResultSet->getInt("msgId");

                    nMsgType = IM::BaseDefine::MsgType(pResultSet->getInt("type"));
                    if (nMsgType == IM::BaseDefine::MSG_TYPE_SINGLE_AUDIO)
                    {
                        // "[语音]"加密后的字符串
                        strMsgData = strAudioEnc;
                    }
                    else
                    {
                        strMsgData = pResultSet->getString("content");
                    }
                }
                delete pResultSet;
            }
            else
            {
                LOGE("no result set: %s", strSql.c_str());
            }
            pDBManager->relDBConn(pDBConn);
        }
        else
        {
            LOGE("no db connection_slave");
        }
    }
    else
    {
        LOGE("no relation between %lu and %lu", nFromId, nToId);
    }
}

void MessageModel::getUnReadCntAll(uint32_t nUserId, uint32_t &nTotalCnt)
{
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn)
    {
        map<string, string> mapUnread;
        string strKey = "unread_" + int2string(nUserId);
        bool bRet = pCacheConn->hgetAll(strKey, mapUnread);
        pCacheManager->relCacheConn(pCacheConn);
        
        if(bRet)
        {
            for (auto it = mapUnread.begin(); it != mapUnread.end(); it++) {
                nTotalCnt += atoi(it->second.c_str());
            }
        }
        else
        {
            LOGE("hgetall %s failed!", strKey.c_str());
        }
    }
    else
    {
        LOGE("no cache connection for unread");
    }
}

void MessageModel::getMsgByMsgId(uint32_t nUserId, uint32_t nPeerId, const list<uint32_t> &lsMsgId, list<IM::BaseDefine::MsgInfo> &lsMsg)
{
    if(lsMsgId.empty())

    {
        return ;
    }
    uint32_t nRelateId = RelationModel::getInstance()->getRelationId(nUserId, nPeerId, false);

    if(nRelateId == INVALID_VALUE)
    {
        LOGE("invalid relation id between %u and %u", nUserId, nPeerId);
        return;
    }

    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if (pDBConn)
    {
        string strTableName = "IMMessage_" + int2string(nRelateId % 8);
        string strClause ;
        bool bFirst = true;
        for(auto it= lsMsgId.begin(); it!=lsMsgId.end();++it)
        {
            if (bFirst) {
                bFirst = false;
                strClause = int2string(*it);
            }
            else
            {
                strClause += ("," + int2string(*it));
            }
        }

        string strSql = "select * from " + strTableName + " where relateId=" + int2string(nRelateId) + "  and status=0 and msgId in (" + strClause + ") order by created desc, id desc limit 100";
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if (pResultSet)
        {
            while (pResultSet->next())
            {
                IM::BaseDefine::MsgInfo msg;
                msg.set_msg_id(pResultSet->getInt("msgId"));
                msg.set_from_session_id(pResultSet->getInt("fromId"));
                msg.set_create_time(pResultSet->getInt("created"));
                IM::BaseDefine::MsgType nMsgType = IM::BaseDefine::MsgType(pResultSet->getInt("type"));
                if(IM::BaseDefine::MsgType_IsValid(nMsgType))
                {
                    msg.set_msg_type(nMsgType);
                    msg.set_msg_data(pResultSet->getString("content"));
                    lsMsg.push_back(msg);
                }
                else
                {
                    LOGE("invalid msgType. userId=%u, peerId=%u, msgType=%u, msgId=%u", nUserId, nPeerId, nMsgType, msg.msg_id());
                }
            }
            delete pResultSet;
        }
        else
        {
            LOGE("no result set for sql:%s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
        if(!lsMsg.empty())
        {
            AudioModel::getInstance()->readAudios(lsMsg);
        }
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
}

bool MessageModel::resetMsgId(uint32_t nRelateId)
{
    bool bRet = false;
    uint32_t nMsgId = 0;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if(pCacheConn)
    {
        string strKey = "msg_id_" + int2string(nRelateId);
        string strValue = "0";
        string strReply = pCacheConn->set(strKey, strValue);
        if(strReply == strValue)
        {
            bRet = true;
        }
        pCacheManager->relCacheConn(pCacheConn);
    }
    return bRet;
}
