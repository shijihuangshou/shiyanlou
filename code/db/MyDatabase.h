#ifndef __MY_DATA_BASE_H__
#define __MY_DATA_BASE_H__

#include "IMyDb.h"
#include "MyParamBinder.h"
#include "../MyThread.h"
#include <list>
#include <string>
#include <vector>
#include <map>

using namespace std;

const DWORD _MAX_DBACCESSTIME = 10000;
const DWORD _MAX_SQLSIZE = 4096;

struct FIELD_INFO
{
	std::string strTableName;
	std::string strFieldName;
	unsigned int uiType;  //数据类型
	unsigned int uiAttr;
};

const int MAX_TABLE_COLUMNS = 64;

struct TABLE_INFO
{
	std::string strTableName;
	int nKeyIndex;
	std::vector<FIELD_INFO*> setFieldInfo;
	
	typedef map<uint64, CMyParamBinder*> TBL_STMTS_T;
	typedef TBL_STMTS_T::iterator Iter_T;
	TBL_STMTS_T m_tblStmt;
	
	TABLE_INFO():nKeyIndex(-1)
	{
	}
	~TABLE_INFO()
	{
		std::vector<FIELD_INFO*>::const_iterator iter = setFieldInfo.begin();
		while(iter != setFieldInfo.end())
		{
			FIELD_INFO* pInfo = (*iter);
			SAFE_DELETE(pInfo);
			iter++;
		}
		setFieldInfo.clear();
		
		Iter_T it = m_tblStmt.begin();
		for(;it != m_tblStmt.end();it++)
		{
			SAFE_RELEASE(it->second);
		}
		m_tblStmt.clear();
	}
	
	const FIELD_INFO* QueryFieldInfo(const char* pszFieldName)const
	{
		if(!pszFieldName)
			return NULL;
		std::vector<FIELD_INFO*>::const_iterator iter = setFieldInfo.begin();
		while(iter != setFieldInfo.end())
		{
			const FIELD_INFO* pInfo = (*iter);
			if(_stricmp(pInfo->strFieldName.c_str(),pszFieldName) ==0)
				return pInfo;
			iter++;
		}
		return NULL;
	}
	BOOL IsKeyField(const char* pszFieldName)const
	{
		if(-1 == nKeyIndex || !pszFieldName)
			return FALSE;
		return (_stricmp(setFieldInfo[nKeyIndex]->strFieldName.c_str(),pszFieldName)==0);
	}
};

class CMyRecord;
class CMyRecordset;
class CMyDatabase: public IMyDatabase,IThreadEvent
{
public:
	CMyDatabase();
	bool Create(const char* pszDBServer,const char* pszLoginName,const char* pszPassword,
	const char* pszDBName,bool bAsyncSupport = false,bool bSQLChk = true,unsigned int uiPort = MYSQL_PORT);
	void Destroy();
	bool ExecuteSQL(const char* pszSQL,void** pInfo=NULL);
	
	bool IsOpen()const {return m_bIsOpen;}
public:
	virtual unsigned long Release(){delete this; return 0;}
	virtual IMyRecord* CreateRecord(const char* szSQL);
	virtual IMyRecord* MakeRecord(const char* szTable,unsigned long id =0);
	virtual IMyRecord* CreateRecord(const char* szTable,unsigned long id);
	
	virtual IMyRecordset* CreateRecordset(const char* szSQL);
	
	virtual bool ExecuteSyncSQL(const char* pszSQL) {return this->ExecuteSQL(pszSQL);}
	virtual bool ExecuteAsyncSQL(const char* pszSQL);
	
	virtual unsigned long GetInsertId();
	virtual unsigned int GetLastErrorNo();
	virtual unsigned long GetLastAffectedRows();
	virtual bool EscapeString(char* pszDst,const char* pszSrc,int nLen);
	
	virtual void EnableUseStmt(){ m_bEnableUseStmt = !m_bEnableUseStmt;}
	
protected:
	virtual ~CMyDatabase();
	MYSQL* Open(const char* szHost,const char* szUser,const char* szPasswd,const char* szDB,
	unsigned int uiPort = MYSQL_PORT,char* szSocket = NULL,unsigned long ulFlag=0);
	bool CheckSQL(const char* pszSQL);
	friend class CMyRecordset;
	friend class CMyRecord;
	
	int OnThreadCreate(CMyThread*){return 0;}
	int OnThreadDestroy() {return 0;}
	int OnThreadWorkEvent();
	int OnThreadProcess() {return 0;}
	
	//加锁用,执行语句，生成默认record加锁,涉及db的执行，
	//以及插入stmt的操作中会有sql语句的执行
	bool InitLock();
	bool Lock();  
	void UnLock(long addTime);
	void DestroyLock();
	
	//默认结构
	CMyRecord* CreateDefaultRecord(const char* pszTable);
	void DestroyDefaultRecord();
	
	//表信息
	const TABLE_INFO* QueryTableInfo(const char* pszTable) const;
	void AddTableInfo(TABLE_INFO* pInfo);
	
protected: //异步调用数据库用？
	bool m_bAsyncSupport;
	MYSQL* m_hdbcAsync;
	CMyThread* m_pThreadAsyncSql;
	CMyCriticalSection m_xLock;  //sql异步队列锁和默认record队列锁？
	CMyCondition m_xCond;  //这个是用来在销毁的时候用来等待异步工作线程处理完
	list<string> m_setAsyncSQL;
	
private:
	bool m_bIsOpen;
	time_t timeDbAccess;
	bool m_bEnableUseStmt; //用来干啥子的
	char m_szDBName[256];
	MYSQL*  m_hdbc;
	pthread_mutex_t m_tMutex;
	CMyCriticalSection m_xLockDefault;
	std::vector<TABLE_INFO*> m_setTableInfo;
	std::vector<CMyRecord*> m_setDefaultRecord;
	bool m_bSQLChk;
};

#endif



