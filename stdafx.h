// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// comment this out to turn off interactive debug messages . . .
#define _RWZ_

#ifdef _DEBUG
#define	__DEBUG_MEMORY_CHECK_UTILITIES__
#endif

#ifndef	__TEST_SOCKETS__
#define	__TEST_SOCKETS__
#endif

#ifndef	__TEST_MODEMS__
#define	__TEST_MODEMS__
#endif

#ifdef __DEBUG_MEMORY_CHECK_UTILITIES__
#ifdef _DEBUG
   #define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_CLIENTBLOCK
#endif // _DEBUG
#endif	//	__DEBUG_MEMORY_CHECK_UTILITIES__

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <shlwapi.h>

#ifdef __DEBUG_MEMORY_CHECK_UTILITIES__
#ifdef _DEBUG
#include <crtdbg.h>
#define new DEBUG_NEW
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif	//	_DEBUG
#endif	//	__DEBUG_MEMORY_CHECK_UTILITIES__

#pragma comment( lib, "Ws2_32.lib" )
#pragma comment( lib, "shlwapi.lib" )

#include <strsafe.h>
#pragma comment( lib, "strsafe.lib" )

// warning C4482: nonstandard extension used: enum 'CProtelHost::ProtelDayOfTheWeek' used in qualified name
#pragma warning( disable : 4482 )

// Report warning 4995 as an error.
#pragma warning( error : 4995 )

// TODO: reference additional headers your program requires here
