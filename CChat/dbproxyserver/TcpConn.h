

#ifndef TCPCONN_H_
#define TCPCONN_H_

#pragma once

#include <memory>
#include "../net/TcpConnection.h"

using namespace net;

//为了让业务与逻辑分开，实际应该新增一个子类继承自TcpConn，让TcpConn中只有逻辑代码，其子类存放业务代码
class TcpConn
{
public:
    TcpConn(const std::weak_ptr<TcpConnection>& tmpconn);
    ~TcpConn();

    TcpConn(const TcpConn& rhs) = delete;
    TcpConn& operator =(const TcpConn& rhs) = delete;

    std::shared_ptr<TcpConnection> getConnectionPtr()
    {
        if (tmpConn_.expired())
            return NULL;

        return tmpConn_.lock();
    }

    void send(int32_t cmd, int32_t seq, int32_t errorcode, const std::string& filemd5,
    		int64_t offset, int64_t filesize, const std::string& filedata);

private:
    void sendPackage(const char* body, int64_t bodylength);

protected:
    //TcpConn引用TcpConnection类必须是弱指针，因为TcpConnection可能会因网络出错自己销毁，此时TcpConn应该也要销毁
    std::weak_ptr<TcpConnection>    tmpConn_;
};



#endif /* TCPCONN_H_ */
