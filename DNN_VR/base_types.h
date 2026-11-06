/*
	Copyright (c) 2002 Cyberon Corp.  All right reserved.
	File: base_types.h
	Desc: Define all base types
	Author: Calvin
	Date: 2013/05/02
	Version : 1.50.00
*/

#ifndef	BASE_TYPES_H__
#define	BASE_TYPES_H__

//#define CEVA_SIMULATE_
//#define DIM_40_

#include <stdlib.h>
//#include <string.h>

// from <stdint.h>. for uintptr_t
// Define _W64 macros to mark types changing their size, like intptr_t.
#if defined(_WIN32)
#ifndef _W64
#  if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
#     define _W64 __w64
#  else
#     define _W64
#  endif
#endif	// _W64

#ifdef _WIN64 // [
   typedef signed __int64    intptr_t;
   typedef unsigned __int64  uintptr_t;	
   typedef unsigned __int64 ULONG_PTR;
#else // _WIN64 ][
   typedef _W64 signed int   intptr_t;
   typedef _W64 unsigned int uintptr_t;
   typedef _W64 unsigned long ULONG_PTR;
#endif // _WIN64 ]

#else	// WIN32

/* Types for `void *' pointers.  */
//#if __WORDSIZE == 64
#if defined(__LP64__) || defined(_LP64)
# ifndef __intptr_t_defined
typedef long int                intptr_t;
#  define __intptr_t_defined
# endif
typedef unsigned long int        uintptr_t;
#else
# ifndef __intptr_t_defined
typedef int                        intptr_t;
#  define __intptr_t_defined
# endif
typedef unsigned int                uintptr_t;
#endif

#endif	// _WIN32


#if defined(_WIN32)

#ifdef CEVA_SIMULATE_
	typedef signed short CHAR;
	typedef unsigned short BYTE;
	typedef signed short INT8;
	typedef unsigned short UINT8;
	typedef signed short SHORT;
	typedef unsigned short WORD;
	typedef unsigned short USHORT;
	typedef signed long LONG;
	typedef unsigned int DWORD;
	typedef unsigned long ULONG;
	typedef signed int INT;
	typedef unsigned int UINT;
	typedef int BOOL;
#ifndef VOID
	#define VOID void
#endif	// VOID
	typedef unsigned short WCHAR;	/*typedef wchar_t WCHAR;*/
	typedef float FLOAT;
#else	// CEVA_SIMULATE_
	// in <windows.h>
	typedef char CHAR;
	typedef unsigned char BYTE;
	typedef signed short SHORT;
	typedef unsigned short WORD;
	typedef signed long LONG;
	typedef unsigned long DWORD;
	typedef unsigned long ULONG;
	typedef signed int INT;
	typedef int BOOL;

	typedef signed char INT8;
	typedef unsigned char UINT8;
	typedef unsigned short USHORT;
	typedef unsigned int UINT;
	typedef unsigned long ULONG;
#ifndef VOID
	#define VOID void	// void is not a type, can't use typedef
#endif
	#define SIZE_T ULONG_PTR	// 	using typedef UINT SIZE_T; will get warning C4142: benign redefinition of type

#endif	// CEVA_SIMULATE_

	
#if (_MSC_VER < 1400)
	typedef unsigned short WCHAR;
#endif	// _MSC_VER

#else	// _WIN32

#ifdef CEVA_SIMULATE_
	typedef signed short CHAR;
	typedef unsigned short BYTE;
	typedef signed short INT8;
	typedef unsigned short UINT8;
#else	// CEVA_SIMULATE_
#ifndef __NVTTYPE_CHAR_DEFINED__ 
#define __NVTTYPE_CHAR_DEFINED__
	typedef signed char CHAR;
#endif
#ifndef __NVTTYPE_BYTE_DEFINED__ 
#define __NVTTYPE_BYTE_DEFINED__
	typedef unsigned char BYTE;
#endif
#ifndef __NVTTYPE_INT8__ 
#define __NVTTYPE_INT8__
	typedef signed char INT8;
#endif
#ifndef __NVTTYPE_UINT8_DEFINED__ 
#define __NVTTYPE_UINT8_DEFINED__
	typedef unsigned char UINT8;
#endif
#endif	// CEVA_SIMULATE_

	typedef signed short SHORT;
	typedef unsigned short WORD;
	typedef unsigned short USHORT;
	typedef signed long LONG;
	typedef unsigned int DWORD;
	typedef unsigned long ULONG;
	typedef signed int INT;
	typedef unsigned int UINT;
//	typedef int BOOL;
#ifndef VOID
	typedef void VOID;
#endif	// VOID
//	typedef unsigned short WCHAR;	/*typedef wchar_t WCHAR;*/
//	typedef float FLOAT;
	#define SIZE_T uintptr_t	// 	using typedef UINT SIZE_T; will get warning C4142: benign redefinition of type

#endif	// _WIN32

#ifdef UNICODE
#undef UNICODE
#endif	// UNICODE

//#define UNICODE WCHAR
#define UNICODE unsigned short

#ifdef LPVOID
#undef LPVOID	// Non-compliant: MISRA Rule 20.5
#endif	// LPVOID
#define LPVOID	void*

#ifndef TRUE
#define TRUE	1
#endif	// TRUE

#ifndef FALSE
#define FALSE	0
#endif	// FALSE

#ifndef HANDLE
#define HANDLE	VOID*
#endif	// HANDLE

#ifndef NULL
#define NULL	((VOID*)0)	//Non-compliant: MISRA Rule 21.1
#endif	// NULL

#define PNULL	((VOID*)0)

#ifndef STATIC
#define STATIC 	static
#endif	// STATIC

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif	// max

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif	// min

#if defined(_WIN32)
	#define EXPAPI WINAPI
#else	// _WIN32
	#define EXPAPI
#endif	// _WIN32

#ifndef SAFE_DELETE
#define SAFE_DELETE(a)	if ( (a) != NULL ) {delete (a); (a) = NULL;}
#endif	// SAFE_DELETE

#ifndef SAFE_FREE
#define SAFE_FREE(a)	if ( (a) != NULL ) {free(a); (a) = NULL;}
#endif	// SAFE_FREE

//#include "DMemory.h"

#ifndef INLINE
#if defined ( _WIN32 )
	#define INLINE		__inline		// inline keyword for WIN32 Compiler
#elif defined ( __CC_ARM )
	#define INLINE		__inline		// inline keyword for ARM Compiler
#elif defined ( __ICCARM__ )
	#define INLINE		inline			// inline keyword for IAR Compiler. Only available in High optimization mode!
#elif defined ( __GNUC__ )
	#define INLINE		inline			// inline keyword for GNU Compiler
#else	// _WIN32, __CC_ARM, __ICCARM__, __GNUC__
	#define INLINE		inline
#endif	// _WIN32, __CC_ARM, __ICCARM__, __GNUC__
#endif	// INLINE

#endif	// BASE_TYPES_H__
