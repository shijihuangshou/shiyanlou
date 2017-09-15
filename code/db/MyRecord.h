#ifndef __MYRECORD_H__
#define __MYRECORD_H__

#include "IMyDb.h"
#include <vector>
#include <string>

using namespace std;

class CMyDatabase;
class CMyParamBinder;
class CMyRecord: public IMyRecord
{
public:
	CMyRecord(CMyDatabase* pMyDatabase);
	
	const TABLE_INFO* GetTableInfo() const{return m_pTableInfo; }
	const char* GetTableName(){if(m_pTableInfo == NULL){return NULL;}else return m_pTableInfo->strTableName.c_str();}
	void SetCondition(const char* pszCondition);
	const char* GetCondition() const {return m_strUpdateCondition;}
	
	CMyField& Fields(const char* pszField);
	CMyField& Fields(unsigned int uiIndex);
	CMyField& KeyField() {return Fields(m_uKeyIndex); };
public:
	bool Open(const char* pszSQL);
	bool OpenDefault(const char* psztable,MYSQL_RES* pRes);
	void Close();
	CMyRecord* Spawn(void);
	
public:
	static CMyRecord* CreateNew(CMyDatabase* pMyDatabase,const char* pszSQL = NULL);
	static CMyRecord* MakeNew(CMyDatabase* pMyDatabase, const char* pszTable,DWORD id=0 );
	static CMyRecord* MakeRecord(CMyDatabase* pMyDatabase,const char* pszTable,DWORD id);

public:
	virtual unsigned long Release(void){delete this; return 0;}
	virtual bool Update(bool bAsync);
	virtual bool UpdateSyncByStmt(); //用stmt更新只用于更新操作？
	virtual void ClsEditFlag();
	
	virtual IMyData& Field(int idx){ return Fields(idx);}
	virtual IMyData& Field(const char* pszName) {return Fields(pszName);}
	virtual IMyData& Key() {return Field(m_uKeyIndex);}
protected:
	friend class CMyRecordset;
	virtual ~CMyRecord();
	
	
private:
	bool IsOpen() const {return m_bIsOpen;}
	bool CanUpdate() const{return(m_bIsOpen && m_bCanUpdate);}
	int UpdateFieldsCount() const;
	uint64 UpdateFieldsMask() const;
	
	void BuildUpdateOperation(char* pszOperationSQL,int nBuffSize);
	CMyParamBinder* BuildSyncStmtUpOperation(uint64 ulUpdateMask);
	
private:
	CMyDatabase* m_pMyDatabase;
	enum{modeRead,modeUpdate,modeInsert} m_nMode;
	
	unsigned int m_uKeyIndex;
	unsigned short m_uFieldsCount;
	
	bool m_bIsOpen;
	bool m_bCanUpdate;
	
	CMyField* m_objFields;
	TABLE_INFO* m_pTableInfo;
	char* m_strUpdateCondition;
};


#endif