#include <map>
#include <set>

#include "../DBPool.h"
#include "../CachePool.h"
#include "GroupMessageModel.h"
#include "AudioModel.h"
#include "SessionModel.h"
#include "MessageCounter.h"
#include "Common.h"
#include "GroupModel.h"
#include "AsyncLog.h"
using namespace std;

extern string strAudioEnc;

GroupMessageModel* GroupMessageModel::m_pInstance = NULL;

/**
 *  构造函数
 */
GroupMessageModel::GroupMessageModel()
{

}

/**
 *  析构函数
 */
GroupMessageModel::~GroupMessageModel()
{

}

/**
 *  单例
 *
 *  @return 返回单例指针
 */
GroupMessageModel* GroupMessageModel::getInstance()
{
	if (!m_pInstance) {
		m_pInstance = new GroupMessageModel();
	}

	return m_pInstance;
}

/**
 *  发送群消息接口
 *
 *  @param nRelateId     关系Id
 *  @param nFromId       发送者Id
 *  @param nGroupId      群组Id
 *  @param nMsgType      消息类型
 *  @param nCreateTime   消息创建时间
 *  @param nMsgId        消息Id
 *  @param strMsgContent 消息类容
 *
 *  @return 成功返回true 失败返回false
 */
bool GroupMessageModel::sendMessage(uint32_t nFromId, uint32_t nGroupId, IM::BaseDefine::MsgType nMsgType, uint32_t nCreateTime,uint32_t nMsgId, const string& strMsgContent)
{
    bool bRet = false;
    if(GroupModel::getInstance()->isInGroup(nFromId, nGroupId))
    {
        DBManager* pDBManager = DBManager::getInstance();
        DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
        if (pDBConn)
        {
            string strTableName = "IMGroupMessage_" + int2string(nGroupId % 8);
            string strSql = "insert into " + strTableName + " (`groupId`, `userId`, `msgId`, `content`, `type`, `status`, `updated`, `created`) "\
            "values(?, ?, ?, ?, ?, ?, ?, ?)";
            
            // 必须在释放连接前delete PrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
            PrepareStatement* pStmt = new PrepareStatement();
            if (pStmt->init(pDBConn->getMysql(), strSql))
            {
                uint32_t nStatus = 0;
                uint32_t nType = nMsgType;
                uint32_t index = 0;
                pStmt->setParam(index++, nGroupId);
                pStmt->setParam(index++, nFromId);
                pStmt->setParam(index++, nMsgId);
                pStmt->setParam(index++, strMsgContent);
                pStmt->setParam(index++, nType);
                pStmt->setParam(index++, nStatus);
                pStmt->setParam(index++, nCreateTime);
                pStmt->setParam(index++, nCreateTime);
                
                bool bRet = pStmt->executeUpdate();
                if (bRet)
                {
                    GroupModel::getInstance()->updateGroupChat(nGroupId);
                    incMessageCount(nFromId, nGroupId);
                    clearMessageCount(nFromId, nGroupId);
                } else {
                    LOGE("insert message failed: %s", strSql.c_str());
                }
            }
            delete pStmt;
            pDBManager->relDBConn(pDBConn);
        }
        else
        {
            LOGE("no db connection for AChat_master");
        }
    }
    else
    {
        LOGE("not in the group.fromId=%u, groupId=%u", nFromId, nGroupId);
    }
    return bRet;
}

/**
 *  发送群组语音信息
 *
 *  @param nRelateId   关系Id
 *  @param nFromId     发送者Id
 *  @param nGroupId    群组Id
 *  @param nMsgType    消息类型
 *  @param nCreateTime 消息创建时间
 *  @param nMsgId      消息Id
 *  @param pMsgContent 指向语音类容的指针
 *  @param nMsgLen     语音消息长度
 *
 *  @return 成功返回true，失败返回false
 */
bool GroupMessageModel::sendAudioMessage(uint32_t nFromId, uint32_t nGroupId, IM::BaseDefine::MsgType nMsgType, uint32_t nCreateTime, uint32_t nMsgId, const char* pMsgContent, uint32_t nMsgLen)
{
	if (nMsgLen <= 4) {
		return false;
	}

    if(!GroupModel::getInstance()->isInGroup(nFromId, nGroupId))
    {
        LOGE("not in the group.fromId=%u, groupId=%u", nFromId, nGroupId);
        return false;
    }
    
	AudioModel* pAudioModel = AudioModel::getInstance();
	int nAudioId = pAudioModel->saveAudioInfo(nFromId, nGroupId, nCreateTime, pMsgContent, nMsgLen);

	bool bRet = true;
	if (nAudioId != -1) {
		string strMsg = int2string(nAudioId);
        bRet = sendMessage(nFromId, nGroupId, nMsgType, nCreateTime, nMsgId, strMsg);
	} else {
		bRet = false;
	}

	return bRet;
}

/**
 *  清除群组消息计数
 *
 *  @param nUserId  用户Id
 *  @param nGroupId 群组Id
 *
 *  @return 成功返回true，失败返回false
 */
bool GroupMessageModel::clearMessageCount(uint32_t nUserId, uint32_t nGroupId)
{
    bool bRet = false;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn)
    {
        string strGroupKey = int2string(nGroupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
        map<string, string> mapGroupCount;
        bool bRet = pCacheConn->hgetAll(strGroupKey, mapGroupCount);
        pCacheManager->relCacheConn(pCacheConn);
        if(bRet)
        {
            string strUserKey = int2string(nUserId) + "_" + int2string(nGroupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strReply = pCacheConn->hmset(strUserKey, mapGroupCount);
            if(strReply.empty())
            {
                LOGE("hmset %s failed !", strUserKey.c_str());
            }
            else
            {
                bRet = true;
            }
        }
        else
        {
            LOGE("hgetAll %s failed !", strGroupKey.c_str());
        }
    }
    else
    {
        LOGE("no cache connection for unread");
    }
    return bRet;
}

/**
 *  增加群消息计数
 *
 *  @param nUserId  用户Id
 *  @param nGroupId 群组Id
 *
 *  @return 成功返回true，失败返回false
 */
bool GroupMessageModel::incMessageCount(uint32_t nUserId, uint32_t nGroupId)
{
    bool bRet = false;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn)
    {
        string strGroupKey = int2string(nGroupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
        pCacheConn->hincrBy(strGroupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD, 1);
        map<string, string> mapGroupCount;
        bool bRet = pCacheConn->hgetAll(strGroupKey, mapGroupCount);
        if(bRet)
        {
            string strUserKey = int2string(nUserId) + "_" + int2string(nGroupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strReply = pCacheConn->hmset(strUserKey, mapGroupCount);
            if(!strReply.empty())
            {
                bRet = true;
            }
            else
            {
                LOGE("hmset %s failed !", strUserKey.c_str());
            }
        }
        else
        {
            LOGE("hgetAll %s failed!", strGroupKey.c_str());
        }
        pCacheManager->relCacheConn(pCacheConn);
    }
    else
    {
        LOGE("no cache connection for unread");
    }
    return bRet;
}

/**
 *  获取群组消息列表
 *
 *  @param nUserId  用户Id
 *  @param nGroupId 群组Id
 *  @param nMsgId   开始的msgId(最新的msgId)
 *  @param nMsgCnt  获取的长度
 *  @param lsMsg    消息列表
 */
void GroupMessageModel::getMessage(uint32_t nUserId, uint32_t nGroupId, uint32_t nMsgId, uint32_t nMsgCnt, list<IM::BaseDefine::MsgInfo>& lsMsg)
{
    //根据 count 和 lastId 获取信息
    string strTableName = "IMGroupMessage_" + int2string(nGroupId % 8);
    
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if (pDBConn)
    {
        uint32_t nUpdated = GroupModel::getInstance()->getUserJoinTime(nGroupId, nUserId);
        //如果nMsgId 为0 表示客户端想拉取最新的nMsgCnt条消息
        string strSql;
        if(nMsgId == 0)
        {
            strSql = "select * from " + strTableName + " where groupId = " + int2string(nGroupId) + " and status = 0 and created>="+ int2string(nUpdated) + " order by created desc, id desc limit " + int2string(nMsgCnt);
        }else {
            strSql = "select * from " + strTableName + " where groupId = " + int2string(nGroupId) + " and msgId<=" + int2string(nMsgId) + " and status = 0 and created>="+ int2string(nUpdated) + " order by created desc, id desc limit " + int2string(nMsgCnt);
        }
        
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if (pResultSet)
        {
            map<uint32_t, IM::BaseDefine::MsgInfo> mapAudioMsg;
            while(pResultSet->next())
            {
                IM::BaseDefine::MsgInfo msg;
                msg.set_msg_id(pResultSet->getInt("msgId"));
                msg.set_from_session_id(pResultSet->getInt("userId"));
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
                    LOGE("invalid msgType. userId=%u, groupId=%u, msgType=%u", nUserId, nGroupId, nMsgType);
                }
            }
            delete pResultSet;
        }
        else
        {
            LOGE("no result set for sql: %s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
        if (!lsMsg.empty()) {
            AudioModel::getInstance()->readAudios(lsMsg);
        }
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
}

/**
 *  获取用户群未读消息计数
 *
 *  @param nUserId       用户Id
 *  @param nTotalCnt     总条数
 *  @param lsUnreadCount 每个会话的未读信息包含了条数，最后一个消息的Id，最后一个消息的类型，最后一个消息的类容
 */
void GroupMessageModel::getUnreadMsgCount(uint32_t nUserId, uint32_t &nTotalCnt, list<IM::BaseDefine::UnreadInfo>& lsUnreadCount)
{
    list<uint32_t> lsGroupId;
    GroupModel::getInstance()->getUserGroupIds(nUserId, lsGroupId, 0);
    uint32_t nCount = 0;
    
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn)
    {
        for(auto it=lsGroupId.begin(); it!=lsGroupId.end(); ++it)
        {
            uint32_t nGroupId = *it;
            string strGroupKey = int2string(nGroupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strGroupCnt = pCacheConn->hget(strGroupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
            if(strGroupCnt.empty())
            {
//                LOGE("hget %s : count failed !", strGroupKey.c_str());
                continue;
            }
            uint32_t nGroupCnt = (uint32_t)(atoi(strGroupCnt.c_str()));
            
            string strUserKey = int2string(nUserId) + "_" + int2string(nGroupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strUserCnt = pCacheConn->hget(strUserKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
            
            uint32_t nUserCnt = ( strUserCnt.empty() ? 0 : ((uint32_t)atoi(strUserCnt.c_str())) );
            if(nGroupCnt >= nUserCnt) {
                nCount = nGroupCnt - nUserCnt;
            }
            if(nCount > 0)
            {
                IM::BaseDefine::UnreadInfo cUnreadInfo;
                cUnreadInfo.set_session_id(nGroupId);
                cUnreadInfo.set_session_type(IM::BaseDefine::SESSION_TYPE_GROUP);
                cUnreadInfo.set_unread_cnt(nCount);
                nTotalCnt += nCount;
                string strMsgData;
                uint32_t nMsgId;
                IM::BaseDefine::MsgType nType;
                uint32_t nFromId;
                getLastMsg(nGroupId, nMsgId, strMsgData, nType, nFromId);
                if(IM::BaseDefine::MsgType_IsValid(nType))
                {
                    cUnreadInfo.set_latest_msg_id(nMsgId);
                    cUnreadInfo.set_latest_msg_data(strMsgData);
                    cUnreadInfo.set_latest_msg_type(nType);
                    cUnreadInfo.set_latest_msg_from_user_id(nFromId);
                    lsUnreadCount.push_back(cUnreadInfo);
                }
                else
                {
                    LOGE("invalid msgType. userId=%u, groupId=%u, msgType=%u, msgId=%u", nUserId, nGroupId, nType, nMsgId);
                }
            }
        }
        pCacheManager->relCacheConn(pCacheConn);
    }
    else
    {
        LOGE("no cache connection for unread");
    }
}

/**
 *  获取一个群组的msgId，自增，通过redis控制
 *  @param nGroupId 群Id
 *  @return 返回msgId
 */
uint32_t GroupMessageModel::getMsgId(uint32_t nGroupId)
{
    uint32_t nMsgId = 0;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if(pCacheConn)
    {
        // TODO
        string strKey = "group_msg_id_" + int2string(nGroupId);
        nMsgId = pCacheConn->incrBy(strKey, 1);
        pCacheManager->relCacheConn(pCacheConn);
    }
    else
    {
        LOGE("no cache connection for unread");
    }
    return nMsgId;
}

/**
 *  获取一个群的最后一条消息
 *
 *  @param nGroupId   群Id
 *  @param nMsgId     最后一条消息的msgId,引用
 *  @param strMsgData 最后一条消息的内容,引用
 *  @param nMsgType   最后一条消息的类型,引用
 */
void GroupMessageModel::getLastMsg(uint32_t nGroupId, uint32_t &nMsgId, string &strMsgData, IM::BaseDefine::MsgType &nMsgType, uint32_t& nFromId)
{
    string strTableName = "IMGroupMessage_" + int2string(nGroupId % 8);
    
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if (pDBConn)
    {
        string strSql = "select msgId, type,userId, content from " + strTableName + " where groupId = " + int2string(nGroupId) + " and status = 0 order by created desc, id desc limit 1";
        
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if (pResultSet)
        {
            while(pResultSet->next()) {
                nMsgId = pResultSet->getInt("msgId");
                nMsgType = IM::BaseDefine::MsgType(pResultSet->getInt("type"));
                nFromId = pResultSet->getInt("userId");
                if(nMsgType == IM::BaseDefine::MSG_TYPE_GROUP_AUDIO)
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
            LOGE("no result set for sql: %s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
}

/**
 *  获取某个用户所有群的所有未读计数之和
 *
 *  @param nUserId   用户Id
 *  @param nTotalCnt 未读计数之后,引用
 */
void GroupMessageModel::getUnReadCntAll(uint32_t nUserId, uint32_t &nTotalCnt)
{
    list<uint32_t> lsGroupId;
    GroupModel::getInstance()->getUserGroupIds(nUserId, lsGroupId, 0);
    uint32_t nCount = 0;
    
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn)
    {
        for(auto it=lsGroupId.begin(); it!=lsGroupId.end(); ++it)
        {
            uint32_t nGroupId = *it;
            string strGroupKey = int2string(nGroupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strGroupCnt = pCacheConn->hget(strGroupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
            if(strGroupCnt.empty())
            {
//                LOGE("hget %s : count failed !", strGroupKey.c_str());
                continue;
            }
            uint32_t nGroupCnt = (uint32_t)(atoi(strGroupCnt.c_str()));
            
            string strUserKey = int2string(nUserId) + "_" + int2string(nGroupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strUserCnt = pCacheConn->hget(strUserKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
            
            uint32_t nUserCnt = ( strUserCnt.empty() ? 0 : ((uint32_t)atoi(strUserCnt.c_str())) );
            if(nGroupCnt >= nUserCnt) {
                nCount = nGroupCnt - nUserCnt;
            }
            if(nCount > 0)
            {
                nTotalCnt += nCount;
            }
        }
        pCacheManager->relCacheConn(pCacheConn);
    }
    else
    {
        LOGE("no cache connection for unread");
    }
}

void GroupMessageModel::getMsgByMsgId(uint32_t nUserId, uint32_t nGroupId, const list<uint32_t> &lsMsgId, list<IM::BaseDefine::MsgInfo> &lsMsg)
{
    if(!lsMsgId.empty())
    {
        if (GroupModel::getInstance()->isInGroup(nUserId, nGroupId))
        {
            DBManager* pDBManager = DBManager::getInstance();
            DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
            if (pDBConn)
            {
                string strTableName = "IMGroupMessage_" + int2string(nGroupId % 8);
                uint32_t nUpdated = GroupModel::getInstance()->getUserJoinTime(nGroupId, nUserId);
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
                
                string strSql = "select * from " + strTableName + " where groupId=" + int2string(nGroupId) + " and msgId in (" + strClause + ") and status=0 and created >= " + int2string(nUpdated) + " order by created desc, id desc limit 100";
                ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
                if (pResultSet)
                {
                    while (pResultSet->next())
                    {
                        IM::BaseDefine::MsgInfo msg;
                        msg.set_msg_id(pResultSet->getInt("msgId"));
                        msg.set_from_session_id(pResultSet->getInt("userId"));
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
                            LOGE("invalid msgType. userId=%u, groupId=%u, msgType=%u", nUserId, nGroupId, nMsgType);
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
        else
        {
            LOGE("%u is not in group:%u", nUserId, nGroupId);
        }
    }
    else
    {
        LOGE("msgId is empty.");
    }
}

bool GroupMessageModel::resetMsgId(uint32_t nGroupId)
{
    bool bRet = false;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if(pCacheConn)
    {
        string strKey = "group_msg_id_" + int2string(nGroupId);
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
