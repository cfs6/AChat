

#ifndef HANDLERMAP_H_
#define HANDLERMAP_H_

#include "../base/util.h"
#include "ProxyTask.h"

typedef std::map<uint32_t, pduHandlerFunc> pHandlerMap;

class HandlerMap
{
public:
	virtual ~HandlerMap();
	void init();
	pduHandlerFunc getHandler(uint32_t pdu_type);

	static HandlerMap* getInstance();

private:
	HandlerMap();
	HandlerMap(const HandlerMap&) = delete;
	HandlerMap& operator=(const HandlerMap) = delete;
private:
	static HandlerMap*         handlerInstance;
	static pHandlerMap         handlerMap;
};



#endif /* HANDLERMAP_H_ */
