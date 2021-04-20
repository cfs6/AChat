
#ifndef __USER_SERVICE_H__
#define __USER_SERVICE_H__

#include "ImPduBase.h"

namespace DB_PROXY {

    void getUserInfo(ImPdu* pPdu, uint32_t conn_uuid);
    void getChangedUser(ImPdu* pPdu, uint32_t conn_uuid);
    void changeUserSignInfo(ImPdu* pPdu, uint32_t conn_uuid);
    void doPushShield(ImPdu* pPdu, uint32_t conn_uuid);
    void doQueryPushShield(ImPdu* pPdu, uint32_t conn_uuid);
};


#endif
