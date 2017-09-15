#ifndef _SYNC_OBJECTS_H
#define _SYNC_OBJECTS_H

#include<pthread.h>
#include<time.h>
#include<semaphore.h>

class ILockObj
{
public:
	virtual void Lock(void)=0;
	virtual void UnLock(void)=0;
};

class CMyCriticalSection: public ILockObj
{
public:
	friend class CMyCondition;
	CMyCriticalSection()
	{
		pthread_mutex_init(&m_tMutex,NULL);
	};
	virtual ~CMyCriticalSection()
	{
		pthread_mutex_destroy(&m_tMutex);
	};
	void Lock(void)
	{
		pthread_mutex_lock(&m_tMutex);
	};
	void UnLock(void)
	{
		pthread_mutex_unlock(&m_tMutex);
	};
	
private:
	pthread_mutex_t m_tMutex;
};

class CMyMutex: public ILockObj
{
public:
	friend class CMyCondition;
	CMyMutex()
	{
		pthread_mutex_init(&m_tMutex,NULL);
	};
	virtual ~CMyMutex()
	{
		pthread_mutex_destroy(&m_tMutex);
	};
	void Lock(void)
	{
		pthread_mutex_lock(&m_tMutex);
	};
	bool TimeLock(time_t tMilliSecs)
	{
		struct timespec abs_time;
		clock_gettime(CLOCK_REALTIME,&abs_time);
		abs_time.tv_sec += tMilliSecs /1000;
		if(0 == pthread_mutex_timedlock(&m_tMutex,&abs_time))
			return true;
		return false;
	};
	void UnLock(void)
	{
		pthread_mutex_unlock(&m_tMutex);
	};
	
private:
	pthread_mutex_t m_tMutex;
};

class CMyCondition
{
public:
	CMyCondition()
	{
		pthread_cond_init(&m_tCond,NULL);
	}
	~CMyCondition()
	{
		pthread_cond_destroy(&m_tCond);
	}
	int wait_cond(CMyCriticalSection& lockobj)
	{
		return pthread_cond_wait(&m_tCond,&(lockobj.m_tMutex));
	}
	int timewait_cond(CMyCriticalSection& lockobj,int nSec)
	{
		struct timespec to;
		to.tv_sec = time(NULL) + nSec;
		to.tv_nsec = 0;
		return pthread_cond_timedwait(&m_tCond,&(lockobj.m_tMutex),&to);
	}
	void notify_one()
	{
		pthread_cond_signal(&m_tCond);
	}
private:
	pthread_cond_t m_tCond;
};

class CMySingleLock
{
public:
	CMySingleLock(ILockObj& lock)
	: m_lock(lock)
	{
		m_lock.Lock();
	}
	~CMySingleLock(void)
	{
		m_lock.UnLock();
	}
private:
	ILockObj& m_lock;
};

class CMyTimedSingleLock
{
public:
	CMyTimedSingleLock(CMyMutex& lock,time_t tMilliSecs)
	: m_lock(lock)
	{
		m_bLocked = m_lock.TimeLock(tMilliSecs);
	}
	~CMyTimedSingleLock(void)
	{
		if(m_bLocked)
			m_lock.UnLock();
	}
	bool IsLocked() const
	{
		return m_bLocked;
	}
private:
	CMyMutex& m_lock;
	bool m_bLocked;
};

class IRWLockObj
{
public:
	virtual void LockRead(void) = 0;
	virtual void LockWrite(void) = 0;
	virtual void UnLock(void) = 0;
};

class CMyRWLock: public IRWLockObj
{
public:
	CMyRWLock()
	{
		m_nWaitingReaders = 0;
		m_nWaitingWriters = 0;
		m_nActive = 0;
		sem_init(&m_semReaders,0,0);
		sem_init(&m_semWriters,0,0);
	}
	~CMyRWLock()
	{
		sem_destroy(&m_semReaders);
		sem_destroy(&m_semWriters);
	}
	virtual void LockRead()
	{
		bool writeLock;
		{
			CMySingleLock lock(m_CritSec);
			writeLock = (m_nActive < 0) || (m_nWaitingWriters > 0);
			if(writeLock)
			{
				m_nWaitingReaders++;
			}
			else
			{
				m_nActive++;
			}
		}
		if(writeLock)
		{
			sem_wait(&m_semReaders);
		}
		
	}
	virtual void LockWrite()
	{
		bool readLock;
		{
			CMySingleLock lock(m_CritSec);
			readLock = (m_nActive != 0);
			if(readLock)
			{
				++m_nWaitingWriters;
			}
			else
			{
				m_nActive = -1;
			}
		}
		if(readLock)
		{
			sem_wait(&m_semWriters);
		}
	}
	virtual void Unlock()
	{
		{
			CMySingleLock lock (m_CritSec);
			if(m_nActive > 0)
			{
				--m_nActive;
			}
			else
			{
				++m_nActive;
			}
			if(0 == m_nActive)
			{
				if(m_nWaitingWriters > 0)
				{
					m_nActive = -1;
					m_nWaitingWriters--;
					sem_post(&m_semWriters);
				}
				else if (m_nWaitingReaders > 0)
				{
					m_nActive = m_nWaitingReaders;
					m_nWaitingReaders = 0;
					for(int i =0; i<m_nActive;i++)
					{
						sem_post(&m_semReaders);
					}
				}
			}
		}
		
	}
private:
	CMyCriticalSection m_CritSec;
	int m_nWaitingReaders;
	int m_nWaitingWriters;
	int m_nActive;
	sem_t m_semReaders;
	sem_t m_semWriters;
};

class CMyReadLock
{
public:
	CMyReadLock(IRWLockObj& lock)
	:m_lock(lock)
	{
		m_lock.LockRead();
	}
	~CMyReadLock()
	{
		m_lock.UnLock();
	}
private:
	IRWLockObj& m_lock;
};

class CMyWriteLock
{
public:
	CMyWriteLock(IRWLockObj& lock)
	:m_lock(lock)
	{
		m_lock.LockRead();
	}
	~CMyWriteLock()
	{
		m_lock.UnLock();
	}
private:
	IRWLockObj& m_lock;
};






#endif