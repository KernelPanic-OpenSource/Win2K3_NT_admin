// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__F0090FB0_76BE_411A_B5FD_CF9FD0E58107__INCLUDED_)
#define AFX_STDAFX_H__F0090FB0_76BE_411A_B5FD_CF9FD0E58107__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include "common.h"

#define REGKEY_SYSTEM_EVENTSOURCE L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"
#define REMOTEASSISTANCE_EVENT_NAME L"Remote Assistance"
#define REMOTEASSISTANCE_EVENT_SOURCE   REGKEY_SYSTEM_EVENTSOURCE L"\\" REMOTEASSISTANCE_EVENT_NAME

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__F0090FB0_76BE_411A_B5FD_CF9FD0E58107__INCLUDED)
