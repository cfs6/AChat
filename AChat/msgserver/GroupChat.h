#ifndef GROUPCHAT_H_
#define GROUPCHAT_H_

#include "ImPduBase.h" 
#include <set>
#include <hash_map>
using namespace std;
typedef set<uint32_t> group_member_t;
typedef hash_map<uint32_t, group_member_t*> group_map_t;

class CMsgConn;

class GroupChat
{
public:
	virtual ~GroupChat() {}

	static GroupChat* GetInstance();

    void HandleClientGroupNormalRequest(ImPdu* pPdu, CMsgConn* pFromConn);
    void HandleGroupNormalResponse(ImPdu* pPdu);
    
    void HandleClientGroupInfoRequest(ImPdu* pPdu, CMsgConn* pFromConn);
    void HandleGroupInfoResponse(ImPdu* pPdu);

	void HandleGroupMessage(ImPdu* pPdu);
    void HandleGroupMessageBroadcast(ImPdu* pPdu);
    
	void HandleClientGroupCreateRequest(ImPdu* pPdu, CMsgConn* pFromConn);
	void HandleGroupCreateResponse(ImPdu* pPdu);
    
	void HandleClientGroupChangeMemberRequest(ImPdu* pPdu, CMsgConn* pFromConn);
	void HandleGroupChangeMemberResponse(ImPdu* pPdu);
	void HandleGroupChangeMemberBroadcast(ImPdu* pPdu);

    void HandleClientGroupShieldGroupRequest(ImPdu* pPdu,
        CMsgConn* pFromConn);
    
    void HandleGroupShieldGroupResponse(ImPdu* pPdu);
    void HandleGroupGetShieldByGroupResponse(ImPdu* pPdu);
private:
	GroupChat() {}

	void _SendPduToUser(ImPdu* pPdu, uint32_t user_id, CMsgConn* pReqConn = NULL);
private:

	static GroupChat* s_group_chat_instance;

	group_map_t m_group_map;
};


#endif /* GROUPCHAT_H_ */
