#include "ipparser.h"

IpParser::IpParser()
{
}

IpParser::~IpParser()
{
    
}

bool IpParser::isTelcome(const char *pIp)
{
    if(!pIp)
    {
        return false;
    }
    CStrExplode strExp((char*)pIp,'.');
    if(strExp.GetItemCnt() != 4)
    {
        return false;
    }
    return true;
}
