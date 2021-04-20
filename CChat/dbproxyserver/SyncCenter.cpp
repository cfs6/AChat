#include <stdlib.h>
#include <sys/signal.h>
#include "SyncCenter.h"
//#include "Lock.h"
#include "HttpClient.h"
#include "json/json.h"
#include "DBPool.h"
#include "CachePool.h"
#include "business/Common.h"
#include "business/UserModel.h"
#include "business/GroupModel.h"
#include "business/SessionModel.h"
#include "AsyncLog.h"

static SyncCenter* SyncCenter::syncInstance = nullptr;
static bool SyncCenter::syncGroupChatRunning = false;

SyncCenter* SyncCenter::getInstance()
{
	unique_lock<mutex> lock(groupChatMutex);
	if(syncInstance = nullptr)
	{
		syncInstance = new SyncCenter();
	}
	return syncInstance;
}

void SyncCenter::getDept(uint32_t nDeptId, DBDeptInfo_t** pDept)
{

}

//开启内网数据同步以及群组聊天记录同步
void SyncCenter::startSync()
{
//    (void)pthread_create(&m_nGroupChatThreadId, NULL, doSyncGroupChat, NULL);
    thread syncThread(doSyncGtoupChat);
}

/**
 *  停止同步，为了"优雅"的同步，使用了条件变量
 */
void SyncCenter::stopSync()
{
	syncGroupChatWaiting = false;
	groupChatCond.notify_one();
	while(syncGroupChatRunning)
	{
		usleep(500);
	}
}
/*
 * 初始化函数，从cache里面加载上次同步的时间信息等
 */
void SyncCenter::init()
{
    // Load total update time
    CacheManager* pCacheManager = CacheManager::getInstance();
    // increase message count
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn)
    {
        string strTotalUpdate = pCacheConn->get("total_user_updated");

        string strLastUpdateGroup = pCacheConn->get("last_update_group");
        pCacheManager->relCacheConn(pCacheConn);
	if(strTotalUpdate != "")
        {
			lastUpdate = string2int(strTotalUpdate);
        }
        else
        {
            updateTotalUpdate(time(NULL));
        }
        if(strLastUpdateGroup.empty())
        {
        	lastUpdateGroup = string2int(strLastUpdateGroup);
        }
        else
        {
            updateLastUpdateGroup(time(NULL));
        }
    }
    else
    {
        LOGE("no cache connection to get total_user_updated");
    }
}

/**
 *  更新上次同步内网信息时间
 *
 *  @param nUpdated 时间
 */

void SyncCenter::updateTotalUpdate(uint32_t nUpdated)
{
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn) {
    	{
    		unique_lock<std::mutex> locker(lastUpdateLock);
    		lastUpdate = nUpdated;
    	}
        string strUpdated = int2string(nUpdated);
        pCacheConn->set("total_user_update", strUpdated);
        pCacheManager->relCacheConn(pCacheConn);
    }
    else
    {
        LOGE("no cache connection to get total_user_updated");
    }
}

/**
 *  更新上次同步群组信息时间
 *
 *  @param nUpdated 时间
 */
void SyncCenter::updateLastUpdateGroup(uint32_t nUpdated)
{
    CacheManager* pCacheManager = CacheManager::getInstance();
    CacheConn* pCacheConn = pCacheManager->getCacheConn("unread");
    if (pCacheConn) {
    	string strUpdated;
    	{
    		unique_lock<std::mutex> locker(lastUpdateLock);
    		lastUpdateGroup = nUpdated;
    		strUpdated = int2string(nUpdated);
    	}


        pCacheConn->set("last_update_group", strUpdated);
        pCacheManager->relCacheConn(pCacheConn);
    }
    else
    {
        LOGE("no cache connection to get total_user_updated");
    }
}

/**
 *  同步群组聊天信息
 *
 *  @param arg NULL
 *
 *  @return NULL
 */
void* SyncCenter::doSyncGroupChat(void* arg)
{
	syncGroupChatRunning = true;
    DBManager* pDBManager = DBManager::getInstance();
    map<uint32_t, uint32_t> mapChangedGroup;
    do{
        mapChangedGroup.clear();
        DBConn* pDBConn = pDBManager->getDBConn("teamtalk_slave");
        if(pDBConn)
        {
            string strSql = "select id, lastChated from IMGroup where status=0 and lastChated >=" +
            		int2string(syncInstance->getLastUpdateGroup());
            ResultSet* pResult = pDBConn->executeQuery(strSql.c_str());
            if(pResult)
            {
                while (pResult->Next()) {
                    uint32_t nGroupId = pResult->GetInt("id");
                    uint32_t nLastChat = pResult->GetInt("lastChated");
                    if(nLastChat != 0)
                    {
                        mapChangedGroup[nGroupId] = nLastChat;
                    }
                }
                delete pResult;
            }
            pDBManager->RelDBConn(pDBConn);
        }
        else
        {
            log("no db connection for teamtalk_slave");
        }
        m_pInstance->updateLastUpdateGroup(time(NULL));
        for (auto it=mapChangedGroup.begin(); it!=mapChangedGroup.end(); ++it)
        {
            uint32_t nGroupId =it->first;
            list<uint32_t> lsUsers;
            uint32_t nUpdate = it->second;
            CGroupModel::getInstance()->getGroupUser(nGroupId, lsUsers);
            for (auto it1=lsUsers.begin(); it1!=lsUsers.end(); ++it1)
            {
                uint32_t nUserId = *it1;
                uint32_t nSessionId = INVALID_VALUE;
                nSessionId = CSessionModel::getInstance()->getSessionId(nUserId, nGroupId, IM::BaseDefine::SESSION_TYPE_GROUP, true);
                if(nSessionId != INVALID_VALUE)
                {
                    CSessionModel::getInstance()->updateSession(nSessionId, nUpdate);
                }
                else
                {
                    CSessionModel::getInstance()->addSession(nUserId, nGroupId, IM::BaseDefine::SESSION_TYPE_GROUP);
                }
            }
        }
//    } while (!m_pInstance->m_pCondSync->waitTime(5*1000));
    } while (m_pInstance->m_bSyncGroupChatWaitting && !(m_pInstance->m_pCondGroupChat->waitTime(5*1000)));
//    } while(m_pInstance->m_bSyncGroupChatWaitting);
    m_bSyncGroupChatRuning = false;
    return NULL;
}









