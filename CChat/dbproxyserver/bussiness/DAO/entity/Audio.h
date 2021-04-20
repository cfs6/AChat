/*
 * Audio.h
 *
 *  Created on: 2021年4月16日
 *      Author: cfs
 */

#ifndef AUDIO_H_
#define AUDIO_H_
#include <string>
using namespace std;

typedef struct AudioMsgInfo{
    uint32_t    audioId;
    uint32_t    fileSize;
    uint32_t    data_len;
    uchar_t*    data;
    string      path;

} AudioMsgInfo_t;



#endif /* AUDIO_H_ */
