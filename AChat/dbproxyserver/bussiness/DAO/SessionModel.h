
#ifndef __SESSIONMODEL_H__
#define __SESSIONMODEL_H__

#include "ImPduBase.h"
#include "IM.BaseDefine.pb.h"
#include <string>
using namespace std;
struct DBUserInfo_t;
class SessionModel
{
public:
    static SessionModel* getInstance();
    ~SessionModel() {}

    void getRecentSession(uint32_t userId, uint32_t lastTime, list<IM::BaseDefine::ContactSessionInfo>& lsContact);
    uint32_t getSessionId(uint32_t nUserId, uint32_t nPeerId, uint32_t nType, bool isAll);
    bool updateSession(uint32_t nSessionId, uint32_t nUpdateTime);
    bool removeSession(uint32_t nSessionId);
    uint32_t addSession(uint32_t nUserId, uint32_t nPeerId, uint32_t nType);
    
private:
    SessionModel() {};
    void fillSessionMsg(uint32_t nUserId, list<IM::BaseDefine::ContactSessionInfo>& lsContact);
private:
    static SessionModel* m_pInstance;
};

#endif /*defined(__SESSIONMODEL_H__) */
