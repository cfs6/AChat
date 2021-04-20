/*
 * SyncCenter.h
 *
 *  Created on: 2021年3月24日
 *      Author: cfs
 */

#ifndef SyncCenter_H_
#define SyncCenter_H_


#include <list>
#include <map>

#include "ostype.h"
//#include "Lock.h"
#include <condition_variable>
#include <mutex>
#include "Condition.h"
#include "ImPduBase.h"
#include "public_define.h"
#include "IM.BaseDefine.pb.h"
using namespace std;
class SyncCenter
{
public:
	static SyncCenter* getInstance();

	uint32_t getLastUpdate()
	{
		unique_lock<mutex> lock(lastUpdateLock);
		return lastUpdate;
	}

	uint32_t getLastUpdateGroup()
	{
		unique_lock<mutex> lock(lastUpdateLock);
		return lastUpdateGroup;
	}

	string getDeptName(uint32_t depId);
	void startSync();
	void stopSync();
	void init();
	void updateTotalUpdate(uint32_t updateed);

	SyncCenter(const SyncCenter&) = delete;
	SyncCenter& oprator(const SyncCenter&) = delete;
private:
	SyncCenter();
	~SyncCenter();
	void updateLastUpdateGroup(uint32_t UPdated);
	static void* doSyncGroupChat(void* arg);

    void getDept(uint32_t nDeptId, DBDeptInfo_t** pDept);
    DBDeptMap_t* m_pDeptInfo;
private:
	static SyncCenter*            syncInstance;
	uint32_t                  lastUpdateGroup;
	uint32_t                  lastUpdate;

	condition_variable        groupChatCond;
	std::mutex                groupChatMutex;
	mutex                     lastUpdateLock;

	static bool               syncGroupChatRunning;
	bool                      syncGroupChatWaiting;

	pthread_t                 groupChatThreadId;
};



#endif /* SyncCenter_H_ */
