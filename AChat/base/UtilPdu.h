#ifndef UTILPDU_H_
#define UTILPDU_H_

#include "ostype.h"
#include <set>
#include <map>
#include <list>
#include <string>
using namespace std;

	#define DLL_MODIFIER


// exception code
#define ERROR_CODE_PARSE_FAILED 		1
#define ERROR_CODE_WRONG_SERVICE_ID		2
#define ERROR_CODE_WRONG_COMMAND_ID		3
#define ERROR_CODE_ALLOC_FAILED			4

class PduException {
public:
	PduException(uint32_t service_id, uint32_t command_id, uint32_t error_code, const char* error_msg)
	{
		m_service_id = service_id;
		m_command_id = command_id;
		m_error_code = error_code;
		m_error_msg = error_msg;
	}

	PduException(uint32_t error_code, const char* error_msg)
	{
		m_service_id = 0;
		m_command_id = 0;
		m_error_code = error_code;
		m_error_msg = error_msg;
	}

	virtual ~PduException() {}

	uint32_t GetServiceId() { return m_service_id; }
	uint32_t GetCommandId() { return m_command_id; }
	uint32_t GetErrorCode() { return m_error_code; }
	char* GetErrorMsg() { return (char*)m_error_msg.c_str(); }
private:
	uint32_t	m_service_id;
	uint32_t	m_command_id;
	uint32_t	m_error_code;
	string		m_error_msg;
};


char* idtourl(uint32_t id);
uint32_t urltoid(const char* url);

#endif /* UTILPDU_H_ */
