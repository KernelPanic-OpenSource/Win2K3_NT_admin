/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2002.
//
//  File:       TemplateV2RequestPropertyPage.cpp
//
//  Contents:   Implementation of CTemplateV2RequestPropertyPage
//
//----------------------------------------------------------------------------
// TemplateV2RequestPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "TemplateV2RequestPropertyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTemplateV2RequestPropertyPage property page
enum {
    REQUEST_PURPOSE_SIGNATURE = 0,
    REQUEST_PURPOSE_ENCRYPTION,
    REQUEST_PURPOSE_SIGNATURE_AND_ENCRYPTION,
    REQUEST_PURPOSE_SIGNATURE_AND_SMARTCARD_LOGON
};

const DWORD CERTTMPL_NON_LOCAL_CSP = (DWORD) -1;

CTemplateV2RequestPropertyPage::CTemplateV2RequestPropertyPage(
        CCertTemplate& rCertTemplate, bool& rbIsDirty) : 
    CHelpPropertyPage(CTemplateV2RequestPropertyPage::IDD),
    m_rCertTemplate (rCertTemplate),
    m_rbIsDirty (rbIsDirty),
    m_nProvDSSCnt (0)
{
    _TRACE (1, L"Entering CTemplateV2RequestPropertyPage::CTemplateV2RequestPropertyPage ()\n");
    //{{AFX_DATA_INIT(CTemplateV2RequestPropertyPage)
    //}}AFX_DATA_INIT
    m_rCertTemplate.AddRef ();

    _TRACE (-1, L"Leaving CTemplateV2RequestPropertyPage::CTemplateV2RequestPropertyPage ()\n");
}

CTemplateV2RequestPropertyPage::~CTemplateV2RequestPropertyPage()
{
    _TRACE (1, L"Entering CTemplateV2RequestPropertyPage::~CTemplateV2RequestPropertyPage ()\n");

    while ( !m_CSPList.IsEmpty () )
    {
        CT_CSP_DATA* pCSPData = m_CSPList.RemoveHead ();
        if ( pCSPData )
            delete pCSPData;
    }

   m_rCertTemplate.Release ();
    _TRACE (-1, L"Leaving CTemplateV2RequestPropertyPage::~CTemplateV2RequestPropertyPage ()\n");
}

void CTemplateV2RequestPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CHelpPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTemplateV2RequestPropertyPage)
    DDX_Control(pDX, IDC_MINIMUM_KEYSIZE_VALUE, m_minKeySizeCombo);
    DDX_Control(pDX, IDC_PURPOSE_COMBO, m_purposeCombo);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTemplateV2RequestPropertyPage, CHelpPropertyPage)
    //{{AFX_MSG_MAP(CTemplateV2RequestPropertyPage)
    ON_CBN_SELCHANGE(IDC_PURPOSE_COMBO, OnSelchangePurposeCombo)
    ON_BN_CLICKED(IDC_EXPORT_PRIVATE_KEY, OnExportPrivateKey)
    ON_BN_CLICKED(IDC_ARCHIVE_KEY_CHECK, OnArchiveKeyCheck)
    ON_BN_CLICKED(IDC_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK, OnIncludeSymmetricAlgorithmsCheck)
    ON_CBN_SELCHANGE(IDC_MINIMUM_KEYSIZE_VALUE, OnSelchangeMinimumKeysizeValue)
    ON_BN_CLICKED(IDC_DELETE_PERMANENTLY, OnDeletePermanently)
    ON_BN_CLICKED(IDC_CSPS, OnCsps)
    ON_BN_CLICKED(IDC_ENROLL_WITHOUT_INPUT, OnEnrollWithoutInput)
    ON_BN_CLICKED(IDC_ENROLL_PROMPT_USER, OnEnrollPromptUser)
    ON_BN_CLICKED(IDC_ENROLL_PROMPT_USER_REQUIRE_IF_PRIVATE_KEY, OnEnrollPromptUserRequireIfPrivateKey)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTemplateV2RequestPropertyPage message handlers


BOOL CTemplateV2RequestPropertyPage::OnInitDialog() 
{
    _TRACE (1, L"Entering CTemplateV2RequestPropertyPage::OnInitDialog ()\n");
    CHelpPropertyPage::OnInitDialog();

    CString text;

    VERIFY (text.LoadString (IDS_SIGNATURE));
    int nIndex = m_purposeCombo.AddString (text);
    if ( nIndex >= 0 )
    {
        m_purposeCombo.SetItemData (nIndex, (DWORD_PTR) REQUEST_PURPOSE_SIGNATURE);
        if ( m_rCertTemplate.HasKeySpecSignature () )
            m_purposeCombo.SetCurSel (nIndex);
    }

    VERIFY (text.LoadString (IDS_ENCRYPTION));
    nIndex = m_purposeCombo.AddString (text);
    if ( nIndex >= 0 )
    {
        m_purposeCombo.SetItemData (nIndex, (DWORD_PTR) REQUEST_PURPOSE_ENCRYPTION);
        if ( m_rCertTemplate.HasEncryptionSignature () )
            m_purposeCombo.SetCurSel (nIndex);
    }
    
    VERIFY (text.LoadString (IDS_SIGNATURE_AND_ENCRYPTION));
    nIndex = m_purposeCombo.AddString (text);
    if ( nIndex >= 0 )
    {
        // NTRAID# 278356  CertSRV: No CSPs in mmc certificate snapin advanced 
        // option list with v2 templates that have ENC and SIG as purpose.
        bool bHasDigitalSignature = false;

        m_rCertTemplate.GetDigitalSignature (bHasDigitalSignature);

        m_purposeCombo.SetItemData (nIndex, (DWORD_PTR) REQUEST_PURPOSE_SIGNATURE_AND_ENCRYPTION);
        if ( m_rCertTemplate.HasEncryptionSignature () && 
                (bHasDigitalSignature || m_rCertTemplate.HasKeySpecSignature ()) )
            m_purposeCombo.SetCurSel (nIndex);
    }

    // NTRAID# 476615 Certtmpl: Signature and Smart Card Logon Certificate 
    // Purpose should only be available for User Templates.
    if ( !m_rCertTemplate.IsMachineType () )
    {
        VERIFY (text.LoadString (IDS_SIGNATURE_AND_SMARTCARD_LOGON));
        nIndex = m_purposeCombo.AddString (text);
        if ( nIndex >= 0 )
        {
            // NTRAID# 278356  CertSRV: No CSPs in mmc certificate snapin advanced 
            // option list with v2 templates that have ENC and SIG as purpose.
            bool bHasDigitalSignature = false;
            bool bHasOnlyDigitalSignature = false;

            m_rCertTemplate.GetDigitalSignature (bHasDigitalSignature,
                    &bHasOnlyDigitalSignature);

            m_purposeCombo.SetItemData (nIndex, (DWORD_PTR) REQUEST_PURPOSE_SIGNATURE_AND_SMARTCARD_LOGON);
            if ( bHasOnlyDigitalSignature && 
                    bHasDigitalSignature && m_rCertTemplate.HasEncryptionSignature () )
            {
                m_purposeCombo.SetCurSel (nIndex);
            }
        }
    }


    // Initialize minimum key size combo box- values in powers of 2 from 512 to 16384
    DWORD   dwMinKeySize = 0;
    m_rCertTemplate.GetMinimumKeySize (dwMinKeySize);
    AddKeySizeToCombo(512, L"512", dwMinKeySize);
    AddKeySizeToCombo(768, L"768", dwMinKeySize);
    AddKeySizeToCombo(1024, L"1024", dwMinKeySize);
    AddKeySizeToCombo(2048, L"2048", dwMinKeySize);
    AddKeySizeToCombo(4096, L"4096", dwMinKeySize);
    AddKeySizeToCombo(8192, L"8192", dwMinKeySize);
    AddKeySizeToCombo(16384, L"16384", dwMinKeySize);

    if ( SUCCEEDED (EnumerateCSPs (dwMinKeySize)) )
    {

    }

    if ( m_rCertTemplate.PrivateKeyIsExportable () )
        SendDlgItemMessage (IDC_EXPORT_PRIVATE_KEY, BM_SETCHECK, BST_CHECKED);

    if ( m_rCertTemplate.AllowPrivateKeyArchival () )
        SendDlgItemMessage (IDC_ARCHIVE_KEY_CHECK, BM_SETCHECK, BST_CHECKED);

    if ( m_rCertTemplate.IncludeSymmetricAlgorithms () )
        SendDlgItemMessage (IDC_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK, BM_SETCHECK, BST_CHECKED);

    GetDlgItem (IDC_ARCHIVE_KEY_CHECK)->ShowWindow (SW_SHOW);
    GetDlgItem (IDC_MINIMUM_KEYSIZE_VALUE)->ShowWindow (SW_SHOW);
    GetDlgItem (IDC_MINIMUM_KEYSIZE_LABEL)->ShowWindow (SW_SHOW);
    GetDlgItem (IDC_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK)->ShowWindow (SW_SHOW);


    if ( m_rCertTemplate.UserInteractionRequired () )
    {
        if ( m_rCertTemplate.StrongKeyProtectionRequired () )
            SendDlgItemMessage (IDC_ENROLL_PROMPT_USER_REQUIRE_IF_PRIVATE_KEY, BM_SETCHECK, BST_CHECKED);
        else
            SendDlgItemMessage (IDC_ENROLL_PROMPT_USER, BM_SETCHECK, BST_CHECKED);
    }
    else
    {
        SendDlgItemMessage (IDC_ENROLL_WITHOUT_INPUT, BM_SETCHECK, BST_CHECKED);
    }
      

    if ( m_rCertTemplate.RemoveInvalidCertFromPersonalStore () )
        SendDlgItemMessage (IDC_DELETE_PERMANENTLY, BM_SETCHECK, BST_CHECKED);

    EnableControls ();

    _TRACE (-1, L"Leaving CTemplateV2RequestPropertyPage::OnInitDialog ()\n");
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CTemplateV2RequestPropertyPage::EnableControls ()
{
    if (  m_rCertTemplate.ReadOnly () )
    {
        GetDlgItem (IDC_PURPOSE_COMBO)->EnableWindow (FALSE);

        GetDlgItem (IDC_EXPORT_PRIVATE_KEY)->EnableWindow (FALSE);

        //version 2 fields
        GetDlgItem (IDC_ARCHIVE_KEY_CHECK)->EnableWindow (FALSE);
        GetDlgItem (IDC_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK)->EnableWindow (FALSE);
        SendDlgItemMessage (IDC_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK, BM_SETCHECK, BST_UNCHECKED);
        GetDlgItem (IDC_MINIMUM_KEYSIZE_LABEL)->EnableWindow (FALSE);
        GetDlgItem (IDC_MINIMUM_KEYSIZE_VALUE)->EnableWindow (FALSE);
        GetDlgItem (IDC_DELETE_PERMANENTLY)->EnableWindow (FALSE);
        GetDlgItem (IDC_ENROLL_WITHOUT_INPUT)->EnableWindow (FALSE);
        GetDlgItem (IDC_ENROLL_PROMPT_USER)->EnableWindow (FALSE);
        GetDlgItem (IDC_ENROLL_PROMPT_USER_REQUIRE_IF_PRIVATE_KEY)->EnableWindow (FALSE);

        GetDlgItem (IDC_PURPOSE_LABEL)->EnableWindow (FALSE);
        GetDlgItem (IDC_INPUT_LABEL)->EnableWindow (FALSE);
        GetDlgItem (IDC_CSP_LABEL)->EnableWindow (FALSE);
    }
    else
    {
        BOOL bEncryptionSelected = FALSE; 
        int nIndex = m_purposeCombo.GetCurSel ();

        
        if ( nIndex >= 0 )
        {
            switch (m_purposeCombo.GetItemData (nIndex))
            {
            case REQUEST_PURPOSE_SIGNATURE:
                GetDlgItem (IDC_ENROLL_WITHOUT_INPUT)->EnableWindow (TRUE);
                GetDlgItem (IDC_EXPORT_PRIVATE_KEY)->EnableWindow (TRUE);
                bEncryptionSelected = FALSE;
                break;

            case REQUEST_PURPOSE_SIGNATURE_AND_SMARTCARD_LOGON:
                GetDlgItem (IDC_ENROLL_WITHOUT_INPUT)->EnableWindow (FALSE);
                GetDlgItem (IDC_EXPORT_PRIVATE_KEY)->EnableWindow (FALSE);
                bEncryptionSelected = FALSE;
                break;

            case REQUEST_PURPOSE_SIGNATURE_AND_ENCRYPTION:
            case REQUEST_PURPOSE_ENCRYPTION:
                GetDlgItem (IDC_ENROLL_WITHOUT_INPUT)->EnableWindow (TRUE);
                GetDlgItem (IDC_EXPORT_PRIVATE_KEY)->EnableWindow (TRUE);
                bEncryptionSelected = TRUE;
                break;

            default:
                _ASSERT (0);
                break;
            }
        }

        GetDlgItem (IDC_DELETE_PERMANENTLY)->EnableWindow (!bEncryptionSelected);
        if ( bEncryptionSelected && 
                BST_CHECKED == SendDlgItemMessage (IDC_DELETE_PERMANENTLY, BM_GETCHECK) )
        {
            SendDlgItemMessage (IDC_DELETE_PERMANENTLY, BM_SETCHECK, BST_UNCHECKED);
            m_rCertTemplate.SetRemoveInvalidCertFromPersonalStore (false);
        }

        GetDlgItem (IDC_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK)->EnableWindow (bEncryptionSelected);
        if ( !bEncryptionSelected )
        {
            SendDlgItemMessage (IDC_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK, BM_SETCHECK, BST_UNCHECKED);
        }

        BOOL bEnableArchiveKeyCheck = bEncryptionSelected;

        if ( m_nProvDSSCnt > 0 )
            bEnableArchiveKeyCheck = FALSE;

        if ( bEnableArchiveKeyCheck )
        {
            GetDlgItem (IDC_ARCHIVE_KEY_CHECK)->EnableWindow (TRUE);
        }
        else
        {
            SendDlgItemMessage (IDC_ARCHIVE_KEY_CHECK, BM_SETCHECK, BST_UNCHECKED);
            OnArchiveKeyCheck ();   // clear flag
            GetDlgItem (IDC_ARCHIVE_KEY_CHECK)->EnableWindow (FALSE);
        }

        if ( m_rCertTemplate.IsMachineType () || m_rCertTemplate.SubjectIsCA () ||
                m_rCertTemplate.SubjectIsCrossCA () )
        {
            GetDlgItem (IDC_ENROLL_WITHOUT_INPUT)->EnableWindow (FALSE);
            GetDlgItem (IDC_ENROLL_PROMPT_USER)->EnableWindow (FALSE);
            GetDlgItem (IDC_ENROLL_PROMPT_USER_REQUIRE_IF_PRIVATE_KEY)->EnableWindow (FALSE);
        }
    }
}


HRESULT CTemplateV2RequestPropertyPage::EnumerateCSPs(DWORD dwMinKeySize)
{
    _TRACE (1, L"Entering CTemplateV2RequestPropertyPage::EnumerateCSPs\n");
    HRESULT hr = S_OK;
    for (DWORD dwIndex = 0; ;dwIndex++)
    {
        DWORD   cbName = 0;
        DWORD   dwProvType = 0;

        if ( CryptEnumProviders (dwIndex, NULL, 0, &dwProvType, NULL, &cbName) )
        {
            PWSTR  pszTypeName = new WCHAR[cbName];
            if ( pszTypeName )
            {
                if ( CryptEnumProviders (dwIndex, NULL, 0, &dwProvType,
                        pszTypeName, &cbName) )
                {
                    DWORD   dwSigMaxKey = (DWORD) -1;
                    DWORD   dwKeyExMaxKey = (DWORD) -1;

                    CSPGetMaxKeySupported (pszTypeName, dwProvType, dwSigMaxKey, dwKeyExMaxKey);
                    // If either of these values is still -1, then it was not 
                    // set.  Set to 0.
                    if ( -1 == dwSigMaxKey )
                        dwSigMaxKey = 0;
                    if ( -1 == dwKeyExMaxKey )
                        dwKeyExMaxKey = 0;
                    CT_CSP_DATA* pNewData = new CT_CSP_DATA (pszTypeName, 
                            dwProvType, dwSigMaxKey, dwKeyExMaxKey);
                    if ( pNewData )
                    {
                        m_CSPList.AddTail (pNewData);
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32 (GetLastError ());
                    _TRACE (0, L"CryptEnumProviderTypes () failed: 0x%x\n", hr);
                    break;
                }

                delete [] pszTypeName;
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            _TRACE (0, L"CryptEnumProviderTypes () failed: 0x%x\n", hr);
            break;
        }
    }


    int     nCSPIndex = 0;
    CString szCSP;
    CString szInvalidCSPs;

    // Set the selection
    // Get the list of selected CSPs from the cert template.
    // If a selected CSP is not found in the generated CSP list, add it to the list
    m_nProvDSSCnt = 0;
    nCSPIndex = 0;
    while ( SUCCEEDED (m_rCertTemplate.GetCSP (nCSPIndex, szCSP)) )
    {
        bool    bFound = false;
        for (POSITION nextPos = m_CSPList.GetHeadPosition (); nextPos; )
        {
            CT_CSP_DATA* pCSPData = m_CSPList.GetNext (nextPos);
            if ( pCSPData )
            {
                if ( !LocaleStrCmp (szCSP, pCSPData->m_szName) )
                {
                    pCSPData->m_bSelected = true;
                    if ( PROV_DSS == pCSPData->m_dwProvType || 
                            PROV_DSS_DH == pCSPData->m_dwProvType )
                    {
                        m_nProvDSSCnt++;
                    }
                    bFound = true;
                    break;
                }
            }
        }

        if ( !bFound )
        {
            if ( !szInvalidCSPs.IsEmpty () )
                szInvalidCSPs += L", ";
            szInvalidCSPs += szCSP;

            // Should show CSPs that are not available on the client
            CT_CSP_DATA* pNewData = new CT_CSP_DATA (szCSP, 
                    CERTTMPL_NON_LOCAL_CSP, 0, 0);
            if ( pNewData )
            {
                pNewData->m_bSelected = true;
                m_CSPList.AddTail (pNewData);
            }
        }
        nCSPIndex++;
    }

    if ( !szInvalidCSPs.IsEmpty () )
    {
        CString text;
        CString caption;
        CThemeContextActivator activator;

        VERIFY (caption.LoadString (IDS_CERTTMPL));
        text.FormatMessage (IDS_CSPS_NOT_INSTALLED_LOCALLY, (PCWSTR) szInvalidCSPs);

        MessageBox (text, caption, MB_OK | MB_ICONWARNING);
    }

    NormalizeCSPList (dwMinKeySize);

    EnableControls ();

    _TRACE (-1, L"Entering CTemplateV2RequestPropertyPage::EnumerateCSPs: 0x%x\n", hr);
    return hr;
}



void CTemplateV2RequestPropertyPage::OnSelchangePurposeCombo() 
{
    int nIndex = m_purposeCombo.GetCurSel ();
    if ( nIndex >= 0 )
    {
        // NTRAID# 278356  CertSRV: No CSPs in mmc certificate snapin advanced 
        // option list with v2 templates that have ENC and SIG as purpose.
        switch (m_purposeCombo.GetItemData (nIndex))
        {
        case REQUEST_PURPOSE_SIGNATURE:
            m_rCertTemplate.SetEncryptionSignature (false);
            m_rCertTemplate.SetKeySpecSignature (true);
            m_rCertTemplate.IncludeSymmetricAlgorithms (false);
            m_rCertTemplate.AllowPrivateKeyArchival (false);
            break;

        case REQUEST_PURPOSE_ENCRYPTION:
            m_rCertTemplate.SetEncryptionSignature (true);
            m_rCertTemplate.SetKeySpecSignature (false);
            m_rCertTemplate.SetDigitalSignature (false);
            break;

        case REQUEST_PURPOSE_SIGNATURE_AND_ENCRYPTION:
            m_rCertTemplate.SetEncryptionSignature (true);
            m_rCertTemplate.SetKeySpecSignature (false);
            m_rCertTemplate.SetDigitalSignature (true);
            break;

        case REQUEST_PURPOSE_SIGNATURE_AND_SMARTCARD_LOGON:
            m_rCertTemplate.SetKeySpecSignature (false);
            m_rCertTemplate.SetEncryptionSignature (true);
            m_rCertTemplate.SetDigitalSignature (true, true);
            // 499389 CertTMPL: Signature and Smartcard Logon purpose does not 
            // clear CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS flag
            m_rCertTemplate.IncludeSymmetricAlgorithms (false);
            if ( BST_CHECKED == SendDlgItemMessage (IDC_ENROLL_WITHOUT_INPUT, BM_GETCHECK) )
            {
                m_rCertTemplate.SetUserInteractionRequired (true);
                m_rCertTemplate.SetStrongKeyProtectionRequired (false);
                SendDlgItemMessage (IDC_ENROLL_WITHOUT_INPUT, BM_SETCHECK, BST_UNCHECKED);
                SendDlgItemMessage (IDC_ENROLL_PROMPT_USER, BM_SETCHECK, BST_CHECKED);
            }

            // smart card keys not exportable
            SendDlgItemMessage (IDC_EXPORT_PRIVATE_KEY, BM_SETCHECK, BST_UNCHECKED);
            m_rCertTemplate.MakePrivateKeyExportable (false);
            break;

        default:
            _ASSERT (0);
            break;
        }
    }


    int nSel = m_minKeySizeCombo.GetCurSel ();
    ASSERT (nSel >= 0);
    if ( nSel >= 0 )
    {
        DWORD   dwMinKeySize = (DWORD) m_minKeySizeCombo.GetItemData (nSel);
        ASSERT (dwMinKeySize > 0);

        // Clear CSP list and add only values that correspond, saving 
        // selection for conforming CSPs
        int nInitialSelCnt = GetSelectedCSPCount ();
        NormalizeCSPList (dwMinKeySize);
        int nFinalSelCnt = GetSelectedCSPCount ();

        if ( nInitialSelCnt > 0 && 0 == nFinalSelCnt )
        {
            CString caption;
            CString text;

            VERIFY (caption.LoadString (IDS_CSP_SELECTION));
            VERIFY (text.LoadString (IDS_NO_SELECTED_CSPS));

            if ( IDYES == MessageBox (text, caption, MB_YESNO) )
            {
                OnCsps();
            }
        }
    }


    SetModified ();
    m_rbIsDirty = true;
    EnableControls ();
}

void CTemplateV2RequestPropertyPage::AddKeySizeToCombo(DWORD dwValue, PCWSTR strValue, DWORD dwSizeToSelect)
{
    int nIndex = m_minKeySizeCombo.AddString (strValue);
    if ( nIndex >= 0 )
    {
        m_minKeySizeCombo.SetItemData (nIndex, dwValue);
        if ( dwSizeToSelect == dwValue )
            m_minKeySizeCombo.SetCurSel (nIndex);
    }
}

void CTemplateV2RequestPropertyPage::OnExportPrivateKey() 
{
    bool bMakeExportable = (BST_CHECKED == SendDlgItemMessage (IDC_EXPORT_PRIVATE_KEY, BM_GETCHECK));
    
    m_rCertTemplate.MakePrivateKeyExportable (bMakeExportable);
    SetModified ();
    m_rbIsDirty = true;
}

void CTemplateV2RequestPropertyPage::OnArchiveKeyCheck() 
{
    bool bAllowKeyArchival = (BST_CHECKED == SendDlgItemMessage (IDC_ARCHIVE_KEY_CHECK, BM_GETCHECK));
    
    m_rCertTemplate.AllowPrivateKeyArchival (bAllowKeyArchival);
    SetModified ();
    m_rbIsDirty = true;
}

void CTemplateV2RequestPropertyPage::OnIncludeSymmetricAlgorithmsCheck() 
{
    bool bInclude = 
            (BST_CHECKED == SendDlgItemMessage (IDC_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK, BM_GETCHECK));
    
    m_rCertTemplate.IncludeSymmetricAlgorithms (bInclude);
    SetModified ();
    m_rbIsDirty = true;
}

void CTemplateV2RequestPropertyPage::OnSelchangeMinimumKeysizeValue() 
{
    SetModified ();
    m_rbIsDirty = true;

    int nSel = m_minKeySizeCombo.GetCurSel ();
    ASSERT (nSel >= 0);
    if ( nSel >= 0 )
    {
        DWORD   dwMinKeySize = (DWORD) m_minKeySizeCombo.GetItemData (nSel);
        ASSERT (dwMinKeySize > 0);
        HRESULT hr = m_rCertTemplate.SetMinimumKeySizeValue (dwMinKeySize);
        if ( FAILED (hr) )
        {
            CString text;
            CString caption;
            CThemeContextActivator activator;

            VERIFY (caption.LoadString (IDS_CERTTMPL));
            // security review 2/20/2002 BryanWal ok
            text.FormatMessage (IDS_CANNOT_WRITE_MINKEYSIZE, hr);

            MessageBox (text, caption, MB_OK | MB_ICONWARNING);
        }


        // Clear CSP list and add only values that correspond, saving checks
        int nInitialSelCnt = GetSelectedCSPCount ();
        NormalizeCSPList (dwMinKeySize);
        int nFinalSelCnt = GetSelectedCSPCount ();

        if ( nInitialSelCnt > 0 && 0 == nFinalSelCnt )
        {
            CString caption;
            CString text;

            VERIFY (caption.LoadString (IDS_CSP_SELECTION));
            VERIFY (text.LoadString (IDS_NO_SELECTED_CSPS));

            if ( IDYES == MessageBox (text, caption, MB_YESNO) )
            {
                OnCsps();
            }
        }
    }
}


void CTemplateV2RequestPropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE(1, L"Entering CTemplateV2RequestPropertyPage::DoContextHelp\n");
    
    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_STATIC:
    case IDC_MINIMUM_KEYSIZE_LABEL:
    case IDC_INPUT_LABEL:
        break;

    default:
        // Display context help for a control
        if ( !::WinHelp (
                hWndControl,
                GetContextHelpFile (),
                HELP_WM_HELP,
                (DWORD_PTR) g_aHelpIDs_IDD_TEMPLATE_V2_REQUEST) )
        {
            _TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
        }
        break;
    }
    _TRACE(-1, L"Leaving CTemplateV2RequestPropertyPage::DoContextHelp\n");
}


void CTemplateV2RequestPropertyPage::OnDeletePermanently() 
{
    m_rCertTemplate.SetRemoveInvalidCertFromPersonalStore (
            BST_CHECKED == SendDlgItemMessage (IDC_DELETE_PERMANENTLY, BM_GETCHECK));
    SetModified ();
    m_rbIsDirty = true;
}


HRESULT CTemplateV2RequestPropertyPage::CSPGetMaxKeySupported (
        PCWSTR pszProvider, 
        DWORD dwProvType, 
        DWORD& rdwSigMaxKey, 
        DWORD& rdwKeyExMaxKey)
{
    _TRACE (1, L"Entering CTemplateV2RequestPropertyPage::CSPGetMaxKeySupported (%s)\n",
            pszProvider);
    HRESULT     hr = S_OK;
    HCRYPTPROV  hProv = 0;

    BOOL bResult = ::CryptAcquireContext (&hProv,
            NULL,
            pszProvider,
            dwProvType,
            CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    if ( bResult )
    {
        PROV_ENUMALGS_EX EnumAlgs;     //   Structure to hold information on 
                                   //   a supported algorithm
        DWORD dFlag = CRYPT_FIRST;     //   Flag indicating that the first
                                       //   supported algorithm is to be
                                       //   enumerated. Changed to 0 after the
                                       //   first call to the function.
        DWORD   cbData = sizeof(PROV_ENUMALGS_EX);

        while (CryptGetProvParam(
                hProv,              // handle to an open cryptographic provider
                PP_ENUMALGS_EX, 
                (BYTE *)&EnumAlgs,  // information on the next algorithm
                &cbData,            // number of bytes in the PROV_ENUMALGS_EX
                dFlag))             // flag to indicate whether this is a first or
                                    // subsequent algorithm supported by the
                                    // CSP.
        {
            if ( ALG_CLASS_SIGNATURE == GET_ALG_CLASS (EnumAlgs.aiAlgid) )
                rdwSigMaxKey = EnumAlgs.dwMaxLen;

            if ( ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS (EnumAlgs.aiAlgid) )
                rdwKeyExMaxKey = EnumAlgs.dwMaxLen;

            if ( -1 != rdwSigMaxKey && -1 != rdwKeyExMaxKey )
                break;  // both have been set

            dFlag = 0;          // Set to 0 after the first call,
        } //  end of while loop. When all of the supported algorithms have
          //  been enumerated, the function returns FALSE.

        ::CryptReleaseContext (hProv, 0);
    }
    else
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"CryptAcquireContext () failed: 0x%x\n", dwErr);
        hr = HRESULT_FROM_WIN32 (dwErr);
    }

    _TRACE (-1, L"Leaving CTemplateV2RequestPropertyPage::CSPGetMaxKeySupported (%s)\n",
            pszProvider);
    return hr;
}

// NTRAID# 313348 Cert Template UI: Need to warn the user if the template 
// minimum key length is not supported by the CSPs
void CTemplateV2RequestPropertyPage::NormalizeCSPList (DWORD dwMinKeySize)
{
    bool bSignatureOnly = false;
    int nIndex = m_purposeCombo.GetCurSel ();
    if ( nIndex >= 0 )
        bSignatureOnly = (REQUEST_PURPOSE_SIGNATURE == m_purposeCombo.GetItemData (nIndex));

    // Check for conforming CSPs
    for (POSITION nextPos = m_CSPList.GetHeadPosition (); nextPos; )
    {
        CT_CSP_DATA* pCSPData = m_CSPList.GetNext (nextPos);
        if ( pCSPData )
        {
            bool bAddString = false;

            // NTRAID# 460671: Certtmpl.msc: Should show  CSPs that is not 
            // available on the client
            if ( CERTTMPL_NON_LOCAL_CSP == pCSPData->m_dwProvType )
                bAddString = true;
            else if ( bSignatureOnly && pCSPData->m_dwSigMaxKeySize >= dwMinKeySize )
                bAddString = true;
            else if ( pCSPData->m_dwKeyExMaxKeySize >= dwMinKeySize )
                bAddString = true;

            if ( bAddString )
            {
                pCSPData->m_bConforming = true;
            }
            else
            {
                pCSPData->m_bConforming = false;

                if ( pCSPData->m_bSelected )
                {
                    HRESULT hr = m_rCertTemplate.ModifyCSPList (pCSPData->m_szName, false); // remove
                    if ( SUCCEEDED (hr) )
                    {
                        pCSPData->m_bSelected = false;
                        if ( PROV_DSS == pCSPData->m_dwProvType || 
                                PROV_DSS_DH == pCSPData->m_dwProvType )
                        {
                            m_nProvDSSCnt--;
                        }

                        SetModified ();
                        m_rbIsDirty = true;
                    }
                }
            }
        }
    }
}

int CTemplateV2RequestPropertyPage::GetSelectedCSPCount ()
{
    int nSelected = 0;

    for (POSITION nextPos = m_CSPList.GetHeadPosition (); nextPos; )
    {
        CT_CSP_DATA* pCSPData = m_CSPList.GetNext (nextPos);
        if ( pCSPData )
        {
            if ( pCSPData->m_bSelected )
                nSelected++;
        }
    }

    return nSelected;
}

void CTemplateV2RequestPropertyPage::OnCsps() 
{
    bool bSignatureOnly = false;
    int nIndex = m_purposeCombo.GetCurSel ();
    if ( nIndex >= 0 )
        bSignatureOnly = (REQUEST_PURPOSE_SIGNATURE == m_purposeCombo.GetItemData (nIndex));


    CSelectCSPDlg dlg (this, m_rCertTemplate, m_CSPList, m_nProvDSSCnt);

    if ( IDOK == dlg.DoModal () )
    {
        HRESULT hr = S_OK;
        for (POSITION nextPos = m_CSPList.GetHeadPosition (); nextPos; )
        {
            CT_CSP_DATA* pCSPData = m_CSPList.GetNext (nextPos);
            if ( pCSPData )
            {
                if ( pCSPData->m_bSelected )
                    hr = m_rCertTemplate.ModifyCSPList (pCSPData->m_szName, true); // add
                else
                    hr = m_rCertTemplate.ModifyCSPList (pCSPData->m_szName, false); // remove
                if ( FAILED (hr) )
                    break;
            }
        }

        if ( SUCCEEDED (hr) )
        {
            SetModified ();
            m_rbIsDirty = true;
        }
        EnableControls ();
    }
}

void CTemplateV2RequestPropertyPage::OnEnrollWithoutInput() 
{
    m_rCertTemplate.SetUserInteractionRequired (false);
    m_rCertTemplate.SetStrongKeyProtectionRequired (false);
    SetModified ();
    m_rbIsDirty = true;
}

void CTemplateV2RequestPropertyPage::OnEnrollPromptUser() 
{
    m_rCertTemplate.SetUserInteractionRequired (true);
    m_rCertTemplate.SetStrongKeyProtectionRequired (false);
    SetModified ();
    m_rbIsDirty = true;
}

void CTemplateV2RequestPropertyPage::OnEnrollPromptUserRequireIfPrivateKey() 
{
    m_rCertTemplate.SetUserInteractionRequired (true);
    m_rCertTemplate.SetStrongKeyProtectionRequired (true);
    SetModified ();
    m_rbIsDirty = true;
}
