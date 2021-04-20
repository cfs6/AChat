#ifndef _IPPARSER_H_
#define _IPPARSER_H_

#include "util.h"

class IpParser
{
    public:
        IpParser();
        virtual ~IpParser();
    bool isTelcome(const char* ip);
};

#endif

