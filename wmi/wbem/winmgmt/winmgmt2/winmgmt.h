/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    winmgmt.h

Abstract:

    defines constant useful for the entire project

--*/

#ifndef __WINMGT_H__
#define __WINMGT_H__

#include <cntserv.h>
#include <reg.h>
#include <stdio.h>

#include "resync2.h"
#include "writer.h"

#include <stdio.h>
#include <helper.h>
#include <flexarry.h>

#define SERVICE_NAME TEXT("winmgmt")

#define HOME_REG_PATH TEXT("Software\\Microsoft\\WBEM\\CIMOM")
#define INITIAL_BREAK TEXT("Break")

#define WBEM_REG_ADAP           __TEXT("Software\\Microsoft\\WBEM\\CIMOM\\ADAP")
#define WBEM_REG_REVERSE_KEY   __TEXT("SOFTWARE\\Microsoft\\WBEM\\PROVIDERS\\Performance")
#define WBEM_REG_REVERSE_VALUE __TEXT("Performance Refresh")

#define WBEM_NORESYNCPERF    __TEXT("NoResyncPerf")
#define WBEM_NOSHELL        __TEXT("NoShell")
#define WBEM_WMISETUP        __TEXT("WMISetup")
#define WBEM_ADAPEXTDLL        __TEXT("ADAPExtDll")


#define DO_THROTTLE        __TEXT("ThrottleDrege")

//
// For Fast ShutDown
// 
///////////////////////////////////////

#define SERVICE_SHUTDOWN 0x80000000 

//  prototype for reg code

DWORD RegSetDWORD(HKEY hKey,
                    TCHAR * pName,
                    TCHAR * pValue,
                    DWORD dwValue);

DWORD RegGetDWORD(HKEY hKey,
                    TCHAR * pName,
                    TCHAR * pValue,
                    DWORD * pdwValue);

//
//
//  prototype for the wbemcore!ShutDown function
//
//////////////////////////////////

typedef HRESULT (STDAPICALLTYPE *pfnShutDown)(DWORD,DWORD);

//
// The DeltaDredge Function (implementing the Pre-Delta) will return
//
///////////////////////////////////

#define FULL_DREDGE    2
#define PARTIAL_DREDGE 1
#define NO_DREDGE      0

//
//
/////////////////////////////////////

#define   WMIADAP_DEFAULT_DELAY          240   // 4 minutes
#define   WMIADAP_DEFAULT_DELAY_LODCTR   60    // 1 minute

#define   WMIADAP_DEFAULT_TIMETOFULL     ((DWORD)-1)

#define ADAP_TIMESTAMP_FULL         TEXT("LastFullDredgeTimestamp")
#define ADAP_TIME_TO_FULL             TEXT("TimeToFullDredge")
#define ADAP_TIME_TO_KILL_ADAP   TEXT("TimeToTerminateAdap")

//
// _PROG_RESOURCES
//
// Holds various resource that need to be freed at the end of execution.
//
//////////////////////////////////////////////////////////////////

struct _PROG_RESOURCES
{
    BOOL            m_bOleInitialized;

    IClassFactory*  m_pLoginFactory;
    IClassFactory*  m_pBackupFactory;
    DWORD           m_dwLoginClsFacReg;
    DWORD           m_dwBackupClsFacReg;

    //-----------
    BOOL g_fSetup;
    BOOL g_fDoResync;

    HANDLE hMainMutex;
    BOOL   bShuttingDownWinMgmt;
    BOOL gbCoreLoaded;

    HANDLE ghCoreCanUnload;
    HANDLE ghProviderCanUnload;
    HANDLE ghMofDirChange;

    TCHAR * szHotMofDirectory;

    CMonitorEvents m_Monitor;

    DWORD ServiceStatus;

    CWbemVssWriter* pWbemVssWriter;
    bool bWbemVssWriterSubscribed;
    DWORD dwWaitThreadID;
        

//------------------------------

    void Init();
    BOOL Phase1Build();
    BOOL Phase2Build(HANDLE hTerminate);

    BOOL RegisterLogin();
    BOOL RevokeLogin();
    BOOL RegisterBackup();
    BOOL RevokeBackup();
    
    BOOL Phase1Delete(BOOL bIsSystemShutDown);
    BOOL Phase2Delete(BOOL bIsSystemShutdown);
    BOOL Phase3Delete();
    
};


extern struct _PROG_RESOURCES g_ProgRes;
extern HINSTANCE g_hInstance;

inline
BOOL GLOB_Monitor_IsRegistred()
{
    BOOL bRet;
    g_ProgRes.m_Monitor.Lock();
    bRet = g_ProgRes.m_Monitor.IsRegistred();
    g_ProgRes.m_Monitor.Unlock();    
    return bRet;
};

inline 
CMonitorEvents * GLOB_GetMonitor()
{
    return &g_ProgRes.m_Monitor;
}

//
// Adap might be disallowed because of Setup running
// or because of a registry setting
//
/////////////////////////////////////////////////////
inline 
BOOL GLOB_IsResyncAllowed()
{
    return (g_ProgRes.g_fDoResync && !g_ProgRes.g_fSetup);
}


//
//
//  functions exported from mofdutil.cpp
//
//////////////////////////////////////////////////////////////////

BOOL InitHotMofStuff( IN OUT struct _PROG_RESOURCES * pProgRes);
void LoadMofsInDirectory(const TCHAR *szDirectory);

BOOL CheckSetupSwitch( void );
void SetNoShellADAPSwitch( void );
BOOL CheckNoResyncSwitch( void );

void AddToAutoRecoverList(TCHAR * pFileName);
BOOL IsValidMulti(TCHAR * pMultStr, DWORD dwSize);
BOOL IsStringPresent(TCHAR * pTest, TCHAR * pMultStr);

//
//
// yet an other class factory
//
///////////////////////////////////////////////////////////////////

class CForwardFactory : public IClassFactory
{
protected:
    long m_lRef;
    CLSID m_ForwardClsid;

public:
    CForwardFactory(REFCLSID rForwardClsid) 
        : m_lRef(1), m_ForwardClsid(rForwardClsid)
    {}
    ~CForwardFactory();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, 
                                REFIID riid, void** ppv);
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);
};

//
//
// MyService 
//
//
//////////////////////////////////////////////////////////////////

class MyService : public CNtService
{
public:
    MyService(DWORD CtrlAccepted);
    ~MyService();
    
    // CNtService interface
    DWORD WorkerThread();
    void Stop(BOOL bSystemShutDownCalled);
    void Log(LPCSTR lpszMsg);

    VOID Pause();
    VOID Continue();

    //
    VOID FinalCleanup();
private:

    HANDLE m_hStopEvent;
};

//
//
//  yet another smart mutex
//
//
//////////////////////////////////////////////////////////////////

class CInMutex
{
protected:
    HANDLE m_hMutex;
public:
    CInMutex(HANDLE hMutex) : m_hMutex(hMutex)
    {
        if(m_hMutex)
            WaitForSingleObject(m_hMutex, INFINITE);
    }
    ~CInMutex()
    {
        if(m_hMutex)
            ReleaseMutex(m_hMutex);
    }
};


#endif
