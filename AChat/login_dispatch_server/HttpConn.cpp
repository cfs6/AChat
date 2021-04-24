
#include "HttpConn.h"
#include "json/json.h"
#include "LoginConn.h"
#include "HttpParserWrapper.h"
#include "ipparser.h"
#include "AsyncLog.h"
static HttpConnMap_t g_http_conn_map;

extern map<uint32_t, msg_serv_info_t*>  g_msg_serv_info;

extern IpParser* pIpParser;
extern string strMsfsUrl;
extern string strDiscovery;

// conn_handle 从0开始递增，可以防止因socket handle重用引起的一些冲突
static uint32_t g_conn_handle_generator = 0;

HttpConn* FindHttpConnByHandle(uint32_t conn_handle)
{
    HttpConn* pConn = NULL;
    HttpConnMap_t::iterator it = g_http_conn_map.find(conn_handle);
    if (it != g_http_conn_map.end()) {
        pConn = it->second;
    }

    return pConn;
}

void httpconn_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam)
{
	NOTUSED_ARG(uParam);
	NOTUSED_ARG(pParam);

	// convert void* to uint32_t, oops
	uint32_t conn_handle = *((uint32_t*)(&callback_data));
    HttpConn* pConn = FindHttpConnByHandle(conn_handle);
    if (!pConn) {
        return;
    }

	switch (msg)
	{
	case NETLIB_MSG_READ:
		pConn->OnRead();
		break;
	case NETLIB_MSG_WRITE:
		pConn->OnWrite();
		break;
	case NETLIB_MSG_CLOSE:
		pConn->OnClose();
		break;
	default:
		LOGE("!!!httpconn_callback error msg: %d ", msg);
		break;
	}
}

void HttpConn::http_conn_timer_callback()
{
	HttpConn* pConn = NULL;
	HttpConnMap_t::iterator it, it_old;
	uint64_t cur_time = get_tick_count();

	for (it = g_http_conn_map.begin(); it != g_http_conn_map.end(); ) {
		it_old = it;
		it++;

		pConn = it_old->second;
		pConn->OnTimer(cur_time);
	}
}

void HttpConn::httpConnCheck()
{
	TcpConnection conn = getConn();
    if (conn)
    {
        m_checkOnlineTimerId = conn.getLoop()->runEvery(1000, std::bind(&HttpConn::http_conn_timer_callback, this));
    }
}


//void init_http_conn()
//{
//	netlib_register_timer(http_conn_timer_callback, NULL, 1000);
//}

//////////////////////////
HttpConn::HttpConn()
{
	busy = false;
	sockfd = NETLIB_INVALID_HANDLE;
    state = CONN_STATE_IDLE;
    
    lastSendTick = lastSendTick = get_tick_count();
    connHandle = ++g_conn_handle_generator;
	if (connHandle == 0) {
		connHandle = ++g_conn_handle_generator;
	}

	//LOGE("HttpConn, handle=%u\n", connHandle);
}

HttpConn::~HttpConn()
{
	//LOGE("~HttpConn, handle=%u\n", connHandle);
}

int HttpConn::Send(void* data, int len)
{
	m_last_send_tick = get_tick_count();

	if (busy)
	{
		outBuf.Write(data, len);
		return len;
	}

	int ret = netlib_send(sockfd, data, len);
	if (ret < 0)
		ret = 0;

	if (ret < len)
	{
		outBuf.Write((char*)data + ret, len - ret);
		busy = true;
		//LOGE("not send all, remain=%d\n", outBuf.GetWriteOffset());
	}
    else
    {
        OnWriteComlete();
    }

	return len;
}

void HttpConn::Close()
{
    state = CONN_STATE_CLOSED;
    
    g_http_conn_map.erase(connHandle);
    tcpConn.forceClose();
}

void HttpConn::OnConnect(net_handle_t handle)
{
    printf("OnConnect, handle=%d\n", handle);
    sockfd = handle;
    state = CONN_STATE_CONNECTED;
    g_http_conn_map.insert(make_pair(connHandle, this));
    //TODO SET_CALLBACK
}

void HttpConn::OnRead()
{
	for (;;)
	{
		uint32_t free_buf_len = inBuf.GetAllocSize() - inBuf.GetWriteOffset();
		if (free_buf_len < READ_BUF_SIZE + 1)
			inBuf.Extend(READ_BUF_SIZE + 1);

		int ret = netlib_recv(sockfd, inBuf.GetBuffer() + inBuf.GetWriteOffset(), READ_BUF_SIZE);
		if (ret <= 0)
			break;

		inBuf.IncWriteOffset(ret);

		lastRecvTick = get_tick_count();
	}

	// 每次请求对应一个HTTP连接，所以读完数据后，不用在同一个连接里面准备读取下个请求
	char* in_buf = (char*)inBuf.GetBuffer();
	uint32_t buf_len = inBuf.GetWriteOffset();
	in_buf[buf_len] = '\0';
    
    // 如果buf_len 过长可能是受到攻击，则断开连接
    // 正常的url最大长度为2048，我们接受的所有数据长度不得大于1K
    if(buf_len > 1024)
    {
        LOGE("get too much data:%s ", in_buf);
        Close();
        return;
    }

	//LOGE("OnRead, buf_len=%u, conn_handle=%u\n", buf_len, connHandle); // for debug

	
	m_cHttpParser.ParseHttpContent(in_buf, buf_len);

	if (m_cHttpParser.IsReadAll()) {
		string url =  m_cHttpParser.GetUrl();
		if (strncmp(url.c_str(), "/msg_server", 11) == 0) {
            string content = m_cHttpParser.GetBodyContent();
            _HandleMsgServRequest(url, content);
		} else {
			LOGE("url unknown, url=%s ", url.c_str());
			Close();
		}
	}
}

void HttpConn::OnWrite()
{
	if (!busy)
		return;

	int ret = netlib_send(sockfd, outBuf.GetBuffer(), outBuf.GetWriteOffset());
	if (ret < 0)
		ret = 0;

	int out_buf_size = (int)outBuf.GetWriteOffset();

	outBuf.Read(NULL, ret);

	if (ret < out_buf_size)
	{
		busy = true;
		LOGE("not send all, remain=%d ", outBuf.GetWriteOffset());
	}
	else
	{
        OnWriteComlete();
		busy = false;
	}
}

void HttpConn::OnClose()
{
    Close();
}

void HttpConn::OnTimer(uint64_t curr_tick)
{
	if (curr_tick > lastRecvTick + HTTP_CONN_TIMEOUT) {
		LOGE("HttpConn timeout, handle=%d ", connHandle);
		Close();
	}
}

void HttpConn::_HandleMsgServRequest(string& url, string& post_data)
{
    msg_serv_info_t* pMsgServInfo;
    uint32_t min_user_cnt = (uint32_t)-1;
    map<uint32_t, msg_serv_info_t*>::iterator it_min_conn = g_msg_serv_info.end();
    map<uint32_t, msg_serv_info_t*>::iterator it;
    if(g_msg_serv_info.size() <= 0)
    {
        Json::Value value;
        value["code"] = 1;
        value["msg"] = "没有msg_server";
        string strContent = value.toStyledString();
        char* szContent = new char[HTTP_RESPONSE_HTML_MAX];
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, strContent.length(), strContent.c_str());
        Send((void*)szContent, strlen(szContent));
        delete [] szContent;
        return ;
    }
    
    for (it = g_msg_serv_info.begin() ; it != g_msg_serv_info.end(); it++) {
        pMsgServInfo = it->second;
        if ( (pMsgServInfo->cur_conn_cnt < pMsgServInfo->max_conn_cnt) &&
            (pMsgServInfo->cur_conn_cnt < min_user_cnt)) {
            it_min_conn = it;
            min_user_cnt = pMsgServInfo->cur_conn_cnt;
        }
    }
    
    if (it_min_conn == g_msg_serv_info.end()) {
        LOGE("All TCP MsgServer are full ");
        Json::Value value;
        value["code"] = 2;
        value["msg"] = "负载过高";
        string strContent = value.toStyledString();
        char* szContent = new char[HTTP_RESPONSE_HTML_MAX];
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, strContent.length(), strContent.c_str());
        Send((void*)szContent, strlen(szContent));
        delete [] szContent;
        return;
    } else {
        Json::Value value;
        value["code"] = 0;
        value["msg"] = "";
        if(pIpParser->isTelcome(GetPeerIP()))
        {
            value["priorIP"] = string(it_min_conn->second->ip_addr1);
            value["backupIP"] = string(it_min_conn->second->ip_addr2);
            value["msfsPrior"] = strMsfsUrl;
            value["msfsBackup"] = strMsfsUrl;
        }
        else
        {
            value["priorIP"] = string(it_min_conn->second->ip_addr2);
            value["backupIP"] = string(it_min_conn->second->ip_addr1);
            value["msfsPrior"] = strMsfsUrl;
            value["msfsBackup"] = strMsfsUrl;
        }
        value["discovery"] = strDiscovery;
        value["port"] = int2string(it_min_conn->second->port);
        string strContent = value.toStyledString();
        char* szContent = new char[HTTP_RESPONSE_HTML_MAX];
        uint32_t nLen = strContent.length();
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, strContent.c_str());
        Send((void*)szContent, strlen(szContent));
        delete [] szContent;
        return;
    }
}

void HttpConn::OnWriteComlete()
{
    LOGD("write complete ");
    Close();
}

