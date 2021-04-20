#ifndef __EXTERLOGIN_H__
#define __EXTERLOGIN_H__

#include "LoginStrategy.h"

class ExterLoginStrategy:public LoginStrategy
{
public:
    virtual bool doLogin(const std::string& strName, const std::string& strPass, IM::BaseDefine::UserInfo& user);
};
#endif /*defined(__EXTERLOGIN_H__) */
