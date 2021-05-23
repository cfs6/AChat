#include "EncDec.h"
#include "ConfigFileReader.h"
#include "MsgConn.h"
#include "LoginServConn.h"
#include "DBServConn.h"
#include "FileServConn.h"
#include "AsyncLog.h"
#include "LoginClient.h"
#include "DBClient.h"
#define DEFAULT_CONCURRENT_DB_CONN_CNT  10

EventLoop mainLoop;
Aes *pAes;


int main(int argc, char* argv[])
{
	if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
//		printf("Server Version: MsgServer/%s\n", VERSION);
		printf("Server Build: %s %s\n", __DATE__, __TIME__);
		return 0;
	}

	signal(SIGPIPE, SIG_IGN);
	srand(time(NULL));

	LOGE("MsgServer max files can open: %d ", getdtablesize());

	ConfigFileReader config_file("msgserver.conf");

	char* listen_ip = config_file.getConfigName("ListenIP");
	char* str_listen_port = config_file.getConfigName("ListenPort");
	char* ip_addr1 = config_file.getConfigName("IpAddr1");
	char* str_max_conn_cnt = config_file.getConfigName("MaxConnCnt");
    char* str_aes_key = config_file.getConfigName("aesKey");
	uint32_t db_server_count = 0;
	serv_info_t* db_server_list = read_server_config(&config_file, "DBServerIP", "DBServerPort", db_server_count);

	uint32_t login_server_count = 0;
	serv_info_t* login_server_list = read_server_config(&config_file, "LoginServerIP", "LoginServerPort", login_server_count);

//    uint32_t file_server_count = 0;
//    serv_info_t* file_server_list = read_server_config(&config_file, "FileServerIP",
//                                                       "FileServerPort", file_server_count);
    
    if (!str_aes_key || strlen(str_aes_key)!=32)
    {
        LOGE("aes key is invalied");
        return -1;
    }
 
    pAes = new Aes(str_aes_key);
    //至少配置2个dbserver实例，一个用于用户登录业务，一个用于其他业务
	if (db_server_count < 2)
	{
		LOGE("DBServerIP need 2 instance at least ");
		return 1;
	}
	// 到BusinessServer的开多个并发的连接
	uint32_t concurrent_db_conn_cnt = DEFAULT_CONCURRENT_DB_CONN_CNT;
	uint32_t db_server_count2 = db_server_count * DEFAULT_CONCURRENT_DB_CONN_CNT;
	char* concurrent_db_conn = config_file.getConfigName("ConcurrentDBConnCnt");
	if (concurrent_db_conn)
	{
		concurrent_db_conn_cnt  = atoi(concurrent_db_conn);
		db_server_count2 = db_server_count * concurrent_db_conn_cnt;
	}

	serv_info_t* dbServerList2 = new serv_info_t [ db_server_count2];
	for (uint32_t i = 0; i < db_server_count2; i++)
	{
		dbServerList2[i].server_ip = db_server_list[i / concurrent_db_conn_cnt].server_ip.c_str();
		dbServerList2[i].server_port = db_server_list[i / concurrent_db_conn_cnt].server_port;
	}

	if (!listen_ip || !str_listen_port || !ip_addr1)
	{
		LOGE("config file miss, exit... ");
		return -1;
	}


	uint16_t listen_port = atoi(str_listen_port);
	uint32_t max_conn_cnt = atoi(str_max_conn_cnt);

	MsgServer::getInstance()->init(listen_ip, listen_port, &mainLoop);

	LOGD("server start listen on: %s:%d\n", listen_ip, listen_port);

	uint32_t g_db_server_login_count = 0;
	uint32_t total_db_instance = db_server_count2 / concurrent_db_conn_cnt;
	g_db_server_login_count = (total_db_instance / 2) * concurrent_db_conn_cnt;
	LOGE("DB server connection index for login business: [0, %u), for other business: [%u, %u) ",
			g_db_server_login_count, g_db_server_login_count, db_server_count2);

	vector<pair<const char*,short>> ipPortVec;
	vector<EventLoop*> loopVec;
	vector<string> nameArgVec;
	for(int i=0; i<db_server_count2; ++i)
	{
		pair<const char*,short> ipPort = make_pair(dbServerList2[i].server_ip,dbServerList2[i].server_port);
		ipPortVec.push_back(ipPort);
		EventLoop* loop = new EventLoop();
		loopVec.push_back(loop);
		char curIndex = '0'+i;
		string name = "DBServConn" + curIndex;
		nameArgVec.push_back(name);
	}
	DBClient::getInstance()->init(ipPortVec, loopVec, nameArgVec);





	printf("now enter the event loop...\n");
    
	return 0;
}
