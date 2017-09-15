#include "MyDb.h"
#include "../BaseCode.h"
#include "MyParamBinder.h"
#include "../BaseType.h"

CMyRecord::CMyRecord(CMyDatabase* pMyDatabase)
{
	m_bIsOpen = false;
	m_bCanUpdate = false;
	
	m_nMode = modeRead;
	m_uKeyIndex = 0;
	m_uFieldsCount = 0;
	m_pMyDatabase = pMyDatabase;
	m_objFields = NULL;
	m_pTableInfo = NULL;
	
	m_strUpdateCondition = NULL;
}

CMyRecord::~CMyRecord()
{
	if(m_bIsOpen)
		this->Close();
	if(m_strUpdateCondition)
	{
		delete[] m_strUpdateCondition;
		m_strUpdateCondition = NULL;
	}
}

bool CMyRecord::Open(const char* pszSQL)
{
	if(!pszSQL || strlen(pszSQL) < 1)
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
	MYSQL_RES* pRes = NULL;
	if(!m_pMyDatabase->ExecuteSQL(pszSQL,(void**)&pRes))
	{
		return false;
	}
	if(!pRes)
	{
		return false;
	}
	if(pRes->row_count <= 0)
	{
		mysql_free_result(pRes);
		return false;
	}
	
	MYSQL_FIELD* fields = mysql_fetch_fields(pRes);
	if(fields == NULL)
	{
		mysql_free_result(pRes);
		return false;
	}
	
	m_uFieldsCount = mysql_num_fields(pRes);
	if(m_uFieldsCount <= 0)
	{
		mysql_free_result(pRes);
		return false;
	}
	
	m_objFields = new CMyField[m_uFieldsCount];
	if(m_objFields == NULL)
	{
		mysql_free_result(pRes);
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
				mysql_free_result(pRes);
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
					mysql_free_result(pRes);
					return false;
				}
			}
			else
			{
				mysql_free_result(pRes);
				return false;
			}
		}
	}
	else
	{
		strcpy_s(szTable,256,fields[0].table);
	}
	
	if(!m_pMyDatabase->CreateDefaultRecord(szTable))
	{
		mysql_free_result(pRes);
		return false;
	}
	
	const TABLE_INFO* pTableInfo = m_pMyDatabase->QueryTableInfo(szTable);
	if(pTableInfo == NULL)
	{
		mysql_free_result(pRes);
		return false;
	}
	
	m_bCanUpdate = false;
	for(unsigned int i = 0; i < m_uFieldsCount; i++)
	{
		const FIELD_INFO* pFieldInfo = pTableInfo->QueryFieldInfo(fields[i].name);
		if(!pFieldInfo)
		{
			printf("Table [%s] field [%s] not found!\n",szTable,fields[i].name);
			mysql_free_result(pRes);
			return false;
		}
		m_objFields[i].m_pFieldInfo = (FIELD_INFO*)pFieldInfo;
		if(!m_bCanUpdate && pTableInfo->IsKeyField(fields[i].name))
		{
			m_bCanUpdate = true;
			m_uKeyIndex = i;
		}
	}
	m_pTableInfo = (TABLE_INFO*)pTableInfo;
	MYSQL_ROW row = mysql_fetch_row(pRes);
	if(row)
	{
		for(unsigned int i = 0; i < m_uFieldsCount; i++)
		{
			m_objFields[i].SetValue(row[i]);
			m_objFields[i].m_bChanged = false;
		}
		if(m_bCanUpdate)
		{
			char szCondition[256] = "";
			sprintf_s(szCondition,256,"%s=%s",fields[m_uKeyIndex].name,
			row[m_uKeyIndex]);
			this->SetCondition(szCondition);
		}
		
	}
	mysql_free_result(pRes);
	m_nMode = m_bCanUpdate? modeUpdate:modeRead;
	m_bIsOpen = true;
	return true;
}


bool CMyRecord::OpenDefault(const char* pszTable,MYSQL_RES* pRes)
{
	if(!pszTable)
	{
		return false;
	}
	if(!pRes)
	{
		return false;
	}
	if(!m_pMyDatabase || !m_pMyDatabase->IsOpen())
	{
		return false;
	}
	
	m_uFieldsCount = mysql_num_fields(pRes);
	if(m_uFieldsCount <= 0)
	{
		return false;
	}
	
	MYSQL_FIELD* fields = mysql_fetch_fields(pRes);
	if(!fields)
	{
		return false;
	}
	m_objFields = new CMyField[m_uFieldsCount];
	if(!m_objFields)
	{
		return false;
	}
	
	TABLE_INFO* pTableInfo = (TABLE_INFO*)m_pMyDatabase->QueryTableInfo(pszTable);
	if(pTableInfo)
	{
		for(unsigned int i = 0; i < m_uFieldsCount; i++)
		{
			m_objFields[i].m_pFieldInfo = pTableInfo->setFieldInfo[i];
		}
		m_bCanUpdate = pTableInfo->nKeyIndex != -1 ? true:false; //没有主键不能更新？
		m_uKeyIndex = (unsigned)pTableInfo->nKeyIndex;
	}
	else
	{
		pTableInfo = new TABLE_INFO;
		if(pTableInfo == NULL)
		{
			printf("OpenDefault pTableInfo is NULL \n");
			return false;
		}
		pTableInfo->nKeyIndex = -1;
		pTableInfo->strTableName = pszTable;
		
		m_bCanUpdate = false;
		for(unsigned int i = 0; i < m_uFieldsCount; i++)
		{
			FIELD_INFO* pFieldInfo = new FIELD_INFO;
			pFieldInfo->strTableName = fields[i].table;
			pFieldInfo->strFieldName = fields[i].name;
			pFieldInfo->uiType = fields[i].type;
			pFieldInfo->uiAttr = fields[i].flags;
			
			pTableInfo->setFieldInfo.push_back(pFieldInfo);
			m_objFields[i].m_pFieldInfo = pFieldInfo;
			m_objFields[i].SetValue(fields[i].def);
			m_objFields[i].m_bChanged = false;
			
			if(fields[i].flags & PRI_KEY_FLAG)
			{
				m_bCanUpdate = true;
				m_uKeyIndex = i;
				pTableInfo->nKeyIndex = i;
			}
			else
			{
				if(!m_bCanUpdate)
				{
					if(fields[i].flags & UNIQUE_KEY_FLAG ||
					fields[i].flags & AUTO_INCREMENT_FLAG)
					{
						m_bCanUpdate = true;
						m_uKeyIndex = i;
						pTableInfo->nKeyIndex = i;
					}
				}
			}
			
		}
		//原来是for里面的，觉得应该在for外面
		{
				FIELD_INFO* pFieldInfo = new FIELD_INFO;
				pFieldInfo->strTableName = pszTable;
				pFieldInfo->strFieldName = "count(*)";
				pFieldInfo->uiType = FIELD_TYPE_LONG;
				pFieldInfo->uiAttr = 32769;
				
				pTableInfo->setFieldInfo.push_back(pFieldInfo);
		}
		m_pMyDatabase->AddTableInfo(pTableInfo);
	}
	m_pTableInfo = pTableInfo;
	if(m_bCanUpdate)
	{
		this->SetCondition(" ");
		m_bIsOpen = true;
		return true;
	}
	else
	{
		SAFE_DELETE_V(m_objFields);
		m_bIsOpen = false;
		printf("log/db No valid primiry key found in table[%s] \n",pszTable);
		return false;
	}
}


void CMyRecord::Close()
{
	SAFE_DELETE_V(m_objFields);
	m_bIsOpen = false;
	m_bCanUpdate = false;
	m_uKeyIndex = 0;
}

void CMyRecord::SetCondition(const char* pszCondition)
{
	if(pszCondition == NULL)
	{
		return;
	}
	if(m_strUpdateCondition)
	{
		if(0 == strcmp(pszCondition,m_strUpdateCondition))
		{
			return;
		}
	}
	int nSize = strlen(pszCondition) + 1;
	char* str = new char[nSize];
	if(!str)
	{
		return;
	}
	delete[]m_strUpdateCondition;
	m_strUpdateCondition = str;
	strcpy_s(m_strUpdateCondition,nSize,pszCondition);
}

CMyField& CMyRecord::Fields(const char* pszField)
{
	if(pszField)
	{
		for(int i = 0; i < m_uFieldsCount;i++)
		{
			if(0 == _stricmp(pszField,m_objFields[i].FieldName()))
			{
				return m_objFields[i];
			}
		}
		printf("log/db Error field name[%s] of %s in MyRecord::Fields \n",pszField,this->GetTableName());
		return this->Fields((unsigned int)0);
	}
}

CMyField& CMyRecord::Fields(unsigned int uiIndex)
{
	if(uiIndex > m_uFieldsCount)
	{
		printf("log/db Error index[%u] of %s in MyRecord::Fields \n",uiIndex,this->GetTableName());
		uiIndex = 0;
	}
	return m_objFields[uiIndex];
}

void CMyRecord::ClsEditFlag()
{
	for(int i=0; i<m_uFieldsCount; i++)
	{
		m_objFields[i].m_bChanged = false;
	}
}

int CMyRecord::UpdateFieldsCount() const
{
	int count = 0;
	for(int i=0; i<m_uFieldsCount; i++)
	{
		if(m_objFields[i].IsChanged())
		{
			count++;
		}
	}
	return count;
}
	
uint64 CMyRecord::UpdateFieldsMask() const
{
	int count = m_uFieldsCount > 64 ? 64:m_uFieldsCount;
	int nCount = 0;
	for(int i = 0; i < count; i++)
	{
		if(m_objFields[i].IsChanged())
		{
			nCount |= 1L << i;
		}
	}
	return nCount;
}
	
void CMyRecord::BuildUpdateOperation(char* pszOperationSql,int nBuffSize)
{
	if(!pszOperationSql)
	{
		return;
	}
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

unsigned int countof1(uint64 w)
{
	unsigned int w1 = (unsigned int)w;
	unsigned int res = w1 -((w>>1 & 0x55555555));
	res = (res& 0x33333333) + ((res>>2)&0x33333333);
	res = (res+ (res>>4))& 0x0F0F0F0F;
	res = res + (res >> 8);
	unsigned int count1 = (res+ (res>>16)) & 0x000000FF;
	
	unsigned int w2 = w >>32;
	res = w2-((w2>>1 & 0x55555555));
	res = (res& 0x33333333) + ((res>>2)&0x33333333);
	res = (res+ (res>>4))& 0x0F0F0F0F;
	res = res + (res >> 8);
	unsigned int count2 = (res+ (res>>16)) & 0x000000FF;
	return count1 + count2;
}
CMyParamBinder* CMyRecord::BuildSyncStmtUpOperation(uint64 ulUpdateMask)
{
	CMyParamBinder* pRetBinder = NULL;
	if(m_pMyDatabase->Lock())
	{
		TABLE_INFO::Iter_T it = m_pTableInfo->m_tblStmt.begin();
		for(; it != m_pTableInfo->m_tblStmt.end();it++ )
		{
			if((it->first& ulUpdateMask) == ulUpdateMask && countof1(it->first) >= countof1(ulUpdateMask))
			{
				pRetBinder = it->second;
			}
			else if((it->first & ulUpdateMask) == it->first && countof1(it->first) < countof1(ulUpdateMask))
			{
				SAFE_RELEASE(it->second);
				m_pTableInfo->m_tblStmt.erase(it->first);
			}
		}
		m_pMyDatabase->UnLock(0);
		if(pRetBinder)
		{
			return pRetBinder;
		}
	}
	char pszOperationSql[_MAX_SQLSIZE] = "";
	int nBufSize = _MAX_SQLSIZE;
	
	char szFormat[_MAX_SQLSIZE] = "";
	bool bFirst = true;
	
	for(int i = 0;i<m_uFieldsCount;i++)
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
			if((m_objFields[i].FieldType() == FIELD_TYPE_DATETIME || 
				m_objFields[i].FieldType() == FIELD_TYPE_TIMESTAMP) &&
				0 == _stricmp("now()",m_objFields[i].m_strVal))
				{
					sprintf_s(szFormat,sizeof(szFormat),"=%s",m_objFields[i].m_strVal);
				}
				else
				{
					sprintf_s(szFormat,sizeof(szFormat),"=?");
				}
			}
			break;
			
			case FIELD_TYPE_FLOAT:
			case FIELD_TYPE_DOUBLE:
			case FIELD_TYPE_TINY:
			case FIELD_TYPE_SHORT:
			case FIELD_TYPE_LONG:
			case FIELD_TYPE_LONGLONG:
			{
				sprintf_s(szFormat,sizeof(szFormat),"=?");
			}
			break;
			default:
				printf("log/db Error unknow type in CMyRecord BuildSyncStmtUpOperation field[%s] \n",m_objFields[i].FieldName());
			break;
		}
		
		if(!bFirst)
		{
			strcat_s(pszOperationSql,nBufSize,",");
		}
		else
		{
			bFirst = false;
		}
		
		strcat_s(pszOperationSql,nBufSize,m_objFields[i].FieldName());
		strcat_s(pszOperationSql,nBufSize,szFormat);
	}
	if(m_pMyDatabase->Lock())
	{
		MYSQL_STMT* pStmt = mysql_stmt_init(m_pMyDatabase->m_hdbc);
		if(!pStmt)
		{
			m_pMyDatabase->UnLock(0);
			printf("log/db mysql_stmt_init failed %s \n",mysql_stmt_error(pStmt));
			return NULL;
		}
		//不能用来查询么？
		char strSQL[4096];
		switch(m_nMode)
		{
			case modeUpdate:
			{
				sprintf_s(strSQL,4096,"UPDATE %s SET %s WHERE %s=? LIMIT 1",this->GetTableName(),
				pszOperationSql,m_objFields[m_uKeyIndex].FieldName());
			}
			break;
			case modeInsert:
			{
				sprintf_s(strSQL,4096,"INSERT %s SET %s",this->GetTableName(),pszOperationSql);
			}
			break;
			default:
				break;
		}
		if(mysql_stmt_prepare(pStmt,strSQL,strlen(strSQL)))
		{
			m_pMyDatabase->UnLock(0);
			printf("log/db mysql_stmt_prapare failed %s sql %s \n",mysql_stmt_error(pStmt),strSQL);
			return NULL;
		}
		
		size_t uParamCount = mysql_stmt_param_count(pStmt);
		pRetBinder = new CMyParamBinder(pStmt,uParamCount,ulUpdateMask);
		m_pTableInfo->m_tblStmt.insert(make_pair(ulUpdateMask,pRetBinder));
		m_pMyDatabase->UnLock(0);
		return pRetBinder;
	}
	return NULL;
}

bool CMyRecord::Update(bool bAsync)
{
	if(m_pMyDatabase->m_bEnableUseStmt && m_uFieldsCount < MAX_TABLE_COLUMNS && !bAsync && m_nMode == modeUpdate)
	{
		return UpdateSyncByStmt();
	}
	if(!(m_bIsOpen && m_bCanUpdate && m_nMode != modeRead))
	{
		return false;
	}
	int nUpdateFields = this->UpdateFieldsCount();
	if(nUpdateFields <= 0 && modeInsert == m_nMode)
	{
		this->KeyField().m_bChanged = true;
		nUpdateFields = 1;
	}
	bool ret = true;
	if(nUpdateFields > 0)
	{
		char szOperateSQL[_MAX_SQLSIZE] = "";
		this->BuildUpdateOperation(szOperateSQL,_MAX_SQLSIZE);
		this->ClsEditFlag();
		char strSQL[4096];
		switch(m_nMode)
		{
			case modeUpdate:
			{
				//m_strUpdateCondition 是where后面的条件
				sprintf_s(strSQL,4096,SQL_STMT_UPDATE,this->GetTableName(),szOperateSQL,m_strUpdateCondition);
				ret = bAsync? m_pMyDatabase->ExecuteAsyncSQL(strSQL)
				:m_pMyDatabase->ExecuteSyncSQL(strSQL);
			}
			break;
			case modeInsert:
			{
				//m_strUpdateCondition 是where后面的条件
				sprintf_s(strSQL,4096,SQL_STMT_INSERT,this->GetTableName(),szOperateSQL);
				ret = m_pMyDatabase->ExecuteSQL(strSQL,NULL);
				if(ret)
				{
					char szCondition[256]="";
					DWORD key = m_pMyDatabase->GetInsertId();
					if(key != 0)
					{
						sprintf_s(szCondition,256,"%s=%u",m_objFields[m_uKeyIndex].FieldName(),key);
						this->SetCondition(szCondition);
						m_objFields[m_uKeyIndex] = key;
					}
					else if(0 != (DWORD)KeyField())
					{
						sprintf_s(szCondition,256,"%s=%u",m_objFields[m_uKeyIndex].FieldName(),(DWORD)KeyField());
						this->SetCondition(szCondition);
					}
					else
						return false;
					m_nMode = modeUpdate;
				}
			}
			break;
			default:
				break;
		}
	}
	return ret;
}

bool CMyRecord::UpdateSyncByStmt()
{
	if(!(m_bIsOpen && m_bCanUpdate && m_nMode != modeRead) || m_nMode == modeInsert)
	{
		return false;
	}
	uint64 nUpdateFieldsMask = this->UpdateFieldsMask();
	if(nUpdateFieldsMask > 0)
	{
		CMyParamBinder* pBinder = this->BuildSyncStmtUpOperation(nUpdateFieldsMask);
		if(!pBinder)
		{
			return false;
		}
		pBinder->CleanData();
		nUpdateFieldsMask = pBinder->m_ulMask;
		
		unsigned short int i = 0,uIndex = 0;
		for(; i < m_uFieldsCount; i++)
		{
			uint64 bChanged = (nUpdateFieldsMask & (1L<<i));
			switch(m_objFields[i].FieldType())
			{
				case FIELD_TYPE_STRING:
				case FIELD_TYPE_VAR_STRING:
				case FIELD_TYPE_DATETIME:
				case FIELD_TYPE_TIMESTAMP:
				{
					if(i == m_uKeyIndex && m_nMode == modeUpdate)
					{
						size_t nLen = strlen(m_objFields[i].m_strVal);
						pBinder->SetParamText((void*)m_objFields[i].m_strVal,nLen,&nLen,pBinder->GetParamNum()-1);
					}
					if(bChanged)
					{
						if(!m_objFields[i].m_strVal)
						{
							static bool bIsNull = true;
							pBinder->SetParamNull(&bIsNull,uIndex++);
						}
						else
						{
							if(!((m_objFields[i].FieldType() == FIELD_TYPE_DATETIME ||
							m_objFields[i].FieldType() == FIELD_TYPE_TIMESTAMP ) && 
							0 == _stricmp("now()",m_objFields[i].m_strVal)))
							{
								size_t nLen = strlen(m_objFields[i].m_strVal);
								pBinder->SetParamText((void*)m_objFields[i].m_strVal,nLen+1,&nLen,uIndex);
							}
						}
					}
				}
				break;
				case FIELD_TYPE_FLOAT:
				{
					if(i == m_uKeyIndex && m_nMode == modeUpdate)
					{
						pBinder->SetParamFloat(&m_objFields[i].m_fVal,pBinder->GetParamNum()-1);
					}
					if(bChanged)
					{
						pBinder->SetParamFloat(&m_objFields[i].m_fVal,uIndex++);
					}
				}
				break;
				case FIELD_TYPE_DOUBLE:
				{
					if(i == m_uKeyIndex && m_nMode == modeUpdate)
					{
						pBinder->SetParamDouble(&m_objFields[i].m_dVal,pBinder->GetParamNum()-1);
					}
					if(bChanged)
					{
						pBinder->SetParamDouble(&m_objFields[i].m_dVal,uIndex++);
					}
				}
				break;
				case FIELD_TYPE_TINY:
				{
					if((m_objFields[i].FieldAttr() & UNSIGNED_FLAG)!=0)
					{
						if(i == m_uKeyIndex && m_nMode == modeUpdate)
						{
							pBinder->SetParamUint8((uint8*)&m_objFields[i].m_ucVal,pBinder->GetParamNum()-1);
						}
						if(bChanged)
						{
							pBinder->SetParamUint8((uint8*)&m_objFields[i].m_ucVal,uIndex++);
						}
					}
					else
					{
						if(i == m_uKeyIndex && m_nMode == modeUpdate)
						{
							pBinder->SetParamInt8((int8*)&m_objFields[i].m_cVal,pBinder->GetParamNum()-1);
						}
						if(bChanged)
						{
							pBinder->SetParamInt8((int8*)&m_objFields[i].m_cVal,uIndex++);
						}
					}
				}
				break;
				case FIELD_TYPE_SHORT:
				{
					if((m_objFields[i].FieldAttr() & UNSIGNED_FLAG)!=0)
					{
						if(i == m_uKeyIndex && m_nMode == modeUpdate)
						{
							pBinder->SetParamUint16(&m_objFields[i].m_usVal,pBinder->GetParamNum()-1);
						}
						if(bChanged)
						{
							pBinder->SetParamUint16(&m_objFields[i].m_usVal,uIndex++);
						}
					}
					else
					{
						if(i == m_uKeyIndex && m_nMode == modeUpdate)
						{
							pBinder->SetParamInt16(&m_objFields[i].m_sVal,pBinder->GetParamNum()-1);
						}
						if(bChanged)
						{
							pBinder->SetParamInt16(&m_objFields[i].m_sVal,uIndex++);
						}
					}
				}
				break;
				case FIELD_TYPE_LONG:
				{
					if((m_objFields[i].FieldAttr() & UNSIGNED_FLAG)!=0)
					{
						if(i == m_uKeyIndex && m_nMode == modeUpdate)
						{
							pBinder->SetParamUint32((uint32*)&m_objFields[i].m_ulVal,pBinder->GetParamNum()-1);
						}
						if(bChanged)
						{
							pBinder->SetParamUint32((uint32*)&m_objFields[i].m_ulVal,uIndex++);
						}
					}
					else
					{
						if(i == m_uKeyIndex && m_nMode == modeUpdate)
						{
							pBinder->SetParamInt32((int32*)&m_objFields[i].m_lVal,pBinder->GetParamNum()-1);
						}
						if(bChanged)
						{
							pBinder->SetParamInt32((int32*)&m_objFields[i].m_lVal,uIndex++);
						}
					}
				}
				break;
				case FIELD_TYPE_LONGLONG:
				{
					if(i == m_uKeyIndex && m_nMode == modeUpdate)
						{
							pBinder->SetParamInt64(&m_objFields[i].m_i64Val,pBinder->GetParamNum()-1);
						}
						if(bChanged)
						{
							pBinder->SetParamInt64(&m_objFields[i].m_i64Val,uIndex++);
						}
				}
				break;
				default:
				{
					printf("log/db error unkonw type in UpdateSyncByStmt fieldname %s \n",m_objFields[i].FieldName());
					return false;
				}
				break;
			}
		}
		if((m_nMode == modeInsert && uIndex != pBinder->GetParamNum()) ||
		m_nMode == modeUpdate && uIndex != (pBinder->GetParamNum()-1))
		{
			printf("log/db %d:field count error real count: %d experted: %zu(0x%llu)-Total:%d \n",::GetCurrentThreadId(),uIndex,
			pBinder->GetParamNum(),pBinder->m_ulMask,m_uFieldsCount);
			return false;
		}
		
		time_t tStart = ::_TimeGet();
		if(m_pMyDatabase->Lock())
		{
			const int MAX_TRY_COUNT_ONLOST_CONNECT = 3;
			bool bSucc = true;
			for(int nTryCount = 0; nTryCount < MAX_TRY_COUNT_ONLOST_CONNECT; nTryCount++)
			{
				if(! pBinder->Bind())
				{
					printf("log/db mysql_stmt_bind_param fail with error%s \n",mysql_stmt_error(pBinder->GetStatement()));
					bSucc = false;
				}
				if(bSucc && mysql_stmt_execute(pBinder->GetStatement()))
				{
					printf("log/db mysql_stmt_execute fail:%s \n",mysql_stmt_error(pBinder->GetStatement()));
					bSucc = false;
				}
				
				if(!bSucc)
				{
					int nSqlErrno = mysql_errno(m_pMyDatabase->m_hdbc);
					if(nSqlErrno == 2006 || nSqlErrno == 2013)
					{
						if(mysql_ping(m_pMyDatabase->m_hdbc) == 0)
						{
							printf("log/db db mysql_ping ok.\n");
							continue;
						}
						else
						{
							m_pMyDatabase->UnLock(::_TimeGet() - tStart);
							printf("log/db db reconnect error.\n");
							return false;
						}
					}
					else
					{
						m_pMyDatabase->UnLock(::_TimeGet() - tStart);
						return false;
					}
				}
				break;
				this->ClsEditFlag();
				if(m_nMode == modeInsert)
				{
					DWORD key = m_pMyDatabase->GetInsertId();
					if(key != 0)
					{
						m_objFields[m_uKeyIndex] = key;
					}
					m_nMode = modeUpdate;
					m_pMyDatabase->UnLock(::_TimeGet() - tStart);
				}
			}
		}
		else
		{
			printf("log/db DATABASE WARNING ExecuteSQL(0x%llu) overtime\n",nUpdateFieldsMask);
			return false;
		}
	}
	return true;
}
CMyRecord* CMyRecord::Spawn()
{
	if(!(m_pMyDatabase && m_pMyDatabase->IsOpen()))
	{
		return NULL;
	}
	if(!(this->IsOpen()))
	{
		return NULL;
	}
	if(!(m_uFieldsCount > 0))
	{
		return NULL;
	}
	CMyRecord* pNewRecord = new CMyRecord(m_pMyDatabase);
	if(!(pNewRecord))
	{
		return NULL;
	}
	*pNewRecord = *this;
	pNewRecord->m_objFields = new CMyField[m_uFieldsCount];
	if(pNewRecord->m_objFields == NULL)
	{
		return NULL;
	}
	pNewRecord->m_uFieldsCount = m_uFieldsCount;
	pNewRecord->m_uKeyIndex = m_uKeyIndex;
	for(int i = 0; i < m_uFieldsCount; i++)
	{
		pNewRecord->m_objFields[i] = m_objFields[i];
		pNewRecord->m_objFields[i].m_bChanged = false;
	}
	pNewRecord->m_nMode = modeInsert;
	//主键默认递增的
	pNewRecord->m_objFields[m_uKeyIndex] = 0;
	pNewRecord->m_pTableInfo = m_pTableInfo;
	pNewRecord->m_strUpdateCondition = NULL;
	pNewRecord->SetCondition(m_strUpdateCondition);
	return pNewRecord;
}

CMyRecord* CMyRecord::CreateNew(CMyDatabase* pMyDatabase,const char* pszSQL )
{
	if(!(pMyDatabase && pMyDatabase->IsOpen()))
	{
		return NULL;
	}
	CMyRecord* pRecord = new CMyRecord(pMyDatabase);
	if(!pRecord)
	{
		return NULL;
	}
	if(NULL == pszSQL)
	{
		return pRecord;
	}
	else
	{
		char szExeSQL[_MAX_SQLSIZE] = "";
		::SafeStrcpy(szExeSQL,pszSQL,sizeof(szExeSQL));
		if(pRecord->Open(szExeSQL))
		{
			return pRecord;
		}
		else
		{
			SAFE_DELETE(pRecord);
			return NULL;
		}
	}
}
CMyRecord* CMyRecord::MakeNew(CMyDatabase* pMyDatabase, const char* pszTable,DWORD id)
{
	if(!(pMyDatabase || pszTable))
	{
		return NULL;
	}
	CMyRecord* pDefaultRecord = pMyDatabase->CreateDefaultRecord(pszTable);
	if(!pDefaultRecord)
		return NULL;
	CMyRecord* pRecord = pDefaultRecord->Spawn();
	if(!pRecord)
	{
		return NULL;
	}
	pRecord->KeyField() = id;
	return pRecord;
}
CMyRecord* CMyRecord::MakeRecord(CMyDatabase* pMyDatabase,const char* pszTable,DWORD id)
{
	if(!(pMyDatabase || pszTable))
	{
		return NULL;
	}
	CMyRecord* pDefaultRecord = pMyDatabase->CreateDefaultRecord(pszTable);
	if(!pDefaultRecord)
		return NULL;
	CMyRecord* pRecord = pDefaultRecord->Spawn();
	if(!pRecord)
	{
		return NULL;
	}
	pRecord->KeyField() = id;
	char szCondition[256] = "";
	sprintf_s(szCondition,256,"%s=%u",pRecord->KeyField().FieldName(),id);
	pRecord->SetCondition(szCondition);
	pRecord->m_nMode = pRecord->m_bCanUpdate?modeUpdate:modeRead;
	return pRecord;
}

















