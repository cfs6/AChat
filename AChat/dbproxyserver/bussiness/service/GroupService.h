
#ifndef GROUPSERVICE_H_
#define GROUPSERVICE_H_

#include "ImPduBase.h"

namespace DB_PROXY
{

    void createGroup(ImPdu* pPdu, uint32_t conn_uuid);
    
    void getNormalGroupList(ImPdu* pPdu, uint32_t conn_uuid);
    
    void getGroupInfo(ImPdu* pPdu, uint32_t conn_uuid);
    
    void modifyMember(ImPdu* pPdu, uint32_t conn_uuid);
    
    void setGroupPush(ImPdu* pPdu, uint32_t conn_uuid);
    
    void getGroupPush(ImPdu* pPdu, uint32_t conn_uuid);

};


#endif
