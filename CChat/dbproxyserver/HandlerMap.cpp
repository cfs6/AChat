#include "HandlerMap.h"

#include "business/Login.h"
#include "business/MessageContent.h"
#include "business/RecentSession.h"
#include "business/UserAction.h"
#include "business/MessageCounter.h"
#include "business/GroupAction.h"
#include "business/DepartAction.h"
#include "business/FileAction.h"
#include "IM.BaseDefine.pb.h"
#include "AsyncLog.h"

using namespace IM::BaseDefine;


HandlerMap* HandlerMap::handlerInstance = nullptr;

HandlerMap* HandlerMap::getInstance()
{
	if (!handlerInstance) {
		handlerInstance = new HandlerMap();
		handlerInstance->init();
	}

	return handlerInstance;
}

void HandlerMap::init()
{
    //DB_PROXY是命名空间，不是类名
	// Login validate
	handlerMap.insert(make_pair(uint32_t(CID_OTHER_VALIDATE_REQ), DB_PROXY::doLogin));
    m_handler_map.insert(make_pair(uint32_t(CID_LOGIN_REQ_PUSH_SHIELD), DB_PROXY::doPushShield));
    m_handler_map.insert(make_pair(uint32_t(CID_LOGIN_REQ_QUERY_PUSH_SHIELD), DB_PROXY::doQueryPushShield));

    // recent session
    m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_RECENT_CONTACT_SESSION_REQUEST), DB_PROXY::getRecentSession));
    m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_REMOVE_SESSION_REQ), DB_PROXY::deleteRecentSession));

    // users
    m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_USER_INFO_REQUEST), DB_PROXY::getUserInfo));
    m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_ALL_USER_REQUEST), DB_PROXY::getChangedUser));
    m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_DEPARTMENT_REQUEST), DB_PROXY::getChgedDepart));
    m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_CHANGE_SIGN_INFO_REQUEST), DB_PROXY::changeUserSignInfo));


    // message content
    m_handler_map.insert(make_pair(uint32_t(CID_MSG_DATA), DB_PROXY::sendMessage));
    m_handler_map.insert(make_pair(uint32_t(CID_MSG_LIST_REQUEST), DB_PROXY::getMessage));
    m_handler_map.insert(make_pair(uint32_t(CID_MSG_UNREAD_CNT_REQUEST), DB_PROXY::getUnreadMsgCounter));
    m_handler_map.insert(make_pair(uint32_t(CID_MSG_READ_ACK), DB_PROXY::clearUnreadMsgCounter));
    m_handler_map.insert(make_pair(uint32_t(CID_MSG_GET_BY_MSG_ID_REQ), DB_PROXY::getMessageById));
    m_handler_map.insert(make_pair(uint32_t(CID_MSG_GET_LATEST_MSG_ID_REQ), DB_PROXY::getLatestMsgId));

    // device token
    m_handler_map.insert(make_pair(uint32_t(CID_LOGIN_REQ_DEVICETOKEN), DB_PROXY::setDevicesToken));
    m_handler_map.insert(make_pair(uint32_t(CID_OTHER_GET_DEVICE_TOKEN_REQ), DB_PROXY::getDevicesToken));

    //push 推送设置
    m_handler_map.insert(make_pair(uint32_t(CID_GROUP_SHIELD_GROUP_REQUEST), DB_PROXY::setGroupPush));
    m_handler_map.insert(make_pair(uint32_t(CID_OTHER_GET_SHIELD_REQ), DB_PROXY::getGroupPush));


    // group
    m_handler_map.insert(make_pair(uint32_t(CID_GROUP_NORMAL_LIST_REQUEST), DB_PROXY::getNormalGroupList));
    m_handler_map.insert(make_pair(uint32_t(CID_GROUP_INFO_REQUEST), DB_PROXY::getGroupInfo));
    m_handler_map.insert(make_pair(uint32_t(CID_GROUP_CREATE_REQUEST), DB_PROXY::createGroup));
    m_handler_map.insert(make_pair(uint32_t(CID_GROUP_CHANGE_MEMBER_REQUEST), DB_PROXY::modifyMember));


    // file
    m_handler_map.insert(make_pair(uint32_t(CID_FILE_HAS_OFFLINE_REQ), DB_PROXY::hasOfflineFile));
    m_handler_map.insert(make_pair(uint32_t(CID_FILE_ADD_OFFLINE_REQ), DB_PROXY::addOfflineFile));
    m_handler_map.insert(make_pair(uint32_t(CID_FILE_DEL_OFFLINE_REQ), DB_PROXY::delOfflineFile));

}

pduHandlerFunc HandlerMap::getHandler(uint32_t pduType)
{
	auto it = handlerMap.find(pduType);
	if(it != handlerMap.end())
	{
		return it->second;
	}
	else
	{
		LOGE("Wrong pduType! can not find int handlerMap!");
		return nullptr;
	}
}








