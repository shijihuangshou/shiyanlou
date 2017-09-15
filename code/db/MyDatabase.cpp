#include "MyDb.h"
#include "../BaseCode.h"

const int MAX_TRY_COUNT_ONLOST_CONNECT = 3;

CMyDatabase::CMyDatabase():m_bIsOpen(FALSE),m_hdbc(NULL),m_hdbcAsync(NULL),m_bEnableUseStmt(false),
m_pThreadAsyncSql(NULL),m_bSQLChk(true),m_bAsyncSupport(false)
{
	memset(m_szDBName,0L,sizeof(m_szDBName));
	memset(&m_tMutex,0L,sizeof(m_tMutex));
}

CMyDatabase::~CMyDatabase()
{
	this->Destroy();
}

bool CMyDatabase::Create(const char* pszDBServer,const char* pszLoginName,const char* pszPassword,
	const char* pszDBName,bool bAsyncSupport,bool bSQLChk ,unsigned int uiPort)
{
	char szDBServer[256],szDBName[256],szDBUser[256],szPassword[256];
	
	if(pszDBServer)
	{
		::SafeStrcpy(szDBServer,pszDBServer,256);
	}
	else
	{
		::SafeStrcpy(szDBServer,"localhost",256);
	}
	
	if(pszLoginName)
	{
		::SafeStrcpy(szDBUser,pszLoginName,256);
	}
	else
	{
		::SafeStrcpy(szDBUser,"games",256);
	}
	
	if(pszDBName)
	{
		::SafeStrcpy(szDBName,pszDBName,256);
	}
	else
	{
		::SafeStrcpy(szDBName,"test",256);
	}
	
	if(pszPassword)
	{
		::SafeStrcpy(szPassword,pszPassword,256);
	}
	else
	{
		::SafeStrcpy(szPassword,"test",256);
	}
	::SafeStrcpy(m_szDBName,szDBName,256);
	
	if(!InitLock())
	{
		return false;
	}
	m_hdbc = this->Open(szDBServer,szDBUser,szPassword,szDBName,uiPort);
	if(!m_hdbc)
	{
		printf("log/db Database %s:%s open failed \n",szDBServer,szDBName);
		return false;
	}
	if(bAsyncSupport)
	{
		m_hdbcAsync = this->Open(szDBServer,szDBUser,szPassword,szDBName,uiPort);
		if(!m_hdbc)
		{
			printf("log/db async Database %s:%s open failed \n",szDBServer,szDBName);
			return false;
		}
		m_pThreadAsyncSql = CMyThread::CreateNew(*this,CMyThread::RUN);
		if(!m_pThreadAsyncSql)
		{
			printf("log/db async SQL thread %s:%s open failed \n",szDBServer,szDBName);
			return false;
		}
		m_bAsyncSupport = true;
	}
	m_bIsOpen = true;
	m_bSQLChk = bSQLChk;
	return true;
}

MYSQL* CMyDatabase::Open(const char* szHost,const char* szUser,const char* szPasswd,const char* szDB,
	unsigned int uiPort ,char* szSocket,unsigned long ulFlag)
{
	MYSQL* hdbc = mysql_init(NULL);
	if(hdbc == NULL)
	{
		printf("log/db DB init error.\n");
		return NULL;
	}
	char reconnectArg = TRUE;
	mysql_options(hdbc,MYSQL_OPT_RECONNECT,(char*)&reconnectArg);
	mysql_options(hdbc,MYSQL_SET_CHARSET_NAME,"utf8");
	if(!mysql_real_connect(hdbc,szHost,szUser,szPasswd,szDB,uiPort,szSocket,ulFlag))
	{
		printf("log/db DB real connect error %s,%s,%s \n",szHost,szUser,szDB);
		return NULL;
	}
	return hdbc;
}

void CMyDatabase::Destroy()
{
	m_bIsOpen = false;
	DestroyDefaultRecord();
	
	std::vector<TABLE_INFO*>::const_iterator iter = m_setTableInfo.begin();
	while(iter != m_setTableInfo.end())
	{
		TABLE_INFO* ptr = *iter;
		SAFE_DELETE(ptr);
		iter++;
	}
	m_setTableInfo.clear();
	this->DestroyLock();
	if(m_hdbc != NULL)
	{
		mysql_close(m_hdbc);
		m_hdbc = NULL;
	}
	if(m_pThreadAsyncSql)
	{
		m_pThreadAsyncSql->SetWorkEvent();
		CMySingleLock xLock(m_xLock);
		while(m_setAsyncSQL.size() >0)
		{
			m_xCond.wait_cond(m_xLock);
		}
	}
	SAFE_DELETE(m_pThreadAsyncSql)
	if(m_hdbcAsync != NULL)
	{
		mysql_close(m_hdbcAsync);
		m_hdbcAsync = NULL;
	}
}

CMyRecord* CMyDatabase::CreateDefaultRecord(const char* pszTable)
{
	if(!m_bIsOpen || pszTable == NULL)
	{
		return NULL;
	}
	{
		CMySingleLock xLock(m_xLock);
		int nSize = m_setDefaultRecord.size();
		for(int i = nSize -1; i>=0;i--)
		{
			CMyRecord* pRecord = m_setDefaultRecord[i];
			if(0 == _stricmp(pRecord->GetTableName(),pszTable))
			{
				return pRecord;
			}
		}
	}
	try
	{
		if(!m_hdbc)
		{
			printf("log/db db error not init \n");
			return NULL;
		}
		time_t tStart = ::_TimeGet();
		if(Lock())
		{
			CMyRecord* pRecord = CMyRecord::CreateNew(this);
			if(pRecord == NULL)
			{
				return NULL;
			}
			MYSQL_RES* pRes = NULL;
			for(int nTryCount = 0; nTryCount < MAX_TRY_COUNT_ONLOST_CONNECT;nTryCount++)
			{
				DEBUG_TRY
				if(pRes = mysql_list_fields(m_hdbc,pszTable,NULL))
				{
					break;
				}
				DEBUG_CATCH("mysql_list_fields")
				
				int nSqlErrno = mysql_errno(m_hdbc);
				printf("log/db mysql_list_fields error %u table=%s \n",nSqlErrno,pszTable);
				if(nSqlErrno == 2006 || nSqlErrno == 2013)
				{
					if(mysql_ping(m_hdbc) == 0)
					{
						printf("log/db db mysql_ping ok.\n");
						continue;
					}
					else
					{
						UnLock(::_TimeGet() - tStart);
						printf("log/db db reconnect error.\n");
						return NULL;
					}
				}
				else
				{
					UnLock(::_TimeGet() - tStart);
					return NULL;
				}
			}
			UnLock(::_TimeGet() - tStart);
			
			if(pRes)
			{
				bool bRet = pRecord->OpenDefault(pszTable,pRes);
				mysql_free_result(pRes);
				if(bRet)
				{
					CMySingleLock xLock(m_xLock);
					int nSize = m_setDefaultRecord.size();
					for(int i = nSize -1; i>=0;i++)
					{
						CMyRecord* defaultRecord = m_setDefaultRecord[i];
						if(0 == _stricmp(defaultRecord->GetTableName(),pszTable))
						{
							SAFE_RELEASE(pRecord);
							return defaultRecord;
						}
					}
					m_setDefaultRecord.push_back(pRecord);
					return pRecord;
				}
				else
				{
					SAFE_RELEASE(pRecord);
					return NULL;
				}
			}
			else
			{
				SAFE_RELEASE(pRecord);
				return NULL;
			}
		}
		else
		{
			printf("log/db database warning CreateDefaultRecord(%s) overtime\n",pszTable);
			return NULL;
		}
	}
	catch(...)
	{
		printf("log/db database  CreateDefaultRecord exception error(%s) \n",pszTable);
		return NULL;
	}
}

void CMyDatabase::DestroyDefaultRecord()
{
	int nSize = m_setDefaultRecord.size();
	for(int i = nSize -1;i>=0; i--)
	{
		CMyRecord* pRecord = m_setDefaultRecord[i];
		SAFE_RELEASE(pRecord);
	}
	m_setDefaultRecord.clear();
}
const TABLE_INFO* CMyDatabase::QueryTableInfo(const char* pszTable) const
{
	if(pszTable == NULL)
	{
		return NULL;
	}
	std::vector<TABLE_INFO*>::const_iterator iter = m_setTableInfo.begin();
	while(iter != m_setTableInfo.end())
	{
		const TABLE_INFO* pTableInfo = (*iter);
		if(_stricmp(pTableInfo->strTableName.c_str(),pszTable) == 0)
		{
			return pTableInfo;
		}
		else
		{
			iter++;
		}
	}
	return NULL;
}


void CMyDatabase::AddTableInfo(TABLE_INFO* pInfo)
{
	if(pInfo == NULL)
	{
		return ;
	}
	if(QueryTableInfo(pInfo->strTableName.c_str()))
	{
		return;
	}
	m_setTableInfo.push_back(pInfo);
	
}

//createRecord 和 makerecord有什么不同
IMyRecord* CMyDatabase::CreateRecord(const char* szSQL)
{
	if(this->IsOpen() == false)
	{
		return NULL;
	}
	return CMyRecord::CreateNew(this,szSQL);
}

IMyRecord* CMyDatabase::MakeRecord(const char* szTable,unsigned long id)
{
	if(this->IsOpen() == false)
	{
		return NULL;
	}
	return CMyRecord::MakeNew(this,szTable,id);
}

IMyRecord* CMyDatabase::CreateRecord(const char* szSQL,unsigned long id)
{
	if(this->IsOpen() == false)
	{
		return NULL;
	}
	return CMyRecord::MakeRecord(this,szSQL,id);
}

IMyRecordset* CMyDatabase::CreateRecordset(const char* szSQL)
{
	if(this->IsOpen() == false)
	{
		return NULL;
	}
	if(szSQL == NULL)
	{
		return NULL;
	}
	CMyRecordset* pRet = CMyRecordset::CreateNew(this);
	if(pRet == NULL)
	{
		return NULL;
	}
	if(!pRet->Open(szSQL))
	{
		pRet->Release();
		return NULL;
	}
	return pRet;
}

bool CMyDatabase::InitLock()
{
	int nRet = pthread_mutex_init(&m_tMutex,NULL);
	if(nRet != 0)
	{
		printf("log/db pthread_mutex_init error %d \n",nRet);
		return false;
	}
	return true;
}

bool CMyDatabase::Lock()
{
	struct timespec abs_time;
	clock_gettime(CLOCK_REALTIME,&abs_time);
	abs_time.tv_sec += _MAX_DBACCESSTIME/1000;
	if(0 == pthread_mutex_timedlock(&m_tMutex,&abs_time))
	{
		return true;
	}
	else
	{
		return false;
	}
}
void CMyDatabase::UnLock(long addTime)
{
	__sync_fetch_and_add(&timeDbAccess,addTime);
	pthread_mutex_unlock(&m_tMutex);
}
void CMyDatabase::DestroyLock()
{
	pthread_mutex_destroy(&m_tMutex);
}

unsigned long CMyDatabase::GetInsertId()
{
	if(m_hdbc)
	{
		return (unsigned long)mysql_insert_id(m_hdbc);
	}
	else
	{
		return 0;
	}
}
unsigned int CMyDatabase::GetLastErrorNo()
{
	if(m_hdbc)
	{
		return mysql_errno(m_hdbc);
	}
	else
	{
		return 0;
	}
}
unsigned long CMyDatabase::GetLastAffectedRows()
{
	if(m_hdbc)
	{
		return (unsigned long)mysql_affected_rows(m_hdbc);
	}
	else
	{
		return 0;
	}
}
bool CMyDatabase::EscapeString(char* pszDst,const char* pszSrc,int nLen)
{
	if(pszDst == NULL || pszSrc == NULL)
	{
		return false;
	}
	if(IsOpen() == false)
	{
		return false;
	}
	mysql_real_escape_string(m_hdbc,pszDst,pszSrc,nLen);
	return true;
}

bool CMyDatabase::CheckSQL(const char* pszSQL)
{
	if(!m_bSQLChk)
	{
		return true;
	}
	if(!pszSQL)
	{
		return false;
	}
	char szSQL[_MAX_SQLSIZE] = "";
	::SafeStrcpy(szSQL,pszSQL,_MAX_SQLSIZE);
	_strupr_s(szSQL,_MAX_SQLSIZE);
	
	char szOperation[256] = "";
	strncpy_s(szOperation,256,szSQL,6);
	
	if(0 == strcmp(szOperation,"UPDATE"))
	{
		if(!strstr(szSQL,"WHERE") || !strstr(szSQL,"LIMIT"))
		{
			printf("log/db Invalid update SQL [%s] \n",szSQL);
			return false;
		}
	}
	if(0 == strcmp(szOperation,"DELETE"))
	{
		if(!strstr(szSQL,"WHERE"))
		{
			printf("log/db Invalid delete SQL [%s] \n",szSQL);
			return false;
		}
	}
	return true;
}

bool CMyDatabase::ExecuteSQL(const char* pszSQL,void** pInfo)
{
	if(this->IsOpen() == false)
	{
		return false;
	}
	
	MYSQL_RES** pRes = (MYSQL_RES**)pInfo;
	if(this->CheckSQL(pszSQL) == false)
	{
		return false;
	}
	try
	{
		if(!m_hdbc)
		{
			printf("log/db DB error not init \n");
			return false;
		}
		time_t tStart = ::_TimeGet();
		if(Lock())
		{
			for(int nTryCount = 0; nTryCount < MAX_TRY_COUNT_ONLOST_CONNECT && mysql_query(m_hdbc,pszSQL) != 0;nTryCount++)
			{
				int nSqlErrno = mysql_errno(m_hdbc);
				printf("log/db DB error:[%u]%s when execute:%s.\n",mysql_errno(m_hdbc),mysql_error(m_hdbc),pszSQL);
				if(nSqlErrno == 2006 || nSqlErrno == 2013)
				{
					if(mysql_ping(m_hdbc) == 0)
					{
						printf("log/db db mysql_ping ok.\n");
						continue;
					}
					else
					{
						UnLock(::_TimeGet() - tStart);
						printf("log/db db reconnect error.\n");
						return false;
					}
				}
				else
				{
					UnLock(::_TimeGet() - tStart);
					return false;
				}
			}
			if(pRes)
			{
				*pRes = mysql_store_result(m_hdbc);
				unsigned int nFieldCount = mysql_field_count(m_hdbc);
				UnLock(::_TimeGet()-tStart);
				if(*pRes)
				{
					return true;
				}
				else
				{
					if(nFieldCount > 0)
					{
						printf("log/db DB error:[%u]%s when execute:%s.\n",mysql_errno(m_hdbc),mysql_error(m_hdbc),pszSQL);
						return false;
					}
					else
					{
						return true;
					}
				}
			}
			else
			{
				UnLock(::_TimeGet() - tStart);
				return true;
			}
		}
		else
		{
			printf("log/db *DATABASE* WARNING: ExecuteSQL(%s) overtime \n",pszSQL);
			return false;
		}
	}
	catch(...)
	{
		printf("log/db MyDataBase Exe Sql exception error: %s \n",pszSQL);
		return false;
	}
}

bool CMyDatabase::ExecuteAsyncSQL(const char* pszSQL)
{
	if(!m_bAsyncSupport)
	{
		return false;
	}
	if(this->IsOpen() == false)
	{
		return false;
	}
	if(!(pszSQL && strlen(pszSQL) > 0 && strlen(pszSQL) < _MAX_SQLSIZE))
	{
		if(pszSQL)
		{
			printf("SQL error: %s \n",pszSQL);
		}
		return false;
	}
	if(strlen(pszSQL) >= 6 && (0 == _strnicmp(pszSQL,"select",6)))
	{
		return false;
	}
	CMySingleLock xLock(m_xLock);
	m_setAsyncSQL.push_back(pszSQL);
	
	if(m_pThreadAsyncSql)
	{
		m_pThreadAsyncSql->SetWorkEvent();
	}
	return true;
}

int CMyDatabase::OnThreadWorkEvent()
{
	if(!m_hdbcAsync)
	{
		return 0;
	}
	while(true)
	{
		DEBUG_TRY
		string strSQL;
		{
			CMySingleLock xLock(m_xLock);
			//调用析构函数上的锁
			if(m_setAsyncSQL.empty())
			{
				m_xCond.notify_one();
				break;
			}
			strSQL = m_setAsyncSQL.front();
			m_setAsyncSQL.pop_front();
		}
		if(!strSQL.empty() && this->CheckSQL(strSQL.c_str()))
		{
			for(int nTryCount = 0; nTryCount < MAX_TRY_COUNT_ONLOST_CONNECT && mysql_query(m_hdbcAsync,strSQL.c_str());nTryCount++)
			{
				int nSqlErrno = mysql_errno(m_hdbcAsync);
				printf("log/db DB error:[%u]%s when execute:%s.\n",mysql_errno(m_hdbcAsync),mysql_error(m_hdbcAsync),strSQL.c_str());
				if(nSqlErrno == 2006 || nSqlErrno == 2013)
				{
					if(mysql_ping(m_hdbcAsync) == 0)
					{
						continue;
					}
					else
					{
						printf("log/db db reconnect error.\n");
						break;
					}
				}
			}
		}
		DEBUG_CATCH("CMyDatabase::OnThreadWorkEvent \n");
	}
	return 1;
}





















