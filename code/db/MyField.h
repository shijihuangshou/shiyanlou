#ifndef __MYFIELD_H__
#define __MYFIELD_H__

#include "IMyDb.h"
#include "MyDatabase.h"
#include <string>

using namespace std;

//实际用来绑定stmt的类
class CMyField: public IMyData
{
public:
	CMyField();
	virtual ~CMyField();
	virtual unsigned long Release() {delete this; return 0;}
protected:
	friend class CMyRecord;
	friend class CMyRecordset;
	
	union
	{
		char m_cVal;
		unsigned char m_ucVal;
		short m_sVal;
		unsigned short m_usVal;
		long m_lVal;
		unsigned long m_ulVal;
		int64 m_i64Val;
		uint64 m_ui64Val;
		float m_fVal;
		double m_dVal;
	};
	char* m_strVal;
	FIELD_INFO* m_pFieldInfo;
	bool m_bChanged;
	bool m_bTypeCheck;
	
public:
	const char* FieldName() const
	{
		if(!m_pFieldInfo)
		{
			return NULL;
		}
		return m_pFieldInfo->strFieldName.c_str();
	}
	
	unsigned int FieldAttr() const
	{
		if(!m_pFieldInfo)
		{
			return 0;
		}
		return m_pFieldInfo->uiAttr;
	}
	
	unsigned int FieldType() const
	{
		if(!m_pFieldInfo)
		{
			return 0;
		}
		return m_pFieldInfo->uiType;
	}
	
	bool IsChanged() const
	{
		return m_bChanged;
	}
	
	void SetTypeCheck(bool bCheck)
	{
		m_bTypeCheck = bCheck;
	}
	
	bool CheckType(int type,bool check = true) const
	{
		if(m_pFieldInfo == NULL)
		{
			return false;
		}
		if(!m_bTypeCheck || type == m_pFieldInfo->uiType)
		{
			return true;
		}
		else
		{
			if(check)
			{
				printf("log/db type chk failed at field[%s] of table[%s]\n",
				m_pFieldInfo->strFieldName.c_str(),m_pFieldInfo->strTableName.c_str());
			}
			return false;
		}
	}
	
	void CopyTo(char* pszVal, int nLen);
	void CopyFrom(const char* pszVal);
public:
	void SetValue(const char* pszValue);
	
	virtual operator char() const
	{
		if(this->CheckType(FIELD_TYPE_TINY))
			return m_cVal;
		else return -1;
	}
	
	virtual operator unsigned char() const
	{
		if(this->CheckType(FIELD_TYPE_TINY))
			return m_ucVal;
		else return 0;
	}
	
	virtual operator short() const
	{
		if(this->CheckType(FIELD_TYPE_SHORT))
			return m_sVal;
		else return -1;
	}
	
	virtual operator unsigned short() const
	{
		if(this->CheckType(FIELD_TYPE_SHORT))
			return m_usVal;
		else return 0;
	}
	
	virtual operator int() const
	{
		if(this->CheckType(FIELD_TYPE_LONG))
			return m_lVal;
		else return -1;
	}
	
	virtual operator unsigned int() const
	{
		if(this->CheckType(FIELD_TYPE_LONG))
			return m_ulVal;
		else return 0;
	}
	
	virtual operator long() const
	{
		if(this->CheckType(FIELD_TYPE_LONG))
			return m_lVal;
		else return -1;
	}
	
	virtual operator unsigned long() const
	{
		if(this->CheckType(FIELD_TYPE_LONG))
			return m_ulVal;
		else return 0;
	}
	
	virtual operator int64() const
	{
		if(this->CheckType(FIELD_TYPE_LONGLONG))
			return m_i64Val;
		else return -1;
	}
	
	virtual operator unsigned uint64() const
	{
		if(this->CheckType(FIELD_TYPE_LONGLONG))
			return m_ui64Val;
		else return 0;
	}
	
	virtual operator float() const
	{
		if(this->CheckType(FIELD_TYPE_FLOAT))
			return m_fVal;
		else return 0.0f;
	}
	
	virtual operator double() const
	{
		if(this->CheckType(FIELD_TYPE_DOUBLE))
			return m_dVal;
		else return 0.0;
	}
	
	virtual operator const char* () const
	{
		if( this->CheckType(FIELD_TYPE_STRING,false) ||
		this->CheckType(FIELD_TYPE_VAR_STRING,false) ||
		this->CheckType(FIELD_TYPE_DATETIME,false) ||
		this->CheckType(FIELD_TYPE_TIMESTAMP,false) )
		{
			return m_strVal;
		}
		else
		{
			return NULL;
		}
	}
	
	virtual IMyData& operator = (const CMyField& field);
	
	virtual IMyData& operator = (char chOp);
	virtual IMyData& operator = (unsigned char chOp);
	
	virtual IMyData& operator = (short nOp);
	virtual IMyData& operator = (unsigned short unOp);
	
	virtual IMyData& operator = (int iOp);
	virtual IMyData& operator = (unsigned int uiOp);
	
	virtual IMyData& operator = (long lOp);
	virtual IMyData& operator = (unsigned long ulOp);
	
	virtual IMyData& operator = (int64 i64Op);
	virtual IMyData& operator = (uint64 ui64Op);
	
	virtual IMyData& operator = (float fltOp);
	virtual IMyData& operator = (double dblOp);
	
	virtual IMyData& operator = (const char* szVal);

};
















#endif