#include "MyThread.h"
#include <time.h>
#include <errno.h>

#define RESUME_SIG SIGUSR2
#define SUSPEND_SIG SIGUSR1

static sigset_t wait_mask;
static __thread int suspended = 0;  //每个线程都会有一个独立的

void resume_handler(int sig,siginfo_t* pInfo,void* pVoid)
{
	suspended = 0;
}

void suspend_handler(int sig,siginfo_t* pInfo,void* pVoid)
{
	if(suspended)
		return;
	suspended = 1;
	do
	{
		sigsuspend(&wait_mask); //收到挂起信号之后，除了挂起和调度信号，其他都屏蔽了
	}
	while(suspended);
}

//加了信号掩码，处理挂起和调度的信号

CMyThread::CMyThread(IThreadEvent& event)
:m_event(event),m_fTerminated(false)
{
	m_tid = 0;
	__sync_lock_release(&m_tWorkInterval,0);
	struct sigaction sa;
	sigfillset(&wait_mask);
	sigdelset(&wait_mask,RESUME_SIG);
	sigdelset(&wait_mask,SUSPEND_SIG);
	
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = &resume_handler;
	sigaction(RESUME_SIG,&sa,NULL);
	
	sa.sa_sigaction = &suspend_handler;
	sigaction(SUSPEND_SIG,&sa,NULL);
	
	m_nStatus = STATUS_INIT;
}

CMyThread::~CMyThread()
{
	if(STATUS_INIT == m_nStatus)
		return;
#ifdef SUPPORT_SUSPEND
	if(SUPPORT_SUSPEND == m_nStatus)
	{
		this->Resume();
	}
#endif
	if(STATUS_CLOSING != m_nStatus && STATUS_CLOSED != m_nStatus)
	{
		this->Close();
	}
	
	struct timespec ts;
	if(clock_gettime(CLOCK_REALTIME, &ts) == 0)
	{
		ts.tv_sec += TIME_WAITINGCLOSE / 1000;
		int nRet = 0;
		while((nRet = sem_timedwait(&m_semClosed,&ts)) == -1 && EINTR == errno )
		{
			continue;
		}
		if(-1 == nRet)
		{
			if(ETIMEDOUT == errno)
			{
				
			}
			else
			{
				printf("wait m_semClosed error (error=%d)\n",errno);
			}
		}
	}
	int nResult = pthread_join(m_tid,NULL);
	if(nResult != 0)
	{
		printf("pthread_join error %s\n",strerror(nResult));
	}
	
	if(sem_destroy(&m_semThread) || sem_destroy(&m_semExitEvent) || 
	sem_destroy(&m_semWorkEvent) || sem_destroy(&m_semClosed))
	{
		printf("sem_destory error\n");
	}
	m_tid = 0;
}

BOOL CMyThread::Close()
{
	if(sem_post(&m_semExitEvent) == 0 && sem_post(&m_semThread) == 0)
	{
		m_fTerminated = true;
		m_nStatus = STATUS_CLOSING;
		return true;
	}
	else
	{
		return false;
	}
}

BOOL CMyThread::Create(BOOL bSuspend,DWORD dwWorkInterval)
{
#ifndef SUPPORT_SUSPEND
	bSuspend = RUN;
#endif
	if(STATUS_INIT != m_nStatus)
	{
		return false;
	}
	if(sem_init(&m_semThread,0,0) || sem_init(&m_semExitEvent,0,0) ||
	sem_init(&m_semWorkEvent,0,0) || sem_init(&m_semClosed,0,0))
	{
		printf("sem init error\n");
		return false;
	}
	m_nStatus = STATUS_RUNNING;
#ifdef SUPPORT_SUSPEND
	if(bSuspend)
	{
		Suspend();
	}
#endif
	__sync_lock_test_and_set(&m_tWorkInterval,dwWorkInterval);
	int n = pthread_create(&m_tid,NULL,&CMyThread::Run,(void*)this);
	if(n != 0)
	{
		if(sem_destroy(&m_semThread) || sem_destroy(&m_semExitEvent) || 
		sem_destroy(&m_semWorkEvent) || sem_destroy(&m_semClosed))
		{
			
			printf("sem_destory error\n");
		}
		errno = n;
		printf("pthread_create error\n");
		return false;
	}
	return true;
}

#ifdef SUPPORT_SUSPEND
void CMyThread::Resume()
{
	if(STATUS_SUSPEND != m_nStatus)
	{
		return;
	}
	pthread_kill(m_tid,RESUME_SIG);
}

void CMyThread::Suspend()
{
	if(STATUS_RUNNING != m_nStatus)
	{
		return;
	}
	pthread_kill(m_tid,SUSPEND_SIG);
}
#endif

void CMyThread::ResumeThread(pthread_t tid)
{
	pthread_kill(tid,RESUME_SIG);
}

void CMyThread::SuspendThread(pthread_t tid)
{
	pthread_kill(tid,SUSPEND_SIG);
}

CMyThread* CMyThread::CreateNew(IThreadEvent& refEvent,BOOL bSuspend,DWORD dwWorkInterval)
{
	CMyThread* pThread = new CMyThread(refEvent);
	if(!pThread)
	{
		return NULL;
	}
	if(!pThread->Create(bSuspend,dwWorkInterval))
	{
		delete pThread;
		return NULL;
	}
	return pThread;
}

void* CMyThread::Run(void* pThreadParameter)
{
	try
	{
		int n = 0;
		if(n == 0)
		{
			CMyThread* pThread = (CMyThread*)pThreadParameter;
			if(pThread)
			{
				pThread->RunThread(pThreadParameter);
			}
		}
		else
		{
			errno = n;
			printf("pthread_detach error\n");
		}
	}
	catch(...)
	{
		printf("CATCH: *** CMyThread::Run() crash! ***\n");
	}
	return (void*)0;
}

DWORD CMyThread::RunThread(void* pThreadParameter)
{
	try
	{
		CMyThread* pThread = (CMyThread*)pThreadParameter;
		if(!pThread)
			return 1;
		if(-1 == pThread->m_event.OnThreadCreate(pThread))
			return 2;
		
		time_t tLastTick = ::_TimeGet();
		time_t tLastSecond = ::time(NULL);
		time_t tCostTime = 0;
		
#ifdef USE_THREAD_LOCAL_TIMEPTR
	g_TimerTLSPtr.set<time_t>(&tLastSecond);
	g_TimerMSTLSPtr.set<time_t>(&tLastTick);
#endif
		while(!pThread->m_fTerminated)
		{
			int rval;
			if(pThread->m_tWorkInterval > 0)
			{
				struct timespec ts;
				if(clock_gettime(CLOCK_REALTIME,&ts) == -1)
				{
					continue;
				}
				
				time_t tWorkInterval = 0; //这个是毫秒
				if(tCostTime < pThread->m_tWorkInterval)
				{
					tWorkInterval = pThread->m_tWorkInterval - tCostTime;
					ts.tv_sec += tWorkInterval / 1000;
					ts.tv_nsec += tWorkInterval % 1000 * 1000000;
					ts.tv_sec += ts.tv_nsec / 1000000000;
					ts.tv_nsec = ts.tv_nsec % 1000000000;
				}
				rval = sem_timedwait(&pThread->m_semThread,&ts);
				tLastTick = ::_TimeGet();
				tLastSecond = ::time(NULL);
				
				if(rval == -1)
				{
					if(errno == ETIMEDOUT)
					{
						try
						{
							if(-1 == pThread->m_event.OnThreadProcess())
							{
								break;
							}
						}
						catch(...)
						{
							printf("CATCH: *** CMyThread:RunThread Call OnThreadProcess crash! *** at %s %d\n",__FILE__,__LINE__);
						}
					}
					else if(errno == EINTR)
					{
						continue;
					}
					else
					{
						printf("sem_timedwait error(%d) ts(%ld %ld)\n",errno,ts.tv_sec,ts.tv_nsec);
						break;
					}
				}
				else
				{
					if(sem_trywait(&pThread->m_semExitEvent) == 0)
					{
						printf("CMyThread catch semExitEvent\n");
						break;
					}
					try
					{
						if(sem_trywait(&pThread->m_semWorkEvent) == 0)
						{
							if(-1 == pThread->m_event.OnThreadWorkEvent())
								break;
						}
					}
					catch(...)
					{
						printf("CATCH: *** CMyThread::RunThread Call OnThreadWorkEvent*** at %s %d \n",__FILE__,__LINE__);
					}
				}
				tCostTime = ::_TimeGet() - tLastTick;
			}
		}
		pThread->m_nStatus = CMyThread::STATUS_CLOSED;
		int rval = pThread->m_event.OnThreadDestroy();
		int nResult = sem_post(&pThread->m_semClosed);
		if(0 == nResult)
		{
			printf("CMyThread post semClosed \n");
		}
		else
		{
			printf("thread post semClose %s \n",strerror(nResult));
		}
		return rval;
	}
	catch(...)
	{
		printf("CATCH: *** CMyThread::RunThread() crash! ***\n");
	}
	return 0;
}


