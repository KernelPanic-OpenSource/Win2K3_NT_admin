//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dbg.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "uiutil.h"

/////////////////////////////////////////////////////////////////////
// debug helpers

#if defined(_USE_DSA_TRACE) || defined(_USE_DSA_ASSERT) || defined(_USE_DSA_TIMER)

UINT GetInfoFromIniFile(LPCWSTR lpszSection, LPCWSTR lpszKey, INT nDefault = 0)
{

  static LPCWSTR lpszFile = L"\\system32\\dsadmin.ini";

    WCHAR szFilePath[2*MAX_PATH];
	UINT nLen = ::GetSystemWindowsDirectory(szFilePath, 2*MAX_PATH);
	if (nLen == 0)
		return nDefault;

  wcscat(szFilePath, lpszFile);
  return ::GetPrivateProfileInt(lpszSection, lpszKey, nDefault, szFilePath);
}
#endif


#if defined(_USE_DSA_TRACE)

#ifdef DEBUG_DSA
DWORD g_dwTrace = 0x1;
#else
DWORD g_dwTrace = ::GetInfoFromIniFile(L"Debug", L"Trace");
#endif

void __cdecl DSATrace(LPCTSTR lpszFormat, ...)
{
  if (g_dwTrace == 0)
    return;

	va_list args;
	va_start(args, lpszFormat);

	int nBuf;

  //
  // Might need to deal with some long path names when the OU structure gets really deep.
  // bug #30432
  //
	WCHAR szBuffer[2048];

	nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), lpszFormat, args);

	// was there an error? was the expanded string too long?
	ASSERT(nBuf >= 0);
  ::OutputDebugString(szBuffer);

	va_end(args);
}

#endif // defined(_USE_DSA_TRACE)

#if defined(_USE_DSA_ASSERT)

DWORD g_dwAssert = ::GetInfoFromIniFile(L"Debug", L"Assert");

BOOL DSAAssertFailedLine(LPCSTR lpszFileName, int nLine)
{
  CThemeContextActivator activator;

  if (g_dwAssert == 0)
    return FALSE;

  WCHAR szMessage[_MAX_PATH*2];

	// assume the debugger or auxiliary port
	wsprintf(szMessage, _T("Assertion Failed: File %hs, Line %d\n"),
		lpszFileName, nLine);
	OutputDebugString(szMessage);

	// display the assert
	int nCode = ::MessageBox(NULL, szMessage, _T("Assertion Failed!"),
		MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);

  OutputDebugString(L"after message box\n");
	if (nCode == IDIGNORE)
  {
		return FALSE;   // ignore
  }

	if (nCode == IDRETRY)
  {
		return TRUE;    // will cause DebugBreak
  }

	abort();     // should not return 
	return TRUE;

}
#endif // _USE_DSA_ASSERT

#if defined(_USE_DSA_TIMER)

#ifdef TIMER_DSA
DWORD g_dwTimer = 0x1;
#else
DWORD g_dwTimer = ::GetInfoFromIniFile(L"Debug", L"Timer");
#endif

DWORD StartTicks = ::GetTickCount();
DWORD LastTicks = 0;

void __cdecl DSATimer(LPCTSTR lpszFormat, ...)
{
  if (g_dwTimer == 0)
    return;

	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	WCHAR szBuffer[512], szBuffer2[512];

        DWORD CurrentTicks = GetTickCount() - StartTicks;
        DWORD Interval = CurrentTicks - LastTicks;
        LastTicks = CurrentTicks;
//NTRAID#NTBUG9-571985-2002/03/10-jmessec   buffer overrun potential: how long is lpszFormat?
        nBuf = swprintf(szBuffer2,
                           L"%d, (%d): %ws", CurrentTicks,
							Interval, lpszFormat);
	nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), 
                           szBuffer2, 
                           args);

	// was there an error? was the expanded string too long?
	ASSERT(nBuf >= 0);
  ::OutputDebugString(szBuffer);

	va_end(args);
}
#endif // _USE_DSA_TIMER


