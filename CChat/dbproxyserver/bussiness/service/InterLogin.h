#ifndef __INTERLOGIN_H__
#define __INTERLOGIN_H__

#include "LoginStrategy.h"

class CInterLoginStrategy :public LoginStrategy
{
public:
    virtual bool doLogin(const std::string& strName, const std::string& strPass, IM::BaseDefine::UserInfo& user);
};

#endif /*defined(__INTERLOGIN_H__) */
