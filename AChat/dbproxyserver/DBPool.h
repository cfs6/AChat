/*
 * DBPool.h
 *
 *  Created on: 2021年3月20日
 *      Author: cfs
 */

#ifndef DBPOOL_H_
#define DBPOOL_H_

#include "../base/util.h"
#include "ThreadPool.h"
#include <mysql/mysql.h>
#define MAX_ESCAPE_STRING_LEN 10240
using namespace std;
class ResultSet
{
public:
	ResultSet(MYSQL_RES* res);
	virtual ~ResultSet();

	bool next();
	int getInt(const char* key); //todo change name
	char* getString(const char* key);//todo change name

private:
	int getIndex(const char* key);//todo change name

	MYSQL_RES*              mysqlRes;
	MYSQL_ROW*              mysqlRow;
	map<string, int>        keyMap; //todo change name
};

class PrepareStatement {
public:
	PrepareStatement();
	virtual ~PrepareStatement();

	bool init(MYSQL* mysql, string& sql);

	void setParam(uint32_t index, int& value);
	void setParam(uint32_t index, uint32_t& value);
    void setParam(uint32_t index, string& value);
    void setParam(uint32_t index, const string& value);

	bool executeUpdate();
	uint32_t getInsertId();
private:
	MYSQL_STMT*	  mysqlStmt;
	MYSQL_BIND*	  mysqlParamBind;
	uint32_t	  mysqlParamCnt;
};

class DBPool;

class DBConn {
public:
	DBConn(DBPool* pDBPool);
	virtual ~DBConn();
	int init();

	ResultSet* executeQuery(const char* sql_query);
	bool executeUpdate(const char* sql_query);
	char* getEscapeString(const char* content, uint32_t content_len);

	uint32_t getInsertId();

	const char* getPoolName();
	MYSQL* getMysql() { return mysql; }
private:
	DBPool* 	dbPool;	// to get MySQL server information
	MYSQL* 		mysql;
	//MYSQL_RES*	m_res;
	char		escapeString[MAX_ESCAPE_STRING_LEN + 1];
};



class DBPool {
public:
	DBPool(const char* pool_name, const char* db_server_ip, uint16_t db_server_port,
			const char* username, const char* password, const char* db_name, int max_conn_cnt);
	virtual ~DBPool();

	int init();
	DBConn* getDBConn();
	void relDBConn(DBConn* pConn);

	const char* getPoolName() { return poolName.c_str(); }
	const char* getDBServerIP() { return dbServerIp.c_str(); }
	uint16_t getDBServerPort() { return dbServerPort; }
	const char* getUsername() { return username.c_str(); }
	const char* getPasswrod() { return password.c_str(); }
	const char* getDBName() { return dbname.c_str(); }
private:
	string 			    poolName;
	string 			    dbServerIp;
	uint16_t	     	dbServerPort;
	string 				username;
	string 				password;
	string 				dbname;
	int			 		dbCurConnCnt;
	int 				dbMaxConnCnt;
	list<DBConn*>		freeList;        //实际保存mysql连接的容器
	mutex           	dbPoolMutex;
	condition_variable  dbPoolCond;
//	CThreadNotify	  	m_free_notify;
};

// manage db pool (master for write and slave for read)
class DBManager {
public:
	virtual ~DBManager();

	static DBManager* getInstance();

	int init();

	DBConn* getDBConn(const char* dbpool_name);
	void relDBConn(DBConn* pConn);
private:
	DBManager();
	DBManager(const DBManager&) = delete;
	DBManager& operator=(const DBManager&) = delete;
private:
	static DBManager*		dbManager;
	map<string, DBPool*>	dbPoolMap;
};



#endif /* DBPOOL_H_ */
