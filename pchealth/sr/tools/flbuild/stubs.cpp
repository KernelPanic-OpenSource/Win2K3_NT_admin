#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <srconfig.h>
#include <utils.h>
#include <respoint.h>
#include <srapi.h>
#include <evthandler.h>
#include <enumlogs.h>
#include <ntservice.h>

CEventHandler * g_pEventHandler = NULL;
CSRConfig * g_pSRConfig = NULL;
CNTService * g_pSRService = NULL;

DWORD CEventHandler::SRUpdateMonitoredListS (LPWSTR pszXMLFile)
{
    return 0;
}

void CEventHandler::RefreshCurrentRp (BOOL fScanAllDrives)
{
}

void CNTService::LogEvent(WORD wType, DWORD dwID,
                  void * pRawData,
                  DWORD dwDataSize,
                  const WCHAR* pszS1,
                  const WCHAR* pszS2,
                  const WCHAR* pszS3)
{
}

DWORD PatchComputePatch(
    LPCWSTR pszCurrentDir)
{
    return ERROR_SUCCESS;
}

DWORD
PatchGetPatchWindow()
{
    return ERROR_SUCCESS;
}

DWORD 
PatchGetReferenceRpNum(
    DWORD  dwCurrentRp) 
{
    return ERROR_SUCCESS;
}
