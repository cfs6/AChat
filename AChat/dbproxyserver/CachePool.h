
#ifndef CACHEPOOL_H_
#define CACHEPOOL_H_

#include <vector>
#include <mutex>
#include <memory>
#include "../base/util.h"
#include "ThreadPool.h"
#include "hiredis.h"
using namespace std;

class CachePool;

class CacheConn
{
public:
	CacheConn(CachePool* pCachePool);
	virtual ~CacheConn();

	int init();
	const char* getPoolName();

	string get(string key);
	string setex(string key, int timeout, string value);
	string set(string key, string& value);

    //批量获取
    bool mget(const vector<string>& keys, map<string, string>& ret_value);
    // 判断一个key是否存在
    bool isExists(string &key);

	// Redis hash structure
	long hdel(string key, string field);
	string hget(string key, string field);
	bool hgetAll(string key, map<string, string>& ret);
	long hset(string key, string field, string value);

	long hincrBy(string key, string field, long value);
    long incrBy(string key, long value);
	string hmset(string key, map<string, string>& hash);
	bool hmget(string key, list<string>& fields, list<string>& ret_value);

    //原子加减1
    long incr(string key);
    long decr(string key);

	// Redis list structure
	long lpush(string key, string value);
	long rpush(string key, string value);
	long llen(string key);
	bool lrange(string key, long start, long end, list<string>& ret_value);

private:
	CachePool*           cachePool;
	redisContext*        cacheContext;
	uint64_t             lastConnectTime;
};

class CachePool
{
public:
	CachePool(const char* pool_name, const char* server_ip, int server_port, int db_num, int max_conn_cnt);
	virtual ~CachePool();

	int init();

	CacheConn* getCacheConn();
	void relCacheConn(CacheConn* pCacheConn);

	const char* getPoolName() { return poolName.c_str(); }
	const char* getServerIP() { return serverIp.c_str(); }
	int getServerPort() { return serverPort; }
	int getDBNum() { return dbNum; }

private:
	string                poolName;
	string                serverIp;
	int                   serverPort;
	int                   dbNum;

	int                   curConnCnt;
	int                   maxConnCnt;
	list<CacheConn*>      freeList;
//	CThreadNotify         freeNotify;
	std::mutex            cacheMutex;
	condition_variable    cacheCond;
};

class CacheManager
{
public:
	virtual ~CacheManager();

	static CacheManager* getInstance();

	int init();

	CacheConn* getCacheConn(const char* pool_name);
	void relCacheConn(CacheConn* pCacheConn);
private:
	CacheManager();

private:
	static CacheManager* cacheManager;
	map<string, CachePool*> cachePoolMap;
};






#endif /* CACHEPOOL_H_ */
