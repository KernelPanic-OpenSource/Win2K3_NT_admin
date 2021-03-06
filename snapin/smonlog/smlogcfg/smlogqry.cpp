/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smlogqry.cpp

Abstract:

    Implementation of the CSmLogQuery base class. This object 
    is used to represent performance data log queries (a.k.a.
    sysmon log queries).

--*/

#include "Stdafx.h"
#include <pdh.h>            // for MIN_TIME_VALUE, MAX_TIME_VALUE
#include <pdhmsg.h>         // for PDH status values
#include <pdhp.h>           // for PLA methods
#include <strsafe.h>
#include "ipropbag.h"
#include "smlogs.h"
#include "smcfgmsg.h"
#include "smproppg.h"
#include "strnoloc.h"
#include "smlogqry.h"

USE_HANDLE_MACROS("SMLOGCFG(smlogqry.cpp)");

#define  DEFAULT_LOG_FILE_SERIAL_NUMBER ((DWORD)0x00000001)
#define  DEFAULT_LOG_FILE_MAX_SIZE      ((DWORD)-1)
#define  DEFAULT_CTR_LOG_FILE_TYPE      (SLF_BIN_FILE)
#define  DEFAULT_TRACE_LOG_FILE_TYPE    (SLF_SEQ_TRACE_FILE)
#define  DEFAULT_LOG_FILE_AUTO_FORMAT   SLF_NAME_NNNNNN
#define  DEFAULT_CURRENT_STATE          SLQ_QUERY_STOPPED
#define  DEFAULT_EXECUTE_ONLY           0
#define  DEFAULT_EOF_COMMAND_FILE       L""
#define  DEFAULT_RESTART_VALUE         ((DWORD)0)

#define  DEFAULT_COMMENT            L""
#define  DEFAULT_SQL_LOG_BASE_NAME  L""

#pragma warning ( disable : 4201)

typedef union {                        
    struct {
        SHORT      iMajor;     
        SHORT      iMinor;     
    }; 
    DWORD          dwVersion;      
} SMONCTRL_VERSION_DATA;

#pragma warning ( default : 4201 )


#define SMONCTRL_MAJ_VERSION    3
#define SMONCTRL_MIN_VERSION    3

typedef struct _REG_HTML_VALUE_NAME_MAP {
    UINT    iID;
    LPCWSTR szNonLocValueName;
} REG_HTML_VALUE_NAME_MAP, *PREG_HTML_VALUE_NAME_MAP;

//
// NOTE:  Unless otherwise specified, all registry Resource Id values are contiguous
// beginning with 816, so they can be used as indexes into the map.
//
const REG_HTML_VALUE_NAME_MAP RegValueNameMap[] = 
{
    { IDS_REG_COMMENT                 , CGlobalString::m_cszRegComment },                         
    { IDS_REG_LOG_TYPE                , CGlobalString::m_cszRegLogType },
    { IDS_REG_CURRENT_STATE           , CGlobalString::m_cszRegCurrentState },
    { IDS_REG_LOG_FILE_MAX_SIZE       , CGlobalString::m_cszRegLogFileMaxSize },
    { IDS_REG_LOG_FILE_BASE_NAME      , CGlobalString::m_cszRegLogFileBaseName },      
    { IDS_REG_LOG_FILE_FOLDER         , CGlobalString::m_cszRegLogFileFolder },         
    { IDS_REG_LOG_FILE_SERIAL_NUMBER  , CGlobalString::m_cszRegLogFileSerialNumber },  
    { IDS_REG_LOG_FILE_AUTO_FORMAT    , CGlobalString::m_cszRegLogFileAutoFormat },    
    { IDS_REG_LOG_FILE_TYPE           , CGlobalString::m_cszRegLogFileType },         
    { IDS_REG_START_TIME              , CGlobalString::m_cszRegStartTime },            
    { IDS_REG_STOP_TIME               , CGlobalString::m_cszRegStopTime },                 
    { IDS_REG_RESTART                 , CGlobalString::m_cszRegRestart },              
    { IDS_REG_LAST_MODIFIED           , CGlobalString::m_cszRegLastModified },         
    { IDS_REG_COUNTER_LIST            , CGlobalString::m_cszRegCounterList },          
    { IDS_REG_SAMPLE_INTERVAL         , CGlobalString::m_cszRegSampleInterval },       
    { IDS_REG_EOF_COMMAND_FILE        , CGlobalString::m_cszRegEofCommandFile },       
    { IDS_REG_COMMAND_FILE            , CGlobalString::m_cszRegCommandFile },          
    { IDS_REG_NETWORK_NAME            , CGlobalString::m_cszRegNetworkName },  
    { IDS_REG_USER_TEXT               , CGlobalString::m_cszRegUserText },   
    { IDS_REG_PERF_LOG_NAME           , CGlobalString::m_cszRegPerfLogName },       
    { IDS_REG_ACTION_FLAGS            , CGlobalString::m_cszRegActionFlags },           
    { IDS_REG_TRACE_BUFFER_SIZE       , CGlobalString::m_cszRegTraceBufferSize },      
    { IDS_REG_TRACE_BUFFER_MIN_COUNT  , CGlobalString::m_cszRegTraceBufferMinCount },  
    { IDS_REG_TRACE_BUFFER_MAX_COUNT  , CGlobalString::m_cszRegTraceBufferMaxCount },            
    { IDS_REG_TRACE_BUFFER_FLUSH_INT  , CGlobalString::m_cszRegTraceBufferFlushInterval },         
    { IDS_REG_TRACE_FLAGS             , CGlobalString::m_cszRegTraceFlags },                        
    { IDS_REG_TRACE_PROVIDER_LIST     , CGlobalString::m_cszRegTraceProviderList },     
    { IDS_REG_ALERT_THRESHOLD         , CGlobalString::m_cszRegAlertThreshold },           
    { IDS_REG_ALERT_OVER_UNDER        , CGlobalString::m_cszRegAlertOverUnder },
    { IDS_REG_TRACE_PROVIDER_COUNT    , CGlobalString::m_cszRegTraceProviderCount },
    { IDS_REG_TRACE_PROVIDER_GUID     , CGlobalString::m_cszRegTraceProviderGuid },
    { IDS_DEFAULT_LOG_FILE_FOLDER     , CGlobalString::m_cszRegDefaultLogFileFolder },  // In registry, but not part of query config.
    { IDS_REG_COLLECTION_NAME         , CGlobalString::m_cszRegCollectionName },
    { IDS_REG_DATA_STORE_ATTRIBUTES   , CGlobalString::m_cszRegDataStoreAttributes },         
    { IDS_REG_REALTIME_DATASOURCE     , CGlobalString::m_cszRegRealTimeDataSource },    
    { IDS_REG_SQL_LOG_BASE_NAME       , CGlobalString::m_cszRegSqlLogBaseName },   
    { IDS_REG_COMMENT_INDIRECT        , CGlobalString::m_cszRegCommentIndirect },      
    { IDS_REG_LOG_FILE_BASE_NAME_IND  , CGlobalString::m_cszRegLogFileBaseNameInd },       
    { IDS_REG_USER_TEXT_INDIRECT      , CGlobalString::m_cszRegUserTextIndirect },  
    // NOTE:  IDS_REG_EXECUTE_ONLY is 890, so cannot be indexed.
    { IDS_REG_EXECUTE_ONLY            , CGlobalString::m_cszRegExecuteOnly } 
};    
    
static const DWORD dwRegValueNameMapEntries = sizeof(RegValueNameMap)/sizeof(RegValueNameMap[0]);

//
// NOTE:  Unless otherwise specified, all Html Resource Id values are contiguous
// beginning with 900, so they can be used as indexes into the map.
//
const REG_HTML_VALUE_NAME_MAP HtmlPropNameMap[] = 
{
    { IDS_HTML_COMMENT                , CGlobalString::m_cszHtmlComment },
    { IDS_HTML_LOG_TYPE               , CGlobalString::m_cszHtmlLogType },
    { IDS_HTML_CURRENT_STATE          , CGlobalString::m_cszHtmlCurrentState },
    { IDS_HTML_LOG_FILE_MAX_SIZE      , CGlobalString::m_cszHtmlLogFileMaxSize },
    { IDS_HTML_LOG_FILE_BASE_NAME     , CGlobalString::m_cszHtmlLogFileBaseName },
    { IDS_HTML_LOG_FILE_FOLDER        , CGlobalString::m_cszHtmlLogFileFolder },
    { IDS_HTML_LOG_FILE_SERIAL_NUMBER , CGlobalString::m_cszHtmlLogFileSerialNumber },
    { IDS_HTML_LOG_FILE_AUTO_FORMAT   , CGlobalString::m_cszHtmlLogFileAutoFormat },
    { IDS_HTML_LOG_FILE_TYPE          , CGlobalString::m_cszHtmlLogFileType },
    { IDS_HTML_EOF_COMMAND_FILE       , CGlobalString::m_cszHtmlEOFCommandFile },
    { IDS_HTML_COMMAND_FILE           , CGlobalString::m_cszHtmlCommandFile },
    { IDS_HTML_NETWORK_NAME           , CGlobalString::m_cszHtmlNetworkName },
    { IDS_HTML_USER_TEXT              , CGlobalString::m_cszHtmlUserText },
    { IDS_HTML_PERF_LOG_NAME          , CGlobalString::m_cszHtmlPerfLogName }, 
    { IDS_HTML_ACTION_FLAGS           , CGlobalString::m_cszHtmlActionFlags }, 
    { IDS_HTML_RESTART                , L'\0' },                                      // Obsolete
    { IDS_HTML_TRACE_BUFFER_SIZE      , CGlobalString::m_cszHtmlTraceBufferSize }, 
    { IDS_HTML_TRACE_BUFFER_MIN_COUNT , CGlobalString::m_cszHtmlTraceBufferMinCount }, 
    { IDS_HTML_TRACE_BUFFER_MAX_COUNT , CGlobalString::m_cszHtmlTraceBufferMaxCount }, 
    { IDS_HTML_TRACE_BUFFER_FLUSH_INT , CGlobalString::m_cszHtmlTraceBufferFlushInterval }, 
    { IDS_HTML_TRACE_FLAGS            , CGlobalString::m_cszHtmlTraceFlags }, 
    { IDS_HTML_SYSMON_LOGFILENAME     , CGlobalString::m_cszHtmlLogFileName }, 
    { IDS_HTML_SYSMON_COUNTERCOUNT    , CGlobalString::m_cszHtmlCounterCount }, 
    { IDS_HTML_SYSMON_SAMPLECOUNT     , CGlobalString::m_cszHtmlSampleCount }, 
    { IDS_HTML_SYSMON_UPDATEINTERVAL  , CGlobalString::m_cszHtmlUpdateInterval }, 
    { IDS_HTML_SYSMON_COUNTERPATH     , CGlobalString::m_cszHtmlCounterPath }, 
    { IDS_HTML_RESTART_MODE           , CGlobalString::m_cszHtmlRestartMode }, 
    { IDS_HTML_SAMPLE_INT_UNIT_TYPE   , CGlobalString::m_cszHtmlSampleIntervalUnitType }, 
    { IDS_HTML_SAMPLE_INT_VALUE       , CGlobalString::m_cszHtmlSampleIntervalValue },      
    { IDS_HTML_START_MODE             , CGlobalString::m_cszHtmlStartMode },   
    { IDS_HTML_START_AT_TIME          , CGlobalString::m_cszHtmlStartAtTime },  
    { IDS_HTML_STOP_MODE              , CGlobalString::m_cszHtmlStopMode },   
    { IDS_HTML_STOP_AT_TIME           , CGlobalString::m_cszHtmlStopAtTime },   
    { IDS_HTML_STOP_AFTER_UNIT_TYPE   , CGlobalString::m_cszHtmlStopAfterUnitType },   
    { IDS_HTML_STOP_AFTER_VALUE       , CGlobalString::m_cszHtmlStopAfterValue },       
    { IDS_HTML_ALERT_THRESHOLD        , CGlobalString::m_cszHtmlCounterAlertThreshold }, 
    { IDS_HTML_ALERT_OVER_UNDER       , CGlobalString::m_cszHtmlCounterAlertOverUnder }, 
    { IDS_HTML_TRACE_PROVIDER_COUNT   , CGlobalString::m_cszHtmlTraceProviderCount }, 
    { IDS_HTML_TRACE_PROVIDER_GUID    , CGlobalString::m_cszHtmlTraceProviderGuid }, 
    { IDS_HTML_LOG_NAME               , CGlobalString::m_cszHtmlLogName },                   
    { IDS_HTML_ALERT_NAME             , CGlobalString::m_cszHtmlAlertName }, 
    { IDS_HTML_SYSMON_VERSION         , CGlobalString::m_cszHtml_Version }, 
    { IDS_HTML_DATA_STORE_ATTRIBUTES  , CGlobalString::m_cszHtmlDataStoreAttributes }, 
    { IDS_HTML_REALTIME_DATASOURCE    , CGlobalString::m_cszHtmlRealTimeDataSource }, 
    { IDS_HTML_SQL_LOG_BASE_NAME      , CGlobalString::m_cszHtmlSqlLogBaseName } 
};
    
static const DWORD dwHtmlPropNameMapEntries = sizeof(HtmlPropNameMap)/sizeof(HtmlPropNameMap[0]);

DWORD   g_dwRealTimeQuery = DATA_SOURCE_REGISTRY;
const   CString CSmLogQuery::cstrEmpty;

//
//  Constructor
CSmLogQuery::CSmLogQuery( CSmLogService* pLogService )
:   m_pLogService ( pLogService ),
    m_bReadOnly ( FALSE ),
    m_bIsModified ( FALSE ),
    m_bIsNew ( FALSE ),
    m_bExecuteOnly ( FALSE ),
    m_pActivePropPage ( NULL ),
    mr_dwCurrentState ( SLQ_QUERY_STOPPED ),
    mr_dwMaxSize ( 0 ),
    mr_dwFileSizeUnits ( 0 ),
    mr_dwAppendMode ( 0 ),
    mr_dwLogAutoFormat ( 0 ),
    mr_dwLogFileType ( 0 ), 
    m_pInitialPropertySheet ( NULL )
{
    // initialize member variables
    memset (&mr_stiStart, 0, sizeof(mr_stiStart));
    memset (&mr_stiStop, 0, sizeof(mr_stiStop));
    memset (&m_PropData.stiSampleTime, 0, sizeof(m_PropData.stiSampleTime));
    m_PropData.dwMaxFileSize = 0;
    m_PropData.dwLogFileType = 0;
    m_PropData.dwSuffix = 0;
    m_PropData.dwSerialNumber = 0;
    mr_dwRealTimeQuery = g_dwRealTimeQuery;
    m_fDirtyPassword = PASSWORD_CLEAN;

    // All CString variables are empty on construction

    return;
}

//
//  Destructor
CSmLogQuery::~CSmLogQuery()
{
    // make sure Close method was called first!
    ASSERT ( NULL == m_hKeyQuery );
    ASSERT ( m_strName.IsEmpty() );
    ASSERT ( mr_strComment.IsEmpty() );
    ASSERT ( mr_strCommentIndirect.IsEmpty() );
    ASSERT ( mr_strBaseFileName.IsEmpty() );
    ASSERT ( mr_strBaseFileNameIndirect.IsEmpty() );
    ASSERT ( mr_strSqlName.IsEmpty() );
    ASSERT ( mr_strDefaultDirectory.IsEmpty() );
    return;
}
//
//  helper functions
//
LONG
CSmLogQuery::WriteRegistryStringValue (
    HKEY    hKey,
    UINT    uiValueName,
    DWORD   dwType,
    LPCWSTR pszBuffer,
    LPDWORD pdwBufSize
)
//  writes the contents of pszBuffer to szValue under hKey
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwLclSize;
    CONST BYTE *pLclBuffer = NULL;

    ResourceStateManager   rsm;

    if ( NULL != hKey ) {

        ASSERT ((dwType == REG_SZ) || 
                (dwType == REG_MULTI_SZ) ||
                (dwType == REG_EXPAND_SZ));

        if ( NULL == pszBuffer ) {
            // substitute an empty string
            pLclBuffer = (CONST BYTE *)L"\0";
            dwLclSize = sizeof(WCHAR);
        } else {
            // use args passed in
            pLclBuffer = (CONST BYTE *)pszBuffer;

            if ( NULL != pdwBufSize ) {
                if( 0 == *pdwBufSize ){
                    dwLclSize = lstrlen( pszBuffer );
                    if ( 0 < dwLclSize ) {
                        dwLclSize *= sizeof(WCHAR);
                    } else {
                        dwLclSize = sizeof(WCHAR);
                    }
                } else {
                    dwLclSize = *pdwBufSize;
                }
            } else {
                dwLclSize = lstrlen( pszBuffer );
                dwLclSize *= sizeof(WCHAR);
            }
        }

        dwStatus = RegSetValueEx (
            hKey, 
            GetNonLocRegValueName ( uiValueName ), 
            0L,
            dwType,
            (CONST BYTE *)pLclBuffer,
            dwLclSize); 
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}

LONG
CSmLogQuery::WriteRegistryDwordValue (
    HKEY     hKey,
    UINT     uiValueName,
    LPDWORD  pdwValue,
    DWORD    dwType   
)
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwSize = sizeof(DWORD);
    ResourceStateManager   rsm;

    if ( NULL != pdwValue && NULL != hKey ) {

        ASSERT ((dwType == REG_DWORD) || 
                (dwType == REG_BINARY));

        dwStatus = RegSetValueEx (
            hKey, 
            GetNonLocRegValueName ( uiValueName ), 
            0L,
            dwType,
            (CONST BYTE *)pdwValue,
            dwSize);

    } else {
        ASSERT ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}

LONG
CSmLogQuery::WriteRegistrySlqTime (
    HKEY     hKey,
    UINT     uiValueName,
    PSLQ_TIME_INFO  pSlqTime
)
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(SLQ_TIME_INFO);

    ResourceStateManager   rsm;

    if ( NULL != pSlqTime && NULL != hKey ) {

        dwStatus = RegSetValueEx (
            hKey, 
            GetNonLocRegValueName ( uiValueName ), 
            0L,
            REG_BINARY,
            (CONST BYTE *)pSlqTime,
            dwValue);
    } else {
        ASSERT ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}

LONG
CSmLogQuery::ReadRegistryStringValue (
    HKEY     hKey,
    LPCWSTR  szValueName,
    LPCWSTR  szNonLocValueName,
    LPCWSTR  szDefault,
    LPWSTR   *pszBuffer,
    LPDWORD  pdwBufferSize
)
//
//  Reads the string value from named value under hKey and
//  frees any existing buffer referenced by szInBuffer, 
//  then allocates a new buffer returning it with the 
//  string value read from the registry and the size of the
//  buffer in bytes.
//  For Win2K and XP through SP1, value names might have been localized. 
//  If a value is not found using the non-localized value name,
//  retry with a localized string.
//  If no value found in the registry, return the default value.
//  The last WCHAR in the buffer is specifically set to NULL.
//
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;
    WCHAR*  szNewStringBuffer = NULL;
    size_t  cchBufLen = 0;
    BOOL    bLocalized = FALSE;

    if ( NULL != pdwBufferSize ) {
        *pdwBufferSize = dwBufferSize;
    }

    if ( hKey != NULL) {
        //
        // There should be something to read.
        // Find out the size of the required buffer.
        //
        // Try first with the non-localized name.
        dwStatus = RegQueryValueExW (
            hKey,
            szNonLocValueName,
            NULL,
            &dwType,
            NULL,
            &dwBufferSize);

        if ( ERROR_SUCCESS != dwStatus ) { 
            //
            // Unable to read buffer.
            // If the non-localized value name is different from the possibly
            // localized version, retry with the localized version.
            if ( 0 != lstrcmpi ( szValueName, szNonLocValueName ) ) { 
                dwStatus = RegQueryValueExW (
                    hKey,
                    szValueName,
                    NULL,
                    &dwType,
                    NULL,
                    &dwBufferSize);

                if ( ERROR_SUCCESS == dwStatus ) {
                    bLocalized = TRUE;
                }
            }
        }
        
        if (dwStatus == ERROR_SUCCESS) {
            //
            // NULL character size is 2 bytes
            //
            if (dwBufferSize > sizeof(WCHAR) ) {
                //
                // There's something to read
                //
                MFC_TRY
                    szNewStringBuffer = new WCHAR[dwBufferSize/sizeof(WCHAR)];
                    dwType = 0;
                    dwStatus = RegQueryValueExW (
                        hKey,
                        ( bLocalized ? szValueName : szNonLocValueName),
                        NULL,
                        &dwType,
                        (LPBYTE)szNewStringBuffer,
                        &dwBufferSize);
                MFC_CATCH_DWSTATUS

                if ( ERROR_SUCCESS == dwStatus ) {
                    //
                    // Ensure that the registry string is null terminated.
                    //
                    cchBufLen = dwBufferSize/sizeof(WCHAR);
                    szNewStringBuffer[cchBufLen - 1] = L'\0';

                    if ( 0 == lstrlenW ( szNewStringBuffer ) ) {
                        dwStatus = ERROR_NO_DATA;
                    }
                }
            } else {
                //
                // Nothing to read.
                //
                dwStatus = ERROR_NO_DATA;
            }
        }   // else unable to read buffer.
            // dwStatus has error.
        
    } else {
        //
        // Null key.
        //
        dwStatus = ERROR_BADKEY;
    }

    if (dwStatus != ERROR_SUCCESS) {
        if (szNewStringBuffer != NULL) {
            delete [] szNewStringBuffer;
            szNewStringBuffer = NULL;
        }
        //
        // Apply default.
        //
        if ( szDefault != NULL ) {
            cchBufLen = 0;
            //
            // StringCchLen fails if szDefault is null.
            //
            hr = StringCchLength ( szDefault, STRSAFE_MAX_CCH, &cchBufLen );

            if ( SUCCEEDED (hr) ) {
                // Null terminator.
                cchBufLen++;
            }
            MFC_TRY
                szNewStringBuffer = new WCHAR[cchBufLen];
                StringCchCopy (
                    szNewStringBuffer,
                    cchBufLen,
                    szDefault);
                dwStatus = ERROR_SUCCESS;
                dwBufferSize = (DWORD)(cchBufLen * sizeof(WCHAR));
            MFC_CATCH_DWSTATUS
        } // else no default so no data returned
    }

    if (dwStatus == ERROR_SUCCESS) {
    
        //
        // Delete the old buffer and replace it with 
        // the new one.
        //
        if ( NULL != *pszBuffer ) {
            delete [] (*pszBuffer );
        }
        *pszBuffer = szNewStringBuffer;
        if ( NULL != pdwBufferSize ) {
            *pdwBufferSize = dwBufferSize;
        }
    } else {
        //
        // If error then delete the buffer
        if (szNewStringBuffer != NULL) {
            delete [] szNewStringBuffer;
        }
    }
    return dwStatus;
    
}

LONG
CSmLogQuery::ReadRegistryStringValue (
    HKEY     hKey,
    UINT     uiValueName,
    LPCWSTR  szDefault,
    LPWSTR   *pszBuffer,
    LPDWORD  pdwBufferSize
)
//
//  reads the string value from key (name based on resource 
//  uiValueName) under hKey and
//  frees any existing buffer referenced by szInBuffer, 
//  then allocates a new buffer returning it with the 
//  string value read from the registry and the size of the
//  buffer (in bytes) 
//
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;
    WCHAR*  szNewStringBuffer = NULL;
    CString strRegValueName;

    ResourceStateManager   rsm;

    MFC_TRY
        strRegValueName.LoadString ( uiValueName );
    MFC_CATCH_DWSTATUS;

    ASSERT (!strRegValueName.IsEmpty());

    if ( ERROR_SUCCESS == dwStatus ) {
        dwStatus = ReadRegistryStringValue (
                        hKey,
                        strRegValueName,
                        GetNonLocRegValueName ( uiValueName ),
                        szDefault,
                        pszBuffer,
                        pdwBufferSize );
    }

    return dwStatus;
}   

DWORD 
CSmLogQuery::SmNoLocReadRegIndStrVal (
    HKEY hKey, 
    UINT uiValueName,
    LPCWSTR szDefault, 
    LPWSTR* pszBuffer, 
    UINT*   puiLength )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LPCWSTR szNonLocValueName = NULL;
    CString strRegValueName;

    szNonLocValueName = GetNonLocRegValueName ( uiValueName );

    if ( NULL != szNonLocValueName ) {

        //
        // There should be something to read.
        //
        // Try first with the non-localized name.
    
        dwStatus = SmReadRegistryIndirectStringValue (
                        hKey,
                        szNonLocValueName,
                        szDefault,
                        pszBuffer,
                        puiLength );

        if ( ERROR_SUCCESS != dwStatus ) { 
            //
            // Unable to read buffer.
            // If the non-localized value name is different from the possibly
            // localized version, retry with the localized version.
            dwStatus = ERROR_SUCCESS;
            MFC_TRY
                strRegValueName.LoadString ( uiValueName );
            MFC_CATCH_DWSTATUS;

            ASSERT (!strRegValueName.IsEmpty());

            if ( ERROR_SUCCESS == dwStatus 
                &&  ( 0 != lstrcmpi ( strRegValueName, szNonLocValueName ) ) ) {

                dwStatus = SmReadRegistryIndirectStringValue (
                            hKey,
                            strRegValueName,
                            szDefault,
                            pszBuffer,
                            puiLength );
            }
        }
    }

    return dwStatus;
}

LONG
CSmLogQuery::ReadRegistrySlqTime (
    HKEY     hKey,
    UINT     uiValueName,
    PSLQ_TIME_INFO pstiDefault,
    PSLQ_TIME_INFO pSlqValue
)
//
//  reads the time value "szValueName" from under hKey and
//  returns it in the Value buffer
//
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;

    SLQ_TIME_INFO   slqLocal;
    CString strRegValueName;
    LPCWSTR szNonLocValueName = NULL;
    BOOL    bLocalizedValueName = FALSE;

    ResourceStateManager   rsm;

    if ( NULL != pSlqValue && NULL != hKey ) {

        szNonLocValueName = GetNonLocRegValueName ( uiValueName );

        if ( NULL != szNonLocValueName ) {
            memset (&slqLocal, 0, sizeof(SLQ_TIME_INFO));
            //
            // Find out the size of the required buffer,
            // and whether the value exists in the registry.
            // Try first with the non-localized value name.
            //
            dwStatus = RegQueryValueExW (
                hKey,
                szNonLocValueName,
                NULL,
                &dwType,
                NULL,
                &dwBufferSize);

            if ( ERROR_SUCCESS != dwStatus ) {

                dwStatus = ERROR_SUCCESS;
                MFC_TRY
                    strRegValueName.LoadString ( uiValueName );
                MFC_CATCH_DWSTATUS;

                ASSERT (!strRegValueName.IsEmpty());

                if ( ERROR_SUCCESS == dwStatus 
                    &&  ( 0 != lstrcmpi ( strRegValueName, szNonLocValueName ) ) ) {

                    //
                    // Retry if the value name has been localized.
                    //
                    memset (&slqLocal, 0, sizeof(SLQ_TIME_INFO));
                    dwStatus = RegQueryValueExW (
                        hKey,
                        strRegValueName,
                        NULL,
                        &dwType,
                        NULL,
                        &dwBufferSize);

                    if ( ERROR_SUCCESS == dwStatus ) {
                        bLocalizedValueName = TRUE;
                    }
                }
            }
            if (dwStatus == ERROR_SUCCESS) {
                if ((dwBufferSize == sizeof(SLQ_TIME_INFO)) && (dwType == REG_BINARY)) {
                    // then there's something to read
                    dwType = 0;
                    dwStatus = RegQueryValueExW (
                        hKey,
                        ( bLocalizedValueName? strRegValueName : szNonLocValueName ),
                        NULL,
                        &dwType,
                        (LPBYTE)&slqLocal,
                        &dwBufferSize);
                } else {
                    // nothing to read                
                    dwStatus = ERROR_NO_DATA;
                }
            }   // else 
                // unable to read buffer
                // dwStatus has error
        
            if (dwStatus == ERROR_SUCCESS) {
                *pSlqValue = slqLocal;
            } else {
                // apply default if it exists
                if (pstiDefault != NULL) {
                    *pSlqValue = *pstiDefault;
                    dwStatus = ERROR_SUCCESS;
                }
            }
        } else {
            // Value name Id is out of range.
            ASSERT ( FALSE );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        ASSERT ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}


LONG
CSmLogQuery::ReadRegistryDwordValue (
    HKEY    hKey,
    UINT    uiValueName,
    DWORD   dwDefault,
    LPDWORD pdwValue
)
//
//  reads the DWORD value "szValueName" from under hKey and
//  returns it in the Value buffer
//
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;
    DWORD   dwRegValue = 0;
    CString strRegValueName;
    LPCWSTR szNonLocValueName = NULL;
    BOOL    bLocalizedValueName = FALSE;

    ResourceStateManager   rsm;

    if ( NULL != pdwValue && NULL != hKey ) {

        szNonLocValueName = GetNonLocRegValueName ( uiValueName );

        if ( NULL != szNonLocValueName ) {
            //
            // Find out the size of the required buffer,
            // and whether the value exists in the registry.
            // Try first with the non-localized value name.
            //
            dwStatus = RegQueryValueExW (
                hKey,
                szNonLocValueName,
                NULL,
                &dwType,
                NULL,
                &dwBufferSize);

            if ( ERROR_SUCCESS != dwStatus ) {

                dwStatus = ERROR_SUCCESS;
                MFC_TRY
                    strRegValueName.LoadString ( uiValueName );
                MFC_CATCH_DWSTATUS;

                ASSERT (!strRegValueName.IsEmpty());

                if ( ERROR_SUCCESS == dwStatus 
                    &&  ( 0 != lstrcmpi ( strRegValueName, szNonLocValueName ) ) ) {

                    //
                    // Retry if the value name has been localized.
                    //
                    dwStatus = RegQueryValueExW (
                        hKey,
                        strRegValueName,
                        NULL,
                        &dwType,
                        NULL,
                        &dwBufferSize);
                    if ( ERROR_SUCCESS == dwStatus ) {
                        bLocalizedValueName = TRUE;
                    }
                }
            }
            if (dwStatus == ERROR_SUCCESS) {
                if ( (dwBufferSize == sizeof(DWORD)) 
                    && ( (REG_DWORD == dwType) || ( REG_BINARY == dwType) ) ) {
                    // then there's something to read
                    dwType = 0;
                    dwStatus = RegQueryValueExW (
                        hKey,
                        ( bLocalizedValueName ? strRegValueName : szNonLocValueName ),
                        NULL,
                        &dwType,
                        (LPBYTE)&dwRegValue,
                        &dwBufferSize);
                } else {
                    // nothing to read                
                    dwStatus = ERROR_NO_DATA;
                }
                
            }   // else 
                // unable to read buffer
                // dwStatus has error

            if (dwStatus == ERROR_SUCCESS) {
                *pdwValue = dwRegValue;
            } else {
                *pdwValue = dwDefault;
                dwStatus = ERROR_SUCCESS;
            }
        } else {
            // Value name Id is out of range.
            ASSERT ( FALSE );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        ASSERT ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}


HRESULT 
CSmLogQuery::StringToPropertyBag (
    IPropertyBag* pPropBag, 
    UINT uiPropName, 
    const CString& rstrData )
{
    return StringToPropertyBag ( 
            pPropBag, 
            GetNonLocHtmlPropName ( uiPropName ), 
            rstrData );
}

typedef struct _HTML_ENTITIES {
    LPWSTR szHTML;
    LPWSTR szEntity;
} HTML_ENTITIES;

HTML_ENTITIES g_htmlentities[] = {
    L"&",    L"&amp;",
    L"\"",   L"&quot;",
    L"<",    L"&lt;",
    L">",    L"&gt;",
    NULL, NULL
};

HRESULT 
CSmLogQuery::StringToPropertyBag (
    IPropertyBag* pIPropBag, 
    const CString& rstrPropName, 
    const CString& rstrData )
{
    HRESULT hr = S_OK;
    VARIANT vValue;
    LPWSTR  szTrans = NULL;
    BOOL    bAllocated = FALSE;
    int     i;
    size_t  cchLen = 0;
    LPWSTR  szScan = NULL;


    if ( NULL != pIPropBag ) {
        VariantInit( &vValue );
        vValue.vt = VT_BSTR;
        vValue.bstrVal = NULL;

        if ( !rstrData.IsEmpty() ) {
            MFC_TRY
                for( i=0 ;g_htmlentities[i].szHTML != NULL; i++ ){
                    //
                    // rstrData is const
                    //
                    // Max length of szHTML is 6.  Add 5 because 1 will be added
                    // when add rstrData.GetLength() below.
                    //
                    szScan = ((CString)rstrData).GetBuffer ( rstrData.GetLength() );
                    while( *szScan != L'\0' ){
                        if( *szScan == *g_htmlentities[i].szHTML ){
                            cchLen += 5;
                        }
                        szScan++;
                    }
                    ((CString)rstrData).ReleaseBuffer();
                }
                if( cchLen > 0 ){

                    //
                    // Add space for the original text.
                    //
                    cchLen += rstrData.GetLength() + 1;

                    szTrans = new WCHAR [cchLen];
                    bAllocated = TRUE;
                    ZeroMemory( szTrans, ( cchLen * sizeof(WCHAR) ) );
                    szScan = ((CString)rstrData).GetBuffer ( rstrData.GetLength() );
                    while( *szScan != L'\0' ){
                        BOOL bEntity = FALSE;
                        for( i=0; g_htmlentities[i].szHTML != NULL; i++ ){
                            if( *szScan == *g_htmlentities[i].szHTML ){
                                bEntity = TRUE;
                                StringCchCat ( szTrans, cchLen, g_htmlentities[i].szEntity );
                                break;
                            }
                        }
                        if( !bEntity ){
                            StringCchCatN ( szTrans, cchLen, szScan, 1 );
                        }
                        szScan++;
                    }
                } else {
                    szTrans = ((CString)rstrData).GetBuffer ( rstrData.GetLength() );
                }

                vValue.bstrVal = ::SysAllocString ( szTrans );
                hr = pIPropBag->Write ( rstrPropName, &vValue );    
                VariantClear ( &vValue );
                ((CString)rstrData).ReleaseBuffer();
            MFC_CATCH_HR
        } else {
            hr = pIPropBag->Write(rstrPropName, &vValue );    
        }
    }
    if( NULL != szTrans && bAllocated ){
        delete [] szTrans;
    }
    return hr;
}

HRESULT 
CSmLogQuery::DwordToPropertyBag (
    IPropertyBag* pPropBag, 
    UINT uiPropName, 
    DWORD dwData )
{
    return DwordToPropertyBag (
            pPropBag,
            GetNonLocHtmlPropName ( uiPropName ), 
            dwData );
}
    
HRESULT 
CSmLogQuery::DwordToPropertyBag (
    IPropertyBag* pPropBag, 
    const CString& rstrPropName, 
    DWORD dwData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_I4;
    vValue.lVal = (INT)dwData;

    hr = pPropBag->Write(rstrPropName, &vValue );

    VariantClear ( &vValue );

    return hr;
}

HRESULT 
CSmLogQuery::DoubleToPropertyBag (
    IPropertyBag* pPropBag, 
    UINT uiPropName, 
    DOUBLE dData )
{
    return DoubleToPropertyBag (
            pPropBag,
            GetNonLocHtmlPropName ( uiPropName ), 
            dData );
}

HRESULT 
CSmLogQuery::DoubleToPropertyBag (
    IPropertyBag* pPropBag, 
    const CString& rstrPropName, 
    DOUBLE dData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_R8;
    vValue.dblVal = dData;

    hr = pPropBag->Write(rstrPropName, &vValue );

    VariantClear ( &vValue );

    return hr;
}

HRESULT 
CSmLogQuery::FloatToPropertyBag (
    IPropertyBag* pPropBag, 
    UINT uiPropName, 
    FLOAT fData )
{
    return FloatToPropertyBag (
            pPropBag,
            GetNonLocHtmlPropName ( uiPropName ), 
            fData );
}

HRESULT 
CSmLogQuery::FloatToPropertyBag (
    IPropertyBag* pPropBag, 
    const CString& rstrPropName, 
    FLOAT fData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_R4;
    vValue.fltVal = fData;

    hr = pPropBag->Write(rstrPropName, &vValue );

    VariantClear ( &vValue );

    return hr;
}

HRESULT 
CSmLogQuery::LLTimeToPropertyBag (
    IPropertyBag* pIPropBag, 
    UINT uiPropName,
    LONGLONG& rllData )
{
    HRESULT hr;
    VARIANT vValue;
    CString strPropName;
    
    MFC_TRY
        strPropName = GetNonLocHtmlPropName ( uiPropName ), 

        VariantInit( &vValue );
        vValue.vt = VT_DATE;

        if ( LLTimeToVariantDate ( rllData, &vValue.date ) ) {

            hr = pIPropBag->Write(strPropName, &vValue );

            VariantClear ( &vValue );
    
        } else { 
            hr = E_FAIL;
        }
    MFC_CATCH_HR

    return hr;
}

HRESULT 
CSmLogQuery::SlqTimeToPropertyBag (
    IPropertyBag* pPropBag, 
    DWORD dwFlags, 
    PSLQ_TIME_INFO pSlqData )
{
    HRESULT hr = NOERROR;

    ASSERT ( NULL != pSlqData );

    switch (dwFlags) {
        case SLQ_TT_TTYPE_START:
            ASSERT ( SLQ_TT_TTYPE_START == pSlqData->wTimeType );

            hr = DwordToPropertyBag ( pPropBag, IDS_HTML_START_MODE, pSlqData->dwAutoMode );
            if ( SLQ_AUTO_MODE_AT == pSlqData->dwAutoMode ) {
                ASSERT ( SLQ_TT_DTYPE_DATETIME == pSlqData->wDataType );
                hr = LLTimeToPropertyBag ( pPropBag, IDS_HTML_START_AT_TIME, pSlqData->llDateTime );
            }
            
            break;

        case SLQ_TT_TTYPE_STOP:
            ASSERT ( SLQ_TT_TTYPE_STOP == pSlqData->wTimeType );

            hr = DwordToPropertyBag ( pPropBag, IDS_HTML_STOP_MODE, pSlqData->dwAutoMode );
            if ( SLQ_AUTO_MODE_AT == pSlqData->dwAutoMode ) {
                ASSERT ( SLQ_TT_DTYPE_DATETIME == pSlqData->wDataType );
                hr = LLTimeToPropertyBag ( pPropBag, IDS_HTML_STOP_AT_TIME, pSlqData->llDateTime );
            } else if ( SLQ_AUTO_MODE_AFTER == pSlqData->dwAutoMode ) {
                ASSERT ( SLQ_TT_DTYPE_UNITS == pSlqData->wDataType );
                hr = DwordToPropertyBag ( 
                        pPropBag, 
                        IDS_HTML_STOP_AFTER_UNIT_TYPE, 
                        pSlqData->dwUnitType );
                hr = DwordToPropertyBag ( 
                        pPropBag, 
                        IDS_HTML_STOP_AFTER_VALUE, 
                        pSlqData->dwValue );
            }
            
            break;
            
        case SLQ_TT_TTYPE_SAMPLE:
        {
            LONGLONG    llMillisecondSampleInt;
            FLOAT fSampleIntSeconds;
            
            ASSERT ( SLQ_TT_TTYPE_SAMPLE == pSlqData->wTimeType );
            ASSERT ( SLQ_TT_DTYPE_UNITS == pSlqData->wDataType );
//            ASSERT ( SLQ_AUTO_MODE_AFTER == pSlqData->dwAutoMode );

            // Write best approximation of sample time to Sysmon property.
            TimeInfoToMilliseconds ( pSlqData, &llMillisecondSampleInt );
                
            // Ensure that the millisecond sample interval fits in a DWORD.
            ASSERT ( llMillisecondSampleInt < ULONG_MAX );

            fSampleIntSeconds = (FLOAT)(llMillisecondSampleInt / 1000);
            hr = FloatToPropertyBag ( 
                    pPropBag, 
                    IDS_HTML_SYSMON_UPDATEINTERVAL, 
                    fSampleIntSeconds );

            hr = DwordToPropertyBag ( pPropBag, IDS_HTML_SAMPLE_INT_UNIT_TYPE, pSlqData->dwUnitType );
            hr = DwordToPropertyBag ( pPropBag, IDS_HTML_SAMPLE_INT_VALUE, pSlqData->dwValue );
            break;
        }
        // Restart mode stored as a single DWORD
        case SLQ_TT_TTYPE_RESTART:
        default:
            hr = E_INVALIDARG;
            break;
    }

    return hr;
}

HRESULT 
CSmLogQuery::StringFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    UINT uiPropName, 
    const CString& rstrDefault,
    LPWSTR   *pszBuffer,
    LPDWORD  pdwLength )
{
    HRESULT hr;
    CString strPropName;
    ResourceStateManager rsm;

//
//  reads the string value from property bag and
//  frees any existing buffer referenced by szData, 
//  then allocates a new buffer returning it with the 
//  string value read from the property bag and the size of the
//  buffer (in bytes) 
//

    MFC_TRY
        strPropName.LoadString ( uiPropName );

        hr = StringFromPropertyBag (
                pPropBag,
                pIErrorLog,
                strPropName,
                GetNonLocHtmlPropName ( uiPropName ),
                rstrDefault,
                pszBuffer,
                pdwLength );
    MFC_CATCH_HR
    return hr;
}

HRESULT 
CSmLogQuery::StringFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    const CString& rstrPropName, 
    const CString& rstrNonLocPropName, 
    const CString& rstrDefault,
    LPWSTR   *pszBuffer,
    LPDWORD  pdwLength )
{
    HRESULT hr = E_POINTER;
    VARIANT vValue;
    DWORD   cchNewBufLen = 0;
    LPWSTR  szNewStringBuffer = NULL;
    LPWSTR  szTrans = NULL;
    LPWSTR  szScan = NULL;

//
//  Reads the string value from property bag and
//  frees any existing buffer referenced by szData, 
//  then allocates a new buffer returning it with the 
//  string value read from the property bag and the size of the
//  buffer (in bytes) 
//

    ASSERT (pdwLength!= NULL);
    ASSERT (pszBuffer != NULL);

    if ( NULL == pdwLength || NULL == pszBuffer ) {
        return E_POINTER;
    }

    *pdwLength = 0;

    VariantInit( &vValue );
    vValue.vt = VT_BSTR;
    vValue.bstrVal = NULL;

    MFC_TRY
        hr = pPropBag->Read(rstrNonLocPropName, &vValue, pIErrorLog );

        if ( FAILED(hr) || NULL == vValue.bstrVal ) {
            hr = pPropBag->Read(rstrPropName, &vValue, pIErrorLog );
        }

        if ( SUCCEEDED(hr) && NULL != vValue.bstrVal ) {
            //
            // SysStringLen returns number of characters allocated for BSTR
            // It includes embedded nulls, but not the terminating null.
            //
            cchNewBufLen = SysStringLen(vValue.bstrVal);
            //
            // Ensure that the BSTR is null-terminated.
            //
            vValue.bstrVal[cchNewBufLen] = L'\0';
            
            cchNewBufLen++;    // Add 1 for null character to be allocated.

            if ( cchNewBufLen > 1 ) {
                // then there's something to read
                szTrans = new WCHAR[cchNewBufLen];
                szNewStringBuffer = new WCHAR[cchNewBufLen];    
                StringCchCopy ( szNewStringBuffer, cchNewBufLen, vValue.bstrVal );
                for( int i=0;g_htmlentities[i].szHTML != NULL;i++ ){
                    szScan = NULL;
                    while( szScan = wcsstr( szNewStringBuffer, g_htmlentities[i].szEntity ) ){
                        //
                        // Null the character at szScan, so that the (new) string 
                        // at the beginning of szNewStringBuffer will copied to szTrans.  
                        // Then the NULL character is overwritten with the character
                        // represented by the specified HTML entity.
                        //
                        *szScan = L'\0';
                        StringCchCopy(szTrans, cchNewBufLen, szNewStringBuffer);
                        StringCchCat(szTrans, cchNewBufLen, g_htmlentities[i].szHTML);

                        //
                        // szScan is then set to one character past the HTML entity.
                        //
                        szScan += lstrlen( g_htmlentities[i].szEntity);
                        //
                        // The rest of the original string is concatenated onto
                        // szTrans, and szNewStringBuffer replaced by the string at 
                        // szTrans, so the next loop will start again at the beginning
                        // of the string.
                        //
                        StringCchCat(szTrans, cchNewBufLen, szScan);

                        StringCchCopy(szNewStringBuffer, cchNewBufLen, szTrans);
                    }
                }
                delete [] szTrans;
                szTrans = NULL;
            } else if ( 0 != rstrDefault.GetLength() ) {
                //
                // Missing data in the property bag, so apply the default.
                // Add 1 for null character to be allocated.
                //
                cchNewBufLen = rstrDefault.GetLength() + 1; 

                szNewStringBuffer = new WCHAR[cchNewBufLen];

                StringCchCopy ( szNewStringBuffer, cchNewBufLen, rstrDefault );
                hr = S_OK;
            }
        } else if ( 0 != rstrDefault.GetLength() ) {
            //
            // Missing data in the property bag, so apply the default.
            // Add 1 for null character to be allocated.
            //
            cchNewBufLen = rstrDefault.GetLength() + 1;
        
            szNewStringBuffer = new WCHAR[cchNewBufLen];
            StringCchCopy ( szNewStringBuffer, cchNewBufLen, rstrDefault );
            hr = S_OK;
        }
    MFC_CATCH_HR

    if ( SUCCEEDED(hr)) {
        // then delete the old buffer and replace it with 
        // the new one
        if (*pszBuffer != NULL) {
            delete [] (*pszBuffer );
        }
        *pszBuffer = szNewStringBuffer;
        szNewStringBuffer = NULL;
        *pdwLength = cchNewBufLen;
    }        
    
    // if error then delete the buffer
    if ( NULL != szNewStringBuffer ) {
        delete [] szNewStringBuffer;
    }
    
    if ( NULL != szTrans ) {
        delete [] szTrans;
    }
    return hr;
}

HRESULT 
CSmLogQuery::DwordFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    UINT uiPropName, 
    DWORD  dwDefault,
    DWORD& rdwData )
{
    HRESULT hr;
    CString strPropName;

    ResourceStateManager rsm;

    MFC_TRY
        strPropName.LoadString ( uiPropName );

        hr = DwordFromPropertyBag ( 
                pPropBag,
                pIErrorLog,
                strPropName,
                GetNonLocHtmlPropName ( uiPropName ),
                dwDefault, 
                rdwData );
    MFC_CATCH_HR
    return hr;
}

HRESULT 
CSmLogQuery::DwordFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    const CString& rstrPropName, 
    const CString& rstrNonLocPropName, 
    DWORD  dwDefault,
    DWORD& rdwData )
{
    VARIANT vValue;
    HRESULT hr;

    rdwData = dwDefault;

    VariantInit( &vValue );
    vValue.vt = VT_I4;
    vValue.lVal = 0;

    hr = pPropBag->Read(rstrNonLocPropName, &vValue, pIErrorLog );

    if ( FAILED ( hr ) ) {
        hr = pPropBag->Read(rstrPropName, &vValue, pIErrorLog );
    }

    if ( E_INVALIDARG != hr ) {
        rdwData = (DWORD)vValue.lVal;
    } else {
        hr = S_OK;
    }

    return hr;
}

HRESULT 
CSmLogQuery::DoubleFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    UINT uiPropName, 
    DOUBLE  dDefault,
    DOUBLE& rdData )
{
    HRESULT hr;
    CString strPropName;

    ResourceStateManager rsm;

    MFC_TRY
        strPropName.LoadString ( uiPropName );
    
        hr = DoubleFromPropertyBag ( 
                pPropBag,
                pIErrorLog,
                strPropName,
                GetNonLocHtmlPropName ( uiPropName ), 
                dDefault, 
                rdData );
    MFC_CATCH_HR

    return hr;
}
    
HRESULT 
CSmLogQuery::DoubleFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    const CString& rstrPropName, 
    const CString& rstrNonLocPropName, 
    DOUBLE  dDefault,
    DOUBLE& rdData )
{
    VARIANT vValue;
    HRESULT hr;

    rdData = dDefault;

    VariantInit( &vValue );
    vValue.vt = VT_R8;
    vValue.dblVal = 0;

    hr = pPropBag->Read(rstrNonLocPropName, &vValue, pIErrorLog );

    if ( FAILED ( hr ) ) {
        hr = pPropBag->Read(rstrPropName, &vValue, pIErrorLog );
    }

    if ( E_INVALIDARG != hr ) {
        rdData = vValue.dblVal;
    } else {
        hr = S_OK;
    }

    return hr;
}

HRESULT 
CSmLogQuery::FloatFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    UINT uiPropName, 
    FLOAT  fDefault,
    FLOAT& rfData )
{
    HRESULT hr;
    CString strPropName;

    ResourceStateManager rsm;

    strPropName.LoadString ( uiPropName );
    
    hr = FloatFromPropertyBag ( 
            pPropBag,
            pIErrorLog,
            strPropName,
            GetNonLocHtmlPropName ( uiPropName ),
            fDefault, 
            rfData );

    return hr;
}
    
HRESULT 
CSmLogQuery::FloatFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    const CString& rstrPropName, 
    const CString& rstrNonLocPropName,
    FLOAT  fDefault,
    FLOAT& rfData )
{
    VARIANT vValue;
    HRESULT hr;

    rfData = fDefault;

    VariantInit( &vValue );
    vValue.vt = VT_R4;
    vValue.fltVal = 0;

    hr = pPropBag->Read(rstrNonLocPropName, &vValue, pIErrorLog );

    if ( FAILED ( hr ) ) {
        hr = pPropBag->Read(rstrPropName, &vValue, pIErrorLog );
    }

    if ( E_INVALIDARG != hr ) {
        rfData = vValue.fltVal;
    } else {
        hr = S_OK;
    }

    return hr;
}

HRESULT
CSmLogQuery::LLTimeFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    UINT uiPropName,
    LONGLONG&  rllDefault,
    LONGLONG& rllData )
{
    HRESULT hr = NOERROR;
    CString strPropName;
    VARIANT vValue;
    ResourceStateManager rsm;
    
    MFC_TRY
        strPropName.LoadString ( uiPropName );

        rllData = rllDefault;

        VariantInit( &vValue );
        vValue.vt = VT_DATE;

        hr = pIPropBag->Read(GetNonLocHtmlPropName( uiPropName ), &vValue, pIErrorLog );

        if ( FAILED ( hr ) ) {
            hr = pIPropBag->Read(strPropName, &vValue, pIErrorLog );
        }
        // If parameter not missing, translate and return.  Otherwise,
        // return the default.
        if ( E_INVALIDARG != hr ) {
            if ( !VariantDateToLLTime ( vValue.date, &rllData ) ) {
                hr = E_FAIL;
            }
            VariantClear( &vValue );
        } else {
            hr = S_OK;
        }
    MFC_CATCH_HR

    return hr;
}

HRESULT
CSmLogQuery::SlqTimeFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog*  pIErrorLog,
    DWORD dwFlags, 
    PSLQ_TIME_INFO pSlqDefault,
    PSLQ_TIME_INFO pSlqData )
{
    HRESULT hr = NOERROR;

    ASSERT ( NULL != pSlqData );

    switch (dwFlags) {
        case SLQ_TT_TTYPE_START:

            pSlqData->wTimeType = SLQ_TT_TTYPE_START;
            pSlqData->wDataType = SLQ_TT_DTYPE_DATETIME;

            hr = DwordFromPropertyBag ( 
                    pPropBag, 
                    pIErrorLog, 
                    IDS_HTML_START_MODE, 
                    pSlqDefault->dwAutoMode, 
                    pSlqData->dwAutoMode );
            
            if ( SLQ_AUTO_MODE_AT == pSlqData->dwAutoMode ) {
                hr = LLTimeFromPropertyBag ( 
                        pPropBag, 
                        pIErrorLog, 
                        IDS_HTML_START_AT_TIME, 
                        pSlqDefault->llDateTime, 
                        pSlqData->llDateTime );

            } else {
                // Original state is stopped.
                ASSERT ( SLQ_AUTO_MODE_NONE == pSlqData->dwAutoMode );
                pSlqData->llDateTime = MAX_TIME_VALUE;
            }
            
            break;

        case SLQ_TT_TTYPE_STOP:
            pSlqData->wTimeType = SLQ_TT_TTYPE_STOP;

            hr = DwordFromPropertyBag ( 
                    pPropBag, 
                    pIErrorLog, 
                    IDS_HTML_STOP_MODE, 
                    pSlqDefault->dwAutoMode, 
                    pSlqData->dwAutoMode );
            
            if ( SLQ_AUTO_MODE_AT == pSlqData->dwAutoMode ) {
                pSlqData->wDataType = SLQ_TT_DTYPE_DATETIME;
                hr = LLTimeFromPropertyBag ( 
                        pPropBag, 
                        pIErrorLog, 
                        IDS_HTML_STOP_AT_TIME, 
                        pSlqDefault->llDateTime, 
                        pSlqData->llDateTime );

            } else if ( SLQ_AUTO_MODE_AFTER == pSlqData->dwAutoMode ) {
                pSlqData->wDataType = SLQ_TT_DTYPE_UNITS;

                hr = DwordFromPropertyBag ( 
                        pPropBag, 
                        pIErrorLog, 
                        IDS_HTML_STOP_AFTER_UNIT_TYPE, 
                        pSlqDefault->dwUnitType, 
                        pSlqData->dwUnitType );

                hr = DwordFromPropertyBag ( 
                        pPropBag, 
                        pIErrorLog, 
                        IDS_HTML_STOP_AFTER_VALUE, 
                        pSlqDefault->dwValue, 
                        pSlqData->dwValue );
            } else {
                // Original state is stopped.
                // Mode is NONE or SIZE
                pSlqData->wDataType = SLQ_TT_DTYPE_DATETIME;
                pSlqData->llDateTime = MIN_TIME_VALUE;
            }
            
            break;
            
        case SLQ_TT_TTYPE_SAMPLE:
        {
            DWORD dwNullDefault = (DWORD)(-1);
            BOOL bUnitTypeMissing = FALSE;
            BOOL bUnitValueMissing = FALSE;

            hr = DwordFromPropertyBag ( 
                            pPropBag, 
                            pIErrorLog, 
                            IDS_HTML_SAMPLE_INT_UNIT_TYPE, 
                            dwNullDefault, 
                            pSlqData->dwUnitType );

            if ( (DWORD)(-1) == pSlqData->dwUnitType ) {
                pSlqData->dwUnitType = pSlqDefault->dwUnitType;
                bUnitTypeMissing = TRUE;
            }

            hr = DwordFromPropertyBag ( 
                            pPropBag, 
                            pIErrorLog, 
                            IDS_HTML_SAMPLE_INT_VALUE, 
                            dwNullDefault, 
                            pSlqData->dwValue );

            if ( (DWORD)(-1) == pSlqData->dwValue ) {
                pSlqData->dwValue = pSlqDefault->dwValue;
                bUnitValueMissing = TRUE;
            }

            if ( bUnitTypeMissing || bUnitValueMissing ) {
                FLOAT fDefaultUpdateInterval;
                FLOAT fUpdateInterval;

                // If unit type or unit count missing from the property bag,
                // look for "UpdateInterval" value, from the Sysmon control object,
                // and use it to approximate the sample time.
                fDefaultUpdateInterval = (FLOAT)(pSlqDefault->dwValue);

                hr = FloatFromPropertyBag ( 
                        pPropBag, 
                        pIErrorLog, 
                        IDS_HTML_SYSMON_UPDATEINTERVAL, 
                        fDefaultUpdateInterval, 
                        fUpdateInterval );

                if ( SUCCEEDED ( hr ) ) {
                    pSlqData->dwValue = (DWORD)(fUpdateInterval);
                    pSlqData->dwUnitType = SLQ_TT_UTYPE_SECONDS;
                }
            }
            break;
        }
        
        // Restart mode stored as a single DWORD
        case SLQ_TT_TTYPE_RESTART:
        default:
            hr = E_INVALIDARG;
            break;
    }

    return hr;
}




//  
//  Open function. either opens an existing log query entry
//  or creates a new one
//
DWORD   
CSmLogQuery::Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    //open the subkey for this log query
    m_hKeyQuery = hKeyQuery;
    m_bReadOnly = bReadOnly;
    m_bIsModified = FALSE;

    MFC_TRY
        m_strName = rstrName;
        dwStatus = SyncWithRegistry();
    
    MFC_CATCH_DWSTATUS
    return dwStatus;
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//      
DWORD   
CSmLogQuery::Close ()
{
    LOCALTRACE (L"Closing Query\n");

    KillString ( m_strPassword );

    m_strName.Empty();
    mr_strComment.Empty();
    mr_strCommentIndirect.Empty();
    mr_strBaseFileName.Empty();
    mr_strBaseFileNameIndirect.Empty();
    mr_strDefaultDirectory.Empty();
    mr_strSqlName.Empty();
    
    // close any open registry keys
    if (m_hKeyQuery != NULL) {
        RegCloseKey (m_hKeyQuery);
        m_hKeyQuery = NULL;
    }

    return ERROR_SUCCESS;
}

//  
//  ManualStart function. 
//      Sets the start mode to manual and starts the query.
//
DWORD   
CSmLogQuery::ManualStart ()
{
    DWORD           dwStatus = ERROR_SUCCESS;
    SLQ_TIME_INFO   slqTime;
    BOOL            bSetStopToMax;
    BOOL            bStarted = FALSE;
    DWORD           dwTimeout = 10;
    BOOL            bRegistryUpdated;

    memset (&slqTime, 0, sizeof(slqTime));
    slqTime.wTimeType = SLQ_TT_TTYPE_START;
    slqTime.wDataType = SLQ_TT_DTYPE_DATETIME;
    slqTime.dwAutoMode = SLQ_AUTO_MODE_NONE;
    slqTime.llDateTime = MIN_TIME_VALUE;

    SetLogTime ( &slqTime, SLQ_TT_TTYPE_START );

    // If stop time mode set to manual, or StopAt with time before Now,
    // set the mode to Manual, value to MAX_TIME_VALUE
    // For Size mode, just set the stop time to MAX_TIME_VALUE
    bSetStopToMax = FALSE;
    GetLogTime ( &slqTime, SLQ_TT_TTYPE_STOP );
    if ( SLQ_AUTO_MODE_NONE == slqTime.dwAutoMode 
        || SLQ_AUTO_MODE_SIZE == slqTime.dwAutoMode ) {
        bSetStopToMax = TRUE;
    } else if ( SLQ_AUTO_MODE_AT == slqTime.dwAutoMode ) {
        SYSTEMTIME      stLocalTime;
        FILETIME        ftLocalTime;
        LONGLONG        llLocalTime = 0;

        // get local time
        // Milliseconds set to 0 for Schedule times
        ftLocalTime.dwLowDateTime = ftLocalTime.dwHighDateTime = 0;
        GetLocalTime (&stLocalTime);
        stLocalTime.wMilliseconds = 0;
        SystemTimeToFileTime (&stLocalTime, &ftLocalTime);
        
        llLocalTime = *(LONGLONG*)&ftLocalTime;

        if ( llLocalTime >= slqTime.llDateTime ) {
            bSetStopToMax = TRUE;
        }    
    }

    if ( bSetStopToMax ) {    
        ASSERT( SLQ_TT_DTYPE_DATETIME == slqTime.wDataType );
        if ( SLQ_AUTO_MODE_SIZE != slqTime.dwAutoMode ) { 
            slqTime.dwAutoMode = SLQ_AUTO_MODE_NONE;
        }
        slqTime.llDateTime = MAX_TIME_VALUE;
        SetLogTime ( &slqTime, SLQ_TT_TTYPE_STOP );
    }

    // Service needs to distinguish between Running and Start Pending
    // at service startup, so always set state to start pending.
    SetState ( SLQ_QUERY_START_PENDING );
    
    dwStatus = UpdateServiceSchedule( bRegistryUpdated );
    
    if ( bRegistryUpdated ) { 

        while (--dwTimeout && !bStarted ) {    
            bStarted = IsRunning();
        }
    
        if ( !bStarted ) {
            dwStatus = SMCFG_START_TIMED_OUT;
        }
    }

    SyncPropPageSharedData();   // Sync the start time and stop auto mode

    return dwStatus;
}

//  
//  ManualStop function. 
//      
//      Clears the restart bit, sets the stop mode to manual and stops the query.
//
DWORD   
CSmLogQuery::ManualStop ( )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    SLQ_TIME_INFO   slqTime;
    BOOL            bRegistryUpdated;

    if ( IsAutoRestart() ) {
        mr_dwAutoRestartMode = SLQ_AUTO_MODE_NONE;
    }

    memset (&slqTime, 0, sizeof(slqTime));
    slqTime.wTimeType = SLQ_TT_TTYPE_STOP;
    slqTime.wDataType = SLQ_TT_DTYPE_DATETIME;
    slqTime.dwAutoMode = SLQ_AUTO_MODE_NONE;
    slqTime.llDateTime = MIN_TIME_VALUE;

    SetLogTime ( &slqTime, SLQ_TT_TTYPE_STOP );

    // If start time mode set to manual, set the value to MAX_TIME_VALUE
    GetLogTime ( &slqTime, SLQ_TT_TTYPE_START );
    if ( SLQ_AUTO_MODE_NONE == slqTime.dwAutoMode ) {
        ASSERT( SLQ_TT_DTYPE_DATETIME == slqTime.wDataType );
        slqTime.llDateTime = MAX_TIME_VALUE;
        SetLogTime ( &slqTime, SLQ_TT_TTYPE_START );
    }
    
    dwStatus = UpdateServiceSchedule ( bRegistryUpdated );
    
    if ( bRegistryUpdated ) { 
        DWORD   dwTimeout = 25;
        BOOL    bStopped = FALSE;
        
        while ( dwTimeout-- && !bStopped ) {
            // IsRunning implements no delay if the current state is not
            // SLQ_QUERY_START_PENDING, so add a delay for the registry
            // change to be written.
            bStopped = !IsRunning();
            Sleep ( 200 );
        }
    
        if ( !bStopped ) {
            dwStatus = SMCFG_STOP_TIMED_OUT;
        }
    }

    SyncPropPageSharedData();   // Sync the start time and stop auto mode

    return dwStatus;
}

//  
//  SaveAs function. 
//      Saves the query properties as a System Monitor ActiveX object
//      in an HTML file.
//
DWORD   
CSmLogQuery::SaveAs ( const CString& rstrPathName )
{
    DWORD dwStatus = ERROR_SUCCESS;
    CString strNonConstPathName = rstrPathName;
    ResourceStateManager rsm;

    // Create a file.
    HANDLE hFile;
    hFile =  CreateFile (
                strNonConstPathName, 
                GENERIC_READ | GENERIC_WRITE,
                0,              // Not shared
                NULL,           // Security attributes
                CREATE_ALWAYS,  // The user has already decided to override any existing file.
                FILE_ATTRIBUTE_NORMAL,
                NULL );

    if ( INVALID_HANDLE_VALUE != hFile ) {
        // Save the current configuration to the file.
        DWORD   dwTempLength;
        BOOL    bStatus;
        WCHAR   szByteOrderMark[2];

        szByteOrderMark[0] = 0xFEFF;
        szByteOrderMark[1] = 0;
        bStatus = FileWrite ( hFile, szByteOrderMark, sizeof(WCHAR) );

        if ( bStatus ) {
            dwTempLength = lstrlen ( CGlobalString::m_cszHtmlFileHeader1 ) * sizeof(WCHAR);
            bStatus = FileWrite ( hFile, (void *)CGlobalString::m_cszHtmlFileHeader1, dwTempLength );
        }

        if ( bStatus ) {
            dwTempLength = lstrlen ( CGlobalString::m_cszHtmlFileHeader2 ) * sizeof(WCHAR);
            bStatus = FileWrite ( hFile, (void *)CGlobalString::m_cszHtmlFileHeader2, dwTempLength );
        }

        if ( bStatus ) {
            DWORD dwByteCount;
            LPWSTR  pszData = NULL;

            HRESULT hr = CopyToBuffer ( pszData, dwByteCount );
            
            if ( SUCCEEDED ( hr ) ) {
                ASSERT ( NULL != pszData );
                ASSERT ( 0 != dwByteCount );
                bStatus = FileWrite ( hFile, pszData, dwByteCount );
                delete [] pszData;
            } else {
                bStatus = FALSE;
            }
        }

        if ( bStatus ) {
            dwTempLength = lstrlen ( CGlobalString::m_cszHtmlFileFooter ) * sizeof(WCHAR);
            bStatus = FileWrite ( hFile, (void *)CGlobalString::m_cszHtmlFileFooter, dwTempLength );
        }

        bStatus = CloseHandle ( hFile );

    }
    
    return dwStatus;
}

DWORD
CSmLogQuery::UpdateService( BOOL& rbRegistryUpdated ) {

    DWORD dwStatus;

    rbRegistryUpdated = FALSE;

    dwStatus = UpdateRegistry();

    if ( ERROR_SUCCESS == dwStatus ) {
        rbRegistryUpdated = TRUE;
        dwStatus = m_pLogService->Synchronize();    
    }
    
    if ( ERROR_SUCCESS == dwStatus ) {
        m_bIsModified = TRUE;
    }

    return dwStatus;
}

//
//  UpdateServiceSchedule function.
//      copies the current schedule settings to the registry
//      and synchs the log service
//
DWORD
CSmLogQuery::UpdateServiceSchedule( BOOL& rbRegistryUpdated ) {

    LONG    dwStatus = ERROR_SUCCESS;
    
    rbRegistryUpdated = FALSE;

    if (!m_bReadOnly) {

        dwStatus = UpdateRegistryScheduleValues();

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = UpdateRegistryLastModified();
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            rbRegistryUpdated = TRUE;
            dwStatus = m_pLogService->Synchronize();    
        }
    
        if ( ERROR_SUCCESS == dwStatus ) {
            m_bIsModified = TRUE;
        }
    } else {
        dwStatus = ERROR_ACCESS_DENIED;
    }
    return dwStatus;
}

//
//  UpdateRegistryLastModified function.
//      Copies the current "last modified date" to the registry where 
//      it is read by the log service
//
DWORD   
CSmLogQuery::UpdateRegistryLastModified() 
{
    LONG    dwStatus = ERROR_SUCCESS;

    if (!m_bReadOnly) {
        SLQ_TIME_INFO   plqLastModified;
        FILETIME        ftModified;
        SYSTEMTIME      stLocalTime;

        RegFlushKey( m_hKeyQuery );

        ftModified.dwLowDateTime = ftModified.dwHighDateTime = 0;

        dwStatus = RegQueryInfoKey ( 
                    m_hKeyQuery,
                    NULL,           // Class buffer
                    NULL,           // Size of class buffer
                    NULL,           // Reserved
                    NULL,           // Subkey count
                    NULL,           // Length of longest subkey name
                    NULL,           // Longest subkey class
                    NULL,           // Value count
                    NULL,           // Length of longest value name
                    NULL,           // Length of longest value 
                    NULL,           // Security descriptor
                    &ftModified );
        if (ERROR_SUCCESS != dwStatus ) {
            // Get local time for last modified value, if the
            // registry doesn't return the last written time.
            GetLocalTime (&stLocalTime);
            SystemTimeToFileTime (&stLocalTime, &ftModified);
        }

        plqLastModified.wDataType = SLQ_TT_DTYPE_DATETIME;
        plqLastModified.wTimeType = SLQ_TT_TTYPE_LAST_MODIFIED;
        plqLastModified.dwAutoMode = SLQ_AUTO_MODE_NONE;    // not used.
        plqLastModified.llDateTime = *(LONGLONG *)&ftModified;

        dwStatus = WriteRegistrySlqTime (
            m_hKeyQuery, 
            IDS_REG_LAST_MODIFIED,
            &plqLastModified);
    } else {
        dwStatus = ERROR_ACCESS_DENIED;
    }

    return dwStatus;
}

//
//  UpdateRegistryScheduleValues function.
//      copies the current schedule settings to the registry where they
//      are read by the log service
//
DWORD   
CSmLogQuery::UpdateRegistryScheduleValues() 
{
    LONG    dwStatus = ERROR_SUCCESS;
    
    if (!m_bReadOnly) {

        // Stop and start times
    
        if ( ERROR_SUCCESS == dwStatus ) {
            ASSERT (mr_stiStart.wTimeType == SLQ_TT_TTYPE_START);
            dwStatus = WriteRegistrySlqTime (
                m_hKeyQuery, 
                IDS_REG_START_TIME,
                &mr_stiStart);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            ASSERT (mr_stiStop.wTimeType == SLQ_TT_TTYPE_STOP);
            dwStatus = WriteRegistrySlqTime (
                m_hKeyQuery, 
                IDS_REG_STOP_TIME,
                &mr_stiStop);
        }

        // Auto restart value is "immediately on log file close" only.
        // Use binary for future enhancements.
        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_RESTART,
                &mr_dwAutoRestartMode,
                REG_BINARY);
        }

        // Only write the state when requesting the service to
        // start a query.
        if ( SLQ_QUERY_START_PENDING == mr_dwCurrentState ) {

            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_CURRENT_STATE,
                &mr_dwCurrentState);
        }

    } else {
        dwStatus = ERROR_ACCESS_DENIED;
    }

    return dwStatus;
}
//
//  UpdateRegistry function.
//      copies the current settings to the registry where they
//      are read by the log service
//
DWORD   
CSmLogQuery::UpdateRegistry() 
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwBufferSize = 0;
    DWORD   dwLogType;
	DWORD	dwLogFileType = 0;
    DWORD   dwTempFileSizeUnits;
    DWORD   dwTempDataStoreAttributes = 0;
    DWORD   dwTempMaxFileSize;
    DWORD   dwTempAppendMode;

    if ( IsModifiable() ) {
                
        if ( !mr_strComment.IsEmpty() ) {
            dwBufferSize = mr_strComment.GetLength() + 1;
            dwBufferSize *= sizeof(WCHAR);
        } else {
            dwBufferSize = 0;
        }

        dwStatus = WriteRegistryStringValue (
            m_hKeyQuery,
            IDS_REG_COMMENT,
            REG_SZ,
            (LPCWSTR)mr_strComment,
            &dwBufferSize);

        if ( ERROR_SUCCESS == dwStatus && !mr_strCommentIndirect.IsEmpty() ) {
            dwBufferSize = mr_strCommentIndirect.GetLength() + 1;
            dwBufferSize *= sizeof(WCHAR);

            dwStatus = WriteRegistryStringValue (
                m_hKeyQuery,
                IDS_REG_COMMENT_INDIRECT,
                REG_SZ,
                (LPCWSTR)mr_strCommentIndirect,
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwLogType = GetLogType();
            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_LOG_TYPE,
                &dwLogType);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_REALTIME_DATASOURCE,
                & mr_dwRealTimeQuery);
        }
        
        // Files

        if ( ERROR_SUCCESS == dwStatus ) {
            // Within the app, counter data store size units are in MB.
            // Translate to back to KB when write to registry
            dwTempFileSizeUnits = GetDataStoreSizeUnits();
            dwTempMaxFileSize = mr_dwMaxSize;
			GetLogFileType ( dwLogFileType );
            if ( SLQ_COUNTER_LOG == GetLogType()
                && SLF_SQL_LOG != dwLogFileType ) 
            {
                if ( ONE_MB == dwTempFileSizeUnits ) { 
                    dwTempFileSizeUnits = ONE_KB;
                    // Round up to next MB
                    if ( SLQ_DISK_MAX_SIZE != mr_dwMaxSize ) {
                        dwTempMaxFileSize *= dwTempFileSizeUnits;
                    }
                }
            }

            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_LOG_FILE_MAX_SIZE,
                &dwTempMaxFileSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            // Data store size units
            if ( ONE_MB == dwTempFileSizeUnits ) {
                dwTempDataStoreAttributes = SLF_DATA_STORE_SIZE_ONE_MB;
            } else if ( ONE_KB == dwTempFileSizeUnits ) {
                dwTempDataStoreAttributes = SLF_DATA_STORE_SIZE_ONE_KB;
            } else if ( ONE_RECORD == dwTempFileSizeUnits ) {
                dwTempDataStoreAttributes = SLF_DATA_STORE_SIZE_ONE_RECORD;
            }

            // Data store append mode
            GetDataStoreAppendMode( dwTempAppendMode );
            dwTempDataStoreAttributes |= dwTempAppendMode;

            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_DATA_STORE_ATTRIBUTES,
                &dwTempDataStoreAttributes);
        }

        if ( !mr_strBaseFileName.IsEmpty() ) {
            dwBufferSize = mr_strBaseFileName.GetLength() + 1;
            dwBufferSize *= sizeof(WCHAR);
        } else {
            dwBufferSize = 0;
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery, 
                IDS_REG_LOG_FILE_BASE_NAME,
                REG_SZ,
                (LPCWSTR)mr_strBaseFileName, 
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus && !mr_strBaseFileNameIndirect.IsEmpty() ) {
            dwBufferSize = mr_strBaseFileNameIndirect.GetLength() + 1;
            dwBufferSize *= sizeof(WCHAR);

            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery, 
                IDS_REG_LOG_FILE_BASE_NAME_IND,
                REG_SZ,
                (LPCWSTR)mr_strBaseFileNameIndirect, 
                &dwBufferSize);
        }

        if ( !mr_strSqlName.IsEmpty() ) {
            dwBufferSize = mr_strSqlName.GetLength() + 1;
            dwBufferSize *= sizeof(WCHAR);
        } else {
            dwBufferSize = 0;
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery, 
                IDS_REG_SQL_LOG_BASE_NAME,
                REG_SZ,
                (LPCWSTR)mr_strSqlName, 
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = WriteRegistryDwordValue (
                        m_hKeyQuery, 
                        IDS_REG_LOG_FILE_SERIAL_NUMBER,
                        &mr_dwCurrentSerialNumber );
        }

        if ( !mr_strDefaultDirectory.IsEmpty() ) {
            dwBufferSize = mr_strDefaultDirectory.GetLength() + 1;
            dwBufferSize *= sizeof(WCHAR);
        } else {
            dwBufferSize = 0;
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery,
                IDS_REG_LOG_FILE_FOLDER,
                REG_SZ,
                (LPCWSTR)mr_strDefaultDirectory,
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus  = WriteRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_LOG_FILE_AUTO_FORMAT,
                &mr_dwLogAutoFormat);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus  = WriteRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_LOG_FILE_TYPE,
                &mr_dwLogFileType);
        }

        // Schedule

        // Eof command is used for counter and trace logs only.
        if ( ERROR_SUCCESS == dwStatus ) {
            if ( SLQ_COUNTER_LOG == GetLogType()
                 || SLQ_TRACE_LOG == GetLogType() ) {
                dwBufferSize = mr_strEofCmdFile.GetLength() + 1;
                dwBufferSize *= sizeof(WCHAR);
                dwStatus  = WriteRegistryStringValue (
                    m_hKeyQuery,
                    IDS_REG_EOF_COMMAND_FILE,
                    REG_SZ,
                    (LPCWSTR)mr_strEofCmdFile,
                    &dwBufferSize);
            }
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = UpdateRegistryScheduleValues();
        }

        // This must be the last registry value updated.
        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = UpdateRegistryLastModified();
        }
    } else {
        dwStatus = ERROR_ACCESS_DENIED;
    }

    return dwStatus;
}

//
//  SyncSerialNumberWithRegistry()
//      reads the current value for the serial number 
//      from the registry and reloads the internal value 
//      to match
//  
DWORD   
CSmLogQuery::SyncSerialNumberWithRegistry()
{
    DWORD   dwStatus = ERROR_SUCCESS;

    ASSERT (m_hKeyQuery != NULL);

    // Get starting serial number for serial suffix.

    dwStatus = ReadRegistryDwordValue (
                m_hKeyQuery, 
                IDS_REG_LOG_FILE_SERIAL_NUMBER,
                DEFAULT_LOG_FILE_SERIAL_NUMBER, 
                &mr_dwCurrentSerialNumber );
    ASSERT (dwStatus == ERROR_SUCCESS);

    return dwStatus;
}

//
//  SyncWithRegistry()
//      reads the current values for this query from the registry
//      and reloads the internal values to match
//  
DWORD   
CSmLogQuery::SyncWithRegistry()
{
    DWORD   dwBufferSize = 0;
    DWORD   dwStatus = ERROR_SUCCESS;
    SLQ_TIME_INFO   stiDefault;
    WCHAR   szDefault[MAX_PATH + 1];
    LPWSTR  szTemp = NULL;
    DWORD   dwDefault;
    DWORD   dwTemp;
    LPWSTR  szIndTemp = NULL;
    UINT    uiBufferLen = 0;
    LPWSTR  szEofCmd = NULL;

    ResourceStateManager   rsm;

    ASSERT (m_hKeyQuery != NULL);
    
    MFC_TRY
        // Modify bit
        dwTemp = DEFAULT_EXECUTE_ONLY;
        dwStatus = ReadRegistryDwordValue (
                    m_hKeyQuery, 
                    IDS_REG_EXECUTE_ONLY,
                    DEFAULT_EXECUTE_ONLY, 
                    &dwTemp);
        ASSERT ( ERROR_SUCCESS == dwStatus );

        if ( 0 == dwTemp ) {
            m_bExecuteOnly = FALSE;
        } else {
            m_bExecuteOnly = TRUE;
        }

        // File attributes
    
        // Comment field can be indirect

        dwStatus = SmNoLocReadRegIndStrVal (
            m_hKeyQuery,
            IDS_REG_COMMENT, 
            DEFAULT_COMMENT,
            &szIndTemp,
            &uiBufferLen );

        mr_strComment.Empty();

        if ( NULL != szIndTemp ) {
            if ( L'\0' != *szIndTemp ) {
                mr_strComment = szIndTemp;
            }
        }

        if ( NULL != szIndTemp ) {
            G_FREE ( szIndTemp );
            szIndTemp = NULL;
        }
        uiBufferLen = 0;

        dwStatus = ReadRegistryDwordValue (
                    m_hKeyQuery, 
                    IDS_REG_LOG_FILE_MAX_SIZE,
                    DEFAULT_LOG_FILE_MAX_SIZE, 
                    &mr_dwMaxSize );

        ASSERT ( ERROR_SUCCESS == dwStatus );

        if ( SLQ_TRACE_LOG == GetLogType() ) {

            dwStatus = ReadRegistryDwordValue (
                        m_hKeyQuery, 
                        IDS_REG_LOG_FILE_TYPE,
                        DEFAULT_TRACE_LOG_FILE_TYPE, 
                        &mr_dwLogFileType);
            ASSERT ( ERROR_SUCCESS == dwStatus );

        } else {

            dwStatus = ReadRegistryDwordValue (
                        m_hKeyQuery, 
                        IDS_REG_LOG_FILE_TYPE,
                        DEFAULT_CTR_LOG_FILE_TYPE, 
                        &mr_dwLogFileType);
            ASSERT ( ERROR_SUCCESS == dwStatus );

        }

        // Data store attributes must be read after log file type and log file max size.

        dwDefault = 0;  // Eliminate PREFIX warning
        InitDataStoreAttributesDefault ( mr_dwLogFileType, dwDefault );

        dwStatus = ReadRegistryDwordValue (
                    m_hKeyQuery,
                    IDS_REG_DATA_STORE_ATTRIBUTES,
                    dwDefault, 
                    &dwTemp );
        
        ProcessLoadedDataStoreAttributes ( dwTemp );

        // Log file base name field can be indirect
        StringCchCopy ( ( LPWSTR)szDefault, MAX_PATH+1, m_strName );

        dwStatus = SmNoLocReadRegIndStrVal (
            m_hKeyQuery,
            IDS_REG_LOG_FILE_BASE_NAME, 
            szDefault,              // Default to query name
            &szIndTemp,
            &uiBufferLen );

        ASSERT ( ERROR_SUCCESS == dwStatus );

        mr_strBaseFileName.Empty();
        
        if ( NULL != szIndTemp ) {
            if ( L'\0' != *szIndTemp ) {
                ReplaceBlanksWithUnderscores ( szIndTemp );
                mr_strBaseFileName = szIndTemp;
            }
        }

        if ( NULL != szIndTemp ) {
            G_FREE ( szIndTemp );
            szIndTemp = NULL;
        }
        uiBufferLen = 0;

        dwStatus = ReadRegistryStringValue (
                    m_hKeyQuery,
                    IDS_REG_LOG_FILE_FOLDER,
                    m_pLogService->GetDefaultLogFileFolder(),
                    &szTemp,
                    &dwBufferSize);
        ASSERT ( ERROR_SUCCESS == dwStatus );
        mr_strDefaultDirectory.Empty();
        if ( NULL != szTemp ) {
            if ( dwBufferSize > sizeof(WCHAR) ) {
                if ( L'\0' != szTemp[0] ) {
                    mr_strDefaultDirectory = szTemp;
                }
            }

            delete [] szTemp;
            szTemp = NULL;
            dwBufferSize = 0;
        }

        // Default log set name to log name

        StringCchPrintf (
            szDefault, 
            MAX_PATH+1,
            L"SQL:!%s",
            m_strName.GetBuffer(0));

        dwStatus = ReadRegistryStringValue (
                    m_hKeyQuery,
                    IDS_REG_SQL_LOG_BASE_NAME,
                    szDefault,             
                    &szTemp,
                    &dwBufferSize);
        ASSERT ( ERROR_SUCCESS == dwStatus );
        mr_strSqlName.Empty();
        if ( dwBufferSize > sizeof(WCHAR) ) {
            ASSERT ( NULL != szTemp );
            ASSERT ( 0 != *szTemp );
            mr_strSqlName = szTemp;
        }

        delete [] szTemp;
        szTemp = NULL;
        dwBufferSize = 0;

        dwStatus = ReadRegistryDwordValue(
                    m_hKeyQuery, 
                    IDS_REG_REALTIME_DATASOURCE,
                    g_dwRealTimeQuery, 
                    & mr_dwRealTimeQuery);
        ASSERT ( ERROR_SUCCESS == dwStatus );

        dwStatus = ReadRegistryDwordValue (
                    m_hKeyQuery, 
                    IDS_REG_LOG_FILE_AUTO_FORMAT,
                    DEFAULT_LOG_FILE_AUTO_FORMAT, 
                    &mr_dwLogAutoFormat);
        ASSERT ( ERROR_SUCCESS == dwStatus );

        // Get starting serial number for serial suffix.
        dwStatus = SyncSerialNumberWithRegistry ();
        ASSERT ( ERROR_SUCCESS == dwStatus );

        dwStatus = ReadRegistryDwordValue (
                    m_hKeyQuery, 
                    IDS_REG_CURRENT_STATE,
                    DEFAULT_CURRENT_STATE, 
                    &mr_dwCurrentState);
        ASSERT ( ERROR_SUCCESS == dwStatus );

        //  Start, stop and restart values

        VERIFY ( GetDefaultLogTime (stiDefault, SLQ_TT_TTYPE_START ) );

        dwStatus = ReadRegistrySlqTime (
            m_hKeyQuery, IDS_REG_START_TIME,
            &stiDefault, &mr_stiStart);
        ASSERT ( ERROR_SUCCESS == dwStatus );
        ASSERT (mr_stiStart.wTimeType == SLQ_TT_TTYPE_START);

        VERIFY ( GetDefaultLogTime (stiDefault, SLQ_TT_TTYPE_STOP ) );

        dwStatus = ReadRegistrySlqTime (
            m_hKeyQuery, IDS_REG_STOP_TIME,
            &stiDefault, &mr_stiStop);
        ASSERT ( ERROR_SUCCESS == dwStatus );
        ASSERT (mr_stiStop.wTimeType == SLQ_TT_TTYPE_STOP);

        dwStatus = ReadRegistryDwordValue (
                    m_hKeyQuery, 
                    IDS_REG_RESTART,
                    DEFAULT_RESTART_VALUE, 
                    &mr_dwAutoRestartMode);
        ASSERT ( ERROR_SUCCESS == dwStatus );

        // Eof command is used by Counter and Trace logs only.
        if ( SLQ_COUNTER_LOG == GetLogType()
             || SLQ_TRACE_LOG == GetLogType() ) {

            dwStatus = ReadRegistryStringValue (
                m_hKeyQuery,
                IDS_REG_EOF_COMMAND_FILE,
                DEFAULT_EOF_COMMAND_FILE,
                &szEofCmd,
                &dwBufferSize);
            ASSERT ( ERROR_SUCCESS == dwStatus );
            if (dwBufferSize > sizeof(WCHAR)) {
                mr_strEofCmdFile = szEofCmd;
            } else {
                mr_strEofCmdFile.Empty();
            }
        }

    MFC_CATCH_DWSTATUS;
    if ( NULL != szTemp ) {
        delete [] szTemp;
    }

    if ( NULL != szIndTemp ) {
        G_FREE ( szIndTemp );
    }

    if ( NULL != szEofCmd ) {
        delete [] szEofCmd;
    }
    SyncPropPageSharedData();

    return dwStatus;
}
    
CSmLogService*    
CSmLogQuery::GetLogService ( void )
{
    return m_pLogService;
}

DWORD
CSmLogQuery::GetMachineDisplayName ( CString& rstrMachineName )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    // rstrMachineName is writable.  CString copy-on-write 
    // semantics will support a writable string created from read-only.
    // A new string data buffer is allocated the first time that
    // it is modified.

    MFC_TRY
        rstrMachineName = m_pLogService->GetMachineDisplayName();
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

//
//  Get log file type and return as a string
//
//
const CString&
CSmLogQuery::GetLogFileType( void )
{
    int     nStringIdx;

    ResourceStateManager    rsm;

    m_strLogFileType.Empty();

    switch (LOWORD(mr_dwLogFileType)) {
        case SLF_CSV_FILE:
            nStringIdx = IDS_FT_CSV;
            break;
            
        case SLF_TSV_FILE:
            nStringIdx = IDS_FT_TSV;
            break;
            
        case SLF_BIN_FILE:
            nStringIdx = IDS_FT_BINARY;
            break;

        case SLF_BIN_CIRC_FILE:
            nStringIdx = IDS_FT_BINARY_CIRCULAR;
            break;

        case SLF_SEQ_TRACE_FILE:
            nStringIdx = IDS_FT_SEQUENTIAL_TRACE;
            break;

        case SLF_CIRC_TRACE_FILE:
            nStringIdx = IDS_FT_CIRCULAR_TRACE;
            break;

        case SLF_SQL_LOG:
            nStringIdx = IDS_FT_SQL;
            break;

        default:
            nStringIdx = IDS_FT_UNKNOWN;
            break;
    }

    MFC_TRY
        m_strLogFileType.LoadString ( nStringIdx );
    MFC_CATCH_MINIMUM
    
    return m_strLogFileType;
}

void
CSmLogQuery::GetLogFileType ( DWORD& rdwFileType )
{
    rdwFileType = LOWORD(mr_dwLogFileType);
    return;
}

void
CSmLogQuery::GetDataStoreAppendMode(DWORD &rdwAppend)
{
    rdwAppend = mr_dwAppendMode;

    return;
}

void
CSmLogQuery::SetDataStoreAppendMode(DWORD dwAppend)
{
    mr_dwAppendMode = dwAppend;

    return;
}
//
//  Get log file name currently in use
//
//
const CString&
CSmLogQuery::GetLogFileName ( BOOL bLatestRunning )
{
    HRESULT hr = NOERROR;
    PPDH_PLA_INFO  pInfo = NULL;
    DWORD dwStrBufLen = 0;
    DWORD dwFlags = 0;
    CString strMachineName;

    m_strFileName.Empty();

    MFC_TRY
        strMachineName = m_pLogService->GetMachineName();
    MFC_CATCH_HR;

    // Todo:  dwStatus or hr?

    if ( SUCCEEDED ( hr ) ) {
        if ( bLatestRunning ) {
            dwFlags = PLA_FILENAME_CURRENTLOG;  // Latest running log
        }

        hr = PdhPlaGetLogFileName (
                (LPWSTR)(LPCWSTR)GetLogName(),
                (LPWSTR)(LPCWSTR)strMachineName,
                NULL,  
                dwFlags,
                &dwStrBufLen,
                NULL );

        if ( SUCCEEDED ( hr ) || PDH_INSUFFICIENT_BUFFER == (PDH_STATUS)hr ) {
            if ( bLatestRunning ) {
                dwFlags = PLA_FILENAME_CURRENTLOG;  // Latest running log
            }
            hr = PdhPlaGetLogFileName (
                    (LPWSTR)(LPCWSTR)GetLogName(),
                    (LPWSTR)(LPCWSTR)strMachineName,
                    NULL,
                    dwFlags,
                    &dwStrBufLen,
                    m_strFileName.GetBufferSetLength ( dwStrBufLen ) );
            m_strFileName.ReleaseBuffer();
        }
    }

    SetLastError ( hr );

    return m_strFileName;
}

//
//  Get log file name currently in use
//
//
DWORD
CSmLogQuery::GetLogFileName ( CString& rstrLogFileName )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    MFC_TRY
        rstrLogFileName = GetLogFileName();
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD 
CSmLogQuery::GetLogType ( void )
{
    // Subclass must override
    ASSERT ( FALSE ); 

    return ((DWORD)-1);
}

BOOL    
CSmLogQuery::SetLogFileType ( const DWORD dwType )
{
    DWORD dwLogFileType = LOWORD(dwType);
    if (dwLogFileType < (SLF_FIRST_FILE_TYPE + SLF_NUM_FILE_TYPES)) {
        mr_dwLogFileType = dwLogFileType;
        return TRUE;
    } else {
        return FALSE;
    }
}

const CString&
CSmLogQuery::GetLogName()
{
    return m_strName;
}

DWORD
CSmLogQuery::GetLogName ( CString& rstrLogName )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    MFC_TRY
        rstrLogName = GetLogName();
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD    
CSmLogQuery::SetLogName ( const CString& rstrLogName )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        m_strName = rstrLogName;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

const CString&
CSmLogQuery::GetLogKeyName()
{
    return mr_strLogKeyName;
}

DWORD
CSmLogQuery::GetLogKeyName ( CString& rstrLogKeyName )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    MFC_TRY
        rstrLogKeyName = GetLogKeyName();
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD    
CSmLogQuery::SetLogKeyName ( const CString& rstrLogKeyName )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        mr_strLogKeyName = rstrLogKeyName;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD    
CSmLogQuery::GetEofCommand ( CString& rstrCmdString)
{
    DWORD   dwStatus = ERROR_SUCCESS;

    MFC_TRY
        rstrCmdString = mr_strEofCmdFile;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}
                
DWORD  
CSmLogQuery::SetEofCommand ( const CString& rstrCmdString )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        mr_strEofCmdFile = rstrCmdString;
    MFC_CATCH_DWSTATUS
    
    return dwStatus;
}

const CString& 
CSmLogQuery::GetLogComment()
{
    return mr_strComment;
}

DWORD
CSmLogQuery::GetLogComment ( CString& rstrLogComment )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    MFC_TRY
        rstrLogComment = GetLogComment();
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD    
CSmLogQuery::SetLogComment ( const CString& rstrComment )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        mr_strComment = rstrComment;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD    
CSmLogQuery::SetLogCommentIndirect ( const CString& rstrComment )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        mr_strCommentIndirect = rstrComment;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD   
CSmLogQuery::SetLogFileName ( const CString& rstrFileName )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        mr_strBaseFileName = rstrFileName;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD   
CSmLogQuery::SetLogFileNameIndirect ( const CString& rstrFileName )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        mr_strBaseFileNameIndirect = rstrFileName;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD   
CSmLogQuery::SetFileNameParts ( const CString& rstrFolder, const CString& rstrName )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        mr_strDefaultDirectory = rstrFolder;
        mr_strBaseFileName = rstrName;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD   
CSmLogQuery::GetMaxSize()
{
    return mr_dwMaxSize;
}

BOOL    
CSmLogQuery::SetMaxSize ( const DWORD dwMaxSize )
{
    mr_dwMaxSize = dwMaxSize;
    return TRUE;
}

HKEY   
CSmLogQuery::GetQueryKey ( void )
{
    return m_hKeyQuery;
}

const CString&
CSmLogQuery::GetSqlName()
{
    return mr_strSqlName;
}

DWORD
CSmLogQuery::GetSqlName ( CString& rstrSqlName )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    MFC_TRY
        rstrSqlName = GetSqlName();
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD    
CSmLogQuery::SetSqlName ( const CString& rstrSqlName )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        mr_strSqlName = rstrSqlName;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

//
//  return: 1 if the log is currently active or
//          0 if the log is not running
//
BOOL    
CSmLogQuery::IsRunning()
{
    DWORD   dwCurrentState = SLQ_QUERY_START_PENDING;
    DWORD   dwTimeout = 20;
    
    while (--dwTimeout) {
        dwCurrentState = GetState();
        if ( SLQ_QUERY_START_PENDING == dwCurrentState ) {
            Sleep(100);
        } else {
            break;
        }
    }
        
    return ( SLQ_QUERY_RUNNING == dwCurrentState );
}

BOOL    
CSmLogQuery::IsAutoStart()
{
    return ( SLQ_AUTO_MODE_NONE != mr_stiStart.dwAutoMode );
}

BOOL    
CSmLogQuery::IsAutoRestart()
{
    return ( SLQ_AUTO_MODE_AFTER == mr_dwAutoRestartMode );
}

DWORD   
CSmLogQuery::GetFileNameParts( CString& rstrFolder, CString& rstrName)
{
    DWORD dwStatus = ERROR_SUCCESS;

    MFC_TRY
        rstrFolder = mr_strDefaultDirectory;
        rstrName = mr_strBaseFileName;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD   
CSmLogQuery::GetFileNameAutoFormat()
{
    return mr_dwLogAutoFormat;
}

BOOL    
CSmLogQuery::SetFileNameAutoFormat ( const DWORD dwFileSuffix )
{
    if ((dwFileSuffix < ( SLF_NAME_FIRST_AUTO + SLF_NUM_AUTO_NAME_TYPES)) ||
        (dwFileSuffix == SLF_NAME_NONE)) {
        mr_dwLogAutoFormat = dwFileSuffix;
        return TRUE;
    }

    return FALSE;
}

DWORD   
CSmLogQuery::GetFileSerialNumber( void )
{
    SyncSerialNumberWithRegistry();
    return mr_dwCurrentSerialNumber;
}

BOOL    
CSmLogQuery::SetFileSerialNumber ( const DWORD dwSerial )
{
    mr_dwCurrentSerialNumber = dwSerial;
    return TRUE;
}


DWORD   
CSmLogQuery::GetState()
{
    DWORD dwCurrentState = SLQ_QUERY_STOPPED;

    // If the service is running, get the value from the registry.
    if ( m_pLogService->IsRunning() ) {

        DWORD dwStatus;
        dwStatus = ReadRegistryDwordValue (
                    m_hKeyQuery, 
                    IDS_REG_CURRENT_STATE,
                    SLQ_QUERY_STOPPED, 
                    &mr_dwCurrentState);
        ASSERT (dwStatus == ERROR_SUCCESS);
        dwCurrentState = mr_dwCurrentState;

    }

    return dwCurrentState;
}
  
      
BOOL    
CSmLogQuery::SetState ( const DWORD dwNewState )
{
    // Only use this to set the start state.  This is necessary
    // so that the service, at service startup, can differentiate
    // between previously running queries and newly requested query starts.
    ASSERT ( SLQ_QUERY_START_PENDING == dwNewState );

    // Set the local variable if it is different.
    if ( mr_dwCurrentState != dwNewState ) {
        mr_dwCurrentState = dwNewState;
    }
    return TRUE;
}


BOOL    
CSmLogQuery::GetLogTime(PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags)
{
    switch (dwFlags) {
        case SLQ_TT_TTYPE_START:
            *pTimeInfo = mr_stiStart;
            return TRUE;

        case SLQ_TT_TTYPE_STOP:
            *pTimeInfo = mr_stiStop;
            return TRUE;

        case SLQ_TT_TTYPE_RESTART:
            pTimeInfo->wTimeType = SLQ_TT_TTYPE_RESTART;
            pTimeInfo->dwAutoMode = mr_dwAutoRestartMode;
            pTimeInfo->wDataType = SLQ_TT_DTYPE_UNITS;      // not used
            pTimeInfo->dwUnitType = SLQ_TT_UTYPE_MINUTES;   // not used
            pTimeInfo->dwValue = 0;                         // not used
            return TRUE;
            
        case SLQ_TT_TTYPE_SAMPLE:
            *pTimeInfo = mr_stiSampleInterval;
            return TRUE;

        default:
            return FALSE;
    }
}
        
BOOL    
CSmLogQuery::SetLogTime(PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags)
{
    ASSERT (pTimeInfo->wTimeType == dwFlags);

    switch (dwFlags) {
        case SLQ_TT_TTYPE_START:
            mr_stiStart = *pTimeInfo ;
            return TRUE;

        case SLQ_TT_TTYPE_STOP:
            mr_stiStop = *pTimeInfo;
            return TRUE;

        case SLQ_TT_TTYPE_RESTART:
            mr_dwAutoRestartMode = pTimeInfo->dwAutoMode;
            return TRUE;

        case SLQ_TT_TTYPE_SAMPLE:
            mr_stiSampleInterval = *pTimeInfo;
            return TRUE;
            
        default:
            return FALSE;
    }
}

BOOL    
CSmLogQuery::GetDefaultLogTime(SLQ_TIME_INFO& /*rTimeInfo*/, DWORD /*dwFlags*/)
{
    // Subclass must override
    ASSERT( FALSE );
    return FALSE;
}

void    
CSmLogQuery::SyncPropPageSharedData ( void )
{
    // Sync the data shared between property pages
    // from registry values.
    MFC_TRY
        m_PropData.dwMaxFileSize = mr_dwMaxSize;
        m_PropData.dwLogFileType = LOWORD(mr_dwLogFileType);
        m_PropData.strFolderName = mr_strDefaultDirectory;

        m_PropData.strFileBaseName  = mr_strBaseFileName;
        m_PropData.strSqlName       = mr_strSqlName;
        m_PropData.dwSuffix         = mr_dwLogAutoFormat;
        SyncSerialNumberWithRegistry();
        m_PropData.dwSerialNumber   = mr_dwCurrentSerialNumber;
        m_PropData.stiStartTime     = mr_stiStart;
        m_PropData.stiStopTime      = mr_stiStop;
        m_PropData.stiSampleTime    = mr_stiSampleInterval;
    MFC_CATCH_MINIMUM
    // Todo:  Return and use status
}

void    
CSmLogQuery::UpdatePropPageSharedData ( void )
{
    // Update the registry values for data shared between property pages.
    // Note: This is called by the property page OnApply code.  It is assumed
    // that OnApply is called for all property pages, so the shared data is valid.

    // This method handles the problem where default Start mode and time was
    // written to the registry by the counter page OnApply before the 
    // schedule page OnApply modified the value.
    MFC_TRY
        mr_dwMaxSize                = m_PropData.dwMaxFileSize;   
        mr_dwLogFileType            = m_PropData.dwLogFileType; 

        mr_dwLogAutoFormat          = m_PropData.dwSuffix;       
        mr_dwCurrentSerialNumber    = m_PropData.dwSerialNumber; 
        mr_stiStart                 = m_PropData.stiStartTime;   
        mr_stiStop                  = m_PropData.stiStopTime;   
        mr_stiSampleInterval        = m_PropData.stiSampleTime;   

        mr_strBaseFileName          = m_PropData.strFileBaseName;
        mr_strDefaultDirectory      = m_PropData.strFolderName;
        mr_strSqlName               = m_PropData.strSqlName;
    MFC_CATCH_MINIMUM
    // Todo:  Return and use status
}

BOOL    
CSmLogQuery::GetPropPageSharedData (PSLQ_PROP_PAGE_SHARED pData)
{
    BOOL bReturn = FALSE;

    if ( NULL != pData ) {
        MFC_TRY
            pData->dwLogFileType    = m_PropData.dwLogFileType;
            pData->dwMaxFileSize    = m_PropData.dwMaxFileSize;
            pData->strFileBaseName  = m_PropData.strFileBaseName;
            pData->strFolderName    = m_PropData.strFolderName;
            pData->strSqlName       = m_PropData.strSqlName;
            pData->dwLogFileType    = m_PropData.dwLogFileType;
            pData->dwSuffix         = m_PropData.dwSuffix;
            pData->dwSerialNumber   = m_PropData.dwSerialNumber;
            pData->stiStartTime     = m_PropData.stiStartTime;
            pData->stiStopTime      = m_PropData.stiStopTime;
            pData->stiSampleTime    = m_PropData.stiSampleTime;
            bReturn = TRUE;
        MFC_CATCH_MINIMUM
    } 
    // Todo:  Return and use status
    return bReturn;
}

BOOL    
CSmLogQuery::SetPropPageSharedData (PSLQ_PROP_PAGE_SHARED pData)
{
    BOOL bReturn = FALSE;

    if ( NULL != pData ) {
        MFC_TRY
            m_PropData.dwLogFileType    = pData->dwLogFileType;
            m_PropData.dwMaxFileSize    = pData->dwMaxFileSize;
            m_PropData.strFileBaseName  = pData->strFileBaseName;
            m_PropData.strFolderName    = pData->strFolderName;
            m_PropData.strSqlName       = pData->strSqlName;
            m_PropData.dwLogFileType    = pData->dwLogFileType;
            m_PropData.dwSuffix         = pData->dwSuffix;
            m_PropData.dwSerialNumber   = pData->dwSerialNumber;
            m_PropData.stiStartTime     = pData->stiStartTime;
            m_PropData.stiStopTime      = pData->stiStopTime;
            m_PropData.stiSampleTime    = pData->stiSampleTime;
        MFC_CATCH_MINIMUM
    } 
    // Todo:  Return and use status
    return bReturn;
}

void
CSmLogQuery::InitDataStoreAttributesDefault (
    const	DWORD   dwRegLogFileType,
			DWORD&  rdwDefault )
{
    DWORD   dwBeta1AppendFlags;
	DWORD   dwLogFileType;

    // Append vs. Overwrite
    // Win2000 files defaulted to OVERWRITE. 
    // The Append mode flags did not exist.
    // Convert the settings to use new flags.
    // Whistler Beta 1 append mode was stored in high word of log file type

    dwBeta1AppendFlags = dwRegLogFileType & 0x00FF0000;
    rdwDefault = 0;

	GetLogFileType ( dwLogFileType );

    if ( SLF_FILE_APPEND == dwBeta1AppendFlags ) {
        mr_dwAppendMode = SLF_DATA_STORE_APPEND;
        rdwDefault = SLF_DATA_STORE_APPEND;
    } else if ( SLF_FILE_OVERWRITE == dwBeta1AppendFlags ) {
        mr_dwAppendMode = SLF_DATA_STORE_OVERWRITE;
        rdwDefault = SLF_DATA_STORE_OVERWRITE;
    } else if ( 0 == dwBeta1AppendFlags ) {
        if ( SLF_SQL_LOG == dwLogFileType ) {
            mr_dwAppendMode = SLF_DATA_STORE_APPEND;
        } else {
            // Default for Win2K is overwrite.
            // For Whistler, mode is stored in Data Store Attributes
            mr_dwAppendMode = SLF_DATA_STORE_OVERWRITE;
        }
    }

    // Append vs. overwrite flag

    if ( 0 == rdwDefault ) {
        if ( SLF_BIN_FILE == dwLogFileType
                || SLF_SEQ_TRACE_FILE == dwLogFileType
                || SLF_SQL_LOG == dwLogFileType )
        {
            rdwDefault = SLF_DATA_STORE_APPEND;
        } else {
            rdwDefault = SLF_DATA_STORE_OVERWRITE;
        }
    }

    // File size units flag

    if ( SLQ_COUNTER_LOG == GetLogType() ) {
        if ( SLF_SQL_LOG != dwLogFileType ) {
            rdwDefault |= SLF_DATA_STORE_SIZE_ONE_KB;
        } else {
            rdwDefault |= SLF_DATA_STORE_SIZE_ONE_RECORD;
        }
    } else if ( SLQ_TRACE_LOG == GetLogType() ){
        rdwDefault |= SLF_DATA_STORE_SIZE_ONE_MB;
    }
}

void
CSmLogQuery::ProcessLoadedDataStoreAttributes (
    DWORD   dwDataStoreAttributes )
{
	DWORD	dwLogFileType = 0;
    
    if ( dwDataStoreAttributes & SLF_DATA_STORE_SIZE_ONE_MB ) {
        mr_dwFileSizeUnits = ONE_MB;
    } else if ( dwDataStoreAttributes & SLF_DATA_STORE_SIZE_ONE_KB ) {
        mr_dwFileSizeUnits = ONE_KB;
    } else if ( dwDataStoreAttributes & SLF_DATA_STORE_SIZE_ONE_RECORD ) {
        mr_dwFileSizeUnits = ONE_RECORD;
    }
    // Within the app, counter data store size units are in MB.
    // Translate to MB here, back to KB when write to registry.
	GetLogFileType( dwLogFileType );

    if ( SLQ_COUNTER_LOG == GetLogType()
        && SLF_SQL_LOG != dwLogFileType ) 
    {
        ASSERT ( ONE_KB == GetDataStoreSizeUnits() );
        if ( ONE_KB == GetDataStoreSizeUnits() ) { 
            mr_dwFileSizeUnits = ONE_MB;
            if ( SLQ_DISK_MAX_SIZE != mr_dwMaxSize ) {
                // Round up to next MB
                mr_dwMaxSize = ( mr_dwMaxSize + (ONE_KB - 1) ) / ONE_KB;
            }
        }
    }

    // Data store append mode

    ASSERT ( dwDataStoreAttributes & SLF_DATA_STORE_APPEND_MASK );
    // Todo:  Does defalt value setting overwrite the Whistler Beta 1 setting?

    if ( dwDataStoreAttributes & SLF_DATA_STORE_APPEND ) {
        mr_dwAppendMode = SLF_DATA_STORE_APPEND;
    } else if ( dwDataStoreAttributes & SLF_DATA_STORE_OVERWRITE ) {
        mr_dwAppendMode = SLF_DATA_STORE_OVERWRITE;
    }
}

HRESULT 
CSmLogQuery::LoadFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT hr = S_OK;
    DWORD   cchBufLen;    
    SLQ_TIME_INFO   stiDefault;
    LPWSTR  pszTemp = NULL;
    DWORD   dwTemp;
    DWORD   dwDefault;
    WCHAR   szDefault[MAX_PATH + 1];
    LPWSTR  szEofCmd = NULL;

    // Subclass must call this method at the end of their override, to sync the 
    // property page shared data.

    // Continue even if error, using defaults for missing values.
    mr_strComment.Empty();
    cchBufLen = 0;
    hr = StringFromPropertyBag ( 
            pPropBag, 
            pIErrorLog, 
            IDS_HTML_COMMENT, 
            DEFAULT_COMMENT, 
            &pszTemp, 
            &cchBufLen );
    // 1 for Null
    if ( NULL != pszTemp && cchBufLen > 1 ) {
        if ( L'\0'!= pszTemp[0] ) {
            mr_strComment = pszTemp;
        }
        delete [] pszTemp;
        pszTemp = NULL;
    }

    StringCchCopy ( ( LPWSTR)szDefault, MAX_PATH+1, m_strName );
    ReplaceBlanksWithUnderscores ( szDefault );
    
    mr_strBaseFileName.Empty();
    cchBufLen = 0;
    hr = StringFromPropertyBag ( 
        pPropBag, 
        pIErrorLog, 
        IDS_HTML_LOG_FILE_BASE_NAME, 
        szDefault, 
        &pszTemp, 
        &cchBufLen );
    if ( 1 < cchBufLen ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != *pszTemp );
        mr_strBaseFileName = pszTemp;
    }
    delete [] pszTemp;
    pszTemp = NULL;

    mr_strDefaultDirectory.Empty();
    cchBufLen = 0;    
    hr = StringFromPropertyBag ( 
            pPropBag, 
            pIErrorLog, 
            IDS_HTML_LOG_FILE_FOLDER, 
            m_pLogService->GetDefaultLogFileFolder(), 
            &pszTemp, 
            &cchBufLen );

    if ( 1 < cchBufLen ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != *pszTemp );
        mr_strDefaultDirectory = pszTemp;
    }
    delete [] pszTemp;
    pszTemp = NULL;    

    mr_strSqlName.Empty();
    cchBufLen = 0;
    hr = StringFromPropertyBag ( 
            pPropBag, 
            pIErrorLog, 
            IDS_HTML_SQL_LOG_BASE_NAME, 
            DEFAULT_SQL_LOG_BASE_NAME, 
            &pszTemp, 
            &cchBufLen );
    if ( 1 < cchBufLen ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != *pszTemp );
        mr_strSqlName = pszTemp;
    }
    delete [] pszTemp;
    pszTemp = NULL;


    hr = DwordFromPropertyBag ( pPropBag, pIErrorLog, IDS_HTML_REALTIME_DATASOURCE, g_dwRealTimeQuery, mr_dwRealTimeQuery );
    
    hr = DwordFromPropertyBag ( pPropBag, pIErrorLog, IDS_HTML_LOG_FILE_MAX_SIZE, DEFAULT_LOG_FILE_MAX_SIZE, mr_dwMaxSize );

    if ( SLQ_COUNTER_LOG == GetLogType() ) {
        hr = DwordFromPropertyBag ( pPropBag, pIErrorLog, IDS_HTML_LOG_FILE_TYPE, DEFAULT_CTR_LOG_FILE_TYPE, dwTemp );
    } else {
        // Read only for counter and trace logs, not for alerts?
        hr = DwordFromPropertyBag ( pPropBag, pIErrorLog, IDS_HTML_LOG_FILE_TYPE, DEFAULT_TRACE_LOG_FILE_TYPE, dwTemp );
    }

    SetLogFileType ( dwTemp );

    // Data store attributes must be read after log file type and log file max size.
    InitDataStoreAttributesDefault ( dwTemp, dwDefault );

    // If file size unit value is missing, default to Win2000 values
    hr = DwordFromPropertyBag ( pPropBag, pIErrorLog, IDS_HTML_DATA_STORE_ATTRIBUTES, dwDefault, dwTemp );

    ProcessLoadedDataStoreAttributes ( dwTemp );

    hr = DwordFromPropertyBag ( pPropBag, pIErrorLog, IDS_HTML_LOG_FILE_AUTO_FORMAT, DEFAULT_LOG_FILE_AUTO_FORMAT, mr_dwLogAutoFormat );
    hr = DwordFromPropertyBag ( pPropBag, pIErrorLog, IDS_HTML_LOG_FILE_SERIAL_NUMBER, DEFAULT_LOG_FILE_SERIAL_NUMBER, mr_dwCurrentSerialNumber );
    
    // Do not load "Current State", since a new query is always stopped when created.
    
    // Start and Stop values.
    VERIFY ( GetDefaultLogTime (stiDefault, SLQ_TT_TTYPE_START ) );
    hr = SlqTimeFromPropertyBag ( pPropBag, pIErrorLog, SLQ_TT_TTYPE_START, &stiDefault, &mr_stiStart );
    VERIFY ( GetDefaultLogTime (stiDefault, SLQ_TT_TTYPE_STOP ) );
    hr = SlqTimeFromPropertyBag ( pPropBag, pIErrorLog, SLQ_TT_TTYPE_STOP, &stiDefault, &mr_stiStop );
    hr = DwordFromPropertyBag ( pPropBag, pIErrorLog, IDS_HTML_RESTART_MODE,  DEFAULT_RESTART_VALUE, mr_dwAutoRestartMode);
        
    // Eof command file for counter and trace logs only.
    if ( SLQ_COUNTER_LOG == GetLogType()
         || SLQ_TRACE_LOG == GetLogType() ) {
        
        mr_strEofCmdFile.Empty();
        cchBufLen = 0;

        hr = StringFromPropertyBag ( 
                pPropBag, 
                pIErrorLog, 
                IDS_HTML_EOF_COMMAND_FILE, 
                DEFAULT_EOF_COMMAND_FILE, 
                &szEofCmd, 
                &cchBufLen );

        if ( 1 < cchBufLen ) {
            ASSERT ( NULL != szEofCmd );
            MFC_TRY
                mr_strEofCmdFile = szEofCmd;
            MFC_CATCH_MINIMUM
        }
        
        if ( NULL != szEofCmd ) {
            delete [] szEofCmd;
        }
    }

    SyncPropPageSharedData();

    return hr;
}

HRESULT
CSmLogQuery::SaveToPropertyBag (
    IPropertyBag* pPropBag,
    BOOL /* fSaveAllProps */ )
{
    HRESULT hr = NOERROR;
    SMONCTRL_VERSION_DATA VersData;
    DWORD   dwTemp;
    DWORD   dwTempFileSizeUnits;
    DWORD   dwTempDataStoreAttributes = 0;
    DWORD   dwTempMaxFileSize;
    DWORD   dwTempAppendMode;
	DWORD	dwLogFileType = 0;

    VersData.iMajor = SMONCTRL_MAJ_VERSION;
    VersData.iMinor = SMONCTRL_MIN_VERSION;

    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_SYSMON_VERSION, VersData.dwVersion );
    
    if ( SLQ_ALERT == GetLogType() ) {
        hr = StringToPropertyBag ( pPropBag, IDS_HTML_ALERT_NAME, m_strName );
    } else {
        hr = StringToPropertyBag ( pPropBag, IDS_HTML_LOG_NAME, m_strName );
    }
    hr = StringToPropertyBag ( pPropBag, IDS_HTML_COMMENT, mr_strComment );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_LOG_TYPE, GetLogType() );
    // Save current state. It can be used to determine the validity of the logfilename.
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_CURRENT_STATE, mr_dwCurrentState );

    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_REALTIME_DATASOURCE, mr_dwRealTimeQuery );
    
    // Within the app, counter data store size units are in MB.
    // Translate to back to KB when write to registry
    dwTempFileSizeUnits = GetDataStoreSizeUnits();
    dwTempMaxFileSize = mr_dwMaxSize;
	GetLogFileType ( dwLogFileType );
    if ( SLQ_COUNTER_LOG == GetLogType()
        && SLF_SQL_LOG != dwLogFileType ) 
    {
        if ( ONE_MB == dwTempFileSizeUnits ) { 
            dwTempFileSizeUnits = ONE_KB;
            // Round up to next MB
            if ( SLQ_DISK_MAX_SIZE != mr_dwMaxSize ) {
                dwTempMaxFileSize *= dwTempFileSizeUnits;
            }
        }
    }

    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_LOG_FILE_MAX_SIZE, dwTempMaxFileSize );

    // Data store size units
    if ( ONE_MB == dwTempFileSizeUnits ) {
        dwTempDataStoreAttributes = SLF_DATA_STORE_SIZE_ONE_MB;
    } else if ( ONE_KB == dwTempFileSizeUnits ) {
        dwTempDataStoreAttributes = SLF_DATA_STORE_SIZE_ONE_KB;
    } else if ( ONE_RECORD == dwTempFileSizeUnits ) {
        dwTempDataStoreAttributes = SLF_DATA_STORE_SIZE_ONE_RECORD;
    }

    // Data store append mode
    GetDataStoreAppendMode( dwTempAppendMode );
    dwTempDataStoreAttributes |= dwTempAppendMode;

    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_DATA_STORE_ATTRIBUTES, dwTempDataStoreAttributes );

    hr = StringToPropertyBag ( pPropBag, IDS_HTML_LOG_FILE_BASE_NAME, mr_strBaseFileName );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_LOG_FILE_SERIAL_NUMBER, mr_dwCurrentSerialNumber );
    hr = StringToPropertyBag ( pPropBag, IDS_HTML_LOG_FILE_FOLDER, mr_strDefaultDirectory );
    hr = StringToPropertyBag ( pPropBag, IDS_HTML_SQL_LOG_BASE_NAME, mr_strSqlName );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_LOG_FILE_AUTO_FORMAT, mr_dwLogAutoFormat );

    // Write only for counter and trace logs, not for alerts?
    // Log file type for Alerts is -1, so the new query will keep its default value.
    GetLogFileType ( dwTemp );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_LOG_FILE_TYPE, dwTemp );
    hr = SlqTimeToPropertyBag ( pPropBag, SLQ_TT_TTYPE_START, &mr_stiStart );
    hr = SlqTimeToPropertyBag ( pPropBag, SLQ_TT_TTYPE_STOP, &mr_stiStop );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_RESTART_MODE, mr_dwAutoRestartMode );

    hr = StringToPropertyBag ( pPropBag, IDS_HTML_SYSMON_LOGFILENAME, GetLogFileName(TRUE) );

    // Eof command file for counter and trace logs only.
    if ( SLQ_COUNTER_LOG == GetLogType()
         || SLQ_TRACE_LOG == GetLogType() ) {
        hr = StringToPropertyBag ( pPropBag, IDS_HTML_EOF_COMMAND_FILE, mr_strEofCmdFile );
    }
    return hr;
}

HRESULT
CSmLogQuery::CopyToBuffer ( LPWSTR& rpszData, DWORD& rcbBufferSize )
{
    HRESULT hr = S_OK;
    CImpIPropertyBag    IPropBag;
    size_t  cchBufLen = 0;
    LPWSTR  pszConfig = NULL;

    ResourceStateManager rsm;

    ASSERT ( NULL == rpszData );
    rcbBufferSize = 0;

    hr = SaveToPropertyBag (&IPropBag, TRUE );
   
    if ( SUCCEEDED ( hr ) ) {
        MFC_TRY
            pszConfig = IPropBag.GetData();        
            if ( NULL != pszConfig ) {

                cchBufLen = lstrlen ( CGlobalString::m_cszHtmlObjectHeader )
                            + lstrlen ( CGlobalString::m_cszHtmlObjectFooter )
                            + lstrlen ( pszConfig ) + 1;

                rpszData = new WCHAR[cchBufLen];
            } else {
                hr = E_UNEXPECTED;
            }
        MFC_CATCH_HR

        if ( SUCCEEDED ( hr ) ) {

            StringCchCopy ( rpszData, cchBufLen, CGlobalString::m_cszHtmlObjectHeader );
            StringCchCat ( rpszData, cchBufLen, pszConfig );
            StringCchCat ( rpszData, cchBufLen, CGlobalString::m_cszHtmlObjectFooter );

            rcbBufferSize = (DWORD)(cchBufLen * sizeof(WCHAR));
        }
    }

    return hr;
}

BOOL
CSmLogQuery::LLTimeToVariantDate (
    IN  LONGLONG llTime,
    OUT DATE *pdate
    )
{
    SYSTEMTIME SystemTime;

    if (!FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime))
        return FALSE;

    if (FAILED(SystemTimeToVariantTime(&SystemTime, pdate)))
        return FALSE;

    return TRUE;
}

    
BOOL
CSmLogQuery::VariantDateToLLTime (
    IN  DATE date,
    OUT LONGLONG *pllTime
    )
{
    SYSTEMTIME SystemTime;

    if (FAILED(VariantTimeToSystemTime(date, &SystemTime)))
        return FALSE;

    if (!SystemTimeToFileTime(&SystemTime,(FILETIME*)pllTime))
        return FALSE;

    return TRUE;
}

DWORD 
CSmLogQuery::UpdateExecuteOnly( void )
{
    DWORD dwStatus = ERROR_SUCCESS;

    if (!m_bReadOnly) {
    
        DWORD dwExecuteOnly;

        dwExecuteOnly = 1;        // TRUE

        dwStatus = WriteRegistryDwordValue (
            m_hKeyQuery, 
            IDS_REG_EXECUTE_ONLY,
            &dwExecuteOnly);

        ASSERT ( ERROR_SUCCESS == dwStatus );

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = m_pLogService->Synchronize();    
        }
    } else {
        dwStatus = ERROR_ACCESS_DENIED;
    }

    return dwStatus;
}

CWnd* 
CSmLogQuery::GetActivePropertySheet( void )
{
    CWnd* pwndReturn = NULL;

    if ( NULL != m_pActivePropPage ) {
        pwndReturn = m_pActivePropPage->GetParentOwner();
    }
    return pwndReturn;
}

CPropertySheet* 
CSmLogQuery::GetInitialPropertySheet( void )
{
    return m_pInitialPropertySheet;
}

void 
CSmLogQuery::SetInitialPropertySheet( CPropertySheet* pSheet )
{
    m_pInitialPropertySheet = (pSheet);
}

void
CSmLogQuery::SetActivePropertyPage( CSmPropertyPage* pPage)
{
    // The first property page of each property sheet sets 
    // and clears this member variable.
    // It is assumed that the first page is always created.
    m_pActivePropPage = pPage;

    return;
}

BOOL   
CSmLogQuery::IsFirstModification( void ) 
{
    BOOL    bIsFirstModification = FALSE;
    bIsFirstModification = ( m_bIsModified && m_bIsNew );

    if ( bIsFirstModification ) {
        m_bIsNew = FALSE;
    }

    return bIsFirstModification;
}

LPCWSTR 
CSmLogQuery::GetNonLocRegValueName ( UINT uiValueName )
{
    UINT    uiLocalIndex;
    LPCWSTR szReturn = NULL;
//
// NOTE:  Unless otherwise specified, all registry Resource Id values are contiguous
// beginning with 816, so they can be used as indexes into the map.
//
    if ( IDS_REG_EXECUTE_ONLY != uiValueName ) {
        uiLocalIndex = uiValueName - IDS_REG_FIRST_VALUE_NAME;
    } else { 
        uiLocalIndex = dwRegValueNameMapEntries - 1;
    }

    if ( uiLocalIndex < dwRegValueNameMapEntries ) {
        szReturn = RegValueNameMap [ uiLocalIndex ].szNonLocValueName;
    }

    // Programming error if no string found.
    ASSERT ( NULL != szReturn );

    return szReturn;
}

LPCWSTR 
CSmLogQuery::GetNonLocHtmlPropName ( UINT uiValueName )
{
    UINT    uiLocalIndex;
    LPCWSTR szReturn = NULL;
//
// NOTE:  Unless otherwise specified, all Html Resource Id values are contiguous
// beginning with 900, so they can be used as indexes into the map.
//
    
    uiLocalIndex = uiValueName - IDS_HTML_FIRST_VALUE_NAME;

    if ( uiLocalIndex < dwHtmlPropNameMapEntries ) {
        szReturn = HtmlPropNameMap [ uiLocalIndex ].szNonLocValueName;
    }

    // Programming error if no string found.
    ASSERT ( NULL != szReturn );

    return szReturn;
}
