/*
 * ImPduBase.cpp
 *
 *  Created on: 2013-8-27
 *      Author: ziteng@mogujie.com
 */

#include "util.h"
#include "ImPduBase.h"
#include "IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;

ImPdu::ImPdu()
{
	m_pdu_header.version = IM_PDU_VERSION;
	m_pdu_header.flag = 0;
	m_pdu_header.service_id = SID_OTHER;
	m_pdu_header.command_id = 0;
	m_pdu_header.seq_num = 0;
    m_pdu_header.reversed = 0;
}

uchar_t* ImPdu::GetBuffer()
{
//    return m_buf.GetBuffer();
	return buf.peek();
}

uint32_t ImPdu::GetLength()
{
//    return m_buf.GetWriteOffset();
	return buf.writableBytes();
}
//TODO
uchar_t* ImPdu::GetBodyData()
{
    return m_buf.GetBuffer() + sizeof(PduHeader_t);
}

uint32_t ImPdu::GetBodyLength()
{
    uint32_t body_length = 0;
    body_length = m_buf.GetWriteOffset() - sizeof(PduHeader_t);
    return body_length;
}

void ImPdu::WriteHeader()
{
	uchar_t* buf = GetBuffer();

	CByteStream::WriteInt32(buf, GetLength());
	CByteStream::WriteUint16(buf + 4, m_pdu_header.version);
	CByteStream::WriteUint16(buf + 6, m_pdu_header.flag);
	CByteStream::WriteUint16(buf + 8, m_pdu_header.service_id);
	CByteStream::WriteUint16(buf + 10, m_pdu_header.command_id);
    CByteStream::WriteUint16(buf + 12, m_pdu_header.seq_num);
    CByteStream::WriteUint16(buf + 14, m_pdu_header.reversed);
}

void ImPdu::SetVersion(uint16_t version)
{
	uchar_t* buf = GetBuffer();
	CByteStream::WriteUint16(buf + 4, version);
}

void ImPdu::SetFlag(uint16_t flag)
{
	uchar_t* buf = GetBuffer();
	CByteStream::WriteUint16(buf + 6, flag);
}

void ImPdu::SetServiceId(uint16_t service_id)
{
    uchar_t* buf = GetBuffer();
    CByteStream::WriteUint16(buf + 8, service_id);
}

void ImPdu::SetCommandId(uint16_t command_id)
{
    uchar_t* buf = GetBuffer();
    CByteStream::WriteUint16(buf + 10, command_id);
}

void ImPdu::SetError(uint16_t error)
{
    uchar_t* buf = GetBuffer();
    CByteStream::WriteUint16(buf + 12, error);
}

void ImPdu::SetSeqNum(uint16_t seq_num)
{
	uchar_t* buf = GetBuffer();
	CByteStream::WriteUint16(buf + 12, seq_num);
}

void ImPdu::SetReversed(uint32_t reversed)
{
    uchar_t* buf = GetBuffer();
    CByteStream::WriteUint16(buf+14, reversed);
}

int ImPdu::ReadPduHeader(uchar_t* buf, uint32_t len)
{
	int ret = -1;
	if (len >= IM_PDU_HEADER_LEN && buf) {
		CByteStream is(buf, len);

		is >> m_pdu_header.length;
		is >> m_pdu_header.version;
		is >> m_pdu_header.flag;
		is >> m_pdu_header.service_id;
		is >> m_pdu_header.command_id;
		is >> m_pdu_header.seq_num;
        is >> m_pdu_header.reversed;

		ret = 0;
	}

	return ret;
}

ImPdu* ImPdu::ReadPdu(uchar_t *buf, uint32_t len)
{
	uint32_t pdu_len = 0;
	if (!IsPduAvailable(buf, len, pdu_len))
		return NULL;

	uint16_t service_id = CByteStream::ReadUint16(buf + 8);
	uint16_t command_id = CByteStream::ReadUint16(buf + 10);
	ImPdu* pPdu = NULL;

    pPdu = new ImPdu();
    //pPdu->_SetIncomingLen(pdu_len);
    //pPdu->_SetIncomingBuf(buf);
    pPdu->Write(buf, pdu_len);
    pPdu->ReadPduHeader(buf, IM_PDU_HEADER_LEN);
    
    return pPdu;
}

bool ImPdu::IsPduAvailable(uchar_t* buf, uint32_t len, uint32_t& pdu_len)
{
	if (len < IM_PDU_HEADER_LEN)
		return false;

	pdu_len = CByteStream::ReadUint32(buf);
	if (pdu_len > len)
	{
		//log("pdu_len=%d, len=%d\n", pdu_len, len);
		return false;
	}
    
    if(0 == pdu_len)
    {
        throw CPduException(1, "pdu_len is 0");
    }

	return true;
}

void ImPdu::SetPBMsg(const google::protobuf::MessageLite* msg)
{
    //设置包体，则需要重置下空间
    m_buf.Read(NULL, m_buf.GetWriteOffset());
    m_buf.Write(NULL, sizeof(PduHeader_t));
    uint32_t msg_size = msg->ByteSize();
    uchar_t* szData = new uchar_t[msg_size];
    //ALLOC_FAIL_ASSERT(szData)
    if (!msg->SerializeToArray(szData, msg_size))
    {
        log("pb msg miss required fields.");
    }
    
    m_buf.Write(szData, msg_size);
    delete []szData;
    WriteHeader();
}

