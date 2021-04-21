
#pragma once

#include <memory>
#include "../net/TcpConnection.h"

using namespace net;

class TcpSession
{
public:
    TcpSession(const std::weak_ptr<TcpConnection>& tmpconn);
    ~TcpSession();

    TcpSession(const TcpSession& rhs) = delete;
    TcpSession& operator =(const TcpSession& rhs) = delete;

    std::shared_ptr<TcpConnection> getConnectionPtr()
    {
        if (tmpConn_.expired())
            return NULL;
        
        return tmpConn_.lock();
    }

    void send(int32_t cmd, int32_t seq, int32_t errorcode, const std::string& filemd5, int64_t offset, int64_t filesize, const std::string& filedata);

private:
    void sendPackage(const char* body, int64_t bodylength);

protected:
    std::weak_ptr<TcpConnection>    tmpConn_;
};
