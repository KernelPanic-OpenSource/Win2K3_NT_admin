//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       UserCert.cxx
//
//  Contents:   
//
//  History:    12-November-97 BryanWal created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "commdlg.h"
#include "UserCert.h"
#include "pages.h"
#include "proppage.h"
#include "crtdbg.h"
#include "certifct.h"

#ifdef DSADMIN

CStrW GetSystemMessage (DWORD dwErr)
{
    CStrW message;

    LPVOID lpMsgBuf = NULL;
        
    // security review 2/26/2002 BryanWal ok - message is from system
    ::FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
            NULL,
            dwErr,
            MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
             (LPWSTR) &lpMsgBuf,    0,    NULL );
    message = (LPWSTR) lpMsgBuf;

    // Free the buffer.
    LocalFree (lpMsgBuf);

    return message;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsUserCertPage::CDsUserCertPage
//
//-----------------------------------------------------------------------------
CDsUserCertPage::CDsUserCertPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                 HWND hNotifyObj, DWORD dwFlags) 
    : CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags),
    m_hCertStore (0),
    m_hImageList (0),
    m_hbmCert (0),
    m_nCertImageIndex (0),
    m_nUserCerts (0),
    m_fUserStoreInitiallyEmpty (false)
{
    TRACE(CDsUserCertPage,CDsUserCertPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsUserCertPage");
#endif

    ::ZeroMemory (&m_selCertStruct, sizeof (m_selCertStruct));
}


//+----------------------------------------------------------------------------
//
//  Member:     CDsUserCertPage::~CDsUserCertPage
//
//-----------------------------------------------------------------------------
CDsUserCertPage::~CDsUserCertPage()
{
    TRACE(CDsUserCertPage,~CDsUserCertPage);

    // Clean up enumerated store list
    for (DWORD dwIndex = 0; dwIndex < m_selCertStruct.cDisplayStores; dwIndex++)
    {
        dspAssert (m_selCertStruct.rghDisplayStores);
        ::CertCloseStore (m_selCertStruct.rghDisplayStores[dwIndex], CERT_CLOSE_STORE_FORCE_FLAG);
    }
    if ( m_selCertStruct.rghDisplayStores )
        delete [] m_selCertStruct.rghDisplayStores;


    if ( m_hbmCert )
        DeleteObject (m_hbmCert);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateUserCertPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT CreateUserCertPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                             PWSTR pwzADsPath, PWSTR pwzClass,
                             HWND hNotifyObj, DWORD dwFlags,
                             const CDSSmartBasePathsInfo& basePathsInfo,
                             HPROPSHEETPAGE *phPage)
{
    TRACE_FUNCTION(CreateUserCertPage);

    CDsUserCertPage * pPageObj = new CDsUserCertPage(pDsPage, pDataObj,
                                                     hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, basePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CDsUserCertPage::DlgProc(HWND /*hDlg*/, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return InitDlg(lParam);

    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

    case WM_SHOWWINDOW:
        return OnShowWindow();

    case WM_SETFOCUS:
        return OnSetFocus((HWND)wParam);

    case WM_HELP:
        return OnHelp((LPHELPINFO)lParam);

    case WM_COMMAND:
        if (m_fInInit)
        {
            return TRUE;
        }
        return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam)));
    case WM_DESTROY:
        return OnDestroy();

    default:
        return(FALSE);
    }

    return(TRUE);
}


typedef struct _ENUM_ARG {
    DWORD               dwFlags;
    DWORD*              pcDisplayStores;          
    HCERTSTORE **       prghDisplayStores;        
} ENUM_ARG, *PENUM_ARG;

static BOOL WINAPI EnumStoresSysCallback(
    IN const void* pwszSystemStore,
    IN DWORD /*dwFlags*/,
    IN PCERT_SYSTEM_STORE_INFO /*pStoreInfo*/,
    IN OPTIONAL void * /*pvReserved*/,
    IN OPTIONAL void *pvArg
    )
{
    PENUM_ARG pEnumArg = (PENUM_ARG) pvArg;
    void*       pvPara = (void*)pwszSystemStore;



    HCERTSTORE  hNewStore  = ::CertOpenStore (CERT_STORE_PROV_SYSTEM, 
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, 
                CERT_SYSTEM_STORE_CURRENT_USER, pvPara);
    if ( !hNewStore )
    {
        hNewStore = ::CertOpenStore (CERT_STORE_PROV_SYSTEM, 
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, 
                CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG, pvPara);
    }
    if ( hNewStore )
    {
        DWORD       dwCnt = *(pEnumArg->pcDisplayStores);
        HCERTSTORE* phStores = 0;

        phStores = new HCERTSTORE[dwCnt+1];
        if ( phStores )
        {
            DWORD   dwIndex = 0;
            if ( *(pEnumArg->prghDisplayStores) )
            {
                for (; dwIndex < dwCnt; dwIndex++)
                {
                    phStores[dwIndex] = (*(pEnumArg->prghDisplayStores))[dwIndex];
                }
                delete [] (*(pEnumArg->prghDisplayStores));
            }
            (*(pEnumArg->pcDisplayStores))++;
            (*(pEnumArg->prghDisplayStores)) = phStores;
            (*(pEnumArg->prghDisplayStores))[dwIndex] = hNewStore;
        }
        else
        {
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    return TRUE;
}



//+----------------------------------------------------------------------------
//
//  Method:     CDsUserCertPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsUserCertPage::OnInitDialog(LPARAM /*lParam*/)
{
    TRACE(CDsUserCertPage,OnInitDialog);
    HRESULT hr = S_OK;
    CWaitCursor WaitCursor;
    const   LPWSTR  CERT_PROPERTY_NAME = L"UserCertificate";

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    // Get the object name and open its Published Certificate store
    LPCWSTR pszObjName = GetObjPathName ();
    dspAssert (pszObjName);
    if ( pszObjName )
    {
        // NTRAID#NTBUG9-706271-2002/09/24-ericb Forward slash in user name causes CertOpenStore failure.
        // This is because the object path returned by GetObjPathName is escaped using
        // ADSI escaping rules. CertOpenStore only requires LDAP escaping and fails on
        // ADSI escaping. Use the IADsPathname interface to remove any ADSI escaping.
        CComPtr<IADsPathname> pADsPath;
        hr = GetADsPathname(pADsPath);
        CHECK_HRESULT_REPORT(hr, GetHWnd(), return hr);
        hr = pADsPath->Set(CComBSTR(pszObjName), ADS_SETTYPE_FULL);
        CHECK_HRESULT_REPORT(hr, GetHWnd(), return hr);
        hr = pADsPath->put_EscapedMode(ADS_ESCAPEDMODE_OFF); // only do LDAP escaping, not ADSI escaping
        CHECK_HRESULT_REPORT(hr, GetHWnd(), return hr);
        CComBSTR bstr;
        hr = pADsPath->Retrieve(ADS_FORMAT_X500, &bstr);
        CStrW strObjName = bstr.m_str;
        strObjName += L"?";
        strObjName += CERT_PROPERTY_NAME;
        dspDebugOut((DEB_ITRACE, "Opening cert store for %ws\n", strObjName.GetBuffer(0)));

        m_hCertStore = ::CertOpenStore (CERT_STORE_PROV_LDAP,
            0, NULL,
            0,
            (void*) strObjName.GetBuffer(0));
        if ( !m_hCertStore )
        {
            DWORD dwErr = GetLastError ();
            hr = HRESULT_FROM_WIN32 (dwErr);
            if ( ERROR_ACCESS_DENIED == dwErr )
            {
                MessageBox (IDS_USER_TITLE_PUBLISHED_CERTS, IDS_CANT_OPEN_STORE_OPEN_READ_ONLY,
                        MB_ICONINFORMATION | MB_OK);
                
                m_hCertStore = ::CertOpenStore (CERT_STORE_PROV_LDAP,
                    0, NULL,
                    CERT_STORE_READONLY_FLAG,
                    (void*) strObjName.GetBuffer(0));
                if ( m_hCertStore )
                {
                    dwErr = 0;
                    hr = S_OK;
                    ::EnableWindow (GetDlgItem (GetHWnd (), IDC_ADD_FROM_STORE), FALSE);
                    ::EnableWindow (GetDlgItem (GetHWnd (), IDC_ADD_FROM_FILE), FALSE);
                }
                else
                {
                    dwErr = GetLastError ();
                    hr = HRESULT_FROM_WIN32 (dwErr);
                }
            }

            if ( FAILED (hr) )
            {
                DisplaySystemError (dwErr, IDS_CANT_OPEN_STORE);
                ::EnableWindow (GetDlgItem (GetHWnd (), IDC_ADD_FROM_STORE), FALSE);
                ::EnableWindow (GetDlgItem (GetHWnd (), IDC_ADD_FROM_FILE), FALSE);
            }
        }
    }

    // Set up result list view
    COLORREF    crMask = RGB (255, 0, 255);
    m_hImageList = ImageList_Create (16, 16, ILC_MASK, 10, 10);
    dspAssert (m_hImageList);
    if ( m_hImageList )
    {
        m_hbmCert = ::LoadBitmap (g_hInstance, MAKEINTRESOURCE (IDB_CERTIFICATE));
        dspAssert (m_hbmCert);
        if ( m_hbmCert )
        {
            m_nCertImageIndex = ImageList_AddMasked (m_hImageList, m_hbmCert,
                    crMask);
            dspAssert (m_nCertImageIndex != -1);
            if ( m_nCertImageIndex != -1 )
            {
                ListView_SetImageList (::GetDlgItem (GetHWnd (), IDC_CERT_LIST),
                        m_hImageList, LVSIL_SMALL);     
            }
        }
    }


    hr = AddListViewColumns ();
    if ( SUCCEEDED  (hr) && m_hCertStore )
        hr = PopulateListView ();

    EnableControls ();

    // Enumerate User's certificate stores for use in selecting certificates
    // from stores.
    ENUM_ARG    EnumArg;

    m_selCertStruct.dwSize = sizeof (CRYPTUI_SELECTCERTIFICATE_STRUCT);
    m_selCertStruct.hwndParent = GetHWnd ();
    EnumArg.pcDisplayStores = &m_selCertStruct.cDisplayStores;
    EnumArg.prghDisplayStores = &m_selCertStruct.rghDisplayStores;

    ::CertEnumSystemStore (CERT_SYSTEM_STORE_CURRENT_USER, 0, &EnumArg, 
            EnumStoresSysCallback);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUserCertPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserCertPage::OnApply(void)
{
    TRACE(CDsUserCertPage,OnApply);
    HRESULT hr = S_OK;
    CWaitCursor WaitCursor;

    // NTRAID#NTBUG9-541102-2002/08/01-ericb Adding, removing, then applying causes error.
    dspAssert(m_nUserCerts > -1);
    if (m_nUserCerts < 1 && m_fUserStoreInitiallyEmpty)
    {
        return PSNRET_NOERROR;
    }

    if ( m_hCertStore )
    {
        BOOL bResult = ::CertControlStore (m_hCertStore, CERT_STORE_CTRL_COMMIT_FORCE_FLAG, 
                CERT_STORE_CTRL_COMMIT, NULL);
        if ( !bResult )
        {
            DWORD   dwErr = GetLastError ();
            dspAssert (dwErr == ERROR_NOT_SUPPORTED);
            if ( dwErr != ERROR_NOT_SUPPORTED )
            {
                hr = E_FAIL;
                LPWSTR  pszCaption = 0;
                CStrW   strMsg;

                strMsg.FormatMessage(g_hInstance, IDS_CANT_SAVE_STORE, GetSystemMessage (dwErr));

                if ( LoadStringToTchar (IDS_USER_TITLE_PUBLISHED_CERTS, &pszCaption) )
                {
                    ::MessageBox (GetHWnd (), strMsg, pszCaption, MB_ICONINFORMATION | MB_OK);
                }
                else
                    hr = E_OUTOFMEMORY;

                if ( pszCaption )
                    delete [] pszCaption;
            }
        }
    }

    m_fUserStoreInitiallyEmpty = (0 == m_nUserCerts);

    return (SUCCEEDED(hr)) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUserCertPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserCertPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    switch (codeNotify)
    {
    case BN_CLICKED:
        switch (id)
        {
        case IDC_VIEW_CERT:
            return OnClickedViewCert ();
            break;

        case IDC_ADD_FROM_STORE:
            return OnClickedAddFromStore ();
            break;

        case IDC_ADD_FROM_FILE:
            return OnClickedAddFromFile ();
            break;

        case IDC_REMOVE:
            return OnClickedRemove ();
            break;

        case IDC_COPY_TO_FILE:
            return OnClickedCopyToFile ();
            break;

        default:
            _ASSERT (0);
            return E_UNEXPECTED;
            break;
        }
        break;

    default:
        break;
    }
    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}


//+----------------------------------------------------------------------------
//
//  Method:     CDsUserCertPage::OnNotify
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserCertPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pNMHdr = (LPNMHDR) lParam;
    _ASSERT (pNMHdr);
    if ( !pNMHdr )
        return E_POINTER;

    switch (pNMHdr->code)
    {
    case NM_DBLCLK:
        if (m_fInInit)
            return TRUE;
        else if ( wParam == IDC_CERT_LIST )
            return OnDblClkCertList (pNMHdr);
        break;

    case LVN_COLUMNCLICK:
        if (m_fInInit)
            return TRUE;
        else if ( wParam == IDC_CERT_LIST )
            return OnColumnClickCertList ((LPNMLISTVIEW) lParam);
        break;

    case LVN_DELETEALLITEMS:
        if (m_fInInit)
            return TRUE;
        else if ( wParam == IDC_CERT_LIST )
            return FALSE;   // Do not suppress LVN_DELETEITEM messages
        break;

    case LVN_DELETEITEM:
        if (m_fInInit)
            return TRUE;
        else if ( wParam == IDC_CERT_LIST )
            return OnDeleteItemCertList ((LPNMLISTVIEW) lParam);
        break;

    case LVN_ODSTATECHANGED:
        OnNotifyStateChanged ((LPNMLVODSTATECHANGE) lParam);
        return 0;

    case LVN_ITEMCHANGED:
        OnNotifyItemChanged ((LPNMLISTVIEW) lParam); 
        return 0;
 
    default:
        break;
    }

    return CDsPropPageBase::OnNotify(wParam, lParam);
}


//+----------------------------------------------------------------------------
//
//  Method:     CDsUserCertPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserCertPage::OnDestroy(void)
{
    ListView_DeleteAllItems (::GetDlgItem (GetHWnd (), IDC_CERT_LIST));
    CDsPropPageBase::OnDestroy();

    if ( m_hCertStore )
    {
        // Back out of uncommitted changes before closing the store.
        BOOL    bResult = ::CertControlStore (m_hCertStore, 
            CERT_STORE_CTRL_COMMIT_CLEAR_FLAG, 
            CERT_STORE_CTRL_COMMIT, NULL);
        if ( !bResult )
        {
            DWORD   dwErr = GetLastError ();
            dspAssert (dwErr != ERROR_NOT_SUPPORTED && dwErr != ERROR_CALL_NOT_IMPLEMENTED);
        }
        ::CertCloseStore (m_hCertStore, 0);
        m_hCertStore = 0;
    }

    // If an application processes this message, it should return zero.
    return 0;
}




HRESULT CDsUserCertPage::AddListViewColumns()
{
    // Add list view columns
    LVCOLUMN    lvCol;
    ::ZeroMemory (&lvCol, sizeof (lvCol));
    PTSTR       ptsz = 0;

    if ( !LoadStringToTchar (IDS_CERTCOL_ISSUED_TO, &ptsz) )
    {
        ReportError(GetLastError(), 0, GetHWnd());
        return E_OUTOFMEMORY;
    }
    
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
    lvCol.fmt = LVCFMT_LEFT;
    lvCol.cx = 90;
    lvCol.pszText = ptsz;     
    lvCol.iSubItem = CERTCOL_ISSUED_TO; 
    HWND    hWndList = ::GetDlgItem (GetHWnd (), IDC_CERT_LIST);
    int nIndex = ListView_InsertColumn (hWndList, CERTCOL_ISSUED_TO, &lvCol);
    _ASSERT (nIndex != -1);
    delete [] ptsz;
    if ( nIndex == -1 )
        return E_UNEXPECTED;

    if ( !LoadStringToTchar (IDS_CERTCOL_ISSUED_BY, &ptsz) )
    {
        ReportError (GetLastError (), 0, GetHWnd ());
        return E_OUTOFMEMORY;
    }
    lvCol.cx = 90;
    lvCol.pszText = ptsz;     
    lvCol.iSubItem = CERTCOL_ISSUED_BY; 
    nIndex = ListView_InsertColumn (hWndList, IDS_CERTCOL_ISSUED_BY, &lvCol);
    _ASSERT (nIndex != -1);
    delete [] ptsz;
    if ( nIndex == -1 )
        return E_UNEXPECTED;


    if ( !LoadStringToTchar (IDS_CERTCOL_PURPOSES, &ptsz) )
    {
        ReportError (GetLastError (), 0, GetHWnd ());
        return E_OUTOFMEMORY;
    }
    lvCol.cx = 125;
    lvCol.pszText = ptsz;     
    lvCol.iSubItem = CERTCOL_PURPOSES; 
    nIndex = ListView_InsertColumn (hWndList, IDS_CERTCOL_PURPOSES, &lvCol);
    _ASSERT (nIndex != -1);
    delete [] ptsz;
    if ( nIndex == -1 )
        return E_UNEXPECTED;

    if ( !LoadStringToTchar (IDS_CERTCOL_EXP_DATE, &ptsz) )
    {
        ReportError (GetLastError (), 0, GetHWnd ());
        return E_OUTOFMEMORY;
    }
    lvCol.cx = 125;
    lvCol.pszText = ptsz;     
    lvCol.iSubItem = CERTCOL_EXP_DATE; 
    nIndex = ListView_InsertColumn (hWndList, IDS_CERTCOL_EXP_DATE, &lvCol);
    _ASSERT (nIndex != -1);
    delete [] ptsz;
    if ( nIndex == -1 )
        return E_UNEXPECTED;

    return S_OK;
}

HRESULT CDsUserCertPage::OnClickedViewCert()
{
    HRESULT hr = S_OK;
    int             nSelItem = -1;
    CCertificate*   pCert = GetSelectedCertificate (nSelItem);
    if ( pCert )
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCT  vcs;
        HCERTSTORE                      hCertStore = ::CertDuplicateStore (pCert->GetCertStore ());


        ::ZeroMemory (&vcs, sizeof (vcs));
        vcs.dwSize = sizeof (vcs);
        vcs.hwndParent = GetHWnd ();
        vcs.dwFlags = 0;
        vcs.cStores = 1;
        vcs.rghStores = &hCertStore;
        vcs.pCertContext = pCert->GetCertContext ();

        BOOL fPropertiesChanged = FALSE;
        BOOL bResult = ::CryptUIDlgViewCertificate (&vcs, &fPropertiesChanged);
        if ( bResult )
        {
            if ( fPropertiesChanged )
            {
                pCert->Refresh ();
                RefreshItemInList (pCert, nSelItem);
            }
        }
        ::CertCloseStore (hCertStore, 0);
    }
    
    ::SetFocus (::GetDlgItem (GetHWnd (), IDC_CERT_LIST));
    return hr;
}

HRESULT CDsUserCertPage::OnClickedAddFromStore()
{
    HRESULT                             hr = S_OK;

    PCCERT_CONTEXT  pCertContext = ::CryptUIDlgSelectCertificate (&m_selCertStruct);
    if ( pCertContext )
    {
        hr = AddCertToStore (pCertContext);
        SetDirty();
    }
    

    ::SetFocus (::GetDlgItem (GetHWnd (), IDC_CERT_LIST));
    return hr;
}

HRESULT CDsUserCertPage::OnClickedAddFromFile()
{
    HRESULT         hr = S_OK;
    HWND            hwndList = ::GetDlgItem (GetHWnd (), IDC_CERT_LIST);


    PWSTR   pszFilter = 0;
    PWSTR   pszDlgTitle = 0;
    if ( LoadStringToTchar (IDS_CERT_SAVE_FILTER, &pszFilter) &&
            LoadStringToTchar (IDS_OPEN_FILE_DLG_TITLE, &pszDlgTitle) )
    {
        LPWSTR          pszDefExt = _T("cer");
        OPENFILENAME    ofn;
        WCHAR           szFile[MAX_PATH];
        WCHAR           chReplace = pszFilter[wcslen (pszFilter)-1]; // retrieve wild character
        for (int i = 0; pszFilter[i]; i++)
        {
            if ( pszFilter[i] == chReplace )
                pszFilter[i] = 0;
        }


        ::ZeroMemory (szFile, MAX_PATH * sizeof (WCHAR));
        ::ZeroMemory (&ofn, sizeof (ofn));
        ofn.lStructSize = sizeof (OPENFILENAME);     
        ofn.hwndOwner = GetHWnd ();
        ofn.lpstrFilter = pszFilter;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;     
        ofn.lpstrTitle = pszDlgTitle;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; 
        ofn.lpstrDefExt = pszDefExt;     


        BOOL bResult = GetOpenFileName (&ofn);
        if ( bResult )
        {
            DWORD           dwMsgAndCertEncodingType = 0;
            DWORD           dwContentType = 0;
            DWORD           dwFormatType = 0;
            PCERT_CONTEXT   pCertContext = 0;

            bResult = ::CryptQueryObject (
                    CERT_QUERY_OBJECT_FILE,
                    (void *) ofn.lpstrFile,
                    CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
                        CERT_QUERY_CONTENT_FLAG_CERT |
                        CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED | 
                        CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED |  
                        CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                    CERT_QUERY_FORMAT_FLAG_ALL,
                    0,
                    &dwMsgAndCertEncodingType,
                    &dwContentType,
                    &dwFormatType,
                    NULL,
                    NULL,
                    (const void **) &pCertContext);
            if ( bResult && pCertContext )
            {       
                hr = AddCertToStore (pCertContext);
//              ::CertFreeCertificateContext (pCertContext);
                SetDirty();
            }
            else
            {
                LPWSTR  pszCaption = 0;
                LPWSTR  pszMsg = 0;

                if ( LoadStringToTchar (IDS_OPEN_FILE_DLG_TITLE, &pszCaption) &&
                        LoadStringToTchar (IDS_UNKNOWN_CERT_FILE_TYPE, &pszMsg) )
                {
                    ::MessageBox (GetHWnd (), pszMsg, pszCaption, MB_ICONWARNING | MB_OK);
                }
                else
                    hr = E_OUTOFMEMORY;

                if ( pszCaption )
                    delete [] pszCaption;

                if ( pszMsg )
                    delete [] pszMsg;
            }
        }
    }

    if ( pszFilter )
        delete [] pszFilter;
    if ( pszDlgTitle )
        delete [] pszDlgTitle;

    ::SetFocus (hwndList);
    return hr;
}

HRESULT CDsUserCertPage::OnClickedRemove()
{
    HRESULT         hr = S_OK;
    int             nSelItem = -1;
    HWND            hwndList = ::GetDlgItem (GetHWnd (), IDC_CERT_LIST);
    bool            bConfirmationRequested = false;
    int             iResult = 0;
    int             nSelCnt = ListView_GetSelectedCount (hwndList);

    if ( nSelCnt < 1 )
        return E_FAIL;

    while (1)
    {
        CCertificate*   pCert = GetSelectedCertificate (nSelItem);
        if ( pCert )
        {
            if ( !bConfirmationRequested )
            {
                LPWSTR  pszCaption = 0;
                LPWSTR  pszText = 0;
                int     textId = 0;

                if ( 1 == nSelCnt )
                    textId = IDS_CONFIRM_DELETE_CERT;
                else
                    textId = IDS_CONFIRM_DELETE_CERTS;

                if ( LoadStringToTchar (textId, &pszText) &&
                        LoadStringToTchar (IDS_REMOVE_CERT, &pszCaption) )
                {
                    iResult = ::MessageBox (GetHWnd (), pszText, pszCaption,
                            MB_YESNO);
                }
                if ( pszCaption )
                    delete [] pszCaption;
                if ( pszText )
                    delete [] pszText;
                bConfirmationRequested = true;
                if ( IDYES != iResult )
                    break;
            }

            BOOL bResult = pCert->DeleteFromStore ();
            dspAssert (bResult);
            if ( bResult )
            {
                // NTRAID#NTBUG9-541102-2002/08/01-ericb Adding, removing, then applying causes error.
                m_nUserCerts--;

                bResult = ListView_DeleteItem (
                        hwndList, 
                        nSelItem);
                dspAssert (bResult);
                if ( bResult )
                    SetDirty ();
                else
                    hr = E_FAIL;
            }
            else
            {
                DWORD dwErr = GetLastError ();
                DisplaySystemError (dwErr, IDS_REMOVE_CERT);
                hr = HRESULT_FROM_WIN32 (dwErr);
                break;
            }
        }
        else
            break;
    }

    ::SetFocus (hwndList);
    EnableControls ();

    return hr;
}

HRESULT CDsUserCertPage::OnClickedCopyToFile()
{
    HRESULT         hr = S_OK;

    PWSTR   pszFilter = 0;
    PWSTR   pszDlgTitle = 0;
    if ( LoadStringToTchar (IDS_CERT_SAVE_FILTER, &pszFilter) &&
            LoadStringToTchar (IDS_SAVE_FILE_DLG_TITLE, &pszDlgTitle) )
    {
        LPWSTR          pszDefExt = _T("cer");
        OPENFILENAME    ofn;
        WCHAR           szFile[MAX_PATH];
        WCHAR           chReplace = pszFilter[wcslen (pszFilter)-1]; // retrieve wild character
        for (int i = 0; pszFilter[i]; i++)
        {
            if ( pszFilter[i] == chReplace )
                pszFilter[i] = 0;
        }


        ::ZeroMemory (szFile, MAX_PATH * sizeof (WCHAR));
        ::ZeroMemory (&ofn, sizeof (ofn));
        ofn.lStructSize = sizeof (OPENFILENAME);     
        ofn.hwndOwner = GetHWnd ();
        ofn.lpstrFilter = pszFilter;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;     
        ofn.lpstrTitle = pszDlgTitle;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY; 
        ofn.lpstrDefExt = pszDefExt;     

        BOOL bResult = ::GetSaveFileName (&ofn);
        //NTRAID#NTBUG9-572003-2002/03/10-jmessec    GetSaveFileName may fail due to an internal error (versus user cancellation); error is
        //silently ignored and processing stopped, possibly causing fear and confusion on the part of the user
        if ( bResult )
        {
            if ( wcsstr (_wcsupr (ofn.lpstrFile), _T(".CER")) )
            {
                HANDLE hFile = ::CreateFile (ofn.lpstrFile, // pointer to name of the file
                      GENERIC_READ | GENERIC_WRITE,         // access (read-write) mode
                      0,                                    // share mode
                      NULL,                                 // pointer to security attributes
                      CREATE_ALWAYS,                        // how to create
                      FILE_ATTRIBUTE_NORMAL,                // file attributes
                      NULL);                                // handle to file with attributes to copy
                dspAssert (hFile != INVALID_HANDLE_VALUE);
                if ( hFile != INVALID_HANDLE_VALUE )
                {
                    int iSelItem = -1;

                    CCertificate* pCert = GetSelectedCertificate (iSelItem);
                    dspAssert (pCert);
                    if ( pCert )
                    {
                        // To cer file -> put out encoded blob
                        // pbEncodedCert
                        hr = pCert->WriteToFile (hFile);
                        if ( !SUCCEEDED (hr) )
                            DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
                    }

                    if ( !CloseHandle (hFile) )
                    {
                        dspAssert (0);
                        DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
                    }
                }
                else
                    DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
            }
            else
            {
                void* pvSaveToPara = (void*) ofn.lpstrFile;

                HCERTSTORE hCertStore = ::CertOpenStore (CERT_STORE_PROV_MEMORY, 
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, 
                        CERT_FILE_STORE_COMMIT_ENABLE_FLAG, 0);
                if ( hCertStore )
                {
                    int iSelItem = -1;

                    CCertificate* pCert = GetSelectedCertificate (iSelItem);
                    dspAssert (pCert);
                    if ( pCert )
                    {
                        bResult = ::CertAddCertificateContextToStore (
                                hCertStore,
                                ::CertDuplicateCertificateContext (pCert->GetCertContext ()),
                                CERT_STORE_ADD_ALWAYS,
                                NULL);
                        dspAssert (bResult);
                        if ( bResult )
                        {
                            bResult = ::CertSaveStore (
                                    hCertStore,
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    CERT_STORE_SAVE_AS_PKCS7,
                                    CERT_STORE_SAVE_TO_FILENAME,
                                    pvSaveToPara,
                                    0);
                            dspAssert (bResult);
                            if ( !bResult )
                                DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
                        }
                        else
                            DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
                    }
                    ::CertCloseStore (hCertStore, 0);
                }
            }
        }
    }
    else
        hr = E_OUTOFMEMORY;

    if ( pszFilter )
        delete [] pszFilter;
    if ( pszDlgTitle )
        delete [] pszDlgTitle;
    
    ::SetFocus (::GetDlgItem (GetHWnd (), IDC_CERT_LIST));
    return hr;
}

HRESULT CDsUserCertPage::OnDblClkCertList(LPNMHDR /*pNMHdr*/)
{
    HRESULT hr = S_OK;
    
    hr = OnClickedViewCert ();
    return hr;
}

HRESULT CDsUserCertPage::OnColumnClickCertList(LPNMLISTVIEW /*pNMListView*/)
{
    HRESULT hr = S_OK;
    
    return hr;
}

HRESULT CDsUserCertPage::OnDeleteItemCertList (LPNMLISTVIEW pNMListView)
{
    HRESULT hr = S_OK;

    dspAssert (pNMListView);
    if ( pNMListView )
    {
        LVITEM  lvItem;

        ::ZeroMemory (&lvItem, sizeof (lvItem));

        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = pNMListView->iItem;

        if ( ListView_GetItem (::GetDlgItem (GetHWnd (), IDC_CERT_LIST), &lvItem) )
        {
            CCertificate* pCert = (CCertificate*) lvItem.lParam;
            dspAssert (pCert);
            if ( pCert )
            {
                delete pCert;
            }
            else
                hr = E_UNEXPECTED;
        }
        else
            hr = E_UNEXPECTED;
    }
    else
        hr = E_POINTER;

    return hr;
}

HRESULT CDsUserCertPage::PopulateListView()
{
    CWaitCursor         cursor;
    PCCERT_CONTEXT      pCertContext = 0;
    HRESULT             hr = S_OK;
    CCertificate*       pCert = 0;

    //  Iterate through the list of certificates in the system store,
    //  allocate new certificates with the CERT_CONTEXT returned,
    //  and store them in the certificate list.
    int nItem = -1;
    while ( SUCCEEDED (hr) )
    {
        pCertContext = ::CertEnumCertificatesInStore (m_hCertStore, pCertContext);
        if ( !pCertContext )
            break;

        pCert = new CCertificate (pCertContext, m_hCertStore);
        if ( pCert )
        {
            nItem++;
            // NTRAID#NTBUG9-541102-2002/08/01-ericb Adding, removing, then applying causes error.
            m_nUserCerts++;

            hr = InsertCertInList (pCert, nItem);
            if ( !SUCCEEDED (hr) )
                delete pCert;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // NTRAID#NTBUG9-541102-2002/08/01-ericb Adding, removing, then applying causes error.
    if (0 == m_nUserCerts)
    {
        m_fUserStoreInitiallyEmpty = true;
    }

    return hr;
}

// Get the first selected certificate, starting at the end of the list
// and previous to the passed in nSelItem.  Pass in a -1 to search
// the entire list
CCertificate* CDsUserCertPage::GetSelectedCertificate (int& nSelItem)
{
    HWND            hWndList = ::GetDlgItem (GetHWnd (), IDC_CERT_LIST);
    int             nCnt = ListView_GetItemCount (hWndList);
    CCertificate*   pCert = 0;
    int             nSelCnt = ListView_GetSelectedCount (hWndList);
    LVITEM          lvItem;

    ::ZeroMemory (&lvItem, sizeof (lvItem));
    lvItem.mask = LVIF_PARAM;


    if ( nSelCnt >= 1 )
    {
        if ( -1 != nSelItem )
            nCnt = nSelItem;

        while (--nCnt >= 0)
        {
            UINT    flag = ListView_GetItemState (hWndList, 
                nCnt, LVIS_SELECTED);
            if ( flag & LVNI_SELECTED )
            {
                lvItem.iItem = nCnt;

                if ( ListView_GetItem (::GetDlgItem (GetHWnd (), 
                        IDC_CERT_LIST),
                        &lvItem) )
                {
                    pCert = (CCertificate*) lvItem.lParam;
                    dspAssert (pCert);
                    if ( pCert )
                    {
                        nSelItem = nCnt;
                    }
                }
                else
                {
                    dspAssert (0);
                }
                break; 
            }
        }
    }

    return pCert;
}

void CDsUserCertPage::RefreshItemInList (CCertificate * /*pCert*/, int nItem)
{
    HWND            hWndList = ::GetDlgItem (GetHWnd (), IDC_CERT_LIST);
    BOOL bResult = ListView_Update (hWndList, nItem);
    dspAssert (bResult);
}


HRESULT CDsUserCertPage::InsertCertInList(CCertificate * pCert, int nItem)
{
    HRESULT         hr = S_OK;
    HWND            hWndList = ::GetDlgItem (GetHWnd (), IDC_CERT_LIST);
    LVITEM          lvItem;
    PWSTR           pszText = 0;
    int             nIndex = 0;


    // Insert icon and subject name
    ::ZeroMemory (&lvItem, sizeof (lvItem));
    lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvItem.iItem = nItem;
    lvItem.iSubItem = CERTCOL_ISSUED_TO;
    lvItem.iImage = m_nCertImageIndex;
    lvItem.lParam = (LPARAM) pCert;
    hr = pCert->GetSubjectName (&pszText);
    if ( SUCCEEDED (hr) )
        lvItem.pszText = pszText;
    else
    {
        hr = pCert->GetAlternateSubjectName (&pszText);
        if ( SUCCEEDED (hr) )
            lvItem.pszText = pszText;
    }
    if ( SUCCEEDED (hr) )
    {
        nIndex = ListView_InsertItem (hWndList, &lvItem);       
        _ASSERT (nIndex != -1);
        if ( nIndex == -1 )
        {
            delete pCert;
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        delete pCert;
        hr = E_UNEXPECTED;
    }


    if ( SUCCEEDED (hr) )
    {
        // Insert issuer name
        ::ZeroMemory (&lvItem, sizeof (lvItem));
        HRESULT hResultToo = pCert->GetIssuerName (&pszText);
        if ( !SUCCEEDED (hResultToo) )
        {
            hr = pCert->GetAlternateIssuerName (&pszText);
        }
        if ( SUCCEEDED (hResultToo) )
        {
            ListView_SetItemText (hWndList, nIndex, CERTCOL_ISSUED_BY, pszText);
        }
    }

    // Insert intended purpose
    if ( SUCCEEDED (hr) )
    {
        HRESULT hResultToo = pCert->GetEnhancedKeyUsage (&pszText);
        if ( SUCCEEDED (hResultToo) && pszText )
        {
            ListView_SetItemText (hWndList, nIndex, CERTCOL_PURPOSES, pszText);
        }
    }

    // Insert expiration date
    if ( SUCCEEDED (hr) )
    {
        HRESULT hResultToo = pCert->GetValidNotAfter (&pszText);
        if ( SUCCEEDED (hResultToo) )
        {
            ListView_SetItemText (hWndList, nIndex, CERTCOL_EXP_DATE, pszText);
        }
    }

    if ( pszText )
        delete [] pszText;

    return hr;
}

void CDsUserCertPage::DisplaySystemError(DWORD dwErr, int iCaptionText)
{
    LPVOID lpMsgBuf = 0;


    //NTRAID#NTBUG9-571997-2002/03/10-jmessec   FormatMessage can throw an exception if allocating buffer; not handled here,
    //is this by design?
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
            NULL,
            dwErr,
            MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf, 0, NULL);
        
    // Display the string.
    PWSTR   pszCaption = 0;
    if ( LoadStringToTchar (IDS_MSG_TITLE, & pszCaption) )
    {
        PWSTR pszText = 0;
        if ( LoadStringToTchar (iCaptionText, &pszText) )
        {
            //NTRAID#NTBUG9-569720-2002/03/10-jmessec   buffer length calculation uses magic number which is highly dependent
            //upon hard-coded strings later in code; and is wrong anyway (looks like it should be 3, not 4)

            PWSTR pszFinalText = new WCHAR[wcslen ((PWSTR) lpMsgBuf) +
                    wcslen (pszText) + 4];
            if ( pszFinalText )
            {
                wcscpy (pszFinalText, pszText);

                if ( HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER) != dwErr )
                {
                    //NTRAID#NTBUG9-571073-2002/03/10-jmessec   Non-localized UI Text hard-coded in code.
                    wcscat (pszFinalText, L"  ");
                    wcscat (pszFinalText, (PWSTR) lpMsgBuf);
                }
                ::MessageBox (GetHWnd (), pszFinalText, pszCaption, 
                        MB_ICONWARNING | MB_OK);
                delete [] pszFinalText;
            }
            delete [] pszText;
        }
        delete [] pszCaption;
    }

    // Free the buffer.
    LocalFree (lpMsgBuf);
}

void CDsUserCertPage::EnableControls()
{
    HWND    hWndDlg = GetHWnd ();
    HWND    hWndList = ::GetDlgItem (hWndDlg, IDC_CERT_LIST);
    int     nSelCnt = ListView_GetSelectedCount (hWndList);
    int     nSelItem = -1;
    bool    bCanDelete = true;

    while (bCanDelete)
    {
        CCertificate*   pCert = GetSelectedCertificate (nSelItem);
        if ( pCert )
            bCanDelete = pCert->CanDelete ();
        else
            break;
    }

    ::EnableWindow (::GetDlgItem (hWndDlg, IDC_REMOVE), bCanDelete && nSelCnt > 0);
    ::EnableWindow (::GetDlgItem (hWndDlg, IDC_COPY_TO_FILE), nSelCnt == 1);
    ::EnableWindow (::GetDlgItem (hWndDlg, IDC_VIEW_CERT), nSelCnt == 1);
}

void CDsUserCertPage::OnNotifyStateChanged(LPNMLVODSTATECHANGE /*pStateChange*/)
{
    EnableControls ();
}

void CDsUserCertPage::OnNotifyItemChanged (LPNMLISTVIEW /*pnmv*/)
{
    EnableControls ();
}

HRESULT CDsUserCertPage::AddCertToStore(PCCERT_CONTEXT pCertContext)
{
    HRESULT hr = S_OK;

    if ( pCertContext )
    {
        BOOL bResult = ::CertAddCertificateContextToStore (
                    m_hCertStore,
                    pCertContext,
                    CERT_STORE_ADD_NEW,
                    0);
        if ( bResult )
        {
            DWORD   cbData = 20;
            BYTE    certHash[20];
            BOOL bReturn = ::CertGetCertificateContextProperty (
                    pCertContext,
                    CERT_SHA1_HASH_PROP_ID,
                    certHash,
                    &cbData);
            ASSERT (bReturn);
            if ( bReturn )
            {
                CRYPT_DATA_BLOB blob = {sizeof (certHash), certHash};
                PCCERT_CONTEXT pNewCertContext = CertFindCertificateInStore(
                    m_hCertStore,
                    0,
                    0,
                    CERT_FIND_SHA1_HASH,
                    &blob,
                    0);
                if ( pNewCertContext )
                {
                    CCertificate* pCert = new CCertificate (pNewCertContext, m_hCertStore); 
                    if ( pCert )
                    {
                        // NTRAID#NTBUG9-541102-2002/08/01-ericb Adding, removing, then applying causes error.
                        m_nUserCerts++;

                        hr = InsertCertInList (pCert, 
                                ListView_GetItemCount (
                                ::GetDlgItem (GetHWnd (), IDC_CERT_LIST)));
                        if ( !SUCCEEDED (hr) )
                            delete pCert;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    ::CertFreeCertificateContext (pNewCertContext);
                }
            }
        }
        else
        {
            DWORD   dwErr = GetLastError ();
            if ( dwErr == CRYPT_E_EXISTS )
            {
                LPWSTR  pszMsg = 0;
                LPWSTR  pszCaption = 0;

                if ( LoadStringToTchar (IDS_DUPLICATE_CERT, &pszMsg) &&
                        LoadStringToTchar (IDS_ADD_FROM_STORE, &pszCaption) )
                {
                    ::MessageBox (GetHWnd (), pszMsg, pszCaption, 
                            MB_ICONINFORMATION | MB_OK);
                    hr = E_FAIL;
                }
                else
                    hr = E_OUTOFMEMORY;

                if ( pszMsg )
                    delete [] pszMsg;
                if ( pszCaption )
                    delete [] pszCaption;
            }
            else
            {
                DisplaySystemError (dwErr, IDS_ADD_FROM_STORE);
                hr = HRESULT_FROM_WIN32 (dwErr);
            }
        }

        ::CertFreeCertificateContext (pCertContext);
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

int CDsUserCertPage::MessageBox(int caption, int text, UINT flags)
{
    int iReturn = -1;

    LPWSTR  pszMsg = 0;
    LPWSTR  pszCaption = 0;

    if ( LoadStringToTchar (text, &pszMsg) && 
            LoadStringToTchar (caption, &pszCaption) )
    {
        iReturn = ::MessageBox (GetHWnd (), pszMsg, pszCaption, flags);
    }

    if ( pszMsg )
        delete [] pszMsg;
    if ( pszCaption )
        delete [] pszCaption;

    return iReturn;
}

#endif // DSADMIN

