#ifndef __BASETYPE_H__
#define __BASETYPE_H__
#include<stdio.h>
#include<string>
#include<time.h>
#include<math.h>

#define INFINITE 0xFFFFFFFF
#define TRUE 1
#define FALSE 0
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define LINEEND "\n"

typedef signed char int8;
typedef unsigned char uint8;

typedef signed short int16;
typedef unsigned short uint16;

typedef signed int int32;
typedef unsigned int uint32;

typedef long long int64;
typedef unsigned long long uint64;

typedef unsigned long ulong;
typedef float float32;
typedef double float64;
#define VOID  void
typedef unsigned char UCHAR;
typedef char CHAR;
typedef unsigned int UINT;
typedef int INT;
typedef unsigned short USHORT;
typedef short SHORT;
typedef unsigned long ULONG;
typedef long LONG;
typedef float FLOAT;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int BOOL;
typedef void* HMODULE;
typedef void* HANDLE;



#endif