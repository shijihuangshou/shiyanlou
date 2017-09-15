#ifndef MYRECORDSET_H
#define MYRECORDSET_H

#include "IMyDb.h"

class CMyDatabase;
class CMyRecordset:public IMyRecordset
{
public:
	CMyRecordset(CMyDatabase* pMyDatabase);

protected:
	virtual ~CMyRecordset();
	friend class CMyRecord;
	friend class CMyField;
public:
	virtual unsigned long Release() {delete this; return 0;}
	virtual IMyData& Field(int idx){return Fields(idx);}
	virtual IMyData& Field(const char* pszName){return Fields(pszName);}
	virtual IMyData& Key() {return Field(m_uKeyIndex);}
	
	virtual bool Update(bool bAsync);
	virtual void ClsEditFlag();
	virtual int RecordCount() const;
	virtual IMyRecord* CreateRecord(int index){return CreateNewRecord(index);}
	virtual void MoveNext();
	virtual void Move(int index);
public:
	enum{modeRead,modeEdit,modeAddNew,modeDelete};
	int FieldCount() const;
	int UpdateFieldsCount()const;
	
	bool IsOpen()const{return m_bIsOpen;}
	bool CanUpdate() const;
	int GetMode() const{return m_nEditMode;}
	
	CMyField& Fields(const char* pszField);
	CMyField& Fields(unsigned int uiIndex);
public:
	CMyRecord* CreateNewRecord(int index);
	bool Open(const char* pszSQL);
	void Close();
	
	void ReadOnly();
	void AddNew();
	void Edit();
	void Delete();
	void MoveFirst();
	void MoveLast();
	void MovePrev();
	
	static CMyRecordset* CreateNew(CMyDatabase* pMyDatabase);
private:
	CMyDatabase* m_pMyDatabase;
	MYSQL_RES* m_Recordset; //这个只有set会有
	
	int m_nCursor;
	int m_nEditMode;
	unsigned int m_uKeyIndex;
	
	bool m_bIsOpen;
	bool m_bCanUpdate;
	CMyField* m_objFields;
	char m_szTableName[32];
	char m_szUpdateCondition[128];
	unsigned short m_uFieldsCount;
	
	void BuildUpdateOperation(char* pszOperationSQL,int nBufSize);
};

#endif