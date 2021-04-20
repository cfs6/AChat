#ifndef __USERMODEL_H__
#define __USERMODEL_H__

#include "IM.BaseDefine.pb.h"
#include "ImPduBase.h"
//#include "public_define.h"
using namespace std;
//DAO
struct DBUserInfo_t;
class UserModel
{
public:
    static UserModel* getInstance();
    ~UserModel();
    void getChangedId(uint32_t& nLastTime, list<uint32_t>& lsIds);
    void getUsers(list<uint32_t> lsIds, list<IM::BaseDefine::UserInfo>& lsUsers);
    bool getUser(uint32_t nUserId, DBUserInfo_t& cUser);

    bool updateUser(DBUserInfo_t& cUser);
    bool insertUser(DBUserInfo_t& cUser);
//    void getUserByNick(const list<string>& lsNicks, list<IM::BaseDefine::UserInfo>& lsUsers);
    void clearUserCounter(uint32_t nUserId, uint32_t nPeerId, IM::BaseDefine::SessionType nSessionType);
    void setCallReport(uint32_t nUserId, uint32_t nPeerId, IM::BaseDefine::ClientType nClientType);

    bool updateUserSignInfo(uint32_t user_id, const string& sign_info);
    bool getUserSingInfo(uint32_t user_id, string* sign_info);
    bool updatePushShield(uint32_t user_id, uint32_t shield_status);
    bool getPushShield(uint32_t user_id, uint32_t* shield_status);

private:
    UserModel();
private:
    static UserModel* m_pInstance;
};

#endif /*defined(__USERMODEL_H__) */
