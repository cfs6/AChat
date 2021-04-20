#ifndef __FILESERVICE_H__
#define __FILESERVICE_H__

#include "ImPduBase.h"

namespace DB_PROXY {
    void hasOfflineFile(ImPdu* pPdu, uint32_t conn_uuid);
    void addOfflineFile(ImPdu* pPdu, uint32_t conn_uuid);
    void delOfflineFile(ImPdu* pPdu, uint32_t conn_uuid);
};

#endif
