
#ifndef PROXYTASK_H_
#define PROXYTASK_H_

#include "Task.h"
#include "util.h"
#include "ImPduBase.h"
#include <functional>
#include <memory>

typedef std::function<void(ImPdu* pPdu, uint32_t connUUid)> pduHandlerFunc;

class ProxyTask: public Task
{
public:
	ProxyTask(uint32_t conn_UUid, pduHandlerFunc pdu_handler, ImPdu* pPdu);
	~ProxyTask();

	virtual void run();

private:
	uint32_t                    connUUid;
	pduHandlerFunc              pduHandler;
	std::unique_ptr<ImPdu>     pdu;
};

#endif /* PROXYTASK_H_ */
