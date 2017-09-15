#include "MyParamBinder.h"
#include "../BaseCode.h"

CDbBind::CDbBind()
{
	is_null = NULL;
	buffer = NULL;
	length = NULL;
}

CDbBind::~CDbBind()
{
	SetBuffer(NULL);
	SetIsNull(NULL);
	SetLength(NULL);
}

void CDbBind::SetBuffer(const void* pBuffer)
{
	buffer = const_cast<void*>(pBuffer);
}

void CDbBind::SetIsNull(bool* pIsNull)
{
	is_null = reinterpret_cast<my_bool*>(pIsNull);
}

void CDbBind::SetLength(size_t* pLength)
{
	length = reinterpret_cast<unsigned long*>(pLength);
}

void CDbBind::SetBuffType(enum_field_types eType)
{
	buffer_type = eType;
}

void CDbBind::SetIsUnsigned(bool bIsUnsigned)
{
	is_unsigned = bIsUnsigned;
}

void CDbBind::SetBuffLength(size_t uSize)
{
	buffer_length = uSize;
}

CMyParamBinder::CMyParamBinder(MYSQL_STMT* pStmt,size_t uParamNum,unsigned long ulMask)
:m_pStmt(pStmt),m_uParamNum(uParamNum),m_ulMask(ulMask),m_bIgnore(false)
{
	m_aryBind = new CDbBind[uParamNum];
}

CMyParamBinder::~CMyParamBinder()
{
	SAFE_DELETE_V(m_aryBind);
	if(m_pStmt)
	{
		mysql_stmt_free_result(m_pStmt);
		mysql_stmt_close(m_pStmt);
		m_pStmt = NULL;
	}
}
void CMyParamBinder::ConfirmParamIndexValid(size_t uIndex)
{
	if(uIndex < m_uParamNum)
		return;
}
void CMyParamBinder::SetParamNull(const bool* pBuffer,size_t uIndex)
{
	ConfirmParamIndexValid(uIndex);
	CDbBind* pbind = m_aryBind + uIndex;
	pbind->SetIsNull(const_cast<bool*>(pBuffer));
	pbind->SetBuffer(NULL);
	pbind->SetBuffType(MYSQL_TYPE_NULL);
	pbind->SetIsUnsigned(false);
}

void CMyParamBinder::SetParamIntegerSeries(const void* pBuffer,size_t uIndex,enum_field_types eBufferType,bool bIsUnsigned)
{
	ConfirmParamIndexValid(uIndex);
	
	CDbBind* pbind = m_aryBind + uIndex;
	pbind->SetIsNull(NULL);
	pbind->SetBuffer(pBuffer);
	pbind->SetBuffType(eBufferType);
	pbind->SetIsUnsigned(bIsUnsigned);
}

void CMyParamBinder::SetParamRealSeries(const void* pBuffer,size_t uIndex,enum_field_types eBufferType)
{
	ConfirmParamIndexValid(uIndex);
	
	CDbBind* pbind = m_aryBind + uIndex;
	pbind->SetIsNull(NULL);
	pbind->SetBuffer(pBuffer);
	pbind->SetBuffType(eBufferType);
}
void CMyParamBinder::SetParamBufferSeries(const void* pBuffer,size_t uMaxSize,size_t* pActualSize,size_t uIndex,enum_field_types eBufferType)
{
	ConfirmParamIndexValid(uIndex);
	
	CDbBind* pbind = m_aryBind + uIndex;
	pbind->SetIsNull(NULL);
	pbind->SetBuffer(pBuffer);
	pbind->SetBuffType(eBufferType);
	pbind->SetBuffLength(uMaxSize);
	pbind->SetLength(pActualSize);
}

void CMyParamBinder::SetParamInt8(const int8* pBuffer,size_t uIndex)
{
	SetParamIntegerSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_TINY,false);
}

void CMyParamBinder::SetParamUint8(const uint8* pBuffer,size_t uIndex)
{
	SetParamIntegerSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_TINY,true);
}

void CMyParamBinder::SetParamInt16(const int16* pBuffer,size_t uIndex)
{
	SetParamIntegerSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_SHORT,false);
}

void CMyParamBinder::SetParamUint16(const uint16* pBuffer,size_t uIndex)
{
	SetParamIntegerSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_SHORT,true);
}

void CMyParamBinder::SetParamInt32(const int32* pBuffer,size_t uIndex)
{
	SetParamIntegerSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_LONG,false);
}

void CMyParamBinder::SetParamUint32(const uint32* pBuffer,size_t uIndex)
{
	SetParamIntegerSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_LONG,true);
}

void CMyParamBinder::SetParamInt64(const int64* pBuffer,size_t uIndex)
{
	SetParamIntegerSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_LONGLONG,false);
}

void CMyParamBinder::SetParamUint64(const uint64* pBuffer,size_t uIndex)
{
	SetParamIntegerSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_LONGLONG,true);
}

void CMyParamBinder::SetParamFloat(const float* pBuffer,size_t uIndex)
{
	SetParamRealSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_FLOAT);
}

void CMyParamBinder::SetParamDouble(const double* pBuffer,size_t uIndex)
{
	SetParamRealSeries(static_cast<const void*>(pBuffer),uIndex,MYSQL_TYPE_DOUBLE);
}

void CMyParamBinder::SetParamText(const void* pBuffer,size_t uMaxSize,size_t*pActualSize,size_t uIndex)
{
	SetParamBufferSeries(static_cast<const void*>(pBuffer),uMaxSize,pActualSize,uIndex,MYSQL_TYPE_STRING);
}

void CMyParamBinder::SetParamBinary(const void* pBuffer,size_t uMaxSize,size_t*pActualSize,size_t uIndex)
{
	SetParamBufferSeries(static_cast<const void*>(pBuffer),uMaxSize,pActualSize,uIndex,MYSQL_TYPE_BLOB);
}

MYSQL_STMT* CMyParamBinder::GetStatement()const
{
	return m_pStmt;
}

bool CMyParamBinder::Bind()
{
	if(mysql_stmt_bind_param(m_pStmt,(MYSQL_BIND*)m_aryBind))
	{
		return false;
	}
	return true;
}

void CMyParamBinder::CleanData()
{
	memset(m_aryBind,0,sizeof(m_aryBind));
}
void CMyParamBinder::Release()
{
	delete this;
}