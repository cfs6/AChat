#include "LoginConn.h"
//#include "netlib.h"
#include "ConfigFileReader.h"
#include "version.h"
#include "HttpConn.h"
#include "ipparser.h"
#include "AsyncLog.h"
#include "LoginServer.h"
EventLoop mainLoop;
IpParser* pIpParser = NULL;
string strMsfsUrl;
string strDiscovery;//发现获取地址


int main(int argc, char* argv[])
{
	if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
		printf("Server Version: LoginServer/%s\n", VERSION);
		printf("Server Build: %s %s\n", __DATE__, __TIME__);
		return 0;
	}

	signal(SIGPIPE, SIG_IGN);

	ConfigFileReader config_file("loginserver.conf");

    char* client_listen_ip = config_file.getConfigName("ClientListenIP");
    char* str_client_port = config_file.getConfigName("ClientPort");
    char* http_listen_ip = config_file.getConfigName("HttpListenIP");
    char* str_http_port = config_file.getConfigName("HttpPort");
	char* msg_server_listen_ip = config_file.getConfigName("MsgServerListenIP");
	char* str_msg_server_port = config_file.getConfigName("MsgServerPort");
    char* str_msfs_url = config_file.getConfigName("msfs");
    char* str_discovery = config_file.getConfigName("discovery");

	if (!msg_server_listen_ip || !str_msg_server_port || !http_listen_ip
        || !str_http_port || !str_msfs_url || !str_discovery)
	{
		LOGE("config item missing, exit... ");
		return -1;
	}

	uint16_t client_port = atoi(str_client_port);
	uint16_t msg_server_port = atoi(str_msg_server_port);
    uint16_t http_port = atoi(str_http_port);
    strMsfsUrl = str_msfs_url;
    strDiscovery = str_discovery;
    
    
    pIpParser = new IpParser();
    
    LoginServer::getInstance()->init(client_listen_ip, client_port, &mainLoop);

    //TODO:MsgServer
    MsgdispatchServer::getInstance()->init(msg_server_listen_ip, msg_server_port, &mainLoop);

	HttpServer::getInstance()->init(http_listen_ip, http_port, &mainLoop);
    
    LOGI("loginserver initialization completed, now you can use client to connect it.");

    mainLoop.loop();
    
    LOGI("exit loginserver.");

	return 0;
}
