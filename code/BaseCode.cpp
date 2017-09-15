#include "BaseCode.h"


void SafeStrcpy(char* pszTarge,const char* pszSource,int nBufSize)
{
	if(!pszTarge || ! pszSource || nBufSize <= 0)
	{
		return;
	}
	size_t nsize = strlen(pszTarge);
	if((size_t)nBufSize <= nsize)
	{
		memcpy_s(pszTarge,nBufSize,pszSource,nBufSize);
		pszTarge[nBufSize-1] = 0;
	}
	else
	{
		strcpy_s(pszTarge,nBufSize,pszSource);
	}
}
int safe_sprintf(char* pBuffer,size_t sizeOfBuffer,const char* fmt,...)
{
	if(!pBuffer)
	{
		return false;
	}
	char szBuff[4096];
	va_list ap;
	va_start(ap,fmt);
	int nRet = vsprintf_s(szBuff,sizeof(szBuff),fmt,ap);
	va_end(ap);
	if(nRet < 0)
	{
		printf("safe_sprintf nRet error");
		return false;
	}
	if(strlen(szBuff) > sizeOfBuffer)
	{
		printf("safe_sprintf szBuff > sizeOfBuffer");
	}
	::SafeStrcpy(pBuffer,szBuff,(int)sizeOfBuffer);
	return nRet;
}

//毫秒返回时间
time_t timeGetTime()
{
	timespec _ts;
//#ifdef _DEBUG
	if(clock_gettime(CLOCK_REALTIME,&_ts) != 0)
//#else
	//if(clock_gettime(CLOCK_MONOTONIC,&_ts) !=0)
//#endif
	{
		return 0;
	}
	time_t _tick = (time_t)(_ts.tv_sec * 1000+ _ts.tv_nsec /1000000);
	return _tick;
}

time_t _TimeGetSecond()
{
	return ::time(NULL);
}

time_t _TimeGet()
{
	return timeGetTime();
}

//timezone 哪里来的？
time_t gmt2local(time_t tNow)
{
	return tNow - timezone;
}

time_t local2gmt(time_t tNow)
{
	return tNow + timezone;
}

#ifdef USE_THREAD_LOCAL_TIMEPTR
MyTLSPtr g_TimerTLSPtr;
MyTLSPtr g_TimerMSTLSPtr

time_t TimeGet()
{
	time_t* pTimer = g_TimerMSTLSPtr.get<time_t>();
	return ((pTimer)?(*pTimer):(timeGetTime()));
}

time_t TimeGetSecond()
{
	time_t* pTimer = g_TimerTLSPtr.get<time_t>();
	return ((pTimer)?(*pTimer):(::time(NULL)));
}

time_t TimeGetSecondLocal()
{
	time_t* pTimer = g_TimerTLSPtr.get<time_t>();
	return gmt2local((pTimer)?(*pTimer):(::time(NULL)));
}
#else
time_t TimeGet()
{
	return _TimeGet();
}
time_t TimeGetSecond()
{
	return ::time(NULL);
}
time_t TimeGetSecondLocal()
{
	return gmt2local(TimeGetSecond());
}
#endif

DWORD TimeGet(TIME_TYPE type)
{
	DWORD dwTime = 0;
	switch(type)
	{
		case TIME_SECOND:
		{
			dwTime = (DWORD)TimeGetSecond();
		}
		break;
		case TIME_SECOND_LOCAL:
		{
			dwTime = (DWORD)TimeGetSecondLocal();
		}
		case TIME_MINUTE:
		{
			time_t tTime = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_year)%100*100000000 +
			(curtime.tm_mon+1)*1000000+
			curtime.tm_mday*10000 + curtime.tm_hour*100+curtime.tm_min;
		}
		break;
		case TIME_DAY:
		{
			time_t tTime = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_year+1900)*10000+
			(curtime.tm_mon+1)*100 + curtime.tm_mday;
		}
		break;
		case TIME_DATE:
		{
			time_t tTime = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_mon+1)*100 + curtime.tm_mday;
		}
		break;
		case TIME_WEEK:
		{
			time_t tTime = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = curtime.tm_wday;
		}
		break;
		case TIME_DAYTIME:
		{
			time_t tTime = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = curtime.tm_hour*10000+curtime.tm_min*100+curtime.tm_sec;
		}
		break;
		case TIME_STAMP:
		{
			time_t tTime = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_mon+1)*100000000 + curtime.tm_mday*1000000+
			curtime.tm_hour*10000 + curtime.tm_min * 100 + curtime.tm_sec;
		}
		break;
		case TIME_DATMIN:
		{
			time_t tTime = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = curtime.tm_hour*100 + curtime.tm_min;
		}
		break;
		case TIME_YEARMONTH:
		{
			time_t tTime = TimeGetSecond();
			//time_t long_time = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_year+1900)*100 + curtime.tm_mon+1;
		}
		break;
		case TIME_MONTH:
		{
			time_t tTime = TimeGetSecond();
			//time_t long_time = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = curtime.tm_mon+1;
		}
		break;
		default:
		{
			dwTime = (DWORD)TimeGet();
		}
		break;
	}
	return dwTime;
}

DWORD TimeGet(time_t tTime,TIME_TYPE type)
{
	DWORD dwTime = 0;
	switch(type)
	{
		case TIME_MINUTE:
		{
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_year)%100*100000000 +
			(curtime.tm_mon+1)*1000000+
			curtime.tm_mday*10000 + curtime.tm_hour*100+curtime.tm_min;
		}
		break;
		case TIME_DAY:
		{
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_year+1900)*10000+
			(curtime.tm_mon+1)*100 + curtime.tm_mday;
		}
		break;
		case TIME_DATE:
		{
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_mon+1)*100 + curtime.tm_mday;
		}
		break;
		case TIME_WEEK:
		{
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = curtime.tm_wday;
		}
		break;
		case TIME_DAYTIME:
		{
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = curtime.tm_hour*10000+curtime.tm_min*100+curtime.tm_sec;
		}
		break;
		case TIME_STAMP:
		{
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_mon+1)*100000000 + curtime.tm_mday*1000000+
			curtime.tm_hour*10000 + curtime.tm_min * 100 + curtime.tm_sec;
		}
		break;
		case TIME_DATMIN:
		{
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = curtime.tm_hour*100 + curtime.tm_min;
		}
		break;
		case TIME_YEARMONTH:
		{
			//time_t long_time = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = (curtime.tm_year+1900)*100 + curtime.tm_mon+1;
		}
		break;
		case TIME_MONTH:
		{
			//time_t long_time = TimeGetSecond();
			struct tm curtime;
			if(0 != localtime_s(&curtime,&tTime))
				break;
			dwTime = curtime.tm_mon+1;
		}
		break;
		case TIME_MILLISECOND:
		case TIME_SECOND:
		{
		}
		break;
	}
	return dwTime;
}

DWORD MakeTime(const char szDateTime[20])
{
	int nYear= 0;
	int nMonth= 0;
	int nDay= 0;
	int nHour= 0;
	int nMin= 0;
	int nSec=0;
	if(6 == sscanf(szDateTime,"%d-%d-%d %d:%d:%d",&nYear,&nMonth,&nDay,&nHour,&nMin,&nSec))
	{
		struct tm time0;
		memset(&time0,sizeof(tm),0);
		time0.tm_year = nYear - 1900;
		time0.tm_mon = nMonth -1;
		time0.tm_mday = nDay;
		time0.tm_hour = nHour;
		time0.tm_min = nMin;
		time0.tm_sec = nSec;
		
		time_t t0 = mktime(&time0);
		return (DWORD)t0;
	}
	return 0;
}

std::string FormatTime(time_t tTime,TIME_FORMAT eFormat)
{
	const char* DEFAULT_TIME_FORMAT[3]={"%04d-%02d-%02d %02d-%02d-%02d",
	"%04d-%02d-%02d %02d-%02d-%02d","%04d-%02d-%02d %02d-%02d"};
	struct tm curtime;
	if(0 != localtime_s(&curtime,&tTime))
		return "";
	char szTime[512];
	switch(eFormat)
	{
		case ETF_AUTO:
		case ETF_FULL:
			safe_sprintf(szTime,sizeof(szTime),DEFAULT_TIME_FORMAT[eFormat],
			curtime.tm_year+1900,curtime.tm_mon+1,curtime.tm_mday,
			curtime.tm_hour,curtime.tm_min,curtime.tm_sec);
			break;
		case ETF_MINUTE:
			safe_sprintf(szTime,sizeof(szTime),DEFAULT_TIME_FORMAT[eFormat],
			curtime.tm_year+1900,curtime.tm_mon+1,curtime.tm_mday,
			curtime.tm_hour,curtime.tm_min);
			break;
		default:
			safe_sprintf(szTime,sizeof(szTime),DEFAULT_TIME_FORMAT[0],
			curtime.tm_year+1900,curtime.tm_mon+1,curtime.tm_mday,
			curtime.tm_hour,curtime.tm_min,curtime.tm_sec);
			break;
	}
	return szTime;
}
std::string FormatLocalTime(time_t tTime,TIME_FORMAT eFormat)
{
	const char* DEFAULT_TIME_FORMAT[3]={"%04d-%02d-%02d %02d-%02d-%02d",
	"%04d-%02d-%02d %02d-%02d-%02d","%04d-%02d-%02d %02d-%02d"};
	struct tm curtime;
	if(0 != gmtime_s(&curtime,&tTime))
		return "";
	char szTime[512];
	switch(eFormat)
	{
		case ETF_AUTO:
		case ETF_FULL:
			safe_sprintf(szTime,sizeof(szTime),DEFAULT_TIME_FORMAT[eFormat],
			curtime.tm_year+1900,curtime.tm_mon+1,curtime.tm_mday,
			curtime.tm_hour,curtime.tm_min,curtime.tm_sec);
			break;
		case ETF_MINUTE:
			safe_sprintf(szTime,sizeof(szTime),DEFAULT_TIME_FORMAT[eFormat],
			curtime.tm_year+1900,curtime.tm_mon+1,curtime.tm_mday,
			curtime.tm_hour,curtime.tm_min);
			break;
		default:
			safe_sprintf(szTime,sizeof(szTime),DEFAULT_TIME_FORMAT[0],
			curtime.tm_year+1900,curtime.tm_mon+1,curtime.tm_mday,
			curtime.tm_hour,curtime.tm_min,curtime.tm_sec);
			break;
	}
	return szTime;
}

bool CheckSameDay(time_t time1,time_t time2)
{
	struct tm tm1;
	if(0 != localtime_s(&tm1,&time1))
		return false;
	struct tm tm2;
	if(0 != localtime_s(&tm2,&time2))
		return false;
	return (tm1.tm_yday == tm2.tm_yday) && (tm1.tm_mon = tm2.tm_mon) && (tm1.tm_year == tm2.tm_year);
}

bool CheckSameWeek(time_t time1,time_t time2,WEEK_BEGIN day)
{
	struct tm tm1;
	if(0 != localtime_s(&tm1,&time1))
		return false;
	tm1.tm_hour = 0;
	tm1.tm_min = 0;
	tm1.tm_sec = 0;
	time1 = mktime(&tm1);
	if(tm1.tm_wday < day)
	{
		int nDay = 7 -day +tm1.tm_wday;
		time1 += 24*3600*nDay;
	}
	else 
	{
		int nDay = tm1.tm_wday - day;
		time1 += 24*3600*nDay;
	}
	struct tm tm2;
	if(0 != localtime_s(&tm2,&time2))
		return false;
	tm2.tm_hour = 0;
	tm2.tm_min = 0;
	tm2.tm_sec = 0;
	time2 = mktime(&tm2);
	if(tm2.tm_wday < day)
	{
		int nDay = 7 -day +tm2.tm_wday;
		time2 -= 24*3600*nDay;
	}
	else 
	{
		int nDay = tm2.tm_wday - day;
		time2 -= 24*3600*nDay;
	}
	return time1 == time2;
}

int DateDiff(time_t time1,time_t time2)
{
	struct tm tm1;
	if(0 != localtime_s(&tm1,&time1))
		return false;
	struct tm tm2;
	if(0 != localtime_s(&tm2,&time2))
		return false;
	int nDay = tm2.tm_yday - tm1.tm_yday;
	int nYear = tm2.tm_year - tm1.tm_year;
	int tempYear = 1900+tm1.tm_yday;
	if(nYear > 0)
	{
		while(nYear >0)
		{
			if(isleap(tempYear))
			{
				nDay += 366;
			}
			else
			{
				nDay += 365;
			}
			nYear--;
			tempYear++;
		}
	}
	else if(nYear < 0)
	{
		while(nYear <0)
		{
			if(isleap(tempYear))
			{
				nDay -= 366;
			}
			else
			{
				nDay -= 365;
			}
			nYear++;
			tempYear--;
		}
	}
	return nDay;
}

int DateDiffLocal(time_t time1,time_t time2)
{
	struct tm tm1;
	if(0 != gmtime_s(&tm1,&time1))
		return false;
	struct tm tm2;
	if(0 != gmtime_s(&tm2,&time2))
		return false;
	int nDay = tm2.tm_yday - tm1.tm_yday;
	int nYear = tm2.tm_year - tm1.tm_year;
	int tempYear = 1900+tm1.tm_yday;
	if(nYear > 0)
	{
		while(nYear >0)
		{
			if(isleap(tempYear))
			{
				nDay += 366;
			}
			else
			{
				nDay += 365;
			}
			nYear--;
			tempYear++;
		}
	}
	else if(nYear < 0)
	{
		while(nYear <0)
		{
			if(isleap(tempYear))
			{
				nDay -= 366;
			}
			else
			{
				nDay -= 365;
			}
			nYear++;
			tempYear--;
		}
	}
	return nDay;
}

int GetDaysSince(time_t tData,bool bAutoNextDay,time_t* ptNow)
{
	struct tm tm1;
	if(0 != gmtime_s(&tm1,&tData))
		return false;
	time_t tNow = (ptNow==NULL)?_TimeGetSecond():*ptNow;
	struct tm tm2;
	if(0 != gmtime_s(&tm2,&tNow))
		return false;
	return GetDaysSince(tm1,bAutoNextDay,tm2);
}
int GetDaysSince(const struct tm& tm1,bool bAutoNextDay,const struct tm& tm2)
{
	int nDay = tm2.tm_yday - tm1.tm_yday;
	int nYear = tm2.tm_year - tm1.tm_year;
	int tempYear = 1900+tm1.tm_yday;
	if(nYear > 0)
	{
		while(nYear >0)
		{
			if(isleap(tempYear))
			{
				nDay += 366;
			}
			else
			{
				nDay += 365;
			}
			nYear--;
			tempYear++;
		}
	}
	else if(nYear < 0)
	{
		while(nYear <0)
		{
			if(isleap(tempYear))
			{
				nDay -= 366;
			}
			else
			{
				nDay -= 365;
			}
			nYear++;
			tempYear--;
		}
	}
	if(bAutoNextDay)
	{
		int nHour = tm2.tm_hour - tm1.tm_hour;
		int nMin = tm2.tm_min - tm1.tm_min;
		int nSec = tm2.tm_sec - tm1.tm_sec;
		if((nHour > 0)||(nHour==0&&nMin>0)||(nHour==0&&nMin==0&&nSec>0))
		{
			nDay++;
		}
	}
	return nDay;
}

MyTLSPtr g_NextDayBeginTimeTLSPtr;
DWORD GetNextDayBeginTime()
{
	time_t* pTimer = g_NextDayBeginTimeTLSPtr.get<time_t>();
	if(pTimer == NULL)
	{
		pTimer = new time_t(0);
		g_NextDayBeginTimeTLSPtr.set(pTimer);
	}
	time_t& tNextDayBeginTime = *pTimer;
	time_t long_time = TimeGetSecond();
	if(long_time < tNextDayBeginTime)
		return (DWORD)tNextDayBeginTime;
	struct tm curtime;
	if(0==localtime_s(&curtime,&long_time))
	{
		curtime.tm_hour = 0;
		curtime.tm_min = 0;
		curtime.tm_sec = 0;
		tNextDayBeginTime = mktime(&curtime)+86400;
		return (DWORD)tNextDayBeginTime;
	}
	return 0;
}
time_t GetDayBeginTime(time_t tTime)
{
	struct tm curtime;
	if(0==localtime_s(&curtime,&tTime))
	{
		curtime.tm_hour = 0;
		curtime.tm_min = 0;
		curtime.tm_sec = 0;
		return mktime(&curtime);
	}
	return 0;
}

bool CheckBetweenTime(tm tmCurTime,DWORD dwCheckType,const char* pszBeginTime,const char* pszEndTime)
{
	if(pszBeginTime == NULL)
	{
		return false;
	}
	if(pszEndTime == NULL)
	{
		return false;
	}
	int nCurYear,nCurMonth,nCurDay,nCurWeekDay,nCurYearDay,nCurHour,nCurMinute;
	
	nCurYear = tmCurTime.tm_year+1900;
	nCurMonth = tmCurTime.tm_mon+1;
	nCurDay = tmCurTime.tm_mday;
	nCurWeekDay = tmCurTime.tm_wday;
	nCurYearDay = tmCurTime.tm_yday;
	nCurHour = tmCurTime.tm_hour;
	nCurMinute = tmCurTime.tm_min;
	
	time_t tCurTime = mktime(&tmCurTime);
	int nYear0=0,nMonth0=0,nDay0=0,nHour0=0,nMinute0=0;
	int nYear1=0,nMonth1=0,nDay1=0,nHour1=0,nMinute1=0;
	
	switch(dwCheckType)
	{
		case CHECKTIMETYPE_EVERY_DAY:
		{
			if(2 == sscanf(pszBeginTime,"%d:%d",&nHour0,&nMinute0) &&
			2 == sscanf(pszEndTime,"%d:%d",&nHour1,&nMinute1) )
			{
				if(nCurHour*60+nCurMinute >= nHour0*60+nMinute0 &&
				nCurHour*60+nCurMinute <= nHour1*60+nMinute1)
				{
					return true;
				}
			}
		}
		break;
		case CHECKTIMETYPE_EVERY_WDAY:
		{
			if(3 == sscanf(pszBeginTime,"%d %d:%d",&nDay0,&nHour0,&nMinute0) &&
			3 == sscanf(pszEndTime,"%d %d:%d",&nDay1,&nHour1,&nMinute1) )
			{
				if(nCurWeekDay*24*60+nCurHour*60+nCurMinute >= nDay0*24*60+nHour0*60+nMinute0 &&
				nCurWeekDay*24*60+nCurHour*60+nCurMinute <= nDay1*24*60+nHour1*60+nMinute1)
				{
					return true;
				}
			}
		}
		break;
		case CHECKTIMETYPE_EVERY_MDAY:
		{
			if(3 == sscanf(pszBeginTime,"%d %d:%d",&nDay0,&nHour0,&nMinute0) &&
			3 == sscanf(pszEndTime,"%d %d:%d",&nDay1,&nHour1,&nMinute1) )
			{
				tm tmTime0,tmTime1;
				memset(&tmTime0,0L,sizeof(tm));
				memset(&tmTime1,0L,sizeof(tm));
				tmTime0.tm_year = nCurYear-1900;
				tmTime0.tm_mon = nCurMonth-1;
				tmTime0.tm_mday = nDay0;
				tmTime0.tm_hour = nHour0;
				tmTime0.tm_min = nMinute0;
				tmTime0.tm_isdst = tmCurTime.tm_isdst;
				
				tmTime1.tm_year = nCurYear-1900;
				tmTime1.tm_mon = nCurMonth-1;
				tmTime1.tm_mday = nDay1;
				tmTime1.tm_hour = nHour1;
				tmTime1.tm_min = nMinute1;
				tmTime1.tm_isdst = tmCurTime.tm_isdst;
				
				time_t t0 = mktime(&tmTime0);
				time_t t1 = mktime(&tmTime1);
				
				if(tCurTime >= t0 && tCurTime <= t1)
					return true;
			}
		}
		break;
		case CHECKTIMETYPE_EVERY_YDAY:
		{
			if(4 == sscanf(pszBeginTime,"%d-%d %d:%d",&nMonth0,&nDay0,&nHour0,&nMinute0) &&
			4 == sscanf(pszEndTime,"%d-%d %d:%d",&nMonth1,&nDay1,&nHour1,&nMinute1) )
			{
				tm tmTime0,tmTime1;
				memset(&tmTime0,0L,sizeof(tm));
				memset(&tmTime1,0L,sizeof(tm));
				tmTime0.tm_year = nCurYear-1900;
				tmTime0.tm_mon = nMonth0-1;
				tmTime0.tm_mday = nDay0;
				tmTime0.tm_hour = nHour0;
				tmTime0.tm_min = nMinute0;
				tmTime0.tm_isdst = tmCurTime.tm_isdst;
				
				tmTime1.tm_year = nCurYear-1900;
				tmTime1.tm_mon = nMonth1-1;
				tmTime1.tm_mday = nDay1;
				tmTime1.tm_hour = nHour1;
				tmTime1.tm_min = nMinute1;
				tmTime1.tm_isdst = tmCurTime.tm_isdst;
				
				time_t t0 = mktime(&tmTime0);
				time_t t1 = mktime(&tmTime1);
				
				if(tCurTime >= t0 && tCurTime <= t1)
					return true;
			}
		}
		break;
		case CHECKTIMETYPE_SOME_DAY:
		{
			if(4 == sscanf(pszBeginTime,"%d-%d-%d %d:%d",&nYear0,&nMonth0,&nDay0,&nHour0,&nMinute0) &&
			4 == sscanf(pszEndTime,"%d-%d-%d %d:%d",&nYear1,&nMonth1,&nDay1,&nHour1,&nMinute1) )
			{
				tm tmTime0,tmTime1;
				memset(&tmTime0,0L,sizeof(tm));
				memset(&tmTime1,0L,sizeof(tm));
				tmTime0.tm_year = nYear0-1900;
				tmTime0.tm_mon = nMonth0-1;
				tmTime0.tm_mday = nDay0;
				tmTime0.tm_hour = nHour0;
				tmTime0.tm_min = nMinute0;
				tmTime0.tm_isdst = tmCurTime.tm_isdst;
				
				tmTime1.tm_year = nYear1-1900;
				tmTime1.tm_mon = nMonth1-1;
				tmTime1.tm_mday = nDay1;
				tmTime1.tm_hour = nHour1;
				tmTime1.tm_min = nMinute1;
				tmTime1.tm_isdst = tmCurTime.tm_isdst;
				
				time_t t0 = mktime(&tmTime0);
				time_t t1 = mktime(&tmTime1);
				
				if(tCurTime >= t0 && tCurTime <= t1)
					return true;
			}
		}
		break;
	}
	return false;
}


bool IsInDayRange(int nMon1,int nDay1,int nMon2,int nDay2)
{
	time_t long_time = TimeGetSecond();
	struct tm curtime;
	if(0 != localtime_s(&curtime,&long_time))
		return false;
	tm tmTime0,tmTime1;
	memset(&tmTime0,0L,sizeof(tm));
	memset(&tmTime1,0L,sizeof(tm));
	tmTime0.tm_year = curtime.tm_year;
	tmTime0.tm_mon = curtime.tm_mon;
	tmTime0.tm_mday = curtime.tm_mday;
	tmTime0.tm_hour = 0;
	tmTime0.tm_min = 0;
	tmTime0.tm_sec = 0;
	tmTime0.tm_isdst = curtime.tm_isdst;

	tmTime1.tm_year = curtime.tm_year;
	tmTime1.tm_mon = curtime.tm_mon;
	tmTime1.tm_mday = curtime.tm_mday;
	tmTime1.tm_hour = 23;
	tmTime1.tm_min = 59;
	tmTime1.tm_sec = 59;
	tmTime1.tm_isdst = curtime.tm_isdst;
	time_t t0 = mktime(&tmTime0);
	time_t t1 = mktime(&tmTime1);
				
	if(long_time >= t0 && long_time <= t1)
		return true;
	
	return false;
}

int GetMDay()
{
	time_t long_time = TimeGetSecond();
	struct tm curtime;
	if(0 != localtime_s(&curtime,&long_time))
	{
		return 0;
	}
	return curtime.tm_mon;
}

char* strupr(char* pszStr)
{
	if(NULL == pszStr)
	{
		errno = EINVAL;
		return NULL;
	}
	char* cp;
	for(cp = pszStr;*cp;cp++)
	{
		if((*cp >= 'a') && (*cp <= 'z'))
		{
			*cp -= 'a'-'A';
		}
	}
	return pszStr;
}
char* strlwr(char* pszStr)
{
		if(NULL == pszStr)
	{
		errno = EINVAL;
		return NULL;
	}
	char* cp;
	for(cp = pszStr;*cp;cp++)
	{
		if((*cp >= 'A') && (*cp <= 'Z'))
		{
			*cp += 'a'-'A';
		}
	}
	return pszStr;
}
void Sleep(DWORD dwMilliseconds)
{
	struct timespec interval;
	interval.tv_sec = dwMilliseconds / 1000;
	interval.tv_nsec = dwMilliseconds % 1000 *  1000000;
	nanosleep(&interval,&interval);
}
pid_t gettid(void)
{
	return syscall(__NR_gettid);
}



