#include<pthread.h>
#ifndef _MYTLS_H_
#define _MYTLS_H_


//enum{PTHREAD_ONCE_INIT = 0,};

typedef DWORD pthread_key_t;
const DWORD INVALID_PTHREAD_KEY = DWORD(0xFFFFFFFF);

class MyTLSPtr
{
public:
	MyTLSPtr():m_key(INVALID_PTHREAD_KEY)
	{
		init();
	}
	virtual ~MyTLSPtr()
	{
		destory();
	}
public:
	template<typename T>
	void set(T* pPtr)
	{
		pthread_setspecific(m_key,pPtr);
	}
	template<typename T>
	T* get() const
	{
		return static_cast<T*>(pthread_getspecific(m_key));
	}
	
	void init()
	{
		if(m_key != INVALID_PTHREAD_KEY)
		{
			destory();
		}
		pthread_key_create(&m_key,NULL);
	}
	
	void destory()
	{
		if(m_key == INVALID_PTHREAD_KEY)
		{
			return;
		}
		pthread_key_delete(m_key);
		m_key = INVALID_PTHREAD_KEY;
	}

private:
	pthread_key_t m_key;
};

template<typename T>
class MyTLSTypePtr : public MyTLSPtr
{
public:
	MyTLSTypePtr():MyTLSPtr()
	{
		
	}
	virtual ~MyTLSTypePtr()
	{
		
	}
public:
	void set(T* pPtr)
	{
		MyTLSPtr::set<T>(pPtr);
	}
	T* get() const
	{
		return MyTLSPtr::get<T>();
	}

};

#endif