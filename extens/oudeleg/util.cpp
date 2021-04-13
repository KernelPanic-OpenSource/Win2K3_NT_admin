//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       util.cpp
//
//--------------------------------------------------------------------------


#include "pch.h"

#include "resource.h"

#include "util.h"

#include "delegWiz.h"

#include <_util.cpp>

VOID DisplayMessageBox(HWND hwnd, LPWSTR lpszText)
{
    CWString szTitle;
    szTitle.LoadFromResource(IDS_DELEGWIZ_WIZ_TITLE);
    ::MessageBox(hwnd,lpszText, szTitle, MB_OK);
}


//This function checks if current user has read and write
//access to the szObjectPath. If not it shows appropriate 
//Message box.
HRESULT InitCheckAccess( HWND hwndParent, LPCWSTR pszObjectLADPPath )
{
    HRESULT hr = S_OK;
    WCHAR szSDRightsProp[]      = L"sDRightsEffective";
    LPWSTR pProp = (LPWSTR)szSDRightsProp;
    PADS_ATTR_INFO pSDRightsInfo = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD dwAttributesReturned;
    IDirectoryObject *pDsObject = NULL;
    SECURITY_INFORMATION si = 0;

    //Check Permission to "Read Permission"
    hr = ::GetSDForDsObjectPath(IN const_cast<LPWSTR>(pszObjectLADPPath),
                                NULL,
                                &pSecurityDescriptor);
  

   if(FAILED(hr))
	{
        WCHAR szMsg[512];
        LoadStringHelper(IDS_DELEGWIZ_ERR_GET_SEC_INFO, szMsg, 512);
		DisplayMessageBox(hwndParent, szMsg);
      goto exit_gracefully;
	}
    
    // Bind to the object 
    hr = ADsOpenObjectHelper(pszObjectLADPPath,
                             IID_IDirectoryObject,
                             ADS_FAST_BIND,
                             (LPVOID*)&pDsObject);
    if( hr != S_OK )
        goto exit_gracefully;

    // Read the sDRightsEffective property to determine writability
    pDsObject->GetObjectAttributes(  &pProp,
                                     1,
                                     &pSDRightsInfo,
                                     &dwAttributesReturned);
    if (pSDRightsInfo)
    {
        si = pSDRightsInfo->pADsValues->Integer;
        FreeADsMem(pSDRightsInfo);
    }
    else
    {
        //
        // Note that GetObjectAttributes commonly returns S_OK even when
        // it fails, so the HRESULT is basically useless here.
        //
        // This can fail if we don't have read_property access, which can
        // happen when an admin is trying to restore access to an object
        // that has had all access removed or denied
        //
        // Assume we can write the Owner and DACL. If not, the worst that
        // happens is the user gets an "Access Denied" message when trying
        // to save changes.
        //
        si = DACL_SECURITY_INFORMATION;
    }

    if( !(si & DACL_SECURITY_INFORMATION) )
	{
		WCHAR szMsg[512];
        LoadStringHelper(IDS_DELEGWIZ_ERR_ACCESS_DENIED, szMsg, 512);
		DisplayMessageBox(hwndParent, szMsg);
        //NTRAID#NTBUG9-530206-2002/06/18-ronmart-PREFAST: Use E_FAIL
        //hr = !S_OK;
        hr = E_FAIL;
	}


exit_gracefully:
     if( pSecurityDescriptor )
        LocalFree(pSecurityDescriptor);
    if( pDsObject )
        pDsObject->Release();
    return hr;
}


DWORD
FormatStringID(LPTSTR *ppszResult, UINT idStr , ...)
{
    va_list args;
    va_start(args, idStr);
	TCHAR szFormat[1024];
	LoadStringHelper(idStr, szFormat, ARRAYSIZE(szFormat));
    return FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                         szFormat,
                         0,
                         0,
                         (LPTSTR)ppszResult,
                         1,
                         &args);
}

