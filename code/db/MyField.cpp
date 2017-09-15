#include "MyDb.h"
#include "../BaseCode.h"

CMyField::CMyField()
: m_pFieldInfo(NULL),m_bChanged(false),m_bTypeCheck(true),
	m_i64Val(0),m_strVal(NULL)
{
	
}
CMyField::~CMyField()
{
	if( this->CheckType(FIELD_TYPE_STRING,false) ||
		this->CheckType(FIELD_TYPE_VAR_STRING,false) ||
		this->CheckType(FIELD_TYPE_DATETIME,false) ||
		this->CheckType(FIELD_TYPE_TIMESTAMP,false) )
	{
		delete[] m_strVal;
	}
}

void CMyField::CopyTo(char* pszVal, int nLen)
{
	if(!pszVal)
	{
		return ;
	}
	if( this->CheckType(FIELD_TYPE_STRING,false) ||
		this->CheckType(FIELD_TYPE_VAR_STRING,false) ||
		this->CheckType(FIELD_TYPE_DATETIME,false) ||
		this->CheckType(FIELD_TYPE_TIMESTAMP,false) )
	{
		::SafeStrcpy(pszVal,m_strVal,nLen);
	}
}

void CMyField::CopyFrom(const char* pszVal)
{
	if(!pszVal)
		return;
	if( this->CheckType(FIELD_TYPE_STRING,false) ||
		this->CheckType(FIELD_TYPE_VAR_STRING,false) ||
		this->CheckType(FIELD_TYPE_DATETIME,false) ||
		this->CheckType(FIELD_TYPE_TIMESTAMP,false) )
	{
		if(!m_strVal)
		{
			int nSize = strlen(pszVal) + 1;
			m_strVal = new char[nSize];
			strcpy_s(m_strVal,nSize,pszVal);
			m_bChanged = true;
		}
		else
		{
			if(0 == strcmp(m_strVal,pszVal))
			{
				return;
			}
			delete[]m_strVal;
			int nSize = strlen(pszVal) + 1;
			m_strVal = new char[nSize];
			strcpy_s(m_strVal,nSize,pszVal);
			m_bChanged = true;
		}
	}
}

void CMyField::SetValue(const char* pszValue)
{
	if(m_pFieldInfo == NULL)
	{
		return;
	}
	switch(m_pFieldInfo->uiType)
	{
		case FIELD_TYPE_TINY:
		case FIELD_TYPE_SHORT:
		{
			if(!pszValue)
			{
				m_lVal = 0;
			}
			else
			{
				m_lVal = atol(pszValue);
			}
		}
		break;
		case FIELD_TYPE_LONG:
		{
			if(!pszValue)
			{
				m_lVal = 0;
			}
			else
			{
				m_lVal = (long)_atoi64(pszValue);
			}
		}
		break;
		case FIELD_TYPE_LONGLONG:
		{
			if(!pszValue)
			{
				m_i64Val = 0;
			}
			else
			{
				m_i64Val = (long)_atoi64(pszValue);
			}
		}
		break;
		case FIELD_TYPE_FLOAT:
		{
			if(!pszValue)
			{
				m_fVal = 0.0f;
			}
			else
			{
				m_fVal = (float)atof(pszValue);
			}
		}
		break;
		case FIELD_TYPE_DOUBLE:
		{
			if(!pszValue)
			{
				m_dVal = 0;
			}
			else
			{
				m_dVal = atof(pszValue);
			}
		}
		break;
		case FIELD_TYPE_STRING:
		case FIELD_TYPE_VAR_STRING:
		case FIELD_TYPE_DATETIME:
		case FIELD_TYPE_TIMESTAMP:
		{
			if(!pszValue)
			{
				this->CopyFrom("");
			}
			else
			{
				this->CopyFrom(pszValue);
			}
		}
		break;
		default:
		{
			printf("log/db CMyField::SetValue unkonw field[%s] type %u \n",FieldName(),m_pFieldInfo->uiType);
		}
		break;
	}
}

IMyData& CMyField::operator = (const CMyField& field)
{
	if(m_strVal)
	{
		delete[] m_strVal;
		m_strVal = NULL;
	}
	memcpy(this,&field,sizeof(CMyField));
	if(m_strVal)
	{
		m_strVal = NULL;
		this->SetValue(field.m_strVal);
	}
	m_bChanged = true;
	return *this;
}
	
IMyData& CMyField::operator = (char chOp)
{
	if(this->CheckType(FIELD_TYPE_TINY))
	{
		if(chOp != m_cVal)
		{
			m_cVal = chOp;
			m_bChanged = true;
		}
	}
	return *this;
}
IMyData& CMyField::operator = (unsigned char chOp)
{
	if(this->CheckType(FIELD_TYPE_TINY))
	{
		if(chOp != m_ucVal)
		{
			m_ucVal = chOp;
			m_bChanged = true;
		}
	}
	return *this;
}
	
IMyData& CMyField::operator = (short nOp)
{
	if(this->CheckType(FIELD_TYPE_SHORT))
	{
		if(nOp != m_sVal)
		{
			m_sVal = nOp;
			m_bChanged = true;
		}
	}
	return *this;
}
IMyData& CMyField::operator = (unsigned short unOp)
{
	if(this->CheckType(FIELD_TYPE_SHORT))
	{
		if(unOp != m_usVal)
		{
			m_usVal = unOp;
			m_bChanged = true;
		}
	}
	return *this;
}
	
IMyData& CMyField::operator = (int iOp)
{
	if(this->CheckType(FIELD_TYPE_LONG))
	{
		if(iOp != m_lVal)
		{
			m_lVal = iOp;
			m_bChanged = true;
		}
	}
	return *this;
}
IMyData& CMyField::operator = (unsigned int uiOp)
{
	if(this->CheckType(FIELD_TYPE_LONG))
	{
		if(uiOp != m_ulVal)
		{
			m_ulVal = uiOp;
			m_bChanged = true;
		}
	}
	return *this;
}
	
IMyData& CMyField::operator = (long lOp)
{
	if(this->CheckType(FIELD_TYPE_LONG))
	{
		if(lOp != m_lVal)
		{
			m_lVal = lOp;
			m_bChanged = true;
		}
	}
	return *this;
}
IMyData& CMyField::operator = (unsigned long ulOp)
{
	if(this->CheckType(FIELD_TYPE_LONG))
	{
		if(ulOp != m_ulVal)
		{
			m_ulVal = ulOp;
			m_bChanged = true;
		}
	}
	return *this;
}
	
IMyData& CMyField::operator = (int64 i64Op)
{
	if(this->CheckType(FIELD_TYPE_LONGLONG))
	{
		if(i64Op != m_i64Val)
		{
			m_i64Val = i64Op;
			m_bChanged = true;
		}
	}
	return *this;
}
IMyData& CMyField::operator = (uint64 ui64Op)
{
	if(this->CheckType(FIELD_TYPE_LONGLONG))
	{
		if(ui64Op != m_ui64Val)
		{
			m_ui64Val = ui64Op;
			m_bChanged = true;
		}
	}
	return *this;
}
	
IMyData& CMyField::operator = (float fltOp)
{
	if(this->CheckType(FIELD_TYPE_FLOAT))
	{
		if(fltOp != m_fVal)
		{
			m_fVal = fltOp;
			m_bChanged = true;
		}
	}
	return *this;
}
IMyData& CMyField::operator = (double dbOp)
{
	if(this->CheckType(FIELD_TYPE_DOUBLE))
	{
		if(dbOp != m_dVal)
		{
			m_dVal = dbOp;
			m_bChanged = true;
		}
	}
	return *this;
}
	
IMyData& CMyField::operator = (const char* szVal)
{
	this->CopyFrom(szVal);
	return *this;
}