#include "LoginClient.h"



bool LoginClient::init(const char* ip, short port, EventLoop* loop)
{
	InetAddress addr(ip, port);
//	handlerMap = HandlerMap::getInstance();
	//threadpool?
	loginClient.reset(new TcpServer(loop, addr, "LoginClient", TcpServer::kReusePort));
	loginClient->setConnectionCallback(std::bind(&MsgServer::onConnected, this, std::placeholders::_1));
}


void LoginClient::onConnected(shared_ptr<TcpConnection> conn)
{

}

