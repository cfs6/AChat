#pragma once
#include <memory>
#include <list>
#include <map>
#include <mutex>
#include "../net/TcpServer.h"
#include "../net/EventLoop.h"
#include "FileSession.h"

using namespace net;

struct StoredUserInfo
{
    int32_t         userid;
    std::string     username;
    std::string     password;
    std::string     nickname;
};

class FileServer final
{
public:
    FileServer() = default;
    ~FileServer() = default;

    FileServer(const FileServer& rhs) = delete;
    FileServer& operator =(const FileServer& rhs) = delete;

    bool init(const char* ip, short port, EventLoop* loop, const char* fileBaseDir = "filecache/");
    void uninit();

private:
    void onConnected(std::shared_ptr<TcpConnection> conn);  
    void onDisconnected(const std::shared_ptr<TcpConnection>& conn);
   

private:
    std::unique_ptr<TcpServer>                     m_server;
    std::list<std::shared_ptr<FileSession>>        m_sessions;
    std::mutex                                     m_sessionMutex;
    std::string                                    m_strFileBaseDir;
};
