#ifndef GROUP_MESSAGE_MODEL_H_
#define GROUP_MESSAGE_MODEL_H_

#include <list>
#include <string>

#include "util.h"
#include "ImPduBase.h"
#include "AudioModel.h"
#include "GroupModel.h"
#include "IM.BaseDefine.pb.h"

using namespace std;


class GroupMessageModel {
public:
	virtual ~GroupMessageModel();
	static GroupMessageModel* getInstance();
    
    bool sendMessage(uint32_t nFromId, uint32_t nGroupId, IM::BaseDefine::MsgType nMsgType, uint32_t nCreateTime, uint32_t nMsgId, const string& strMsgContent);
    bool sendAudioMessage(uint32_t nFromId, uint32_t nGroupId, IM::BaseDefine::MsgType nMsgType, uint32_t nCreateTime, uint32_t nMsgId,const char* pMsgContent, uint32_t nMsgLen);
    void getMessage(uint32_t nUserId, uint32_t nGroupId, uint32_t nMsgId, uint32_t nMsgCnt,
                    list<IM::BaseDefine::MsgInfo>& lsMsg);
    bool clearMessageCount(uint32_t nUserId, uint32_t nGroupId);
    uint32_t getMsgId(uint32_t nGroupId);
    void getUnreadMsgCount(uint32_t nUserId, uint32_t &nTotalCnt, list<IM::BaseDefine::UnreadInfo>& lsUnreadCount);
    void getLastMsg(uint32_t nGroupId, uint32_t& nMsgId, string& strMsgData, IM::BaseDefine::MsgType & nMsgType, uint32_t& nFromId);
    void getUnReadCntAll(uint32_t nUserId, uint32_t &nTotalCnt);
    void getMsgByMsgId(uint32_t nUserId, uint32_t nGroupId, const list<uint32_t>& lsMsgId, list<IM::BaseDefine::MsgInfo>& lsMsg);
    bool resetMsgId(uint32_t nGroupId);
private:
    GroupMessageModel();
    bool incMessageCount(uint32_t nUserId, uint32_t nGroupId);

private:
	static GroupMessageModel*	m_pInstance;
};



#endif /* MESSAGE_MODEL_H_ */
