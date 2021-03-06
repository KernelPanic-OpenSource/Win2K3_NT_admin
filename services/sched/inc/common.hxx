//+---------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       common.hxx
//
//  Contents:   Common globals.
//
//  Classes:    None.
//
//  Functions:  InitGlobals & FreeGlobals
//
//  History:    08-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.hxx.
//
//----------------------------------------------------------------------------

#ifndef __COMMON_HXX__
#define __COMMON_HXX__

//
// Default maximum run time - 3 days (in milliseconds).
//
#define MAX_RUN_TIME_DEFAULT (3 * 24 * 60 * 60 * 1000)

const DWORD RUN_TIME_NO_END = INFINITE;

//
// Default maximum log file size is in KB.
//
#define MAX_LOG_SIZE_DEFAULT (0x20)

//
// Default log name
//
#define TSZ_LOG_NAME_DEFAULT    L"%SystemRoot%\\Tasks\\SCHEDLGU.TXT"

//
// DLL that contains the service code and resources if we are
// running in svchost.exe.
//
#define SCH_SERVICE_DLL_NAME    TEXT("schedsvc.dll")

//
// Constants
//

const int SCH_BUF_LEN = 80;
const int SCH_SMBUF_LEN = 16;
const int SCH_TIMEBUF_LEN = 32;
const int SCH_DATEBUF_LEN = 64;
const int SCH_MED0BUF_LEN = 32;
const int SCH_MEDBUF_LEN = 64;
const int SCH_BIGBUF_LEN = 256;
const int SCH_XBIGBUF_LEN = 512;
const int SCH_DB_BUFLEN = 256;

#define TSZ_JOB                 TEXT("job")
#define TSZ_DOTJOB              TEXT(".job")
#define TSZ_AT_JOB_PREFIX       TEXT("At")

#define SCH_FOLDER_VALUE        TEXT("TasksFolder")
#define SCH_NOTIFYMISS_VALUE    TEXT("NotifyOnTaskMiss")
#define SCH_LASTRUN_VALUE       TEXT("LastTaskRun")
#define SCH_OLDNAME_VALUE       TEXT("OldName")
#define SCH_FIRSTBOOT_VALUE     TEXT("FirstBoot")
#define DOTEXE                  TEXT(".exe")

//
// The main window class and title names are necessary for Sage
// compatibility.
//
#define SCHED_CLASS             TEXT("SAGEWINDOWCLASS")
#define SCHED_TITLE             TEXT("SYSTEM AGENT COM WINDOW")

#define SCHED_SERVICE_NAME      TEXT("Schedule")
#define SCHED_SETUP_APP_NAME    TEXT("mstinit.exe")
#define SCH_SVC_KEY             REGSTR_PATH_SERVICES TEXT("\\") SCHED_SERVICE_NAME
#define SCH_AGENT_KEY           TEXT("SOFTWARE\\Microsoft\\SchedulingAgent")
#define SCH_RUN_VALUE           TEXT("SchedulingAgent")
#define REGSTR_WINLOGON         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define REGSTR_DEFAULT_DOMAIN   TEXT("DefaultDomainName")

const WORD SCH_DEFAULT_IDLE_TIME = 10;      // 10 minutes.
const WORD SCH_DEFAULT_IDLE_DEADLINE = 60;  // 1 hour

#define MAX_SID_SIZE 68                 // SIDs are 68 bytes maximum
                                        // (computed from winnt.h SID def)

//
// Private service control codes. Sdk Help says user defined control codes
// should be in the range of 128 to 255. Note that some of these controls
// are used for intra-thread signalling within the service and are not sent
// to the service from outside sources. This is to leverage the functionality
// of the control event.
//
#define SERVICE_CONTROL_USER_LOGON              128
#define SERVICE_CONTROL_TIME_CHANGED            129
#define SERVICE_CONTROL_POWER_SUSPEND           130
#define SERVICE_CONTROL_POWER_SUSPEND_FAILED    131
#define SERVICE_CONTROL_POWER_RESUME            132
#define SERVICE_CONTROL_USER_LOGOFF             133

//
// Service State -- for CurrentState, from winsvc.h
//
// these are defined for NT, define them here for Windows
#if !defined(SERVICE_STOPPED)
#define SERVICE_STOPPED                 0x00000001
#endif
#if !defined(SERVICE_START_PENDING)
#define SERVICE_START_PENDING           0x00000002
#endif
#if !defined(SERVICE_STOP_PENDING)
#define SERVICE_STOP_PENDING            0x00000003
#endif
#if !defined(SERVICE_RUNNING)
#define SERVICE_RUNNING                 0x00000004
#endif
#if !defined(SERVICE_CONTINUE_PENDING)
#define SERVICE_CONTINUE_PENDING        0x00000005
#endif
#if !defined(SERVICE_PAUSE_PENDING)
#define SERVICE_PAUSE_PENDING           0x00000006
#endif
#if !defined(SERVICE_PAUSED)
#define SERVICE_PAUSED                  0x00000007
#endif

//
// Messages from the service main thread to the message-pump thread
// to call functions in msidle.dll
//

//
//  Message:    WM_SCHED_SetNextIdleNotification
//  wParam:     wIdleWait
//  lParam:     Unused
//  Return:     Unused
//
#define WM_SCHED_SetNextIdleNotification    (WM_USER + 210)

//
//  Message:    WM_SCHED_SetIdleLossNotification
//  wParam:     Unused
//  lParam:     Unused
//  Return:     Unused
//
#define WM_SCHED_SetIdleLossNotification    (WM_USER + 211)


//
// Globals
//

extern HINSTANCE g_hInstance;
extern TCHAR   g_tszSrvcName[];
extern BOOL    g_fNotifyMiss;
extern DWORD   g_WakeCountSlot;
extern WCHAR g_wszAtJobSearchPath[];

//
// Tasks folder structure definition.
//

typedef enum _FILESYSTEMTYPE {
    FILESYSTEM_FAT,
    FILESYSTEM_NTFS
} FILESYSTEMTYPE;

typedef struct _TasksFolderInfo {
    LPTSTR         ptszPath;
    FILESYSTEMTYPE FileSystemType;
} TasksFolderInfo;

//
// Registry settings for the scheduler (and more).
//
extern TasksFolderInfo g_TasksFolderInfo;

//
// BUGBUG: global __int64 initialization is not working without the CRT.
// BUG # 37752.
//
extern __int64 FILETIMES_PER_DAY;

//
// Global data initialization/cleanup routines.
//

HRESULT
InitGlobals(void);

void
FreeGlobals(void);

//
// Routines for reading/writing other registry data.
//
BOOL ReadLastTaskRun(SYSTEMTIME * pstLastRun);
void WriteLastTaskRun(const SYSTEMTIME * pstLastRun);

//
// Retrieves the file system type, either FAT or NTFS, of the path indicated.
// This function initializes the FileSystemType field of g_TasksFolderInfo.
//
HRESULT
GetFileSystemTypeFromPath(
    LPCWSTR          pwszPath,
    FILESYSTEMTYPE * pFileSystemType);

HRESULT
GetTasksFolder(
    WCHAR** ppwszPath
    );

HRESULT
GetNotifyOnTaskMiss(
    BOOL* pfNotifyOnTaskMiss
    );

HRESULT
EnsureTasksFolderExists(
    LPWSTR pwszPath,
    BOOL   bEnableShellExtension = TRUE
    );

//
// Routines in path.cxx for working with file & path names
//

#define MAX_PATH_VALUE  1024

VOID
StripLeadTrailSpace(LPTSTR ptsz);

VOID
GetAppPathInfo(
        LPCTSTR ptszFilename,
        LPTSTR  ptszAppPathDefault,
        ULONG   cchDefaultBuf,
        LPTSTR  ptszAppPathVar,
        ULONG   cchPathVarBuf);

BOOL
ProcessApplicationName(LPTSTR ptszFilename, size_t cchBuff, LPCTSTR tszWorkingDir);

BOOL
IsLocalFilename(LPCTSTR tszFilename);

VOID
DeleteQuotes(LPTSTR ptsz);

VOID
AddQuotes(LPTSTR ptsz, ULONG cchBuf);

BOOL
FileExists(LPTSTR ptszFileName, size_t cchBuff);


//+---------------------------------------------------------------------------
//
//  Function:   NextChar
//
//  Synopsis:   Return [wszCur] + 1, or [wszCur] if it points to the end of
//              the string.
//
//  History:    10-24-96   DavidMun   Created
//
//----------------------------------------------------------------------------
inline LPWSTR
NextChar(LPCWSTR wszCur)
{
    if (*wszCur)
    {
        return (LPWSTR) (wszCur + 1);
    }
    return (LPWSTR) wszCur;
}

#endif // __COMMON_HXX__
