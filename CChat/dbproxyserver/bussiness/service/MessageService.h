#ifndef MESSAGESERVICE_H_
#define MESSAGESERVICE_H_

#include "ImPduBase.h"
namespace DB_PROXY {

	void getMessage(ImPdu* pPdu, uint32_t conn_uuid);

	void sendMessage(ImPdu* pPdu, uint32_t conn_uuid);

	void getMessageById(ImPdu* pPdu, uint32_t conn_uuid);

	void getLatestMsgId(ImPdu* pPdu, uint32_t conn_uuid);

    void getUnreadMsgCounter(ImPdu* pPdu, uint32_t conn_uuid);
    void clearUnreadMsgCounter(ImPdu* pPdu, uint32_t conn_uuid);

    void setDevicesToken(ImPdu* pPdu, uint32_t conn_uuid);
    void getDevicesToken(ImPdu* pPdu, uint32_t conn_uuid);
};



#endif /* MESSAGESERVICE_H_ */
