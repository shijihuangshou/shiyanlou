#include <stdio.h>
#include "BaseCode.h"
#include<iostream>
#include<string.h>

int main()
{
	time_t t1 = _TimeGetSecond();
	time_t t2 = _TimeGet();
	printf("t1: %ld t2 %ld \n",t1,t2);
	std::string tStr = FormatTime(t1,ETF_MINUTE);
	std::string tStrLocal = FormatLocalTime(t1,ETF_MINUTE);
	printf("t1 str: %s \n t1 str local %s \n",tStr.c_str(),tStrLocal.c_str());
	t2 = 1481644800;
	time_t t3 = t2 + 86399;
	time_t t4 = t2 - 1;
	printf("check same day %d,%d,%d \n",CheckSameDay(t2,t3),CheckSameDay(t2,t4),CheckSameDay(t4,t3));
	t3 = 1481385600;
	t4 = 1482076800;
	printf("check same week %d,%d,%d \n",CheckSameWeek(t2,t3),CheckSameWeek(t2,t4),CheckSameWeek(t4,t3));
	printf("get the date diff %d,%d,%d \n",DateDiff(t2,t3),DateDiff(t2,t4),DateDiff(t4,t3));
	
	printf(" min: %d\n day: %d\n daytime: %d\n stamp: %d\n week: %d\n",TimeGet(TIME_MINUTE),TimeGet(TIME_DAY),
	TimeGet(TIME_DAYTIME),TimeGet(TIME_STAMP),TimeGet(TIME_WEEK));
	
	printf(" datemin: %d\n date: %d\n yearmonth: %d\n month: %d\n ",TimeGet(TIME_DATMIN),TimeGet(TIME_DATE),
	TimeGet(TIME_YEARMONTH),TimeGet(TIME_MONTH));
	
	printf("GetDaysSince %d %d\n",GetDaysSince(t3,false),GetDaysSince(t4,false));
	printf("GetDaysSince true %d %d\n",GetDaysSince(t3,true),GetDaysSince(t4,true));
	printf("GetNextDayBeginTime %u \n GetDayBeginTime %ld\n",GetNextDayBeginTime(),GetDayBeginTime(t2));
	return 0;
}