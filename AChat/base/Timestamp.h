/*
 * Timestamp.h
 *
 *  Created on: 2021年2月7日
 *      Author: cfs
 */

#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_

#include<string>
#include<stdint.h>
#include<algorithm>

using namespace std;

class Timestamp
{
public:
	Timestamp():microSecondSinceEpoch(0){}

	explicit Timestamp(int64_t microSecondSinceEpoch);

	Timestamp& operator+=(Timestamp lhs)
	{
		this->microSecondSinceEpoch += lhs.microSecondSinceEpoch;
		return *this;
	}

	Timestamp& operator+=(int64_t lhs)
	{
		this->microSecondSinceEpoch += lhs;
		return *this;
	}

	Timestamp& operator-=(Timestamp lhs)
	{
		this->microSecondSinceEpoch -= lhs.microSecondSinceEpoch;
		return *this;
	}

	Timestamp& operator-=(int64_t lhs)
	{
		this->microSecondSinceEpoch -= lhs;
		return *this;
	}

	void swap(Timestamp& that)
	{
		std::swap(microSecondSinceEpoch, that.microSecondSinceEpoch);
	}

	string toString()const;
	string toFormattedString(bool showMicroseconds = true)const;

	bool valid()const
	{
		return microSecondSinceEpoch > 0;
	}

	int64_t microSecondsSinceEpoch()const
	{
		return microSecondSinceEpoch;
	}
	time_t secondsSinceEpoch()const
	{
		return static_cast<time_t>(microSecondSinceEpoch);
	}

	static Timestamp now();
	//make Timestamp invalid
	static Timestamp invalid();


	static const int kMicroSecondPerSecond = 1000 * 1000;
private:
	int64_t microSecondSinceEpoch;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
	return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator>(Timestamp lhs, Timestamp rhs)
{
	return rhs<lhs;
}

inline bool operator<=(Timestamp lhs, Timestamp rhs)
{
	return !(lhs>rhs);
}
inline bool operator>=(Timestamp lhs, Timestamp rhs)
{
	return !(lhs<rhs);
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
	return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline bool operator!=(Timestamp lhs, Timestamp rhs)
{
	return !(lhs==rhs);
}

inline double timeDifference(Timestamp high, Timestamp low)
{
	int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
	return static_cast<double>(diff) / Timestamp::kMicroSecondPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, int64_t microseconds)
{
	return Timestamp(timestamp.microSecondsSinceEpoch() + microseconds);
}

#endif /* TIMESTAMP_H_ */
