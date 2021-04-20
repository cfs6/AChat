
#ifndef LOGIN_H_
#define LOGIN_H_

#include "ImPduBase.h"

namespace DB_PROXY {

void doLogin(ImPdu* pPdu, uint32_t conn_uuid);

};


#endif /* LOGIN_H_ */
