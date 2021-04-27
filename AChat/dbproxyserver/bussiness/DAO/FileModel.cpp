#include "FileModel.h"
#include "../DBPool.h"
#include "AsyncLog.h"
FileModel* FileModel::m_pInstance = NULL;

FileModel::FileModel()
{
    
}

FileModel::~FileModel()
{
    
}

FileModel* FileModel::getInstance()
{
    if (m_pInstance == NULL) {
        m_pInstance = new FileModel();
    }
    return m_pInstance;
}

void FileModel::getOfflineFile(uint32_t userId, list<IM::BaseDefine::OfflineFileInfo>& lsOffline)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if (pDBConn)
    {
        string strSql = "select * from IMTransmitFile where toId="+int2string(userId) + " and status=0 order by created";
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->next())
            {
                IM::BaseDefine::OfflineFileInfo offlineFile;
                offlineFile.set_from_user_id(pResultSet->getInt("fromId"));
                offlineFile.set_task_id(pResultSet->getString("taskId"));
                offlineFile.set_file_name(pResultSet->getString("fileName"));
                offlineFile.set_file_size(pResultSet->getInt("size"));
                lsOffline.push_back(offlineFile);
            }
            delete pResultSet;
        }
        else
        {
            LOGE("no result for:%s", strSql.c_str());
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
}

void FileModel::addOfflineFile(uint32_t fromId, uint32_t toId, string& taskId, string& fileName, uint32_t fileSize)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if (pDBConn)
    {
        string strSql = "insert into IMTransmitFile (`fromId`,`toId`,`fileName`,`size`,`taskId`,`status`,`created`,`updated`) values(?,?,?,?,?,?,?,?)";
        
        // 必须在释放连接前delete CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
        PrepareStatement* pStmt = new PrepareStatement();
        if (pStmt->init(pDBConn->getMysql(), strSql))
        {
            uint32_t status = 0;
            uint32_t nCreated = (uint32_t)time(NULL);
            
            uint32_t index = 0;
            pStmt->setParam(index++, fromId);
            pStmt->setParam(index++, toId);
            pStmt->setParam(index++, fileName);
            pStmt->setParam(index++, fileSize);
            pStmt->setParam(index++, taskId);
            pStmt->setParam(index++, status);
            pStmt->setParam(index++, nCreated);
            pStmt->setParam(index++, nCreated);
            
            bool bRet = pStmt->executeUpdate();
            
            if (!bRet)
            {
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

void FileModel::delOfflineFile(uint32_t fromId, uint32_t toId, string& taskId)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_master");
    if (pDBConn)
    {
        string strSql = "delete from IMTransmitFile where  fromId=" + int2string(fromId) + " and toId="+int2string(toId) + " and taskId='" + taskId + "'";
        if(pDBConn->executeUpdate(strSql.c_str()))
        {
            LOGE("delete offline file success.%d->%d:%s", fromId, toId, taskId.c_str());
        }
        else
        {
            LOGE("delete offline file failed.%d->%d:%s", fromId, toId, taskId.c_str());
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_master");
    }
}
