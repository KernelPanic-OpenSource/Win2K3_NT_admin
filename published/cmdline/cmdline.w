// ****************************************************************************
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//
//        Cmdline.h
//
//  Abstract:
//
//        This modules contains the common functionality file headers and type
//        definitions
//
//  Author:
//
//        Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000
//
//  Revision History:
//
//        Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000 : Created It.
//
// ****************************************************************************

#ifndef __CMDLINE_H
#define __CMDLINE_H

#ifndef CMDLINE_VERSION
#define CMDLINE_VERSION         100
#endif

#ifndef UNICODE
#error this library can be compiled only in UNICODE environment
#endif

#ifdef __cplusplus
extern "C" {
#endif

// constants / defines

#define MAX_RES_STRING                  254
#define MAX_STRING_LENGTH               254

// stubs for 'displaying'
#define DISPLAY_MESSAGE( file, message )           ShowMessage( file, message )


#if CMDLINE_VERSION == 100

// ****************************************************************************
// all the below stuff in deprecated in cmdline library version
// greater that 1.0
// ****************************************************************************

//
// custom macros ( for trapping memory leaks )
//
#ifdef _DEBUG

#define __malloc( size )        \
        _malloc_dbg( size, _NORMAL_BLOCK, __FILE__, __LINE__ )

#define __calloc( count, size )         \
        _calloc_dbg( count, size, _NORMAL_BLOCK, __FILE__, __LINE__ )

#define __free( block )                             \
        if ( (block) != NULL )                      \
        {                                           \
            _free_dbg( (block), _NORMAL_BLOCK );    \
            (block) = NULL;                         \
        }                                           \
        1

#define __realloc( block, size )                                    \
        (((block) != NULL) ?                                        \
                _realloc_dbg( (block), size,                        \
                            _NORMAL_BLOCK, __FILE__, __LINE__ ) :   \
                __malloc( size ))

#else   // #ifdef _DEBUG

#define __malloc( size )            malloc( size )

#define __calloc( count, size )     calloc( count, size )

#define __free( block )             \
        if ( (block) != NULL )      \
        {                           \
            free( (block) );        \
            (block) = NULL;         \
        }                           \
        1

#define __realloc( block, size )                \
        (((block) != NULL) ? realloc( block, size ) : __malloc( size ))

#endif  // #ifdef _DEBUG

//
// custom macros
//
#define CHECK_NULL( statement, value )  \
        if ( ( statement ) == NULL )    \
        {                               \
            return value;               \
        }       \
        1

#define SHOW_RESULT_MESSAGE( server, tag, message )             \
        if ( IsLocalSystem( server ) == TRUE )                  \
        {                                                       \
                DISPLAY_MESSAGE2( stderr, szBuffer,             \
                        _T( "%s %s" ), tag, message );          \
        }                                                       \
        else                                                    \
        {                                                       \
                DISPLAY_MESSAGE3( stderr, szBuffer,             \
                    _T( "%s %s: %s" ), tag, server, message );  \
        }                                                       \
        1

//
// string formatting stubs
#define FORMAT_STRING( buffer, format, value )                          \
        swprintf( buffer, format, value )

#define FORMAT_STRING2( buffer, format, value1, value2 )                \
        swprintf( buffer, format, value1, value2 )

#define FORMAT_STRING3( buffer, format, value1, value2, value3 )        \
        swprintf( buffer, format, value1, value2, value3 )

#define FORMAT_STRING_EX( buffer, format, value, length, alignflag )    \
        swprintf( buffer, format, value );      \
        AdjustStringLength( buffer, length, alignflag )

#define FORMAT_STRING_EX2( buffer, format, value1, value2, length, alignflag )  \
        swprintf( buffer, format, value1, value2 );                             \
        AdjustStringLength( buffer, length, alignflag )

#define FORMAT_STRING_EX3( buffer, format, value1, value2, value3, length, alignflag )  \
        swprintf( buffer, format, value1, value2, value3 );                             \
        AdjustStringLength( buffer, length, alignflag )

//
// stubs for 'displaying'
#define DISPLAY_MESSAGE( file, message )                                    \
        ShowMessage( file, message )

#define DISPLAY_MESSAGE1( file, buffer, format, value1 )                    \
        FORMAT_STRING( buffer, format, value1 );                            \
        ShowMessage( file, buffer )

#define DISPLAY_MESSAGE2( file, buffer, format, value1, value2 )            \
        FORMAT_STRING2( buffer, format, value1, value2 );                   \
        ShowMessage( file, buffer )

#define DISPLAY_MESSAGE3( file, buffer, format, value1, value2, value3 )    \
        FORMAT_STRING3( buffer, format, value1, value2, value3 );           \
        ShowMessage( file, buffer )

//
// display messages from resource file's string table ( stubs )
#define DISPLAY_RES_MESSAGE( file, id ) \
        ShowResMessage( file, id )

#define DISPLAY_RES_MESSAGE1( file, buffer, id, value1 )        \
        FORMAT_STRING( buffer, GetResString( id ), value1 );    \
        ShowMessage( file, buffer )

#define DISPLAY_RES_MESSAGE2( file, buffer, id, value1, value2 )        \
        FORMAT_STRING2( buffer, GetResString( id ), value1, value2 );   \
        ShowMessage( file, buffer )

#define DISPLAY_RES_MESSAGE3( file, buffer, id, value1, value2, value3 )        \
        FORMAT_STRING3( buffer, GetResString( id ), value1, value2, value3 );   \
        ShowMessage( file, buffer )



#define MAX_USERNAME_LENGTH             64
#define MAX_PASSWORD_LENGTH             64

#define FULL_SUCCESS                    0
#define PARTIALLY_SUCCESS               128
#define COMPLETELY_FAILED               255

//
// type definitions
//
typedef TCHAR __STRING_32[ 32 ];
typedef TCHAR __STRING_64[ 64 ];
typedef TCHAR __STRING_128[ 128 ];
typedef TCHAR __STRING_256[ 256 ];
typedef TCHAR __STRING_512[ 512 ];
typedef TCHAR __STRING_1024[ 1024 ];
typedef TCHAR __STRING_2048[ 2048 ];
typedef TCHAR __STRING_4096[ 4096 ];
typedef TCHAR __RESOURCE_STRING [ MAX_RES_STRING + 1 ];
typedef TCHAR __MAX_SIZE_STRING[ MAX_STRING_LENGTH + 1 ];


// string manipulation macros
#define STRING_COPY_STATIC( destination, source )           \
            StringCopy( destination, source, SIZE_OF_ARRAY( destination ) )

#define STRING_COPY_DYNAMIC( destination, source )          \
            StringCopyEx( destination, source )

#define STRING_CONCAT_STATIC( destination, source )         \
        StringConcat( destination, source, SIZE_OF_ARRAY( destination ) )

#define STRING_CONCAT_DYNAMIC( destination, source )        \
        StringConcatEx( destination, source )

#define STRING_COPY( destination, source )      \
    if ( destination != NULL && _msize( destination ) != -1 )       \
    {                                                               \
        STRING_COPY_DYNAMIC( destination, source );                 \
    }                                                               \
    else                                                            \
    {                                                               \
        STRING_COPY_STATIC( destination, source );                  \
    }                                                               \
    1

#define STRING_CONCAT( destination, source )     \
    if ( destination != NULL && _msize( destination ) != -1 )       \
    {                                                               \
        STRING_CONCAT_DYNAMIC( destination, source );               \
    }                                                               \
    else                                                            \
    {                                                               \
        STRING_CONCAT_STATIC( destination, source );                \
    }                                                               \
    1

//
// function re-defs
//
#ifdef UNICODE

#define GetCompatibleStringFromMultiByte        GetAsUnicodeStringEx
#define GetCompatibleStringFromUnicode          GetAsUnicodeString

#else

#define GetCompatibleStringFromMultiByte        GetAsMultiByteString
#define GetCompatibleStringFromUnicode          GetAsMultiByteStringEx

#endif  // UNICODE

/////////
// conversion functions
// old legacy functions -- these are made obsolete
LPSTR GetAsMultiByteString( LPCWSTR pszSource,
                            LPSTR pszDestination, DWORD dwLength );
LPWSTR GetAsUnicodeStringEx( LPCSTR pszSource,
                             LPWSTR pwszDestination, DWORD dwLength );

/////////
// this function is mapped to the StringCopyW
// LPWSTR GetAsUnicodeString( LPCWSTR pszSource,
//                            LPWSTR pwszDestination,
//                            DWORD dwLength );
#define GetAsUnicodeString( source,                                         \
                            destination,                                    \
                            length )        StringCopyW( destination,       \
                                                         source,            \
                                                         length )

/////////
// this function is mapped to its another function which is of similar type
// LPSTR GetAsMultiByteStringEx( LPCWSTR pwszSource,
//                               LPSTR pszDestination, DWORD dwLength );
#define GetAsMultiByteStringEx      GetAsMultiByteString

// ****************************************************************************
// all the above stuff is deprecated in cmdline library version
// greater that 1.0
// ****************************************************************************

#endif      // CMDLINE_VERSION == 100


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                      COMMON FUNCTIONALITY                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// wrappers for quote meta
#define _X( text )              text
#define _X1( text )             text
#define _X2( text )             text
#define _X3( text )             text

//
// macros
//
#define TAG_ERROR                    GetResString( IDS_TAG_ERROR )
#define TAG_WARNING                  GetResString( IDS_TAG_WARNING )
#define TAG_SUCCESS                  GetResString( IDS_TAG_SUCCESS )
#define TAG_INFORMATION              GetResString( IDS_TAG_INFORMATION )
#define SIZE_OF_ARRAY( array )       ( sizeof( array ) / sizeof( array[ 0 ] ) )

//
// constants / defines / enumerations
//
#define _DEFAULT_CODEPAGE               CP_OEMCP

#define BACK_SPACE                      0x08
#define BLANK_CHAR                      0x00
#define CARRIAGE_RETURN                 0x0D

#define ASTERIX                         _T( "*" )
#define BEEP_SOUND                      _T( "\a" )

#define TRIM_LEFT                       0x00000001
#define TRIM_RIGHT                      0x00000002
#define TRIM_ALL                        0x00000003

extern const WCHAR cwchNullChar;
extern const WCHAR cwszNullString[ 2 ];

#if CMDLINE_VERSION == 100
#define NULL_CHAR                       L'\0'
#define NULL_STRING                     L"\0"
#endif

#define V_NOT_AVAILABLE                 GetResString( IDS_V_NOT_AVAILABLE )
#define ERROR_OS_INCOMPATIBLE           GetResString( IDS_ERROR_OS_INCOMPATIBLE )
#define ERROR_REMOTE_INCOMPATIBLE       GetResString( IDS_ERROR_REMOTE_INCOMPATIBLE )

// macros for copy or concatenation ansi/unicode strings
#ifdef UNICODE

#define StringCopy              StringCopyW
#define StringCopyEx            StringCopyExW
#define StringConcat            StringConcatW
#define StringConcatEx          StringConcatExW
#define StringLength            StringLengthW
#define StringCompare           StringCompareW
#define StringCompareEx         StringCompareExW
#else

#define StringCopy              StringCopyA
#define StringCopyEx            StringCopyExA
#define StringConcat            StringConcatA
#define StringConcatEx          StringConcatExA
#define StringLength            StringLengthA
#define StringCompare           StringCompareA
#define StringCompareEx         StringCompareExA

#endif

//
// flags -- specific to ShowLastErrorEx
#define SLE_TYPE_ERROR          0x00000001
#define SLE_TYPE_INFO           0x00000002
#define SLE_TYPE_WARNING        0x00000004
#define SLE_TYPE_SUCCESS        0x00000008
#define SLE_SYSTEM              0x00010000
#define SLE_INTERNAL            0x00020000
#define SLE_MASK                0x000F0000

//
// pattern matching flags
#define PATTERN_LOCALE_USENGLISH             0x00000001
#define PATTERN_NOPARSING                    0x00000002
#define PATTERN_COMPARE_IGNORECASE           0x00000100
#define PATTERN_COMPARE_IGNOREKANATYPE       0x00000200
#define PATTERN_COMPARE_IGNORENONSPACE       0x00000400
#define PATTERN_COMPARE_IGNORESYMBOLS        0x00000800
#define PATTERN_COMPARE_IGNOREWIDTH          0x00001000
#define PATTERN_COMPARE_STRINGSORT           0x00002000



//
// function prototypes
//

BOOL InitGlobals();

LPCWSTR GetReason();
BOOL SetReason( LPCWSTR pwszReason );
BOOL SetReason2( DWORD dwCount, LPCWSTR pwszFormat, ... );

BOOL SaveLastError();
DWORD WNetSaveLastError();
BOOL ShowLastError( FILE* fp );
BOOL ShowLastErrorEx( FILE* fp, DWORD dwFlags );

BOOL ReleaseGlobals();

BOOL IsWin2KOrLater();
BOOL IsCompatibleOperatingSystem( DWORD dwVersion );
BOOL SetOsVersion( DWORD dwMajor, DWORD dwMinor, WORD wServicePackMajor );

LPCWSTR GetResString( UINT uID );
LPCWSTR GetResString2( UINT uID, DWORD dwIndexNumber );

double AsFloat( LPCWSTR pwszValue );
BOOL IsFloatingPoint( LPCWSTR pwszValue );
LONG AsLong( LPCWSTR pwszValue, DWORD dwBase );
BOOL IsNumeric( LPCWSTR pwszValue, DWORD dwBase, BOOL bSigned );

LPCWSTR FindChar( LPCWSTR pwszString, WCHAR wch, DWORD dwFrom );
LONG FindChar2( LPCWSTR pwszString, WCHAR wch, BOOL bIgnoreCase, DWORD dwFrom );
BOOL InString( LPCWSTR pwszString, LPCWSTR pwszList, BOOL bIgnoreCase );
LPCWSTR FindOneOf( LPCWSTR pwszText, LPCWSTR pwszTextToFind, DWORD dwFrom );
LONG FindOneOf2( LPCWSTR pwszText, LPCWSTR pwszTextToFind, BOOL bIgnoreCase, DWORD dwFrom );
LPCWSTR FindString( LPCWSTR pwszText, LPCWSTR pwszTextToFind, DWORD dwFrom );
LONG FindString2( LPCWSTR pwszText, LPCWSTR pwszTextToFind, BOOL bIgnoreCase, DWORD dwFrom );

LONG StringLengthInBytes( LPCWSTR pwszText );

LPCWSTR TrimString( LPWSTR pwszString, DWORD dwFlags );
LPCWSTR TrimString2( LPWSTR pwszString, LPCWSTR pwszTrimChars, DWORD dwFlags );
LPCWSTR QuoteMeta( LPCWSTR pwszText, DWORD dwQuoteIndex );
LPCWSTR AdjustStringLength( LPWSTR pwszValue, DWORD dwLength, BOOL bPadLeft );
LPCWSTR Replicate( LPWSTR pwszBuffer,
                   LPCWSTR pwszText, DWORD dwCount, DWORD dwLength );

BOOL IsConsoleFile( FILE* fp );
LCID GetSupportedUserLocale( BOOL* pbLocaleChanged );

BOOL StringCopyExA( LPSTR pszDest, LPCSTR pszSource );
BOOL StringConcatExA( LPSTR pszDest, LPCSTR pszSource );
BOOL StringCopyExW( LPWSTR pwszDest, LPCWSTR pwszSource );
BOOL StringConcatExW( LPWSTR pwszDest, LPCWSTR pwszSource );
BOOL StringCopyA( LPSTR pszDest, LPCSTR pszSource, LONG lSize );
BOOL StringConcatA( LPSTR pszDest, LPCSTR pszSource, LONG lSize );
BOOL StringCopyW( LPWSTR pwszDest, LPCWSTR pwszSource, LONG lSize );
BOOL StringConcatW( LPWSTR pwszDest, LPCWSTR pwszSource, LONG lSize );
DWORD StringLengthA( LPCSTR pszSource, DWORD dwReserved );
DWORD StringLengthW( LPCWSTR pwszSource, DWORD dwReserved );

LONG StringCompareW( LPCWSTR pwszString1,
                     LPCWSTR pwszString2, BOOL bIgnoreCase, DWORD dwCount );
LONG StringCompareA( LPCSTR pwszString1,
                     LPCSTR pwszString2, BOOL bIgnoreCase, DWORD dwCount );
LONG StringCompareExW( LPCWSTR pwszString1,
                       LPCWSTR pwszString2, BOOL bIgnoreCase, DWORD dwCount );
LONG StringCompareExA( LPCSTR pwszString1,
                       LPCSTR pwszString2, BOOL bIgnoreCase, DWORD dwCount );

BOOL ShowResMessage( FILE* fp, UINT uID );
BOOL ShowMessage( FILE* fp, LPCWSTR pwszMessage );
BOOL ShowMessageEx( FILE* fp, DWORD dwCount, BOOL bStyle, LPCWSTR pwszFormat, ... );

BOOL GetAsUnicodeString2( LPCSTR pszSource, LPWSTR pwszDestination, DWORD* pdwLength );
BOOL GetAsMultiByteString2( LPCWSTR pwszSource, LPSTR pszDestination, DWORD* pdwLength );

BOOL GetPassword( LPWSTR pwszPassword, DWORD dwMaxPasswordSize );

BOOL MatchPattern( LPWSTR pwszPattern, LPWSTR pwszText );

LPCWSTR ParsePattern( LPCWSTR pwszPattern );
BOOL MatchPatternEx( LPCWSTR pwszText, LPCWSTR pwszPattern, DWORD dwFlags );

BOOL FreeMemory( LPVOID* ppv );
BOOL CheckMemory( LPVOID pv );
LPVOID AllocateMemory( DWORD dwBytes );
BOOL ReallocateMemory( LPVOID* ppv, DWORD dwBytesNew );
LONG GetBufferSize( LPVOID pv );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                      REMOTE CONNECTIVITY                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// error codes
//
#define E_LOCAL_CREDENTIALS             0xA0010001              // 1010 0000 0000 0001 0000 0000 0000 0001
#define I_NO_CLOSE_CONNECTION           0x50010001              // 0101 0000 0000 0001 0000 0000 0000 0001

//
// structs
//
typedef struct tagConnectionInfo
{
    DWORD dwFlags;                                  // flags
    LPCTSTR pszServer;                              // server name
    LPTSTR pszUserName;                             // user name
    DWORD dwUserLength;                             // max. no. of characters allowed for user name
    LPTSTR pszPassword;                             // password
    DWORD dwPasswordLength;                         // max. no. of characters allowed for password
    LPCTSTR pszShare;                               // tells the custom share name
    LPVOID lpReserved1;                             // reserved for future use
    LPVOID lpReserved2;                             // reserved for future use
    LPVOID lpReserved3;                             // reserved for future use
} TCONNECTIONINFO, *PTCONNECTIONINFO;

//
// connection info flags
//

// general flags
#define CI_ACCEPT_PASSWORD              0x00000001                      // 0000 0001

// share (if nothing specified, default will be assumed as IPC$ )
#define CI_SHARE_IPC                    0x00000010                      // 0000 0000 0001 XXXX XXXX
#define CI_SHARE_ADMIN                  0x00000020                      // 0000 0000 0010 XXXX XXXX
#define CI_SHARE_CUSTOM                 0x00000040                      // 0000 0001 0000 XXXX XXXX

// extra flag used while closing connection
#define CI_CLOSE_BY_FORCE               0x10000000

//
// function prototypes
//

BOOL IsUserAdmin();
BOOL IsUNCFormat( LPCWSTR pwszServer );
BOOL IsLocalSystem( LPCWSTR pwszServer );
BOOL IsValidServer( LPCWSTR pwszServer );
BOOL IsValidIPAddress( LPCWSTR pwszAddress );

BOOL GetHostByIPAddr( LPCWSTR pwszServer,
                      LPWSTR pwszHostName,
                      DWORD* pdwHostNameLength, BOOL bNeedFQDN );

DWORD GetTargetVersion( LPCWSTR pwszServer );

DWORD ConnectServer( LPCWSTR pwszServer, LPCWSTR pwszUser, LPCWSTR pwszPassword );
DWORD ConnectServer2( LPCWSTR pwszServer, LPCWSTR pwszUser,
                      LPCWSTR pwszPassword, LPCWSTR pwszShare );

DWORD CloseConnection( LPCWSTR pwszServer );
DWORD CloseConnection2( LPCWSTR pwszServer, LPCWSTR pwszShare, DWORD dwFlags );

BOOL EstablishConnectionEx( PTCONNECTIONINFO pci );
BOOL EstablishConnection( LPCWSTR pwszServer,
                          LPWSTR pwszUserName, DWORD dwUserLength,
                          LPWSTR pwszPassword, DWORD dwPasswordLength, BOOL bNeedPassword );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                 MULTI-DIMENSIONAL MULTI-TYPE DYNAMIC ARRAYS               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// constants / defines / enumerations
//

// item types supported by dynamic array
#define DA_TYPE_NONE                    0x00000000
#define DA_TYPE_GENERAL                 0x00010000
#define DA_TYPE_STRING                  0x00020000
#define DA_TYPE_LONG                    0x00030000
#define DA_TYPE_DWORD                   0x00040000
#define DA_TYPE_BOOL                    0x00050000
#define DA_TYPE_FLOAT                   0x00060000
#define DA_TYPE_DOUBLE                  0x00070000
#define DA_TYPE_ARRAY                   0x00080000
#define DA_TYPE_HANDLE                  0x00090000
#define DA_TYPE_SYSTEMTIME              0x000A0000
#define DA_TYPE_FILETIME                0x000B0000

//
// type definitions
//
typedef VOID* TARRAY;
typedef TARRAY* PTARRAY;

//
// public function prototypes
//

//
// array pointer validation function(s)
BOOL IsValidArray( TARRAY pArray );

//
// dynamic array creation / destraction functions
TARRAY CreateDynamicArray();
VOID DestroyDynamicArray( PTARRAY pArray );

//
// general helper function(s)
DWORD DynArrayGetCount( TARRAY pArray );
DWORD DynArrayGetCount2( TARRAY pArray, DWORD dwRow );
DWORD DynArrayGetItemType( TARRAY pArray, DWORD dwIndex );
DWORD DynArrayGetItemType2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );

//
// adding columns
LONG DynArrayAddColumns( TARRAY pArray, DWORD dwColumns );
LONG DynArrayInsertColumns( TARRAY pArray, DWORD dwIndex, DWORD dwColumns );

//
// array append function(s) ( for one-dimensional array )
LONG DynArrayAppend( TARRAY pArray, LPVOID pValue );
LONG DynArrayAppendLong( TARRAY pArray, LONG lValue );
LONG DynArrayAppendBOOL( TARRAY pArray, BOOL bValue );
LONG DynArrayAppendDWORD( TARRAY pArray, DWORD dwValue );
LONG DynArrayAppendFloat( TARRAY pArray, float dwValue );
LONG DynArrayAppendDouble( TARRAY pArray, double dwValue );
LONG DynArrayAppendString( TARRAY pArray, LPCTSTR szValue, DWORD dwLength );
LONG DynArrayAppendHandle( TARRAY pArray, HANDLE hValue );
LONG DynArrayAppendSystemTime( TARRAY pArray, SYSTEMTIME stValue );
LONG DynArrayAppendFileTime( TARRAY pArray, FILETIME ftValue );

// helper to append 2-dimensional array
LONG DynArrayAppendRow( TARRAY pArray, DWORD dwColumns );

//  ( for two-dimensional array )
LONG DynArrayAppend2( TARRAY pArray, DWORD dwRow, LPVOID pValue );
LONG DynArrayAppendLong2( TARRAY pArray, DWORD dwRow, LONG lValue );
LONG DynArrayAppendBOOL2( TARRAY pArray, DWORD dwRow, BOOL bValue );
LONG DynArrayAppendDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwValue );
LONG DynArrayAppendFloat2( TARRAY pArray, DWORD dwRow, float dwValue );
LONG DynArrayAppendDouble2( TARRAY pArray, DWORD dwRow, double dwValue );
LONG DynArrayAppendString2( TARRAY pArray, DWORD dwRow, LPCTSTR szValue, DWORD dwLength );
LONG DynArrayAppendHandle2( TARRAY pArray, DWORD dwRow, HANDLE hValue );
LONG DynArrayAppendSystemTime2( TARRAY pArray, DWORD dwRow, SYSTEMTIME stValue );
LONG DynArrayAppendFileTime2( TARRAY pArray, DWORD dwRow, FILETIME ftValue );

//
// array insert function(s) ( for one-dimensional array )
LONG DynArrayInsert( TARRAY pArray, DWORD dwIndex, LPVOID pValue );
LONG DynArrayInsertLong( TARRAY pArray, DWORD dwIndex, LONG lValue );
LONG DynArrayInsertBOOL( TARRAY pArray, DWORD dwIndex, BOOL bValue );
LONG DynArrayInsertDWORD( TARRAY pArray, DWORD dwIndex, DWORD dwValue );
LONG DynArrayInsertFloat( TARRAY pArray, DWORD dwIndex, float dwValue );
LONG DynArrayInsertDouble( TARRAY pArray, DWORD dwIndex, double dwValue );
LONG DynArrayInsertString( TARRAY pArray, DWORD dwIndex, LPCTSTR szValue, DWORD dwLength );
LONG DynArrayInsertHandle( TARRAY pArray, DWORD dwIndex, HANDLE hValue );
LONG DynArrayInsertSystemTime( TARRAY pArray, DWORD dwIndex, SYSTEMTIME stValue );
LONG DynArrayInsertFileTime( TARRAY pArray, DWORD dwIndex, FILETIME ftValue );

// helper to insert 2-dimensional array
LONG DynArrayInsertRow( TARRAY pArray, DWORD dwIndex, DWORD dwColumns );

// ( for two-dimensional array )
LONG DynArrayInsert2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, LPVOID pValue );
LONG DynArrayInsertLong2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, LONG lValue );
LONG DynArrayInsertBOOL2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, BOOL bValue );
LONG DynArrayInsertDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, DWORD dwValue );
LONG DynArrayInsertFloat2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, float dwValue );
LONG DynArrayInsertDouble2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, double dwValue );
LONG DynArrayInsertString2( TARRAY pArray, DWORD dwRow,
                                                    DWORD dwColIndex, LPCTSTR szValue, DWORD dwLength );
LONG DynArrayInsertHandle2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, HANDLE hValue );
LONG DynArrayInsertSystemTime2( TARRAY pArray, DWORD dwRow,
                                                         DWORD dwColIndex, SYSTEMTIME stValue );
LONG DynArrayInsertFileTime2( TARRAY pArray, DWORD dwRow,
                                                          DWORD dwColIndex, FILETIME ftValue );

//
// item value set function(s) ( for one-dimensional array )
BOOL DynArraySet( TARRAY pArray, DWORD dwIndex, LPVOID pValue );
BOOL DynArraySetLong( TARRAY pArray, DWORD dwIndex, LONG lValue );
BOOL DynArraySetBOOL( TARRAY pArray, DWORD dwIndex, BOOL bValue );
BOOL DynArraySetDWORD( TARRAY pArray, DWORD dwIndex, DWORD dwValue );
BOOL DynArraySetFloat( TARRAY pArray, DWORD dwIndex, float dwValue );
BOOL DynArraySetDouble( TARRAY pArray, DWORD dwIndex, double dwValue );
BOOL DynArraySetString( TARRAY pArray, DWORD dwIndex, LPCTSTR szValue, DWORD dwLength );
BOOL DynArraySetHandle( TARRAY pArray, DWORD dwIndex, HANDLE hValue );
BOOL DynArraySetSystemTime( TARRAY pArray, DWORD dwIndex, SYSTEMTIME stValue );
BOOL DynArraySetFileTime( TARRAY pArray, DWORD dwIndex, FILETIME ftValue );

// ( for two-dimensional array )
BOOL DynArraySet2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, LPVOID pValue );
BOOL DynArraySetLong2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, LONG lValue );
BOOL DynArraySetBOOL2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, BOOL bValue );
BOOL DynArraySetDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, DWORD dwValue );
BOOL DynArraySetFloat2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, float dwValue );
BOOL DynArraySetDouble2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, double dwValue );
BOOL DynArraySetString2( TARRAY pArray, DWORD dwRow,
                         DWORD dwColumn, LPCTSTR szValue, DWORD dwLength );
BOOL DynArraySetHandle2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, HANDLE hValue );
BOOL DynArraySetSystemTime2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, SYSTEMTIME stValue );
BOOL DynArraySetFileTime2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, FILETIME ftValue );

//
// item value get function(s) ( for one-dimensional array )
LPVOID DynArrayItem( TARRAY pArray, DWORD dwIndex );
LONG DynArrayItemAsLong( TARRAY pArray, DWORD dwIndex );
BOOL DynArrayItemAsBOOL( TARRAY pArray, DWORD dwIndex );
DWORD DynArrayItemAsDWORD( TARRAY pArray, DWORD dwIndex );
float DynArrayItemAsFloat( TARRAY pArray, DWORD dwIndex );
double DynArrayItemAsDouble( TARRAY pArray, DWORD dwIndex );
LPCTSTR DynArrayItemAsString( TARRAY pArray, DWORD dwIndex );
HANDLE DynArrayItemAsHandle( TARRAY pArrray, DWORD dwIndex );
SYSTEMTIME DynArrayItemAsSystemTime( TARRAY pArray, DWORD dwIndex );
FILETIME DynArrayItemAsFileTime( TARRAY pArray, DWORD dwIndex );
DWORD DynArrayItemAsStringEx( TARRAY pArray, DWORD dwIndex, LPTSTR szBuffer, DWORD dwLength );

// ( for two-dimensional array )
LPVOID DynArrayItem2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
LONG DynArrayItemAsLong2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
BOOL DynArrayItemAsBOOL2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
DWORD DynArrayItemAsDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
float DynArrayItemAsFloat2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
double DynArrayItemAsDouble2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
LPCTSTR DynArrayItemAsString2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
HANDLE DynArrayItemAsHandle2( TARRAY pArrray, DWORD dwRow, DWORD dwColumn );
SYSTEMTIME DynArrayItemAsSystemTime2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
FILETIME DynArrayItemAsFileTime2( TARRAY pArray, DWORD dwRow, DWORD dwColumn );
DWORD DynArrayItemAsStringEx2( TARRAY pArray,
                               DWORD dwRow, DWORD dwColumn, LPTSTR szBuffer, DWORD dwLength );

//
// array item removal function(s) ( for one-dimensional array )
VOID DynArrayRemoveAll( TARRAY pArray );
BOOL DynArrayRemove( TARRAY pArray, DWORD dwIndex );

// ( for two-dimensional array )
BOOL DynArrayRemoveColumn( TARRAY pArray, DWORD dwRow, DWORD dwColumn );

//
// find value function(s) ( for one-dimensional array )
LONG DynArrayFindLong( TARRAY pArray, LONG lValue );
LONG DynArrayFindDWORD( TARRAY pArray, DWORD dwValue );
LONG DynArrayFindString( TARRAY pArray, LPCTSTR szValue, BOOL bIgnoreCase, DWORD dwCount );
LONG DynArrayFindFloat( TARRAY pArray, float fValue );
LONG DynArrayFindDouble( TARRAY pArray, double dblValue );
LONG DynArrayFindHandle( TARRAY pArray, HANDLE hValue );
LONG DynArrayFindSystemTime( TARRAY pArray, SYSTEMTIME stValue );
LONG DynArrayFindFileTime( TARRAY pArray, FILETIME ftValue );

// ( for two-dimensional array )
LONG DynArrayFindLong2( TARRAY pArray, DWORD dwRow, LONG lValue );
LONG DynArrayFindDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwValue );
LONG DynArrayFindString2( TARRAY pArray, DWORD dwRow,
                          LPCTSTR szValue, BOOL bIgnoreCase, DWORD dwCount );
LONG DynArrayFindFloat2( TARRAY pArray, DWORD dwRow, float fValue );
LONG DynArrayFindDouble2( TARRAY pArray, DWORD dwRow, double dblValue );
LONG DynArrayFindHandle2( TARRAY pArray, DWORD dwRow, HANDLE hValue );
LONG DynArrayFindSystemTime2( TARRAY pArray, DWORD dwRow, SYSTEMTIME stValue );
LONG DynArrayFindFileTime2( TARRAY pArray, DWORD dwRow, FILETIME ftValue );

// ( for two-dimensional array ... column wise searching )
LONG DynArrayFindLongEx( TARRAY pArray, DWORD dwColumn, LONG lValue );
LONG DynArrayFindDWORDEx( TARRAY pArray, DWORD dwColumn, DWORD dwValue );
LONG DynArrayFindStringEx( TARRAY pArray, DWORD dwColumn,
                           LPCTSTR szValue, BOOL bIgnoreCase, DWORD dwCount );
LONG DynArrayFindFloatEx( TARRAY pArray, DWORD dwColumn, float fValue );
LONG DynArrayFindDoubleEx( TARRAY pArray, DWORD dwColumn, double dblValue );
LONG DynArrayFindHandleEx( TARRAY pArray, DWORD dwColumn, HANDLE hValue );
LONG DynArrayFindSystemTimeEx( TARRAY pArray, DWORD dwColumn, SYSTEMTIME ftValue );
LONG DynArrayFindFileTimeEx( TARRAY pArray, DWORD dwColumn, FILETIME ftValue );

//
// array attachment helpers ( one-dimensional )
LONG DynArrayAppendEx( TARRAY pArray, TARRAY pArrItem );
LONG DynArrayInsertEx( TARRAY pArray, DWORD dwIndex, TARRAY pArrItem );
BOOL DynArraySetEx( TARRAY pArray, DWORD dwIndex, TARRAY pArrItem );

// ( for two-dimensional array )
LONG DynArrayAppendEx2( TARRAY pArray, DWORD dwRow, TARRAY pArrItem );
LONG DynArrayInsertEx2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, TARRAY pArrItem );
BOOL DynArraySetEx2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, TARRAY pArrItem );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                            COMMONAND LINE PARSING                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
//  constants / definitions / enumerations
//

// type ( dwType )
#define CP_TYPE_TEXT                    0x00000001              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 0001
#define CP_TYPE_NUMERIC                 0x00000002              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 0010
#define CP_TYPE_UNUMERIC                0x00000003              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 0011
#define CP_TYPE_DATE                    0x00000004              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 0100
#define CP_TYPE_TIME                    0x00000005              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 0101
#define CP_TYPE_DATETIME                0x00000006              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 0110
#define CP_TYPE_FLOAT                   0x00000007              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 0111
#define CP_TYPE_DOUBLE                  0x00000008              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 1000
#define CP_TYPE_CUSTOM                  0x00000009              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 1001
#define CP_TYPE_BOOLEAN                 0x0000000A              // XXXX XXXX XXXX XXXX XXXX XXXX 0000 1010
#define CP_TYPE_MASK                    0x000000FF              // XXXX XXXX XXXX XXXX XXXX XXXX 1111 1111

//
// version 1
//

#define CP_MODE_ARRAY                   0x00000100              // 0001 XXXX XXXX
#define CP_MODE_VALUES                  0x00000200              // 0010 XXXX XXXX
#define CP_MODE_MASK                    0x00000F00              // 1111 XXXX XXXX

#define CP_VALUE_OPTIONAL               0x00001000              // 0001 XXXX XXXX XXXX
#define CP_VALUE_MANDATORY              0x00002000              // 0010 XXXX XXXX XXXX
#define CP_VALUE_NODUPLICATES           0x00004000              // 0100 XXXX XXXX XXXX
#define CP_VALUE_NOLENGTHCHECK          0x00008000              // 1000 XXXX XXXX XXXX
#define CP_VALUE_MASK                   0x0000F000              // 1111 XXXX XXXX XXXX

#define CP_MAIN_OPTION                  0x00010000              // 0000 0000 0001 XXXX XXXX XXXX XXXX
#define CP_USAGE                        0x00020000              // 0000 0000 0010 XXXX XXXX XXXX XXXX
#define CP_DEFAULT                      0x00040000              // 0000 0000 0100 XXXX XXXX XXXX XXXX
#define CP_MANDATORY                    0x00080000              // 0000 0000 1000 XXXX XXXX XXXX XXXX
#define CP_CASESENSITIVE                0x00100000              // 0000 0001 0000 XXXX XXXX XXXX XXXX
#define CP_IGNOREVALUE                  0x00200000              // 0000 0010 0000 XXXX XXXX XXXX XXXX
#define CP_MASK                         0x0FFF0000              // 1111 1111 1111 XXXX XXXX XXXX XXXX

//
// version 2
//

#define CP2_MODE_ARRAY                  0x00000001              // XXXX XXXX XXXX XXXX XXXX XXXX XXXX 0001
#define CP2_MODE_VALUES                 0x00000002              // XXXX XXXX XXXX XXXX XXXX XXXX XXXX 0010
#define CP2_MODE_MASK                   0x0000000F              // XXXX XXXX XXXX XXXX XXXX XXXX XXXX 1111

#define CP2_VALUE_OPTIONAL              0x00000010              // XXXX XXXX XXXX XXXX 0000 0000 0001 XXXX
#define CP2_VALUE_NODUPLICATES          0x00000020              // XXXX XXXX XXXX XXXX 0000 0000 0010 XXXX
#define CP2_VALUE_TRIMINPUT             0x00000040              // XXXX XXXX XXXX XXXX 0000 0000 0100 XXXX
#define CP2_VALUE_NONULL                0x00000080              // XXXX XXXX XXXX XXXX 0000 0000 1000 XXXX
#define CP2_VALUE_MASK                  0x0000FFF0              // XXXX XXXX XXXX XXXX 1111 1111 1111 XXXX

#define CP2_ALLOCMEMORY                 0x00010000              // XXXX 0000 0000 0001 XXXX XXXX XXXX XXXX
#define CP2_USAGE                       0x00020000              // XXXX 0000 0000 0010 XXXX XXXX XXXX XXXX
#define CP2_DEFAULT                     0x00040000              // XXXX 0000 0000 0100 XXXX XXXX XXXX XXXX
#define CP2_MANDATORY                   0x00080000              // XXXX 0000 0000 1000 XXXX XXXX XXXX XXXX
#define CP2_CASESENSITIVE               0x00100000              // XXXX 0000 0001 0000 XXXX XXXX XXXX XXXX
#define CP2_MASK                        0x0FFF0000              // XXXX 1111 1111 1111 XXXX XXXX XXXX XXXX

//
// user defined types
//
typedef TCHAR OPTION[ 256 ];
typedef TCHAR OPTVALUE[ 256 ];

// custom value validation routine prototype
typedef BOOL ( *PARSERFUNC )( LPCWSTR pwszOption, LPCWSTR pwszValue, LPVOID pData );
typedef BOOL ( *PARSERFUNC2 )( LPCWSTR pwszOption, LPCWSTR pwszValue, LPVOID pData, DWORD* pdwIncrement );

// command line parser -- version 1
typedef struct __tagCmdParser
{
    OPTION szOption;                // options
    DWORD dwFlags;                  // flags ( specifies the type etc )
    DWORD dwCount;                  // tells the no. of times the option can repeat
    DWORD dwActuals;                // no. of times the option actually repeated
    LPVOID pValue;                  // pointer to the option value
    OPTVALUE szValues;              // to hold valid value in case flag = CP_MODE_VALUES
    PARSERFUNC pFunction;           // pointer to custom value validation function
    LPVOID pFunctionData;           // extra data that will be passed to custom function
} TCMDPARSER;

typedef TCMDPARSER* PTCMDPARSER;

extern const CHAR cszParserSignature[ 8 ];

// command line parser version 2
typedef struct __tagCmdParser2
{
    CHAR szSignature[ 8 ];          // should always be "PARSER2\0"

    DWORD dwType;                   // type of the argument
    DWORD64 dwFlags;                // flags

    DWORD dwCount;                  // tells the no. of times the option can repeat
    DWORD dwActuals;                // no. of times the option actually repeated

    LPCWSTR pwszOptions;            // options
    LPCWSTR pwszFriendlyName;       // specifies friendly name for the option
    LPCWSTR pwszValues;             // to hold valid value in case flag = CP_MODE_VALUES
                                    // format that can be used for this
                                    // CP_TYPE_TEXT         apple|orange|banana
                                    // ********************************************
                                    // CP_TYPE_NUMERIC      1,100;105;200,256;1000,
                                    // CP_TYPE_UNUMERIC     (this example tells
                                    //                       argument can be any
                                    //                       number from 1 to 100
                                    //                       or 105 or 200 or
                                    //                       256 or any value
                                    //                       greater than or
                                    //                       equal to 1000

    LPVOID pValue;                  // pointer to the option value
    DWORD dwLength;                 // max. length of argument value that can be stored
                                    // valid only for CP_TYPE_TEXT

    PARSERFUNC2 pFunction;          // pointer to custom value validation function
    LPVOID pFunctionData;           // extra data that will be passed to custom function

    DWORD dwReserved;               // reserver for future use
    LPVOID pReserved1;              // reserved for future use
    LPVOID pReserved2;              // reserved for future use
    LPVOID pReserved3;              // reserved for future use

} TCMDPARSER2;

typedef TCMDPARSER2* PTCMDPARSER2;

BOOL DoParseParam2( DWORD dwCount,
                    LPCWSTR argv[],
                    LONG lSubOptionIndex,
                    DWORD dwOptionsCount,
                    PTCMDPARSER2 pcmdOptions,
                    DWORD dwReserved );

//
// public function prototypes
//
LONG GetOptionCount( LPCTSTR szOption,
                     DWORD dwCount, PTCMDPARSER pcmdOptions );
BOOL DoParseParam( DWORD dwCount,
                   LPCTSTR argv[],
                   DWORD dwOptionsCount,
                   PTCMDPARSER pcmdOptions );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                        VALIDATING AND FILTERING RESULTS                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
//  constants / definitions / enumerations
//

// types
#define F_TYPE_TEXT                             0x00000001              // 0000 0000 0001
#define F_TYPE_NUMERIC                  0x00000002              // 0000 0000 0010
#define F_TYPE_UNUMERIC                 0x00000003              // 0000 0000 0011
#define F_TYPE_DATE                             0x00000004              // 0000 0000 0100
#define F_TYPE_TIME                             0x00000005              // 0000 0000 0101
#define F_TYPE_DATETIME                 0x00000006              // 0000 0000 0110
#define F_TYPE_FLOAT                    0x00000007              // 0000 0000 0111
#define F_TYPE_DOUBLE                   0x00000008              // 0000 0000 1000
#define F_TYPE_CUSTOM                   0x00000009              // 0000 0000 1001
#define F_TYPE_MASK                             0x00000FFF              // 1111 1111 1111

// modes
#define F_MODE_VALUES                   0x00001000              // 0001 XXXX XXXX XXXX
#define F_MODE_PATTERN                  0x00002000              // 0010 XXXX XXXX XXXX
#define F_MODE_ARRAY                    0x00004000              // 0100 XXXX XXXX XXXX

// custom filter data validation result
#define F_FILTER_INVALID                0x00000000
#define F_FILTER_VALID                  0x00000001
#define F_RESULT_KEEP                   0x000000FF
#define F_RESULT_REMOVE                 0x00000000

//
// Bit no. 2 is '='
// Bit no. 3 is '<'
// Bit no. 4 is '>'
//
//              7654 3210
// EQ   0000 0010               0x02
// NE   1111 XX0X               0xFD
// LT   0000 0100               0x04
// GT   0000 1000               0x08
// LE   0000 0110               0x06
// GE   0000 1010               0x0A
//
#define MASK_EQ         0x00000002
#define MASK_NE         0x000000FC
#define MASK_LT         0x00000004
#define MASK_GT         0x00000008
#define MASK_LE         0x00000006
#define MASK_GE         0x0000000A
#define MASK_ALL        0x000000FF

// mathematical operators
#define MATH_EQ         _T( "=" )
#define MATH_NE         _T( "!=" )
#define MATH_LT         _T( "<" )
#define MATH_GT         _T( ">" )
#define MATH_LE         _T( "<=" )
#define MATH_GE         _T( ">=" )

// parsed filters information
#define F_PARSED_INDEX_FILTER                   0
#define F_PARSED_INDEX_PROPERTY                 1
#define F_PARSED_INDEX_OPERATOR                 2
#define F_PARSED_INDEX_VALUE                    3

#define F_PARSED_INFO_COUNT                             4

//
// structures / user defined data types
//
typedef TCHAR OPERATORS[ 101 ];
typedef TCHAR FILTERVALUES[ 256 ];
typedef TCHAR FILTERPROPERTY[ 256 ];

// custom value validation routine prototype
typedef DWORD ( *FILTERFUNC )( LPCTSTR pszProperty, LPCTSTR pszOperator,
                                                           LPCTSTR pszValue, LPVOID pData, TARRAY arrRow );

typedef struct __tagFilterConfig
{
        DWORD dwColumn;                         // mapping from filter to column in data
        FILTERPROPERTY szProperty;      // filter property
        OPERATORS szOperators;          // valid operators for the filter
        DWORD dwFlags;                          // flags ( specifies the valid type for the filter )
        FILTERVALUES szValues;          // to hold valid value in case flag = F_MODE_VALUES
        FILTERFUNC pFunction;           // pointer to custom value validation function
        LPVOID pFunctionData;           // extra data that will be passed to custom function
} TFILTERCONFIG;

typedef TFILTERCONFIG *PTFILTERCONFIG;

//
// public function prototypes
//
LPCTSTR FindOperator( LPCTSTR szOperator );
BOOL ParseAndValidateFilter( DWORD dwCount,
                                                         PTFILTERCONFIG pfilterConfigs,
                                                         TARRAY arrFiltersArgs, PTARRAY parrParsedFilters );
BOOL CanFilterRecord( DWORD dwCount,
                                      TFILTERCONFIG filterConfigs[],
                                      TARRAY arrRecord, TARRAY arrParsedFilters );
DWORD FilterResults( DWORD dwCount,
                                         TFILTERCONFIG arrFilters[],
                                     TARRAY arrData, TARRAY arrParsedFilters );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                               DISPLAYING RESULTS                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
//  constants / definitions / enumerations
//

// formats
#define SR_FORMAT_LIST                          0x00000001              // 0000 0001
#define SR_FORMAT_TABLE                         0x00000002              // 0000 0010
#define SR_FORMAT_CSV                           0x00000003              // 0000 0011
#define SR_FORMAT_MASK                          0x0000000F              // 1111 1111

// column types
#define SR_TYPE_NONE                            0x00000000              // 0000 0000 0000 0000 XXXX XXXX
#define SR_TYPE_STRING                          0x00000010              // 0000 0000 0000 0001 XXXX XXXX
#define SR_TYPE_NUMERIC                         0x00000020              // 0000 0000 0000 0010 XXXX XXXX
#define SR_TYPE_FLOAT                           0x00000030              // 0000 0000 0000 0011 XXXX XXXX
#define SR_TYPE_DOUBLE                          0x00000040              // 0000 0000 0000 0100 XXXX XXXX
#define SR_TYPE_DATE                            0x00000050              // 0000 0000 0000 0101 XXXX XXXX
#define SR_TYPE_TIME                            0x00000060              // 0000 0000 0000 0110 XXXX XXXX
#define SR_TYPE_DATETIME                        0x00000070              // 0000 0000 0000 0111 XXXX XXXX
#define SR_TYPE_CUSTOM                          0x00000080              // 0000 0000 0000 1000 XXXX XXXX
#define SR_TYPE_MASK                            0x00000FF0              // 1111 1111 1111 1111 XXXX XXXX

// flags ( global level )
#define SR_NOHEADER                             0x00001000              // 0000 0001 XXXX XXXX XXXX XXXX XXXX XXXX

// flags ( column level )
#define SR_HIDECOLUMN                           0x00001000              // 0000 0000 0001 XXXX XXXX XXXX XXXX XXXX XXXX
#define SR_VALUEFORMAT                          0x00002000              // 0000 0000 0010 XXXX XXXX XXXX XXXX XXXX XXXX
#define SR_ARRAY                                0x00004000              // 0000 0000 0100 XXXX XXXX XXXX XXXX XXXX XXXX
#define SR_WORDWRAP                             0x00008000              // 0000 0000 1000 XXXX XXXX XXXX XXXX XXXX XXXX
#define SR_ALIGN_LEFT                           0x00010000              // 0000 0001 0000 XXXX XXXX XXXX XXXX XXXX XXXX
#define SR_ALIGN_RIGHT                          0x00020000              // 0000 0010 0000 XXXX XXXX XXXX XXXX XXXX XXXX
#define SR_ALIGN_CENTER                         0x00040000              // 0000 0100 0000 XXXX XXXX XXXX XXXX XXXX XXXX
#define SR_NO_TRUNCATION                        0x00088000              // 0000 1000 1000 XXXX XXXX XXXX XXXX XXXX XXXX
#define SR_SHOW_NA_WHEN_BLANK                   0x00100000              // 0001 0000 0000 XXXX XXXX XXXX XXXX XXXX XXXX

//
// user defined types
//

// custom value formatter
typedef TCHAR COLHEADER[ 256 ];
typedef TCHAR COLFORMAT[ 65 ];
typedef VOID ( *FORMATFUNC )( DWORD dwColumn, TARRAY arrData, LPVOID pData, LPTSTR szValue );

typedef struct __tagColumns
{
        COLHEADER szColumn;                             // column header name
        DWORD dwWidth;                                  // width of each field
        DWORD dwFlags;                                  // flags ( specifies the type etc )
        COLFORMAT szFormat;                             // custom format
        FORMATFUNC pFunction;                   // formatter function
        LPVOID pFunctionData;                   // function data
} TCOLUMNS;

typedef TCOLUMNS* PTCOLUMNS;

//
// public functions
//
VOID ShowResults( DWORD dwColumns, PTCOLUMNS pColumns, DWORD dwFlags, TARRAY arrData );

///////////////////////////////////////////////////////////////////////////////////
//                    COMING UP *** DO NOT USE UNLESS INTIMATED                  //
///////////////////////////////////////////////////////////////////////////////////
VOID ShowResults2( FILE* fp, DWORD dwColumns, PTCOLUMNS pColumns, DWORD dwFlags, TARRAY arrData );
///////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif  // __CMDLINE_H
