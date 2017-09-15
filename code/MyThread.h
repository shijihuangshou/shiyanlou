#ifndef __MYTHREAD_H__
#define __MYTHREAD_H__
#include "MySyncObjs.h"
#include "BaseType.h"
#include <signal.h>
#include<pthread.h>
#include "BaseCode.h"

const int TIME_WAITINGCLOSE = 3000;

#define SUPPORT_SUSPEND_X
#define WINAPI
#include <semaphore.h>

class CMyThread;
class IThreadEvent
{
public:
	virtual int OnThreadCreate(CMyThread*) = 0;
	virtual int OnThreadDestroy(void) = 0;
	virtual int OnThreadWorkEvent(void) = 0;
	virtual int OnThreadProcess (void) = 0;
};

class CMyThread
{
public:
	CMyThread(IThreadEvent& event);
	virtual ~CMyThread();
public:
	enum{SUSPEND=true,RUN=false,};
	BOOL Create(BOOL bSuspend=RUN,DWORD dwWorkInterval=INFINITE);
	BOOL Close();
#ifdef SUPPORT_SUSPEND
	void Resume();
	void Suspend();
#endif
	static void ResumeThread(pthread_t tid);
	static void SuspendThread(pthread_t tid);
	DWORD GetWorkInterval() const {return (DWORD)m_tWorkInterval;}
	void SetWorkEvent(){sem_post(&m_semWorkEvent); sem_post(&m_semThread); }
	void SetWorkInterval(DWORD dwWorkInterval){__sync_lock_test_and_set(&m_tWorkInterval,dwWorkInterval);}
	enum
	{
		STATUS_INIT = 0,
		STATUS_SUSPEND,
		STATUS_RUNNING,
		STATUS_CLOSING,
		STATUS_CLOSED,
	};
	
	int GetStatus() const{return m_nStatus;}
	void _SetStatus(int nStatus){m_nStatus = nStatus;}
	
	pthread_t GetThreadId() const {return m_tid;}
	pthread_t GetThreadHandle() const {return m_tid;}
	
protected:
	IThreadEvent& m_event;
	time_t m_tWorkInterval;
private:
	BOOL m_fTerminated;
	volatile int m_nStatus;
	pthread_t m_tid;
	
	sem_t m_semThread;
	sem_t m_semExitEvent;
	sem_t m_semWorkEvent;
	sem_t m_semClosed;
	
#ifdef SUPPORT_SUSPEND
	sem_t m_semSuspend;
	sem_t m_semResume;
#endif
private:
	static void* Run(void* pThreadParameter);
	static DWORD  RunThread(void* pThreadParameter);
public:
	static CMyThread* CreateNew(IThreadEvent& refEvent,BOOL bSuspend = RUN,DWORD dwWorkInterval = INFINITE);

};


#endif