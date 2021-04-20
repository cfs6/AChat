#include "../DBPool.h"
#include "../HttpClient.h"
#include "AudioModel.h"
#include "AsyncLog.h"
using namespace std;

//AudioModel
AudioModel* AudioModel::m_pInstance = NULL;

/**
 *  构造函数
 */
AudioModel::AudioModel()
{

}

/**
 *  析构函数
 */
AudioModel::~AudioModel()
{

}

/**
 *  单例
 *
 *  @return 单例的指针
 */
AudioModel* AudioModel::getInstance()
{
	if (!m_pInstance) {
		m_pInstance = new AudioModel();
	}

	return m_pInstance;
}

/**
 *  这只语音存储的url地址
 *
 *  @param strFileSite 上传的url
 */
void AudioModel::setUrl(string& strFileSite)
{
    m_strFileSite = strFileSite;
    if(m_strFileSite[m_strFileSite.length()] != '/')
    {
        m_strFileSite += "/";
    }
}

/**
 *  读取语音消息
 *
 *  @param nAudioId 语音的Id
 *  @param cMsg     语音消息，引用
 *
 *  @return bool 成功返回true，失败返回false
 */
bool AudioModel::readAudios(list<IM::BaseDefine::MsgInfo>& lsMsg)
{
    if(lsMsg.empty())
    {
        return true;
    }
    bool bRet = false;
    DBManager* pDBManger = DBManager::getInstance();
    DBConn* pDBConn = pDBManger->getDBConn("teamtalk_slave");
    if (pDBConn)
    {
        for (auto it=lsMsg.begin(); it!=lsMsg.end(); )
        {
            IM::BaseDefine::MsgType nType = it->msg_type();
            if((IM::BaseDefine::MSG_TYPE_GROUP_AUDIO ==  nType) || (IM::BaseDefine::MSG_TYPE_SINGLE_AUDIO == nType))
            {
                string strSql = "select * from IMAudio where id=" + it->msg_data();
                ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
                if (pResultSet)
                {
                    while (pResultSet->next()) {
                        uint32_t nCostTime = pResultSet->getInt("duration");
                        uint32_t nSize = pResultSet->getInt("size");
                        string strPath = pResultSet->getString("path");
                        readAudioContent(nCostTime, nSize, strPath, *it);
                    }
                    ++it;
                    delete pResultSet;
                }
                else
                {
                    LOGE("no result for sql:%s", strSql.c_str());
                    it = lsMsg.erase(it);
                }
            }
            else
            {
                ++it;
            }
        }
        pDBManger->relDBConn(pDBConn);
        bRet = true;
    }
    else
    {
        LOGE("no connection for teamtalk_slave");
    }
    return bRet;
}

/**
 *  存储语音消息
 *
 *  @param nFromId     发送者Id
 *  @param nToId       接收者Id
 *  @param nCreateTime 发送时间
 *  @param pAudioData  指向语音消息的指针
 *  @param nAudioLen   语音消息的长度
 *
 *  @return 成功返回语音id，失败返回-1
 */
int AudioModel::saveAudioInfo(uint32_t nFromId, uint32_t nToId, uint32_t nCreateTime, const char* pAudioData, uint32_t nAudioLen)
{
	// parse audio data
	uint32_t nCostTime = CByteStream::ReadUint32((uchar_t*)pAudioData);
	uchar_t* pRealData = (uchar_t*)pAudioData + 4;
	uint32_t nRealLen = nAudioLen - 4;
    int nAudioId = -1;
    
	CHttpClient httpClient;
	string strPath = httpClient.UploadByteFile(m_strFileSite, pRealData, nRealLen);
	if (!strPath.empty())
    {
        DBManager* pDBManager = DBManager::getInstance();
        DBConn* pDBConn = pDBManager->getDBConn("teamtalk_master");
        if (pDBConn)
        {
            uint32_t nStartPos = 0;
            string strSql = "insert into IMAudio(`fromId`, `toId`, `path`, `size`, `duration`, `created`) "\
            "values(?, ?, ?, ?, ?, ?)";
            replace_mark(strSql, nFromId, nStartPos);
            replace_mark(strSql, nToId, nStartPos);
            replace_mark(strSql, strPath, nStartPos);
            replace_mark(strSql, nRealLen, nStartPos);
            replace_mark(strSql, nCostTime, nStartPos);
            replace_mark(strSql, nCreateTime, nStartPos);
            if (pDBConn->executeUpdate(strSql.c_str()))
            {
                nAudioId = pDBConn->getInsertId();
                LOGE("audioId=%d", nAudioId);
            } else
            {
                LOGE("sql failed: %s", strSql.c_str());
            }
            pDBManager->relDBConn(pDBConn);
        }
        else
        {
            LOGE("no db connection for teamtalk_master");
        }
	}
    else
    {
        LOGE("upload file failed");
    }
	return nAudioId;
}

/**
 *  读取语音的具体内容
 *
 *  @param nCostTime 语音时长
 *  @param nSize     语音大小
 *  @param strPath   语音存储的url
 *  @param cMsg      msg结构体
 *
 *  @return 成功返回true，失败返回false
 */
bool AudioModel::readAudioContent(uint32_t nCostTime, uint32_t nSize, const string& strPath, IM::BaseDefine::MsgInfo& cMsg)
{
	if (strPath.empty() || nCostTime == 0 || nSize == 0) {
		return false;
	}

	// 分配内存，写入音频时长
    AudioMsgInfo cAudioMsg;
    uchar_t* pData = new uchar_t [4 + nSize];
	cAudioMsg.data = pData;
	CByteStream::WriteUint32(cAudioMsg.data, nCostTime);
    cAudioMsg.data_len = 4;
    cAudioMsg.fileSize = nSize;

	// 获取音频数据，写入上面分配的内存
    CHttpClient httpClient;
    if(!httpClient.DownloadByteFile(strPath, &cAudioMsg))
    {
        delete [] pData;
        return false;
    }

    LOGE("download_path=%s, data_len=%d", strPath.c_str(), cAudioMsg.data_len);
    cMsg.set_msg_data((const char*)cAudioMsg.data, cAudioMsg.data_len);
    
    delete [] pData;
    return true;
}
