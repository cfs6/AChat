#include "UserModel.h"
#include "../DBPool.h"
#include "../CachePool.h"
#include "Common.h"
#include "SyncCenter.h"
#include "AsyncLog.h"

UserModel* UserModel::m_pInstance = NULL;

UserModel::UserModel()
{

}

UserModel::~UserModel()
{
    
}

UserModel* UserModel::getInstance()
{
    if(m_pInstance == NULL)
    {
        m_pInstance = new UserModel();
    }
    return m_pInstance;
}

void UserModel::getChangedId(uint32_t& nLastTime, list<uint32_t> &lsIds)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("teamtalk_slave");
    if (pDBConn)
    {
        string strSql ;
        if(nLastTime == 0)
        {
            strSql = "select id, updated from IMUser where status != 3";
        }
        else
        {
            strSql = "select id, updated from IMUser where updated>=" + int2string(nLastTime);
        }
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->next()) {
                uint32_t nId = pResultSet->getInt("id");
                uint32_t nUpdated = pResultSet->getInt("updated");
        	 if(nLastTime < nUpdated)
                {
                    nLastTime = nUpdated;
                }
                lsIds.push_back(nId);
  		}
            delete pResultSet;
        }
        else
        {
            LOGE(" no result set for sql:%s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for teamtalk_slave");
    }
}

void UserModel::getUsers(list<uint32_t> lsIds, list<IM::BaseDefine::UserInfo> &lsUsers)
{
    if (lsIds.empty()) {
        LOGE("list is empty");
        return;
    }
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("teamtalk_slave");
    if (pDBConn)
    {
        string strClause;
        bool bFirst = true;
        for (auto it = lsIds.begin(); it!=lsIds.end(); ++it)
        {
            if(bFirst)
            {
                bFirst = false;
                strClause += int2string(*it);
            }
            else
            {
                strClause += ("," + int2string(*it));
            }
        }
        string  strSql = "select * from IMUser where id in (" + strClause + ")";
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->next())
            {
                IM::BaseDefine::UserInfo cUser;
                cUser.set_user_id(pResultSet->getInt("id"));
                cUser.set_user_gender(pResultSet->getInt("sex"));
                cUser.set_user_nick_name(pResultSet->getString("nick"));
                cUser.set_user_domain(pResultSet->getString("domain"));
                cUser.set_user_real_name(pResultSet->getString("name"));
                cUser.set_user_tel(pResultSet->getString("phone"));
                cUser.set_email(pResultSet->getString("email"));
                cUser.set_avatar_url(pResultSet->getString("avatar"));
		cUser.set_sign_info(pResultSet->getString("sign_info"));
             
                cUser.set_department_id(pResultSet->getInt("departId"));
  		 cUser.set_department_id(pResultSet->getInt("departId"));
                cUser.set_status(pResultSet->getInt("status"));
                lsUsers.push_back(cUser);
            }
            delete pResultSet;
        }
        else
        {
            LOGE(" no result set for sql:%s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for teamtalk_slave");
    }
}

bool UserModel::getUser(uint32_t nUserId, DBUserInfo_t &cUser)
{
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("teamtalk_slave");
    if (pDBConn)
    {
        string strSql = "select * from IMUser where id="+int2string(nUserId);
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->next())
            {
                cUser.nId = pResultSet->getInt("id");
                cUser.nSex = pResultSet->getInt("sex");
                cUser.strNick = pResultSet->getString("nick");
                cUser.strDomain = pResultSet->getString("domain");
                cUser.strName = pResultSet->getString("name");
                cUser.strTel = pResultSet->getString("phone");
                cUser.strEmail = pResultSet->getString("email");
                cUser.strAvatar = pResultSet->getString("avatar");
                cUser.sign_info = pResultSet->getString("sign_info");
                cUser.nDeptId = pResultSet->getInt("departId");
                cUser.nStatus = pResultSet->getInt("status");
                bRet = true;
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
        LOGE("no db connection for teamtalk_slave");
    }
    return bRet;
}


bool UserModel::updateUser(DBUserInfo_t &cUser)
{
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("teamtalk_master");
    if (pDBConn)
    {
        uint32_t nNow = (uint32_t)time(NULL);
        string strSql = "update IMUser set `sex`=" + int2string(cUser.nSex)+ ", `nick`='" + cUser.strNick +"', `domain`='"+ cUser.strDomain + "', `name`='" + cUser.strName + "', `phone`='" + cUser.strTel + "', `email`='" + cUser.strEmail+ "', `avatar`='" + cUser.strAvatar + "', `sign_info`='" + cUser.sign_info +"', `departId`='" + int2string(cUser.nDeptId) + "', `status`=" + int2string(cUser.nStatus) + ", `updated`="+int2string(nNow) + " where id="+int2string(cUser.nId);
        bRet = pDBConn->executeUpdate(strSql.c_str());
        if(!bRet)
        {
            LOGE("updateUser: update failed:%s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for teamtalk_master");
    }
    return bRet;
}

bool UserModel::insertUser(DBUserInfo_t &cUser)
{
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("teamtalk_master");
    if (pDBConn)
    {
        string strSql = "insert into IMUser(`id`,`sex`,`nick`,`domain`,`name`,`phone`,`email`,`avatar`,`sign_info`,`departId`,`status`,`created`,`updated`) values(?,?,?,?,?,?,?,?,?,?,?,?)";
        PrepareStatement* stmt = new PrepareStatement();
        if (stmt->init(pDBConn->getMysql(), strSql))
        {
            uint32_t nNow = (uint32_t) time(NULL);
            uint32_t index = 0;
            uint32_t nGender = cUser.nSex;
            uint32_t nStatus = cUser.nStatus;
            stmt->setParam(index++, cUser.nId);
            stmt->setParam(index++, nGender);
            stmt->setParam(index++, cUser.strNick);
            stmt->setParam(index++, cUser.strDomain);
            stmt->setParam(index++, cUser.strName);
            stmt->setParam(index++, cUser.strTel);
            stmt->setParam(index++, cUser.strEmail);
            stmt->setParam(index++, cUser.strAvatar);
            
            stmt->setParam(index++, cUser.sign_info);
            stmt->setParam(index++, cUser.nDeptId);
            stmt->setParam(index++, nStatus);
            stmt->setParam(index++, nNow);
            stmt->setParam(index++, nNow);
            bRet = stmt->executeUpdate();
            
            if (!bRet)
            {
                LOGE("insert user failed: %s", strSql.c_str());
            }
        }
        delete stmt;
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for teamtalk_master");
    }
    return bRet;
}

void UserModel::clearUserCounter(uint32_t nUserId, uint32_t nPeerId, IM::BaseDefine::SessionType nSessionType)
{
    if(IM::BaseDefine::SessionType_IsValid(nSessionType))
    {
        CacheManager* pCacheManager = CacheManager::getInstance();
        CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
        if (pCacheConn)
        {
            // Clear P2P msg Counter
            if(nSessionType == IM::BaseDefine::SESSION_TYPE_SINGLE)
            {
                int nRet = pCacheConn->hdel("unread_" + int2string(nUserId), int2string(nPeerId));
                if(!nRet)
                {
                    LOGE("hdel failed %d->%d", nPeerId, nUserId);
                }
            }
            // Clear Group msg Counter
            else if(nSessionType == IM::BaseDefine::SESSION_TYPE_GROUP)
            {
                string strGroupKey = int2string(nPeerId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
                map<string, string> mapGroupCount;
                bool bRet = pCacheConn->hgetAll(strGroupKey, mapGroupCount);
                if(bRet)
                {
                    string strUserKey = int2string(nUserId) + "_" + int2string(nPeerId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
                    string strReply = pCacheConn->hmset(strUserKey, mapGroupCount);
                    if(strReply.empty()) {
                        LOGE("hmset %s failed !", strUserKey.c_str());
                    }
                }
                else
                {
                    LOGE("hgetall %s failed!", strGroupKey.c_str());
                }
                
            }
            pCacheManager->relCacheConn(pCacheConn);
        }
        else
        {
            LOGE("no cache connection for unread");
        }
    }
    else{
        LOGE("invalid sessionType. userId=%u, fromId=%u, sessionType=%u", nUserId, nPeerId, nSessionType);
    }
}

void UserModel::setCallReport(uint32_t nUserId, uint32_t nPeerId, IM::BaseDefine::ClientType nClientType)
{
    if(IM::BaseDefine::ClientType_IsValid(nClientType))
    {
        DBManager* pDBManager = DBManager::getInstance();
        DBConn* pDBConn = pDBManager->getDBConn("teamtalk_master");
        if(pDBConn)
        {
            string strSql = "insert into IMCallLOGE(`userId`, `peerId`, `clientType`,`created`,`updated`) values(?,?,?,?,?)";
            PrepareStatement* stmt = new PrepareStatement();
            if (stmt->init(pDBConn->getMysql(), strSql))
            {
                uint32_t nNow = (uint32_t) time(NULL);
                uint32_t index = 0;
                uint32_t nClient = (uint32_t) nClientType;
                stmt->setParam(index++, nUserId);
                stmt->setParam(index++, nPeerId);
                stmt->setParam(index++, nClient);
                stmt->setParam(index++, nNow);
                stmt->setParam(index++, nNow);
                bool bRet = stmt->executeUpdate();
                
                if (!bRet)
                {
                    LOGE("insert report failed: %s", strSql.c_str());
                }
            }
            delete stmt;
            pDBManager->relDBConn(pDBConn);
        }
        else
        {
            LOGE("no db connection for teamtalk_master");
        }
        
    }
    else
    {
        LOGE("invalid clienttype. userId=%u, peerId=%u, clientType=%u", nUserId, nPeerId, nClientType);
    }
}


bool UserModel::updateUserSignInfo(uint32_t user_id, const string& sign_info) {
   
    if (sign_info.length() > 128) {
        LOGE("updateUserSignInfo: sign_info.length()>128.\n");
        return false;
    }
    bool rv = false;
    DBManager* db_manager = DBManager::getInstance();
    DBConn* db_conn = db_manager->getDBConn("teamtalk_master");
    if (db_conn) {
        uint32_t now = (uint32_t)time(NULL);
        string str_sql = "update IMUser set `sign_info`='" + sign_info + "', `updated`=" + int2string(now) + " where id="+int2string(user_id);
        rv = db_conn->executeUpdate(str_sql.c_str());
        if(!rv) {
            LOGE("updateUserSignInfo: update failed:%s", str_sql.c_str());
        }else{
                SyncCenter::getInstance()->updateTotalUpdate(now);
           
        }
        db_manager->relDBConn(db_conn);
        } else {
            LOGE("updateUserSignInfo: no db connection for teamtalk_master");
            }
    return rv;
    }

bool UserModel::getUserSingInfo(uint32_t user_id, string* sign_info) {
    bool rv = false;
    DBManager* db_manager = DBManager::getInstance();
    DBConn* db_conn = db_manager->getDBConn("teamtalk_slave");
    if (db_conn) {
        string str_sql = "select sign_info from IMUser where id="+int2string(user_id);
        ResultSet* result_set = db_conn->executeQuery(str_sql.c_str());
        if(result_set) {
            if (result_set->next()) {
                *sign_info = result_set->getString("sign_info");
                rv = true;
                }
            delete result_set;
            } else {
                        LOGE("no result set for sql:%s", str_sql.c_str());
                   }
                db_manager->relDBConn(db_conn);
        } else {
                    LOGE("no db connection for teamtalk_slave");
               }
    return rv;
   }

bool UserModel::updatePushShield(uint32_t user_id, uint32_t shield_status) {
    bool rv = false;
    
    DBManager* db_manager = DBManager::getInstance();
    DBConn* db_conn = db_manager->getDBConn("teamtalk_master");
    if (db_conn) {
        uint32_t now = (uint32_t)time(NULL);
        string str_sql = "update IMUser set `push_shield_status`="+ int2string(shield_status) + ", `updated`=" + int2string(now) + " where id="+int2string(user_id);
        rv = db_conn->executeUpdate(str_sql.c_str());
        if(!rv) {
            LOGE("updatePushShield: update failed:%s", str_sql.c_str());
        }
        db_manager->relDBConn(db_conn);
    } else {
        LOGE("updatePushShield: no db connection for teamtalk_master");
    }
    
    return rv;
}

bool UserModel::getPushShield(uint32_t user_id, uint32_t* shield_status) {
    bool rv = false;
    
    DBManager* db_manager = DBManager::getInstance();
    DBConn* db_conn = db_manager->getDBConn("teamtalk_slave");
    if (db_conn) {
        string str_sql = "select push_shield_status from IMUser where id="+int2string(user_id);
        ResultSet* result_set = db_conn->executeQuery(str_sql.c_str());
        if(result_set) {
            if (result_set->next()) {
                *shield_status = result_set->getInt("push_shield_status");
                rv = true;
            }
            delete result_set;
        } else {
            LOGE("getPushShield: no result set for sql:%s", str_sql.c_str());
        }
        db_manager->relDBConn(db_conn);
    } else {
        LOGE("getPushShield: no db connection for teamtalk_slave");
    }
    
    return rv;
}

