#include <vector>

#include "../DBPool.h"
#include "RelationModel.h"
#include "MessageModel.h"
#include "GroupMessageModel.h"
#include "AsyncLog.h"
using namespace std;

RelationModel* RelationModel::m_pInstance = NULL;

RelationModel::RelationModel()
{

}

RelationModel::~RelationModel()
{

}

RelationModel* RelationModel::getInstance()
{
	if (!m_pInstance) {
		m_pInstance = new RelationModel();
	}

	return m_pInstance;
}

/**
 *  获取会话关系ID
 *  对于群组，必须把nUserBId设置为群ID
 *
 *  @param nUserAId  <#nUserAId description#>
 *  @param nUserBId  <#nUserBId description#>
 *  @param bAdd      <#bAdd description#>
 *  @param nStatus 0 获取未被删除会话，1获取所有。
 */
uint32_t RelationModel::getRelationId(uint32_t nUserAId, uint32_t nUserBId, bool bAdd)
{
    uint32_t nRelationId = INVALID_VALUE;
    if (nUserAId == 0 || nUserBId == 0) {
        LOGE("invalied user id:%u->%u", nUserAId, nUserBId);
        return nRelationId;
    }
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if (pDBConn)
    {
        uint32_t nBigId = nUserAId > nUserBId ? nUserAId : nUserBId;
        uint32_t nSmallId = nUserAId > nUserBId ? nUserBId : nUserAId;
        string strSql = "select id from IMRelationShip where smallId=" + int2string(nSmallId) + " and bigId="+ int2string(nBigId) + " and status = 0";
        
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if (pResultSet)
        {
            while (pResultSet->next())
            {
                nRelationId = pResultSet->getInt("id");
            }
            delete pResultSet;
        }
        else
        {
            LOGE("there is no result for sql:%s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
        if (nRelationId == INVALID_VALUE && bAdd)
        {
            nRelationId = addRelation(nSmallId, nBigId);
        }
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
    return nRelationId;
}

uint32_t RelationModel::addRelation(uint32_t nSmallId, uint32_t nBigId)
{
    uint32_t nRelationId = INVALID_VALUE;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if (pDBConn)
    {
        uint32_t nTimeNow = (uint32_t)time(NULL);
        string strSql = "select id from IMRelationShip where smallId=" + int2string(nSmallId) + " and bigId="+ int2string(nBigId);
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet && pResultSet->next())
        {
            nRelationId = pResultSet->getInt("id");
            strSql = "update IMRelationShip set status=0, updated=" + int2string(nTimeNow) + " where id=" + int2string(nRelationId);
            bool bRet = pDBConn->executeUpdate(strSql.c_str());
            if(!bRet)
            {
                nRelationId = INVALID_VALUE;
            }
            LOGE("has relation ship set status");
            delete pResultSet;
        }
        else
        {
            strSql = "insert into IMRelationShip (`smallId`,`bigId`,`status`,`created`,`updated`) values(?,?,?,?,?)";
            // 必须在释放连接前delete PrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
            PrepareStatement* stmt = new PrepareStatement();
            if (stmt->init(pDBConn->getMysql(), strSql))
            {
                uint32_t nStatus = 0;
                uint32_t index = 0;
                stmt->SetParam(index++, nSmallId);
                stmt->SetParam(index++, nBigId);
                stmt->SetParam(index++, nStatus);
                stmt->SetParam(index++, nTimeNow);
                stmt->SetParam(index++, nTimeNow);
                bool bRet = stmt->executeUpdate();
                if (bRet)
                {
                    nRelationId = pDBConn->getInsertId();
                }
                else
                {
                    LOGE("insert message failed. %s", strSql.c_str());
                }
            }
            if(nRelationId != INVALID_VALUE)
            {
                // 初始化msgId
                if(!MessageModel::getInstance()->resetMsgId(nRelationId))
                {
                    LOGE("reset msgId failed. smallId=%u, bigId=%u.", nSmallId, nBigId);
                }
            }
            delete stmt;
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
    return nRelationId;
}

bool RelationModel::updateRelation(uint32_t nRelationId, uint32_t nUpdateTime)
{
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if (pDBConn)
    {
        string strSql = "update IMRelationShip set `updated`="+int2string(nUpdateTime) + " where id="+int2string(nRelationId);
        bRet = pDBConn->executeUpdate(strSql.c_str());
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
    return bRet;
}

bool RelationModel::removeRelation(uint32_t nRelationId)
{
    bool bRet = false;
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if (pDBConn)
    {
        uint32_t nNow = (uint32_t) time(NULL);
        string strSql = "update IMRelationShip set status = 1, updated="+int2string(nNow)+" where id=" + int2string(nRelationId);
        bRet = pDBConn->executeUpdate(strSql.c_str());
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
    return bRet;
}
