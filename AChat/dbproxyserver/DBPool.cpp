/*
 * DBPool.cpp
 *
 *  Created on: 2021年3月20日
 *      Author: cfs
 */
#include "DBPool.h"
#include "ConfigFileReader.h"
#include "../base/AsyncLog.h"
#define MIN_DB_CONN_CNT 2

DBManager* DBManager::dbManager = nullptr;

ResultSet::ResultSet(MYSQL_RES* res):mysqlRes(res)
{
	int numFields = mysql_num_fields(mysqlRes);
	MYSQL_FIELD* fields = mysql_fetch_fields(mysqlRes);
	for(int i = 0; i < numFields; ++i)
	{
		keyMap.insert(make_pair(fields[i].name, i));
	}
}

ResultSet::~ResultSet()
{
	if(mysqlRes)
	{
		mysql_free_result(mysqlRes);
		mysqlRes = nullptr;
	}
}

bool ResultSet::next()
{
	mysqlRow = mysql_fetch_row(mysqlRes);
	if(mysqlRow)
	{
		return true;
	}
	else
	{
		return false;
	}
}

int ResultSet::getIndex(const char* key)
{
	map<string, int>::iterator it = keyMap.find(key);
	if (it == keyMap.end())
	{
		return -1;
	}
	else
	{
		return it->second;
	}
}

int ResultSet::getInt(const char* key)
{
	int idx = getIndex(key);
	if (idx == -1)
	{
		return 0;
	}
	else
	{
		return atoi((const char*)mysqlRow[idx]);
	}
}

char* ResultSet::getString(const char* key)
{
	int idx = getIndex(key);
	if (idx == -1)
	{
		return nullptr;
	}
	else
	{
		return mysqlRow[idx];
	}
}

PrepareStatement::PrepareStatement()
{
	mysqlStmt = NULL;
	mysqlParamBind = NULL;
	mysqlParamCnt = 0;
}

PrepareStatement::~PrepareStatement()
{
	if (mysqlStmt) {
		mysql_stmt_close(mysqlStmt);
		mysqlStmt = NULL;
	}

	if (mysqlParamBind) {
		delete [] mysqlParamBind;
		mysqlParamBind = NULL;
	}
}

bool PrepareStatement::init(MYSQL* mysql, string& sql)
{
	mysql_ping(mysql);

	mysqlStmt = mysql_stmt_init(mysql);
	if (!mysqlStmt) {
		LOGE("mysql_stmt_init failed");
		return false;
	}

	if (mysql_stmt_prepare(mysqlStmt, sql.c_str(), sql.size())) {
		LOGE("mysql_stmt_prepare failed: %s", mysql_stmt_error(mysqlStmt));
		return false;
	}

	mysqlParamCnt = mysql_stmt_param_count(mysqlStmt);
	if (mysqlParamCnt > 0) {
		mysqlParamBind = new MYSQL_BIND [mysqlParamCnt];
		if (!mysqlParamBind) {
			LOGE("new MYSQL_BIND failed");
			return false;
		}

		memset(mysqlParamBind, 0, sizeof(MYSQL_BIND) * mysqlParamCnt);
	}

	return true;
}

void PrepareStatement::setParam(uint32_t index, int& value)
{
	if (index >= mysqlParamCnt) {
		LOGE("index too large: %d", index);
		return;
	}

	mysqlParamBind[index].buffer_type = MYSQL_TYPE_LONG;
	mysqlParamBind[index].buffer = &value;
}

void PrepareStatement::setParam(uint32_t index, uint32_t& value)
{
	if (index >= mysqlParamCnt) {
		LOGE("index too large: %d", index);
		return;
	}

	mysqlParamBind[index].buffer_type = MYSQL_TYPE_LONG;
	mysqlParamBind[index].buffer = &value;
}

void PrepareStatement::setParam(uint32_t index, string& value)
{
	if (index >= mysqlParamCnt) {
		LOGE("index too large: %d", index);
		return;
	}

	mysqlParamBind[index].buffer_type = MYSQL_TYPE_STRING;
	mysqlParamBind[index].buffer = (char*)value.c_str();
	mysqlParamBind[index].buffer_length = value.size();
}

void PrepareStatement::setParam(uint32_t index, const string& value)
{
    if (index >= mysqlParamCnt) {
        LOGE("index too large: %d", index);
        return;
    }

    mysqlParamBind[index].buffer_type = MYSQL_TYPE_STRING;
    mysqlParamBind[index].buffer = (char*)value.c_str();
    mysqlParamBind[index].buffer_length = value.size();
}

bool PrepareStatement::executeUpdate()
{
	if (!mysqlParamCnt) {
		LOGE("no m_stmt");
		return false;
	}

	if (mysql_stmt_bind_param(mysqlStmt, mysqlParamBind)) {
		LOGE("mysql_stmt_bind_param failed: %s", mysql_stmt_error(mysqlStmt));
		return false;
	}

	if (mysql_stmt_execute(m_stmt)) {
		LOGE("mysql_stmt_execute failed: %s", mysql_stmt_error(mysqlStmt));
		return false;
	}

	if (mysql_stmt_affected_rows(mysqlStmt) == 0) {
		LOGE("ExecuteUpdate have no effect");
		return false;
	}

	return true;
}

uint32_t PrepareStatement::getInsertId()
{
	return mysql_stmt_insert_id(mysqlStmt);
}


DBConn::DBConn(DBPool* pPool)
{
	dbPool = pPool;
	mysql = NULL;
}

DBConn::~DBConn()
{

}

int DBConn::init()
{
	mysql = mysql_init(NULL);
	if (!mysql)
	{
		LOGE("mysql_init failed");
		return 1;
	}

	my_bool reconnect = true;
	mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);
	mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

	if (!mysql_real_connect(mysql, dbPool->getDBServerIP(), dbPool->getUsername(), dbPool->getPasswrod(),
			dbPool->getDBName(), dbPool->getDBServerPort(), NULL, 0)) {
		LOGE("mysql_real_connect failed: %s", mysql_error(mysql));
		return 2;
	}

	return 0;
}

const char* DBConn::getPoolName()
{
	return dbPool->getPoolName();
}

ResultSet* DBConn::executeQuery(const char* sql_query)
{
	mysql_ping(m_mysql);

	if (mysql_real_query(mysql, sql_query, strlen(sql_query))) {
		LOGE("mysql_real_query failed: %s, sql: %s", mysql_error(mysql), sql_query);
		return NULL;
	}

	MYSQL_RES* res = mysql_store_result(mysql);
	if (!res) {
		LOGE("mysql_store_result failed: %s", mysql_error(mysql));
		return NULL;
	}

	ResultSet* result_set = new ResultSet(res);
	return result_set;
}

bool DBConn::executeUpdate(const char* sql_query)
{
	mysql_ping(m_mysql);

	if (mysql_real_query(mysql, sql_query, strlen(sql_query)))
	{
		LOGE("mysql_real_query failed: %s, sql: %s", mysql_error(mysql), sql_query);
		return false;
	}

	if (mysql_affected_rows(mysql) > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

char* DBConn::getEscapeString(const char* content, uint32_t content_len)
{
	if (content_len > (MAX_ESCAPE_STRING_LEN >> 1))
	{
		escapeString[0] = 0;
	}
	else
	{
		mysql_real_escape_string(mysql, escapeString, content, content_len);
	}

	return escapeString;
}

uint32_t DBConn::getInsertId()
{
	return (uint32_t)mysql_insert_id(mysql);
}

DBPool::DBPool(const char* pool_name, const char* db_server_ip, uint16_t db_server_port,
			const char* username, const char* password, const char* db_name, int max_conn_cnt)
			  : poolName(pool_name), dbServerIp(db_server_ip), dbServerPort(db_server_port),
			    username(username), password(password), dbname(db_name),
			    dbCurConnCnt(MIN_DB_CONN_CNT), dbMaxConnCnt(max_conn_cnt)
{

}

DBPool::~DBPool()
{
	for (auto it = freeList.begin(); it != freeList.end(); it++) {
		DBConn* pConn = *it;
		delete pConn;
	}

	freeList.clear();
}
//创建dbCurConnCnt 数目的DB连接，放入list
int DBPool::init()
{
	for (int i = 0; i < dbCurConnCnt; i++) {
		DBConn* pDBConn = new DBConn(this);
		int ret = pDBConn->init();
		if (ret) {
			delete pDBConn;
			return ret;
		}

		freeList.push_back(pDBConn);
	}

	LOGE("db pool: %s, size: %d", poolName.c_str(), (int)freeList.size());
	return 0;
}

DBConn* DBPool::getDBConn()
{
	unique_lock<mutex> lock(dbPoolMutex);
	while (freeList.empty())
	{
		if (dbCurConnCnt >= dbMaxConnCnt)
		{
			dbPoolCond.wait(lock);
		}
		else
		{
			DBConn* pDBConn = new DBConn(this);
			int ret = pDBConn->init();
			if (ret)
			{
				LOGE("Init DBConnecton failed");
				delete pDBConn;
				return NULL;
			} else
			{
				freeList.push_back(pDBConn);
				dbCurConnCnt++;
				LOGE("new db connection: %s, conn_cnt: %d", poolName.c_str(), dbCurConnCnt);
			}
		}
	}

	DBConn* pConn = freeList.front();
	freeList.pop_front();


	return pConn;
}

void DBPool::relDBConn(DBConn* pConn)
{
	unique_lock<mutex> lock(dbPoolMutex);

	auto it = freeList.begin();
	for (; it != freeList.end(); it++)
	{
		if (*it == pConn)
		{
			break;
		}
	}

	if (it == freeList.end())
	{
		freeList.push_back(pConn);
	}

	dbPoolCond.notify_one();
}


DBManager* DBManager::getInstance()
{
	if (!dbManager)
    {
		dbManager = new DBManager();
		if (dbManager->init()) {
			delete dbManager;
			dbManager = NULL;
		}
	}

	return dbManager;
}
//根据配置文件得到DB实例，创建连接池，并存入dbPoolMap
int DBManager::init()
{
	ConfigFileReader config_file("dbproxyserver.conf");
	char* db_instances = config_file.getConfigName("DBInstances");

	if (!db_instances)
	{
		LOGE("not configure DBInstances");
		return 1;
	}

	char host[64];
	char port[64];
	char dbname[64];
	char username[64];
	char password[64];
    char maxconncnt[64];
	CStrExplode instances_name(db_instances, ',');

	for (uint32_t i = 0; i < instances_name.GetItemCnt(); i++)
	{
		char* pool_name = instances_name.GetItem(i);
		snprintf(host, 64, "%s_host", pool_name);
		snprintf(port, 64, "%s_port", pool_name);
		snprintf(dbname, 64, "%s_dbname", pool_name);
		snprintf(username, 64, "%s_username", pool_name);
		snprintf(password, 64, "%s_password", pool_name);
        snprintf(maxconncnt, 64, "%s_maxconncnt", pool_name);

		char* db_host = config_file.getConfigName(host);
		char* str_db_port = config_file.getConfigName(port);
		char* db_dbname = config_file.getConfigName(dbname);
		char* db_username = config_file.getConfigName(username);
		char* db_password = config_file.getConfigName(password);
        char* str_maxconncnt = config_file.getConfigName(maxconncnt);

		if (!db_host || !str_db_port || !db_dbname || !db_username || !db_password || !str_maxconncnt)
		{
			LOGE("not configure db instance: %s", pool_name);
			return 2;
		}

		int db_port = atoi(str_db_port);
        int db_maxconncnt = atoi(str_maxconncnt);
		DBPool* pDBPool = new DBPool(pool_name, db_host, db_port, db_username, db_password, db_dbname, db_maxconncnt);
		if (pDBPool->init())
        {
			LOGE("init db instance failed: %s", pool_name);
			return 3;
		}
		dbPoolMap.insert(make_pair(pool_name, pDBPool));
	}

	return 0;
}

DBConn* DBManager::getDBConn(const char* dbpool_name)
{
	map<string, DBPool*>::iterator it = dbPoolMap.find(dbpool_name);
	if (it == dbPoolMap.end())
	{
		return NULL;
	}
	else
	{
		return it->second->getDBConn();
	}
}

void DBManager::relDBConn(DBConn* pConn)
{
	if (!pConn)
	{
		return;
	}

	map<string, DBPool*>::iterator it = dbPoolMap.find(pConn->getPoolName());
	if (it != dbPoolMap.end())
	{
		it->second->relDBConn(pConn);
	}
}












