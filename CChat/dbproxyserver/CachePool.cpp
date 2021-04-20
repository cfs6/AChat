#include "CachePool.h"
#include "ConfigFileReader.h"
#include "AsyncLog.h"
#include <utility>
#define MIN_CACHE_CONN_CNT 2

CacheManager* CacheManager::cacheManager = nullptr;

CacheConn::CacheConn(CachePool* pCachePool):cachePool(pCachePool),
		 	 	 	 	 	 	 	 	    cacheContext(nullptr),
		 	 	 	 	 	 	 	 	    lastConnectTime(0)
{

}

CacheConn::~CacheConn()
{
	if(cacheContext)
	{
		redisFree(cacheContext);
		cacheContext = nullptr;
	}
}
//todo 修改返回值为宏
//return 0: succ
int CacheConn::init()
{
	if(cacheContext)
	{
		return 0;
	}

	// 4s 尝试重连一次
	uint64_t curTime = (uint64_t)time(NULL);
	if (curTime < lastConnectTime + 4)
	{
		return 1;
	}

	lastConnectTime = curTime;

	// 200ms超时
	struct timeval timeout = {0, 200000};
	cacheContext = redisConnectWithTimeout(cachePool->GetServerIP(),
			cachePool->GetServerPort(), timeout);

	if(!cacheContext || cacheContext->err)
	{
		if(cacheContext)
		{
			LOGE("redisConnect failed, cacheContext->errstr: %s", cacheContext->errstr);
			redisFree(cacheContext);
			cacheContext = nullptr;
		}
		else
		{
			LOGE("redisConnect failed, cacheContext==nullptr");
		}
		return 1;
	}

	redisReply* reply = (redisReply*) redisCommand(cacheContext, "SELECT %d", cachePool->GetDBNum()));
	if(reply && (reply->type == REDIS_REPLY_STATUS) && (strncmp(reply->str, "OK", 2)==0))
	{
		freeReplyObject(reply);
		return 0;
	}
	else
	{
		LOGE("select cache db failed");
		return 2;
	}
}

const char* CacheConn::getPoolName()
{
	return cachePool->GetPoolName();
}

string CacheConn::get(string key)
{
	string value;

	if(init())
	{
		return value;
	}

	redisReply* reply = (redisReply*)redisCommand(cacheContext, "GET %s", key.c_str());
	if(!reply)
	{
		LOGE("redisCommand GET %s failed: %s", key.c_str(), cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = nullptr;
		return value;
	}

	if(reply->type == REDIS_REPLY_STRING)
	{
		value.append(reply->str, reply->len);
	}
	freeReplyObject(reply);
	return value;
}

string CacheConn::setex(string key, int timeout, string value)
{
	string ret;
	if(init())
	{
		return ret;
	}

	redisReply* reply = (redisReply*)redisCommand(cacheContext,
			"SETEX %s %d %s", key.c_str(), timeout, value.c_str());
	if(!reply)
	{
		LOGE("redisCommand SETEX %s failed: %s", key.c_str(), cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = nullptr;
		return ret;
	}

	ret.append(reply->str, reply->len);
	freeReplyObject(reply);
	return ret;
}

string CacheConn::set(string key, string& value)
{
	string ret;
	if(init())
	{
		return ret;
	}

	redisReply* reply = (redisReply*)redisCommand(cacheContext,
			"SET %s %s", key.c_str(), value.c_str());
	if(!reply)
	{
		LOGE("redisCommand SET %s failed: %s", key.c_str(), cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = nullptr;
		return ret;
	}

	ret.append(reply->str, reply->len);
	freeReplyObject(reply);
	return ret;
}

bool CacheConn::mget(const vector<string>& keys, map<string, string>& ret)
{
	if(init())
	{
		return false;
	}
	if(keys.empty())
	{
		return false;
	}

	string strKey;
	bool first = true;

	for(auto it = keys.begin(); it != keys.end(); ++it)
	{
		if(first)
		{
			first = false;
			strKey += *it;
		}
		else
		{
			strKey += " " + *it;
		}
	}
	if(strKey.empty())
	{
		return false;
	}

	strKey = "MGET " + strKey;
	redisReply* reply = (redisReply*) redisCommand(cacheContext, strKey.c_str());
	if(!reply)
	{
		LOGE("redisCommand %s failed: %s", strKey.c_str(), cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = nullptr;
		return false;
	}

	if(reply->type == REDIS_REPLY_ARRAY)
	{
		for(size_t i = 0; i < reply->elements; ++i)
		{
			auto childReply = reply->element[i];
			if(childReply->type == REDIS_REPLY_STRING)
			{
				ret[keys[i]] = childReply->str;
			}
		}
	}
	freeReplyObject(reply);
	return true;
}

bool CacheConn::isExists(string& key)
{
	if(init())
	{
		return false;
	}

    redisReply* reply = (redisReply*) redisCommand(cacheContext, "EXISTS %s", key.c_str());
    if(!reply)
    {
        LOGE("redisCommand EXISTS %s failed:%s", key.c_str(), cacheContext->errstr);
        redisFree(cacheContext);
        return false;
    }
    long ret = reply->integer;
    freeReplyObject(reply);
    if(0 == ret)
    {
        return false;
    }
    else
    {
        return true;
    }
}

long CacheConn::hdel(string key, string field)
{
	if(init())
	{
		return false;
	}

    redisReply* reply = (redisReply*) redisCommand(cacheContext, "HDEL %s %s", key.c_str(), field.c_str());
    if(!reply)
    {
        LOGE("redisCommand HDEL %s failed:%s", key.c_str(), cacheContext->errstr);
        redisFree(cacheContext);
        cacheContext = nullptr;
        return 0;
    }

    long ret = reply->integer;
    freeReplyObject(reply);
    return ret;
}

string CacheConn::hget(string key, string field)
{
	string ret;
	if(init())
	{
		return ret;
	}

    redisReply* reply = (redisReply*) redisCommand(cacheContext, "HGET %s %s", key.c_str(), field.c_str());
    if(!reply)
    {
        LOGE("redisCommand HGET %s failed: %s", key.c_str(), cacheContext->errstr);
        redisFree(cacheContext);
        cacheContext = nullptr;
        return ret;
    }

    if(reply->type == REDIS_REPLY_STRING)
    {
    	ret.append(reply->str, reply->len);
    }
    freeReplyObject(reply);
    return ret;
}

bool CacheConn::hgetAll(string key, map<string, string>& ret)
{
	if (init()) {
		return false;
	}

	redisReply* reply = (redisReply *)redisCommand(cacheContext, "HGETALL %s", key.c_str());
	if (!reply) {
		LOGE("redisCommand failed:%s", cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = NULL;
		return false;
	}

	if ( (reply->type == REDIS_REPLY_ARRAY) && (reply->elements % 2 == 0) ) {
		for (size_t i = 0; i < reply->elements; i += 2) {
			redisReply* fieldReply = reply->element[i];
			redisReply* valueReply = reply->element[i + 1];

			string field(fieldReply->str, fieldReply->len);
			string value(valueReply->str, valueReply->len);
			ret.insert(make_pair(field, value));
		}
	}

	freeReplyObject(reply);
	return true;
}

long CacheConn::hset(string key, string field, string value)
{
	if (init()) {
		return -1;
	}

	redisReply* reply = (redisReply *)redisCommand(cacheContext, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
	if (!reply) {
		LOGE("redisCommand HSET %s failed: %s", key.c_str(), cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = nullptr;
		return -1;
	}

	long ret = reply->integer;
	freeReplyObject(reply);
	return ret;
}

long CacheConn::hincrBy(string key, string field, long value)
{
	if (init()) {
		return -1;
	}

	redisReply* reply = (redisReply *)redisCommand(cacheContext, "HINCRBY %s %s %ld", key.c_str(), field.c_str(), value);
	if (!reply) {
		LOGE("redisCommand failed:%s", cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = NULL;
		return -1;
	}

	long ret = reply->integer;
	freeReplyObject(reply);
	return ret;
}

long CacheConn::incrBy(string key, long value)
{
    if(init())
    {
        return -1;
    }

    redisReply* reply = (redisReply*)redisCommand(cacheContext, "INCRBY %s %ld", key.c_str(), value);
    if(!reply)
    {
        LOGE("redis Command failed:%s", cacheContext->errstr);
        redisFree(cacheContext);
        cacheContext = NULL;
        return -1;
    }
    long ret = reply->integer;
    freeReplyObject(reply);
    return ret;
}

string CacheConn::hmset(string key, map<string, string>& hash)
{
	string ret;

	if (init()) {
		return ret;
	}

	int argc = hash.size() * 2 + 2;
	const char** argv = new const char* [argc];
	if (!argv) {
		return ret;
	}

	argv[0] = "HMSET";
	argv[1] = key.c_str();
	int i = 2;
	for (map<string, string>::iterator it = hash.begin(); it != hash.end(); it++) {
		argv[i++] = it->first.c_str();
		argv[i++] = it->second.c_str();
	}

	redisReply* reply = (redisReply *)redisCommandArgv(cacheContext, argc, argv, NULL);
	if (!reply) {
		LOGE("redisCommand HMSET %s failed:%s", key.c_str(), cacheContext->errstr);
		delete [] argv;

		redisFree(cacheContext);
		cacheContext = NULL;
		return ret;
	}

	ret.append(reply->str, reply->len);

	delete [] argv;
	freeReplyObject(reply);
	return ret;
}

bool CacheConn::hmget(string key, list<string>& fields, list<string>& ret)
{
	if (init()) {
		return false;
	}

	int argc = fields.size() + 2;
	const char** argv = new const char* [argc];
	if (!argv) {
		return false;
	}

	argv[0] = "HMGET";
	argv[1] = key.c_str();
	int i = 2;
	for (list<string>::iterator it = fields.begin(); it != fields.end(); it++) {
		argv[i++] = it->c_str();
	}

	redisReply* reply = (redisReply *)redisCommandArgv(cacheContext, argc, (const char**)argv, NULL);
	if (!reply) {
		LOGE("redisCommand failed:%s", cacheContext->errstr);
		delete [] argv;

		redisFree(cacheContext);
		cacheContext = NULL;

		return false;
	}

	if (reply->type == REDIS_REPLY_ARRAY) {
		for (size_t i = 0; i < reply->elements; i++) {
			redisReply* valueReply = reply->element[i];
			string value(valueReply->str, valueReply->len);
			ret.push_back(value);
		}
	}

	delete [] argv;
	freeReplyObject(reply);
	return true;
}

long CacheConn::incr(string key)
{
    if(init())
    {
        return -1;
    }

    redisReply* reply = (redisReply*)redisCommand(cacheContext, "INCR %s", key.c_str());
    if(!reply)
    {
        LOGE("redis Command INCR %s failed:%s", key.c_str(), cacheContext->errstr);
        redisFree(cacheContext);
        cacheContext = NULL;
        return -1;
    }
    long ret = reply->integer;
    freeReplyObject(reply);
    return ret;
}

long CacheConn::decr(string key)
{
    if(init())
    {
        return -1;
    }

    redisReply* reply = (redisReply*)redisCommand(cacheContext, "DECR %s", key.c_str());
    if(!reply)
    {
        log("redis Command DECR %s failed:%s", key.c_str(), cacheContext->errstr);
        redisFree(cacheContext);
        cacheContext = NULL;
        return -1;
    }
    long ret = reply->integer;
    freeReplyObject(reply);
    return ret;
}

long CacheConn::lpush(string key, string value)
{
	if (init()) {
		return -1;
	}

	redisReply* reply = (redisReply *)redisCommand(cacheContext, "LPUSH %s %s", key.c_str(), value.c_str());
	if (!reply) {
		LOGE("redisCommand LPUSH %s failed:%s", key.c_str(), cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = NULL;
		return -1;
	}

	long ret = reply->integer;
	freeReplyObject(reply);
	return ret;
}

long CacheConn::rpush(string key, string value)
{
	if (init()) {
		return -1;
	}

	redisReply* reply = (redisReply *)redisCommand(cacheContext, "RPUSH %s %s", key.c_str(), value.c_str());
	if (!reply) {
		LOGE("redisCommand failed:%s", cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = NULL;
		return -1;
	}

	long ret = reply->integer;
	freeReplyObject(reply);
	return ret;
}

long CacheConn::llen(string key)
{
	if (init()) {
		return -1;
	}

	redisReply* reply = (redisReply *)redisCommand(cacheContext, "LLEN %s", key.c_str());
	if (!reply) {
		LOGE("redisCommand failed:%s", cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = NULL;
		return -1;
	}

	long ret = reply->integer;
	freeReplyObject(reply);
	return ret;
}

bool CacheConn::lrange(string key, long start, long end, list<string>& ret)
{
	if (init()) {
		return false;
	}

	redisReply* reply = (redisReply *)redisCommand(cacheContext, "LRANGE %s %d %d", key.c_str(), start, end);
	if (!reply) {
		LOGE("redisCommand %s failed:%s", cacheContext->errstr);
		redisFree(cacheContext);
		cacheContext = NULL;
		return false;
	}

	if (reply->type == REDIS_REPLY_ARRAY) {
		for (size_t i = 0; i < reply->elements; i++) {
			redisReply* valueReply = reply->element[i];
			string value(valueReply->str, valueReply->len);
			ret.push_back(value);
		}
	}

	freeReplyObject(reply);
	return true;
}

CachePool::CachePool(const char* pool_name, const char* server_ip, int server_port, int db_num, int max_conn_cnt)
					:poolName(pool_name), serverIp(server_ip), serverPort(server_port), dbNum(db_num), maxConnCnt(max_conn_cnt),
					 curConnCnt(MIN_CACHE_CONN_CNT)
{

}

CachePool::~CachePool()
{
	unique_lock<mutex> guard(cacheMutex);
	for(auto it = freeList.begin(); it != freeList.end(); ++it)
	{
		CacheConn* pConn = *it;
		delete pConn;
	}

	freeList.clear();
	curConnCnt = 0;
}

int CachePool::init()
{
	for(int i = 0; i < curConnCnt; ++i)
	{
		CacheConn* pConn = new CacheConn(this);
		if(pConn->init())
		{
			delete pConn;
			return 1;
		}
		freeList.push_back(pConn);
	}

	LOGE("cache pool: %s, list size: %lu", poolName.c_str(), freeList.size());
	return 0;
}

CacheConn* CachePool::getCacheConn()
{
	unique_lock<mutex> lock(cacheMutex);
	while(freeList.empty())
	{
		if(curConnCnt >= maxConnCnt)
		{
			cacheCond.wait(lock);
		}
		else
		{
			CacheConn* pConn = new CacheConn(this);
			int ret = pConn->init();
			if(ret)
			{
				LOGE("Init CacheConn Failed");
				delete pConn;
				return nullptr;
			}
			else
			{
				freeList.push_back(pConn);
				curConnCnt++;
				LOGD("new cache connection: %s, conn_cnt: %d", poolName.c_str(), curConnCnt);
			}
		}
	}

	CacheConn* pConn = freeList.front();
	freeList.pop_front();
	return pConn;
}

void CachePool::relCacheConn(CacheConn* pCacheConn)
{
	unique_lock<mutex> lock(cacheMutex);
	auto it = freeList.begin();
	for(; it != freeList.end(); ++it)
	{
		if(*it == pCacheConn)
		{
			break;
		}
	}

	if(it == freeList.end())
	{
		freeList.push_back(pCacheConn);
	}

	cacheCond.notify_one();
}

CacheManager::~CacheManager()
{

}

CacheManager* CacheManager::getInstance()
{
	if (!cacheManager) {
		cacheManager = new CacheManager();
		if (cacheManager->init()) {
			delete cacheManager;
			cacheManager = NULL;
		}
	}

	return cacheManager;
}

int CacheManager::init()
{
	ConfigFileReader configFile("dbproxyserver.conf");

	char* cacheInstances = configFile.getConfigName("CacheInstances");
	if(!cacheInstances)
	{
		LOGE("Configure CacheInstance failed");
		return 1;
	}

	char host[64];
	char port[64];
	char db[64];
	char maxconncnt[64];

	CStrExplode instanceName(cacheInstances, ',');
	for(uint32_t i = 0; i < instanceName.GetItemCnt(); ++i)
	{
		char* poolname = instanceName.GetItem(i);

		snprintf(host, 64, "%s_host", poolname);
		snprintf(port, 64, "%s_port", poolname);
		snprintf(db, 64, "%s_db", poolname);
        snprintf(maxconncnt, 64, "%s_maxconncnt", poolname);

		char* cachehost = configFile.getConfigName(host);
		char* cacheport = configFile.getConfigName(port);
		char* cachedb = configFile.getConfigName(db);
        char* maxconncnt = configFile.getConfigName(maxconncnt);
		if (!cachehost || !cacheport || !cachedb || !maxconncnt)
		{
			LOGE("not configure cache instance: %s", poolname);
			return 2;
		}

		CachePool* pCachePool = new CachePool(poolname, cachehost,
				atoi(cacheport), atoi(cachedb), atoi(maxconncnt));
		if(pCachePool->init())
		{
			LOGE("Init cache pool failed");
			return 3;
		}

		cachePoolMap.insert(make_pair(poolname, pCachePool));
	}
	return 0;
}

CacheConn* CacheManager::getCacheConn(const char* pool_name)
{
	auto it = cachePoolMap.find(pool_name);
	if(it != cachePoolMap.end())
	{
		return it->second->getCacheConn();
	}
	else
	{
		return nullptr;
	}
}

void CacheManager::relCacheConn(CacheConn* pCacheConn)
{
	if (!pCacheConn) {
		return;
	}

	map<string, CachePool*>::iterator it = cachePoolMap.find(pCacheConn->getPoolName());
	if (it != cachePoolMap.end()) {
		return it->second->relCacheConn(pCacheConn);
	}
}

