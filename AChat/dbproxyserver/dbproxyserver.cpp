#include "ConfigFileReader.h"
#include "version.h"
#include "ThreadPool.h"
#include "DBPool.h"
#include "CachePool.h"
#include "ProxyConn.h"
#include "HttpClient.h"
#include "EncDec.h"
#include "AudioModel.h"
#include "MessageModel.h"
#include "SessionModel.h"
#include "RelationModel.h"
#include "UserModel.h"
#include "GroupModel.h"
#include "GroupMessageModel.h"
#include "FileModel.h"
#include "SyncCenter.h"
#include "AsyncLog.h"
#include <signal.h>

EventLoop mainLoop;

int main(int argc, char* argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
//        printf("Server Version: DBProxyServer/%s\n", VERSION);
        printf("Server Build: %s %s\n", __DATE__, __TIME__);
        return 0;
    }

    signal(SIGPIPE, SIG_IGN);
    srand(time(NULL));

    ConfigFileReader logConfig("fileserver.conf");

    std::string logFileFullPath;

#ifndef WIN32
    const char* logfilepath = logConfig.getConfigName("logfiledir");
    if (logfilepath == NULL)
    {
        LOGF("logdir is not set in config file");
        return 1;
    }

    //���logĿ¼�������򴴽�֮
    DIR* dp = opendir(logfilepath);
    if (dp == NULL)
    {
        if (mkdir(logfilepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        {
            LOGF("create base dir error, %s , errno: %d, %s", logfilepath, errno, strerror(errno));
            return 1;
        }
}
    closedir(dp);

    logFileFullPath = logfilepath;
#endif

    const char* logfilename = logConfig.getConfigName("logfilename");
    logFileFullPath += logfilename;

    CAsyncLog::init(logFileFullPath.c_str());

    //初始化redis
    CacheManager* pCacheManager = CacheManager::getInstance();
    if (!pCacheManager)
    {
        LOGE("CacheManager init failed");
        return -1;
    }

    DBManager* pDBManager = DBManager::getInstance();
    if (!pDBManager)
    {
        LOGE("DBManager init failed");
        return -1;
    }
    puts("db init success");
    // 主线程初始化单例，不然在工作线程可能会出现多次初始化
    //todo 返回值定义宏
    if (!AudioModel::getInstance()) {
        return -1;
    }

    if (!GroupMessageModel::getInstance()) {
        return -1;
    }

    if (!GroupModel::getInstance()) {
        return -1;
    }

    if (!MessageModel::getInstance()) {
        return -1;
    }

    if (!SessionModel::getInstance()) {
        return -1;
    }

    if (!RelationModel::getInstance())
    {
        return -1;
    }

    if (!UserModel::getInstance()) {
        return -1;
    }

    if (!FileModel::getInstance()) {
        return -1;
    }


    ConfigFileReader config_file("dbproxyserver.conf");

    char* listen_ip = config_file.getConfigName("ListenIP");
    char* str_listen_port = config_file.getConfigName("ListenPort");
    char* str_thread_num = config_file.getConfigName("ThreadNum");
    char* str_file_site = config_file.getConfigName("MsfsSite");
    char* str_aes_key = config_file.getConfigName("aesKey");

    if (!listen_ip || !str_listen_port || !str_thread_num || !str_file_site || !str_aes_key) {
        LOGE("missing ListenIP/ListenPort/ThreadNum/MsfsSite/aesKey, exit...");
        return -1;
    }

    if (strlen(str_aes_key) != 32)
    {
    	LOGE("aes key is invalied");
        return -2;
    }
    string strAesKey(str_aes_key, 32);
    Aes cAes = Aes(strAesKey);
    string strAudio = "[语音]";
    char* pAudioEnc;
    uint32_t nOutLen;
    if (cAes.Encrypt(strAudio.c_str(), strAudio.length(), &pAudioEnc, nOutLen) == 0)
    {
        strAudioEnc.clear();
        strAudioEnc.append(pAudioEnc, nOutLen);
        cAes.Free(pAudioEnc);
    }

    //默认监听端口是10600
    uint16_t listen_port = atoi(str_listen_port);
    uint32_t thread_num = atoi(str_thread_num);

    string strFileSite(str_file_site);
    AudioModel::getInstance()->setUrl(strFileSite);

    // for 603 push
//    curl_global_init(CURL_GLOBAL_ALL);

//    init_proxy_conn(thread_num);

    ProxyServer::getInstance()->init(listen_ip, listen_port, &mainLoop);

    SyncCenter::getInstance()->init();      //todo
    SyncCenter::getInstance()->startSync(); //todo

    CStrExplode listen_ip_list(listen_ip, ';');
//    for (uint32_t i = 0; i < listen_ip_list.GetItemCnt(); i++)
//    {
//        ret = netlib_listen(listen_ip_list.GetItem(i), listen_port, proxy_serv_callback, NULL);
//        if (ret == NETLIB_ERROR)
//            return ret;
//    }

//    printf("server start listen on: %s:%d\n", listen_ip, listen_port);
//    printf("now enter the event loop...\n");
//    writePid();
//    netlib_eventloop(10);

    mainLoop.loop();

    return 0;
}
