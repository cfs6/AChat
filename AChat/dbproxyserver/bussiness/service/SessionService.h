#ifndef SESSIONSERVICE_H_
#define SESSIONSERVICE_H_

namespace DB_PROXY {

    void getRecentSession(ImPdu* pPdu, uint32_t conn_uuid);

    void deleteRecentSession(ImPdu* pPdu, uint32_t conn_uuid);

};

#endif /* SESSIONSERVICE_H_ */
