#include "EncDec.h"
#include "ConfigFileReader.h"
#include "MsgConn.h"
#include "LoginServConn.h"
#include "DBServConn.h"
#include "FileServConn.h"
#include "AsyncLog.h"
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
	char* ip_addr1 = config_file.getConfigName("IpAddr1");	// 电信IP
	char* ip_addr2 = config_file.getConfigName("IpAddr2");	// 网通IP
	char* str_max_conn_cnt = config_file.getConfigName("MaxConnCnt");
    char* str_aes_key = config_file.getConfigName("aesKey");
	uint32_t db_server_count = 0;
	serv_info_t* db_server_list = read_server_config(&config_file, "DBServerIP", "DBServerPort", db_server_count);

	uint32_t login_server_count = 0;
	serv_info_t* login_server_list = read_server_config(&config_file, "LoginServerIP", "LoginServerPort", login_server_count);

//    uint32_t file_server_count = 0;
//    serv_info_t* file_server_list = read_server_config(&config_file, "FileServerIP",
//                                                       "FileServerPort", file_server_count);
    
    if (!str_aes_key || strlen(str_aes_key)!=32) {
        LOGE("aes key is invalied");
        return -1;
    }
 
    pAes = new Aes(str_aes_key);
    
	if (db_server_count < 2) {
		LOGE("DBServerIP need 2 instance at lest ");
		return 1;
	}

	uint32_t concurrent_db_conn_cnt = DEFAULT_CONCURRENT_DB_CONN_CNT;
	uint32_t db_server_count2 = db_server_count * DEFAULT_CONCURRENT_DB_CONN_CNT;
	char* concurrent_db_conn = config_file.getConfigName("ConcurrentDBConnCnt");
	if (concurrent_db_conn) {
		concurrent_db_conn_cnt  = atoi(concurrent_db_conn);
		db_server_count2 = db_server_count * concurrent_db_conn_cnt;
	}

	serv_info_t* db_server_list2 = new serv_info_t [ db_server_count2];
	for (uint32_t i = 0; i < db_server_count2; i++) {
		db_server_list2[i].server_ip = db_server_list[i / concurrent_db_conn_cnt].server_ip.c_str();
		db_server_list2[i].server_port = db_server_list[i / concurrent_db_conn_cnt].server_port;
	}

	if (!listen_ip || !str_listen_port || !ip_addr1)
	{
		LOGE("config file miss, exit... ");
		return -1;
	}

	if (!ip_addr2)
	{
		ip_addr2 = ip_addr1;
	}

	uint16_t listen_port = atoi(str_listen_port);
	uint32_t max_conn_cnt = atoi(str_max_conn_cnt);

	MsgServer::getInstance()->init(listen_ip, listen_port, &mainLoop);

	LOGD("server start listen on: %s:%d\n", listen_ip, listen_port);

	printf("now enter the event loop...\n");
    
	return 0;
}
