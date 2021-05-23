#include "DBClient.h"
#include "InetAddress.h"
#include "DBServConn.h"
bool DBClient::init(vector<pair<const char*,short>> ipPortVec, vector<EventLoop*> loopVec,
		vector<string> nameArg)
{
	int clientCnt = ipPortVec.size();
	vector<InetAddress> addrVec;
	for(int i=0; i<clientCnt; ++i)
	{
		InetAddress addr(ipPortVec[i].first, ipPortVec[i].second);
		string name = "DBClient" + ('0' + i);
		std::unique_ptr<TcpClient> tcpClient= new TcpClient(loopVec[i], addr, name);
		tcpClient->setConnectionCallback(std::bind(&DBClient::onConnected,this, std::placeholders::_1));
		tcpClientList.push_back(tcpClient);
	}
}


void DBClient::onConnected(shared_ptr<TcpConnection> conn)
{
	if(conn)
	{
        LOGD("client connected: %s", conn->peerAddress().toIpPort().c_str());
        ++dbConnId;
        std::shared_ptr<DBServConn> dbConn(new DBServConn(conn, dbConnId));
        conn->setMessageCallback();

        std::lock_guard<std::mutex> guard(connMutex);
        DBConnList.push_back(dbConn);
	}
	else
	{
		onDisconnnected(conn);
	}
}

void DBClient::onDisconnected(const std::shared_ptr<TcpConnection>& conn)
{
	//todo
}



