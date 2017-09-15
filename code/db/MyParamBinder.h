#ifndef __MYPARAM_BINDER__
#define __MYPARAM_BINDER__

#include "../BaseType.h"
#include <mysql/mysql.h>

class CDbBind:private MYSQL_BIND
{
public:
	CDbBind();
	~CDbBind();
	void SetBuffer(const void* pBuffer);
	void SetIsNull(bool* pIsNull);
	void SetLength(size_t* pLength);
	void SetBuffType(enum_field_types eType);
	void SetIsUnsigned(bool bIsUnsigned);
	void SetBuffLength(size_t uSize);
};

class CMyParamBinder
{
public:
	CMyParamBinder(MYSQL_STMT* pStmt,size_t uParamNum,unsigned long uMask);
	~CMyParamBinder();
	
	void SetParamNull(const bool* pBuffer,size_t uIndex);
	void SetParamInt8(const int8* pBuffer,size_t uIndex);
	void SetParamUint8(const uint8* pBuffer,size_t uIndex);
	void SetParamInt16(const int16* pBuffer,size_t uIndex);
	void SetParamUint16(const uint16* pBuffer,size_t uIndex);
	void SetParamInt32(const int32* pBuffer,size_t uIndex);
	void SetParamUint32(const uint32* pBuffer,size_t uIndex);
	void SetParamInt64(const int64* pBuffer,size_t uIndex);
	void SetParamUint64(const uint64* pBuffer,size_t uIndex);
	void SetParamFloat(const float* pBuffer,size_t uIndex);
	void SetParamDouble(const double* pBuffer,size_t uIndex);
	void SetParamText(const void* pBuffer,size_t uMaxSize,size_t*pActualSize,size_t uIndex);
	void SetParamBinary(const void* pBuffer,size_t uMaxSize,size_t*pActualSize,size_t uIndex);
	
	MYSQL_STMT* GetStatement()const;
	size_t GetParamNum() {return m_uParamNum;}
	bool Bind();
	void CleanData();
	void Release();
	
	MYSQL_STMT* m_pStmt;
	size_t m_uParamNum;
	uint64 m_ulMask;
	bool m_bIgnore;
private:
	CDbBind* m_aryBind;
private:
	
	void SetParamIntegerSeries(const void* pBuffer,size_t uIndex,enum_field_types eBufferType,bool bIsUnsigned);
	void SetParamRealSeries(const void* pBuffer,size_t uIndex,enum_field_types eBufferType);
	void SetParamBufferSeries(const void* pBuffer,size_t uMaxSize,size_t* pActualSize,size_t uIndex,enum_field_types eBufferType);
	void ConfirmParamIndexValid(size_t uIndex);
};

#endif