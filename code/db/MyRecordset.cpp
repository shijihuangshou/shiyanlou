#include "../BaseCode.h"
#include "MyDb.h"

CMyRecordset::CMyRecordset(CMyDatabase* pMyDatabase)
{
	m_bIsOpen = false;
	m_nEditMode = modeRead;
	m_uKeyIndex = 0;
	m_nCursor = 0;
	m_uFieldsCount = 0;
	memset(m_szTableName,0L,sizeof(m_szTableName));
	memset(m_szUpdateCondition,0L,sizeof(m_szUpdateCondition));
	
	m_pMyDatabase = pMyDatabase;
	m_Recordset = NULL;
	m_objFields = NULL;
}

CMyRecordset::~CMyRecordset()
{
	if(m_bIsOpen)
	{
		this->Close();
	}
}

bool CMyRecordset::Open(const char* pszSQL)
{
	if(pszSQL == NULL && strlen(pszSQL) > 0)
	{
		return false;
	}
	
	if(!m_pMyDatabase || !m_pMyDatabase->IsOpen())
	{
		return false;
	}
	
	if(this->IsOpen())
	{
		this->Close();
	}
	if(!m_pMyDatabase->ExecuteSQL(pszSQL,(void**)&m_Recordset))
	{
		return false;
	}
	if(!m_Recordset)
	{
		return false;
	}
	if(m_Recordset->row_count <= 0)
	{
		mysql_free_result(m_Recordset);
		m_Recordset = NULL;
		return false;
	}
	
	MYSQL_FIELD* fields = mysql_fetch_fields(m_Recordset);
	if(!fields)
	{
		mysql_free_result(m_Recordset);
		m_Recordset = NULL;
		return false;
	}
	
	unsigned int uiFieldsCount = mysql_num_fields(m_Recordset);
	m_uFieldsCount = uiFieldsCount;
	if(uiFieldsCount <= 0)
	{
		mysql_free_result(m_Recordset);
		m_Recordset = NULL;
		return false;
	}
	m_objFields = new CMyField[uiFieldsCount];
	if(!m_objFields)
	{
		mysql_free_result(m_Recordset);
		m_Recordset = NULL;
		return false;
	}
	
	char szTable[256] = "";
	if(strlen(fields[0].table) <= 0)
	{
		char* p = (char*)strstr(pszSQL,"from");
		if(p)
		{
			if(!sscanf(p,"from %s",szTable))
			{
				mysql_free_result(m_Recordset);
				return false;
			}
		}
		else
		{
			p = (char*)strstr(pszSQL,"FROM");
			if(p)
			{
				if(!sscanf(p,"FROM %s",szTable))
				{
					mysql_free_result(m_Recordset);
					return false;
				}
			}
			else
			{
				mysql_free_result(m_Recordset);
				return false;
			}
		}
	}
	else
	{
		strcpy_s(szTable,256,fields[0].table);
	}
	strcpy_s(m_szTableName,32,szTable);
	
	const TABLE_INFO* pTableInfo = m_pMyDatabase->QueryTableInfo(szTable);
	if(!pTableInfo)
	{
		mysql_free_result(m_Recordset);
		m_Recordset = NULL;
		return false;
	}
	
	if(!(m_pMyDatabase->CreateDefaultRecord(szTable)))
	{
		mysql_free_result(m_Recordset);
		m_Recordset = NULL;
		return false;
	}
	m_bCanUpdate = false;
	for(unsigned int i = 0; i < m_uFieldsCount; i++)
	{
		const FIELD_INFO* pFieldInfo = pTableInfo->QueryFieldInfo(fields[i].name);
		if(!pFieldInfo)
		{
			printf("Table [%s] field [%s] not found!\n",szTable,fields[i].name);
			mysql_free_result(m_Recordset);
			return false;
		}
		m_objFields[i].m_pFieldInfo = (FIELD_INFO*)pFieldInfo;
		if(!m_bCanUpdate && pTableInfo->IsKeyField(fields[i].name))
		{
			m_bCanUpdate = true;
			m_uKeyIndex = i;
		}
	}
	m_bIsOpen = true;
	this->MoveFirst();
	
	if(m_bCanUpdate)
	{
		this->Edit();
	}
	else
	{
		this->ReadOnly();
	}
	return true;
	
}
void CMyRecordset::Close()
{
	SAFE_DELETE_V(m_objFields);
	if(m_Recordset != NULL)
	{
		mysql_free_result(m_Recordset);
		m_Recordset = NULL;
	}
	m_bIsOpen = false;
	m_bCanUpdate = false;
	m_nEditMode = modeRead;
	m_uKeyIndex = 0;
	m_nCursor = 0;
	m_uFieldsCount = 0;
	
	memset(m_szTableName,0L,sizeof(m_szTableName));
	memset(m_szUpdateCondition,0L,sizeof(m_szUpdateCondition));
}

void CMyRecordset::ReadOnly()
{
	if(!m_bIsOpen) return;
	m_nEditMode = modeRead;
}
void CMyRecordset::AddNew()
{
	if(!m_bIsOpen) return;
	m_nEditMode = modeAddNew;
}
void CMyRecordset::Edit()
{
	if(!m_bIsOpen) return;
	m_nEditMode = modeEdit;
}
void CMyRecordset::Delete()
{
	if(!m_bIsOpen) return;
	m_nEditMode = modeDelete;
}
void CMyRecordset::MoveFirst()
{
	this->Move(0);
}
void CMyRecordset::MoveLast()
{
	int nRowsCount = this->RecordCount();
	this->Move(nRowsCount-1);
}
void CMyRecordset::MovePrev()
{
	this->Move(m_nCursor-1);
}
void CMyRecordset::MoveNext()
{
	this->Move(m_nCursor+1);
}
void CMyRecordset::Move(int index)
{
	if(!m_bIsOpen || !m_Recordset)
	{
		return;
	}
	if(index < 0 || index >= mysql_num_rows(m_Recordset))
	{
		return;
	}
	m_nCursor = index;
	mysql_data_seek(m_Recordset,index);
	MYSQL_ROW row = mysql_fetch_row(m_Recordset);
	if(row)
	{
		int nFields = this->FieldCount();
		for(int i = 0; i < nFields; i++)
		{
			m_objFields[i].SetValue(row[i]);
			m_objFields[i].m_bChanged = false;
		}
		if(m_bCanUpdate)
		{
			sprintf_s(m_szUpdateCondition,128,"%s=%s",m_objFields[m_uKeyIndex].FieldName(),row[m_uKeyIndex]);
		}
	}
}
int CMyRecordset::FieldCount() const
{
	if(!m_bIsOpen || !m_Recordset)
	{
		return 0;
	}
	else
	{
		return m_uFieldsCount;
	}
}

//没啥用的接口
bool CMyRecordset::CanUpdate() const
{
	if(!m_bIsOpen || !m_Recordset)
	{
		return false;
	}
	else
	{
		return m_bCanUpdate;
	}
}
CMyField& CMyRecordset::Fields(unsigned int uiIndex)
{
	if(uiIndex >=(unsigned int)this->FieldCount())
	{
		printf("log/db error index[%u]of %s in CMyRecordset \n",uiIndex,m_szTableName);
		uiIndex = 0;
	}
	return m_objFields[uiIndex];
}
CMyField& CMyRecordset::Fields(const char* pszField)
{
	if(pszField)
	{
		int fieldCount = this->FieldCount();
		for(int i =0; i<fieldCount; i++)
		{
			if(_stricmp(pszField,m_objFields[i].FieldName())==0)
			{
				return this->Fields(i);
			}
		}
		printf("log/db error fieldname[%s]of %s in CMyRecordset \n",pszField,m_szTableName);
	}
	return Fields((unsigned int)0);
}
int CMyRecordset::UpdateFieldsCount() const
{
	int nCount = 0;
	int nFieldsCount = this->FieldCount();
	for(int i=0; i<nFieldsCount; i++)
	{
		if(m_objFields[i].IsChanged())
		{
			nCount++;
		}
	}
	return nCount;
}
void CMyRecordset::ClsEditFlag()
{
	int nFieldsCount = this->FieldCount();
	for(int i=0; i<nFieldsCount; i++)
	{
		m_objFields[i].m_bChanged = false;
	}
}
int CMyRecordset::RecordCount() const
{
	if(!m_bIsOpen || !m_Recordset)
	{
		return 0;
	}
	else
	{
		return (int)mysql_num_rows(m_Recordset);
	}
}

void CMyRecordset::BuildUpdateOperation(char* pszOperationSql,int nBufSize)
{
	if(!pszOperationSql)
		return;
	pszOperationSql[0] = '\0';
	char szFormat[_MAX_SQLSIZE] = "";
	bool bFirst = true;
	bool bFlag = true;
	
	for(int i =0; i<m_uFieldsCount;i++)
	{
		if(!m_objFields[i].IsChanged())
		{
			continue;
		}
		switch(m_objFields[i].FieldType())
		{
			case FIELD_TYPE_STRING:
			case FIELD_TYPE_VAR_STRING:
			case FIELD_TYPE_DATETIME:
			case FIELD_TYPE_TIMESTAMP:
			{
				if(!m_objFields[i].m_strVal)
				{
					bFlag = false;
				}
				else
				{
					if((m_objFields[i].FieldType() == FIELD_TYPE_DATETIME || 
					m_objFields[i].FieldType() == FIELD_TYPE_TIMESTAMP) &&
					0 == _stricmp("now()",m_objFields[i].m_strVal))
					{
						sprintf_s(szFormat,sizeof(szFormat),"=%s",m_objFields[i].m_strVal);
					}
					else
					{
						sprintf_s(szFormat,sizeof(szFormat),"='%s'",m_objFields[i].m_strVal);
					}
				}
			}
			break;
			case FIELD_TYPE_FLOAT:
			{
				sprintf_s(szFormat,sizeof(szFormat),"=%.2f",m_objFields[i].m_fVal);
			}
			break;
			case FIELD_TYPE_DOUBLE:
			{
				sprintf_s(szFormat,sizeof(szFormat),"=%.2f",m_objFields[i].m_dVal);
			}
			break;
			case FIELD_TYPE_TINY:
			{
				if((m_objFields[i].FieldAttr() & UNSIGNED_FLAG) != 0)
				{
					sprintf_s(szFormat,sizeof(szFormat),"=%u",m_objFields[i].m_ucVal);
				}
				else
				{
					sprintf_s(szFormat,sizeof(szFormat),"=%d",m_objFields[i].m_cVal);
				}
			}
			break;
			case FIELD_TYPE_SHORT:
			{
				if((m_objFields[i].FieldAttr() & UNSIGNED_FLAG) != 0)
				{
					sprintf_s(szFormat,sizeof(szFormat),"=%u",m_objFields[i].m_usVal);
				}
				else
				{
					sprintf_s(szFormat,sizeof(szFormat),"=%d",m_objFields[i].m_sVal);
				}
			}
			break;
			case FIELD_TYPE_LONG:
			{
				if((m_objFields[i].FieldAttr() & UNSIGNED_FLAG) != 0)
				{
					sprintf_s(szFormat,sizeof(szFormat),"=%lu",m_objFields[i].m_ulVal);
				}
				else
				{
					sprintf_s(szFormat,sizeof(szFormat),"=%ld",m_objFields[i].m_lVal);
				}
			}
			break;
			case FIELD_TYPE_LONGLONG:
			{
				sprintf_s(szFormat,sizeof(szFormat),"=%lld",m_objFields[i].m_i64Val);
			}
			break;
			default:
				printf("log/db Error unknow type in CMyRecord BuildUpdateOperation field[%s]\n",m_objFields[i].FieldName());
			break;
		}
		if(bFlag)
		{
			if(!bFirst)
			{
				strcat_s(pszOperationSql,nBuffSize,",");
			}
			else
			{
				bFirst = false;
			}
			strcat_s(pszOperationSql,nBuffSize,m_objFields[i].FieldName());
			strcat_s(pszOperationSql,nBuffSize,szFormat);
		}
		else
		{
			bFlag = true;
		}
	}
}

bool CMyRecordset::Update(bool bAsync)
{
	if(!(m_bIsOpen && m_bCanUpdate && m_nEditMode != modeRead))
	{
		return false;
	}
	bool ret = true;
	if(this->UpdateFieldsCount() >0)
	{
		char szOperateSQL[_MAX_SQLSIZE] = "";
		memset(szOperateSQL,0L,sizeof(szOperateSQL));
		this->BuildUpdateOperation(szOperateSQL,_MAX_SQLSIZE);
		char strSQL[4096]="";
		memset(strSQL,0L,sizeof(strSQL));
		switch(m_nEditMode)
		{
			case modeDelete:
			{
				sprintf_s(strSQL,4096,"DELETE FROM %s WHERE %s LIMIT 1",
				m_szTableName,m_szUpdateCondition);
			}
			break;
			case modeEdit:
			{
				sprintf_s(strSQL,4096,SQL_STMT_UPDATE,
				m_szTableName,szOperateSQL,m_szUpdateCondition);
			}
			break;
			case modeAddNew:
			{
				sprintf_s(strSQL,4096,SQL_STMT_INSERT,
				m_szTableName,szOperateSQL);
			}
			break;
			default:
				break;
		}
		
		ret = bAsync?m_pMyDatabase->ExecuteAsyncSQL(strSQL):
			m_pMyDatabase->ExecuteSyncSQL(strSQL);
			
			this->ClsEditFlag();
	}
	return ret;
}

CMyRecord* CMyRecordset::CreateNewRecord(int index)
{
	if(index >= this->RecordCount())
	{
		return NULL;
	}
	
	if(index < 0)
	{
		index = m_nCursor;
	}
	if(m_nCursor != index)
	{
		this->Move(index);
	}
	CMyRecord* pRecord = CMyRecord::CreateNew(m_pMyDatabase);
	
	if(!pRecord)
	{
		return NULL;
	}
	pRecord->m_bIsOpen = true;
	pRecord->m_bCanUpdate = m_bCanUpdate;
	pRecord->m_uKeyIndex = m_uKeyIndex;
	pRecord->m_uFieldsCount = this->FieldCount();
	
	pRecord->m_pTableInfo = (TABLE_INFO*)m_pMyDatabase->QueryTableInfo(m_szTableName);
	pRecord->SetCondition(m_szUpdateCondition);
	
	pRecord->m_objFields = new CMyField[m_uFieldsCount];
	
	if(!pRecord->m_objFields)
	{
		delete pRecord;
		return NULL;
	}
	
	for(int i =0; i < m_uFieldsCount;i++)
	{
		pRecord->m_objFields[i] = m_objFields[i];
	}
	pRecord->ClsEditFlag();
	if(pRecord->CanUpdate())
	{
		pRecord->m_nMode = CMyRecord::modeUpdate;
	}
	return pRecord;
}

CMyRecordset* CMyRecordset::CreateNew(CMyDatabase* pMyDatabase)
{
	if(!pMyDatabase || !pMyDatabase->IsOpen())
	{
		return NULL;
	}
	return (new CMyRecordset(pMyDatabase));
}




