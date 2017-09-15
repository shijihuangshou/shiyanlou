#include <stdio.h>
#include "../BaseCode.h"
#include "MyDb.h"

int main()
{
	CMyDatabase* database = new CMyDatabase();
	if(!database->Create("localhost","root","123456","test",true))
	{
		printf("create database fail\n");
	}
	printf("create database success\n");
	IMyRecord* myRecord = database->CreateRecord("select * from user where id = 1");
	if(myRecord)
	{
		int num = 0;
		int bAsync = 0;
		int bStmt = 0;
		scanf("%d %d %d",&bAsync,&bStmt,&num);
		unsigned int idx = myRecord->Field(0);
		char userKey[256];
		memset(userKey,0,sizeof(userKey));
		::SafeStrcpy(userKey,myRecord->Field(1),256);
		printf("myRecord %u,%s \n",idx,userKey);
		myRecord->Field(2) = num;
		if(bStmt)
		{
			database->EnableUseStmt();
		}
		myRecord->Update((bool)bAsync);
		SAFE_RELEASE(myRecord);
		
		IMyRecord* newRecord = database->MakeRecord("user");
		newRecord->Field(2) = num;
		newRecord->Update();
	}
	database->Destroy();
	return 0;
}