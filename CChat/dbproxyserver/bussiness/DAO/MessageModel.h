
#ifndef MESSAGE_MODEL_H_
#define MESSAGE_MODEL_H_

#include <list>
#include <string>

#include "util.h"
#include "ImPduBase.h"
#include "AudioModel.h"
#include "IM.BaseDefine.pb.h"
using namespace std;

class MessageModel {
public:
	virtual ~MessageModel();
	static MessageModel* getInstance();

    bool sendMessage(uint32_t nRelateId, uint32_t nFromId, uint32_t nToId, IM::BaseDefine::MsgType nMsgType, uint32_t nCreateTime,
                     uint32_t nMsgId, string& strMsgContent);
    bool sendAudioMessage(uint32_t nRelateId, uint32_t nFromId, uint32_t nToId, IM::BaseDefine::MsgType nMsgType, uint32_t nCreateTime,
                          uint32_t nMsgId, const char* pMsgContent, uint32_t nMsgLen);
    void getMessage(uint32_t nUserId, uint32_t nPeerId, uint32_t nMsgId, uint32_t nMsgCnt,
                    list<IM::BaseDefine::MsgInfo>& lsMsg);
    bool clearMessageCount(uint32_t nUserId, uint32_t nPeerId);
    uint32_t getMsgId(uint32_t nRelateId);
    void getUnreadMsgCount(uint32_t nUserId, uint32_t &nTotalCnt, list<IM::BaseDefine::UnreadInfo>& lsUnreadCount);
    void getLastMsg(uint32_t nFromId, uint32_t nToId, uint32_t& nMsgId, string& strMsgData, IM::BaseDefine::MsgType & nMsgType, uint32_t nStatus = 0);
    void getUnReadCntAll(uint32_t nUserId, uint32_t &nTotalCnt);
    void getMsgByMsgId(uint32_t nUserId, uint32_t nPeerId, const list<uint32_t>& lsMsgId, list<IM::BaseDefine::MsgInfo>& lsMsg);
    bool resetMsgId(uint32_t nRelateId);
private:
	MessageModel();
	MessageModel(const MessageModel&) = delete;
	MessageModel& operator=(const MessageModel&) = delete;
    void incMsgCount(uint32_t nFromId, uint32_t nToId);
private:
	static MessageModel*	m_pInstance;
};



#endif /* MESSAGE_MODEL_H_ */
