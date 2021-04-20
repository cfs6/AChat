/*
 * User.h
 *
 *  Created on: 2021年4月14日
 *      Author: cfs
 */

#ifndef USER_H_
#define USER_H_
#include <string>
using namespace std;
typedef struct DBUserInfo_t
{
    uint32_t nId;//用户ID
    uint8_t nSex;// 用户性别 1.男;2.女
    uint8_t nStatus; // 用户状态0 正常， 1 离职
    uint32_t nDeptId;// 所属部门
    string strNick;// 花名
    string strDomain;// 花名拼音
    string strName;// 真名
    string strTel;// 手机号码
    string strEmail;// Email
    string strAvatar;// 头像
    string sign_info;//个性签名
    DBUserInfo_t& operator=(const DBUserInfo_t& rhs)
    {
        if(this != &rhs)
        {
            nId = rhs.nId;
            nSex = rhs.nSex;
            nStatus = rhs.nStatus;
            nDeptId = rhs.nDeptId;
            strNick = rhs.strNick;
            strDomain = rhs.strDomain;
            strName = rhs.strName;
            strTel = rhs.strTel;
            strEmail = rhs.strEmail;
            strAvatar = rhs.strAvatar;
            sign_info = rhs.sign_info;
        }
        return *this;
    }

} DBUserInfo_t;




#endif /* USER_H_ */
