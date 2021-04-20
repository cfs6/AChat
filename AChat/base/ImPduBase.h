/*
 * ImPduBase.h
 *
 *  Created on: 2021年3月28日
 *      Author: cfs
 */

#ifndef IMPDUBASE_H_
#define IMPDUBASE_H_

#include "UtilPdu.h"
#include "pb/google/protobuf/message_lite.h"
#include "net/Buffer.h"
using namespace net;
#define IM_PDU_HEADER_LEN		16
#define IM_PDU_VERSION			1


#define ALLOC_FAIL_ASSERT(p) if (p == NULL) { \
throw CPduException(m_pdu_header.service_id, m_pdu_header.command_id, ERROR_CODE_ALLOC_FAILED, "allocate failed"); \
}

#define CHECK_PB_PARSE_MSG(ret) { \
    if (ret == false) \
    {\
        log("parse pb msg failed.");\
        return;\
    }\
}


#define DLL_MODIFIER

//////////////////////////////
typedef struct {
    uint32_t 	length;		  // the whole pdu length
    uint16_t 	version;	  // pdu version number
    uint16_t	flag;		  // not used
    uint16_t	service_id;	  //
    uint16_t	command_id;	  //
    uint16_t	seq_num;     // 包序号
    uint16_t    reversed;    // 保留
} PduHeader_t;

class DLL_MODIFIER ImPdu
{
public:
    ImPdu();
    virtual ~ImPdu() {}

    uchar_t* GetBuffer();
    uint32_t GetLength();
    uchar_t* GetBodyData();
    uint32_t GetBodyLength();


    uint16_t GetVersion() { return m_pdu_header.version; }
    uint16_t GetFlag() { return m_pdu_header.flag; }
    uint16_t GetServiceId() { return m_pdu_header.service_id; }
    uint16_t GetCommandId() { return m_pdu_header.command_id; }
    uint16_t GetSeqNum() { return m_pdu_header.seq_num; }
    uint32_t GetReversed() { return m_pdu_header.reversed; }

    void SetVersion(uint16_t version);
    void SetFlag(uint16_t flag);
    void SetServiceId(uint16_t service_id);
    void SetCommandId(uint16_t command_id);
    void SetError(uint16_t error);
    void SetSeqNum(uint16_t seq_num);
    void SetReversed(uint32_t reversed);
    void WriteHeader();

    static bool IsPduAvailable(uchar_t* buf, uint32_t len, uint32_t& pdu_len);
    static ImPdu* ReadPdu(uchar_t* buf, uint32_t len);
    void Write(uchar_t* buf, uint32_t len) { m_buf.Write((void*)buf, len); }
    int ReadPduHeader(uchar_t* buf, uint32_t len);
    void SetPBMsg(const google::protobuf::MessageLite* msg);

protected:
//    CSimpleBuffer	m_buf;
    Buffer          buf;
    PduHeader_t		m_pdu_header;
};




#endif /* IMPDUBASE_H_ */
