#include "FileSession.h"
#include <string.h>
#include <sstream>
#include <list>
#include "../net/TcpConnection.h"
#include "../net/ProtocolStream.h"
#include "../base/AsyncLog.h"
#include "../base/Singleton.h"
#include "FileMsg.h"
#include "FileManager.h"

using namespace net;

#define MAX_PACKAGE_SIZE    50 * 1024 * 1024

FileSession::FileSession(const std::shared_ptr<TcpConnection>& conn, const char* filebasedir) :
TcpSession(conn), 
m_id(0),
m_seq(0),
m_strFileBaseDir(filebasedir),
m_bFileUploading(false)
{
}

FileSession::~FileSession()
{

}

void FileSession::onRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receivTime)
{
    while (true)
    {
        if (pBuffer->readableBytes() < (size_t)sizeof(file_msg_header))
        {
            //LOGI << "buffer is not enough for a package header, pBuffer->readableBytes()=" << pBuffer->readableBytes() << ", sizeof(msg)=" << sizeof(file_msg);
            return;
        }

        file_msg_header header;
        memcpy(&header, pBuffer->peek(), sizeof(file_msg_header));

        if (header.packagesize <= 0 || header.packagesize > MAX_PACKAGE_SIZE)
        {          
            LOGE("Illegal package header size: %lld, close TcpConnection, client: %s", header.packagesize, conn->peerAddress().toIpPort().c_str());
            LOG_DEBUG_BIN((unsigned char*)&header, sizeof(header));
            conn->forceClose();
            return;
        }

        if (pBuffer->readableBytes() < (size_t)header.packagesize + sizeof(file_msg_header))
            return;

        pBuffer->retrieve(sizeof(file_msg_header));
        std::string inbuf;
        inbuf.append(pBuffer->peek(), header.packagesize);
        pBuffer->retrieve(header.packagesize);
        if (!process(conn, inbuf.c_str(), inbuf.length()))
        {
            LOGE("Process error, close TcpConnection, client: %s", conn->peerAddress().toIpPort().c_str());
            conn->forceClose();
        }
    }

}

bool FileSession::process(const std::shared_ptr<TcpConnection>& conn, const char* inbuf, size_t length)
{
    BinaryStreamReader readStream(inbuf, length);
    int32_t cmd;
    if (!readStream.ReadInt32(cmd))
    {
        LOGE("read cmd error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    //int seq;
    if (!readStream.ReadInt32(m_seq))
    {
        LOGE("read seq error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    std::string filemd5;
    size_t md5length;
    if (!readStream.ReadString(&filemd5, 0, md5length) || md5length == 0)
    {
        LOGE("read filemd5 error, client: ", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    int64_t offset;
    if (!readStream.ReadInt64(offset))
    {
        LOGE("read offset error, client: %s" , conn->peerAddress().toIpPort().c_str());
        return false;
    }

    int64_t filesize;
    if (!readStream.ReadInt64(filesize))
    {
        LOGE("read filesize error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    string filedata;
    size_t filedatalength;
    if (!readStream.ReadString(&filedata, 0, filedatalength))
    {
        LOGE("read filedata error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }
   
    LOGI("Request from client: cmd: %d, seq: %d, filemd5: %s, md5length: %d, offset: %lld, filesize: %lld, filedata length: %lld, header.packagesize: %lld, client: %s",
         cmd, m_seq, filemd5.c_str(), md5length, offset, filesize, (int64_t)filedata.length(), (int64_t)length, conn->peerAddress().toIpPort().c_str());


    switch (cmd)
    {
        case msg_type_upload_req:
            return onUploadFileResponse(filemd5, offset, filesize, filedata, conn);
        case msg_type_download_req:
        {
            int32_t clientNetType;
            if (!readStream.ReadInt32(clientNetType))
            {
                LOGE("read clientNetType error, client: %s", conn->peerAddress().toIpPort().c_str());
                return false;
            }

            //if (filedatalength != 0)
            //    return false;
            return onDownloadFileResponse(filemd5, clientNetType, conn);
        }
           

        default:
            //pBuffer->retrieveAll();
            LOGE("unsupport cmd, cmd: %d, client: %s", cmd, conn->peerAddress().toIpPort().c_str());
            //conn->forceClose();
            return false;
    }// end switch

    ++ m_seq;

    return true;
}

bool FileSession::onUploadFileResponse(const std::string& filemd5, int64_t offset, int64_t filesize, const std::string& filedata, const std::shared_ptr<TcpConnection>& conn)
{
    if (filemd5.empty())
    {
        LOGE("Empty filemd5, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }
    if (Singleton<FileManager>::Instance().isFileExsit(filemd5.c_str()) && !m_bFileUploading)
    {
        offset = filesize;      
        string dummyfiledata;      
        send(msg_type_upload_resp, m_seq, file_msg_error_complete, filemd5, offset, filesize, dummyfiledata);
        
        
        LOGI("Response to client: cmd=msg_type_upload_resp, errorcode: file_msg_error_complete, filemd5: %s, offset: %lld, filesize: %lld, client: %s", 
              filemd5.c_str(), offset, filesize, conn->peerAddress().toIpPort().c_str());

        return true;
    }
    
    if (offset == 0)
    {
        std::string filename = m_strFileBaseDir;
        filename += filemd5;
        m_fp = fopen(filename.c_str(), "wb");
        if (m_fp == NULL)
        {
            LOGE("fopen file error, filemd5: %s, client: %s", filemd5.c_str(), conn->peerAddress().toIpPort().c_str());
            return false;
        }

        m_bFileUploading = true;
    }
    else
    {
        if (m_fp == NULL)
        {
            resetFile();
            LOGE("file pointer should not be null, filemd5: %s, offset: %d, client: %s", filemd5.c_str(), offset, conn->peerAddress().toIpPort().c_str());
            return false;
        }
    }

    if (fseek(m_fp, offset, SEEK_SET) == -1)
    {
        LOGE("fseek error, filemd5: %s, errno: %d, errinfo: %s, filedata.length(): %lld, m_fp: 0x%x, buffer size is 512*1024, client: %s",
            filemd5.c_str(), errno, strerror(errno), filedata.length(), m_fp, conn->peerAddress().toIpPort().c_str());

        resetFile();
        return false;
    }

    if (fwrite((char*)filedata.c_str(), 1, filedata.length(), m_fp) != filedata.length())
    {
        resetFile();
        LOGE("fwrite error, filemd5: %s, errno: %d, errinfo: %s, filedata.length(): %lld, m_fp: 0x%x, buffer size is 512*1024, client: %s",
            filemd5.c_str(), errno, strerror(errno), filedata.length(), m_fp, conn->peerAddress().toIpPort().c_str());
        return false;
    }

    if (fflush(m_fp) != 0)
    {
        LOGE("fflush error, filemd5: %s, errno: %d, errinfo: %s, filedata.length(): %lld, m_fp: 0x%x, buffer size is 512*1024, client: %s",
            filemd5.c_str(), errno, strerror(errno), filedata.length(), m_fp, conn->peerAddress().toIpPort().c_str());

        return false;
    }

    int32_t errorcode = file_msg_error_progress;

    if (offset + (int64_t)filedata.length() == filesize)
    {
        offset = filesize;
        errorcode = file_msg_error_complete;
        Singleton<FileManager>::Instance().addFile(filemd5.c_str());
        resetFile();
    }

    string dummyfiledatax;
    send(msg_type_upload_resp, m_seq, errorcode, filemd5, offset, filesize, dummyfiledatax);
       
    std::string errorcodestr = "file_msg_error_progress";
    if (errorcode == file_msg_error_complete)
        errorcodestr = "file_msg_error_complete";

    LOGI("Response to client: cmd=msg_type_upload_resp, errorcode: %s, filemd5: %s, offset: %lld, filesize: %lld, upload percent: %d%%, client: %s", 
         errorcodestr.c_str(), filemd5.c_str(), offset, filesize, (int32_t)(offset * 100 / filesize), conn->peerAddress().toIpPort().c_str());

    return true;
}

bool FileSession::onDownloadFileResponse(const std::string& filemd5, int32_t clientNetType, const std::shared_ptr<TcpConnection>& conn)
{
    if (filemd5.empty())
    {
        LOGE("Empty filemd5, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }
    
    if (!Singleton<FileManager>::Instance().isFileExsit(filemd5.c_str()))
    {
        string dummyfiledata;
        int64_t notExsitFileOffset = 0;
        int64_t notExsitFileSize = 0;
        send(msg_type_download_resp, m_seq, file_msg_error_not_exist, filemd5, notExsitFileOffset, notExsitFileSize, dummyfiledata);
        LOGE("filemd5 not exsit, filemd5: %s, clientNetType: %d, client: %s", filemd5.c_str() , clientNetType, conn->peerAddress().toIpPort().c_str());
        std::ostringstream os;
        os << "Response to client: cmd=msg_type_download_resp, errorcode=file_msg_error_not_exist "
                 << ", filemd5: " << filemd5 << ", clientNetType: " << clientNetType
                 << ", offset: 0"
                 << ", filesize: 0"
                 << ", filedataLength: 0"
                 << ", client:" << conn->peerAddress().toIpPort();
        LOGI(os.str().c_str());
        return true;
    }

    if (m_fp == NULL)
    {
        string filename = m_strFileBaseDir;
        filename += filemd5;
        m_fp = fopen(filename.c_str(), "rb+");
        if (m_fp == NULL)
        {
            LOGE("fopen file error, filemd5: %s, clientNetType: %d, client: %s", filemd5.c_str(), clientNetType, conn->peerAddress().toIpPort().c_str());
            return false;
        }
        
        if (fseek(m_fp, 0, SEEK_END) == -1)
        {
            LOGE("fseek error, m_filesize: %lld, errno: %d, filemd5: %s, clientNetType: %d, client: %s", m_currentDownloadFileSize, errno, filemd5.c_str(), clientNetType, conn->peerAddress().toIpPort().c_str());
            return false;
        }

        m_currentDownloadFileSize = ftell(m_fp);
        if (m_currentDownloadFileSize <= 0)
		{
            LOGE("m_filesize: %lld, errno: %d, filemd5: %s, clientNetType: %d, client: %s", m_currentDownloadFileSize, errno, filemd5.c_str(), clientNetType, conn->peerAddress().toIpPort().c_str());
			return false;
		}

        if (fseek(m_fp, 0, SEEK_SET) == -1)
        {
            
            LOGE("fseek error, m_filesize: %lld, errno: %d, filemd5: %s, clientNetType: %d, client: %s", m_currentDownloadFileSize, errno, filemd5.c_str(), clientNetType, conn->peerAddress().toIpPort().c_str());
            return false;     
        }
    }

    string filedata;
    
    //m_offset += offset;
    int64_t currentSendSize = 512 * 1024;
    if (clientNetType == client_net_type_cellular)
        currentSendSize = 64 * 1024;

    char buffer[512 * 1024] = { 0 };
    if (m_currentDownloadFileSize <= m_currentDownloadFileOffset + currentSendSize)
    {
        currentSendSize = m_currentDownloadFileSize - m_currentDownloadFileOffset;
    }

	if (currentSendSize <= 0 || fread(buffer, currentSendSize, 1, m_fp) != 1)
	{
        std::ostringstream os;
        os << "fread error, filemd5: " << filemd5 << ", clientNetType: " << clientNetType
		    << ", errno: " << errno << ", errinfo: " << strerror(errno)
		    << ", currentSendSize: " << currentSendSize
		    << ", m_fp: " << m_fp
            << ", buffer size is " << currentSendSize
		    << ", connection name:" << conn->peerAddress().toIpPort();
        LOGE(os.str().c_str());
	}

    int sendoffset = m_currentDownloadFileOffset;
    m_currentDownloadFileOffset += currentSendSize;
    filedata.append(buffer, currentSendSize);      

    int errorcode = file_msg_error_progress;
    if (m_currentDownloadFileOffset == m_currentDownloadFileSize)
        errorcode = file_msg_error_complete;

    send(msg_type_download_resp, m_seq, errorcode, filemd5, sendoffset, m_currentDownloadFileSize, filedata);

    std::ostringstream os2;
    os2 << "Response to client: cmd=msg_type_download_resp, errorcode: " << (errorcode == file_msg_error_progress ? "file_msg_error_progress" : "file_msg_error_complete")
             << ", filemd5: " << filemd5 << ", clientNetType: " << clientNetType
             << ", sendoffset: " << sendoffset
             << ", filesize: " << m_currentDownloadFileSize
             << ", filedataLength: " << filedata.length()
             << ", download percent: " << (m_currentDownloadFileOffset * 100 / m_currentDownloadFileSize) << "%"
             << ", client:" << conn->peerAddress().toIpPort();

    LOGI(os2.str().c_str());

    if (errorcode == file_msg_error_complete)
        resetFile();

     return true;
}

void FileSession::resetFile()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
        m_currentDownloadFileOffset = 0;
        m_currentDownloadFileSize = 0;
		m_fp = NULL;
        m_bFileUploading = false;
    }
}
