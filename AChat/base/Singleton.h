/*
 * Singleton.h
 *
 *  Created on: 2021年2月9日
 *      Author: cfs
 */

#ifndef SINGLETON_H_
#define SINGLETON_H_
#pragma once

template<typename T>
class Singleton
{
public:
	static T& Instance()
	{
		if(nullptr == instance)
		{
			instance =  new T();
		}
		return instance;
	}
private:
	Singleton();
	~Singleton();

	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

	static void init()
	{
		instance = new T();
	}

	static void destroy()
	{
		delete instance;
	}
private:
	static T* instance;
};

template<typename T>
T* Singleton<T>::instance = nullptr;


#endif /* SINGLETON_H_ */
