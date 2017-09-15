#ifndef __I_MYDB_H__
#define __I_MYDB_H__

#include "../BaseType.h"

class IMyData
{
public:
	virtual unsigned long Release(void) = 0;
	
	virtual operator char() const = 0;
	virtual operator unsigned char() const = 0;
	
	virtual operator short() const = 0;
	virtual operator unsigned short() const = 0;
	
	virtual operator int() const = 0;
	virtual operator unsigned int() const = 0;
	
	virtual operator long() const = 0;
	virtual operator unsigned long() const = 0;
	
	virtual operator int64() const = 0;
	virtual operator uint64() const = 0;
	
	virtual operator float() const = 0;
	virtual operator double() const = 0;
	
	virtual operator const char* () const = 0;
	
	virtual IMyData& operator = (char chOp) = 0;
	virtual IMyData& operator = (unsigned char chOp) = 0;
	
	virtual IMyData& operator = (short chOp) = 0;
	virtual IMyData& operator = (unsigned short chOp) = 0;
	
	virtual IMyData& operator = (int chOp) = 0;
	virtual IMyData& operator = (unsigned int chOp) = 0;
	
	virtual IMyData& operator = (int64 chOp) = 0;
	virtual IMyData& operator = (uint64 chOp) = 0;
	
	virtual IMyData& operator = (long chOp) = 0;
	virtual IMyData& operator = (unsigned long chOp) = 0;
	
	virtual IMyData& operator = (float chOp) = 0;
	virtual IMyData& operator = (double chOp) = 0;
	
	virtual IMyData& operator = (const char* chOp) = 0;
	
};

class IMyRecord
{
public:
	virtual unsigned long Release(void) = 0;
	
	virtual IMyData& Field(int idx) = 0;
	virtual IMyData& Field(const char* pszName) = 0;
	
	//返回key
	virtual IMyData& Key(void)  = 0;
	
	//更新数据库
	virtual bool Update(bool bAsync = false) = 0;
	//清除所有字段修改标志
	virtual void ClsEditFlag(void) = 0;
};

class IMyRecordset: public IMyRecord
{
public:
	//返回一个数据记录接口IMyRecord
	virtual IMyRecord* CreateRecord(int index = -1) = 0;
	virtual int RecordCount(void) const = 0;
	
	virtual void MoveNext(void) = 0;
	//移动到指定索引记录
	virtual void Move(int index) = 0;
};

class IMyDatabase
{
public:
	virtual unsigned long Release(void) = 0;
	
	virtual IMyRecord* MakeRecord(const char* szTable,unsigned long id = 0) = 0;
	
	virtual IMyRecord* CreateRecord(const char* szTable, unsigned long id) = 0;
	
	virtual IMyRecord* CreateRecord(const char* szSQL) = 0;
	
	virtual IMyRecordset* CreateRecordset(const char* szSQL) = 0;
	
	virtual bool ExecuteSyncSQL(const char* szSQL) = 0;
	
	//异步，只支持insert，update delete
	virtual bool ExecuteAsyncSQL(const char* pszSQL) = 0;
	
	virtual unsigned long GetInsertId(void) = 0;
	
	virtual bool EscapeString(char* pszDst, const char* pszSrc,int nLen) = 0;
	
	virtual unsigned int GetLastErrorNo(void) = 0;
	
	virtual unsigned long GetLastAffectedRows(void) = 0;
	
	virtual void EnableUseStmt() = 0;
};

extern "C" IMyDatabase* MyDatabaseCreate(const char* szDBServer,const char* szLoginName,const char* szPassword,
const char* szDBName,bool bAsyncSupport = false,bool bSQLChk = true,unsigned int uiPort = 3306);

#endif
















