#ifndef _BASE_CODE
#define _BASE_CODE

#include "BaseType.h"
#include<sys/time.h>
#include<sys/io.h>
#include<errno.h>
#include<syslog.h>
#include<string.h>
#include<map>
#include<vector>
#include<string>
#include<algorithm>
#include<functional>
#include <unistd.h> 
#include <sys/types.h>
#include<sys/syscall.h>
#include "MyTLS.h"

#include<stdio.h>
#include<stdarg.h>

#define sprintf_s	snprintf
#define vsprintf_s	vsnprintf
#define printf_s	printf
#define fprintf_s	fprintf
#define fopen_s(fp,name,mode) ((*fp=fopen(fp,name,mode))==NULL?-1:0)
#define localtime_s(tm,time)  (localtime_r(time,tm)==NULL?-1:0)
#define gmtime_s(tm,time)	(gmtime_r(time,tm)==NULL?-1:0)
#define ctime_s(buf,size,time) (ctime_r(time,buf) == NULL?-1:0)

char* strupr(char* pszStr);
char* strlwr(char* pszStr);
void Sleep(DWORD dwMilliseconds);
pid_t gettid(void);

#define GetCurrentThreadId gettid
#define _stricmp strcasecmp
#define stricmp strcasecmp
#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define _strdup strdup
#define _atoi64 atoll
#define _strupr strupr
#define _strlwr strlwr

#define memcpy_s(dest,size,src,count) memcpy(dest,src,count)
#define _strdate_s(buf,size) _strdate(buf)
#define strcat_s(dest,size,src) strcat(dest,src)
#define strncat_s(dest,size,src,n) strncat(dest,src,n)
#define strcpy_s(dest,size,src) strcpy(dest,src)
#define strncpy_s(dest,size,src,count) strncpy(dest,src,count)
#define _strupr_s(str,size) _strupr(str)
#define _strlwr_s(str,size) _strlwr(str)
using namespace std;

#ifdef USE_THREAD_LOCAL_TIMEPTR
extern MyTLSPtr g_TimerTLSPtr;
extern MyTLSPtr g_TimerMSTLSPtr
#endif

#undef SAFE_DELETE
#define SAFE_DELETE(ptr) {if(ptr){try{delete ptr;}catch(...){printf("CATCH:*** SAFE_DELETE() crash! *** at %s,%d",__FILE__,__LINE__);}ptr=0;}}
#define SAFE_DELETE_V(ptr) {if(ptr){try{delete[]ptr;}catch(...){printf("CATCH:*** SAFE_DELETE_V() crash! *** at %s,%d",__FILE__,__LINE__);}ptr=0;}}

#undef SAFE_RELEASE
#define SAFE_RELEASE(ptr) {if(ptr){try{ptr->Release();}catch(...){printf("CATCH:*** RELEASE() crash! *** at %s,%d",__FILE__,__LINE__);}ptr=0;}}

#define DEBUG_TRY try{
#define DEBUG_CATCH(x) }catch(std::exception& e){printf("catch:*** %s crash()! ***With exception %s \n",x,e.what());} catch(...){printf("catch:*** %s crash()!\n",x);}
#define DEBUG_CATCH2(x,y) }catch(std::exception& e){printf("catch:*** %s %s crash()! ***With exception %s \n",x,y,e.what());} catch(...){printf("catch:*** %s %s crash()!\n",x,y);}
#define DEBUG_CATCH3(x,y,z) }catch(std::exception& e){printf("catch:*** %s %s %s crash()! ***With exception %s \n",x,y,z,e.what());} catch(...){printf("catch:*** %s %s %s crash()!\n",x,y,z);}



enum TIME_TYPE
{
	TIME_MILLISECOND=0, //系统启动至今的时间 毫秒
	TIME_SECOND,		//1970年至今的秒数
	TIME_MINUTE,		//YYYYMMDD hhmm
	TIME_DAY,			//YYYYMMDD
	TIME_DAYTIME,		//hhmmss
	TIME_STAMP,			//MMDDhhmmss
	TIME_WEEK,			//星期几
	TIME_DATMIN,		//hhmm
	TIME_DATE,			//MMDDhhmmss
	TIME_YEARMONTH,		//YYMM
	TIME_MONTH,			//MM
	TIME_SECOND_LOCAL,  //返回1970年至今经过的秒数
};

//
void SafeStrcpy(char* pszTarge,const char* pszSource,int nBufSize);
int safe_sprintf(char* pBuffer,size_t sizeOfBuffer,const char* fmt,...);

//time_t == DWORD?
//获取毫秒
time_t timeGetTime();

//返回毫秒
time_t _TimeGet();
//返回秒
time_t _TimeGetSecond();

//local和gmt的转换
time_t gmt2local(time_t tNow);
time_t local2gmt(time_t tNow);

//get 毫秒，秒，local秒，弄这么多层是闹那样呀
time_t TimeGet();
time_t TimeGetSecond();
time_t TimeGetSecondLocal();

DWORD TimeGet(TIME_TYPE type);
DWORD TimeGet(time_t tTime,TIME_TYPE type);

//YYYY-MM-DD HH:MM:SS to DWORD
DWORD MakeTime(const char szDateTime[20]);

enum TIME_FORMAT
{
	ETF_AUTO = 0,
	ETF_FULL = 1, // YYYY年MM月DD日hh时mm分ss秒
	ETF_MINUTE = 2, //YYYY年MM月DD日hh时mm分
};
std::string FormatTime(time_t tTime,TIME_FORMAT eFormat);
std::string FormatLocalTime(time_t tTime,TIME_FORMAT eFormat);

bool CheckSameDay(time_t time1,time_t time2);

enum WEEK_BEGIN
{
	BEGIN_SUNDAY = 0,	//星期天
	BEGIN_MONDAY,
	BEGIN_TUESDAY ,
	BEGIN_WEDNESDAY,
	BEGIN_THURSDAY,
	BEGIN_FRIDAY,
	BEGIN_SATURDAY,
};

bool CheckSameWeek(time_t time1,time_t time2,WEEK_BEGIN day = BEGIN_MONDAY);

inline bool isleap(unsigned int year){return (year%4==0 && year%100==0) || year%400==0;}

int DateDiff(time_t time1,time_t time2);
int DateDiffLocal(time_t time1,time_t time2);

int GetDaysSince(time_t tData,bool bAutoNextDay,time_t* ptNow = NULL);
int GetDaysSince(const struct tm& tm1,bool bAutoNextDay,const struct tm& tm2);

DWORD GetNextDayBeginTime();
time_t GetDayBeginTime(time_t tTime);

enum CHECKTIMETYPE
{
	CHECKTIMETYPE_EVERY_DAY = 0,  //每天的时分 格式为hh:mm
	CHECKTIMETYPE_EVERY_WDAY = 1, //每周某天 格式为 w hh:mm
	CHECKTIMETYPE_EVERY_MDAY = 2, //每月某天 格式为 DD hh:mm
	CHECKTIMETYPE_EVERY_YDAY = 3, //每年某天 格式为 MM-DD hh:mm
	CHECKTIMETYPE_SOME_DAY = 4, //具体年日月 格式为 YYYY-MM-DD hh:mm
};

bool CheckBetweenTime(tm tmCurTime,DWORD dwCheckType,const char* pszBeginTime,const char* pszEndTime);

bool isInDayRange(int nMon1,int nDay1,int nMon2,int nDay2);

int GetMDay();

#endif
