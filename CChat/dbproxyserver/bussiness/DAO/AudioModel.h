#ifndef AUDIO_MODEL_H_
#define AUDIO_MODEL_H_

#include <list>
#include <map>
//#include "public_define.h"
#include "util.h"
#include "IM.BaseDefine.pb.h"

using namespace std;

struct Audio;
class AudioModel {
public:
	virtual ~AudioModel();

	static AudioModel* getInstance();
    void setUrl(string& strFileUrl);
    
    bool readAudios(list<IM::BaseDefine::MsgInfo>& lsMsg);
    
    int saveAudioInfo(uint32_t nFromId, uint32_t nToId, uint32_t nCreateTime, const char* pAudioData, uint32_t nAudioLen);

private:
	AudioModel();
	AudioModel(const AudioModel&) = delete;
	AudioModel& operator=(const AudioModel&) = delete;
//    void GetAudiosInfo(uint32_t nAudioId, IM::BaseDefine::MsgInfo& msg);
    bool readAudioContent(uint32_t nCostTime, uint32_t nSize, const string& strPath, IM::BaseDefine::MsgInfo& msg);
    
private:
	static AudioModel*	m_pInstance;
    string m_strFileSite;
};



#endif /* AUDIO_MODEL_H_ */
