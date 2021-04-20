/*
 * Timestamp.cpp
 *
 *  Created on: 2021年2月9日
 *      Author: cfs
 */

#include "Timestamp.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdio.h>


static_assert(sizeof(Timestamp) == sizeof(int64_t), "sizeof(Timestamp) error");

Timestamp::Timestamp(int64_t microSecondsSinceEpoch_):
		microSecondSinceEpoch(microSecondsSinceEpoch_){}

string Timestamp::toString()const
{
	char buf[64] = {0};
	int64_t seconds = microSecondSinceEpoch / kMicroSecondPerSecond;
	int64_t microseconds = microSecondSinceEpoch % kMicroSecondPerSecond;
	snprintf(buf, sizeof(buf)-1, "%lld.%06lld", (long long)seconds, (long long) microseconds);
}

string Timestamp::toFormattedString(bool showMicroseconds)const
{
	time_t seconds = static_cast<time_t>(microSecondSinceEpoch / kMicroSecondPerSecond);
	struct tm tm_time;

	struct tm *ptm;
	ptm = localtime(&seconds);
	tm_time = *ptm;

	char buf[32] = {0};
	if(showMicroseconds)
	{
		int microseconds = static_cast<int>(microSecondSinceEpoch % kMicroSecondPerSecond);
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%2d:%02d.%06d",
				tm_time.tm_year+1900, tm_time.tm_mon + 1, tm_time.tm_mday,
				tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%2d:%02d",
				tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
				tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
	}
	return buf;
}

Timestamp Timestamp::now()
{
	chrono::time_point<chrono::system_clock, chrono::microseconds> now =
			chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now());

	int64_t microSeconds = now.time_since_epoch().count();
	Timestamp time(microSeconds);
	return time;
}

Timestamp Timestamp::invalid()
{
	return Timestamp();
}






