#include "../DBPool.h"
#include "../CachePool.h"

#include "GroupModel.h"
#include "ImPduBase.h"
#include "Common.h"
#include "AudioModel.h"
#include "UserModel.h"
#include "GroupMessageModel.h"
#include "public_define.h"
#include "SessionModel.h"
#include "AsyncLog.h"
GroupModel* GroupModel::m_pInstance = NULL;

/**
 *  <#Description#>
 */
GroupModel::GroupModel()
{
    
}

GroupModel::~GroupModel()
{
    
}

GroupModel* GroupModel::getInstance()
{
    if (!m_pInstance) {
        m_pInstance = new GroupModel();
    }
    return m_pInstance;
}

/**
 *  创建群
 *
 *  @param nUserId        创建者
 *  @param strGroupName   群名
 *  @param strGroupAvatar 群头像
 *  @param nGroupType     群类型1,固定群;2,临时群
 *  @param setMember      群成员列表，为了踢出重复的userId，使用set存储
 *
 *  @return 成功返回群Id，失败返回0;
 */
uint32_t GroupModel::createGroup(uint32_t nUserId, const string& strGroupName, const string& strGroupAvatar, uint32_t nGroupType, set<uint32_t>& setMember)
{
    uint32_t nGroupId = INVALID_VALUE;
    do {
        if(strGroupName.empty()) {
            break;
        }
        if (setMember.empty()) {
            break;
        }
        // remove repeat user
        
        
        //insert IMGroup
        if(!insertNewGroup(nUserId, strGroupName, strGroupAvatar, nGroupType, (uint32_t)setMember.size(), nGroupId)) {
            break;
        }
        bool bRet = GroupMessageModel::getInstance()->resetMsgId(nGroupId);
        if(!bRet)
        {
            LOGE("reset msgId failed. groupId=%u", nGroupId);
        }
        
        //insert IMGroupMember
        clearGroupMember(nGroupId);
        insertNewMember(nGroupId, setMember);
        
    } while (false);
    
    return nGroupId;
}

bool GroupModel::removeGroup(uint32_t nUserId, uint32_t nGroupId, list<uint32_t>& lsCurUserId)
{
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    set<uint32_t> setGroupUsers;
    if(pDBConn)
    {
        string strSql = "select creator from IMGroup where id="+int2string(nGroupId);
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            uint32_t nCreator;
            while (pResultSet->next()) {
                nCreator = pResultSet->getInt("creator");
            }
            
            if(0 == nCreator || nCreator == nUserId)
            {
                //设置群组不可用。
                strSql = "update IMGroup set status=0 where id="+int2string(nGroupId);
                bRet = pDBConn->executeUpdate(strSql.c_str());
            }
            delete  pResultSet;
        }
        
        if (bRet) {
            strSql = "select userId from IMGroupMember where groupId="+int2string(nGroupId);
            ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
            if(pResultSet)
            {
                while (pResultSet->next()) {
                    uint32_t nId = pResultSet->getInt("userId");
                    setGroupUsers.insert(nId);
                }
                delete pResultSet;
            }
        }
        pDBManager->relDBConn(pDBConn);
    }
    
    if(bRet)
    {
        bRet = removeMember(nGroupId, setGroupUsers, lsCurUserId);
    }
    
    return bRet;
}


void GroupModel::getUserGroup(uint32_t nUserId, list<IM::BaseDefine::GroupVersionInfo>& lsGroup, uint32_t nGroupType)
{
    list<uint32_t> lsGroupId;
    getUserGroupIds(nUserId, lsGroupId,0);
    if(lsGroupId.size() != 0)
    {
        getGroupVersion(lsGroupId, lsGroup, nGroupType);
    }
}


void GroupModel::getGroupInfo(map<uint32_t,IM::BaseDefine::GroupVersionInfo>& mapGroupId, list<IM::BaseDefine::GroupInfo>& lsGroupInfo)
{ 
   if (!mapGroupId.empty())
    {
       DBManager* pDBManager = DBManager::getInstance();
        DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
        if (pDBConn)
        {
            string strClause;
            bool bFirst = true;
            for(auto it=mapGroupId.begin(); it!=mapGroupId.end(); ++it)
            {
                if(bFirst)
                {
                    bFirst = false;
                    strClause = int2string(it->first);
                }
                else
                {
                    strClause += ("," + int2string(it->first));
                }
            }
            string strSql = "select * from IMGroup where id in (" + strClause  + ") order by updated desc";
            ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
            if(pResultSet)
            {
                while (pResultSet->next()) {
                    uint32_t nGroupId = pResultSet->getInt("id");
                    uint32_t nVersion = pResultSet->getInt("version");
               if(mapGroupId[nGroupId].version() < nVersion)
                    {
                        IM::BaseDefine::GroupInfo cGroupInfo;
                        cGroupInfo.set_group_id(nGroupId);
                        cGroupInfo.set_version(nVersion);
                        cGroupInfo.set_group_name(pResultSet->getString("name"));
                        cGroupInfo.set_group_avatar(pResultSet->getString("avatar"));
                        IM::BaseDefine::GroupType nGroupType = IM::BaseDefine::GroupType(pResultSet->getInt("type"));
                        if(IM::BaseDefine::GroupType_IsValid(nGroupType))
                        {
                            cGroupInfo.set_group_type(nGroupType);
                            cGroupInfo.set_group_creator_id(pResultSet->getInt("creator"));
                            lsGroupInfo.push_back(cGroupInfo);
                        }
                        else
                        {
                            LOGE("invalid groupType. groupId=%u, groupType=%u", nGroupId, nGroupType);
                        }
                    }
                }
                delete pResultSet;
            }
            else
            {
                LOGE("no result set for sql:%s", strSql.c_str());
            }
            pDBManager->relDBConn(pDBConn);
       if(!lsGroupInfo.empty())
            {
                fillGroupMember(lsGroupInfo);
            }
        }
        else
        {
            LOGE("no db connection for AChat_slave");
        }
    }
    else
    {
        LOGE("no ids in map");
    }
}

bool GroupModel::modifyGroupMember(uint32_t nUserId, uint32_t nGroupId, IM::BaseDefine::GroupModifyType nType, set<uint32_t>& setUserId, list<uint32_t>& lsCurUserId)
{
    bool bRet = false;
    if(hasModifyPermission(nUserId, nGroupId, nType))
    {
        switch (nType) {
            case IM::BaseDefine::GROUP_MODIFY_TYPE_ADD:
                bRet = addMember(nGroupId, setUserId, lsCurUserId);
                break;
            case IM::BaseDefine::GROUP_MODIFY_TYPE_DEL:
                bRet = removeMember(nGroupId, setUserId, lsCurUserId);
                removeSession(nGroupId, setUserId);
                break;
            default:
                LOGE("unknown type:%u while modify group.%u->%u", nType, nUserId, nGroupId);
                break;
        }
        //if modify group member success, need to inc the group version and clear the user count;
        if(bRet)
        {
            incGroupVersion(nGroupId);
            for (auto it=setUserId.begin(); it!=setUserId.end(); ++it) {
                uint32_t nUserId=*it;
                UserModel::getInstance()->clearUserCounter(nUserId, nGroupId, IM::BaseDefine::SESSION_TYPE_GROUP);
            }
        }
    }
    else
    {
        LOGE("user:%u has no permission to modify group:%u", nUserId, nGroupId);
    }    return bRet;
}

bool GroupModel::insertNewGroup(uint32_t nUserId, const string& strGroupName, const string& strGroupAvatar, uint32_t nGroupType, uint32_t nMemberCnt, uint32_t& nGroupId)
{
    bool bRet = false;
    nGroupId = INVALID_VALUE;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if (pDBConn)
    {
        string strSql = "insert into IMGroup(`name`, `avatar`, `creator`, `type`,`userCnt`, `status`, `version`, `lastChated`, `updated`, `created`) "\
        "values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        
        PrepareStatement* pStmt = new PrepareStatement();
        if (pStmt->init(pDBConn->getMysql(), strSql))
        {
            uint32_t nCreated = (uint32_t)time(NULL);
            uint32_t index = 0;
            uint32_t nStatus = 0;
            uint32_t nVersion = 1;
            uint32_t nLastChat = 0;
            pStmt->setParam(index++, strGroupName);
            pStmt->setParam(index++, strGroupAvatar);
            pStmt->setParam(index++, nUserId);
            pStmt->setParam(index++, nGroupType);
            pStmt->setParam(index++, nMemberCnt);
            pStmt->setParam(index++, nStatus);
            pStmt->setParam(index++, nVersion);
            pStmt->setParam(index++, nLastChat);
            pStmt->setParam(index++, nCreated);
            pStmt->setParam(index++, nCreated);
            
            bRet = pStmt->executeUpdate();
            if(bRet) {
                nGroupId = pStmt->getInsertId();
            }
        }
        delete pStmt;
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
    return bRet;
}

bool GroupModel::insertNewMember(uint32_t nGroupId, set<uint32_t>& setUsers)
{
    bool bRet = false;
    uint32_t nUserCnt = (uint32_t)setUsers.size();
    if(nGroupId != INVALID_VALUE &&  nUserCnt > 0)
    {
        DBManager* pDBManager = DBManager::getInstance();
        DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
        if (pDBConn)
        {
            uint32_t nCreated = (uint32_t)time(NULL);
            // 获取 已经存在群里的用户
            string strClause;
            bool bFirst = true;
            for (auto it=setUsers.begin(); it!=setUsers.end(); ++it)
            {
                if(bFirst)
                {
                    bFirst = false;
                    strClause = int2string(*it);
                }
                else
                {
                    strClause += ("," + int2string(*it));
                }
            }
            string strSql = "select userId from IMGroupMember where groupId=" + int2string(nGroupId) + " and userId in (" + strClause + ")";
            ResultSet* pResult = pDBConn->executeQuery(strSql.c_str());
            set<uint32_t> setHasUser;
            if(pResult)
            {
                while (pResult->next()) {
                    setHasUser.insert(pResult->getInt("userId"));
                }
                delete pResult;
            }
            else
            {
                LOGE("no result for sql:%s", strSql.c_str());
            }
            pDBManager->relDBConn(pDBConn);
            
            pDBConn = pDBManager->getDBConn("AChat_master");
            if (pDBConn)
            {
                CacheManager* pCacheManager = CacheManager::getInstance();
                CacheConn* pCacheConn = pCacheManager->getCacheConn("group_member");
                if (pCacheConn)
                {
                    // 设置已经存在群中人的状态
                    if (!setHasUser.empty())
                    {
                        strClause.clear();
                        bFirst = true;
                        for (auto it=setHasUser.begin(); it!=setHasUser.end(); ++it) {
                            if(bFirst)
                            {
                                bFirst = false;
                                strClause = int2string(*it);
                            }
                            else
                            {
                                strClause += ("," + int2string(*it));
                            }
                        }
                        
                        strSql = "update IMGroupMember set status=0, updated="+int2string(nCreated)+" where groupId=" + int2string(nGroupId) + " and userId in (" + strClause + ")";
                        pDBConn->executeUpdate(strSql.c_str());
                    }
                    strSql = "insert into IMGroupMember(`groupId`, `userId`, `status`, `created`, `updated`) values\
                    (?,?,?,?,?)";
                    
                    //插入新成员
                    auto it = setUsers.begin();
                    uint32_t nStatus = 0;
                    uint32_t nIncMemberCnt = 0;
                    for (;it != setUsers.end();)
                    {
                        uint32_t nUserId = *it;
                        if(setHasUser.find(nUserId) == setHasUser.end())
                        {
                            PrepareStatement* pStmt = new PrepareStatement();
                            if (pStmt->init(pDBConn->getMysql(), strSql))
                            {
                                uint32_t index = 0;
                                pStmt->setParam(index++, nGroupId);
                                pStmt->setParam(index++, nUserId);
                                pStmt->setParam(index++, nStatus);
                                pStmt->setParam(index++, nCreated);
                                pStmt->setParam(index++, nCreated);
                                pStmt->executeUpdate();
                                ++nIncMemberCnt;
                                delete pStmt;
                            }
                            else
                            {
                                setUsers.erase(it++);
                                delete pStmt;
                                continue;
                            }
                        }
                        ++it;
                    }
                    if(nIncMemberCnt != 0)
                    {
                        strSql = "update IMGroup set userCnt=userCnt+" + int2string(nIncMemberCnt) + " where id="+int2string(nGroupId);
                        pDBConn->executeUpdate(strSql.c_str());
                    }
                    
                    //更新一份到redis中
                    string strKey = "group_member_"+int2string(nGroupId);
                    for(auto it = setUsers.begin(); it!=setUsers.end(); ++it)
                    {
                        pCacheConn->hset(strKey, int2string(*it), int2string(nCreated));
                    }
                    pCacheManager->relCacheConn(pCacheConn);
                    bRet = true;
                }
                else
                {
                    LOGE("no cache connection");
                }
                pDBManager->relDBConn(pDBConn);
            }
            else
            {
                LOGE("no db connection for AChat_master");
            }
        }
        else
        {
            LOGE("no db connection for AChat_slave");
        }
    }
    return bRet;
}

void GroupModel::getUserGroupIds(uint32_t nUserId, list<uint32_t>& lsGroupId, uint32_t nLimited)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if(pDBConn)
    {
        string strSql ;
        if (nLimited != 0) {
            strSql = "select groupId from IMGroupMember where userId=" + int2string(nUserId) + " and status = 0 order by updated desc, id desc limit " + int2string(nLimited);
        }
        else
        {
            strSql = "select groupId from IMGroupMember where userId=" + int2string(nUserId) + " and status = 0 order by updated desc, id desc";
        }
        
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while(pResultSet->next())
            {
                uint32_t nGroupId = pResultSet->getInt("groupId");
                lsGroupId.push_back(nGroupId);
            }
            delete pResultSet;
        }
        else
        {
            LOGE("no result set for sql:%s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
}

void GroupModel::getGroupVersion(list<uint32_t> &lsGroupId, list<IM::BaseDefine::GroupVersionInfo> &lsGroup, uint32_t nGroupType)
{
    if(!lsGroupId.empty())
    {
        DBManager* pDBManager = DBManager::getInstance();
        DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
        if(pDBConn)
        {
            string strClause;
            bool bFirst = true;
            for(list<uint32_t>::iterator it=lsGroupId.begin(); it!=lsGroupId.end(); ++it)
            {
                if(bFirst)
                {
                    bFirst = false;
                    strClause = int2string(*it);
                }
                else
                {
                    strClause += ("," + int2string(*it));
                }
            }
            
            string strSql = "select id,version from IMGroup where id in (" +  strClause  + ")";
            if(0 != nGroupType)
            {
                strSql += " and type="+int2string(nGroupType);
            }
            strSql += " order by updated desc";
            
            ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
            if(pResultSet)
            {
                while(pResultSet->next())
                {
                    IM::BaseDefine::GroupVersionInfo group;
                    group.set_group_id(pResultSet->getInt("id"));
                    group.set_version(pResultSet->getInt("version"));
                    lsGroup.push_back(group);
                }
                delete pResultSet;
            }
            else
            {
                LOGE("no result set for sql:%s", strSql.c_str());
            }
            pDBManager->relDBConn(pDBConn);
        }
        else
        {
            LOGE("no db connection for AChat_slave");
        }
    }
    else
    {
        LOGE("group ids is empty");
    }
}

bool GroupModel::isInGroup(uint32_t nUserId, uint32_t nGroupId)
{
    bool bRet = false;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("group_member");
    if (pCacheConn)
    {
        string strKey = "group_member_" + int2string(nGroupId);
        string strField = int2string(nUserId);
        string strValue = pCacheConn->hget(strKey, strField);
        pCacheManager->relCacheConn(pCacheConn);
        if(!strValue.empty())
        {
            bRet = true;
        }
    }
    else
    {
        LOGE("no cache connection for group_member");
    }
    return bRet;
}

bool GroupModel::hasModifyPermission(uint32_t nUserId, uint32_t nGroupId, IM::BaseDefine::GroupModifyType nType)
{
    if(nUserId == 0) {
        return true;
    }
    
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if(pDBConn)
    {
        string strSql = "select creator, type from IMGroup where id="+ int2string(nGroupId);
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->next())
            {
                uint32_t nCreator = pResultSet->getInt("creator");
                IM::BaseDefine::GroupType nGroupType = IM::BaseDefine::GroupType(pResultSet->getInt("type"));
                if(IM::BaseDefine::GroupType_IsValid(nGroupType))
                {
                    if(IM::BaseDefine::GROUP_TYPE_TMP == nGroupType && IM::BaseDefine::GROUP_MODIFY_TYPE_ADD == nType)
                    {
                        bRet = true;
                        break;
                    }
                    else
                    {
                        if(nCreator == nUserId)
                        {
                            bRet = true;
                            break;
                        }
                    }
                }
            }
            delete pResultSet;
        }
        else
        {
            LOGE("no result for sql:%s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
    return bRet;
}

bool GroupModel::addMember(uint32_t nGroupId, set<uint32_t> &setUser, list<uint32_t>& lsCurUserId)
{
    // 去掉已经存在的用户ID
    removeRepeatUser(nGroupId, setUser);
    bool bRet = insertNewMember(nGroupId, setUser);
    getGroupUser(nGroupId,lsCurUserId);
    return bRet;
}

bool GroupModel::removeMember(uint32_t nGroupId, set<uint32_t> &setUser, list<uint32_t>& lsCurUserId)
{
    if(setUser.size() <= 0)
    {
        return true;
    }
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if(pDBConn)
    {
        CacheManager* pCacheManager = CacheManager::getInstance();
        CacheConn* pCacheConn = pCacheManager->getCacheConn("group_member");
        if (pCacheConn)
        {
            string strClause ;
            bool bFirst = true;
            for(auto it= setUser.begin(); it!=setUser.end();++it)
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
            string strSql = "update IMGroupMember set status=1 where  groupId =" + int2string(nGroupId) + " and userId in(" + strClause + ")";
            pDBConn->executeUpdate(strSql.c_str());
            
            //从redis中删除成员
            string strKey = "group_member_"+ int2string(nGroupId);
            for (auto it=setUser.begin(); it!=setUser.end(); ++it) {
                string strField = int2string(*it);
                pCacheConn->hdel(strKey, strField);
            }
            pCacheManager->relCacheConn(pCacheConn);
            bRet = true;
        }
        else
        {
            LOGE("no cache connection");
        }
        pDBManager->relDBConn(pDBConn);
        if (bRet)
        {
            getGroupUser(nGroupId,lsCurUserId);
        }
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
    return bRet;
}

void GroupModel::removeRepeatUser(uint32_t nGroupId, set<uint32_t> &setUser)
{
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("group_member");
    if (pCacheConn)
    {
        string strKey = "group_member_"+int2string(nGroupId);
        for (auto it=setUser.begin(); it!=setUser.end();) {
            string strField = int2string(*it);
            string strValue = pCacheConn->hget(strKey, strField);
            pCacheManager->relCacheConn(pCacheConn);
            if(!strValue.empty())
            {
                setUser.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }
    else
    {
        LOGE("no cache connection for group_member");
    }
}

bool GroupModel::setPush(uint32_t nUserId, uint32_t nGroupId, uint32_t nType, uint32_t nStatus)
{
    bool bRet = false;
    if(!isInGroup(nUserId, nGroupId))
    {
        LOGE("user:%d is not in group:%d", nUserId, nGroupId);
        return bRet;;
    }
    
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("group_set");
    if(pCacheConn)
    {
        string strGroupKey = "group_set_" + int2string(nGroupId);
        string strField = int2string(nUserId) + "_" + int2string(nType);
        int nRet = pCacheConn->hset(strGroupKey, strField, int2string(nStatus));
        pCacheManager->relCacheConn(pCacheConn);
        if(nRet != -1)
        {
            bRet = true;
        }
    }
    else
    {
        LOGE("no cache connection for group_set");
    }
    return bRet;
}

void GroupModel::getPush(uint32_t nGroupId, list<uint32_t>& lsUser, list<IM::BaseDefine::ShieldStatus>& lsPush)
{
    if (lsUser.empty()) {
        return;
    }
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("group_set");
    if(pCacheConn)
    {
        string strGroupKey = "group_set_" + int2string(nGroupId);
        map<string, string> mapResult;
        bool bRet = pCacheConn->hgetAll(strGroupKey, mapResult);
        pCacheManager->relCacheConn(pCacheConn);
        if(bRet)
        {
            for(auto it=lsUser.begin(); it!=lsUser.end(); ++it)
            {
                string strField = int2string(*it) + "_" + int2string(IM_GROUP_SETTING_PUSH);
                auto itResult = mapResult.find(strField);
                IM::BaseDefine::ShieldStatus status;
                status.set_group_id(nGroupId);
                status.set_user_id(*it);
                if(itResult != mapResult.end())
                {
                    status.set_shield_status(string2int(itResult->second));
                }
                else
                {
                    status.set_shield_status(0);
                }
                lsPush.push_back(status);
            }
        }
        else
        {
            LOGE("hgetall %s failed!", strGroupKey.c_str());
        }
    }
    else
    {
        LOGE("no cache connection for group_set");
    }
}

void GroupModel::getGroupUser(uint32_t nGroupId, list<uint32_t> &lsUserId)
{
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("group_member");
    if (pCacheConn)
    {
        string strKey = "group_member_" + int2string(nGroupId);
        map<string, string> mapAllUser;
        bool bRet = pCacheConn->hgetAll(strKey, mapAllUser);
        pCacheManager->relCacheConn(pCacheConn);
        if(bRet)
        {
            for (auto it=mapAllUser.begin(); it!=mapAllUser.end(); ++it) {
                uint32_t nUserId = string2int(it->first);
                lsUserId.push_back(nUserId);
            }
        }
        else
        {
            LOGE("hgetall %s failed!", strKey.c_str());
        }
    }
    else
    {
        LOGE("no cache connection for group_member");
    }
}

void GroupModel::updateGroupChat(uint32_t nGroupId)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if(pDBConn)
    {
        uint32_t nNow = (uint32_t)time(NULL);
        string strSql = "update IMGroup set lastChated=" + int2string(nNow) + " where id=" + int2string(nGroupId);
        pDBConn->executeUpdate(strSql.c_str());
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
}

//bool GroupModel::isValidateGroupId(uint32_t nGroupId)
//{
//    bool bRet = false;
//    DBManager* pDBManager = DBManager::getInstance();
//    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
//    if(pDBConn)
//    {
//        string strSql = "select id from IMGroup where id=" + int2string(nGroupId)+" and status=0";
//        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
//        if(pResultSet && pResultSet->next())
//        {
//            bRet =  true;
//            delete pResultSet;
//        }
//        pDBManager->relDBConn(pDBConn);
//    }
//    else
//    {
//        LOGE("no db connection for AChat_slave");
//    }
//    return bRet;
//}

bool GroupModel::isValidateGroupId(uint32_t nGroupId)
{
    bool bRet = false;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("group_member");
    if(pCacheConn)
    {
        string strKey = "group_member_"+int2string(nGroupId);
        bRet = pCacheConn->isExists(strKey);
        pCacheManager->relCacheConn(pCacheConn);
    }
    return bRet;
}


void GroupModel::removeSession(uint32_t nGroupId, const set<uint32_t> &setUser)
{
    for(auto it=setUser.begin(); it!=setUser.end(); ++it)
    {
        uint32_t nUserId=*it;
        uint32_t nSessionId = SessionModel::getInstance()->getSessionId(nUserId, nGroupId, IM::BaseDefine::SESSION_TYPE_GROUP, false);
        SessionModel::getInstance()->removeSession(nSessionId);
    }
}

bool GroupModel::incGroupVersion(uint32_t nGroupId)
{
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if(pDBConn)
    {
        string strSql = "update IMGroup set version=version+1 where id="+int2string(nGroupId);
        if(pDBConn->executeUpdate(strSql.c_str()))
        {
            bRet = true;
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
    return  bRet;
}


void GroupModel::fillGroupMember(list<IM::BaseDefine::GroupInfo>& lsGroups)
{
    for (auto it=lsGroups.begin(); it!=lsGroups.end(); ++it) {
        list<uint32_t> lsUserIds;
        uint32_t nGroupId = it->group_id();
        getGroupUser(nGroupId, lsUserIds);
        for(auto itUserId=lsUserIds.begin(); itUserId!=lsUserIds.end(); ++itUserId)
        {
            it->add_group_member_list(*itUserId);
        }
    }
}

uint32_t GroupModel::getUserJoinTime(uint32_t nGroupId, uint32_t nUserId)
{
    uint32_t nTime = 0;
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("group_member");
    if (pCacheConn)
    {
        string strKey = "group_member_" + int2string(nGroupId);
        string strField = int2string(nUserId);
        string strValue = pCacheConn->hget(strKey, strField);
        pCacheManager->relCacheConn(pCacheConn);
        if (!strValue.empty()) {
            nTime = string2int(strValue);
        }
    }
    else
    {
        LOGE("no cache connection for group_member");
    }
    return  nTime;
}

void GroupModel::clearGroupMember(uint32_t nGroupId)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if(pDBConn)
    {
        string strSql = "delete from IMGroupMember where groupId="+int2string(nGroupId);
        pDBConn->executeUpdate(strSql.c_str());
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("group_member");
    if(pCacheConn)
    {
        string strKey = "group_member_" + int2string(nGroupId);
        map<string, string> mapRet;
        bool bRet = pCacheConn->hgetAll(strKey, mapRet);
        if(bRet)
        {
            for(auto it=mapRet.begin(); it!=mapRet.end(); ++it)
            {
                pCacheConn->hdel(strKey, it->first);
            }
        }
        else
        {
            LOGE("hgetall %s failed", strKey.c_str());
        }
        pCacheManager->relCacheConn(pCacheConn);
    }
    else
    {
        LOGE("no cache connection for group_member");
    }
}
