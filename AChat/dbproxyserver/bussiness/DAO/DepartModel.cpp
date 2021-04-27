#include "DepartModel.h"
#include "../DBPool.h"
#include "AsyncLog.h"
CDepartModel* CDepartModel::m_pInstance = NULL;

CDepartModel* CDepartModel::getInstance()
{
    if(NULL == m_pInstance)
    {
        m_pInstance = new CDepartModel();
    }
    return m_pInstance;
}

void CDepartModel::getChgedDeptId(uint32_t& nLastTime, list<uint32_t>& lsChangedIds)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if (pDBConn)
    {
        string strSql = "select id, updated from IMDepart where updated > " + int2string(nLastTime);
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->next()) {
                uint32_t id = pResultSet->getInt("id");
                uint32_t nUpdated = pResultSet->getInt("updated");
                if(nLastTime < nUpdated)
                {
                    nLastTime = nUpdated;
                }
                lsChangedIds.push_back(id);
            }
            delete  pResultSet;
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_slave.");
    }
}

void CDepartModel::getDepts(list<uint32_t>& lsDeptIds, list<IM::BaseDefine::DepartInfo>& lsDepts)
{
    if(lsDeptIds.empty())
    {
        LOGE("list is empty");
        return;
    }
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if (pDBConn)
    {
        string strClause;
        bool bFirst = true;
        for (auto it=lsDeptIds.begin(); it!=lsDeptIds.end(); ++it) {
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
        string strSql = "select * from IMDepart where id in ( " + strClause + " )";
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->next()) {
                IM::BaseDefine::DepartInfo cDept;
                uint32_t nId = pResultSet->getInt("id");
                uint32_t nParentId = pResultSet->getInt("parentId");
                string strDeptName = pResultSet->getString("departName");
                uint32_t nStatus = pResultSet->getInt("status");
                uint32_t nPriority = pResultSet->getInt("priority");
                if(IM::BaseDefine::DepartmentStatusType_IsValid(nStatus))
                {
                    cDept.set_dept_id(nId);
                    cDept.set_parent_dept_id(nParentId);
                    cDept.set_dept_name(strDeptName);
                    cDept.set_dept_status(IM::BaseDefine::DepartmentStatusType(nStatus));
                    cDept.set_priority(nPriority);
                    lsDepts.push_back(cDept);
                }
            }
            delete  pResultSet;
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
}

void CDepartModel::getDept(uint32_t nDeptId, IM::BaseDefine::DepartInfo& cDept)
{
    DBManager* pDBManager = DBManager::getInstance();
    DBConn* pDBConn = pDBManager->getDBConn("AChat_slave");
    if (pDBConn)
    {
        string strSql = "select * from IMDepart where id = " + int2string(nDeptId);
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->next()) {
                uint32_t nId = pResultSet->getInt("id");
                uint32_t nParentId = pResultSet->getInt("parentId");
                string strDeptName = pResultSet->getString("departName");
                uint32_t nStatus = pResultSet->getInt("status");
                uint32_t nPriority = pResultSet->getInt("priority");
                if(IM::BaseDefine::DepartmentStatusType_IsValid(nStatus))
                {
                    cDept.set_dept_id(nId);
                    cDept.set_parent_dept_id(nParentId);
                    cDept.set_dept_name(strDeptName);
                    cDept.set_dept_status(IM::BaseDefine::DepartmentStatusType(nStatus));
                    cDept.set_priority(nPriority);
                }
            }
            delete  pResultSet;
        }
        pDBManager->relDBConn(pDBConn);
    }
    else
    {
        LOGE("no db connection for AChat_slave");
    }
}
