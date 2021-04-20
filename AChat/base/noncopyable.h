/*
 * noncopyable.h
 *
 *  Created on: 2021年1月24日
 *      Author: cfs
 */

#ifndef NONCOPYABLE_H_
#define NONCOPYABLE_H_

class noncopyable
{
protected:
	noncopyable() = default;
	~noncopyable() = default;

	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
};



#endif /* NONCOPYABLE_H_ */
