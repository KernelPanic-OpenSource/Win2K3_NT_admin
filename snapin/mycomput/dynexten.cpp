/////////////////////////////////////////////////////////////////////
// DynExten.cpp : enumerates installed services on (possibly remote) computer
//
// HISTORY
// 30-Oct-97	JonN		Creation.
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("MMCFMGMT(dynexten.cpp)")

#include "compdata.h"
#include "cookie.h"
#include "regkey.h" // AMC::CRegKey

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TCHAR SERVICES_KEY[] = TEXT("System\\CurrentControlSet\\Control\\Server Applications");
const TCHAR CLSID_KEY[] = TEXT("Clsid\\");


HRESULT DynextenCheckInstall( const GUID& guidExtension, const TCHAR* pszExtension )
{
    HRESULT hr = S_OK;
#ifdef USE_CLASS_STORE
    IClassAccess* pIClassAccess = NULL;
#endif
    try
    {
        CString strRegPath = CLSID_KEY; // JonN 2/19/02 Security Push
        strRegPath += pszExtension;
        AMC::CRegKey regkeyInstalled;
        BOOL fFound = regkeyInstalled.OpenKeyEx( HKEY_CLASSES_ROOT, strRegPath, KEY_READ );
        if ( fFound )
        {
            return S_OK; // it is already installed
        }

// CODEWORK It would be more efficient to access the Class Store directly
// by calling CoGetClassAccess, then IClassAccess->GetAppInfo(), then CoInstall().
#ifdef USE_CLASS_STORE
        // now we have to get the Class Store to install it
        do { // false loop
            hr = CoGetClassAccess( &pIClassAccess );
            if ( FAILED(hr) )
                break;

            // now what???

        } while (FALSE); // false loop
#else
		IUnknown* pIUnknown = NULL;
		hr = ::CoCreateInstance( guidExtension,
		                         NULL,
		                         CLSCTX_INPROC,
		                         IID_IComponentData,
		                         (PVOID*)&pIUnknown );
		if (NULL != pIUnknown)
			pIUnknown->Release();
		// allow hr to fall through
#endif
    }
    catch (COleException* e)
    {
        // 2002/02/27-JonN I confirmed that this error path is called when
        // AMC::RegKey throws an exception
        e->Delete();
        return E_FAIL;
    }

#ifdef USE_CLASS_STORE
	if (NULL != pIClassAccess)
		pIClassAccess->Release();
#endif

    return hr;
}


//
// CMyComputerComponentData
//

static CLSID CLSID_DnsSnapin =
{ 0x80105023, 0x50B1, 0x11d1, { 0xB9, 0x30, 0x00, 0xA0, 0xC9, 0xA0, 0x6D, 0x2D } };

static CLSID CLSID_FileServiceManagementExt =	{0x58221C69,0xEA27,0x11CF,{0xAD,0xCF,0x00,0xAA,0x00,0xA8,0x00,0x33}};

HRESULT CMyComputerComponentData::ExpandServerApps(
	HSCOPEITEM hParent,
	CMyComputerCookie* pcookie )

{
	// ISSUE-2002/02/27-JonN check parameters

	try
	{
		AMC::CRegKey regkeyServices;
		BOOL fFound = TRUE;
		if (NULL == pcookie->QueryTargetServer())
		{
			fFound = regkeyServices.OpenKeyEx( HKEY_LOCAL_MACHINE, SERVICES_KEY, KEY_READ );
		}
		else
		{
			AMC::CRegKey regkeyRemoteComputer;
			regkeyRemoteComputer.ConnectRegistry(
			  const_cast<LPTSTR>(pcookie->QueryTargetServer()) );
			fFound = regkeyServices.OpenKeyEx( regkeyRemoteComputer, SERVICES_KEY, KEY_READ );
		}
		if ( !fFound )
		{
			return S_OK; // CODEWORK what return code?
		}
		CComQIPtr<IConsoleNameSpace2, &IID_IConsoleNameSpace2> pIConsoleNameSpace2
			= m_pConsole;
		if ( !pIConsoleNameSpace2 )
		{
			ASSERT(FALSE);
			return E_UNEXPECTED;
		}
		TCHAR achValue[ MAX_PATH ];
		DWORD iSubkey;
		// 2002/02/15-JonN Security Push: handle ERROR_MORE_DATA better
		// MYCOMPUT appears to be the only user of EnumValue at this time
		for ( iSubkey = 0;
		      true;
		      iSubkey++ )
		{
			ZeroMemory( achValue, sizeof(achValue) );
			DWORD cchValue = sizeof(achValue)/sizeof(TCHAR);
			HRESULT hr = regkeyServices.EnumValue(
				iSubkey,
				achValue,
				&cchValue );
			if (S_OK != hr)
			{
				if (ERROR_MORE_DATA == hr)
					continue;
				ASSERT(ERROR_NO_MORE_ITEMS == hr);
				break;
			}

			GUID guidExtension;
			hr = ::CLSIDFromString( achValue, &guidExtension );
			if ( !SUCCEEDED(hr) )
				continue;
			hr = DynextenCheckInstall( guidExtension, achValue );
			if ( !SUCCEEDED(hr) )
				continue;
			hr = pIConsoleNameSpace2->AddExtension( hParent, &guidExtension );
			// ignore the return value
		}
	}
    catch (COleException* e)
    {
        e->Delete();
		return S_OK; // CODEWORK what return code?
    }
	return S_OK;
}
