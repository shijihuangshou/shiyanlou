#include <stdio.h>
#include "BaseCode.h"
#include<iostream>
#include<string.h>
#include "MyThread.h"
#include<unistd.h>

class DoEvent: public IThreadEvent
{
public:
	int OnThreadCreate(CMyThread* thread)
	{
		printf("this thread %d create count is %d \n",count,num);
		return 0;
	}
	int OnThreadDestroy(void)
	{
		printf("this thread %d destroy count is %d \n",count,num);
		return 0;
	}
	int OnThreadWorkEvent(void)
	{
		printf("this thread %d work event count is %d \n",count,num);
		return 0;
	}
	int OnThreadProcess (void)
	{
		printf("this thread %d process count is %d \n",count,num);
		num++;
		return 0;
	}
	DoEvent(int n):count(n),num(0)
	{
		
	}
	virtual ~DoEvent()
	{
		if(thread != NULL)
		{
			delete thread;
		}
	}
	bool Init()
	{
		thread = CMyThread::CreateNew(*this,true,1000);
		if(thread == NULL)
		{
			return false;
		}
		
		return true;
	}
private:
	int count;
	int num;
	CMyThread* thread;
};

//int DoEvent::count = 0;

int main()
{
	DoEvent a(1),b(2),c(3);
	a.Init();
	b.Init();
	c.Init();
	sleep(10);
	return 0;
}