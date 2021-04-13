/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMNAME.CPP

Abstract:

    Implements the COM layer of WINMGMT --- the class representing a namespace.
    It is defined in wbemname.h

History:

    raymcc    05-Apr-96  Created.
    raymcc    23-Apr-00  Whistler extensions

--*/

#include "precomp.h"

#pragma warning (disable : 4786)
#include <wbemcore.h>
#include <map>
#include <vector>
#include <genutils.h>
#include <oahelp.inl>
#include <wqllex.h>
#include <autoptr.h>
#include <comutl.h>
#include <helper.h>

#include "wmiarbitrator.h"
#include "wmifinalizer.h"
#include "wmimerger.h"

#ifdef DBG
    TimeTraces gTimeTraceHistory;
    CStaticCritSec OperationStat::lock_;
#endif

extern BOOL g_bDontAllowNewConnections;

//***************************************************************************
//
//***************************************************************************

#define WBEM_MASK_DEPTH (WBEM_FLAG_DEEP | WBEM_FLAG_SHALLOW)
#define WBEM_MASK_CREATE_UPDATE (WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_CREATE_OR_UPDATE)

bool illFormatedClass2 (const wchar_t * pszSuperclass)
{
  for (const wchar_t * p = pszSuperclass; *p != L'\0'; ++p)
  {
    if (!(isunialphanum (*p) ||  *p == L'_'))
    {
      return true;  //Ill formated
    }
  }
  return false;
};


HRESULT IsDerivedFromSystem(CWbemObject& obj, bool * result)
{
    *result = true;
    CVar vClass;
    HRESULT hr = obj.GetClassName(&vClass);
    if (FAILED(hr)) return hr;

    wchar_t * className = vClass.GetLPWSTR();
    if (className[0] == L'_') return S_OK;

    CVar vDeriv;
    hr = obj.GetDerivation(&vDeriv);
    if (FAILED(hr)) return hr;

    CVarVector *pvTemp = vDeriv.GetVarVector();

    for (int j = 0; j < pvTemp->Size(); j++)
    {
        CVar& vParentName = pvTemp->GetAt(j);
	wchar_t * parentName = vParentName.GetLPWSTR();
        if (parentName[0] == L'_')
        {
	    return S_OK;
        }
    }        
    *result = false;
    return S_OK;
};


//***************************************************************************
//
//  StripServer
//   if the string is \\SOMETHING\namespace1\namespace2
//             returns namespace1\namespace2
// but it pre-allocates \\.\ in front of it, so that you can move back the pointer
//
//***************************************************************************


LPWSTR StripServer(LPWSTR pszNamespace)
{
    LPWSTR lpRet = NULL;
    WCHAR * lpPtr = pszNamespace;
    if (*lpPtr == L'\\' || *lpPtr == L'/')
    {
        lpPtr++;
        if (*lpPtr == L'\\' || *lpPtr == L'/')
        {
            BOOL bSlash = FALSE;
            while (*lpPtr)
            {
                lpPtr++;
                if (*lpPtr == L'\\' || *lpPtr == L'/')
                {
                    bSlash = TRUE;
                    break;
                }
            }
            if (bSlash)
            {
                lpPtr++;
                size_t tmpSize = 4 + wcslen(lpPtr) + 1;
                WCHAR * pFull = new WCHAR[tmpSize];
                if (NULL == pFull) return pFull;
                StringCchCopyW(pFull,tmpSize,L"\\\\.\\");
                StringCchCopyW(pFull+4,tmpSize-4,lpPtr);
                lpRet = pFull;
            }
        }
    }

    if (!lpRet)
    {
        size_t tmpSize = 4 + wcslen(lpPtr) + 1;
        WCHAR * pFull = new WCHAR[tmpSize];
        if (NULL == pFull) return pFull;
        StringCchCopyW(pFull,tmpSize,L"\\\\.\\");
        StringCchCopyW(pFull+4,tmpSize-4,lpPtr);
        lpRet = pFull;        
    }

    return lpRet;
}


//
//
//
///////////////////////////////////////////////////////////////
class CSecureEssNamespaceSink
: public CUnkBase<IWbemObjectSink, &IID_IWbemObjectSink>
{
    CWbemPtr<CWbemNamespace> m_pNamespace;
    CWbemPtr<IWbemObjectSink> m_pSink;

public:


    CSecureEssNamespaceSink( CWbemNamespace* pNamespace,
                             IWbemObjectSink* pSink )
     : m_pNamespace(pNamespace), m_pSink(pSink) {}

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray)
    {
        HRESULT hRes = m_pNamespace->CheckNs();
        if (FAILED(hRes))
            return hRes;

        if ( !m_pNamespace->Allowed( WBEM_FULL_WRITE_REP ) )
            return WBEM_E_ACCESS_DENIED;
        return m_pSink->Indicate( lObjectCount, pObjArray );
    }

    STDMETHOD(SetStatus)(long a, long b, BSTR c, IWbemClassObject* d)
    {
        HRESULT hRes = m_pNamespace->CheckNs();
        if (FAILED(hRes))
            return hRes;

        if ( !m_pNamespace->Allowed( WBEM_FULL_WRITE_REP ) )
            return WBEM_E_ACCESS_DENIED;
        return m_pSink->SetStatus( a, b, c, d );
    }
};



//***************************************************************************
//
//***************************************************************************
//
CWbemNamespace::CWbemNamespace()
{
    m_uSecondaryRefCount = 0;
    m_bShutDown = FALSE;

    m_pSession = 0;
    m_pDriver = 0;
    m_pNsHandle = 0;
    m_pScopeHandle = 0;

    m_pThisNamespace = 0;
    m_pThisNamespaceFull = 0;    
    m_dwPermission = 0;
    m_dwSecurityFlags = 0;
    m_wszUserName = 0;

    m_bProvider = FALSE;
    m_bForClient = FALSE;
    m_bESS = FALSE;

    m_bSecurityInitialized = FALSE;

    m_pProvFact = 0;
    m_pCoreSvc = 0;
    m_bRepositOnly = FALSE;
    m_pRefreshingSvc = NULL;


    m_pszClientMachineName = NULL;
    m_dwClientProcessID = -1;

    m_pArb = CWmiArbitrator::GetRefedArbitrator();
    m_pArb->RegisterNamespace((_IWmiCoreHandle *)this);

    gClientCounter.AddClientPtr(&m_Entry);

}



//***************************************************************************
//
//***************************************************************************
//
CWbemNamespace *CWbemNamespace::CreateInstance()
{
    try
    {
        if (NULL == CWmiArbitrator::GetUnrefedArbitrator()) return NULL;
        CWbemNamespace *pNs = new CWbemNamespace;
        if (pNs) pNs->AddRef();
        return pNs;
    }
    catch(CX_Exception &)
    {
        return 0;
    }
}


//***************************************************************************
//
//  CWbemNamespace::Initialize
//
//  Real constructor. In addition to finding the namespace in the database, this
//  function also enumerates all the class providers in the namespace and
//  loads them. It also notifies the ESS of the opening.
//
//  PARAMETERS:
//
//      LPWSTR Namespace        The full of the namespace to create.
//
//  RETURN VALUES:
//
//      Even though this function has no return values, it indicates success
//      or failure by setting the Status member variable to the error code.
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_INVALID_NAMESPACE     No such namespace
//      WBEM_E_CRITICAL_ERROR        Other database error.
//
//***************************************************************************

HRESULT CWbemNamespace::Initialize(
    LPWSTR pszNamespace,
    LPWSTR wszUserName,
    DWORD dwSecFlags,
    DWORD dwPermission,
    BOOL  bForClient,
    BOOL  bRepositOnly,
    LPCWSTR pszClientMachineName,
    DWORD dwClientProcessID,
    BOOL  bSkipSDInitialize,
    IWmiDbSession *pParentSession)
{
    try
    {
        m_dwSecurityFlags = dwSecFlags;
        m_dwPermission = dwPermission;
        if(g_bDontAllowNewConnections)
            return WBEM_E_SHUTTING_DOWN;

        PSID pRawSid;
        SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;

        if (!AllocateAndInitializeSid( &id, 2,                         // SEC:REVIEWED 2002-03-22 : OK
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0,0,0,0,0,0,&pRawSid))
            return WBEM_E_OUT_OF_MEMORY;

        CNtSid Sid(pRawSid);
        FreeSid( pRawSid );
        if (CNtSid::NoError != Sid.GetStatus()) return WBEM_E_OUT_OF_MEMORY;
 
        CNtAce ace(1,ACCESS_ALLOWED_ACE_TYPE,0,Sid);
        if(ace.GetStatus() != 0)
            return WBEM_E_OUT_OF_MEMORY;

        CNtAcl acl;
        acl.AddAce(&ace);
        m_sdCheckAdmin.SetDacl(&acl);
        CNtSid owner(CNtSid::CURRENT_USER);
        if (CNtSid::NoError != owner.GetStatus()) return WBEM_E_OUT_OF_MEMORY;        
        m_sdCheckAdmin.SetGroup(&owner);
        m_sdCheckAdmin.SetOwner(&owner);

        m_bForClient = bForClient;

        m_pThisNamespaceFull = StripServer(pszNamespace);
        if(m_pThisNamespaceFull == NULL) return WBEM_E_OUT_OF_MEMORY;
        m_pThisNamespace = m_pThisNamespaceFull + 4; // move past "\\.\"

        m_pCoreSvc = CCoreServices::CreateInstance();
        if(m_pCoreSvc == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        m_pProvFact = 0;


        m_bRepositOnly = bRepositOnly;

        // Flip the slashes
        // ================

        WCHAR* pwc = m_pThisNamespace;
        while(*pwc)
        {
            if(*pwc == '/')
                *pwc = '\\';
            pwc++;
        }

        m_wszUserName = (wszUserName?Macro_CloneLPWSTR(wszUserName):NULL);

        // Repository binding.
        // ===================

        m_pNsHandle = 0;
        HRESULT hRes;
        IWmiDbSession *pTempSession= pParentSession;
        if (pTempSession == NULL)
        {
            hRes = CRepository::GetDefaultSession(&pTempSession);
            if (FAILED(hRes)) return hRes;
        }
        else
            pTempSession->AddRef();
        CReleaseMe rmSession(pTempSession);

        hRes = CRepository::OpenScope(pTempSession, m_pThisNamespace, 0, &m_pDriver, &m_pSession, &m_pScopeHandle, &m_pNsHandle);
        if (FAILED(hRes))
        {
            Status = WBEM_E_INVALID_NAMESPACE;
            return hRes;
        }

        if (m_pScopeHandle == 0)
        {
            m_bSubscope = FALSE;
            m_pScopeHandle = m_pNsHandle;
            if(m_pScopeHandle == NULL)
            {
                ERRORTRACE((LOG_WBEMCORE, "OpenScope returned success, yet m_pNsHandle is null!\n"));
                return WBEM_E_CRITICAL_ERROR;
            }
            m_pScopeHandle->AddRef();
        }
        else
        {
#ifdef DBG
            DebugBreak();
#endif
            m_bSubscope = TRUE;
            return WBEM_E_INVALID_NAMESPACE;
        }

        m_pProvFact = 0;
        if (!bRepositOnly)
        {
            _IWmiProvSS *pProvSS = 0;
            m_pCoreSvc->GetProviderSubsystem(0, &pProvSS);
            CReleaseMe _(pProvSS);

            if(pProvSS)
            {
                HRESULT hr = pProvSS->Create(
                    this,                           // Stupid because v-table access can occur before constructor completion
                    0,                              // lFlags
                    0,                              // pCtx
                    m_pThisNamespace,               // Path
                    IID__IWmiProviderFactory,
                    (LPVOID *) &m_pProvFact
                    );

                if (FAILED(hr)) return hr ;
            }
        }

        if(pszClientMachineName)
        {
            delete m_pszClientMachineName;
            DUP_STRING_NEW(m_pszClientMachineName, pszClientMachineName);
            if(m_pszClientMachineName == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
        m_dwClientProcessID = dwClientProcessID;

        Status = WBEM_S_NO_ERROR;

        //Initialize Security descriptor
        if (!bSkipSDInitialize)
        {
            hRes = InitializeSD(pTempSession);
            if (FAILED(hRes)) return hRes;
        }

        m_pCoreSvc->IncrementCounter(WMICORE_SELFINST_CONNECTIONS, 1);

        return Status;

    }
    catch(...)
    {
        ExceptionCounter c;
        return WBEM_E_FAILED;
    }
}

//***************************************************************************
//
//  CWbemNamespace::~CWbemNamespace
//
//  Notifies the ESS of namespace closure and frees up all the class providers.
//
//***************************************************************************

CWbemNamespace::~CWbemNamespace()
{
    // do this before releasing the arbitrator
    m_pArb->UnregisterNamespace((_IWmiCoreHandle *)this);

    if (m_pCoreSvc)
        m_pCoreSvc->DecrementCounter(WMICORE_SELFINST_CONNECTIONS, 1);

    ReleaseIfNotNULL(m_pProvFact);
    ReleaseIfNotNULL(m_pCoreSvc);
    ReleaseIfNotNULL(m_pSession);
    ReleaseIfNotNULL(m_pDriver);
    ReleaseIfNotNULL(m_pNsHandle);
    ReleaseIfNotNULL(m_pScopeHandle);
    ReleaseIfNotNULL(m_pRefreshingSvc);
    ReleaseIfNotNULL(m_pArb);

    DeleteAndNull(m_pThisNamespaceFull);
    DeleteAndNull(m_wszUserName);
    DeleteAndNull(m_pszClientMachineName);

    gClientCounter.RemoveClientPtr(&m_Entry);
}

//***************************************************************************
//
//  CWbemNamespace::QueryInterface
//
//  Exports IWbemServices interface.
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{

    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemServices==riid )
    {
        *ppvObj = (IWbemServices*)this;
        AddRef();
        return S_OK;
    }
    else if (IID_IWbemRefreshingServices == riid)
    {
        CInCritSec  ics(&m_cs);

        // Check if we already have this one
        if ( NULL == m_pRefreshingSvc )
        {
            IUnknown * pUnk = NULL;
            // Aggregate this interface - We MUST use IUnknown, so the aggregee does not AddRef us.
            HRESULT hr = CoCreateInstance( CLSID__WbemConfigureRefreshingSvcs,          // SEC:REVIEWED 2002-03-22 : OK
                                         (IWbemServices*) this, CLSCTX_INPROC_SERVER,
                                         IID_IUnknown, (void**) &pUnk );
            if (FAILED(hr)) return hr;
            OnDeleteIf<IUnknown *,void(*)(IUnknown *),RM> rmUnk(pUnk);

            _IWbemConfigureRefreshingSvcs*  pCfgRefrSvc = NULL;
            hr = pUnk->QueryInterface( IID__IWbemConfigureRefreshingSvcs, (void**) &pCfgRefrSvc );
            if (FAILED(hr)) return hr;
            CReleaseMe  rm(pCfgRefrSvc);    

            // Use BSTR's in case any marshaling takes place
            BSTR pstrMachineName = SysAllocString( ConfigMgr::GetMachineName() );
            if (NULL == pstrMachineName) return E_OUTOFMEMORY;
            CSysFreeMe  sfm1( pstrMachineName );

            BSTR    pstrNamespace = SysAllocString( m_pThisNamespace );
            if (NULL == pstrNamespace) return E_OUTOFMEMORY;    
            CSysFreeMe  sfm2( pstrNamespace );

            hr = pCfgRefrSvc->SetServiceData( pstrMachineName, pstrNamespace );

            if (FAILED(hr)) return hr;

            // compensate the Automatic Objects above
            m_pRefreshingSvc = pUnk;
            rmUnk.dismiss();
        }

        return m_pRefreshingSvc->QueryInterface( IID_IWbemRefreshingServices, ppvObj );
    }
    else if(IID_IWbemInternalServices == riid)
    {
        *ppvObj = (IWbemInternalServices*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CWbemNamespace::AddRef()
{
    ULONG uNewCount = InterlockedIncrement((LONG *) &m_uSecondaryRefCount);
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CWbemNamespace::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uSecondaryRefCount);
    if (0 == uNewCount)  delete this;
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::Dump(FILE *f)
{
    // SEC:REVIEWED 2002-03-22 : OK; debugging use only

    fprintf(f, "---Namespace = 0x%p----------------------------\n", this);    // SEC:REVIEWED 2002-03-22 : OK
    fprintf(f, "    Secondary Refcount = %d\n", m_uSecondaryRefCount);        // SEC:REVIEWED 2002-03-22 : OK
    if(m_pThisNamespace)
        fprintf(f,  "    Name = %ls\n", m_pThisNamespace);                    // SEC:REVIEWED 2002-03-22 : OK
    if(m_wszUserName)
        fprintf(f,  "    User Name = %ls\n", m_wszUserName);                  // SEC:REVIEWED 2002-03-22 : OK
    if(m_pszClientMachineName)
        fprintf(f,  "    Client Machine Name = %ls\n", m_pszClientMachineName);  // SEC:REVIEWED 2002-03-22 : OK
    else
        fprintf(f,  "    Client Machine Name = <unknown>\n");                    // SEC:REVIEWED 2002-03-22 : OK
    if(m_dwClientProcessID)
        fprintf(f,  "    Client Process = 0X%X\n", m_dwClientProcessID);         // SEC:REVIEWED 2002-03-22 : OK
    else
        fprintf(f,  "    Client Process = <unknown>\n");                         // SEC:REVIEWED 2002-03-22 : OK


    return S_OK;
}

//***************************************************************************
//
//***************************************************************************
//
/*
HRESULT CWbemNamespace::SetErrorObj(IWbemClassObject* pErrorObj)
{
    if (pErrorObj == NULL)
    {
        return S_OK;
    }

    IErrorInfo* pInfo;
    HRESULT hRes = pErrorObj->QueryInterface(IID_IErrorInfo, (void**)&pInfo);
    if (FAILED(hRes))
        return hRes;

    hRes = SetErrorInfo(0, pInfo);
    pInfo->Release();
    return hRes;
}
*/


//***************************************************************************
//
//  CWbemNamespace::SplitLocalized
//
//***************************************************************************

HRESULT CWbemNamespace::SplitLocalized (
    CWbemObject *pOriginal,
    CWbemObject *pStoredObj
    )
{
    HRESULT hres = 0;

    CVar vProv;

    IWbemQualifierSet *pOrigQs = NULL, *pStoredQs = NULL;
    VARIANT vVal;
    VariantInit(&vVal);

    hres = pOriginal->GetQualifierSet(&pOrigQs);
    if (FAILED(hres))
        return hres;

    if (pStoredObj)
    {
        hres = pStoredObj->GetQualifierSet(&pStoredQs);
        if (FAILED(hres))
        {
            pOrigQs->Release();
            return hres;
        }
    }

    hres = FixAmendedQualifiers(pOrigQs, pStoredQs);
    pOrigQs->Release();
    pOrigQs = NULL;
    if (pStoredQs) 
    {
        pStoredQs->Release();
        pStoredQs = NULL;
    }


    if (SUCCEEDED(hres))
    {
        pOriginal->BeginEnumeration(0);
        LONG lLong;
        CIMTYPE ct;
        VARIANT vNewVal;
        VariantInit(&vNewVal);
        BSTR strPropName = NULL;

        while((hres = pOriginal->Next(0, &strPropName, &vNewVal, &ct, &lLong)) == S_OK)
        {
            CSysFreeMe sfm(strPropName);
            CClearMe   ccm(&vNewVal);
            
            CWStringArray arrDel;
            pOrigQs = NULL;
            pStoredQs = NULL;

            // Ignore system qualifiers.

            if (strPropName[0] == L'_')
            { 
                continue;
            }

            hres = pOriginal->GetPropertyQualifierSet(strPropName, &pOrigQs);
            if (FAILED(hres))
            {
                return hres;
            }

            pStoredQs = NULL;
            if (pStoredObj)
            {
                pStoredObj->GetPropertyQualifierSet(strPropName, &pStoredQs);
            }

            hres = FixAmendedQualifiers(pOrigQs, pStoredQs);
            pOrigQs->Release();
            if (pStoredQs)
                pStoredQs->Release();

        }

        pOriginal->EndEnumeration();

        // Unfortunately, we have to enumerate the methods,
        // and *then* update them...

        BSTR bstrMethodName;

        pOriginal->BeginMethodEnumeration(0);
        IWbemClassObject *pLIn = NULL, *pLOut = NULL, *pOIn = NULL, *pOOut = NULL;

        // first count the number of methods

        while ( pOriginal->NextMethod( 0, &bstrMethodName, 0, 0 ) == S_OK )
        {
            pLIn = pLOut = pOIn = pOOut = NULL;
            pOrigQs = NULL ;
            pStoredQs = NULL ;

            hres = pOriginal->GetMethod(bstrMethodName, 0, &pLIn, &pLOut);
            if ( FAILED ( hres ) )
            {
                continue ;
            }

            CSysFreeMe fm(bstrMethodName);
            CReleaseMe rm0(pLIn);
            CReleaseMe rm2(pLOut);

            hres = pOriginal->GetMethodQualifierSet(bstrMethodName, &pOrigQs);
            if (FAILED(hres))
            {
                continue;
            }

            CReleaseMe rm4 ( pOrigQs ) ;

            if (pStoredObj)
            {
                hres = pStoredObj->GetMethodQualifierSet(bstrMethodName, &pStoredQs);
                if ( FAILED ( hres ) )
                {
                    continue ;
                }
            }

            CReleaseMe rm5 ( pStoredQs ) ;

            // Method qualifiers...

            hres = FixAmendedQualifiers(pOrigQs, pStoredQs);

            if (SUCCEEDED(hres))
            {
                if (pStoredObj)
                    hres = pStoredObj->GetMethod(bstrMethodName, 0, &pOIn, &pOOut);

                CReleaseMe rm1(pOIn);
                CReleaseMe rm3(pOOut);

                if (pLIn)
                    hres = SplitLocalized((CWbemObject *)pLIn, (CWbemObject *)pOIn);

                if (pLOut)
                    hres = SplitLocalized((CWbemObject *)pLOut, (CWbemObject *)pOOut);

                hres = pOriginal->PutMethod(bstrMethodName, 0, pLIn, pLOut);
            }
            else
                break;
        }
        pOriginal->EndMethodEnumeration();

    }

    hres = 0;

    return hres;

}

//***************************************************************************
//
//  CWbemNamespace::FixAmendedQualifiers
//
//***************************************************************************

HRESULT CWbemNamespace::FixAmendedQualifiers(
    IWbemQualifierSet *pOriginal,
    IWbemQualifierSet *pNew
    )
 {
    HRESULT hres = 0;
    CWStringArray arrDelete;
    CWStringArray arrProps;
    BSTR strName = 0;
    long lFlavor;



    int i;

    pOriginal->BeginEnumeration(0);
    // BUGBUG EndEnum
    while(pOriginal->Next(0, &strName, NULL, NULL) == S_OK)
    {
        CSysFreeMe sfm(strName);
        if (CFlexArray::no_error != arrProps.Add(strName))
            return WBEM_E_OUT_OF_MEMORY;
    }

    for (i = 0; i < arrProps.Size(); i++)
    {
        _variant_t vVal;
        pOriginal->Get(arrProps.GetAt(i), 0, &vVal, &lFlavor);
        if (lFlavor & WBEM_FLAVOR_AMENDED)
        {
            // Delete the "amended" qualifier.

            if (CFlexArray::no_error != arrDelete.Add(arrProps.GetAt(i)))
                return WBEM_E_OUT_OF_MEMORY;

            // Restore any original qualifier value.
            if (pNew)
            {
                _variant_t vOldVal;
                long lOldFlavor;
                if (pNew->Get(arrProps.GetAt(i), 0, &vOldVal, &lOldFlavor) != WBEM_E_NOT_FOUND)
                {
                    pOriginal->Put(arrProps.GetAt(i), &vOldVal, lOldFlavor);
                    arrDelete.RemoveAt(arrDelete.Size()-1);
                }
            }
        }
    }
    pOriginal->EndEnumeration();

    for (i = 0; i < arrDelete.Size(); i++)
    {
        pOriginal->Delete(arrDelete.GetAt(i));
    }

    arrDelete.Empty();

    return hres;

 }

//***************************************************************************
//
//  CWbemNamespace::Exec_DeleteClass
//
//  Actually deletes the class from the database. No class provider support.
//  Raises class deletion event.
//
//  Parameters and return values are exacly the same as those for DeleteClass
//  as described in help
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_DeleteClass(
    LPWSTR pszClassName,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{
    TIMETRACE(TimeTraces::DeleteClass);
    HRESULT hRes;
    IWbemServices *pClassProv = 0;
    CSynchronousSink* pSyncSink = 0;
    BSTR bstrClass = 0;
    IWbemClassObject* pStaticClassDef = 0;
    BOOL bInRepository = FALSE;

    if (pszClassName == 0 || pSink == 0)
        return pSink->Return(WBEM_E_INVALID_PARAMETER);

    if (pszClassName[0] == L'_')
        return pSink->Return(WBEM_E_INVALID_OPERATION);

    // Bring up the dynamic class provider symposium for consultation.
    // ===============================================================

    if (!m_bRepositOnly && m_pProvFact)
    {
        hRes = m_pProvFact->GetClassProvider(
                    0,                  // lFlags
                    pCtx,
                    m_wszUserName,
                    m_wsLocale,
                    m_pThisNamespace,                     // IWbemPath pointer
                    0,
                    IID_IWbemServices,
                    (LPVOID *) &pClassProv
                    );

        if (FAILED(hRes))
            return pSink->Return(hRes);
    }

    CReleaseMe _2(pClassProv);

    _IWmiCoreWriteHook *pHook = 0;
    hRes = m_pCoreSvc->NewPerTaskHook(&pHook);
    if (FAILED(hRes))
        return pSink->Return(hRes);
    CReleaseMe _(pHook);
    HRESULT hHookResult = 0;


    // First, try repository.
    // ======================

    if (m_bRepositOnly || m_pProvFact == NULL)
    {

        if (!Allowed(WBEM_FULL_WRITE_REP))
            return pSink->Return(WBEM_E_ACCESS_DENIED);

        if (pHook)
            pHook->PreDelete(WBEM_FLAG_CLASS_DELETE, lFlags, pCtx, NULL,
                                       m_pThisNamespace, pszClassName );

        hRes = CRepository::DeleteByPath(m_pSession, m_pNsHandle, pszClassName, lFlags);

        if (pHook)
            pHook->PostDelete(WBEM_FLAG_CLASS_DELETE, hRes, pCtx, NULL, m_pThisNamespace, pszClassName, NULL);

        return pSink->Return(hRes);
    }

    // If here, we have to get it first because dynamic class providers
    // could be seriously affected by the removal of the class.
    // ================================================================

    hRes = CRepository::GetObject(
             m_pSession,
             m_pNsHandle,
             pszClassName,
             0,
             &pStaticClassDef
             );

    CReleaseMe _1(pStaticClassDef);

    if (SUCCEEDED(hRes))
    {
        bInRepository = TRUE;
        if (!Allowed(WBEM_FULL_WRITE_REP))
            return pSink->Return(WBEM_E_ACCESS_DENIED);
        if (pStaticClassDef == 0)
            return pSink->Return(WBEM_E_CRITICAL_ERROR);
    }

    // Build up a synchronous sink to receive the class.
    // =================================================

    pSyncSink = CSynchronousSink::Create();
    if (pSyncSink == NULL)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    pSyncSink->AddRef();
    CReleaseMe _3(pSyncSink);

    // Try to get it.
    // ==============

    bstrClass = SysAllocString(pszClassName);
    if (bstrClass == 0)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    CSysFreeMe sfm(bstrClass);

    // If the class was in the repository, we are merely advising dynamic
    // class providers that the class is going away.
    // ==================================================================

    if (bInRepository)
        lFlags |= WBEM_FLAG_ADVISORY;
    else
    {
        if (!Allowed(WBEM_WRITE_PROVIDER))
            return pSink->Return(WBEM_E_ACCESS_DENIED);
    }

    hRes = pClassProv->DeleteClassAsync(bstrClass, lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS, pCtx, pSyncSink);

    if (FAILED(hRes) && hRes != WBEM_E_NOT_FOUND)
        return pSink->Return(hRes);

    pSyncSink->Block();
    IWbemClassObject* pErrorObj = 0;
    pSyncSink->GetStatus(&hRes, NULL, &pErrorObj);
    CReleaseMe rmErrObj(pErrorObj);

    if (FAILED(hRes))
    {
        pSink->Return(hRes, pErrorObj);
        return hRes;
    }

    // If here, we can go ahead with it.
    // =================================

    if (pHook)
        pHook->PreDelete(WBEM_FLAG_CLASS_DELETE, lFlags, pCtx, NULL,
                                       m_pThisNamespace, pszClassName );

    if (bInRepository)
        hRes = CRepository::DeleteByPath(m_pSession, m_pNsHandle, pszClassName, lFlags);

    if (pHook)
        pHook->PostDelete(WBEM_FLAG_CLASS_DELETE, hRes, pCtx, NULL, m_pThisNamespace, pszClassName, NULL);

    return pSink->Return(hRes);
}


//***************************************************************************
//
//  CWbemNamespace::Exec_CreateClassEnum
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_CreateClassEnum(
    LPWSTR pszSuperclass,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{
    TIMETRACE(TimeTraces::CreateClassEnum);
    HRESULT hRes;
    IWbemClassObject* pErrorObj = 0;
    IWbemServices *pClassProv = 0;
    CSynchronousSink* pSyncSink = 0;
    BSTR bstrClass = 0;
    IWbemClassObject* pResultObj = 0;
    CCombiningSink* pCombiningSink = NULL;
    CLocaleMergingSink * pLocaleSink = NULL;
    CBasicObjectSink  *pTmp = 0;
    BSTR bstrSuperclass = 0;
    bool bProvSSNotFound = false;
    bool bRepNotFound = false;

    // Quick check of parms.
    // =====================

    if (pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (pszSuperclass == 0)     // Ensure we point to a blank instead of NULL with no superclass
        pszSuperclass = L"";
    else
      {
      if (illFormatedClass2 (pszSuperclass))
        return pSink->Return(WBEM_E_INVALID_CLASS);
      }


    // Prepare some sinks to hold everything.
    // ======================================

    if ((lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS))
    {
        pLocaleSink = new CLocaleMergingSink(pSink, m_wsLocale, m_pThisNamespace);
        if (pLocaleSink == NULL)
            return pSink->Return(WBEM_E_OUT_OF_MEMORY);
        pLocaleSink->AddRef();
        pTmp = pLocaleSink;
    }
    else
        pTmp = pSink;
    CReleaseMe _1(pLocaleSink);

    pCombiningSink = new CCombiningSink(pTmp);
    if (NULL == pCombiningSink) return pSink->Return(WBEM_E_OUT_OF_MEMORY);    
    pCombiningSink->AddRef();    
    CReleaseMe _2(pCombiningSink);

    // Bring up the dynamic class provider symposium for consultation.
    // ===============================================================

    if ( !m_bRepositOnly && m_pProvFact)
    {

        hRes = m_pProvFact->GetClassProvider(
                    0,                  // lFlags
                    pCtx,
                    m_wszUserName,
                    m_wsLocale,
                    m_pThisNamespace,                     // IWbemPath pointer
                    0,
                    IID_IWbemServices,
                    (LPVOID *) &pClassProv
                    );

        if (FAILED(hRes))
            return pCombiningSink->Return(hRes);
    }

    CReleaseMe _3(pClassProv);

    // Get the repository classes.
    // ===========================

    BOOL bUseStatic = !(lFlags & WBEM_FLAG_NO_STATIC);
    if (bUseStatic)
    {
        if ((lFlags & WBEM_MASK_DEPTH) == WBEM_FLAG_DEEP)
        {
            // DEEP ENUM
            // ==========
            IWbemObjectSink *pObjSink = (IWbemObjectSink *) pCombiningSink;
            hRes = CRepository::QueryClasses(
                        m_pSession,
                        m_pNsHandle,
                        WBEM_FLAG_DEEP | WBEM_FLAG_VALIDATE_CLASS_EXISTENCE,
                        pszSuperclass,
                        pObjSink);
        }
        else
        {
            // SHALLOW ENUM
            // =============
            IWbemObjectSink *pObjSink = (IWbemObjectSink *) pCombiningSink;
            hRes = CRepository::QueryClasses(
                        m_pSession,
                        m_pNsHandle,
                        WBEM_FLAG_SHALLOW | WBEM_FLAG_VALIDATE_CLASS_EXISTENCE,
                        pszSuperclass,
                        pObjSink);
        }

        //If a SetStatus of INVALID_CLASS was indicated it means there is no static
        //class, however we need to continue on with dynamic classes, so we need
        //to clear the error.
        if ((pCombiningSink->GetHResult() == WBEM_E_NOT_FOUND) || (hRes == WBEM_E_NOT_FOUND))
        {
            bRepNotFound = true;
            pCombiningSink->ClearHResult();
            hRes = WBEM_S_NO_ERROR;
        }

        if (FAILED(hRes))
        {
            // A real failure.  Give up.
            // =========================
            return pCombiningSink->Return(hRes);
        }
    }

    if (m_bRepositOnly || m_pProvFact == NULL)
        return pCombiningSink->Return(WBEM_S_NO_ERROR);

    // If here, we have to merge in the dynamic classes.
    // =================================================
    // Build up a synchronous sink to receive the classes.
    // ===================================================

    pSyncSink = CSynchronousSink::Create();
    if (pSyncSink == NULL) return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    pSyncSink->AddRef();
    CReleaseMe _4(pSyncSink);

    // Try to get it.
    // ==============

    bstrSuperclass = SysAllocString(pszSuperclass);
    if (bstrSuperclass == 0)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    CSysFreeMe sfm99(bstrSuperclass);

    CDecoratingSink * pDecore = new CDecoratingSink(pSyncSink, this);
    if(pDecore == NULL)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    pDecore->AddRef();
    CReleaseMe cdecor(pDecore);
    hRes = pClassProv->CreateClassEnumAsync(bstrSuperclass, (lFlags & (~WBEM_FLAG_USE_AMENDED_QUALIFIERS)) & ~WBEM_FLAG_NO_STATIC, pCtx, pDecore);
    if ((pSyncSink->GetHResult() == WBEM_E_NOT_FOUND) || (hRes == WBEM_E_NOT_FOUND))
    {
        bProvSSNotFound = true;
        pSyncSink->ClearHResult();
        hRes = WBEM_S_NO_ERROR;
    }

    if (bProvSSNotFound && bRepNotFound)
    {
        //Neither the provider subsystem nor the repository found this object,
        //therefore we need to actually return an error!
        return pCombiningSink->Return(WBEM_E_INVALID_CLASS);
    }

    if (FAILED(hRes) && hRes != WBEM_E_NOT_FOUND)
        return pCombiningSink->Return(hRes);

    pSyncSink->Block();
    pSyncSink->GetStatus(&hRes, NULL, &pErrorObj);

    if (FAILED(hRes))
    {
        pCombiningSink->Return(hRes, pErrorObj);
        if (pErrorObj)
            pErrorObj->Release();
        return hRes;
    }

    // Otherwise, somebody claimed to have supplied some classes.  Add them into to the
    // combining sink.
    // =================================================================================

    CRefedPointerArray<IWbemClassObject>& raObjects = pSyncSink->GetObjects();

    for (int i = 0; i < raObjects.GetSize(); i++)
    {
        IWbemClassObject *pClsDef = (IWbemClassObject *) raObjects[i];
        pCombiningSink->Indicate(1, &pClsDef);
    }

    return pCombiningSink->Return(WBEM_S_NO_ERROR);
}


//***************************************************************************
//
//  CWbemNamespace::Exec_PutClass
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_PutClass(
    READONLY IWbemClassObject* pObj,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink,
    BOOL fIsInternal
    )
{
    TIMETRACE(TimeTraces::PutClass);
    HRESULT hRes;
    IWbemClassObject* pErrorObj = 0;
    IWbemServices *pClassProv = 0;
    CSynchronousSink* pSyncSink = 0;
    BSTR bstrClass = 0;
    IWbemClassObject* pStaticClassDef = 0;
    BOOL bInRepository = FALSE;

    // Maintains old functionality
    long lRealFlags = lFlags;

    if (pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (pObj == 0) return pSink->Return(WBEM_E_INVALID_PARAMETER);

    // Extract the class name.
    // =======================
    CVARIANT v;
    hRes = pObj->Get(L"__CLASS", 0, &v, 0, 0);
    if (FAILED(hRes)) return pSink->Return(hRes);

    WCHAR * pClassName = L"";
    if (VT_BSTR == V_VT(&v) && NULL != V_BSTR(&v))
    {
        pClassName = V_BSTR(&v);
    }

    if (wcslen_max(pClassName,g_IdentifierLimit) > g_IdentifierLimit)
        return pSink->Return(WBEM_E_QUOTA_VIOLATION);            
        
    COperationError OpInfo(pSink, L"PutClass",pClassName);
    if (!OpInfo.IsOk()) return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    
    _IWmiObject * pIntObj = NULL;
    hRes = pObj->QueryInterface(IID__IWmiObject,(void **)&pIntObj);
    CReleaseMe rm1(pIntObj);
    if (FAILED(hRes)) return OpInfo.ErrorOccurred(WBEM_E_INVALID_PARAMETER);

    if (WBEM_S_NO_ERROR == pIntObj->IsObjectInstance())
        return  OpInfo.ErrorOccurred(WBEM_E_INVALID_OPERATION);

    CVARIANT v2;
    hRes = pObj->Get(L"__SuperClass", 0, &v2, 0, 0);
    if (FAILED(hRes))
        return OpInfo.ErrorOccurred(hRes);

    if (v2.GetStr() && wcslen(v2.GetStr()))   // SEC:REVIEWED 2002-03-22 : OK; NULL terminator provably exists
    {
        if (CSystemProperties::IsIllegalDerivedClass(v2.GetStr()))
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_SUPERCLASS);
    }

    if (!fIsInternal )
    {
        if ((v.GetStr() == NULL) || (v.GetStr()[0] == '_'))    // SEC:REVIEWED 2002-03-22 : OK; null terminator provably exists
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_OPERATION);
        if (v.GetStr()[wcslen(v.GetStr())-1] == '_')           // SEC:REVIEWED 2002-03-22 : OK; null terminator provably exists
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT);
    }

    if ( !m_bRepositOnly && !fIsInternal && m_pProvFact )
    {
        // Bring up the dynamic class provider symposium for consultation.
        // ===============================================================

        hRes = m_pProvFact->GetClassProvider(
                    0,                  // lFlags
                    pCtx,
                    m_wszUserName,
                    m_wsLocale,
                    m_pThisNamespace,                     // IWbemPath pointer
                    0,
                    IID_IWbemServices,
                    (LPVOID *) &pClassProv
                    );

        if (FAILED(hRes))
            return OpInfo.ErrorOccurred(hRes);
    }

    CReleaseMe _2(pClassProv);

    // Set up a new per-task hook.
    // ===========================

    _IWmiCoreWriteHook *pHook = 0;
    hRes = m_pCoreSvc->NewPerTaskHook(&pHook);
    if (FAILED(hRes)) return OpInfo.ErrorOccurred(hRes);
    CReleaseMe _(pHook);
    HRESULT hHookResult = 0;

    // First, try repository.
    // ======================

    if (m_bRepositOnly || fIsInternal || m_pProvFact == NULL)
    {
        if (!Allowed(WBEM_FULL_WRITE_REP))
            return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);

        if (pHook)
            pHook->PrePut(WBEM_FLAG_CLASS_PUT, lFlags, pCtx, 0,
                          m_pThisNamespace, v.GetStr(), (_IWmiObject *)pObj);

        hRes = CRepository::PutObject(m_pSession, m_pNsHandle, IID_IWbemClassObject, pObj, lFlags);

        if (pHook)
            pHook->PostPut(WBEM_FLAG_CLASS_PUT, hRes, pCtx, 0, m_pThisNamespace, v.GetStr() , (_IWmiObject *)pObj, NULL);

        return OpInfo.ErrorOccurred(hRes);
    }

    hRes = CRepository::GetObject(
             m_pSession,
             m_pNsHandle,
             v.GetStr(),
             0,
             &pStaticClassDef
             );

    CReleaseMe _1(pStaticClassDef);

    if (SUCCEEDED(hRes))
    {
        bInRepository = TRUE;

        if (pStaticClassDef != 0)
        {
            // Remove all the amended qualifiers
            // =================================

            if (lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS )
            {
                int nRes = SplitLocalized( (CWbemObject*) pObj, (CWbemObject*) pStaticClassDef );
                if (FAILED(nRes))
                {
                    return pSink->Return(nRes);
                }
            }
        }
        else
        {
            return OpInfo.ErrorOccurred(WBEM_E_CRITICAL_ERROR);
        }
    }

    // Build up a synchronous sink to receive the class.
    // =================================================

    pSyncSink = CSynchronousSink::Create();
    if (pSyncSink == NULL) return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
    pSyncSink->AddRef();
    CReleaseMe _3(pSyncSink);

    // Try to put it.
    // ==============

    // If the class was in the repository, we are merely advising dynamic
    // class providers that the class is going away.
    // ==================================================================

    if (bInRepository)
        lFlags |= WBEM_FLAG_ADVISORY;

    if (!Allowed(WBEM_WRITE_PROVIDER))
        hRes = WBEM_E_ACCESS_DENIED;
    else
        hRes = pClassProv->PutClassAsync(pObj, lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS, pCtx, pSyncSink);


    if (FAILED(hRes) && hRes != WBEM_E_NOT_FOUND)
        return OpInfo.ErrorOccurred(hRes);

    pSyncSink->Block();
    pSyncSink->GetStatus(&hRes, NULL, &pErrorObj);

    if (SUCCEEDED(hRes)&&(!bInRepository))
    {
        pSink->Return(hRes, pErrorObj);
        if (pErrorObj)
            pErrorObj->Release();
        return hRes;
    }

    if (FAILED(hRes) && hRes != WBEM_E_NOT_FOUND)
    {
        pSink->Return(hRes, pErrorObj);
        if (pErrorObj)
            pErrorObj->Release();
        return hRes;
    }

    // If here, we can go ahead with it.
    // =================================

    if (!Allowed(WBEM_FULL_WRITE_REP))
        return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);

    if (pHook)
    {
        pHook->PrePut(WBEM_FLAG_CLASS_PUT, lFlags, pCtx, 0,
                      m_pThisNamespace, v.GetStr(), (_IWmiObject *)pObj);
    }

    hRes = CRepository::PutObject(m_pSession, m_pNsHandle, IID_IWbemClassObject, pObj, lFlags);

    // Workaround for forceupdate and instances problem
    if ( WBEM_E_CLASS_HAS_INSTANCES == hRes && ( lRealFlags & WBEM_FLAG_UPDATE_FORCE_MODE ) )
    {
        _variant_t v;
        hRes = pObj->Get( L"__CLASS", 0L, &v, NULL, NULL );
        if ( SUCCEEDED( hRes ) && V_VT( &v ) == VT_BSTR )
        {
            hRes = DeleteObject( V_BSTR( &v ), 0L, pCtx, NULL );
            if ( SUCCEEDED( hRes ) )
            {
                hRes = CRepository::PutObject(m_pSession, m_pNsHandle, IID_IWbemClassObject, pObj, lFlags);
            }
        }
        else
        {
            hRes = WBEM_E_CLASS_HAS_INSTANCES;
        }
    }

    if (pHook)
    {
        pHook->PostPut(WBEM_FLAG_CLASS_PUT, hRes, pCtx, 0, m_pThisNamespace, v.GetStr() , (_IWmiObject *)pObj, NULL);
    }


    return OpInfo.ErrorOccurred(hRes);
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_CancelAsyncCall(
    IWbemObjectSink* pSink
    )
{

    _IWmiArbitrator *pArb = CWmiArbitrator::GetUnrefedArbitrator();
    HRESULT hRes = pArb->CancelTasksBySink(0, IID_IWbemObjectSink, pSink);
    return hRes;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_CancelProvAsyncCall(
    IWbemServices* pProv, IWbemObjectSink* pSink
    )
{
    // Call to the actual provider
    HRESULT hRes = pProv->CancelAsyncCall( pSink );
    return hRes;
}


//***************************************************************************
//
//  CWbemNamespace::Exec_PutInstance
//
//  Actually stores the instance in the database. If the class is dynamic, the
//  call is propagated to the provider.
//  Raises instance creation or modification event.
//
//  Parameters and return values are exacly the same as those for PutInstance
//  as described in help
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_PutInstance(
    IWbemClassObject* pInst,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{
    TIMETRACE(TimeTraces::PutInstance);
    HRESULT hRes = CheckNs();
    if (FAILED(hRes)) return hRes;

    if (NULL == pSink) return WBEM_E_INVALID_PARAMETER;
    if(pInst == NULL) return pSink->Return(WBEM_E_INVALID_PARAMETER);

    COperationError OpInfo(pSink, L"PutInstance", L"");
    if (!OpInfo.IsOk()) return pSink->Return(WBEM_E_OUT_OF_MEMORY);

    CWbemObject *pObj = (CWbemObject *) pInst;
    HRESULT hres;

    _variant_t v;
    hRes = pObj->Get(L"__RELPATH", 0, &v, NULL, NULL);
    if (FAILED(hRes)) return OpInfo.ErrorOccurred(hRes);
    
    if(VT_BSTR == V_VT(&v))
    {
        if (wcslen_max(V_BSTR(&v),g_PathLimit) > g_PathLimit)
            return OpInfo.ErrorOccurred(WBEM_E_QUOTA_VIOLATION);
    }

    if (!pObj->IsInstance())
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT);

    if (!pObj->IsKeyed())
        return OpInfo.ErrorOccurred(WBEM_E_NO_KEY);

    if (pObj->IsLimited())
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT);

    if (pObj->IsClientOnly())
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT);

    // Check if localization bits are set, and if so, that the
    // AMENDED_QUALIFIERS flag was specified.

    if ( ((CWbemObject*) pObj)->IsLocalized() &&
        !( lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS ) )
    {
        return OpInfo.ErrorOccurred( WBEM_E_AMENDED_OBJECT );
    }

    if((lFlags & WBEM_FLAG_UPDATE_ONLY) == 0)
    {
        // Make sure that put extensions are not used without UPDATE_ONLY
        // ==============================================================

        BOOL bExtended;
        hres = GetContextBoolean(pCtx, L"__PUT_EXTENSIONS", &bExtended);
        if(FAILED(hres) || bExtended)
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_CONTEXT);
    }

    BSTR strPropName = NULL;
    CSysFreeMeRef fmref(strPropName);
    if(!pObj->ValidateRange(&strPropName))
    {
        OpInfo.SetParameterInfo(strPropName); // throws
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_PROPERTY);
    }

    // Build the key string.
    // =====================

    CVar vClass;
    hres = pObj->GetClassName(&vClass); // throw
    if (FAILED(hres))
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT);

    OpInfo.SetParameterInfo(vClass.GetLPWSTR()); // throws

    // Get the class definition
    // ========================
    IWbemClassObject* pErrorObj = NULL;
    IWbemClassObject* pClassObj = NULL;
    hres = Exec_GetObjectByPath(vClass.GetLPWSTR(), 0, pCtx,&pClassObj, &pErrorObj);
    CReleaseMe rmErrObj(pErrorObj);
    CReleaseMe rm1(pClassObj);    
    
    if(hres == WBEM_E_NOT_FOUND) hres = WBEM_E_INVALID_CLASS;

    if(FAILED(hres))  return OpInfo.ErrorOccurred(hres, pErrorObj);


    CWbemClass *pClassDef = (CWbemClass*)pClassObj;

    // Dont allow write of old security classes.  This prevents
    // a nefarious user from trying to slip in some extra rights

    if (wbem_wcsicmp(vClass.GetLPWSTR(), L"__NTLMUser") == 0 ||
        wbem_wcsicmp(vClass.GetLPWSTR(), L"__NTLMGroup") == 0)
    {
        if (!Allowed(WRITE_DAC))
            return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
        if((lFlags & WBEM_FLAG_ONLY_STATIC) == 0)
        {
            return PutSecurityClassInstances(vClass.GetLPWSTR(), pInst ,
                        pSink, pCtx, lFlags);
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Dont allow write on the __thisnamespace instance -- except during an upgrade in setup
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (wbem_wcsicmp(vClass.GetLPWSTR(), L"__thisnamespace") == 0 && !IsNtSetupRunning())
    {
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OPERATION);
    }

    // Make sure that the instance and the class match
    // ===============================================

    // SJS - Amendment is the same as Abstract
    if( pClassDef->IsAmendment() || pClassDef->IsAbstract() || !pClassDef->IsKeyed() )
    {
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OPERATION);
    }

    if(!((CWbemInstance*)pObj)->IsInstanceOf(pClassDef)) // trow
    {
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_CLASS);
    }

    // Verify provider validity
    // ========================

    // Only administrators can change provider registration
    // RAID# 566241
    // ==================================

    if(pObj->InheritsFrom(L"__Provider") == S_OK || pObj->InheritsFrom(L"__ProviderRegistration") == S_OK)
    {
        HANDLE hAccess;
        hres = GetAccessToken(hAccess);
        if ( FAILED (hres) )
        {
            if ( hres != 0x80041007 )
            {
                return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
            }
        }
        else
        {
            if ( !IsAdmin(hAccess))
            {
                CloseHandle ( hAccess );
                return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
            }
            CloseHandle ( hAccess );
        }
    }

    // While the class may not be dynamically provided, some of the
    // properties might be.
    // ============================================================

    hres = GetOrPutDynProps(pObj, PUT, pClassDef->IsDynamic());
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE, "Failed to pre-process an instance of %S using "
                                                         "a property provider. Error code: %X\n",
                                                         vClass.GetLPWSTR(), hres));
    }

    // Recursively Put in all the right places
    // ======================================

    CCombiningSink* pCombSink = new CCombiningSink(OpInfo.GetSink());
    if(pCombSink == NULL)
        return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
    pCombSink->AddRef();
    CReleaseMe rm2(pCombSink);

    return RecursivePutInstance((CWbemInstance*)pObj, pClassDef, lFlags,
                pCtx, pCombSink, TRUE);
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::RecursivePutInstance(
    CWbemInstance* pInst,
    CWbemClass* pClassDef,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink,
    BOOL bLast
    )
{
    HRESULT hRes;

    // See if any action is required at this level
    // ===========================================
    if (pClassDef->IsAbstract() || pClassDef->IsAmendment() || !pClassDef->IsKeyed())
        return WBEM_S_FALSE;

    // See if we need to go up
    // =======================
    BOOL bParentTookCareOfItself = TRUE;

    if (pClassDef->IsDynamic())
    {
        // Get the parent class
        // ====================

        CVar vParentName;
        pClassDef->GetSuperclassName(&vParentName);
        if (!vParentName.IsNull())
        {
            IWbemClassObject* pParentClass = NULL;
            IWbemClassObject* pErrorObj = NULL;
            hRes = Exec_GetObjectByPath(vParentName.GetLPWSTR(), 0, pCtx,&pParentClass, &pErrorObj);
            CReleaseMe rm1(pParentClass);
            CReleaseMe rm2(pErrorObj);
            if (FAILED(hRes))
            {
                pSink->Return(hRes, pErrorObj);
                return hRes;
            }

            // Get it to put it's portion
            // ==========================

            hRes = RecursivePutInstance(pInst, (CWbemClass*)pParentClass,lFlags, pCtx, pSink, FALSE);
            if(FAILED(hRes))
                return hRes;

            if(hRes == WBEM_S_FALSE)
                bParentTookCareOfItself = FALSE;
        }
    }

    // Parent Puts have been taken care of. Call it on our own class.
    // ==============================================================

    // Convert the instance to the right class
    // =======================================

    CWbemInstance* pNewInst = NULL;
    pInst->ConvertToClass(pClassDef, &pNewInst);
    CReleaseMe rm1((IWbemClassObject*)pNewInst);

    if (pNewInst == NULL)
    {
        ERRORTRACE((LOG_WBEMCORE, "Failed to convert an instance to a base class\n"));
        return pSink->Return(WBEM_E_CRITICAL_ERROR);
    }

    if (pClassDef->IsDynamic())
    {
        // Check if we need to do a put at this level
        // ==========================================

        if (!bLast && (lFlags & WBEM_FLAG_UPDATE_ONLY))
        {
            hRes = IsPutRequiredForClass(pClassDef, pInst, pCtx, bParentTookCareOfItself);
            if (FAILED(hRes))
                return pSink->Return(hRes);
            if (hRes == WBEM_S_FALSE)
            {
                // No need to put this class
                // =========================

                return pSink->Return(WBEM_S_NO_ERROR);
            }
        }

        // Get the provider name.
        // ======================

        CVar vProv;
        hRes = pClassDef->GetQualifier(L"Provider", &vProv);
        if (FAILED(hRes) || vProv.GetType() != VT_BSTR)
        {
            return pSink->Return(WBEM_E_INVALID_PROVIDER_REGISTRATION);
        }

        // Access the provider cache.  First check permission
        // ==================================================

        if (!Allowed(WBEM_WRITE_PROVIDER))
            return pSink->Return(WBEM_E_ACCESS_DENIED);

        CSynchronousSink* pSyncSink = CSynchronousSink::Create(pSink);
        if(pSyncSink == NULL) return pSink->Return(WBEM_E_OUT_OF_MEMORY);
        pSyncSink->AddRef();
        CReleaseMe rmSync(pSyncSink);

        IWbemServices *pProv = 0;
        if(m_pProvFact == NULL)  return pSink->Return(WBEM_E_CRITICAL_ERROR);

        WmiInternalContext t_InternalContext ;
        ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ; // SEC:REVIEWED 2002-03-22 : OK

        hRes = m_pProvFact->GetProvider(

            t_InternalContext ,
            0,                  // lFlags
            pCtx,
            0,
            m_wszUserName,
            m_wsLocale,
            0,                      // IWbemPath pointer
            vProv,              // Provider
            IID_IWbemServices,
            (LPVOID *) &pProv
        );

        if (FAILED(hRes))  return pSink->Return(hRes);

        CReleaseMe _(pProv);

        pProv->PutInstanceAsync(pNewInst, lFlags& ~WBEM_FLAG_USE_AMENDED_QUALIFIERS, pCtx, pSyncSink);

        pSyncSink->Block();

        IWbemClassObject* pErrorObj = NULL;
        BSTR str;
        pSyncSink->GetStatus(&hRes, &str, &pErrorObj);
        CSysFreeMe sfm(str);
        CReleaseMe rmErrObj(pErrorObj);
        rmSync.release();

        // It is ok if the upper levels report "provider not capable".
        // ===========================================================

        if (!bLast && hRes == WBEM_E_PROVIDER_NOT_CAPABLE)
            hRes = 0;

        if (FAILED(hRes))
        {
            COperationError OpInfo(pSink, L"PutInstance", L"");
            if (!OpInfo.IsOk()) return WBEM_E_OUT_OF_MEMORY;

            OpInfo.ProviderReturned(vProv.GetLPWSTR(), hRes, pErrorObj);
            return hRes;
        }
        else if (str)
        {
            pSink->SetStatus(0, hRes, str, NULL);
        }

        // Provider passes back NULL, we should construct the instance path and return to client
        // NT RAID: 186286 [marioh]
        // ======================================================================================
        else
        {
            BSTR str = NULL;
            LPWSTR wszPath = pNewInst->GetRelPath();
            if (wszPath )
            {
                str = SysAllocString(wszPath);
                delete [] wszPath;
            }

            pSink->SetStatus(0, hRes, str, NULL);
            SysFreeString(str);
        }

        return WBEM_S_NO_ERROR;
    }

    // The class is not dynamically provided.
    // ======================================

    hRes = ((CWbemInstance*)pNewInst)->PlugKeyHoles();
    if (FAILED(hRes))
        return pSink->Return(hRes);

    // Get the path.
    // =============

    CVar vClass;
    hRes = pNewInst->GetClassName(&vClass);
    if (FAILED(hRes))
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);

    WCHAR * ClassNameStr = vClass.GetLPWSTR();

    // Check permissions for writes on system classes.
    // ===============================================

    bool derivedFromSys = false;
    HRESULT hr = IsDerivedFromSystem(*pNewInst, &derivedFromSys); 	   	
    if (FAILED(hr)) return pSink->Return(hr);

    if (derivedFromSys)
    {
    	if (!Allowed(WBEM_FULL_WRITE_REP))
	return pSink->Return(WBEM_E_ACCESS_DENIED);
    }
    else if (!Allowed(WBEM_PARTIAL_WRITE_REP))
    {
     	return pSink->Return(WBEM_E_ACCESS_DENIED);
    }

    CVARIANT v;
    hRes = pNewInst->Get(L"__RELPATH", 0, &v, 0, 0);
    if(FAILED(hRes))
        return pSink->Return(hRes);
    if(v.GetType() != VT_BSTR)
        return pSink->Return(WBEM_E_CRITICAL_ERROR);
    
    // Set up a new per-task hook.
    // ===========================

    _IWmiCoreWriteHook *pHook = 0;
    hRes = m_pCoreSvc->NewPerTaskHook(&pHook);
    if (FAILED(hRes))
        return pSink->Return(hRes);
    CReleaseMe _(pHook);
    HRESULT hHookResult = 0;

    // See if the instance already exists.
    // ===================================

    IWbemClassObject *pExistingObject = 0;
    hRes = CRepository::GetObject(m_pSession, m_pScopeHandle, v.GetStr(),0, &pExistingObject);
    CReleaseMe _2(pExistingObject);


    if (FAILED(hRes))
    {
        // If we here, we failed to get it from the repository.  Thus, it needs to be created from scratch.
        // ================================================================================================

        // Remove all the amended qualifiers
        // =================================

        if (lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS )
        {
            int nRes = SplitLocalized(pNewInst);
            if (FAILED(nRes))
                return pSink->Return(nRes);
        }

        if((lFlags & WBEM_MASK_CREATE_UPDATE) == WBEM_FLAG_UPDATE_ONLY)
        {
            return pSink->Return(WBEM_E_NOT_FOUND);
        }

        // As a special case, see if the object is
        // of class <__NAMESPACE>.  If so, create a new namespace
        // for it.
        // ======================================================

        if ((wbem_wcsicmp(vClass.GetLPWSTR(), L"__NAMESPACE") == 0) ||
            (CRepository::InheritsFrom(m_pSession, m_pNsHandle, L"__NAMESPACE", vClass.GetLPWSTR()) == 0))
        {
            hRes = CreateNamespace(pNewInst);
            if (FAILED(hRes))
                return pSink->Return(hRes);
        }
        // Not a __NAMESPACE or derivative.
        // ================================

        else
        {
            // If here, the object didn't already exist in the repository, so we
            // can add it to the database.
            // ==================================================================

            // Check if this instance makes any sense to
            // hook callbacks.
            // =========================================

            hRes = DecorateObject(pNewInst);
            if (FAILED(hRes))
                return pSink->Return(hRes);
            IWbemClassObject* pInstObj = pNewInst;
            IWbemClassObject* pOldObj = 0;

            if (pHook)
            {
                // If there are hooks, try them and note whether callback is required.
                // ===================================================================
                hHookResult = pHook->PrePut(WBEM_FLAG_INST_PUT, lFlags, pCtx, 0,
                                            m_pThisNamespace, ClassNameStr, pNewInst
                                            );
            }

            if (FAILED(hHookResult))
            {
                return pSink->Return(hHookResult);
            }

            if (hHookResult == WBEM_S_POSTHOOK_WITH_BOTH)
            {
                CRepository::GetObject(m_pSession, m_pNsHandle, v.GetStr(), 0, &pOldObj);
            }

             // Actually create it in the database
            // ==================================

            hRes = CRepository::PutObject(m_pSession, m_pScopeHandle, IID_IWbemClassObject, LPVOID(pNewInst), DWORD(lFlags));

            if (pHook)
                pHook->PostPut(WBEM_FLAG_INST_PUT, hRes, pCtx, 0, m_pThisNamespace, ClassNameStr, pNewInst, (_IWmiObject *) pOldObj);

            delete pOldObj;

            if (FAILED(hRes))
            {
                return pSink->Return(hRes);
            }
        }
    }

    // If here, the object was already in the repository and requires updating.
    // ========================================================================

    else
    {
        if((lFlags & WBEM_MASK_CREATE_UPDATE) == WBEM_FLAG_CREATE_ONLY)
        {
            return pSink->Return(WBEM_E_ALREADY_EXISTS);
        }

        // Remove all the amended qualifiers
        // =================================

        if (lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS )
        {
            int nRes = SplitLocalized(pNewInst, (CWbemObject *) pExistingObject);
            if (FAILED(nRes))
            {
                return pSink->Return(nRes);
            }
        }

        // Check if this update makes any sense to the ESS
        // ===============================================

        hRes = DecorateObject(pNewInst);
        if (FAILED(hRes))
            return pSink->Return(hRes);
        IWbemClassObject* pInstObj = pNewInst;

        // Check pre-hook.
        // ===============

        if (pHook)
        {
            // If there are hooks, try them and note whether callback is required.
            // ===================================================================
             hHookResult = pHook->PrePut(WBEM_FLAG_INST_PUT, lFlags, pCtx, 0,
                                         m_pThisNamespace, ClassNameStr, pNewInst
                                        );
        }

        if (FAILED(hHookResult))
            return pSink->Return(hHookResult);

        // Actually create it in the database
        // ==================================

        hRes = CRepository::PutObject(m_pSession, m_pScopeHandle, IID_IWbemClassObject, LPVOID(pNewInst), DWORD(lFlags));

        // Post put.
        // =========

        if (pHook)
            pHook->PostPut(WBEM_FLAG_INST_PUT, hRes, pCtx, 0, m_pThisNamespace, ClassNameStr, pNewInst, (_IWmiObject *) pExistingObject);

        if (FAILED(hRes))
            return pSink->Return(hRes);
    }


    // Assign appropriate value to the path
    // ====================================

    LPWSTR wszPath = pNewInst->GetRelPath();
    BSTR str = SysAllocString(wszPath);
    delete [] wszPath;

    pSink->SetStatus(0, WBEM_S_NO_ERROR, str, NULL);
    SysFreeString(str);

    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::CreateNamespace(CWbemInstance *pNewInst)
{
    //
    // internal interface throws
    //
    CVar vNsName;
    HRESULT hRes = pNewInst->GetProperty(L"Name", &vNsName);
    if (FAILED(hRes) || vNsName.IsNull())
    {
        return WBEM_E_INVALID_NAMESPACE;
    }

    // verify that this name is valid
    // ==============================

    if (!IsValidElementName(vNsName.GetLPWSTR(),g_PathLimit-NAMESPACE_ADJUSTMENT))
    {
        return WBEM_E_INVALID_NAMESPACE;
    }

    if (!Allowed(WBEM_FULL_WRITE_REP))
    {
        return WBEM_E_ACCESS_DENIED;
    }

    //Get a new session for transactioning purposes...
    IWmiDbSession *pSession = NULL;
    IWmiDbSessionEx *pSessionEx = NULL;

    hRes = CRepository::GetNewSession(&pSession);
    if (FAILED(hRes))
        return hRes;

    //Get an EX version that supports transactioning...
    pSession->QueryInterface(IID_IWmiDbSessionEx, (void**)&pSessionEx);
    if (pSessionEx)
    {
        pSession->Release();
        pSession = pSessionEx;
    }
    CReleaseMe relMe1(pSession);

    //If we have transactionable session, use it!
    if (pSessionEx)
    {
        hRes = pSessionEx->BeginWriteTransaction(0);
        if (FAILED(hRes))
        {
            return hRes;
        }
    }

    try
    {
        // Build the new namespace name.
        // =============================

        // Create the namespace
        // =====================
        if (SUCCEEDED(hRes))
            hRes = CRepository::PutObject(pSession, m_pScopeHandle, IID_IWbemClassObject, LPVOID(pNewInst), 0);

        // Set the default instances.
        // ===============================

        CWbemNamespace* pNewNs = NULL;

        if (SUCCEEDED(hRes))
            pNewNs = CWbemNamespace::CreateInstance();

        if (SUCCEEDED(hRes) && pNewNs != NULL)
        {
            int iLen = 2;
            if(m_pThisNamespace)
                iLen += wcslen(m_pThisNamespace);   // SEC:REVIEWED 2002-03-22 : OK; null terminator is there by prior precondition
            if(vNsName.GetLPWSTR())
                iLen += wcslen(vNsName.GetLPWSTR());  // SEC:REVIEWED 2002-03-22 : OK; provably ok or we couldn't be here
            WCHAR * pTemp = new WCHAR[iLen];
            if(pTemp)
            {
                *pTemp = L'\0';
                if(m_pThisNamespace)
                {
                    StringCchCopyW(pTemp, iLen, m_pThisNamespace);
                    StringCchCatW(pTemp, iLen, L"\\");
                }
                if(vNsName.GetLPWSTR())
                    StringCchCatW(pTemp, iLen, vNsName.GetLPWSTR());

                //Initialize the namespace object
                hRes = pNewNs->Initialize(pTemp,GetUserName(), 0, 0, FALSE, TRUE,   // SEC:REVIEWED 2002-03-22 : OK
                                          NULL, 0xFFFFFFFF, TRUE, pSession);
                delete pTemp;
            }
            else
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }

            if(SUCCEEDED(hRes))
            {
                hRes = CRepository::EnsureNsSystemInstances(pSession, pNewNs->m_pNsHandle, pSession, m_pNsHandle);
            }

            if (SUCCEEDED(hRes))
                hRes = InitializeSD(pSession);

            pNewNs->Release();
        }
        else if (SUCCEEDED(hRes))
        {
            hRes = WBEM_E_OUT_OF_MEMORY;
        }
    }
    catch (CX_MemoryException &)
    {
        hRes = WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        ExceptionCounter c;
        ERRORTRACE((LOG_WBEMCORE, "Namespace Creation of <%S> caused a very critical error!\n", vNsName.GetLPWSTR()));
        hRes = WBEM_E_CRITICAL_ERROR;
    }
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "Namespace Creation of <%S> caused an error <0x%X>!\n", vNsName.GetLPWSTR(), hRes));
        if (pSessionEx)
            pSessionEx->AbortTransaction(0);
    }
    else
    {
        hRes = DecorateObject(pNewInst);
        if (FAILED(hRes))
        {
            if (pSessionEx)
                   pSessionEx->AbortTransaction(0);
        }
        else
        {
            if (pSessionEx)
            {
                hRes = pSessionEx->CommitTransaction(0);
            }
        }
    }

    return hRes;
}

//***************************************************************************
//
//  CWbemNamespace::Exec_DeleteInstance
//
//  Actually deletes the instance from the database. No instance provider
//  support. Raises instance deletion event.
//
//  Parameters and return values are exacly the same as those for DeleteInstance
//  as described in help
//
//***************************************************************************
HRESULT CWbemNamespace::Exec_DeleteInstance(
    READONLY LPWSTR wszObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{
    TIMETRACE(TimeTraces::DeleteInstance);
    return DeleteSingleInstance(wszObjectPath, lFlags, pCtx, pSink);
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::DeleteSingleInstance(
    READONLY LPWSTR wszObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{
    HRESULT hRes;
    int nRes;

    COperationError OpInfo(pSink, L"DeleteInstance", wszObjectPath);
    if ( !OpInfo.IsOk() ) return WBEM_E_OUT_OF_MEMORY;


    // Parse the object path to get the class involved.
    // ================================================
    ParsedObjectPath* pOutput = 0;
    CObjectPathParser p;
    int nStatus = p.Parse(wszObjectPath,  &pOutput);
    OnDeleteObj<ParsedObjectPath*,CObjectPathParser,
                         void (CObjectPathParser:: *)(ParsedObjectPath *pOutput),
                         &CObjectPathParser::Free> FreeMe(&p,pOutput);

    if (nStatus != 0 || !pOutput->IsInstance())
    {
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT_PATH);
    }

    // Exception for __WinMgmtIdentification
    if (!wbem_wcsicmp(pOutput->m_pClass, L"__CIMOMIdentification") ||
        !wbem_wcsicmp(pOutput->m_pClass, L"__SystemSecurity") ||
        !wbem_wcsicmp(pOutput->m_pClass, L"__ADAPStatus" ) )
    {
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OPERATION);
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Dont allow deletion on the __thisnamespace instance
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (wbem_wcsicmp(pOutput->m_pClass, L"__thisnamespace") == 0 )
    {
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OPERATION);
    }


    // Special case for  the old security classes
    // ==========================================

    if (wbem_wcsicmp(pOutput->m_pClass, L"__NTLMUser") == 0 ||
        wbem_wcsicmp(pOutput->m_pClass, L"__NTLMGroup") == 0)
    {
        if (!Allowed(WRITE_DAC))
        {      
            return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
        }
        if((lFlags & WBEM_FLAG_ONLY_STATIC) == 0)
        {
            return DeleteSecurityClassInstances(pOutput, pSink, pCtx,lFlags);
        }
    }

    // As a special case, see if the object is
    // of class <__NAMESPACE>.  If so, (TEMP) disallow deletion.
    // =========================================================

    WString wsNamespaceName;

    if (wbem_wcsicmp(pOutput->m_pClass, L"__NAMESPACE") == 0 ||
        CRepository::InheritsFrom(m_pSession, m_pScopeHandle, L"__NAMESPACE", pOutput->m_pClass) == 0
        )

    {
        if (!Allowed(WBEM_FULL_WRITE_REP))
        {
            return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
        }

        if (pOutput->m_dwNumKeys != 1)
        {
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT_PATH);
        }

        KeyRef* pKey = pOutput->m_paKeys[0];
        if (pKey->m_pName != NULL && wbem_wcsicmp(pKey->m_pName, L"name"))
        {
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT_PATH);
        }

        if (V_VT(&pKey->m_vValue) != VT_BSTR)
        {
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT_PATH);
        }

        // Prevent deletion of standard namespaces.
        // ========================================

        if (wbem_wcsicmp(m_pThisNamespace, L"ROOT") == 0)
        {
            BSTR pNs = V_BSTR(&pKey->m_vValue);
            if (!pNs)
            {
                return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT_PATH);
            }
            if (wbem_wcsicmp(pNs, L"SECURITY") == 0)
            {
                return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
            }
            if (wbem_wcsicmp(pNs, L"DEFAULT") == 0)
            {
                return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
            }
        }

        // Set up hook.
        // ============

        _IWmiCoreWriteHook *pHook = 0;
        HRESULT hRes = m_pCoreSvc->NewPerTaskHook(&pHook);
        CReleaseMe _(pHook);
        HRESULT hHookResult = 0;
        LPWSTR pszClassName = 0;
        IWbemPath *pPath = 0;

        CVectorDeleteMe<WCHAR> vdmClassName(&pszClassName);

        if (pHook)
        {
            // Parse the object path.
            // ======================

            hRes = m_pCoreSvc->CreatePathParser(0, &pPath);
            if (FAILED(hRes))
            {
                return OpInfo.ErrorOccurred(hRes);
            }

            CReleaseMe _3Path(pPath);

            hRes = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wszObjectPath);
            if (FAILED(hRes))
            {
                return OpInfo.ErrorOccurred(hRes);
            }

            ULONG uBuf = 0;
            hRes = pPath->GetClassName(&uBuf, 0);
            if (FAILED(hRes))
            {
                return OpInfo.ErrorOccurred(hRes);
            }

            pszClassName = new wchar_t[uBuf+1];
            if (pszClassName == 0)
            {
                return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
            }

            hRes = pPath->GetClassName(&uBuf, pszClassName);
            if (FAILED(hRes))
            {
                return OpInfo.ErrorOccurred(hRes);
            }

            // If there are hooks, try them and note whether callback is required.
            // ===================================================================
            hHookResult = pHook->PreDelete(WBEM_FLAG_INST_DELETE, lFlags, pCtx, pPath,
                                       m_pThisNamespace, pszClassName);

            if (FAILED(hHookResult))
            {
                return OpInfo.ErrorOccurred(hHookResult);
            }

            pPath->AddRef();
        }

        CReleaseMe _2Path(pPath);

        // Ensure the object can be reached so that we can delete it.
        // ==========================================================

        IWbemClassObject *pExistingObject = 0;
        hRes = CRepository::GetObject(m_pSession, m_pScopeHandle, wszObjectPath, 0, &pExistingObject);

        if (FAILED(hRes))
        {
            return OpInfo.ErrorOccurred(hRes);
        }

        CReleaseMe _2(pExistingObject);

        if (hRes == WBEM_S_NO_ERROR)    // new test
        {
            // Check if we may
            // ===============

            if (!Allowed(WBEM_FULL_WRITE_REP))
            {
                return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
            }

            // Go ahead and try the deletion.
            // ==============================

            WString sNamespace = "__Namespace='";
            sNamespace += V_BSTR(&pKey->m_vValue);
            sNamespace += "'";

            hRes = CRepository::DeleteByPath(m_pSession, m_pScopeHandle, LPWSTR(sNamespace), 0);

            // Call post hook.
            // ===============
            if (pHook)
                pHook->PostDelete(WBEM_FLAG_INST_DELETE, hRes, pCtx, pPath,
                    m_pThisNamespace, pszClassName, (_IWmiObject *) pExistingObject);

            // Decide what to do if things didn't work out.
            // ============================================

            if (FAILED(hRes))
            {
                return OpInfo.ErrorOccurred(hRes);
            }
        }
        else
        {
            return OpInfo.ErrorOccurred(hRes);
        }

        wsNamespaceName = V_BSTR(&pKey->m_vValue);

        return OpInfo.ErrorOccurred(WBEM_S_NO_ERROR);
    }

    // See if the class is dynamic
    // ===========================

    CWbemObject *pClassDef = 0;
    IWbemClassObject* pErrorObj = NULL;
    IWbemClassObject* pClassObj = NULL;

    HRESULT hres = Exec_GetObjectByPath(pOutput->m_pClass, 0, 
                                                              pCtx,&pClassObj, &pErrorObj);
    CReleaseMe rmErrObj(pErrorObj);
    CReleaseMe rmClsDef(pClassObj);

    if(hres == WBEM_E_NOT_FOUND) hres = WBEM_E_INVALID_CLASS;

    if(FAILED(hres))
    {
        OpInfo.ErrorOccurred(hres, pErrorObj);
        return WBEM_S_NO_ERROR;
    }
    pClassDef = (CWbemObject*)pClassObj;

    CVar vDynFlag;
    hres = pClassDef->GetQualifier(L"Dynamic", &vDynFlag);
    if (SUCCEEDED(hres) && vDynFlag.GetType() == VT_BOOL && vDynFlag.GetBool())
    {
        // Get the provider name.
        CVar vProv;
        hres = pClassDef->GetQualifier(L"Provider", &vProv);
        if (FAILED(hres) || vProv.GetType() != VT_BSTR)
        {
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_PROVIDER_REGISTRATION);
        }

        if (!Allowed(WBEM_WRITE_PROVIDER))
            return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);

        // Access the provider cache.
        // ==========================

        IWbemServices *pProv = 0;
        HRESULT hRes;
        if(m_pProvFact == NULL)
            return OpInfo.ErrorOccurred(WBEM_E_CRITICAL_ERROR);

        WmiInternalContext t_InternalContext ;
        ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) );

        hRes = m_pProvFact->GetProvider(t_InternalContext ,
                                                            0,                  // lFlags
                                                            pCtx,
                                                            0,
                                                            m_wszUserName,
                                                            m_wsLocale,
                                                            0,                      // IWbemPath pointer
                                                            vProv,     // Provider
                                                            IID_IWbemServices,
                                                            (LPVOID *) &pProv);


        if (FAILED(hRes))
        {
            return OpInfo.ErrorOccurred(hRes);
        }

        CReleaseMe rmProv(pProv);

        hRes = pProv->DeleteInstanceAsync( wszObjectPath,
                                                                lFlags& ~WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                                                pCtx,
                                                                OpInfo.GetSink());

        return WBEM_S_NO_ERROR;
    }

    // The class is not dynamically provided.
    // ======================================

    // Only administrators can change provider registration
    // RAID# 566241
    // ==================================

    if(pClassDef->InheritsFrom(L"__Provider") == S_OK || pClassDef->InheritsFrom(L"__ProviderRegistration") == S_OK)
    {
        HANDLE hAccess;
        hres = GetAccessToken (hAccess);
        if ( FAILED (hres) )
        {
            if ( hres != 0x80041007 )
            {
                return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
            }
        }
        else
        {
            if ( !IsAdmin(hAccess))
            {
                CloseHandle ( hAccess );
                return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);
            }
            CloseHandle ( hAccess );
        }
    }

    bool derivedFromSys = false;
    HRESULT hr = IsDerivedFromSystem(*pClassDef, &derivedFromSys); 	   	
    if (FAILED(hr)) return pSink->Return(hr);

    if (derivedFromSys)
    {
    	if (!Allowed(WBEM_FULL_WRITE_REP))
	return pSink->Return(WBEM_E_ACCESS_DENIED);
    }
    else if (!Allowed(WBEM_PARTIAL_WRITE_REP))
    {
     	return pSink->Return(WBEM_E_ACCESS_DENIED);
    }

    rmClsDef.release();


    // If here, it is a normal object.   First retrieve
    // the object for the event subsystem, then go ahead and delete it.
    // ================================================================

    // Prehook.
    // ========

    _IWmiCoreWriteHook *pHook = 0;
    hRes = m_pCoreSvc->NewPerTaskHook(&pHook);
    if (FAILED(hRes)) return OpInfo.ErrorOccurred(hRes);
    CReleaseMe rmHook(pHook);
    
    HRESULT hHookResult = 0;
    LPWSTR pszClassName = 0;
    IWbemPath *pPath = 0;
    CReleaseMeRef<IWbemPath*> rmPath(pPath);
    
    CVectorDeleteMe<WCHAR> vdmClassName(&pszClassName);

    if (pHook)
    {
        // Parse the object path.
        hRes = m_pCoreSvc->CreatePathParser(0, &pPath);
        if (FAILED(hRes))  return OpInfo.ErrorOccurred(hRes);

        hRes = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wszObjectPath);
        if (FAILED(hRes))  return OpInfo.ErrorOccurred(hRes);

        ULONG uBuf = 0;
        hRes = pPath->GetClassName(&uBuf, 0);
        if (FAILED(hRes)) return OpInfo.ErrorOccurred(hRes);


        pszClassName = new wchar_t[uBuf+1];
        if (pszClassName == 0) return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);


        hRes = pPath->GetClassName(&uBuf, pszClassName);
        if (FAILED(hRes)) return OpInfo.ErrorOccurred(hRes);

        // If there are hooks, try them and note whether callback is required.
        // ===================================================================
        hHookResult = pHook->PreDelete(WBEM_FLAG_INST_DELETE, lFlags, pCtx, pPath,m_pThisNamespace, pszClassName);

        if (FAILED(hHookResult))  return OpInfo.ErrorOccurred(hHookResult);

    }

    // If anybody wants to see the old object, get it.
    IWbemClassObject *pDoomedInstance = NULL ;
    if (hHookResult == WBEM_S_POSTHOOK_WITH_OLD)
    {
        hRes = CRepository::GetObject(m_pSession, m_pScopeHandle, wszObjectPath,0, &pDoomedInstance);    
        if (FAILED(hRes)) return OpInfo.ErrorOccurred(hRes);        
    }
    CReleaseMe _Doomed (pDoomedInstance) ;

    hRes = CRepository::DeleteByPath(m_pSession, m_pScopeHandle, wszObjectPath, 0);  // new

    // Posthook.
    if (pHook)
        pHook->PostDelete(WBEM_FLAG_INST_DELETE, hRes, pCtx, pPath,
            m_pThisNamespace, pszClassName, (_IWmiObject *) pDoomedInstance);

    if ( FAILED (hRes) ) return OpInfo.ErrorOccurred(hRes);
    
    return OpInfo.ErrorOccurred(WBEM_NO_ERROR);
}


//***************************************************************************
//
//  CWbemNamespace::Exec_CreateInstanceEnum
//
//  Actually creates the enumerator for all instances of a given class,
//  optionally recursively. Interacts with instance providers. Class provider
//  interaction works, but is sparsely tested.
//
//  Parameters and return values are exacly the same as those for
//  CreateInstanceEnum as described in help
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_CreateInstanceEnum(
    LPWSTR wszClass,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{
    TIMETRACE(TimeTraces::CreateInstanceEnum);

    COperationError OpInfo(pSink, L"CreateInstanceEnum", wszClass);
    if ( !OpInfo.IsOk() )  return  WBEM_E_OUT_OF_MEMORY;

    // Make sure the name of the class is a name
    // =========================================

    if(wcschr(wszClass, L':'))
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_CLASS);

    // Create equivalent query
    // =======================

    WString wsQuery;
    wsQuery += L"select * from ";
    wsQuery += wszClass;

    if((lFlags & WBEM_MASK_DEPTH) == WBEM_FLAG_SHALLOW)
    {
        wsQuery += L" where __CLASS = \"";
        wsQuery += wszClass;
        wsQuery += L"\"";
    }

    CErrorChangingSink* pErrSink = new CErrorChangingSink(OpInfo.GetSink(),WBEM_E_INVALID_QUERY, WBEM_E_INVALID_CLASS);
    if(pErrSink == NULL) return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
    CReleaseMe rmErrorChange(pErrSink);

    // Execute it
    CQueryEngine::ExecQuery(this,L"WQL",(LPWSTR)wsQuery,lFlags,pCtx,pErrSink);

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

/*
void CWbemNamespace::SetUserName(LPWSTR wName)
{
    try
    {
        delete m_wszUserName;
        m_wszUserName = (wName) ? Macro_CloneLPWSTR(wName):NULL;
    }
    catch(...)
    {
        ExceptionCounter c;
        m_wszUserName = 0;
    }
}
*/

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::GetObjectByFullPath(
    READONLY LPWSTR wszObjectPath,
    IWbemPath * pOutput,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{

    // Get the namespace part of the path

    DWORD dwSizeNamespace = 0;
    HRESULT hres = pOutput->GetText(WBEMPATH_GET_NAMESPACE_ONLY, &dwSizeNamespace, NULL);
    if(FAILED(hres))
        return hres;

    LPWSTR wszNewNamespace = new WCHAR[dwSizeNamespace];
    if(wszNewNamespace == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<WCHAR> dm1(wszNewNamespace);

    hres = pOutput->GetText(WBEMPATH_GET_NAMESPACE_ONLY, &dwSizeNamespace, wszNewNamespace);
    if(FAILED(hres))
        return hres;

    // Get the relative part of the path

    DWORD dwSizeRelative = 0;
    hres = pOutput->GetText(WBEMPATH_GET_RELATIVE_ONLY, &dwSizeRelative, NULL);
    if(FAILED(hres))
        return hres;

    LPWSTR wszRelativePath = new WCHAR[dwSizeRelative];
    if(wszRelativePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<WCHAR> dm2(wszRelativePath);

    hres = pOutput->GetText(WBEMPATH_GET_RELATIVE_ONLY, &dwSizeRelative, wszRelativePath);
    if(FAILED(hres))
        return hres;

    if (pOutput->IsLocal(ConfigMgr::GetMachineName()))
    {
        // In win2k, we allowed \\.\root\default:whatever, but not root\default:whatever
        // So, the following additional test was added

        ULONGLONG uFlags;
        hres = pOutput->GetInfo(0, &uFlags);
        if(SUCCEEDED(hres))
        {
            if((uFlags & WBEMPATH_INFO_PATH_HAD_SERVER) == 0)
                return pSink->Return(WBEM_E_INVALID_OBJECT_PATH);
        }

        bool bAlreadyAuthenticated = (m_dwSecurityFlags & SecFlagWin9XLocal) != 0;
        CWbemNamespace* pNewLocal = CWbemNamespace::CreateInstance();
        if(pNewLocal == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        hres = pNewLocal->Initialize(wszNewNamespace, m_wszUserName,
                                        (bAlreadyAuthenticated) ? m_dwSecurityFlags : 0,
                                        (bAlreadyAuthenticated) ? m_dwPermission : 0,
                                         m_bForClient, false,
                                         m_pszClientMachineName,
                                         m_dwClientProcessID,
                                         FALSE,NULL);
        if (FAILED(hres))
        {
            pNewLocal->Release();
            return hres;
        }
        else if (pNewLocal->GetStatus())
        {
            hres = pNewLocal->GetStatus();
            pNewLocal->Release();
            return hres;
        }

        // check for security if this isnt the local 9x case

        if(!bAlreadyAuthenticated)
        {
            DWORD dwAccess = pNewLocal->GetUserAccess();
            if((dwAccess  & WBEM_ENABLE) == 0)
            {
                delete pNewLocal;
                return WBEM_E_ACCESS_DENIED;
            }
            else
               pNewLocal->SetPermissions(dwAccess);
        }

        if(pNewLocal->GetStatus())
        {
            hres = pNewLocal->GetStatus();
            delete pNewLocal;
            return pSink->Return(hres);
        }

#if 0
        pNewLocal->AddRef();
#endif
        pNewLocal->SetLocale(GetLocale());

        hres = pNewLocal->Exec_GetObject(wszRelativePath,
            lFlags, pCtx, pSink);
        pNewLocal->Release();
        return hres;
    }
    else
    {
        // Disable remote retrieval for V1
        // ===============================

        return pSink->Return(WBEM_E_NOT_SUPPORTED);
    }
}

//***************************************************************************
//
//  CWbemNamespace::Exec_GetObjectByPath
//
//  Actually retrieves an object (a class or an instance) from the database.
//  Interacts properly with class and instance providers and uses property
//  providers for post-processing (See GetOrPutDynProps).
//
//  Parameters and return values are exacly the same as those for GetObject
//  as described in help
//
//***************************************************************************
HRESULT CWbemNamespace::Exec_GetObjectByPath(
    READONLY LPWSTR wszObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    NEWOBJECT IWbemClassObject** ppObj,
    NEWOBJECT IWbemClassObject** ppErrorObj
    )
{
    TIMETRACE(TimeTraces::GetObjectByPath);
    HRESULT hres = WBEM_S_NO_ERROR;

    CSynchronousSink* pSyncSink = CSynchronousSink::Create();
    if(pSyncSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pSyncSink->AddRef();
    IWbemClassObject* pErrorObj = NULL;
    IWbemClassObject* pObj = NULL;

    hres = Exec_GetObject(wszObjectPath, lFlags, pCtx, pSyncSink);
    if (SUCCEEDED(hres))
    {
        pSyncSink->Block();
        pSyncSink->GetStatus(&hres, NULL, &pErrorObj);

        if(SUCCEEDED(hres))
        {
            if(pSyncSink->GetObjects().GetSize() < 1)
            {
                pSyncSink->Release();
                ERRORTRACE((LOG_WBEMCORE, "Sync sink returned success with no objects!\n"));
                return WBEM_E_CRITICAL_ERROR;
            }
            pObj = pSyncSink->GetObjects()[0];
            pObj->AddRef();
        }
    }

    pSyncSink->Release();

    if(ppObj) *ppObj = pObj;
    else if(pObj) pObj->Release();

    if(ppErrorObj) *ppErrorObj = pErrorObj;
    else if(pErrorObj) pErrorObj->Release();

    return hres;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_GetObject(
    READONLY LPWSTR wszObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink)
{

    TIMETRACE(TimeTraces::GetObject);
    
    // Create a sink that will merge the localized qualifiers
    // over the top of the default qualifiers (if specified)
    // ======================================================

    CLocaleMergingSink *pLocaleSink = NULL;

    HRESULT hres = WBEM_S_NO_ERROR;
    if (wszObjectPath && wszObjectPath[0] && wszObjectPath[0] != L'_')   // SEC:REVIEWED 2002-03-22 : OK; prior testing ensures that this is valid
    {
        if ((lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS))
        {
            pLocaleSink = new CLocaleMergingSink(pSink, m_wsLocale, m_pThisNamespace);
            if(pLocaleSink == NULL)
                return pSink->Return(WBEM_E_OUT_OF_MEMORY);
            else
            {
                pLocaleSink->AddRef();
                pSink = pLocaleSink;
            }
        }
    }
    CReleaseMe rm(pLocaleSink);

    COperationError OpInfo(pSink, L"GetObject", wszObjectPath?wszObjectPath:L"");
    if (!OpInfo.IsOk())   return pSink->Return(WBEM_E_OUT_OF_MEMORY);

    // Check if the path is NULL --- that's valid
    // ==========================================
    if (wszObjectPath == NULL || wszObjectPath[0] == 0)   // SEC:REVIEWED 2002-03-22 : OK; null terminator provably exists
    {
        //
        // BUGBUG consider using CoCreateInstance instead of new CWbemClass
        //
        CWbemClass * pNewObj = new CWbemClass;
        if(NULL == pNewObj) return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
        CReleaseMe rm_((IWbemClassObject*)pNewObj);
        
        hres = pNewObj->InitEmpty(0);
        if (FAILED(hres)) return OpInfo.ErrorOccurred(hres);

        IWbemClassObject* pObj = pNewObj;
        pSink->Indicate(1, &pObj);
        return OpInfo.ErrorOccurred(WBEM_NO_ERROR);
    }

    // here we are sure the path is not null
    if (wcslen_max(wszObjectPath,g_PathLimit) > g_PathLimit)
        return OpInfo.ErrorOccurred(WBEM_E_QUOTA_VIOLATION);


    // Parse the object path to get the class involved.
    // ================================================

    IWbemPath *pPath = ConfigMgr::GetNewPath();
    if (pPath == 0)
    {
        return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
    }

    CReleaseMe _1(pPath);
    hres = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wszObjectPath);
    if (FAILED(hres))
    {
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT_PATH);
    }

    ULONGLONG uResponse;
    hres = pPath->GetInfo(0, &uResponse);
    if (FAILED(hres) || (
        (uResponse & WBEMPATH_INFO_IS_INST_REF) == 0 &&
        (uResponse & WBEMPATH_INFO_IS_CLASS_REF) == 0))
    {
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT_PATH);
    }


    if (!pPath->IsRelative(ConfigMgr::GetMachineName(), m_pThisNamespace))
    {
        // This path points to another namespace. Delegate to it instead
        // =============================================================

        hres = GetObjectByFullPath(wszObjectPath, pPath,
                lFlags, pCtx, OpInfo.GetSink());

        return OpInfo.ErrorOccurred(hres);
    }

    BOOL bInstance = (uResponse & WBEMPATH_INFO_IS_INST_REF);

    // The repository code can't handle paths like root\default:classname
    // So if there is a colon, pass a pointer to one past it

    WCHAR * pRelativePath = wszObjectPath;
    for(WCHAR * pTest = wszObjectPath;*pTest;pTest++)
    {
        if(*pTest == L':')
        {
            // In win2k, we allowed \\.\root\default:whatever, but not root\default:whatever
            // So, the following additional test was added

            if((uResponse & WBEMPATH_INFO_PATH_HAD_SERVER) == 0)
                return OpInfo.ErrorOccurred(WBEM_E_INVALID_OBJECT_PATH);

            pRelativePath = pTest+1;
            break;
        }
        else if (*pTest==L'=')
            break;      //got to key part...
    }

    if (bInstance)
    {
        CFinalizingSink* pFinalSink = new CFinalizingSink(this, OpInfo.GetSink());
        if(pFinalSink == NULL)  return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
        pFinalSink->AddRef();
        CReleaseMe rmFinal(pFinalSink);

        hres = Exec_GetInstance(pRelativePath, pPath, lFlags, pCtx, pFinalSink);
        if (FAILED(hres))  return OpInfo.ErrorOccurred(hres);
    }
    else
    {
        hres = Exec_GetClass(pRelativePath, lFlags, pCtx, OpInfo.GetSink());
        if (FAILED(hres))  return OpInfo.ErrorOccurred(hres);
    }

    return hres;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_GetInstance(LPCWSTR wszObjectPath,
        IWbemPath* pParsedPath, long lFlags, IWbemContext* pCtx,
        CBasicObjectSink* pSink)
{
    if (pParsedPath->IsSameClassName(L"__NTLMUser") ||
        pParsedPath->IsSameClassName(L"__NTLMGroup"))
    {
        if((lFlags & WBEM_FLAG_ONLY_STATIC) == 0)
        {

            ParsedObjectPath* pOutput = 0;         // todo, convert to use new parser
            CObjectPathParser p;
            int nStatus = p.Parse(wszObjectPath,  &pOutput);
            if (nStatus != 0)
            {
                p.Free(pOutput);
                return WBEM_E_INVALID_OBJECT_PATH;
            }

            HRESULT hr = GetSecurityClassInstances(pOutput, pSink, pCtx,lFlags);
            p.Free(pOutput);
            return hr;
        }
    }

    // Try static database first
    // =========================

    if((lFlags & WBEM_FLAG_NO_STATIC) == 0)
    {
        IWbemClassObject *pObj = 0;
        HRESULT hRes = CRepository::GetObject(m_pSession, m_pScopeHandle, wszObjectPath, lFlags, &pObj);

        if (SUCCEEDED(hRes))
        {
            hRes = WBEM_S_NO_ERROR;
            pSink->Add(pObj);
            pObj->Release();
            return pSink->Return(hRes);
        }

    }

    // Try dynamic
    // ===========

    return DynAux_GetInstance((LPWSTR)wszObjectPath, lFlags, pCtx, pSink);
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_GetClass(
    LPCWSTR pszClassName,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{
    HRESULT hRes = 0;
    IWbemClassObject* pErrorObj = 0;
    IWbemServices *pClassProv = 0;
    CSynchronousSink* pSyncSink = 0;
    BSTR bstrClass = 0;
    IWbemClassObject* pResultObj = 0;

    if (pszClassName == 0 || pSink == 0)
        return pSink->Return(WBEM_E_INVALID_PARAMETER);


    if (!m_bRepositOnly && m_pProvFact)
    {
        hRes = m_pProvFact->GetClassProvider(
                    0,                  // lFlags
                    pCtx,
                    m_wszUserName,
                    m_wsLocale,
                    m_pThisNamespace,                     // IWbemPath pointer
                    0,
                    IID_IWbemServices,
                    (LPVOID *) &pClassProv
                    );

        if (FAILED(hRes))
            return pSink->Return(hRes);
    }

    CReleaseMe _1(pClassProv);

    // First, try repository.  If it's there, end of story.
    // ====================================================

    if ((lFlags & WBEM_FLAG_NO_STATIC) == 0)
    {
        if (m_pNsHandle)
        {
            hRes = CRepository::GetObject(
                m_pSession,
                m_pNsHandle,
                pszClassName,
                0,
                &pResultObj
                );
        }
        else        // Something drastically wrong
        {
            hRes = WBEM_E_CRITICAL_ERROR;
            return pSink->Return(hRes);
        }

        if (SUCCEEDED(hRes) && pResultObj)
        {
            pSink->Add(pResultObj);
            pResultObj->Release();
            return pSink->Return(hRes);
        }
    }

    // If we are in repository-only mode, we don't bother
    // with dynamic classes.
    // ===================================================

    if (m_bRepositOnly || m_pProvFact == NULL)
        return pSink->Return(WBEM_E_NOT_FOUND);

    // If here, try the dynamic class providers.
    // =========================================
    // Build up a synchronous sink to receive the class.
    // =================================================

    pSyncSink = CSynchronousSink::Create();
    if (pSyncSink == NULL)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    pSyncSink->AddRef();
    CReleaseMe _2(pSyncSink);

    // Try to get it.
    // ==============


    bstrClass = SysAllocString(pszClassName);
    if (bstrClass == 0) return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    CSysFreeMe sfm(bstrClass);

    
    {
        CDecoratingSink * pDecore = new CDecoratingSink(pSyncSink, this);
        if(pDecore == NULL) return pSink->Return(WBEM_E_OUT_OF_MEMORY);
        pDecore->AddRef();
        CReleaseMe rmDecor(pDecore);
        hRes = pClassProv->GetObjectAsync(bstrClass, lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS, pCtx, pDecore);
    }

    if (FAILED(hRes))
        return pSink->Return(hRes);

    pSyncSink->Block();
    pSyncSink->GetStatus(&hRes, NULL, &pErrorObj);

    if (FAILED(hRes))
    {
        pSink->Return(hRes, pErrorObj);
        if (pErrorObj)
            pErrorObj->Release();
        return hRes;
    }

    // Otherwise, somebody claimed to have supplied it. Do we really believe them?  No choice.
    // =======================================================================================

    if(pSyncSink->GetObjects().GetSize() < 1)
    {
        ERRORTRACE((LOG_WBEMCORE, "Sync sink returned success with no objects!\n"));
        return pSink->Return(WBEM_E_CRITICAL_ERROR);
    }
    pResultObj = pSyncSink->GetObjects()[0];
    pSink->Add(pResultObj);
    pSink->Return(WBEM_S_NO_ERROR);

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// ok
HRESULT CWbemNamespace::ExecNotificationQuery(
    const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext* pCtx,
    IEnumWbemClassObject** ppEnum
    )
{
    try 
    {    
        HRESULT hRes = CheckNs();
        if (FAILED(hRes))
            return hRes;

        DEBUGTRACE((LOG_WBEMCORE,
            "CALL CWbemNamespace::ExecNotificationQuery\n"
            "   BSTR QueryLanguage = %S\n"
            "   BSTR Query = %S\n"
            "   lFlags = 0x%X\n"
            "   IEnumWbemClassObject **pEnum = 0x%X\n",
            QueryLanguage,
            Query,
            lFlags,
            ppEnum
            ));

        // Validate parameters
        // ===================

        if (ppEnum == NULL)
            return WBEM_E_INVALID_PARAMETER;
        *ppEnum = NULL;

        if ((lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) == 0)
            return WBEM_E_INVALID_PARAMETER;

        if ((lFlags & WBEM_FLAG_FORWARD_ONLY) == 0)
            return WBEM_E_INVALID_PARAMETER;

        if (lFlags
            & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS
            & ~WBEM_RETURN_IMMEDIATELY
            & ~WBEM_FLAG_FORWARD_ONLY
        #ifdef _WBEM_WHISTLER_UNCUT
            & ~WBEM_FLAG_MONITOR
        #endif
            )
            return WBEM_E_INVALID_PARAMETER;

        // Create Finalizer.
        // =================

        _IWmiFinalizer *pFnz = 0;
        hRes = CreateSyncFinalizer(pCtx, &pFnz);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pFnz);

        ULONG uTaskType = WMICORE_TASK_EXEC_NOTIFICATION_QUERY;
        if (lFlags & WBEM_RETURN_IMMEDIATELY)
            uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
        else
            uTaskType |= WMICORE_TASK_TYPE_SYNC;

        // Do the work.
        // ============

        hRes = _ExecNotificationQueryAsync(uTaskType, pFnz, 0, QueryLanguage, Query,
                        lFlags & ~WBEM_RETURN_IMMEDIATELY & ~WBEM_FLAG_FORWARD_ONLY,
                        pCtx, NULL);

        if (FAILED(hRes))
        {
            return hRes;
        }

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
            pFnz->GetOperationResult(0, INFINITE, &hRes);

        if (SUCCEEDED(hRes))
        {
            IEnumWbemClassObject* pEnum = NULL;
            hRes = pFnz->GetResultObject(lFlags, IID_IEnumWbemClassObject, (LPVOID*)&pEnum);
            if (FAILED(hRes))
                return hRes;
            CReleaseMe _2(pEnum);

            if (SUCCEEDED(hRes))
            {
                *ppEnum = pEnum;
                pEnum->AddRef();    // Counteract CReleaseMe
            }
        }

        return hRes;
    }
    catch(...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }
}


//
//
// this function can throw or return
//
//////////////////////////////////////////////////////////
HRESULT CWbemNamespace::_ExecNotificationQueryAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    IN const BSTR strQueryLanguage,
    IN const BSTR strQuery,
    IN long lFlags,
    IN IWbemContext __RPC_FAR *pCtx,
    IN IWbemObjectSink __RPC_FAR *pHandler
    )
{

    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;
    if (!Allowed(WBEM_ENABLE))
        return WBEM_E_ACCESS_DENIED;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::_ExecNotificationQueryAsync\n"
        "   BSTR QueryLanguage = %S\n"
        "   BSTR Query = %S\n"
        "   lFlags = 0x%X\n"
        "   IWbemObjectSink* pHandler = 0x%X\n",
        strQueryLanguage,
        strQuery,
        lFlags,
        pHandler));

    // Parameter validation.
    // =====================
    if (pFnz == 0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;
    
    if (strQueryLanguage == 0 || strQuery == 0 || strQueryLanguage[0] == 0) 
        return WBEM_E_INVALID_PARAMETER;
    
    if (strQuery[0] == 0 )
        return WBEM_E_UNPARSABLE_QUERY;

   if (wcslen_max(strQuery,g_QueryLimit) > g_QueryLimit) return WBEM_E_QUOTA_VIOLATION;            

    if (lFlags & ~WBEM_FLAG_SEND_STATUS & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS )
        return WBEM_E_INVALID_PARAMETER;

    m_bForClient=FALSE;     // Forces a cheap fast-track

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Add the request to the queue.
    // =============================

    IWbemEventSubsystem_m4* pEss = ConfigMgr::GetEssSink();
    CReleaseMe _3(pEss);
    if (pEss == 0)
    {
        return WBEM_E_NOT_SUPPORTED;  // ESS must be disabled
    }

    HANDLE hEssValidate = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (NULL == hEssValidate) return WBEM_E_OUT_OF_MEMORY;
    CCloseMe cm(hEssValidate);



    HRESULT hResEssCheck = 0;
    wmilib::auto_ptr<CAsyncReq_ExecNotificationQueryAsync> pReq;
    pReq.reset(new CAsyncReq_ExecNotificationQueryAsync(this, pEss, strQueryLanguage,
                        strQuery, lFlags, pPseudoSink, pCtx,
                        &hResEssCheck, hEssValidate));

    if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;
    if ( NULL == pReq->GetContext() )  return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes))  return hRes;

    _1.release();
     // Enqueue the request.
    hRes = ConfigMgr::EnqueueRequest(pReq.get());

    if (FAILED(hRes))
    {
        pFnz->CancelTask (0);      
        return hRes;
    }

    pReq.release(); // queue took ownership

    // In this case, we have to wait long enough for ESS to accept the task.
    WaitForSingleObject(hEssValidate, INFINITE);

    // If ESS failed, we should cancel the task
    // ========================================
    if ( FAILED (hResEssCheck) )
    {
        pFnz->CancelTask(0);
    }
    return hResEssCheck;
}


//***************************************************************************
//
//***************************************************************************
// done

HRESULT CWbemNamespace::ExecNotificationQueryAsync(
    IN const BSTR strQueryLanguage,
    IN const BSTR strQuery,
    IN long lFlags,
    IN IWbemContext __RPC_FAR *pCtx,
    IN IWbemObjectSink __RPC_FAR *pHandler
    )
{
    try
    {
        return _ExecNotificationQueryAsync(WMICORE_TASK_TYPE_ASYNC | WMICORE_TASK_EXEC_NOTIFICATION_QUERY,
                                                             0, 0,
                                                             strQueryLanguage, strQuery, lFlags, pCtx, pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;    
    }
}



//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::GetImplementationClass(
    IWbemClassObject * pTestClass,
    LPWSTR wszMethodName,
    IWbemContext* pCtx,
    IWbemClassObject ** ppClassObj
    )
{
    try
    {
        // If the method is disabled, or implemented in this class, we are done!
        // =====================================================================

        CVar Var;
        CWbemClass * pClassDef = (CWbemClass *)pTestClass;

        HRESULT hres = pClassDef->GetMethodQualifier(wszMethodName, L"DISABLED", &Var);
        if(hres == S_OK && Var.GetBool() == VARIANT_TRUE)
            return WBEM_E_METHOD_DISABLED;

        hres = pClassDef->GetMethodQualifier(wszMethodName, L"IMPLEMENTED", &Var);
        if(hres == S_OK && Var.GetBool() == VARIANT_TRUE)
        {
            // The test class is correct, return it

            pTestClass->AddRef();
            *ppClassObj = pTestClass;
            return S_OK;
        }
        // Not done, get the name of the parent class.

        SCODE hRes = pClassDef->GetSystemPropertyByName(L"__superclass", &Var);
        if(hRes != S_OK)
            return WBEM_E_CRITICAL_ERROR;

        if(Var.GetType() != VT_BSTR)
            return WBEM_E_METHOD_NOT_IMPLEMENTED; // no superclass --- no implementation

        BSTR bstrParent = Var.GetBSTR();
        if(bstrParent == NULL)
            return WBEM_E_CRITICAL_ERROR; // NULL, but not VT_NULL

        if(wcslen(bstrParent) < 1) 
        {
            SysFreeString(bstrParent);
            return WBEM_E_FAILED; // empty parent name????
        }

        IWbemClassObject * pParent = NULL;
        hres = Exec_GetObjectByPath(bstrParent, 0, pCtx, &pParent, NULL);
        SysFreeString(bstrParent);
        if(FAILED(hres))
            return WBEM_E_FAILED; // class provider failure or weird interaction

        hRes = GetImplementationClass(pParent, wszMethodName, pCtx, ppClassObj);
        pParent->Release();
        return hRes;

    }
    catch(CX_Exception &)
    {
        return WBEM_E_FAILED;
    }
}

//***************************************************************************
//
//  CWbemNamespace::Exec_ExecMethod
//
//  Executes a method.  If the method is not tagged by the [bypass_getobject]
//  qualifier, the method is passed directly to the method provider.  Otherwise,
//  a GetObject call is done first to ensure the instance is valid.
//
//***************************************************************************
//
HRESULT CWbemNamespace::Exec_ExecMethod(
    LPWSTR wszObjectPath,
    LPWSTR wszMethodName,
    long lFlags,
    IWbemClassObject *pInParams,
    IWbemContext *pCtx,
    CBasicObjectSink* pSink
    )
{
    TIMETRACE(TimeTraces::ExecMethod);
    // Lotsa useful variables.
    // =======================

    HRESULT hRes;
    IWbemClassObject* pClassDef = NULL;
    IWbemClassObject* pImplementationClass = NULL;
    IWbemPath *pPath = 0;
    IWbemQualifierSet *pQSet = 0;
    IWbemClassObject* pErrorObj = NULL;
    BOOL bPathIsToClassObject = FALSE;
    LPWSTR pszClassName = 0;
    ULONGLONG uInf = 0;

    // Set up a sink error object and check it.
    COperationError OpInfo(pSink, L"ExecMethod", wszObjectPath);
    if ( !OpInfo.IsOk() ) return WBEM_E_OUT_OF_MEMORY;

    // Parse the path to the object.
    // =============================

    // Backwards compatibility - parsing a NULL path returns WBEM_E_INVALID_OBJECT_PATH
    if ( NULL == wszObjectPath || NULL == *wszObjectPath )
    {
        return OpInfo.ErrorOccurred( WBEM_E_INVALID_METHOD );
    }

    hRes = m_pCoreSvc->CreatePathParser(0, &pPath);
    if (FAILED(hRes))
        return OpInfo.ErrorOccurred(hRes);
    CReleaseMe _1(pPath);

    hRes = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wszObjectPath);
    if (FAILED(hRes))
        return OpInfo.ErrorOccurred(hRes);

    ULONG uBuf = 0;
    hRes = pPath->GetClassName(&uBuf, 0);      // Discover the buffer size
    if (FAILED(hRes))
        return OpInfo.ErrorOccurred(hRes);

    pszClassName = new wchar_t[uBuf+1];         // Allocate a buffer for the class name
    if (pszClassName == 0)
        return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);

    wmilib::auto_buffer <wchar_t> _2(pszClassName);   // Auto-delete buffer

    hRes = pPath->GetClassName(&uBuf, pszClassName);    // Get class name
    if (FAILED(hRes))
        return OpInfo.ErrorOccurred(hRes);

    // Find out if a path to an instance or a class.
    // ==============================================
    hRes = pPath->GetInfo(0, &uInf);
    if (FAILED(hRes))
        return OpInfo.ErrorOccurred(hRes);

    if (uInf & WBEMPATH_INFO_IS_INST_REF)
        bPathIsToClassObject = FALSE;
    else
        bPathIsToClassObject = TRUE;

    // Get the class definition.  We'll need it whether or not we validate the
    // instance or not.
    // =======================================================================

    hRes = Exec_GetObjectByPath(pszClassName,
            (lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS),  pCtx,
            &pClassDef, &pErrorObj);

    if (FAILED(hRes))
    {
        OpInfo.ErrorOccurred(hRes, pErrorObj);
        if (pErrorObj)
            pErrorObj->Release();
        return hRes;
    }

    CReleaseMe _3(pClassDef);

    // Now see if the method exists and if the class definition
    // has the [bypass_getobject] qualifier on that method.
    // ========================================================

    hRes = pClassDef->GetMethodQualifierSet(wszMethodName, &pQSet);
    if (FAILED(hRes))
    {
        // Means the method doesn't even exist
        if ( WBEM_E_NOT_FOUND == hRes )
        {
            hRes = WBEM_E_INVALID_METHOD;
        }

        return OpInfo.ErrorOccurred(hRes);
    }
    CReleaseMe _4(pQSet);

    hRes = pQSet->Get(L"bypass_getobject", 0, 0, 0);

    if (hRes == WBEM_E_NOT_FOUND)
    {
        // If here, we are going to get the object pointed to by the path first to ensure it is
        // valid. Note that the object may be either an instance or class object
        //
        // First, merge in the __GET_EXT_KEYS_ONLY during the GetObject calls to allow
        // the provider to quickly verify the existence of the object.  We don't
        // actually care about the property values other than the keys. We use
        // a copy of the context object, as we want to merge in KEYS_ONLY behavior
        // for the next call only.
        // ============================================================================
        IWbemClassObject *pVerifiedObj = 0;
        IWbemContext *pCopy = 0;

        if (pCtx)
            pCtx->Clone(&pCopy);
        hRes = MergeGetKeysCtx(pCopy);
        if (FAILED(hRes))
            return OpInfo.ErrorOccurred(hRes);

        // If here, we are verifying the object exists before passing the
        // control to the method handler.
        // ==============================================================

        hRes = Exec_GetObjectByPath(wszObjectPath, lFlags, pCopy,
            &pVerifiedObj, &pErrorObj);

        if (pCopy)
            pCopy->Release();

        if (FAILED(hRes))
        {
            OpInfo.ErrorOccurred(hRes, pErrorObj);
            if (pErrorObj)
                pErrorObj->Release();
            return hRes;
        }

        // If here, the class or instance exists!!
        // =======================================

        pVerifiedObj->Release();
    }
    else if (FAILED(hRes))
    {
        return OpInfo.ErrorOccurred(hRes);
    }


    // If this is the special internal security object, handle it internally
    // ======================================================================

    CVar Value;
    hRes = ((CWbemClass *) pClassDef)->GetSystemPropertyByName(L"__CLASS", &Value);
    if (hRes == S_OK && Value.GetType() == VT_BSTR && !Value.IsDataNull())
       if (!wbem_wcsicmp(Value.GetLPWSTR(), L"__SystemSecurity"))
           return SecurityMethod(wszMethodName, lFlags, pInParams, pCtx, pSink);

    // Make sure we have security.
    // ===========================

    if (!Allowed(WBEM_METHOD_EXECUTE))
        return OpInfo.ErrorOccurred(WBEM_E_ACCESS_DENIED);

    // Now, we locate the exact implementation of the method. After all, the
    // subclass may have been very lazy and relied on its parent implementation,
    // the way many kids rely on their parents for gas money.
    // =========================================================================

    hRes = GetImplementationClass(pClassDef, wszMethodName, pCtx, &pImplementationClass);
    if (FAILED(hRes))
        return OpInfo.ErrorOccurred(hRes);

    // The "pImplementatinClass" now points to the class object where the methods is implemented
    // =========================================================================================

    CReleaseMe rm2(pImplementationClass);
    CWbemClass * pImplementationDef = (CWbemClass*)pImplementationClass;

    // Make sure that class paths are only used with static methods.
    // =============================================================

    CVar Var;
    if (bPathIsToClassObject)
    {
        hRes = pImplementationDef->GetMethodQualifier(wszMethodName, L"STATIC", &Var);
        if (hRes != S_OK || Var.GetBool() != VARIANT_TRUE)
        {
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_METHOD_PARAMETERS);
        }
    }

    // Get the provider name.
    // ======================

    CVar vProv;
    hRes = pImplementationDef->GetQualifier(L"Provider", &vProv);

    if (FAILED(hRes) || vProv.GetType() != VT_BSTR)
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_PROVIDER_REGISTRATION);

    // Adjust the path to reference the class of implementation
    // ========================================================

    CVar vImpClassName;
    hRes = pImplementationDef->GetClassName(&vImpClassName);
    if (FAILED(hRes) || vImpClassName.GetType() != VT_BSTR)
        return OpInfo.ErrorOccurred(WBEM_E_CRITICAL_ERROR);

    BSTR strNewPath = CQueryEngine::AdjustPathToClass(wszObjectPath,
                                                    vImpClassName.GetLPWSTR());
    if (strNewPath == NULL)
        return OpInfo.ErrorOccurred(WBEM_E_CRITICAL_ERROR);

    CSysFreeMe sfm1(strNewPath);

    // Load the provider and execute it.
    // ==================================

    CMethodSink * pMethSink = new CMethodSink(OpInfo.GetSink());
    if(pMethSink == NULL)
       return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
    pMethSink->AddRef();
    CReleaseMe _5(pMethSink);

    // Find provider.
    // ==============

    IWbemServices *pProv = 0;
    if(m_pProvFact == NULL)
        hRes = WBEM_E_CRITICAL_ERROR;
    else
    {
        WmiInternalContext t_InternalContext ;
        ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;   // SEC:REVIEWED 2002-03-22 : OK

        hRes = m_pProvFact->GetProvider(

            t_InternalContext ,
            0,                  // lFlags
            pCtx,
            0,
            m_wszUserName,
            m_wsLocale,
            0,                      // IWbemPath pointer
            vProv,     // Provider
            IID_IWbemServices,
            (LPVOID *) &pProv
        );
    }

    if (FAILED(hRes))
    {
        return pSink->Return(hRes);
    }

    CReleaseMe _(pProv);

    hRes = pProv->ExecMethodAsync(
        strNewPath,
        wszMethodName,
        lFlags& ~WBEM_FLAG_USE_AMENDED_QUALIFIERS,
        pCtx,
        pInParams,
        pMethSink
        );

    return hRes;
}



//***************************************************************************
//
//  CWbemNamespace::GetOrPutDynProps
//
//  Processes an instance to see if any properties have been marked
//  as 'dynamic'.
//
//  Short-circuit logic is in effect.  The instance as a whole must be
//  marked with the following Qualifier to signal that the instance has
//  dynamic properties which need evaluation:
//
//      "DYNPROPS" (VT_BOOL) = VARIANT_TRUE
//
//  Optionally, the instance may contain:
//      "INSTANCECONTEXT" VT_BSTR = <provider specific string>
//
//  In addition, each dynamic property is marked
//
//      "DYNAMIC"           VT_BOOL     VARIANT_TRUE
//      "LOCATORCLSID"      VT_BSTR     CLSID of the provider
//      "PROPERTYCONTEXT"   VT_BSTR     <provider specific string>
//
//  "INSTANCECONTEXT" and "PROPERTYCONTEXT" are not checked by this code,
//  since they are optional for each provider.
//
//  PARAMETERS:
//
//      IWbemClassObject* pObj       The object to fill in dynamic properties
//                                  in.
//      Operation op                Can be GET or PUT depending on what is
//                                  needed.
//      bool bIsDynamic             True if a dynamically provided class.  Note that it
//                                  would be very strange to have a dynamic class with
//                                  dynamic properties.
//  RETURN VALUES:
//      <WBEM_NO_ERROR>  No provider was involved or if a provider was
//                        involved, properties were all evaluated.
//
//      <WBEM_E_INVALID_OBJECT>
//          Object was marked as dynamic, but other Qualifiers were missing.
//
//      <WBEM_E_PROVIDER_NOT_FOUND>
//          One or more of the specified providers could not be found.
//
//      <WBEM_E_PROVIDER_FAILURE>
//          One or more providers were not able to provide the properties.
//
//      <WBEM_E_CRITICAL_ERROR>
//
//***************************************************************************

HRESULT CWbemNamespace::GetOrPutDynProps(
    IWbemClassObject *pObj,
    Operation op,
    BOOL bIsDynamic
    )
{
    HRESULT hRes;
    IWbemContext *pCtx = 0;
    CVar vDynTest;
    _IWmiDynamicPropertyResolver *pResolver = 0;
    IWbemQualifierSet *pQSet = 0;
    IWbemClassObject *pClassDef = 0;
    CVARIANT v;

    // Examine the instance to see if there are any dynamic properties.
    // ================================================================

    hRes = pObj->GetQualifierSet(&pQSet);
    if (FAILED(hRes))
        return WBEM_NO_ERROR;
    CReleaseMe _1(pQSet);

    hRes = pQSet->Get(L"DYNPROPS", 0, &v, 0);
    if (FAILED(hRes))
        return WBEM_S_NO_ERROR;
    if (v.GetBool() == FALSE)
        return WBEM_S_NO_ERROR;

    v.Clear();
    hRes = pObj->Get(L"__CLASS", 0, &v, 0, 0);
    if (FAILED(hRes))
        return hRes;

    // Get the class definition for the object.
    // Must be static.
    // ========================================

    hRes = CRepository::GetObject(
             m_pSession,
             m_pNsHandle,
             v.GetStr(),
             0,
             &pClassDef
             );

    CReleaseMe _2(pClassDef);
    if (FAILED(hRes))
        return hRes;

    // Access provider subsystem to do the dirty work.
    // ================================================


    if (m_pProvFact == NULL)
        hRes = WBEM_E_CRITICAL_ERROR;
    else
    {
        pCtx = ConfigMgr::GetNewContext();
        if ( pCtx == NULL )
            return WBEM_E_OUT_OF_MEMORY;

        hRes = m_pProvFact->GetDynamicPropertyResolver (
                                 0,          // lFlags
                                 pCtx,   // context
                                 m_wszUserName,
                                 m_wsLocale,
                                 IID__IWmiDynamicPropertyResolver,
                                 (LPVOID *)&pResolver);
    }

    CReleaseMe _1_pCtx (pCtx) ;

    if (FAILED(hRes))
        return hRes;

    CReleaseMe _3(pResolver);

    // Determine if a put or a get.
    // ============================

    if (op == GET)
    {
        hRes = pResolver->Read(pCtx, pClassDef, &pObj);
    }
    else if (op == PUT)
    {
        hRes = pResolver->Write(pCtx, pClassDef, pObj);
    }
    else
        return WBEM_E_INVALID_PARAMETER;

    return hRes;
}

//***************************************************************************
//
//  AddKey
//
//  Adds a keyname/value pair to a normalized path
//
//  throw CX_MemoryException
//
//***************************************************************************

HRESULT AddKey(WString & wNormalString, WCHAR * pwsKeyName, VARIANT *pvKeyValue,
                                                            int & iNumKey, CWbemInstance* pClassDef)
{
    if(iNumKey++ > 0)
        wNormalString += L",";              // prepend comma for all but the first key

    wNormalString += pwsKeyName;
    wNormalString += "=";
    if(pvKeyValue->vt == VT_BSTR)
    {
        wNormalString += L"\"";

        // if there are any quotes, they must be prepended with a back slash;
        // Also, any back slashes should be doubled up.

        int iLen = 1;       // one for the terminator;
        WCHAR * pTest;
        for(pTest = pvKeyValue->bstrVal;*pTest; pTest++, iLen++)
            if(*pTest == L'\"' || *pTest == L'\\')
                iLen++;
        WCHAR * pString = new WCHAR[iLen];
        if(pString == NULL)
            throw CX_MemoryException();
        wmilib::auto_buffer<WCHAR> rm_(pString);

        WCHAR * pTo = pString;
        for(pTest = pvKeyValue->bstrVal;*pTest; pTest++, pTo++)
        {
            if(*pTest == L'\"' || *pTest == L'\\')
            {
                *pTo = L'\\';
                pTo++;
            }
            *pTo = *pTest;
        }
        *pTo = 0;

        wNormalString += pString;
        wNormalString += L"\"";
        return S_OK;
    }
    if(pvKeyValue->vt != VT_EMPTY && pvKeyValue->vt != VT_NULL)
    {

        // special case for large unsigned numbers
        if(pvKeyValue->vt == VT_I4 && pvKeyValue->lVal < 0)
        {
            CIMTYPE ct;
            HRESULT hRes = pClassDef->Get(pwsKeyName, 0, NULL, &ct, NULL);
            if(hRes == S_OK && ct == CIM_UINT32)
            {
                WCHAR wBuff[32];
                StringCchPrintfW(wBuff, 32, L"%u",pvKeyValue->lVal);
                wNormalString += wBuff;
                return S_OK;
            }
        }

        _variant_t var;
        HRESULT hRes = VariantChangeType(&var, pvKeyValue, 0, VT_BSTR);
        if(hRes == S_OK)
        {
            wNormalString += var.bstrVal;
        }

        return hRes;
    }
    return WBEM_E_INVALID_OBJECT_PATH;
}

//***************************************************************************
//
//  NormalizeObjectPath
//
//  Creates a normalized object path for passing to providers.
//***************************************************************************

HRESULT NormalizeObjectPath(ParsedObjectPath*pOutput, WString & wNormalString,
                            CWbemInstance* pClassDef)
{
    try
    {
        HRESULT hRes;

        // For singleton, so long as the class is singleton

        if(pOutput->m_bSingletonObj)
        {
            CVar Singleton;
            hRes = pClassDef->GetQualifier(L"SINGLETON", &Singleton);
            if (hRes == 0 && Singleton.GetBool() != 0)
            {
                wNormalString = pOutput->m_pClass;
                wNormalString += "=@";
                return S_OK;
            }
            else
                return WBEM_E_INVALID_OBJECT_PATH;
        }

        int iKeyNum = 0;
        int iNumMatch = 0;          // number of keys in the path which were found in the class def

        // Start off by writting the class name followe by a dot

        wNormalString = pOutput->m_pClass;
        wNormalString += L".";

        CWStringArray ClassKeyNames;
        if(!pClassDef->GetKeyProps(ClassKeyNames))
            return WBEM_E_INVALID_CLASS;

        // For each key in the class definition

        for(int iClassKey = 0; iClassKey < ClassKeyNames.Size(); iClassKey++)
        {
            // look for the class key in the path

            bool bClassKeyIsInPath = false;
            int iPathKey;

            for(iPathKey = 0; iPathKey < pOutput->m_dwNumKeys; iPathKey++)
            {
                KeyRef * key = pOutput->m_paKeys[iPathKey];
                if(key->m_pName == 0 && ClassKeyNames.Size() == 1 && pOutput->m_dwNumKeys==1)
                {
                    bClassKeyIsInPath = true;
                    break;
                }
                else if(key->m_pName && !wbem_wcsicmp(key->m_pName, ClassKeyNames[iClassKey]))
                {
                    bClassKeyIsInPath = true;
                    break;
                }
            }
            if(bClassKeyIsInPath)
            {
                iNumMatch++;
                // todo, check type

                KeyRef * key = pOutput->m_paKeys[iPathKey];
                hRes = AddKey(wNormalString, ClassKeyNames[iClassKey],
                                      &key->m_vValue, iKeyNum, pClassDef);
                if(FAILED(hRes))
                    return hRes;
            }
            else
            {
                // If the key has a default value, then use it
                _variant_t var;
                hRes = pClassDef->Get(ClassKeyNames[iClassKey], 0, &var, NULL, NULL);
                if(FAILED(hRes) || var.vt == VT_EMPTY || var.vt == VT_NULL)
                    return WBEM_E_INVALID_OBJECT_PATH;
                hRes = AddKey(wNormalString, ClassKeyNames[iClassKey], &var, iKeyNum,pClassDef);
                if(FAILED(hRes))
                    return hRes;
            }
        }

        if(iNumMatch == pOutput->m_dwNumKeys)
            return S_OK;
        else
            return WBEM_E_INVALID_OBJECT_PATH;
    }
    catch (CX_MemoryException &)
    {

        return WBEM_E_OUT_OF_MEMORY;
    }
}

//***************************************************************************
//
//  CWbemNamespace::DynAux_GetInstance
//
//  Retrieves an instance identified by its path from the dynamic provider
//  registered for that class.
//
//  PARAMETERS:
//
//      IN DWORD  dwNamespace             Namespace handle to the current
//                                        namespace (see objdb.h)
//      IN LPWSTR pObjectPath             Object path to the instance.
//      IN long lFlags                    Flags. Propagated to provider.
//      OUT IWbemClassObject** pObj        Destination for the class definition.
//                                        The caller must release this object
//                                        if the call is successful.
//      OUT IWbemClassObject** ppErrorObj  Destination for the error object. May
//                                        be NULL. Otherwise, the returned
//                                        pointer must be released if not NULL.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              Success.
//      WBEM_E_NOT_FOUND             No such instance, says provider, or the
//                                  class is not dynamic.
//      WBEM_E_INVALID_PARAMETER     One or more parameters are invalid.
//      WBEM_E_INVALID_CLASS         The class specified in the path does not
//                                  exist.
//      WBEM_E_FAILED                Unexpected error occured.
//      WBEM_E_PROVIDER_NOT_FOUND    Provider for this class could not be
//                                  located --- not registered with us or COM.
//      WBEM_E_PROVIDER_FAILURE      Provider reported an error while looking
//                                  for this object.
//      WBEM_E_PROVIDER_NOT_CAPABLE  Provider for this class is not capable of
//                                  getting objects by path.
//
//***************************************************************************

HRESULT CWbemNamespace::DynAux_GetInstance(
    IN LPWSTR wszObjectPath,
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN CBasicObjectSink* pSink
    )
{
    // Parse the object path to get the class involved.
    // ================================================
    ParsedObjectPath* pOutput = 0;
    CObjectPathParser p;
    int nStatus = p.Parse(wszObjectPath,  &pOutput);
    OnDeleteObj<ParsedObjectPath*,CObjectPathParser,
                         void (CObjectPathParser:: *)(ParsedObjectPath *),
                         &CObjectPathParser::Free> FreeMe(&p,pOutput);    

    if(CObjectPathParser::NoError != nStatus || !pOutput->IsInstance())
        return pSink->Return(WBEM_E_INVALID_OBJECT_PATH);

    HRESULT hres = WBEM_E_FAILED;
    IWbemClassObject* pErrorObj = NULL;
    CReleaseMeRef<IWbemClassObject*> rmErrObj(pErrorObj);
    CSetStatusOnMe SetMe(pSink,hres,pErrorObj);

    // See if this class is actually provided dynamically

    BSTR strClass = SysAllocString(pOutput->m_pClass);
    if (NULL == strClass) 
    {
        return hres = WBEM_E_OUT_OF_MEMORY;
    }
    CSysFreeMe sfm(strClass);

    CWbemInstance *pClassDef = 0;
    IWbemClassObject* pClassObj = NULL;

    hres = Exec_GetObjectByPath(strClass,
                            lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                            pCtx,&pClassObj, NULL);
    if(FAILED(hres))
    {
        hres = (hres == WBEM_E_NOT_FOUND) ? WBEM_E_INVALID_CLASS : WBEM_E_FAILED;
        return hres;
    }

    CReleaseMe rm(pClassObj);
    pClassDef = (CWbemInstance*)pClassObj;

    WString wNormalPath;
    hres = NormalizeObjectPath(pOutput, wNormalPath, pClassDef);
    if(FAILED(hres)) return hres;

    if(!pClassDef->IsKeyed())
        return hres = WBEM_E_INVALID_CLASS;

    // Make sure that this class is not static ---
    // i.e. either dynamic or abstract
    // ===========================================

    CVar vDynamic;
    hres = pClassDef->GetQualifier(L"Dynamic", &vDynamic);
    if(FAILED(hres) || vDynamic.GetType() != VT_BOOL || !vDynamic.GetBool())
    {
        // Not dynamic. Check if it is abstract
        // ====================================

        CVar vAbstract;
        hres = pClassDef->GetQualifier(L"Abstract", &vAbstract);
        if(FAILED(hres) || vAbstract.GetType() != VT_BOOL || !vAbstract.GetBool())
            return hres = WBEM_E_NOT_FOUND;
    }

    // Build the class hierarchy
    // =========================

    wmilib::auto_ptr<CDynasty> pDynasty;
    hres = DynAux_BuildClassHierarchy(strClass, lFlags, pCtx, pDynasty,&pErrorObj);
    if(FAILED(hres)) return hres;

    rmErrObj.release(); // set it to null anyway


    // If direct read is requested, only ask the provider in question.
    // ===============================================================

    if (lFlags & WBEM_FLAG_DIRECT_READ)
    {
        DynAux_GetSingleInstance((CWbemClass*) pClassObj,lFlags, wszObjectPath, pCtx, pSink);
    }
    else
    {
        // Create merging sink        
        hres = WBEM_E_OUT_OF_MEMORY; // pre-set the failure
        CSingleMergingSink* pMergeSink = new CSingleMergingSink(pSink, strClass); // throw
        if(pMergeSink == NULL)  return hres;
        pMergeSink->AddRef();
        CReleaseMe rm(pMergeSink);

        // Ask all providers
        DynAux_AskRecursively(pDynasty.get(), lFlags, wNormalPath, pCtx,pMergeSink);
    }

    SetMe.dismiss();
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::DynAux_AskRecursively(CDynasty* pDynasty,
                                              long lFlags,
                                              LPWSTR wszObjectPath,
                                              IWbemContext* pCtx,
                                              CBasicObjectSink* pSink)
{
    // Convert the path to the new class
    // =================================

    BSTR strNewPath = CQueryEngine::AdjustPathToClass(wszObjectPath, pDynasty->m_wszClassName);
    if(strNewPath == NULL) return pSink->Return(WBEM_E_INVALID_OBJECT_PATH);
    CSysFreeMe sfm(strNewPath);

    // Get this provider's object
    // ==========================
    DynAux_GetSingleInstance((CWbemClass*)pDynasty->m_pClassObj,lFlags, strNewPath, pCtx, pSink);

    // Get the children's objects
    // ==========================

    for(int i = 0; i < pDynasty->m_Children.Size(); i++)
    {
        CDynasty* pChildDyn = (CDynasty*)pDynasty->m_Children.GetAt(i);

        DynAux_AskRecursively(pChildDyn, lFlags, wszObjectPath, pCtx,pSink);
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::DynAux_GetSingleInstance(CWbemClass* pClassDef,
                                                 long lFlags,
                                                 LPWSTR wszObjectPath,
                                                 IWbemContext* pCtx,
                                                 CBasicObjectSink* pSink)
{

    COperationError OpInfo(pSink, L"GetObject", wszObjectPath, FALSE);
    // the ctor calls setstatus in bad luck case
    if (!OpInfo.IsOk()) return WBEM_E_OUT_OF_MEMORY;

    // Verify that the class is indeed dynamic
    // =======================================
    if(!pClassDef->IsDynamic())
        return OpInfo.ErrorOccurred(WBEM_E_NOT_FOUND);

    CVar vProvName;
    HRESULT hres = pClassDef->GetQualifier(L"Provider", &vProvName);
    if(FAILED(hres) || vProvName.GetType() != VT_BSTR)
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_PROVIDER_REGISTRATION);

    OpInfo.SetProviderName(vProvName.GetLPWSTR()); //throw

    // Access the provider cache.
    // ==========================

    IWbemServices *pProv = 0;
    HRESULT hRes;
    if(m_pProvFact == NULL)
        hRes = WBEM_E_CRITICAL_ERROR;
    else
    {
        WmiInternalContext t_InternalContext ;
        ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;  // SEC:REVIEWED 2002-03-22 : OK

        hRes = m_pProvFact->GetProvider(

            t_InternalContext ,
            0,                  // lFlags
            pCtx,
            0,
            m_wszUserName,
            m_wsLocale,
            0,                      // IWbemPath pointer
            vProvName,     // Provider
            IID_IWbemServices,
            (LPVOID *) &pProv
        );
    }

    if (FAILED(hRes))
    {
     return pSink->Return(hRes);
    }

    CReleaseMe _(pProv);
    CDecoratingSink * pDecore = new CDecoratingSink(OpInfo.GetSink(), this);
    if(pDecore == NULL) return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    pDecore->AddRef();
    CReleaseMe cdecor(pDecore);

    hRes = pProv->GetObjectAsync(wszObjectPath, lFlags& ~WBEM_FLAG_USE_AMENDED_QUALIFIERS, pCtx, pDecore);

    return hRes;
}

//***************************************************************************
//
//  CWbemNamespace::DynAux_GetInstances
//
//  Gets all instances from the provider specified in the class
//  definition.  Does no inheritance joins; this is a simple retrieval
//  of all instances from the specified class.
//
//  Preconditions:
//  1. The class <pClassDef> is known to be marked 'dynamic', but no
//     other verification has been done on the class definition.
//  2. <pClassDef> is not NULL.
//
//  Postconditions:
//  1. <aInstances> is empty on all error conditions.
//
//  PARAMETERS:
//
//      READONLY CWbemObject *pClassDef  The definition of the class to retrieve
//                                      instances of.
//      long lFlags                     The flags (deep/shallow)
//      CFlexArray &aInstances          Destination for the instances.
//      IWbemClassObject** ppErrorObj    Destination for the error object. If
//                                      not NULL, an error object may be placed
//                                      here. It is the caller's responsibility
//                                      to release it if not NULL.
//  RETURN VALUES:
//
//      WBEM_NO_ERROR  No errors. This includes a no-error situation
//                  with zero instances returned.
//      WBEM_E_INVALID_PROVIDE_REGISTRATION  Provider registration for this
//                                          class is incomplete.
//      WBEM_E_PROVIDER_NOT_FOUND    Provider could not be located. It is not
//                                  registered with us or with COM.
//      WBEM_E_PROVIDER_NOT_CAPABLE  Provider is not capable of enumerating
//                                  instances.
//      WBEM_E_FAILED                Unexpected error has occured.
//
//***************************************************************************

HRESULT CWbemNamespace::DynAux_GetInstances(

    READONLY CWbemObject *pClassDef,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink ,
    BOOL bComplexQuery
)
{
    // First, get the current task - 
    // if there isn't one, then we are on an WbemESS thread
    // ESS use to issue "internal" queries that are processes without arbitration
    CWbemRequest*  pCurrReq = CWbemQueue::GetCurrentRequest();
    CWmiTask * pCurrTask = pCurrReq?((CWmiTask *)pCurrReq->m_phTask):NULL;
        
    // We'll need the finalizer in case we need to cancel something
    HRESULT    hr;
    
    _IWmiFinalizer*    pMainFnz = NULL;    
    if (pCurrTask)
    {
        hr = pCurrTask->GetFinalizer( &pMainFnz );
        if (FAILED(hr)) return hr;
    }    
    CReleaseMe    rm( pMainFnz );
        
    CWmiMerger* pWmiMerger = NULL;

    // Check if query arbitration is enabled
    if ( ConfigMgr::GetEnableQueryArbitration() && pCurrTask)
    {
        // Get the arbitrated query pointer and cast to a merger as appropriate
        _IWmiArbitratedQuery*    pArbQuery = NULL;

        hr = pCurrTask->GetArbitratedQuery( 0L, &pArbQuery );

        if ( SUCCEEDED( hr ) )
        {
            hr = pArbQuery->IsMerger();

            if ( SUCCEEDED( hr ) )
            {
                pWmiMerger = (CWmiMerger*) pArbQuery;
            }
            else
            {
                pArbQuery->Release();
            }
        }

        // Clear errors
        hr = WBEM_S_NO_ERROR;

    }    // IF Query arbitration enabled

    CReleaseMe    rmMerger( (_IWmiArbitratee*) pWmiMerger );

    // Perform correct handling based on whether or not we have a merger
    if ( pWmiMerger )
    {
        hr = pWmiMerger->RegisterArbitratedInstRequest( pClassDef, lFlags, pCtx, pSink,    bComplexQuery, this );

        if (FAILED( hr))
            if (pMainFnz) pMainFnz->CancelTask( 0 );
        
        return hr;
    }

    //
    // when the query arbitration is not enabled we fall here
    //
    CAsyncReq_DynAux_GetInstances * pReq;
    pReq = new CAsyncReq_DynAux_GetInstances (this,
	                                            pClassDef,
	                                            lFlags,
	                                            pCtx,
	                                            pSink); 


    if ( pReq == NULL)
    {
        if (pMainFnz) pMainFnz->CancelTask ( 0 );
        return WBEM_E_OUT_OF_MEMORY;
    }

    if ( NULL == pReq->GetContext() )
    {
        if (pMainFnz) pMainFnz->CancelTask ( 0 );
        delete pReq;
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Set the task for the request - we'll just use the existing one if any
    pReq->m_phTask = pCurrTask;
    if (pReq->m_phTask) pReq->m_phTask->AddRef();
    
    hr = ConfigMgr::EnqueueRequest(pReq);

    if (FAILED(hr))
    {
        if (pMainFnz) pMainFnz->CancelTask ( 0 );
        delete pReq;
    }

    return hr;

}

//***************************************************************************
//
//  CWbemNamespace::DynAux_GetInstances
//
//  Gets all instances from the provider specified in the class
//  definition.  Does no inheritance joins; this is a simple retrieval
//  of all instances from the specified class.
//
//  Preconditions:
//  1. The class <pClassDef> is known to be marked 'dynamic', but no
//     other verification has been done on the class definition.
//  2. <pClassDef> is not NULL.
//
//  Postconditions:
//  1. <aInstances> is empty on all error conditions.
//
//  PARAMETERS:
//
//      READONLY CWbemObject *pClassDef  The definition of the class to retrieve
//                                      instances of.
//      long lFlags                     The flags (deep/shallow)
//      CFlexArray &aInstances          Destination for the instances.
//      IWbemClassObject** ppErrorObj    Destination for the error object. If
//                                      not NULL, an error object may be placed
//                                      here. It is the caller's responsibility
//                                      to release it if not NULL.
//  RETURN VALUES:
//
//      WBEM_NO_ERROR  No errors. This includes a no-error situation
//                  with zero instances returned.
//      WBEM_E_INVALID_PROVIDE_REGISTRATION  Provider registration for this
//                                          class is incomplete.
//      WBEM_E_PROVIDER_NOT_FOUND    Provider could not be located. It is not
//                                  registered with us or with COM.
//      WBEM_E_PROVIDER_NOT_CAPABLE  Provider is not capable of enumerating
//                                  instances.
//      WBEM_E_FAILED                Unexpected error has occured.
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_DynAux_GetInstances(
    READONLY CWbemObject *pClassDef,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
    )
{

    COperationError OpInfo(pSink, L"CreateInstanceEnum", L"", FALSE);
    if ( !OpInfo.IsOk() ) return pSink->Return(WBEM_E_OUT_OF_MEMORY);    
    
    CVar vProv;
    CVar vClassName;
    
    // Get the provider name.
    // ======================

    try // internal fastprox interfaces throw
    {
        HRESULT hres = pClassDef->GetQualifier(L"Provider", &vProv);
        if (FAILED(hres) || vProv.GetType() != VT_BSTR)
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_PROVIDER_REGISTRATION);
        if (FAILED(pClassDef->GetClassName(&vClassName)))
            return pSink->Return(WBEM_E_OUT_OF_MEMORY);
        OpInfo.SetParameterInfo(vClassName.GetLPWSTR());
    }
    catch(CX_MemoryException &)
    {
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    }

    // Access Provider Subsystem.
    // ==========================

    IWbemServices *pProv = 0;
    HRESULT hRes;

    if (m_pProvFact == NULL)
        hRes = WBEM_E_CRITICAL_ERROR;
    else
    {
            WmiInternalContext t_InternalContext ;
            ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) );

            hRes = m_pProvFact->GetProvider(
                t_InternalContext ,
                0,                  // lFlags
                pCtx,
                0,
                m_wszUserName,
                m_wsLocale,
                0,                      // IWbemPath pointer
                vProv,     // Provider
                IID_IWbemServices,
                (LPVOID *) &pProv);
    }

    if (FAILED(hRes))
         return pSink->Return(hRes);
    CReleaseMe _1(pProv);

    // Set up the sink chain to be delivered to the provider.
    // The code & destruct sequence is critical and the
    // refcounting is very carefully thought out.  Do not
    // change this code unless you know exactly what you are
    // doing.  And don't even change it then.
    // ======================================================

    CProviderSink *pProvSink = new CProviderSink(1, vClassName.GetLPWSTR());
    if (pProvSink == 0)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    CReleaseMe _3(pProvSink);

    CDecoratingSink * pDecore = new CDecoratingSink(pSink, this);
    if (pDecore == NULL)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    pProvSink->SetNextSink(pDecore);

    // Before calling the provider, map the provider to the
    // task so that we can cancel it proactively, if required.
    // =======================================================

    hRes = ((CWmiArbitrator *) m_pArb)->MapProviderToTask(0, pCtx, pProv, pProvSink);
    if (FAILED(hRes))
        return pSink->Return(hRes);

    // Now tell the provider to start enumerating.
    hRes = pProv->CreateInstanceEnumAsync(vClassName.GetLPWSTR(),
                                                                    lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                                                    pCtx,
                                                                    pProvSink
                                                                    );

    return hRes;
}

//***************************************************************************
//
//  CWbemNamespace::DynAux_ExecQueryAsync
//
//  Executes a SQL-1 query against a dynamic instance provider for the class
//  in the query.
//
//  PARAMETERS:
//
//      IN DWORD dwNamespace            Namespace handle to the current
//                                      namespace (see objdb.h)
//      IN CWbemObject* pClassDef        Class definition of the class in the
//                                      query. Must be dynamic.
//      IN LPWSTR Query                 The query string.
//      IN LPWSTR QueryFormat           The query language. Must be WQL.
//      IN long lFlags                  The flags. Not used.
//      OUT CFlexArray &aInstances      Destinatino for the instances found.
//      OUT IWbemClassObject** ppErrorObj Destination for the error object. IF
//                                      not NULL, an error object may be placed
//                                      here. In this case, it is the caller's
//                                      responsibility to release it.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR                  Success (even though there may not be
//                                      any instances).
//      WBEM_E_INVALID_PROVIDE_REGISTRATION  Provider registration for this
//                                          class is incomplete.
//      WBEM_E_PROVIDER_NOT_FOUND    Provider could not be located. It is not
//                                  registered with us or with COM.
//      WBEM_E_PROVIDER_NOT_CAPABLE  Provider is not capable of enumerating
//                                  instances.
//      WBEM_E_FAILED                Unexpected error has occured.
//
//***************************************************************************

HRESULT CWbemNamespace::DynAux_ExecQueryAsync (

    CWbemObject* pClassDef,
    LPWSTR Query,
    LPWSTR QueryFormat,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink,
    BOOL bComplexQuery
)
{
    // First, get the current task - if there isn't one, something
    // is very wrong.

    CWbemRequest*        pCurrReq = CWbemQueue::GetCurrentRequest();

    _DBG_ASSERT( NULL != pCurrReq && NULL != pCurrReq->m_phTask );

    if ( NULL == pCurrReq || NULL == pCurrReq->m_phTask )
    {
        return WBEM_E_FAILED;
    }

    // We'll need the finalizer in case we need to cancel something
    _IWmiFinalizer*    pMainFnz = NULL;
    
    HRESULT    hr = ((CWmiTask*) pCurrReq->m_phTask)->GetFinalizer( &pMainFnz );
    CReleaseMe    rm( pMainFnz );

    if ( SUCCEEDED( hr ) )
    {
        CWmiMerger*                pWmiMerger = NULL;

        // Check if query arbitration is enabled
        if ( ConfigMgr::GetEnableQueryArbitration() )
        {
            // Get the arbitrated query pointer and cast to a merger as appropriate
            _IWmiArbitratedQuery*    pArbQuery = NULL;

            hr = ((CWmiTask*) pCurrReq->m_phTask)->GetArbitratedQuery( 0L, &pArbQuery );

            if ( SUCCEEDED( hr ) )
            {
                hr = pArbQuery->IsMerger();

                if ( SUCCEEDED( hr ) )
                {
                    pWmiMerger = (CWmiMerger*) pArbQuery;
                }
                else
                {
                    pArbQuery->Release();
                }
            }

            // Clear errors
            hr = WBEM_S_NO_ERROR;

        }    // IF Query arbitration enabled

        // Auto cleanup
        CReleaseMe    rm( (_IWmiArbitratee*) pWmiMerger );

        // Perform correct handling based on whether or not we have a merger
        if ( NULL != pWmiMerger )
        {
            hr = pWmiMerger->RegisterArbitratedQueryRequest( pClassDef, lFlags, Query, QueryFormat, pCtx, pSink, this );

            if (FAILED(hr))
            {
                pMainFnz->CancelTask ( 0 );
            }

        }
        else
        {
            CAsyncReq_DynAux_ExecQueryAsync *pReq = 0;
            try
            {
                pReq = new CAsyncReq_DynAux_ExecQueryAsync (
                    this,
                    pClassDef,
                    Query,
                    QueryFormat,
                    lFlags,
                    pCtx,
                    pSink
                );
            }
            catch(CX_Exception &)
            {
                pReq = 0;
            }

            if (pReq == NULL)
            {
                pMainFnz->CancelTask ( 0 );
                return WBEM_E_OUT_OF_MEMORY;
            }

            if ( NULL == pReq->GetContext() )
            {
                pMainFnz->CancelTask ( 0 );
                delete pReq;
                return WBEM_E_OUT_OF_MEMORY;
            }

            hr = pReq->Initialize () ;
            if ( SUCCEEDED ( hr ) )
            {

                // Set the task for the request - we'll just use the existing one
                pCurrReq->m_phTask->AddRef();
                pReq->m_phTask = pCurrReq->m_phTask;
                
                hr = ConfigMgr::EnqueueRequest(pReq);

                if (FAILED(hr))
                {
                    pMainFnz->CancelTask ( 0 );
                    delete pReq;
                }

            }    // IF Request Initialized
            else
            {
                pMainFnz->CancelTask ( 0 );
                delete pReq;
            }

        }

    }

    return hr;
}

//***************************************************************************
//
//  CWbemNamespace::DynAux_ExecQueryAsync
//
//  Executes a SQL-1 query against a dynamic instance provider for the class
//  in the query.
//
//  PARAMETERS:
//
//      IN DWORD dwNamespace            Namespace handle to the current
//                                      namespace (see objdb.h)
//      IN CWbemObject* pClassDef        Class definition of the class in the
//                                      query. Must be dynamic.
//      IN LPWSTR Query                 The query string.
//      IN LPWSTR QueryFormat           The query language. Must be WQL.
//      IN long lFlags                  The flags. Not used.
//      OUT CFlexArray &aInstances      Destinatino for the instances found.
//      OUT IWbemClassObject** ppErrorObj Destination for the error object. IF
//                                      not NULL, an error object may be placed
//                                      here. In this case, it is the caller's
//                                      responsibility to release it.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR                  Success (even though there may not be
//                                      any instances).
//      WBEM_E_INVALID_PROVIDE_REGISTRATION  Provider registration for this
//                                          class is incomplete.
//      WBEM_E_PROVIDER_NOT_FOUND    Provider could not be located. It is not
//                                  registered with us or with COM.
//      WBEM_E_PROVIDER_NOT_CAPABLE  Provider is not capable of enumerating
//                                  instances.
//      WBEM_E_FAILED                Unexpected error has occured.
//
//***************************************************************************

HRESULT CWbemNamespace::Exec_DynAux_ExecQueryAsync (

    CWbemObject* pClassDef,
    LPWSTR Query,
    LPWSTR QueryFormat,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink
)
{

    COperationError OpInfo(pSink, L"ExecQuery", Query, FALSE);
    if (! OpInfo.IsOk())  return pSink->Return(WBEM_E_OUT_OF_MEMORY);

    // Get the provider name.
    // ======================

    CVar vProv;
    HRESULT hres = pClassDef->GetQualifier(L"Provider", &vProv);
    if (FAILED(hres) || vProv.GetType() != VT_BSTR)
        return OpInfo.ErrorOccurred(WBEM_E_INVALID_PROVIDER_REGISTRATION);

    // Access the provider cache.
    // ==========================

    IWbemServices *pProv = 0;
    HRESULT hRes;
    if(m_pProvFact == NULL)
        hRes = WBEM_E_CRITICAL_ERROR;
    else
    {
        WmiInternalContext t_InternalContext ;
        ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;  // SEC:REVIEWED 2002-03-22 : OK

        hRes = m_pProvFact->GetProvider(

            t_InternalContext ,
            0,                  // lFlags
            pCtx,
            0,
            m_wszUserName,
            m_wsLocale,
            0,                      // IWbemPath pointer
            vProv,     // Provider
            IID_IWbemServices,
            (LPVOID *) &pProv
            );
    }

    if (FAILED(hRes))
    {
        return pSink->Return(hRes);
    }

    CReleaseMe _1(pProv);

    // Set up the sink chain to be delivered to the provider.
    // The code & destruct sequence is critical and the
    // refcounting is very carefully thought out.  Do not
    // change this code unless you know exactly what you are
    // doing.  And don't even change it then.
    // ======================================================

    CProviderSink *pProvSink = new CProviderSink(1, Query);
    if (pProvSink == 0)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    CReleaseMe _3(pProvSink);

    CDecoratingSink * pDecore = new CDecoratingSink(pSink, this);
    if (pDecore == NULL)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    pProvSink->SetNextSink(pDecore);

    // Before calling the provider, map the provider to the
    // task so that we can cancel it proactively, if required.
    // =======================================================

    hRes = ((CWmiArbitrator *) m_pArb)->MapProviderToTask(0, pCtx, pProv, pProvSink);
    if (FAILED(hRes))
        return pSink->Return(hRes);


    // Now tell the provider to start enumerating.
    hRes = pProv->ExecQueryAsync(QueryFormat, 
                                                     Query, lFlags& ~WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                                     pCtx, pProvSink);

    return hRes;
}

//***************************************************************************
//
//  CWbemNamespace::DynAux_ExecQueryAsync
//
//  Executes a SQL-1 query against a dynamic instance provider for the class
//  in the query.
//
//  PARAMETERS:
//
//      IN DWORD dwNamespace            Namespace handle to the current
//                                      namespace (see objdb.h)
//      IN CWbemObject* pClassDef        Class definition of the class in the
//                                      query. Must be dynamic.
//      IN LPWSTR Query                 The query string.
//      IN LPWSTR QueryFormat           The query language. Must be WQL.
//      IN long lFlags                  The flags. Not used.
//      OUT CFlexArray &aInstances      Destinatino for the instances found.
//      OUT IWbemClassObject** ppErrorObj Destination for the error object. IF
//                                      not NULL, an error object may be placed
//                                      here. In this case, it is the caller's
//                                      responsibility to release it.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR                  Success (even though there may not be
//                                      any instances).
//      WBEM_E_INVALID_PROVIDE_REGISTRATION  Provider registration for this
//                                          class is incomplete.
//      WBEM_E_PROVIDER_NOT_FOUND    Provider could not be located. It is not
//                                  registered with us or with COM.
//      WBEM_E_PROVIDER_NOT_CAPABLE  Provider is not capable of enumerating
//                                  instances.
//      WBEM_E_FAILED                Unexpected error has occured.
//
//***************************************************************************

HRESULT CWbemNamespace::DynAux_ExecQueryExtendedAsync(

    LPWSTR wsProvider,
    LPWSTR Query,
    LPWSTR QueryFormat,
    long lFlags,
    IWbemContext* pCtx,
    CComplexProjectionSink* pSink
)
{
    COperationError OpInfo(pSink, L"ExecQuery", Query, FALSE);
    if ( !OpInfo.IsOk() )  return WBEM_E_OUT_OF_MEMORY;


    // Access the provider cache.
    // ==========================

    IWbemServices *pProv = 0;
    HRESULT hRes;
    if(m_pProvFact == NULL)
        hRes = WBEM_E_CRITICAL_ERROR;
    else
    {
        WmiInternalContext t_InternalContext ;
        ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;  // SEC:REVIEWED 2002-03-22 : OK

        hRes = m_pProvFact->GetProvider(

            t_InternalContext ,
            0,                  // lFlags
            pCtx,
            0,
            m_wszUserName,
            m_wsLocale,
            0,                      // IWbemPath pointer
            wsProvider,              // provider name
            IID_IWbemServices,
            (LPVOID *) &pProv
        );
    }

    if (FAILED(hRes))
    {
        return pSink->Return(hRes);
    }

    CReleaseMe _(pProv);

    _IWmiProviderConfiguration *t_Configuration = NULL ;
    hRes = pProv->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_Configuration ) ;
    if ( SUCCEEDED ( hRes ) )
    {
        CReleaseMe _1(t_Configuration);

        VARIANT t_Variant ;
        VariantInit ( & t_Variant ) ;

        hRes = t_Configuration->Query (

            this ,
            lFlags& ~WBEM_FLAG_USE_AMENDED_QUALIFIERS ,
            pCtx ,
            WBEM_PROVIDER_CONFIGURATION_CLASS_ID_INSTANCE_PROVIDER_REGISTRATION ,
            WBEM_PROVIDER_CONFIGURATION_PROPERTY_ID_EXTENDEDQUERY_SUPPORT ,
            & t_Variant
        ) ;

        if ( SUCCEEDED ( hRes ) )
        {
            if ( t_Variant.boolVal == VARIANT_TRUE )
            {
                CDecoratingSink * pDecore = new CDecoratingSink(pSink, this);
                if(pDecore == NULL)
                    return pSink->Return(WBEM_E_OUT_OF_MEMORY);
                pDecore->AddRef();
                CReleaseMe cdecor(pDecore);

                hRes = pProv->ExecQueryAsync(QueryFormat, Query, lFlags& ~WBEM_FLAG_USE_AMENDED_QUALIFIERS, pCtx, pDecore);
            }
            else
            {
                hRes = WBEM_E_INVALID_QUERY ;
            }

            VariantClear ( & t_Variant ) ;
        }
        else
        {
            hRes = WBEM_E_UNEXPECTED ;
        }
    }
    else
    {
        hRes = WBEM_E_UNEXPECTED ;
    }

    return hRes;
}

//***************************************************************************
//
//  CWbemNamespace::Static_QueryRepository
//
//  Performs query against the repository.  This only happens if there is an
//    associated task.  If not, then we execute the query on the same thread.
//
//  PARAMETERS:
//
//      READONLY CWbemObject *pClassDef  The definition of the class to retrieve
//                                      instances of.
//      long lFlags                     The flags (deep/shallow)
//        
//  RETURN VALUES:
//
//      WBEM_NO_ERROR  No errors. This includes a no-error situation
//                  with zero instances returned.
//      WBEM_E_INVALID_PROVIDE_REGISTRATION  Provider registration for this
//                                          class is incomplete.
//      WBEM_E_PROVIDER_NOT_FOUND    Provider could not be located. It is not
//                                  registered with us or with COM.
//      WBEM_E_PROVIDER_NOT_CAPABLE  Provider is not capable of enumerating
//                                  instances.
//      WBEM_E_FAILED                Unexpected error has occured.
//
//***************************************************************************

HRESULT CWbemNamespace::Static_QueryRepository(

    READONLY CWbemObject *pClassDef,
    long lFlags,
    IWbemContext* pCtx,
    CBasicObjectSink* pSink ,
    QL_LEVEL_1_RPN_EXPRESSION* pParsedQuery,
    LPCWSTR pwszClassName,
    CWmiMerger* pWmiMerger
)
{

    HRESULT    hr = WBEM_S_NO_ERROR;

    // First, get the current task and request.  If there isn't one, we're most
    // likely being called on an ESS callback, and will just perform the call on
    // this thread.

    CWbemRequest*        pCurrReq = CWbemQueue::GetCurrentRequest();

    if ( NULL != pCurrReq && NULL != pCurrReq->m_phTask && 
    	 ConfigMgr::GetEnableQueryArbitration() )
    {
        // We'll need the finalizer in case we need to cancel something
        _IWmiFinalizer*    pMainFnz = NULL;
        
        HRESULT    hr = ((CWmiTask*) pCurrReq->m_phTask)->GetFinalizer( &pMainFnz );
        CReleaseMe    rm( pMainFnz );

        if ( SUCCEEDED( hr ) )
        {
            //
            // creates the CMergerDynReq_Static_GetInstances
            // creates the Merger Request Manager 
            // adds the request to the Request Manager
            //
            hr = pWmiMerger->RegisterArbitratedStaticRequest( pClassDef, lFlags, pCtx, pSink, this, pParsedQuery );

            if (FAILED(hr))
            {
                pMainFnz->CancelTask ( 0 );
            }

        }    // IF Got Finalizer

        // In the case of an error we should do a setstatus of the error.  Otherwise, the setstatus will occur
        // when the new request is processed.
        if ( FAILED( hr ) )
        {
            pSink->SetStatus( 0L, hr, 0L, NULL );
        }
    }
    else
    {
        // If we're here, then we should disallow merger specific throttling since
        // this request is happening through an internal thread without following
        // the request/task hierarchy, meaning that there shouldn't be a hierarchy
        // we need to worry about for this class, so don't let the merger do any
        // internal throttling

        pWmiMerger->EnableMergerThrottling( false );

        int nRes = CQueryEngine::ExecAtomicDbQuery(
                    GetNsSession(),
                    GetNsHandle(),
                    GetScope(),
                    pwszClassName,
                    pParsedQuery,
                    pSink,
                    this );

        if (nRes == CQueryEngine::invalid_query)
            hr = WBEM_E_INVALID_QUERY;
        else if(nRes != 0)
            hr = WBEM_E_FAILED;
        else
            hr = WBEM_S_NO_ERROR;

        pSink->SetStatus( 0L, hr, 0L, NULL );

    }

    return hr;
}

//***************************************************************************
//
//  CWbemNamespace::DecorateObject
//
//  Sets the origin information on a given object to reflect this namespace
//  and this server. See CWbemObject::Decorate in fastobj.h for details.
//  THIS FUNCTION CAN ONLY DECORATE CWbemObject POINTERS, NOT OTHER PEOPLE'S
//  IMPLEMENTATIONS of IWbemClassObject.
//
//  PARAMETERS:
//
//      IWbemClassObject* pObject        The object to decorate.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              Success
//      WBEM_E_INVALID_PARAMETER     pObject == NULL.
//
//***************************************************************************

HRESULT CWbemNamespace::DecorateObject(IWbemClassObject* pObject)
{
    if(pObject == NULL)
        return WBEM_E_INVALID_PARAMETER;

    return ((CWbemObject*)pObject)->Decorate(ConfigMgr::GetMachineName(),
                                          m_pThisNamespace);
}

//***************************************************************************
//
//***************************************************************************

typedef std::vector<CDynasty*, wbem_allocator<CDynasty*> > CDynastyPtrArray;
typedef std::map<LPWSTR, CDynasty*, wcsiless, wbem_allocator<CDynasty*> > CCDynastyMap;

HRESULT AddAllMembers(CDynasty* pDynasty, CCDynastyMap& Map)
{
    // Catch any exceptions the allocator might throw
    try
    {
        Map[pDynasty->m_wszClassName] = pDynasty;
    }
    catch(CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    HRESULT hr = WBEM_S_NO_ERROR;

    CFlexArray* pChildren = &pDynasty->m_Children;
    for(int i = 0; SUCCEEDED(hr) && i < pChildren->Size(); i++)
    {
        hr = AddAllMembers((CDynasty*)pChildren->GetAt(i), Map);
    }
    return hr;
}


//***************************************************************************
//
//  CWbemNamespace::DynAux_BuildClassHierarchy
//
//  Recursively builds the hierarchy of classes derived from a given one.
//  The structure used to represent the hierarchy -- CDynasty is described
//  in objdb.h
//
//  PARAMETERS:
//
//      IN LPWSTR wszClassName      The name of the parent class.
//      IN LONG lFlags              If SHALLOW, just the class itself is
//                                  returned. If DEEP, recursive enumeration
//                                  is performed.
//      OUT CDynasty** ppDynasty  Destination for the tree. The caller must
//                                  delete the pointer on success.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              Success
//      Any of the return values returned by GetObject or CreateClassEnum,
//      as documented in the help file.
//
//***************************************************************************

HRESULT CWbemNamespace::DynAux_BuildClassHierarchy(
                                              IN LPWSTR wszClassName,
                                              IN LONG lFlags,
                                              IN IWbemContext* pCtx,
                                              OUT wmilib::auto_ptr<CDynasty> & pDynasty,
                                              OUT IWbemClassObject** ppErrorObj)
{
    HRESULT hres;
    *ppErrorObj = NULL;

    // Get the list of classes from all class providers
    // ================================================

    CSynchronousSink* pSyncSink = CSynchronousSink::Create();
    if(pSyncSink == NULL) return WBEM_E_OUT_OF_MEMORY;
    pSyncSink->AddRef();
    CReleaseMe rm1(pSyncSink);

    hres = Exec_CreateClassEnum(wszClassName,
                                                  lFlags | WBEM_FLAG_NO_STATIC, 
                                                  pCtx,
                                                  pSyncSink);
    pSyncSink->Block();
    pSyncSink->GetStatus(&hres, NULL, ppErrorObj);

    if(FAILED(hres))
        return hres;

    // Get the static dynasty
    // ======================

    wmilib::auto_ptr<CDynasty> pMainDynasty;

    HRESULT hRes = CRepository::BuildClassHierarchy(m_pSession,
                                                                                m_pNsHandle,
                                                                                wszClassName,
                                                                                lFlags & WBEM_MASK_DEPTH,
                                                                                pMainDynasty);

    if (hRes == WBEM_E_NOT_FOUND)
    {
        IWbemClassObject* pObj;
        HRESULT hres = Exec_GetObjectByPath(wszClassName, lFlags, pCtx,
                                                                  &pObj, 
                                                                  ppErrorObj);
        if(FAILED(hres)) return hres;
        CReleaseMe rmClassObj(pObj);
        
        pMainDynasty.reset(CDynasty::Create(pObj));
        if(NULL == pMainDynasty.get())  return WBEM_E_OUT_OF_MEMORY;
        if(pMainDynasty->m_pClassObj == NULL)
        {
            ERRORTRACE((LOG_WBEMCORE, "Provider returned invalid class for %S\n",wszClassName));
            return WBEM_E_PROVIDER_FAILURE;
        }
        hRes = S_OK;
    }
    if (FAILED(hRes)) return hRes;
    

    CRefedPointerArray<IWbemClassObject> &rProvidedClasses = pSyncSink->GetObjects();

    // Create a map of class names to their dynasties
    // ==============================================

    CCDynastyMap mapClasses;
    hres = AddAllMembers(pMainDynasty.get(), mapClasses);
    if ( FAILED(hres)) return hres;

    CDynastyPtrArray aProvidedDyns;

    try
    {
        aProvidedDyns.reserve(rProvidedClasses.GetSize());
    }
    catch(CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    for(int i = 0; i < rProvidedClasses.GetSize(); i++)
    {
        CDynasty* pNew = CDynasty::Create(rProvidedClasses[i]);
        if(pNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        if(pNew->m_pClassObj == NULL)
        {
            delete pNew;
            ERRORTRACE((LOG_WBEMCORE, "Provider returned invalid class!\n"));
            continue;
        }

        // The vector or the map may throw exceptions
        try
        {
            mapClasses[pNew->m_wszClassName] = pNew;
            aProvidedDyns.push_back(pNew);
        }
        catch(CX_MemoryException &)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

    }

    // Go through it once and add all classes to their parent's dynasty
    // ================================================================

    for(CDynastyPtrArray::iterator it = aProvidedDyns.begin();
        it != aProvidedDyns.end(); it++)
    {
        CDynasty* pDyn = *it;
        CVar vParent;
        CWbemObject *pObj = (CWbemObject *) pDyn->m_pClassObj;

        if(FAILED(pObj->GetSuperclassName(&vParent)) ||
                            vParent.IsNull())
        {
            ERRORTRACE((LOG_WBEMCORE,"Provider returned top-level class %S as a child "
                "of %S\n", pDyn->m_wszClassName, wszClassName));
            continue;
        }

        CCDynastyMap::iterator itParent =
            mapClasses.find(vParent.GetLPWSTR());

        if((itParent == mapClasses.end()))
        {
            if(wbem_wcsicmp(pDyn->m_wszClassName, wszClassName))
            {
                ERRORTRACE((LOG_WBEMCORE,"Provider returned class %S without parent!\n",
                    vParent.GetLPWSTR()));
            }
            continue;
        }
        CDynasty* pParentDyn = itParent->second;
        pParentDyn->AddChild(pDyn);
    }

    // Build the chain up to the highest keyed parent
    // ==============================================
    pDynasty = pMainDynasty;
    hres = DynAux_BuildChainUp( pCtx, pDynasty, ppErrorObj);

    return hres;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::DynAux_BuildChainUp( IN IWbemContext* pCtx,
                                                                              OUT wmilib::auto_ptr<CDynasty> & pTop,
                                                                              OUT IWbemClassObject** ppErrorObj)
{
    *ppErrorObj = NULL;

    // Go up while there is a key at this level and we are dynamic
    // ===========================================================

    while( pTop->IsDynamic() && pTop->IsKeyed())
    {
        CVar vParentName;
        CWbemObject *pObj = (CWbemObject *) pTop->m_pClassObj;

        if(FAILED(pObj->GetSuperclassName(&vParentName)) ||
                vParentName.IsNull())
        {
            // Top level --- time to quit
            // ==========================
            return WBEM_S_NO_ERROR;
        }

        IWbemClassObject* pParent;
        HRESULT hres = Exec_GetObjectByPath(vParentName.GetLPWSTR(), 0,pCtx,&pParent,  ppErrorObj);
        if(FAILED(hres))
            return hres;

        if(pParent == NULL)
            return WBEM_E_PROVIDER_FAILURE;

        // SJS - Amendment is the same as Abstract
        if(!((CWbemClass*)pParent)->IsKeyed() ||
            ((CWbemClass*)pParent)->IsAbstract() ||
            ((CWbemClass*)pParent)->IsAmendment() )
        {
            // We are it
            // =========

            pParent->Release();
            return WBEM_S_NO_ERROR;
        }

        // Extend the dynasty by this class
        // ================================

        wmilib::auto_ptr<CDynasty> pNew( CDynasty::Create(pParent));
        if (NULL == pNew.get()) return WBEM_E_OUT_OF_MEMORY;
        pParent->Release();
        if(pNew->m_pClassObj == NULL)
        {
            return WBEM_E_PROVIDER_FAILURE;
        }
        pNew->AddChild(pTop.get());
        pTop.release();
        pTop.reset(pNew.release());
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::IsPutRequiredForClass(CWbemClass* pClass,
                            CWbemInstance* pInst, IWbemContext* pCtx,
                            BOOL bParentTakenCareOf)
{
    HRESULT hres;

    // Get the per-property put information out of the context
    // =======================================================

    BOOL bRestrictedPut = FALSE;
    BOOL bStrictNulls = FALSE;
    BOOL bPropertyList = FALSE;
    CWStringArray awsProperties;

    hres = GetContextPutExtensions(pCtx, bRestrictedPut, bStrictNulls,
                                bPropertyList, awsProperties);
    if(FAILED(hres))
        return hres;

    if(bRestrictedPut && bStrictNulls && !bPropertyList)
    {
        // All properties must be put, even the NULL ones
        // ==============================================

        return WBEM_S_NO_ERROR;
    }

    // Enumerate all properties of the class
    // =====================================

    long lEnumFlags = 0;
    if(bParentTakenCareOf)
    {
        // Only look at local (non-propagated) properties
        // ==============================================
        lEnumFlags = WBEM_FLAG_LOCAL_ONLY;
    }
    else
    {
        // We are in charge of our parent's properties
        // ===========================================
        lEnumFlags = WBEM_FLAG_NONSYSTEM_ONLY;
    }

    pClass->BeginEnumeration(lEnumFlags);
    BSTR strName = NULL;
    while((hres = pClass->Next(0, &strName, NULL, NULL, NULL)) == S_OK)
    {
        hres = DoesNeedToBePut(strName, pInst, bRestrictedPut,
                            bStrictNulls, bPropertyList, awsProperties);
        SysFreeString(strName);
        if(hres == WBEM_S_NO_ERROR)
        {
            // Found a needed property
            // =======================

            return WBEM_S_NO_ERROR;
        }
        if(FAILED(hres))
            return hres;
    }

    // No properties of this class need to be put
    // ==========================================

    return WBEM_S_FALSE;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::DoesNeedToBePut(LPCWSTR wszName, CWbemInstance* pInst,
            BOOL bRestrictedPut, BOOL bStrictNulls, BOOL bPropertyList,
            CWStringArray& awsProperties)
{
    HRESULT hres;

    // Check if the property is a key
    // ==============================

    CVar vKey;
    pInst->GetPropQualifier((LPWSTR)wszName, L"key", &vKey);
    if(vKey.GetType() == VT_BOOL && vKey.GetBool())
    {
        // It's a key --- no such thing as updating its value, and this code
        // only applies to updates.
        // =================================================================

        return WBEM_S_FALSE;
    }

    // Determine if NULLness and or membership in the list play any role
    // =================================================================

    BOOL bCheckNullness = FALSE;
    BOOL bCheckMembership = FALSE;

    if(bRestrictedPut)
    {
        bCheckNullness = !bStrictNulls;
        bCheckMembership = bPropertyList;
    }
    else
    {
        bCheckNullness = TRUE;
        bCheckMembership = FALSE;
    }

    // Check NULLness and/or membership if required
    // ============================================

    BOOL bNullnessChecked = FALSE;
    BOOL bMembershipChecked = FALSE;

    if(bCheckNullness)
    {
        CVar vVal;
        hres = pInst->GetNonsystemPropertyValue((LPWSTR)wszName, &vVal);
        if(FAILED(hres))
            return hres;

        bNullnessChecked = !vVal.IsNull();
    }
    else
        bNullnessChecked = TRUE;

    if(bCheckMembership)
    {
        int nIndex = awsProperties.FindStr(wszName, CWStringArray::no_case);
        bMembershipChecked = (nIndex >= 0);
    }
    else
        bMembershipChecked = TRUE;

    // Make sure that both NULLness and membership either checked out or were
    // not required
    // ======================================================================

    if(bMembershipChecked && bNullnessChecked)
    {
        return WBEM_S_NO_ERROR;
    }
    else
    {
        return WBEM_S_FALSE;
    }
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::GetContextPutExtensions(IWbemContext* pCtx,
            BOOL& bRestrictedPut, BOOL& bStrictNulls, BOOL& bPropertyList,
            CWStringArray& awsProperties)
{
    HRESULT hres;

    if(pCtx == NULL)
    {
        //
        // Default is: no restructions, which makes the rest of the properties
        // irrelevant
        //

        bRestrictedPut = FALSE;
        return WBEM_S_NO_ERROR;
    }

    // Initialize out-params
    // =====================

    bRestrictedPut = FALSE;
    bStrictNulls = FALSE;
    bPropertyList = FALSE;
    awsProperties.Empty();

    // Check if the context is even present
    // ====================================

    if(pCtx == NULL)
        return WBEM_S_NO_ERROR;

    // Check if put extensions are specified
    // =====================================

    hres = GetContextBoolean(pCtx, L"__PUT_EXTENSIONS", &bRestrictedPut);
    if(FAILED(hres)) return hres;

    if(!bRestrictedPut)
        return WBEM_S_NO_ERROR;

    // Check if NULLs are strict
    // =========================

    hres = GetContextBoolean(pCtx, L"__PUT_EXT_STRICT_NULLS",
                &bStrictNulls);
    if(FAILED(hres)) return hres;

    // Check if the list of properties is available
    // ============================================

    VARIANT v;
    VariantInit(&v);
    CClearMe cm1(&v);

    hres = pCtx->GetValue(L"__PUT_EXT_PROPERTIES", 0, &v);
    if(FAILED(hres))
    {
        if(hres == WBEM_E_NOT_FOUND)
        {
            return WBEM_S_NO_ERROR;
        }
        else
        {
            ERRORTRACE((LOG_WBEMCORE, "Error retrieving list of properties "
                        "from context: %X\n", hres));
            return hres;
        }
    }

    if(V_VT(&v) != (VT_BSTR | VT_ARRAY))
    {
        ERRORTRACE((LOG_WBEMCORE, "Invalid type is used for "
            "the list of properties in the context: must be "
            "string array.  The value will be ignored\n"));
        return WBEM_S_NO_ERROR;
    }

    bPropertyList = TRUE;

    // Transfer property names to the array
    // ====================================

    CSafeArray saProperties(V_ARRAY(&v), VT_BSTR,
                    CSafeArray::no_delete | CSafeArray::bind);

    for(int i = 0; i < saProperties.Size(); i++)
    {
        BSTR strProp = saProperties.GetBSTRAt(i);
        CSysFreeMe sfm(strProp);
        if (strProp)
        {
            if (CFlexArray::no_error != awsProperties.Add(strProp))
                return WBEM_E_OUT_OF_MEMORY;
        }  
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::GetContextBoolean(IWbemContext* pCtx,
                LPCWSTR wszName, BOOL* pbValue)
{
    HRESULT hres;
    *pbValue = FALSE;

    //
    // NULL context means "FALSE"
    //

    if(pCtx == NULL)
        return WBEM_S_NO_ERROR;

    VARIANT v;
    VariantInit(&v);
    CClearMe cm1(&v);

    hres = pCtx->GetValue((LPWSTR)wszName, 0, &v);
    if(FAILED(hres))
    {
        if(hres == WBEM_E_NOT_FOUND)
        {
            return WBEM_S_NO_ERROR;
        }
        else
        {
            ERRORTRACE((LOG_WBEMCORE, "Error retrieving context property %S:"
                        " %X\n", wszName, hres));
            return hres;
        }
    }

    if(V_VT(&v) != VT_BOOL)
    {
        ERRORTRACE((LOG_WBEMCORE, "Invalid type is used for "
            "%S property of the context: must be "
            "boolean.  The value will be ignored\n", wszName));
        return WBEM_S_NO_ERROR;
    }

    if(V_BOOL(&v) != VARIANT_TRUE)
    {
        return WBEM_S_NO_ERROR;
    }

    *pbValue = TRUE;
    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::FindKeyRoot(LPCWSTR wszClassName,
                                IWbemClassObject** ppKeyRootClass)
{
    //
    // Check if the namespace is still valid (returns if not)
    //
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    //
    // Call on the database to do the job
    //

    hRes = CRepository::FindKeyRoot(m_pSession, m_pNsHandle, wszClassName, ppKeyRootClass);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::GetNormalizedPath( BSTR pstrPath,
                                                BSTR* pstrStandardPath )
{
    //
    // Check if the namespace is still valid (returns if not)
    //
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    //
    // check parameters.
    //

    if ( NULL == pstrPath || NULL == pstrStandardPath )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hres;
    *pstrStandardPath = NULL;

    // Parse it
    // ========

    CObjectPathParser Parser;
    ParsedObjectPath* pPath;

    int nRes = Parser.Parse(pstrPath, &pPath);

    if( nRes != CObjectPathParser::NoError )
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    CDeleteMe<ParsedObjectPath> dm(pPath);

    //
    // Figure out the class that defined the key
    //

    IWbemClassObject* pKeyRootClass = NULL;

    hres = FindKeyRoot(pPath->m_pClass, &pKeyRootClass);

    if(FAILED(hres))
    {
        return hres;
    }

    CReleaseMe rm(pKeyRootClass);

    _IWmiObject* pWmiObject = NULL;
    hres = pKeyRootClass->QueryInterface(IID__IWmiObject, (void**)&pWmiObject);

    if(FAILED(hres))
    {
        return hres;
    }

    CReleaseMe rm1(pWmiObject);

    long    lHandle = 0L;
    ULONG   uFlags = 0L;
    WCHAR   wszClassName[64];
    DWORD   dwBuffSize = 64;
    DWORD   dwBuffUsed = 0;
    BOOL    fNull = FALSE;
    LPWSTR  pwszName = wszClassName;
    LPWSTR  pwszDelete = NULL;

    // Try to read in the class name.  Allocate a buffer if we have to.
    hres = pWmiObject->ReadProp( L"__CLASS", 0L, dwBuffSize, NULL, NULL, &fNull, &dwBuffUsed, pwszName );

    if ( FAILED( hres ) )
    {
        if ( WBEM_E_BUFFER_TOO_SMALL == hres )
        {
            pwszName = new WCHAR[dwBuffUsed/2];

            if ( NULL == pwszName )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                dwBuffSize = dwBuffUsed;

                // Try to read in the class name.  Allocate a buffer if we have to.
                hres = pWmiObject->ReadProp( L"__CLASS", 0L, dwBuffSize, NULL, NULL, &fNull, &dwBuffUsed, pwszName );

                if ( FAILED( hres ) )
                {
                    delete [] pwszName;
                    return hres;
                }

                // Allows for scoped cleanup only if we alloctaed something
                pwszDelete = pwszName;
            }
        }
        else
        {
            return hres;
        }
    }

    //
    // Ensures proper cleanup.  If we didn't allocate a buffer to delete,
    // this pointer will be NULL.
    //
    CVectorDeleteMe<WCHAR> vdm1(pwszDelete);

    // oop
    if ( fNull )
    {
        return WBEM_E_INVALID_OPERATION;
    }

    //
    // want to normalize out the single key prop exception
    //

    if ( pPath->m_dwNumKeys == 1 )
    {
        delete pPath->m_paKeys[0]->m_pName;
        pPath->m_paKeys[0]->m_pName = NULL;
    }

    //
    // set the normalized class on the path if different
    // than the one in the path.
    //

    if ( wbem_wcsicmp( pPath->m_pClass, pwszName ) != 0 )
    {
        if ( !pPath->SetClassName( pwszName ) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    //
    // now unparse the normalized path
    //

    LPWSTR wszNormPath;

    nRes = CObjectPathParser::Unparse( pPath, &wszNormPath );

    if ( nRes != CObjectPathParser::NoError )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    *pstrStandardPath = SysAllocString( wszNormPath );

    delete wszNormPath;

    return *pstrStandardPath != NULL ? WBEM_S_NO_ERROR : WBEM_E_OUT_OF_MEMORY;
}

class CRevertCallSec
{
    BOOL m_bEnabled;
    IUnknown * m_pCallSec;
public:
    CRevertCallSec(BOOL bEnabled,IUnknown * pCallSec):
        m_bEnabled(bEnabled),
        m_pCallSec(pCallSec){};
    ~CRevertCallSec(){
        if (m_bEnabled){
            IUnknown * pOld = NULL;
            CoSwitchCallContext(m_pCallSec,&pOld); 
        }    
    };
};

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::InternalGetClass(
            LPCWSTR wszClassName,
            IWbemClassObject** ppClass)
{
    if (g_bDontAllowNewConnections || m_bShutDown)
    {
        return WBEM_E_SHUTTING_DOWN;
    }

    IWbemContext *pContext = NULL ;

    CWbemRequest* pReq = CWbemQueue::GetCurrentRequest() ;
    if (pReq == NULL)
    {
        pContext = ConfigMgr::GetNewContext();
        if ( pContext == NULL )
            return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        pContext = pReq->GetContext();
        pContext->AddRef () ;   //for CReleaseMe
    }

    CReleaseMe _1_pContext (pContext) ;

    HRESULT hr;

    // determine if there is a call context.  If there is, then we
    // dont do anything.

    IServerSecurity * pSec = NULL;
    IWbemCallSecurity * pCallSec = NULL;
    IUnknown * pOld = NULL, *pNew = NULL;
    hr = CoGetCallContext(IID_IServerSecurity, (void**)&pSec);
    if(SUCCEEDED(hr))
    {
        pSec->Release();
    }
    else
    {
        // provider subsystem needs a call context, so create on

        pCallSec = CWbemCallSecurity::CreateInst();
        if(pCallSec == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }
    CReleaseMe rm(pCallSec);

    // initialize call security for provider sub system

    if(pCallSec)
    {
        hr = pCallSec->CloneThreadContext(TRUE);
        if(FAILED(hr))
            return hr;
        hr = CoSwitchCallContext(pCallSec, &pOld);
        if(FAILED(hr))
            return hr;
    }
    CRevertCallSec Revert(pCallSec?TRUE:FALSE,pOld);   // SEC:REVIEWED 2002-03-22 : Assumes success; why is this here? It is the only occurrence.

    hr = Exec_GetObjectByPath((LPWSTR)wszClassName, 0, pContext, ppClass, NULL);

    return hr;
}


//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::InternalGetInstance(
            LPCWSTR wszPath,
            IWbemClassObject** ppInstance)
{
    if (g_bDontAllowNewConnections || m_bShutDown)
    {
        return WBEM_E_SHUTTING_DOWN;
    }

    IWbemContext *pContext = NULL ;

    CWbemRequest* pReq = CWbemQueue::GetCurrentRequest() ;
    if (pReq == NULL)
    {
        pContext = ConfigMgr::GetNewContext();
        if ( pContext == NULL )
            return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        pContext = pReq->GetContext();
        pContext->AddRef () ;   //for CReleaseMe
    }

    CReleaseMe _1_pContext (pContext) ;

    return Exec_GetObjectByPath((LPWSTR)wszPath, 0, pContext, ppInstance, NULL);
}


//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::InternalExecQuery(
             LPCWSTR wszQueryLanguage,
             LPCWSTR wszQuery,
             long lFlags,
             IWbemObjectSink* pSink)
{
    if (g_bDontAllowNewConnections || m_bShutDown)
    {
        return WBEM_E_SHUTTING_DOWN;
    }

    IWbemContext *pContext = NULL ;

    CWbemRequest* pReq = CWbemQueue::GetCurrentRequest() ;
    if (pReq == NULL)
    {
        pContext = ConfigMgr::GetNewContext();
        if ( pContext == NULL )
            return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        pContext = pReq->GetContext();
        pContext->AddRef () ;   //for CReleaseMe
    }

    CReleaseMe _1_pContext (pContext) ;

    CSimpleWrapperSink ws(pSink);
    return CQueryEngine::ExecQuery(this, (LPWSTR)wszQueryLanguage,
                (LPWSTR)wszQuery, lFlags, pContext, &ws);
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::InternalCreateInstanceEnum(
             LPCWSTR wszClassName,
             long lFlags,
             IWbemObjectSink* pSink)
{
    if (g_bDontAllowNewConnections || m_bShutDown)
    {
        return WBEM_E_SHUTTING_DOWN;
    }

    IWbemContext *pContext = NULL ;

    CWbemRequest* pReq = CWbemQueue::GetCurrentRequest() ;
    if (pReq == NULL)
    {
        pContext = ConfigMgr::GetNewContext();
        if ( pContext == NULL )
            return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        pContext = pReq->GetContext();
        pContext->AddRef () ;   //for CReleaseMe
    }

    CReleaseMe _1_pContext (pContext) ;

    CSimpleWrapperSink ws(pSink);
    return Exec_CreateInstanceEnum((LPWSTR)wszClassName, lFlags, pContext, &ws);
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::InternalPutInstance(
             IWbemClassObject* pInst)
{
    if (g_bDontAllowNewConnections || m_bShutDown)
    {
        return WBEM_E_SHUTTING_DOWN;
    }

    IWbemContext *pContext = NULL ;

    CWbemRequest* pReq = CWbemQueue::GetCurrentRequest() ;
    if (pReq == NULL)
    {
        pContext = ConfigMgr::GetNewContext();
        if ( pContext == NULL )
            return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        pContext = pReq->GetContext();
        pContext->AddRef () ;   //for CReleaseMe
    }

    CReleaseMe _1_pContext (pContext) ;

    CSynchronousSink* pSink = CSynchronousSink::Create();
    if(pSink == NULL) return WBEM_E_OUT_OF_MEMORY;
    pSink->AddRef();
    CReleaseMe rm1(pSink);

    Exec_PutInstance(pInst, 0, pContext, pSink);

    HRESULT hres = WBEM_E_CRITICAL_ERROR;
    pSink->GetStatus(&hres, NULL, NULL);
    return hres;
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::GetDbInstance(
             LPCWSTR wszDbKey,
             IWbemClassObject** ppInstance
             )
{
    // ESS uses this one
    return CRepository::GetObject(m_pSession, m_pScopeHandle, wszDbKey, 0, ppInstance);
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CWbemNamespace::GetDbReferences(
             IWbemClassObject* pEndpoint,
             IWbemObjectSink* pSink)
{
    LPWSTR wszRelPath = ((CWbemObject*)pEndpoint)->GetRelPath();
    CVectorDeleteMe<WCHAR> dm(wszRelPath);

    CSimpleWrapperSink ws(pSink);

    HRESULT hRes = CRepository::GetInstanceRefs(m_pSession,m_pScopeHandle, wszRelPath, &ws);

    return hRes;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::InternalPutStaticClass(
             IWbemClassObject* pClass)
{
    if (g_bDontAllowNewConnections || m_bShutDown)
    {
        return WBEM_E_SHUTTING_DOWN;
    }

    IWbemContext *pContext = NULL ;

    CWbemRequest* pReq = CWbemQueue::GetCurrentRequest() ;
    if (pReq == NULL)
    {
        pContext = ConfigMgr::GetNewContext();
        if ( pContext == NULL )
            return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        pContext = pReq->GetContext();
        pContext->AddRef () ;   //for CReleaseMe
    }

    CReleaseMe _1_pContext (pContext) ;

    CSynchronousSink* pSink = CSynchronousSink::Create();
    if(pSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pSink->AddRef();
    CReleaseMe rm1(pSink);

    Exec_PutClass( pClass, 0, pContext, pSink, TRUE );

    HRESULT hres = WBEM_E_CRITICAL_ERROR;
    pSink->GetStatus(&hres, NULL, NULL);
    return hres;
}


//***************************************************************************
//
//  CWbemNamespace::AdjustPutContext
//
//***************************************************************************

HRESULT CWbemNamespace::AdjustPutContext(
    IWbemContext *pCtx
    )
{
    // See if per-property puts are being used.
    // ========================================

    HRESULT hRes;

    if (pCtx == 0)
        return WBEM_S_NO_ERROR;

    CVARIANT v;
    hRes = pCtx->GetValue(L"__PUT_EXTENSIONS", 0, &v);

    if (SUCCEEDED(hRes))
    {
        // If here, they are being used.  Next we have to check and see
        // if the reentrancy flag is set or not.
        // =============================================================

        hRes = pCtx->GetValue(L"__PUT_EXT_CLIENT_REQUEST", 0, &v);
        if (SUCCEEDED(hRes))
        {
            pCtx->DeleteValue(L"__PUT_EXT_CLIENT_REQUEST", 0);
            return WBEM_S_NO_ERROR;
        }

        // If here, we have to clear out the put extensions.
        // =================================================
        pCtx->DeleteValue(L"__PUT_EXTENSIONS", 0);
        pCtx->DeleteValue(L"__PUT_EXT_CLIENT_REQUEST", 0);
        pCtx->DeleteValue(L"__PUT_EXT_ATOMIC", 0);
        pCtx->DeleteValue(L"__PUT_EXT_PROPERTIES", 0);
        pCtx->DeleteValue(L"__PUT_EXT_STRICT_NULLS", 0);
    }

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//  CWbemNamespace::MergeGetKeysCtx
//
//***************************************************************************
//
HRESULT CWbemNamespace::MergeGetKeysCtx(
    IN IWbemContext *pCtx
    )
{
    HRESULT hRes;
    if (pCtx == 0)
        return WBEM_S_NO_ERROR;
    CVARIANT v;
    v.SetBool(TRUE);
    hRes = pCtx->SetValue(L"__GET_EXTENSIONS", 0, &v);
    hRes |= pCtx->SetValue(L"__GET_EXT_KEYS_ONLY", 0, &v);
    hRes |= pCtx->SetValue(L"__GET_EXT_CLIENT_REQUEST", 0, &v);
    return hRes;
}






//***************************************************************************
//
//  CWbemNamespace::CheckNs
//
//  Does a quick check on the available system resources before allowing
//  a new call to proceed.  Retries for 30 seconds.
//
//***************************************************************************
//
HRESULT CWbemNamespace::CheckNs()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (g_bDontAllowNewConnections || m_bShutDown)
    {
        return WBEM_E_SHUTTING_DOWN;
    }

    // Quick memory check. If we are strapped for RAM/Pagefile,
    // wait a while and try again. Requirements are 1 meg of
    // available RAM and 1 meg of available page file.

    int nRetries = 0;

    for (int i = 0; i < 30; i++)
    {
        MEMORYSTATUS ms;
        GlobalMemoryStatus(&ms);

        if (ms.dwMemoryLoad < 99)
            return WBEM_S_NO_ERROR;

        // If here, we have to be careful.  The system is loaded at 99%.
        // =============================================================
        if (nRetries > 30)
        {
            // Sixty seconds waiting for enough memory. Give up.
            return WBEM_E_OUT_OF_MEMORY;
        }

        DWORD dwPracticalMemory = ms.dwAvailPhys + ms.dwAvailPageFile;

        if (dwPracticalMemory < 0x200000)   // 2 meg
        {
            Sleep(2000);
            // Try a 1 meg allocation to see if will succeed.
            LPVOID pTestMem = HeapAlloc(GetProcessHeap(), 0, 0x100000);
            if (pTestMem == 0)
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
            // Free the memory.  The previous allocation may have
            // grown the pagefile and thus we can succeed.
            HeapFree(GetProcessHeap(), 0, pTestMem);
            nRetries++;
        }
        else
        {
            // If here, we have a load of 99%, yet more than 2 meg of memory
            // still available. Now 99% may mean there is a lot of free memory
            // because the machine has huge resources or it may mean we are just
            // about out of memory completely.  We need hard data. If we
            // have at least 5 meg anyway, this is clearly adequate, so we just
            // break out of the loop and let the call continue.
            //
            // Otherwise, we have between 2 and 5 meg, which is starting to push
            // it. We enter a waiting loop and hope for more memory.  After a few
            // retries if we continue to have between 2 and 5 meg, we'll let the call
            // through and let the arbitrator deal with it, since the system appears
            // to have stabilized at this usage.
            //
            hr = WBEM_S_NO_ERROR;

            if (ms.dwAvailPhys < 0x200000)  // If low on physical memory back off a bit for recovery via pagefile
                Sleep(1000);

            if (dwPracticalMemory > 0x100000 * 5)  // > 5 meg; break out immediately
                break;

            // Under 5 meg free, retry a few times to let things clear up and get
            // more memory.  But, we succeed in the end anyway.
            // ==================================================================
            Sleep(1000);
            if (nRetries++ > 5)
            {
                hr = WBEM_S_NO_ERROR;
                break;
            }
        }
    }

    return hr;
}


//***************************************************************************
//
//***************************************************************************
//

HRESULT CWbemNamespace::UniversalConnect(
    CWbemNamespace  *pParent,
    IWbemContext    *pCtx,
    LPCWSTR pszNewScope,
    LPCWSTR pszAssocSelector,
    LPCWSTR pszUserName,
    _IWmiCallSec    *pCallSec,
    _IWmiUserHandle *pUser,
    DWORD  dwUserFlags,
    DWORD  dwInternalFlags,
    DWORD  dwSecFlags,
    DWORD  dwPermission,
    BOOL   bForClient,
    BOOL   bRepositOnly,
    LPCWSTR pszClientMachineName,
    DWORD dwClientProcessID,
    IN  REFIID riid,
    OUT LPVOID *pConnection
    )
{
    HRESULT hRes;
    if(dwUserFlags & WBEM_FLAG_CONNECT_REPOSITORY_ONLY)
        bRepositOnly = TRUE;
    
    // Validate.
    if (pszNewScope == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (riid != IID_IWbemServices )
        return E_NOINTERFACE;

    // If no parent, then this is an 'absolute' connect.
    // =================================================

    if (!pParent)
    {
        CWbemNamespace *pNs = CWbemNamespace::CreateInstance();
        if (NULL == pNs) return WBEM_E_OUT_OF_MEMORY;

        hRes = pNs->Initialize(
                        LPWSTR(pszNewScope),
                        LPWSTR(pszUserName),
                        dwSecFlags,
                        dwPermission,
                        bForClient,
                        bRepositOnly,
                        pszClientMachineName,
                        dwClientProcessID,
                        FALSE,
                        NULL);

        if (FAILED(hRes))
        {
            pNs->Release();
            return hRes;
        }

        pNs->SetIsProvider((dwUserFlags & WBEM_FLAG_CONNECT_PROVIDERS)?TRUE:FALSE);

        *pConnection = pNs;
        return WBEM_S_NO_ERROR;
    }
    else
    {
        return WBEM_E_INVALID_OPERATION;
    }

}

//***************************************************************************
//
//  Called by _IWmiCoreServices to establish a connection from the 'outside'.
//
//***************************************************************************
//
HRESULT CWbemNamespace::PathBasedConnect(
            /* [in] */ LPCWSTR pszPath,
            /* [in] */ LPCWSTR pszUser,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ ULONG uClientFlags,
            /* [in] */ DWORD dwSecFlags,
            /* [in] */ DWORD dwPermissions,
            /* [in] */ ULONG uInternalFlags,
            /* [in] */ LPCWSTR pszClientMachineName,
            /* [in] */ DWORD dwClientProcessID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pServices
            )
{
    HRESULT hRes;

    BOOL bForClient = FALSE;

    if ((uInternalFlags & WMICORE_CLIENT_ORIGIN_LOCAL) ||
        (uInternalFlags & WMICORE_CLIENT_ORIGIN_REMOTE) ||
        (uInternalFlags & WMICORE_CLIENT_TYPE_ALT_TRANSPORT)
       )
    {
        bForClient = TRUE;
    }

    hRes = UniversalConnect(
        0,                          // Parent CWbemNamespace; not known
        pCtx,                       // Context
        pszPath,                    // Path
        0,                          // No assoc selector at this point
        pszUser,                    // User
        0,                          // Call security
        0,                          // User handle
        uClientFlags,               // Flags from client
        uInternalFlags,             // Internal flags
        dwSecFlags,                 // Copy
        dwPermissions,              // Copy
        bForClient,                 // For client?
        FALSE,                      // Repository only
        pszClientMachineName,
        dwClientProcessID,
        riid,
        pServices
        );

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::InitNewTask(
    IN CAsyncReq *pReq,
    IN _IWmiFinalizer *pFnz,
    IN ULONG uTaskType,
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pAsyncClientSink
    )
{
    HRESULT hRes;

    if (pReq == 0 || pFnz == 0)
        return WBEM_E_INVALID_PARAMETER;
    
    // Create a task for tracking the operation.
    // =========================================

    CWmiTask *pNewTask = CWmiTask::CreateTask();
    if (pNewTask == 0)
        return WBEM_E_OUT_OF_MEMORY;
    CReleaseMe _2(pNewTask);

    hRes = pNewTask->Initialize(this, uTaskType, pCtx, pAsyncClientSink,pReq);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetTaskHandle((_IWmiCoreHandle *) pNewTask);
    if (FAILED(hRes))
        return hRes;

    pReq->SetTaskHandle((_IWmiCoreHandle *) pNewTask);

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::CreateSyncFinalizer(
        IN  IWbemContext *pCtx,
        IN _IWmiFinalizer **pResultFnz
        )
{
    HRESULT hRes;

    ULONG uFlags = WMI_FNLZR_FLAG_FAST_TRACK;

    // Determine calling context to see if the call is reentrant or what.
    // ==================================================================

    IWbemCausalityAccess *pCaus = 0;
    if (pCtx != 0)
    {
        hRes = pCtx->QueryInterface(IID_IWbemCausalityAccess, (LPVOID *) &pCaus);
        if (SUCCEEDED(hRes))
        {
            long lNumParents = 0;
            long lNumSiblings = 0;
            pCaus->GetHistoryInfo(&lNumParents, &lNumSiblings);
            if (lNumParents)
                uFlags = WMI_FNLZR_FLAG_FAST_TRACK;
            pCaus->Release();
        }
    }

    // Create Finalizer.
    // =================
    _IWmiFinalizer *pFnz = 0;
    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe _(pFnz);

    hRes = pFnz->Configure(uFlags, 0);
    if (FAILED(hRes))
        return hRes;

    pFnz->AddRef();
    *pResultFnz = pFnz;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//



HRESULT CWbemNamespace::CreateAsyncFinalizer(
    IWbemContext *pCtx,
    IWbemObjectSink *pStartingSink,
    _IWmiFinalizer **pResultFnz,
    IWbemObjectSink **pResultSinkEx
    )
{
    HRESULT hRes;

    ULONG uFlags = WMI_FNLZR_FLAG_FAST_TRACK;

    if ( m_bForClient )
        uFlags = WMI_FNLZR_FLAG_DECOUPLED;

    // Determine calling context to see if the call is reentrant or what.
    // ==================================================================
    IWbemCausalityAccess *pCaus = 0;
    if (pCtx != 0)
    {
        hRes = pCtx->QueryInterface(IID_IWbemCausalityAccess, (LPVOID *) &pCaus);
        if (SUCCEEDED(hRes))
        {
            long lNumParents = 0;
            long lNumSiblings = 0;
            pCaus->GetHistoryInfo(&lNumParents, &lNumSiblings);
            if (lNumParents)
                uFlags = WMI_FNLZR_FLAG_FAST_TRACK;
            pCaus->Release();
        }
    }

    // Create Finalizer.
    // =================
    _IWmiFinalizer *pFnz = 0;
    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe _(pFnz);

    hRes = pFnz->Configure(uFlags, 0);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID) pStartingSink);

    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->NewInboundSink(0, pResultSinkEx);
    if (FAILED(hRes))
        return hRes;

    pFnz->AddRef();
    *pResultFnz = pFnz;
    return WBEM_S_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//  Native async operations
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//  InstEnum    Sync[ ]  Async Impl[x]  AsyncEntry[x]
//  ClassEnum   Sync[ ]  Async Impl[x]  AsyncEntry[x]
//
//  PutInst     Sync[ ]  Async Impl[x]  AsyncEntry[x]
//  PutClass    Sync[ ]  Async Impl[x]  AsyncEntry[x]
//  DelInst     Sync[ ]  Async Impl[x]  AsyncEntry[x]
//  DelClass    Sync[ ]  Async Impl[x]  AsyncEntry[x]
//
//  GetObject   Sync[ ]  Async Impl[x]  AsyncEntry[x]
//
//  ExecQuery   Sync[ ]  Async Impl[x]  AsyncEntry[x]
//  ExecMethod  Sync[ ]  Async Impl[x]  AsyncEntry[x]
//
//

//***************************************************************************
//
//***************************************************************************
// ok
HRESULT CWbemNamespace::DeleteClassAsync(
    const BSTR strClass,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    try
    {
        return _DeleteClassAsync(
            WMICORE_TASK_TYPE_ASYNC | WMICORE_TASK_DELETE_CLASS,
            0, 0, strClass, lFlags, pCtx, pHandler
            );
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }    
}

//***************************************************************************
//
//***************************************************************************
// ok
HRESULT CWbemNamespace::CreateClassEnumAsync(
    const BSTR strParent,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    try
    {
        return _CreateClassEnumAsync(
            WMICORE_TASK_TYPE_ASYNC | WMICORE_TASK_ENUM_CLASSES,
            0, 0, strParent, lFlags, pCtx, pHandler
            );
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }    
}

//***************************************************************************
//
//***************************************************************************
// ok
HRESULT CWbemNamespace::PutClassAsync(
    READONLY IWbemClassObject* pObj,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    try
    {
        return _PutClassAsync(
            WMICORE_TASK_TYPE_ASYNC | WMICORE_TASK_PUT_CLASS,
            0, 0, pObj, lFlags, pCtx, pHandler
            );
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }    
}


//***************************************************************************
//
//***************************************************************************
// ok
HRESULT CWbemNamespace::PutInstanceAsync(
    IWbemClassObject* pInst,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    try
    {
        return _PutInstanceAsync(WMICORE_TASK_TYPE_ASYNC| WMICORE_TASK_PUT_INSTANCE,
                                              0, 0, pInst, lFlags, pCtx, pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }
}

//***************************************************************************
//
//***************************************************************************
//  ok
HRESULT CWbemNamespace::DeleteInstanceAsync(
    const BSTR strObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    try
    {
    return _DeleteInstanceAsync(
        WMICORE_TASK_TYPE_ASYNC| WMICORE_TASK_DELETE_INSTANCE,
        0, 0, strObjectPath, lFlags, pCtx, pHandler
        );
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }    
}

//***************************************************************************
//
//***************************************************************************
//  ok
HRESULT CWbemNamespace::CreateInstanceEnumAsync(
    const BSTR strClass,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    try
    {
    return _CreateInstanceEnumAsync(
        WMICORE_TASK_TYPE_ASYNC | WMICORE_TASK_ENUM_INSTANCES,
        0, 0,
        strClass, lFlags, pCtx, pHandler
        );
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }    
}

//***************************************************************************
//
//***************************************************************************
//  ok
HRESULT CWbemNamespace::ExecQueryAsync(
    const BSTR strQueryFormat,
    const BSTR strQuery,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    try 
    {
        return _ExecQueryAsync(WMICORE_TASK_TYPE_ASYNC | WMICORE_TASK_EXEC_QUERY,
                                             0, 0,
                                            strQueryFormat, strQuery, lFlags, pCtx, pHandler);
    }
    catch(...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;
    }    
}

//***************************************************************************
//
//***************************************************************************
//  ok
HRESULT CWbemNamespace::ExecMethodAsync(
    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext *pCtx,
    IWbemClassObject *pInParams,
    IWbemObjectSink* pHandler
    )
{
    try
    {
        return _ExecMethodAsync(WMICORE_TASK_TYPE_ASYNC | WMICORE_TASK_EXEC_METHOD,
                                                0, 0,
                                                ObjectPath,
                                                MethodName,
                                                lFlags,
                                                pCtx,
                                                pInParams,
                                                pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::GetObjectAsync(
    const BSTR strObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;
    if (!Allowed(WBEM_ENABLE))
        return WBEM_E_ACCESS_DENIED;

    try
    {
        return _GetObjectAsync(
            WMICORE_TASK_TYPE_ASYNC | WMICORE_TASK_GET_OBJECT,
            0, 0,
            strObjectPath,
            lFlags,
            pCtx,
            pHandler
            );
    }
    catch (...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }
}


//
// can throw
//
///////////////////////////////////////////////////////////////////////
HRESULT CWbemNamespace::_PutInstanceAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    IWbemClassObject* pInst,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::PutInstanceAsync"
        "   long lFlags = 0x%X\n"
        "   IWbemClassObject *pInst = 0x%X\n"
        "   IWbemObjectSink* pHandler = 0x%X\n",
        lFlags,
        pInst,
        pHandler
        ));

    // Parameter and object validation.
    // ================================

    if (pFnz == 0 && pHandler == 0)
       return WBEM_E_INVALID_PARAMETER;

    if (pInst == NULL)
       return WBEM_E_INVALID_PARAMETER;

    long lMainFlags = lFlags & ~WBEM_FLAG_SEND_STATUS & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS & ~WBEM_FLAG_OWNER_UPDATE;

    if (lMainFlags != WBEM_FLAG_CREATE_ONLY &&
        lMainFlags != WBEM_FLAG_UPDATE_ONLY &&
        lMainFlags != WBEM_FLAG_CREATE_OR_UPDATE)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    if (!m_bProvider && (lFlags & WBEM_FLAG_OWNER_UPDATE))
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    if (lFlags & WBEM_FLAG_OWNER_UPDATE)
    {
        lFlags -= WBEM_FLAG_OWNER_UPDATE;
        lFlags += WBEM_FLAG_NO_EVENTS;
    }

    // Check for per-property put context info.
    // ========================================

    if (pCtx) AdjustPutContext(pCtx);

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Schedule the request.
    // =====================

    wmilib::auto_ptr<CAsyncReq_PutInstanceAsync> pReq;

    pReq.reset(new CAsyncReq_PutInstanceAsync( this, pInst, lFlags, pPseudoSink, pCtx));

    if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;
    if (NULL == pReq->GetContext()) return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes)) return hRes;

    _1.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask ( 0 );
        return hRes;
    }
    pReq.release();

    return hRes;
}


//
// can throw
//
//////////////////////////////////////////////////////
HRESULT CWbemNamespace::_DeleteClassAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    const BSTR strClassOrg,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Parameter validation.
    // =====================

    if (pFnz == 0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (strClassOrg == 0 || strClassOrg[0] == 0)
            return WBEM_E_INVALID_PARAMETER;

    if (wcslen_max(strClassOrg,g_IdentifierLimit) > g_IdentifierLimit) return WBEM_E_QUOTA_VIOLATION;

    WCHAR * strClass;
    if (L'\\' == strClassOrg[0]  || L'/' == strClassOrg[0] )
    {
        strClass = wcschr(strClassOrg, L':');
        if (NULL == strClass) return WBEM_E_INVALID_OBJECT_PATH;
        strClass++;
        if (0 == strClass[0]) return WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        strClass = strClassOrg;
    }

    if (!m_bProvider && (lFlags & WBEM_FLAG_OWNER_UPDATE))
        return WBEM_E_INVALID_PARAMETER;

    if (lFlags & ~WBEM_FLAG_SEND_STATUS & ~WBEM_FLAG_OWNER_UPDATE)
        return WBEM_E_INVALID_PARAMETER;

    if (lFlags & WBEM_FLAG_OWNER_UPDATE)
    {
        lFlags -= WBEM_FLAG_OWNER_UPDATE;
        lFlags += WBEM_FLAG_NO_EVENTS;
    }

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Create request.
    // ===============

    wmilib::auto_ptr<CAsyncReq_DeleteClassAsync> pReq;
    pReq.reset(new CAsyncReq_DeleteClassAsync( this, strClass, lFlags, pPseudoSink, pCtx));
    
    if (pReq.get() == NULL) return WBEM_E_OUT_OF_MEMORY;
    if (NULL == pReq->GetContext()) return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes)) return hRes;

    _1.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask ( 0 );
        return hRes;
    }
    pReq.release();

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::_CreateClassEnumAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    const BSTR strParent,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;
    if (!Allowed(WBEM_ENABLE))
        return WBEM_E_ACCESS_DENIED;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::CreateClassEnumAsync\n"
        "   BSTR strParent = %S\n"
        "   long lFlags = 0x%X\n"
        "   IWbemObjectSink* pHandler = 0x%X\n",
        strParent,
        lFlags,
        pHandler
        ));

    // Parameter validation.
    // =====================

    if (pFnz == 0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;
    
    if (strParent)
    {
        if (wcslen_max(strParent,g_IdentifierLimit) > g_IdentifierLimit) 
            return WBEM_E_QUOTA_VIOLATION;
    }

    if (lFlags & ~(WBEM_FLAG_DEEP | WBEM_FLAG_SHALLOW | WBEM_FLAG_SEND_STATUS) & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS)
        return WBEM_E_INVALID_PARAMETER;

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);


    // Add this request to the queue.
    // ==============================

    wmilib::auto_ptr<CAsyncReq_CreateClassEnumAsync> pReq;
    pReq.reset(new CAsyncReq_CreateClassEnumAsync(this, strParent, lFlags, pPseudoSink,pCtx));

    if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;
    if (NULL == pReq->GetContext()) return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes)) return hRes;

    _1.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask ( 0 );
        return hRes;
    }
    pReq.release();

    return hRes;

}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::_PutClassAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    READONLY IWbemClassObject* pObj,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    CVARIANT vClass;
    if (pObj)
    {
        hRes = pObj->Get(L"__CLASS", 0, &vClass, 0, 0);
    }
    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::PutClassAsync\n"
        "   long lFlags = 0x%X\n"
        "   IWbemClassObject *pObj = 0x%X\n"
        "   IWbemObjectSink* pNotify = 0x%X\n"
        "   __CLASS=%S\n",
        lFlags,
        pObj,
        pHandler,
        vClass.GetStr()
        ));

    if (pFnz == 0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (pObj == NULL)
        return WBEM_E_INVALID_PARAMETER;

    if(!m_bProvider && (lFlags & WBEM_FLAG_OWNER_UPDATE))
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    long lTestFlags = lFlags & ~WBEM_FLAG_SEND_STATUS & ~WBEM_FLAG_OWNER_UPDATE
        & ~WBEM_MASK_UPDATE_MODE & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS;

    if (!((lTestFlags == WBEM_FLAG_CREATE_OR_UPDATE) ||
          (lTestFlags == WBEM_FLAG_UPDATE_ONLY) ||
          (lTestFlags == WBEM_FLAG_CREATE_ONLY) ||
          (lTestFlags == WBEM_FLAG_UPDATE_SAFE_MODE) ||
          (lTestFlags == WBEM_FLAG_UPDATE_FORCE_MODE) ||
          (lTestFlags == (WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_UPDATE_SAFE_MODE)) ||
          (lTestFlags == (WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_UPDATE_FORCE_MODE))))
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    if (lFlags & WBEM_FLAG_OWNER_UPDATE)
    {
        lFlags -= WBEM_FLAG_OWNER_UPDATE;
        lFlags += WBEM_FLAG_NO_EVENTS;
    }

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Create request.
    // ===============

    wmilib::auto_ptr<CAsyncReq_PutClassAsync> pReq;
    pReq.reset(new CAsyncReq_PutClassAsync(this, pObj, lFlags, pPseudoSink, pCtx));
   
    if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;
    if (NULL == pReq->GetContext()) return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes)) return hRes;

    _1.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask ( 0 );
        return hRes;        
    }
    pReq.release();

    return hRes;
}




//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::_DeleteInstanceAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    READONLY const BSTR strObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::DeleteInstance\n"
        "   BSTR ObjectPath = %S\n"
        "   long lFlags = %d\n"
        "   IWbemObjectSink* pHandler = 0x%X\n",
        strObjectPath, lFlags, pHandler
        ));

    // Parameter validation.
    // =====================

    if (pFnz == 0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;
    
    if (strObjectPath == 0 || strObjectPath[0] == 0) 
        return WBEM_E_INVALID_PARAMETER;
    
    if (wcslen_max(strObjectPath,g_PathLimit) > g_PathLimit) 
        return WBEM_E_QUOTA_VIOLATION;
        
    if (!m_bProvider && (lFlags & WBEM_FLAG_OWNER_UPDATE))
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    if (lFlags & ~WBEM_FLAG_SEND_STATUS & ~WBEM_FLAG_OWNER_UPDATE)
        return WBEM_E_INVALID_PARAMETER;

    if (lFlags & WBEM_FLAG_OWNER_UPDATE)
    {
        lFlags -= WBEM_FLAG_OWNER_UPDATE;
        lFlags += WBEM_FLAG_NO_EVENTS;
    }

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Create request.
    // ===============

    wmilib::auto_ptr<CAsyncReq_DeleteInstanceAsync> pReq;
    pReq.reset(new CAsyncReq_DeleteInstanceAsync(this, strObjectPath, lFlags, pPseudoSink, pCtx));

    if (NULL == pReq.get())return WBEM_E_OUT_OF_MEMORY;
    if ( NULL == pReq->GetContext()) return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes)) return hRes;

    _1.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask ( 0 );
        return hRes;        
    }
    pReq.release();

    return hRes;
}


//***************************************************************************
//
//  CWbemNamespace::CreateInstanceEnumAsync
//
//  Schedules an asynchrnous request that eventuall calls
//  Exec_CreateInstanceEnum.
//
//  Parameters and return values are described in help
//
//***************************************************************************
//
HRESULT CWbemNamespace::_CreateInstanceEnumAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    const BSTR strClass,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;
    if (!Allowed(WBEM_ENABLE))
        return WBEM_E_ACCESS_DENIED;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::CreateInstanceEnumAsync\n"
        "   BSTR Class = %S\n"
        "   long lFlags = 0x%X\n"
        "   IWbemObjectSink pHandler = 0x%X\n",
        strClass,
        lFlags,
        pHandler
        ));

    // Parameter validation.
    // =====================
    if (pFnz == 0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (strClass == 0 || strClass[0] == 0)
            return WBEM_E_INVALID_PARAMETER;

   if (wcslen_max(strClass,g_IdentifierLimit) > g_IdentifierLimit) 
            return WBEM_E_QUOTA_VIOLATION;

    if (lFlags & ~(WBEM_FLAG_DEEP | WBEM_FLAG_SHALLOW | WBEM_FLAG_SEND_STATUS |
        WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_DIRECT_READ))
            return WBEM_E_INVALID_PARAMETER;

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Add this request to the async queue.
    // ====================================

    wmilib::auto_ptr<CAsyncReq_CreateInstanceEnumAsync> pReq;
    pReq.reset(new CAsyncReq_CreateInstanceEnumAsync(this, strClass, lFlags, pPseudoSink,pCtx));
        
    if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;
    if (NULL == pReq->GetContext())  return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes)) return hRes;

    _1.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask ( 0 );
        return hRes;        
    }
    pReq.release();

    return hRes;
}


//***************************************************************************
//
//  CWbemNamespace::ExecQueryAsync
//
//  Schedules an asynchronous request that eventually calls
//  CQueryEngine::ExecQuery (see qengine.h)
//
//  Parameters and return values are described in help
//
//***************************************************************************
//
HRESULT CWbemNamespace::_ExecQueryAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    const BSTR strQueryFormat,
    const BSTR strQuery,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;
    if (!Allowed(WBEM_ENABLE))
        return WBEM_E_ACCESS_DENIED;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::ExecQueryAsync\n"
        "   BSTR QueryFormat = %S\n"
        "   BSTR Query = %S\n"
        "   IWbemObjectSink* pHandler = 0x%X\n",
        strQueryFormat,
        strQuery,
        pHandler
        ));

    // Parameter validation.
    // =====================
    if (pFnz == 0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (strQueryFormat == 0 || strQuery == 0)
        return WBEM_E_INVALID_PARAMETER;
    
    if ( 0 == strQueryFormat[0] || 0 == strQuery[0])
        return WBEM_E_INVALID_PARAMETER;

    if (wcslen_max(strQuery,g_QueryLimit) > g_QueryLimit) return WBEM_E_QUOTA_VIOLATION;            

    if (lFlags & ~WBEM_FLAG_PROTOTYPE & ~WBEM_FLAG_SEND_STATUS &
            ~WBEM_FLAG_ENSURE_LOCATABLE & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS &
            ~WBEM_FLAG_KEEP_SHAPE & ~WBEM_FLAG_DIRECT_READ
            )
        return WBEM_E_INVALID_PARAMETER;

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe rmPseudoSink(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Add the request to the queue.
    // =============================

    // will throw CX_MemoryException
    wmilib::auto_ptr<CAsyncReq_ExecQueryAsync> pReq;
    pReq.reset(new CAsyncReq_ExecQueryAsync(this, strQueryFormat, strQuery, lFlags,pPseudoSink, pCtx));
    
    if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;
    if ( NULL == pReq->GetContext() )  return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes)) return hRes;

    rmPseudoSink.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask ( 0 );
        return hRes;        
    }
    pReq.release();

    return hRes;
}

//
//
//  CWbemNamespace::_GetObjectAsync
//
//  Schedules an asynchrnous request that eventuall calls Exec_GetObjectByPath.
//
//  Parameters and return values are described in help
//
// throws or return
//
///////////////////////////////////////////////////////////////
HRESULT CWbemNamespace::_GetObjectAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    const BSTR strObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::GetObjectAsync\n"
        "   BSTR ObjectPath = %S\n"
        "   long lFlags = %d\n"
        "   IWbemObjectSink* pHandler = 0x%X\n",
        strObjectPath,
        lFlags,
        pHandler
        ));

    // Parameter validation.
    // =====================
    if (pFnz == 0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (lFlags & ~WBEM_FLAG_SEND_STATUS & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS
        & ~WBEM_FLAG_DIRECT_READ)
        return WBEM_E_INVALID_PARAMETER;

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();    // Compensate for CReleaseMe to follow
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Add request to the async queue.
    // ===============================

    wmilib::auto_ptr<CAsyncReq_GetObjectAsync> pReq;
    pReq.reset(new CAsyncReq_GetObjectAsync(this, strObjectPath, lFlags, pPseudoSink, pCtx));
    
    if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;
    if (NULL == pReq->GetContext()) return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes))  return hRes;

    _1.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask(0);
        return hRes;
    }
    pReq.release();

    return hRes;
}


//
//
//  CWbemNamespace::ExecMethodAsync
//
//  can throw
//
//////////////////////////////////////////////////////////
HRESULT CWbemNamespace::_ExecMethodAsync(
    IN ULONG uInternalFlags,
    IN _IWmiFinalizer *pFnz,
    IN _IWmiCoreHandle *phTask,
    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext *pCtx,
    IWbemClassObject *pInParams,
    IWbemObjectSink* pHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::ExecMethodAsync\n"
        "   BSTR ObjectPath = %S\n"
        "   BSTR MethodName = %S\n"
        "   long lFlags = %d\n"
        "   IWbemClassObject * pIn = 0x%X\n",
        ObjectPath, MethodName, lFlags, pInParams
        ));

    // Parameter validation.
    // =====================
    if (pFnz ==0 && pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (lFlags & ~WBEM_FLAG_SEND_STATUS)
        return WBEM_E_INVALID_PARAMETER;

    // Create Finalizer.
    // =================

    IWbemObjectSink *pPseudoSink = 0;
    if (pFnz == 0)
    {
        hRes = CreateAsyncFinalizer(pCtx, pHandler, &pFnz, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
    }
    else // borrowed finalizer
    {
        hRes = pFnz->NewInboundSink(0, &pPseudoSink);
        if (FAILED(hRes))
            return hRes;
        pFnz->AddRef();
    }

    CReleaseMe _1(pPseudoSink);
    CReleaseMe _2(pFnz);

    // Add request to the async queue.
    // ===============================

    wmilib::auto_ptr<CAsyncReq_ExecMethodAsync> pReq;
    pReq.reset(new CAsyncReq_ExecMethodAsync(this,
                                        ObjectPath,
                                        MethodName,
                                        lFlags,
                                        pInParams,
                                        pPseudoSink,
                                        pCtx));

    if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;
    if ( NULL == pReq->GetContext() ) return WBEM_E_OUT_OF_MEMORY;

    hRes = InitNewTask(pReq.get(), pFnz, uInternalFlags, pReq->GetContext(), pHandler);
    if (FAILED(hRes)) return hRes;

    _1.release();
    hRes = ConfigMgr::EnqueueRequest(pReq.get());
    if (FAILED(hRes))
    {
        pFnz->CancelTask ( 0 );
        return hRes;        
    }
    pReq.release();

    return hRes;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//  Native sync operations
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//***************************************************************************
//
//  CWbemNamespace::DeleteClass
//
//  Calls DeleteClassAsync and waits for completion
//
//  Parameters and return values are described in help
//
//***************************************************************************
//
HRESULT CWbemNamespace::DeleteClass(
    const BSTR strClass,
    long lFlags,
    IWbemContext* pCtx,
    IWbemCallResult** ppResult
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    try
    {
        DEBUGTRACE((LOG_WBEMCORE,
            "CALL CWbemNamespace::DeleteClass"
            "   BSTR Class = %S\n"
            "   long lFlags = 0x%X\n",
            strClass,
            lFlags
            ));

        // Parameter validation.
        // =====================

        if (lFlags
            & ~WBEM_FLAG_RETURN_IMMEDIATELY
            & ~WBEM_FLAG_OWNER_UPDATE
            )
            return WBEM_E_INVALID_PARAMETER;

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) && ppResult == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // Create Finalizer.
        // =================

        _IWmiFinalizer *pFnz = 0;
        hRes = CreateSyncFinalizer(pCtx, &pFnz);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pFnz);

        ULONG uTaskType = WMICORE_TASK_DELETE_CLASS;
        if (ppResult)
            uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
        else
            uTaskType |= WMICORE_TASK_TYPE_SYNC;

        // Call the async side.
        // ====================

        hRes = _DeleteClassAsync(uTaskType, pFnz, 0, strClass, 
                                                lFlags & ~WBEM_RETURN_IMMEDIATELY,pCtx, NULL);

        if (FAILED(hRes))
            return hRes;

        // Check for the two return paradigms.
        // ===================================

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
        {
            HRESULT hResTemp = pFnz->GetOperationResult(0, INFINITE, &hRes);
            if (FAILED(hResTemp))
                return hResTemp;
        }
        if (ppResult)
        {
            hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) ppResult);
        }
        return hRes;
    }
    catch(...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;        
    }
}

//***************************************************************************
//
//  CWbemNamespace::PutClass
//
//  Calls PutClassAsync and waits for completion
//
//  Parameters and return values are described in help
//
//***************************************************************************

HRESULT CWbemNamespace::PutClass(
    READONLY IWbemClassObject* pObj,
    long lFlags,
    IWbemContext* pCtx,
    NEWOBJECT IWbemCallResult** ppResult
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    CVARIANT vClass;
    if (pObj)
    {
        hRes = pObj->Get(L"__CLASS", 0, &vClass, 0, 0);
    }

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::PutClass\n"
        "   long lFlags = 0x%X\n"
        "   IWbemClassObject *pObj = 0x%X\n"
        "   __CLASS=%S\n",
        lFlags,
        pObj,
        vClass.GetStr()
        ));


    if (lFlags
        & ~WBEM_FLAG_RETURN_IMMEDIATELY
        & ~WBEM_FLAG_OWNER_UPDATE
        & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS
        & ~WBEM_FLAG_CREATE_OR_UPDATE
        & ~WBEM_FLAG_UPDATE_ONLY
        & ~WBEM_FLAG_CREATE_ONLY
        & ~WBEM_FLAG_UPDATE_SAFE_MODE
        & ~WBEM_FLAG_UPDATE_FORCE_MODE
        )
        return WBEM_E_INVALID_PARAMETER;

    if ((lFlags & WBEM_RETURN_IMMEDIATELY) && ppResult == NULL)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        if( ((lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) == 0)&&(ppResult==NULL))
        {
            HANDLE hCurrentToken;
            if(OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE,
                                &hCurrentToken))
            {
                //
                // Got a thread token --- cannot fast-track because otherwise we
                // will have a thread token on a thread executing internal code
                //

                CloseHandle(hCurrentToken);
            }
            else if (CWbemQueue::GetCurrentRequest() == NULL)
            {
                if (!m_bProvider && (lFlags & WBEM_FLAG_OWNER_UPDATE))
                {
                    return WBEM_E_INVALID_PARAMETER;
                }
                
                if (lFlags & WBEM_FLAG_OWNER_UPDATE)
                {
                    lFlags -= WBEM_FLAG_OWNER_UPDATE;
                    lFlags += WBEM_FLAG_NO_EVENTS;
                }

                IWbemContext *pContext = pCtx ;
                if (pContext)
                    pContext->AddRef () ;
                else
                {
                    pContext = ConfigMgr::GetNewContext();
                    if ( pContext == NULL )
                        return WBEM_E_OUT_OF_MEMORY;
                }

                CReleaseMe _1_pContext (pContext) ;

                CSynchronousSink *pSyncSink = CSynchronousSink::Create();
                if (pSyncSink == NULL)  return WBEM_E_OUT_OF_MEMORY;
                pSyncSink->AddRef();
                CReleaseMe _2(pSyncSink);

                hRes = Exec_PutClass(pObj, lFlags, pContext, pSyncSink);

                // Extract the new object from the sink.
                // ======================================

                pSyncSink->Block();

                IWbemClassObject* pErrorObj = NULL;
                pSyncSink->GetStatus(&hRes, NULL, &pErrorObj);
                CReleaseMe rm1(pErrorObj);

                if(pErrorObj)
                {
                    IErrorInfo* pErrorInfo = NULL;
                    pErrorObj->QueryInterface(IID_IErrorInfo,(void**)&pErrorInfo);
                    SetErrorInfo(0, pErrorInfo);
                    pErrorInfo->Release();
                }

                return hRes;
            }
        }

        // Create Finalizer.
        // =================

        _IWmiFinalizer *pFnz = 0;
        hRes = CreateSyncFinalizer(pCtx, &pFnz);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pFnz);

        ULONG uTaskType = WMICORE_TASK_PUT_CLASS;
        if (ppResult)
            uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
        else
            uTaskType |= WMICORE_TASK_TYPE_SYNC;

        // Do the work elsewhere.
        // ======================

        hRes = _PutClassAsync(uTaskType, pFnz, 0, pObj, lFlags & ~WBEM_RETURN_IMMEDIATELY,
                         pCtx, NULL);

        if (FAILED(hRes))
            return hRes;

        // Check for the two return paradigms.
        // ===================================

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
        {
            HRESULT hResTemp = pFnz->GetOperationResult(0, INFINITE, &hRes);
            if (FAILED(hResTemp))
                return hResTemp;
        }
        if (ppResult)
        {
            hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) ppResult);
        }

        return hRes;

    }
    catch(...) // this interface calls the Exec_[MetrhodName] straight
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;
    }
}


//***************************************************************************
//
//  CWbemNamespace::CancelAsyncRequest.
//
//  Currently a noop, eventually this function will cancel an asynchrnous
//  request based on the handle value it returned.
//
//  Parameters and return values are described in help
//
//***************************************************************************
HRESULT CWbemNamespace::CancelAsyncCall(IWbemObjectSink* pSink)
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Note that LOMEM_CHECK is not needed or wanted here

    if (pSink == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Quickly cancel via Arbitrator.
    // ===============================
    if (m_pArb)
    {
        hRes = m_pArb->CancelTasksBySink(WMIARB_CALL_CANCELLED_CLIENT, IID_IWbemObjectSink, pSink);
    }

    return hRes ;

}



//***************************************************************************
//
//  CWbemNamespace::PutInstance
//
//  Calls PutInstanceAsync and waits for completion
//
//  Parameters and return values are described in help
//
//***************************************************************************


HRESULT CWbemNamespace::PutInstance(
    IWbemClassObject* pInst,
    long lFlags,
    IWbemContext* pCtx,
    IWbemCallResult** ppResult
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Parameter validation.
    // =====================

    if (lFlags
        & ~WBEM_RETURN_IMMEDIATELY
        & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS
        & ~WBEM_FLAG_OWNER_UPDATE
        & ~WBEM_FLAG_CREATE_ONLY
        & ~WBEM_FLAG_UPDATE_ONLY
        & ~WBEM_FLAG_CREATE_OR_UPDATE
        )
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        DEBUGTRACE((LOG_WBEMCORE,
            "CALL CWbemNamespace::PutInstance"
            "   long lFlags = 0x%X\n"
            "   IWbemClassObject *pInst = 0x%X\n",
            lFlags,
            pInst));
        
        if( ((lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) == 0)&&(ppResult==NULL))
        {
            // trick for allowing setup to set the SD
            AutoRevertSecTlsFlag RevSec ( (LPVOID) 1 );
            if ( IsNtSetupRunning() )
            {
                RevSec.SetSecTlsFlag ( (LPVOID) 0 );
            }

            HANDLE hCurrentToken;
            if(OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE,
                                &hCurrentToken))
            {
                //
                // Got a thread token --- cannot fast-track because otherwise we
                // will have a thread token on a thread executing internal code
                //

                CloseHandle(hCurrentToken);
            }
            else if (CWbemQueue::GetCurrentRequest() == NULL)
            {
                IWbemContext *pContext = pCtx ;
                if (pContext)
                    pContext->AddRef () ;
                else
                {
                    pContext = ConfigMgr::GetNewContext();
                    if ( pContext == NULL )
                        return WBEM_E_OUT_OF_MEMORY;
                }

                CReleaseMe _1_pContext (pContext) ;

                CSynchronousSink *pSyncSink = CSynchronousSink::Create();
                if (pSyncSink == NULL)
                    return WBEM_E_OUT_OF_MEMORY;

                pSyncSink->AddRef();
                CReleaseMe _2(pSyncSink);


                if (!m_bProvider && (lFlags & WBEM_FLAG_OWNER_UPDATE))
                {
                    return WBEM_E_INVALID_PARAMETER;
                }

                if (lFlags & WBEM_FLAG_OWNER_UPDATE)
                {
                    lFlags -= WBEM_FLAG_OWNER_UPDATE;
                    lFlags += WBEM_FLAG_NO_EVENTS;
                }

                hRes = Exec_PutInstance(pInst, lFlags, pContext, pSyncSink);

                // Extract the new object from the sink.
                // ======================================

                pSyncSink->Block();
                IWbemClassObject* pErrorObj = NULL;
                pSyncSink->GetStatus(&hRes, NULL, &pErrorObj);
                CReleaseMe rm1(pErrorObj);

                if(pErrorObj)
                {
                    IErrorInfo* pErrorInfo = NULL;
                    pErrorObj->QueryInterface(IID_IErrorInfo,
                                           (void**)&pErrorInfo);
                    SetErrorInfo(0, pErrorInfo);
                    pErrorInfo->Release();
                }

                return hRes;
            }
        }

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) && ppResult == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // Create Finalizer.
        // =================

        _IWmiFinalizer *pFnz = 0;
        hRes = CreateSyncFinalizer(pCtx, &pFnz);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pFnz);

        ULONG uTaskType = WMICORE_TASK_PUT_INSTANCE;
        if (ppResult)
            uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
        else
            uTaskType |= WMICORE_TASK_TYPE_SYNC;

        // Do the work elsewhere.
        // ======================

        hRes = _PutInstanceAsync(uTaskType, pFnz, 0, pInst, 
                                               lFlags & ~WBEM_RETURN_IMMEDIATELY,
                                               pCtx, NULL);

        if (FAILED(hRes))
            return hRes;

        // Check for the two return paradigms.
        // ===================================

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
        {
            HRESULT hResTemp = pFnz->GetOperationResult(0, INFINITE, &hRes);
            if (FAILED(hResTemp))
                return hResTemp;
        }
        if (ppResult)
        {
            hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) ppResult);
        }

        return hRes;

    }
    catch(...) // this interfaces calls the Exec_[MethodName]
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;
    }
}


//***************************************************************************
//
//  CWbemNamespace::DeleteInstance
//
//  Calls DeleteInstanceAsync and waits for completion
//
//  Parameters and return values are described in help
//
//***************************************************************************

HRESULT CWbemNamespace::DeleteInstance(
    READONLY const BSTR strObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    IWbemCallResult** ppResult
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::DeleteInstance\n"
        "   BSTR ObjectPath = %S\n"
        "   long lFlags = %d\n",
        strObjectPath, lFlags
        ));

    if (lFlags & ~WBEM_FLAG_RETURN_IMMEDIATELY &~ WBEM_FLAG_OWNER_UPDATE)
        return WBEM_E_INVALID_PARAMETER;

    if((lFlags & WBEM_RETURN_IMMEDIATELY) && ppResult == NULL)
        return WBEM_E_INVALID_PARAMETER;


    // Create Finalizer.
    // =================

    _IWmiFinalizer *pFnz = 0;
    hRes = CreateSyncFinalizer(pCtx, &pFnz);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe _1(pFnz);

    ULONG uTaskType = WMICORE_TASK_DELETE_INSTANCE;
    if (ppResult)
        uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
    else
        uTaskType |= WMICORE_TASK_TYPE_SYNC;

    // Do the work elsewhere.
    // ======================

    hRes = _DeleteInstanceAsync(uTaskType, pFnz, 0, strObjectPath, lFlags & ~WBEM_RETURN_IMMEDIATELY,
                     pCtx, NULL);

    if (FAILED(hRes))
        return hRes;

    // Check for the two return paradigms.
    // ===================================

    if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
    {
        HRESULT hResTemp = pFnz->GetOperationResult(0,INFINITE, &hRes);
        if (FAILED(hResTemp))
            return hResTemp;
    }
    if (ppResult)
    {
        hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) ppResult);
    }

    return hRes;
}


//***************************************************************************
//
//  CWbemNamespace::GetObject
//
//  Calls GetObjectAsync and waits for completion
//
//  Parameters and return values are described in help
//
//***************************************************************************

HRESULT CWbemNamespace::GetObject(
    READONLY const BSTR strObjectPath,
    long lFlags,
    IWbemContext* pCtx,
    NEWOBJECT IWbemClassObject** ppObj,
    NEWOBJECT IWbemCallResult** ppResult
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;
    if (!Allowed(WBEM_ENABLE))
        return WBEM_E_ACCESS_DENIED;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::GetObject\n"
        "   BSTR ObjectPath = %S\n"
        "   long lFlags = %d\n"
        "   IWbemClassObject ** pObj = 0x%X\n",
        strObjectPath,
        lFlags,
        ppObj
        ));

    if (ppObj)
        *ppObj = NULL;

    if (lFlags
        & ~WBEM_FLAG_RETURN_IMMEDIATELY
        & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS
        & ~WBEM_FLAG_DIRECT_READ
        )
        return WBEM_E_INVALID_PARAMETER;

    if ((lFlags & WBEM_RETURN_IMMEDIATELY) && ppResult == NULL)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        if( ((lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) == 0)&&(ppResult==NULL))
        {
            //
            // This is special case GetObject synchronous. We dont want to allow this unless,
            // we're running setup.
            //
            AutoRevertSecTlsFlag RevSec ( (LPVOID) 1 );
            if ( IsNtSetupRunning() )
            {
                RevSec.SetSecTlsFlag ( (LPVOID) 0 );
            }

            HANDLE hCurrentToken;
            if(OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE,
                                &hCurrentToken))
            {
                //
                // Got a thread token --- cannot fast-track because otherwise we
                // will have a thread token on a thread executing internal code
                //

                CloseHandle(hCurrentToken);
            }
            else if (CWbemQueue::GetCurrentRequest() == NULL)
            {
                IWbemContext *pContext = pCtx ;
                if (pContext)
                    pContext->AddRef () ;
                    else
                {
                    pContext = ConfigMgr::GetNewContext();
                    if ( pContext == NULL )
                        return WBEM_E_OUT_OF_MEMORY;
                }

                CReleaseMe _1_pContext (pContext) ;

                CSynchronousSink *pSyncSink = CSynchronousSink::Create();
                if (pSyncSink == NULL)
                    return WBEM_E_OUT_OF_MEMORY;

                pSyncSink->AddRef();
                CReleaseMe _2(pSyncSink);

                hRes = Exec_GetObject(strObjectPath, lFlags, pContext, pSyncSink);
                //if (FAILED(hRes))
                //{
                //  return hRes;
                //}

                // Extract the new object from the sink.
                // ======================================

                pSyncSink->Block();

                IWbemClassObject* pErrorObj = NULL;
                pSyncSink->GetStatus(&hRes, NULL, &pErrorObj);
                CReleaseMe rm1(pErrorObj);

                if(pErrorObj)
                {
                    IErrorInfo* pErrorInfo = NULL;
                    pErrorObj->QueryInterface(IID_IErrorInfo,
                                           (void**)&pErrorInfo);
                    SetErrorInfo(0, pErrorInfo);
                    pErrorInfo->Release();
                }

                if (SUCCEEDED(hRes))
                {
                    if(pSyncSink->GetObjects().GetSize() != 1)
                        return WBEM_E_CRITICAL_ERROR;

                    // Only access the returned object if ppObj is non-NULL.
                    if ( NULL != ppObj )
                    {
                        *ppObj = pSyncSink->GetObjects()[0];
                        (*ppObj)->AddRef();
                    }
                }

                return hRes;
            }
        }

        // Create Finalizer.
        // =================

        _IWmiFinalizer *pFnz = 0;
        hRes = CreateSyncFinalizer(pCtx, &pFnz);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pFnz);

        ULONG uTaskType = WMICORE_TASK_GET_OBJECT;
        if (ppResult)
            uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
        else
            uTaskType |= WMICORE_TASK_TYPE_SYNC;

        // Do the work elsewhere.
        // ======================

        hRes = _GetObjectAsync(uTaskType, pFnz, 0, strObjectPath, lFlags & ~WBEM_RETURN_IMMEDIATELY,
                         pCtx, NULL);

        if (FAILED(hRes))
        {
            return hRes;
        }

        // Check for the two return paradigms.
        // ===================================

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
        {
            HRESULT hResTemp = pFnz->GetOperationResult(0, INFINITE, &hRes);
            if (FAILED(hResTemp))
                return hResTemp;
            if (FAILED(hRes))
                return hRes;
        }

        if (ppResult)
        {
            hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) ppResult);
            if (FAILED(hRes))
            {
                return hRes;
            }
            if (ppObj && ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0))
                hRes = (*ppResult)->GetResultObject(INFINITE, ppObj);
        }
        else if (ppObj && ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0))
        {
            hRes = pFnz->GetResultObject(0, IID_IWbemClassObject, (LPVOID *) ppObj);
        }

        return hRes;
    }
    catch(...) // this interface goes to the Exec_[MethodName straight]
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;
    }
}


//***************************************************************************
//
//  CWbemNamespace::ExecMethod
//
//***************************************************************************

HRESULT CWbemNamespace::ExecMethod(
    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext *pCtx,
    IWbemClassObject *pInParams,
    IWbemClassObject **ppOutParams,
    IWbemCallResult  **ppResult
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::ExecMethod\n"
        "   BSTR ObjectPath = %S\n"
        "   BSTR MethodName = %S\n"
        "   long lFlags = %d\n"
        "   IWbemClassObject * pIn = 0x%X\n",
        ObjectPath, MethodName, lFlags, pInParams
        ));

    // Parameter validation.
    // =====================

    if (ppOutParams)
        *ppOutParams = NULL;

    if (lFlags & ~WBEM_FLAG_RETURN_IMMEDIATELY)
        return WBEM_E_INVALID_PARAMETER;

    // Create Finalizer.
    // =================

    _IWmiFinalizer *pFnz = 0;
    hRes = CreateSyncFinalizer(pCtx, &pFnz);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe _1(pFnz);

    ULONG uTaskType = WMICORE_TASK_EXEC_METHOD;
    if (ppResult)
        uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
    else
        uTaskType |= WMICORE_TASK_TYPE_SYNC;

    // Do the work elsewhere.
    // ======================

    hRes = _ExecMethodAsync(uTaskType, pFnz, 0, ObjectPath, MethodName,
               lFlags & ~WBEM_RETURN_IMMEDIATELY, pCtx, pInParams, NULL);

    if (FAILED(hRes))
        return hRes;

    // Check for the two return paradigms.
    // ===================================

    if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
    {
        HRESULT hResTemp = pFnz->GetOperationResult(0, INFINITE, &hRes);
        if (FAILED(hResTemp))
            return hResTemp;
        if (FAILED(hRes))
            return hRes;
        if (ppResult)
        {
            hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) ppResult);
            if (FAILED(hRes))
                return hRes;
            if (ppOutParams)
            {
                hRes = (*ppResult)->GetResultObject(INFINITE, ppOutParams);
                if (hRes == WBEM_E_NOT_FOUND)   //If there was no object we still return success!
                    hRes = WBEM_S_NO_ERROR;
            }
        }
        else
        {
            hRes = pFnz->GetResultObject(0, IID_IWbemClassObject, (LPVOID *) ppOutParams);
            if (hRes == WBEM_E_NOT_FOUND)   //If there was no object we still return success!
                hRes = WBEM_S_NO_ERROR;
        }
    }
    else
    {
        //
        // If we have a call result pointer we should try to use it
        //
        if ( ppResult )
        {
            hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) ppResult);
            if (hRes == WBEM_E_NOT_FOUND)   //If there was no object we still return success!
                hRes = WBEM_S_NO_ERROR;
        }
        else // semysync and nobody interested
        {
                pFnz->CancelTask(0);
            hRes = WBEM_S_NO_ERROR;            
        }

    }

    return hRes;
}

//***************************************************************************
//
//  CWbemNamespace::CreateInstanceEnum
//
//  Calls CreateInstanceEnumAsync and waits for completion
//
//  Parameters and return values are described in help
//
//***************************************************************************
HRESULT CWbemNamespace::CreateInstanceEnum(
    const BSTR strClass,
    long lFlags,
    IWbemContext* pCtx,
    IEnumWbemClassObject** ppEnum
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::CreateInstanceEnum\n"
        "   long lFlags = 0x%X\n"
        "   BSTR Class = %S\n"
        "   IEnumWbemClassObject **pEnum = 0x%X\n",
        lFlags,
        strClass,
        ppEnum
        ));

    // Validate parameters
    // ===================

    if (lFlags
        & ~WBEM_RETURN_IMMEDIATELY
        & ~WBEM_FLAG_FORWARD_ONLY
        & ~WBEM_FLAG_DEEP
        & ~WBEM_FLAG_SHALLOW
        & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS
        & ~WBEM_FLAG_DIRECT_READ
        )
        return WBEM_E_INVALID_PARAMETER;

    if (ppEnum == NULL)
        return WBEM_E_INVALID_PARAMETER;
    *ppEnum = NULL;

    // Create Finalizer.
    // =================

    _IWmiFinalizer *pFnz = 0;
    hRes = CreateSyncFinalizer(pCtx, &pFnz);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe _1(pFnz);

    ULONG uTaskType = WMICORE_TASK_ENUM_INSTANCES;
    if (lFlags & WBEM_RETURN_IMMEDIATELY)
        uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
    else
        uTaskType |= WMICORE_TASK_TYPE_SYNC;

    // Do the work.
    // ============

    hRes = _CreateInstanceEnumAsync(uTaskType, pFnz, 0, strClass,
                    lFlags & ~WBEM_RETURN_IMMEDIATELY & ~WBEM_FLAG_FORWARD_ONLY,
                    pCtx, NULL);

    if (FAILED(hRes))
        return hRes;

    if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
        pFnz->GetOperationResult(0, INFINITE, &hRes);

    if (SUCCEEDED(hRes))
    {
        IEnumWbemClassObject* pEnum = NULL;
        HRESULT hResTemp = pFnz->GetResultObject(lFlags, IID_IEnumWbemClassObject, (LPVOID*)&pEnum);
        if (FAILED(hResTemp))
            return hResTemp;
        CReleaseMe _2(pEnum);

        *ppEnum = pEnum;
        pEnum->AddRef();    // counteract CReleaseMe
    }

    return hRes;
}


//***************************************************************************
//
//  CWbemNamespace::CreateClassEnum
//
//  Invokes CreateClassEnumAsync and waits for completion. Actual work is
//  performed in Exec_CreateClassEnum.
//
//  Parameters and return values are described in help
//
//***************************************************************************
HRESULT CWbemNamespace::CreateClassEnum(
    const BSTR strParent,
    long lFlags,
    IWbemContext* pCtx,
    IEnumWbemClassObject **ppEnum
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::CreateClassEnum\n"
        "   BSTR Parent = %S\n"
        "   long lFlags = 0x%X\n"
        "   IEnumWbemClassObject = 0x%X\n",
        strParent,
        lFlags,
        ppEnum
        ));

    // Validate parameters
    // ===================

   if (lFlags
        & ~WBEM_FLAG_DEEP
        & ~WBEM_FLAG_SHALLOW
        & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS
        & ~WBEM_FLAG_RETURN_IMMEDIATELY
        & ~WBEM_FLAG_FORWARD_ONLY
        )
        return WBEM_E_INVALID_PARAMETER;

    if (ppEnum == NULL)
        return WBEM_E_INVALID_PARAMETER;

    *ppEnum = NULL;

    // Create Finalizer.
    // =================

    _IWmiFinalizer *pFnz = 0;
    hRes = CreateSyncFinalizer(pCtx, &pFnz);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe _1(pFnz);

    ULONG uTaskType = WMICORE_TASK_ENUM_CLASSES;
    if (lFlags & WBEM_RETURN_IMMEDIATELY)
        uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
    else
        uTaskType |= WMICORE_TASK_TYPE_SYNC;

    // Do the work.
    // ============

    hRes = _CreateClassEnumAsync(uTaskType, pFnz, 0, strParent,
                    lFlags & ~WBEM_RETURN_IMMEDIATELY & ~WBEM_FLAG_FORWARD_ONLY,
                    pCtx, NULL);

    if (FAILED(hRes))
        return hRes;

    if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
        pFnz->GetOperationResult(0, INFINITE, &hRes);

    if (SUCCEEDED(hRes))
    {
        IEnumWbemClassObject* pEnum = NULL;
        HRESULT hResTemp = pFnz->GetResultObject(lFlags, IID_IEnumWbemClassObject, (LPVOID*)&pEnum);
        if (FAILED(hResTemp))
            return hResTemp;
        CReleaseMe _2(pEnum);

        *ppEnum = pEnum;
        pEnum->AddRef();    // Counteract CReleaseMe
    }
    return hRes;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWbemNamespace::ExecQuery(
    READONLY const BSTR strQueryFormat,
    READONLY const BSTR strQuery,
    long lFlags,
    IWbemContext* pCtx,
    NEWOBJECT IEnumWbemClassObject** ppEnum
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::ExecQuery\n"
        "   BSTR QueryFormat = %S\n"
        "   BSTR Query = %S\n"
        "   IEnumWbemClassObject **pEnum = 0x%X\n",
        strQueryFormat,
        strQuery,
        ppEnum
        ));

    // Validate parameters
    // ===================

    if (lFlags
        & ~WBEM_FLAG_PROTOTYPE
        & ~WBEM_FLAG_ENSURE_LOCATABLE
        & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS
        & ~WBEM_FLAG_KEEP_SHAPE
        & ~WBEM_RETURN_IMMEDIATELY
        & ~WBEM_FLAG_FORWARD_ONLY
        & ~WBEM_FLAG_DIRECT_READ
        )
        return WBEM_E_INVALID_PARAMETER;

    if (ppEnum == NULL)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        *ppEnum = NULL;

        // Create Finalizer.
        _IWmiFinalizer *pFnz = 0;
        hRes = CreateSyncFinalizer(pCtx, &pFnz);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pFnz);

        ULONG uTaskType = WMICORE_TASK_EXEC_QUERY;
        if (lFlags & WBEM_RETURN_IMMEDIATELY)
            uTaskType |= WMICORE_TASK_TYPE_SEMISYNC;
        else
            uTaskType |= WMICORE_TASK_TYPE_SYNC;

        // Do the work.
        // ============

        hRes = _ExecQueryAsync(uTaskType, pFnz, 0, strQueryFormat, strQuery,
                        lFlags & ~WBEM_RETURN_IMMEDIATELY & ~WBEM_FLAG_FORWARD_ONLY,
                        pCtx, NULL);

        if (FAILED(hRes))
            return hRes;

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
            pFnz->GetOperationResult(0, INFINITE, &hRes);

        if (SUCCEEDED(hRes))
        {
            IEnumWbemClassObject* pEnum = NULL;
            HRESULT hResTemp = pFnz->GetResultObject(lFlags, IID_IEnumWbemClassObject, (LPVOID*)&pEnum);
            if (FAILED(hResTemp))
                return hResTemp;
            CReleaseMe _2(pEnum);

            *ppEnum = pEnum;
            pEnum->AddRef();    // Counteract CReleaseMe
        }

        return hRes;

    }
    catch(CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;
    }
}

//***************************************************************************
//
//  CWbemNamespace::QueryObjectSink
//
//  Returns the pointer to the ESS event handler. Clients can use this pointer
//  to supply events to WINMGMT. NOTE: this pointer will be NULL if ESS is
//  disabled (see cfgmgr.h).
//
//  Parameters and return values are described in help
//
//***************************************************************************


HRESULT CWbemNamespace::QueryObjectSink(
    long lFlags,
    IWbemObjectSink** ppHandler
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;
    if (!Allowed(WBEM_FULL_WRITE_REP))
        return WBEM_E_ACCESS_DENIED;

    if (ppHandler == NULL)
        return WBEM_E_INVALID_PARAMETER;

    *ppHandler = NULL;

    if (lFlags != 0)
        return WBEM_E_INVALID_PARAMETER;

    IWbemEventSubsystem_m4* pEss = ConfigMgr::GetEssSink();
    if (pEss)
    {
        CReleaseMe rm_(pEss);

        CWbemPtr<IWbemObjectSink> pEssSink;
        hRes = pEss->GetNamespaceSink(m_pThisNamespaceFull, &pEssSink);

        if ( FAILED(hRes)) return hRes;

        CWbemPtr<CSecureEssNamespaceSink> pSecureSink = new CSecureEssNamespaceSink(this,pEssSink);

        if ( pSecureSink == NULL )
            return WBEM_E_OUT_OF_MEMORY;

        hRes = pSecureSink->QueryInterface( IID_IWbemObjectSink,
                                            (void**)ppHandler );
        return hRes;
    }
    else
    {
        return WBEM_E_NOT_SUPPORTED;
    }
}



//***************************************************************************
//
//  CWbemNamespace::OpenNamespace
//
//  Opens a child namespace of this one. Username, password, locale id,
//  flags and error object parameters are ignored.
//
//  Parameters:
//
//    BSTR NsPath                   Relative path to the namespace
//    BSTR User                     Reserved, must be NULL.
//    BSTR Password                 Reserved, must be NULL.
//    long lLocaleId                Reserved, must be NULL.
//    long lFlags                   Reserved, must be NULL.
//    IWbemServices **pNewContext    Destination for the new namespace pointer.
//                                  Must be released by the caller.
//    IWbemClassObject** ppErrorObj  Reserved, must be NULL.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On success
//      WBEM_E_INVALID_PARAMETER     Invalid name.
//
//***************************************************************************

HRESULT CWbemNamespace::OpenNamespace(
    const BSTR NsPath,
    long lFlags,
    IWbemContext* pCtx,
    IWbemServices **ppNewNamespace,
    IWbemCallResult **ppResult
    )
{
    HRESULT hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL CWbemNamespace::OpenNamespace\n"
        "   BSTR NsPath = %S\n"
        "   long lFlags = %d\n"
        "   IWbemContext* pCtx = 0x%X\n"
        "   IWbemServices **pNewContext = 0x%X\n",
        NsPath,
        lFlags,
        pCtx,
        ppNewNamespace
        ));

    // Parameter validation.
    // =====================

    try
    {
        if (NsPath == 0 || wcslen(NsPath) == 0 ||                        // SEC:REVIEWED 2002-03-22 : OK
            (ppNewNamespace == NULL && ppResult == NULL))
        {
            return WBEM_E_INVALID_PARAMETER;
        }
 
        if (ppNewNamespace == NULL && (lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
        {
            return WBEM_E_INVALID_PARAMETER;
        }
        if ((lFlags & WBEM_RETURN_IMMEDIATELY) && ppNewNamespace)
            return WBEM_E_INVALID_PARAMETER;

        if(ppNewNamespace)
            *ppNewNamespace = NULL;
        if(ppResult)
            *ppResult = NULL;

        if((lFlags & ~WBEM_RETURN_IMMEDIATELY & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS & ~WBEM_FLAG_CONNECT_REPOSITORY_ONLY) != 0)
            return WBEM_E_INVALID_PARAMETER;

        bool bWin9XLocalSecurity = false;

        // If here, we found the object, so we open the
        // corresponding namespace.
        // ============================================

        WString NewNs = m_pThisNamespace;
        NewNs += L"\\";
        NewNs += NsPath;

        CCallResult* pResult = new CCallResult;
        if(pResult == NULL) return WBEM_E_OUT_OF_MEMORY;
        CReleaseMe rmResult(pResult);

        // Schedule a request and wait
        // ===========================

        bool bForClient = m_bForClient ? true : false;

        wmilib::auto_ptr<CAsyncReq_OpenNamespace> pReq;
        pReq.reset( new CAsyncReq_OpenNamespace(this, 
                                               NewNs,
                                               (bWin9XLocalSecurity) ? SecFlagWin9XLocal : lFlags & WBEM_FLAG_CONNECT_REPOSITORY_ONLY,
                                               (bWin9XLocalSecurity) ? m_dwPermission : 0,
                                               pCtx, pResult, bForClient));

        if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;

        hRes = ConfigMgr::EnqueueRequest(pReq.get());
        if (FAILED(hRes)) return hRes;
        pReq.release(); // ownership transferred to queue

        if (ppResult)
        {
            *ppResult = pResult;
            pResult->AddRef();
        }

        if ((lFlags & WBEM_RETURN_IMMEDIATELY) == 0)
        {
            hRes = pResult->GetResultServices(INFINITE, ppNewNamespace);
        }

        return hRes;
    }
    catch(...)
    {
        ExceptionCounter c;
        return WBEM_E_FAILED;
    }
    
}



//***************************************************************************
//
//***************************************************************************
//

HRESULT CWbemNamespace::DeleteObject(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{


    HRESULT hRes;
    ULONGLONG uInf;

    // Parse the path and determine if a class or instance.

    IWbemPath *pPath = ConfigMgr::GetNewPath();
    CReleaseMe _(pPath);

    hRes = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, strObjectPath);
    if (FAILED(hRes))
        return hRes;

    hRes = pPath->GetInfo(0, &uInf);
    if (FAILED(hRes))
        return hRes;

    if (uInf & WBEMPATH_INFO_IS_CLASS_REF)
        return DeleteClass(strObjectPath, lFlags, pCtx, ppCallResult);
    else if (uInf & WBEMPATH_INFO_IS_INST_REF)
        return DeleteInstance(strObjectPath, lFlags, pCtx, ppCallResult);
    else
        return WBEM_E_INVALID_PARAMETER;

}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::ExecSyncQuery(
    IN  LPWSTR pszQuery,
    IN  IWbemContext *pCtx,
    IN  LONG lFlags,
    OUT CFlexArray &aDest
    )
{
    HRESULT hRes;
    CSynchronousSink* pSink = CSynchronousSink::Create();
    if (NULL == pSink) return WBEM_E_OUT_OF_MEMORY;
    pSink->AddRef();
    CReleaseMe _1(pSink);

    hRes = CQueryEngine::ExecQuery(this, L"WQL", pszQuery, lFlags, pCtx, pSink);
    if (FAILED(hRes)) return hRes;
    pSink->Block();
    pSink->GetStatus(&hRes, NULL, NULL);
    if (FAILED(hRes)) return hRes;    

    aDest.Bind(pSink->GetObjects().GetArray());

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CWbemNamespace::MapAssocRefsToClasses
//
//  Analyzes the association and determines which reference properties
//  point to which endpoints.  <pszAssocRef1> is the ref property
//  which can point to instances of <pClsDef1> and <pszAssocRef2> is
//  the property which can point to instances of <pClsDef2>.
//
//***************************************************************************
//
HRESULT CWbemNamespace::MapAssocRefsToClasses(
    IN  IWbemClassObject *pAssocClass,
    IN  IWbemClassObject *pClsDef1,
    IN  IWbemClassObject *pClsDef2,
    wmilib::auto_buffer<WCHAR> & pszAssocRef1,
    wmilib::auto_buffer<WCHAR> & pszAssocRef2)
{
    HRESULT hRes;

    // Note 97: Not valid for ternary assoc types
    // or derived types.
    // ===========================================

    // For each ref property, see if it can point to one of the endpoints.
    // ===================================================================

    pAssocClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY);

    while (1)
    {
        BSTR strPropName = 0;
        hRes = pAssocClass->Next(0,&strPropName,0,0,0);
        CSysFreeMe _1(strPropName);

        if (hRes == WBEM_S_NO_MORE_DATA) break;


        hRes = CAssocQuery::RoleTest(pClsDef1, pAssocClass, this, strPropName, ROLETEST_MODE_CIMREF_TYPE);
        if (SUCCEEDED(hRes))
        {
            pszAssocRef1.reset(Macro_CloneLPWSTR(strPropName));
            if (NULL == pszAssocRef1.get()) return WBEM_E_OUT_OF_MEMORY;
            continue;
        }

        hRes = CAssocQuery::RoleTest(pClsDef2, pAssocClass, this, strPropName, ROLETEST_MODE_CIMREF_TYPE);
        if (SUCCEEDED(hRes))
        {
            pszAssocRef2.reset(Macro_CloneLPWSTR(strPropName));
            if (NULL == pszAssocRef2.get()) return WBEM_E_OUT_OF_MEMORY;            
            continue;
        }
    }   // Enum of ref properties


    pAssocClass->EndEnumeration();

    if (NULL == pszAssocRef1.get() ||NULL == pszAssocRef2.get())
    {
        pszAssocRef1.reset(NULL);
        pszAssocRef2.reset(NULL);
        return WBEM_E_FAILED;
    }

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::BuildAssocTriads(
    IN  IWbemClassObject *pAssocClass,              // Assoc class
    IN  IWbemClassObject *pClsDef1,                 // Class for EP1
    IN  IWbemClassObject *pClsDef2,                 // Class for EP2
    IN  LPWSTR pszJoinProp1,                        // Matching prop in EP1
    IN  LPWSTR pszJoinProp2,                        // Matching prop in EP2
    IN  LPWSTR pszAssocRef1,                        // Prop which points to EP1
    IN  LPWSTR pszAssocRef2,                        // Prop which points to EP2
    IN  CFlexArray &aEp1,                           // EP1 instances
    IN  CFlexArray &aEp2,                           // EP2 instances
    IN OUT CFlexArray &aTriads                      // OUT : Triad list
    )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (pszJoinProp1 == 0 || pszJoinProp2 == 0 || pAssocClass == 0 ||
        pszAssocRef1 == 0 || pszAssocRef2 == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Do the matching.
    // ================

    for (int i = 0; i < aEp1.Size(); i++)
    {
        IWbemClassObject *pObj1 = (IWbemClassObject *) aEp1[i];
        CVARIANT v1;
        if (FAILED(hRes = pObj1->Get(pszJoinProp1, 0, &v1, 0, 0))) return hRes;

        for (int i2 = 0; i2 < aEp2.Size(); i2++)
        {
            BOOL bMatch = FALSE;

            IWbemClassObject *pObj2 = (IWbemClassObject *) aEp2[i2];
            CVARIANT v2;
            pObj2->Get(pszJoinProp2, 0, &v2, 0, 0);

            if (V_VT(&v1) == VT_I4 && V_VT(&v2) == VT_I4)
            {
                if (v1.GetLONG() == v2.GetLONG())
                {
                    bMatch = TRUE;
                }
            }
            else if (V_VT(&v1) == VT_BSTR && V_VT(&v2) == VT_BSTR)
            {
                if (wbem_wcsicmp(v1.GetStr(), v2.GetStr()) == 0)
                {
                    bMatch = TRUE;
                }
            }

            // If a match, spawn the association and bind it.
            // ==============================================

            if (bMatch)
            {
                IWbemClassObject *pAssocInst = 0;
                if (FAILED(hRes = pAssocClass->SpawnInstance(0, &pAssocInst))) return hRes;
                CReleaseMe rmAssoc(pAssocInst);

                CVARIANT vPath1, vPath2;

                if (FAILED(hRes = pObj1->Get(L"__RELPATH", 0, &vPath1, 0, 0))) return hRes;
                if (FAILED(hRes = pObj2->Get(L"__RELPATH", 0, &vPath2, 0, 0))) return hRes;

                if (FAILED(hRes = pAssocInst->Put(pszAssocRef1, 0, &vPath1, 0))) return hRes;
                if (FAILED(hRes = pAssocInst->Put(pszAssocRef2, 0, &vPath2, 0))) return hRes;

                wmilib::auto_ptr<SAssocTriad> pTriad( new SAssocTriad);
                if (NULL == pTriad.get())  return WBEM_E_OUT_OF_MEMORY;

                pTriad->m_pEp1 = pObj1;
                pTriad->m_pEp1->AddRef();
                pTriad->m_pEp2 = pObj2;
                pTriad->m_pEp2->AddRef();
                pTriad->m_pAssoc = pAssocInst;
                pTriad->m_pAssoc->AddRef();                

                if (CFlexArray::no_error != aTriads.Add(pTriad.get())) return WBEM_E_OUT_OF_MEMORY;
                pTriad.release();
            }
        }
    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemNamespace::ExtractEpInfoFromQuery(
    IN IWbemQuery *pQuery,
    wmilib::auto_buffer<WCHAR> & pszRetClass1,
    wmilib::auto_buffer<WCHAR> & pszRetProp1,
    wmilib::auto_buffer<WCHAR> & pszRetClass2,
    wmilib::auto_buffer<WCHAR> & pszRetProp2
    )
{
    HRESULT hRes;
    SWQLNode *pRoot;

    hRes = pQuery->GetAnalysis(
        WMIQ_ANALYSIS_RESERVED,
        0,
        (LPVOID *) &pRoot
        );

    if (FAILED(hRes))
        return hRes;

    // Move down the parse tree to find the JOIN clause.
    // =================================================

    if (!pRoot || pRoot->m_dwNodeType != TYPE_SWQLNode_QueryRoot)
        return WBEM_E_INVALID_QUERY;
    pRoot = pRoot->m_pLeft;
    if (!pRoot || pRoot->m_dwNodeType != TYPE_SWQLNode_Select)
        return WBEM_E_INVALID_QUERY;
    pRoot = pRoot->m_pLeft;
    if (!pRoot || pRoot->m_dwNodeType != TYPE_SWQLNode_TableRefs)
        return WBEM_E_INVALID_QUERY;
    pRoot = pRoot->m_pRight;
    if (!pRoot || pRoot->m_dwNodeType != TYPE_SWQLNode_FromClause)
        return WBEM_E_INVALID_QUERY;
    pRoot = pRoot->m_pLeft;
    if (!pRoot || pRoot->m_dwNodeType != TYPE_SWQLNode_Join)
        return WBEM_E_INVALID_QUERY;

    // We are now at the JOIN node.
    // ============================

    SWQLNode_Join *pJoin = (SWQLNode_Join *) pRoot;

    /* The parse tree is left-heavy and looks like this:

             JN               Join node
            /   \             /        \
          JP     OC       JoinPair     OnClause
         /  \               /   \
        TR   TR        TableRef TableRef
    */

    // First, get the first table & prop.
    // ==================================
    SWQLNode_JoinPair *pPair = (SWQLNode_JoinPair *) pJoin->m_pLeft;
    if (!pPair || pPair->m_dwNodeType != TYPE_SWQLNode_JoinPair)
        return WBEM_E_INVALID_QUERY;

    SWQLNode_TableRef *pT1 = (SWQLNode_TableRef *) pPair->m_pLeft;
    SWQLNode_TableRef *pT2 = (SWQLNode_TableRef *) pPair->m_pRight;

    if (!pT1 || !pT2)
        return WBEM_E_INVALID_QUERY;

    SWQLNode_OnClause *pOC = (SWQLNode_OnClause *) pJoin->m_pRight;
    if (!pOC)
        return WBEM_E_INVALID_QUERY;

    SWQLNode_RelExpr *pRE = (SWQLNode_RelExpr *) pOC->m_pLeft;
    if (!pRE)
        return WBEM_E_INVALID_QUERY;

    if (pRE->m_dwExprType != WQL_TOK_TYPED_EXPR)
        return WBEM_E_INVALID_QUERY;

    // We now have the table names available and the matching condition.
    // ==================================================================

    LPWSTR pszClass = pRE->m_pTypedExpr->m_pTableRef;
    LPWSTR pszProp = pRE->m_pTypedExpr->m_pColRef;
    LPWSTR pszClass2 = pRE->m_pTypedExpr->m_pJoinTableRef;
    LPWSTR pszProp2 = pRE->m_pTypedExpr->m_pJoinColRef;

    if (wbem_wcsicmp(pT1->m_pTableName, pszClass) != 0)
        pszClass = pT1->m_pAlias;

    if (wbem_wcsicmp(pT2->m_pTableName, pszClass2) != 0)
        pszClass2 = pT2->m_pAlias;

    if (pszClass == 0 || pszProp == 0 || pszClass2 == 0 || pszProp2 == 0)
        return WBEM_E_INVALID_QUERY;


    pszRetClass1.reset(Macro_CloneLPWSTR(pszClass));
    if (NULL == pszRetClass1.get()) return WBEM_E_OUT_OF_MEMORY;
    pszRetProp1.reset(Macro_CloneLPWSTR(pszProp));
    if (NULL == pszRetProp1.get()) return WBEM_E_OUT_OF_MEMORY;    
    pszRetClass2.reset(Macro_CloneLPWSTR(pszClass2));
    if (NULL == pszRetClass2.get()) return WBEM_E_OUT_OF_MEMORY;        
    pszRetProp2.reset(Macro_CloneLPWSTR(pszProp2));
    if (NULL ==pszRetProp2.get()) return WBEM_E_OUT_OF_MEMORY;    

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CWbemNamespace::BuildRuleBasedPathToInst
//
//  Based on the incoming info, computes the path to the would-be other
//  endpoint.
//
//  <pEp>           The known endpoint.
//  <pszJoinProp1>  The property in <pEp> which matches the property in the
//                  unknown EP.
//  <pEp2>          Class for the other endpoint.
//  <pszJoinProp2>  The property in the other class which matches the
//                  <pszJoinProp1> in the known endpoint class.
//  <wsNewPath>     The proposed path to the instance of class <pEp2>. Who
//                  knows whether or not we will find it, but we can try.
//
//***************************************************************************
//
HRESULT CWbemNamespace::BuildRuleBasedPathToInst(
    IWbemClassObject *pEp,
    LPWSTR pszJoinProp1,
    IWbemClassObject *pEp2,
    LPWSTR pszJoinProp2,
    OUT WString &wsNewPath
    )
{
    HRESULT hRes;

    // Get the property from the <pEp> which is the cause of all the fuss.
    // ===================================================================

    _variant_t vProp;
    hRes = pEp->Get(pszJoinProp1, 0, &vProp, 0, 0);
    if (FAILED(hRes))
        return hRes;

    _variant_t vClass2;
    hRes = pEp2->Get(L"__CLASS", 0, &vClass2, 0, 0);
    if (FAILED(hRes))  return hRes;    
    if (VT_BSTR != V_VT(&vClass2)) return WBEM_E_INVALID_PARAMETER;

    wsNewPath = V_BSTR(&vClass2);
    wsNewPath += L".";
    wsNewPath += pszJoinProp2;
    wsNewPath += L"=";

    // Note 98: Doesn't work for compound keys!! Yuck.
    // ===============================================

    _variant_t vDest;
    hRes = VariantChangeType(&vDest, &vProp, 0, VT_BSTR);
    if (FAILED(hRes)) return hRes;

    wsNewPath += V_BSTR(&vDest);
    
    return hRes;
}

//***************************************************************************
//
//  CWbemNamespace::ManufactureAssocs
//
//  Manufactures the associations based on the rule in the <pszJoinQuery>
//  which was extracted from the <rulebased> qualifier.  Queries the two
//  endpoint classes and joins the instances to produce the associations
//
//  <pAssocClass>   The association class definition which contains the rule.
//  <pEp>           Optional endpoint object.  If not NULL, only objects
//                  which associate to this endpoint will be returned in
//                  the triad list, typically a single object.
//  <pCtx>          Call context
//  <pszJoinQuery>  The rule query text
//  <aTriads>       Receives the output, an array of SAssocTriad pointers.
//                  Caller must call SAssocTriad::ArrayCleanup.
//
//***************************************************************************

HRESULT CWbemNamespace::ManufactureAssocs(
    IN  IWbemClassObject *pAssocClass,
    IN  IWbemClassObject *pEp,          // Optional
    IN  IWbemContext *pCtx,
    IN  LPWSTR pszJoinQuery,
    OUT CFlexArray &aTriads
    )
{
    HRESULT hRes;
    WString q1, q2;

    CFlexArray aEp1List;
    OnDelete<CFlexArray &,void(*)(CFlexArray &),EmptyObjectList> EmptyMe1(aEp1List);
    CFlexArray aEp2List;
    OnDelete<CFlexArray &,void(*)(CFlexArray  &),EmptyObjectList> EmptyMe2(aEp2List);
    wmilib::auto_buffer<WCHAR> pClassName1;
    wmilib::auto_buffer<WCHAR> pClassName2;
    wmilib::auto_buffer<WCHAR> pszJoinProp1;
    wmilib::auto_buffer<WCHAR> pszJoinProp2;
    wmilib::auto_buffer<WCHAR> pszAssocRef1;
    wmilib::auto_buffer<WCHAR> pszAssocRef2;
    _IWmiQuery *pQuery = 0;

    // Parse the query.
    // ================
    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if (NULL == pSvc) return WBEM_E_OUT_OF_MEMORY;
    CReleaseMe rmSvc(pSvc);

    hRes = pSvc->CreateQueryParser(0, &pQuery);
    if (FAILED(hRes)) return hRes;
    CReleaseMe rmQuery(pQuery);

    hRes = pQuery->Parse(L"SQL", pszJoinQuery, 0);
    if (FAILED(hRes)) return hRes;

    // Extract the endpoint class names.
    // ==================================

    hRes = ExtractEpInfoFromQuery(pQuery, pClassName1,pszJoinProp1, pClassName2, pszJoinProp2);
    if (FAILED(hRes)) return hRes;

    // Get the endpoint class defs.
    // ============================
    IWbemClassObject *pClsDef1 = 0;
    hRes = InternalGetClass(pClassName1.get(), &pClsDef1);
    if (FAILED(hRes)) return hRes;
    CReleaseMe rmCls1(pClsDef1);

    IWbemClassObject *pClsDef2 = 0;
    hRes = InternalGetClass(pClassName2.get(), &pClsDef2);
    if (FAILED(hRes)) return hRes;
    CReleaseMe rmCls2(pClsDef2);

    // Map which assoc ref properties point to which class.
    // ====================================================
    hRes = MapAssocRefsToClasses(pAssocClass, pClsDef1, pClsDef2, pszAssocRef1, pszAssocRef2);
    if (FAILED(hRes)) return hRes;

    // If no specific endpoint, an enumeration is requested.  We query the endpoint
    // classes completely and match everything up.
    // ============================================================================

    IWbemClassObject *pEp2 = 0;
    CReleaseMeRef<IWbemClassObject *> rmRefEp2(pEp2);

    if (pEp == 0)
    {
        // Build the queries.
        // ===================
        q1 = "select * from ";
        q1 += pClassName1.get();
        q2 = "select * from ";
        q2 += pClassName2.get();

        hRes = ExecSyncQuery(q1, pCtx, 0, aEp1List);
        if (FAILED(hRes)) return hRes;

        hRes = ExecSyncQuery(q2, pCtx, 0, aEp2List);
        if (FAILED(hRes)) return hRes;
    }
    else
    {
        // Note 99: Oversimplified in that it doesn't do an enum; assumes a 1:1 mapping.
        // Compute the path to the other endpoint based on the rule.
        // =============================================================================

        WString wsNewPath;
        hRes = BuildRuleBasedPathToInst(pEp, pszJoinProp1.get(), pClsDef2, pszJoinProp2.get(), wsNewPath); // throws
        if (FAILED(hRes)) return hRes;

        // Do a get object.
        // ================

        hRes = InternalGetInstance(wsNewPath, &pEp2);
        if (FAILED(hRes)) return hRes;

        if (CFlexArray::no_error != aEp1List.Add(pEp)) return WBEM_E_OUT_OF_MEMORY;
        pEp->AddRef();
        if (CFlexArray::no_error != aEp2List.Add(pEp2)) return WBEM_E_OUT_OF_MEMORY;
        pEp2->AddRef();
    }

    // Now, match up the results.
    // For single-object type scenarios, the arrays simply have one element in them. Ho hum.
    // =====================================================================================

    OnDeleteIf<CFlexArray &,void(*)(CFlexArray &),EmptyObjectList> EmptyMeTriads(aTriads);

    hRes = BuildAssocTriads(
        pAssocClass,
        pClsDef1,
        pClsDef2,
        pszJoinProp1.get(),
        pszJoinProp2.get(),
        pszAssocRef1.get(),
        pszAssocRef2.get(),
        aEp1List,
        aEp2List,
        aTriads                         // OUT
        );

    if (FAILED(hRes)) return hRes;

    EmptyMeTriads.dismiss();
    return hRes;
}



//***************************************************************************
//
//  CWbemNamespace::GetAceList
//
//  Retrieves the ACEs associated with this namespace
//
//  <ppAceList>     Flexarray to hold ACE list
//
//***************************************************************************
HRESULT CWbemNamespace::GetAceList(CFlexAceArray** ppAceList)
{
    HRESULT hRes=S_OK;

    *ppAceList = new CFlexAceArray;
    if (ppAceList==NULL)
        hRes = WBEM_E_OUT_OF_MEMORY;
    else
    {
        // 1. Get security descriptor
        CNtSecurityDescriptor& sd = GetSDRef();

        // 2. Get the DACL
        CNtAcl* pDacl;
        pDacl = sd.GetDacl();
        if ( pDacl==NULL )
            return WBEM_E_OUT_OF_MEMORY;

        CDeleteMe<CNtAcl> dm(pDacl);

        // 3. Loop through DACL
        int iNumAces = pDacl->GetNumAces();
        for ( int i=0; i<iNumAces; i++ )
        {
            CNtAce* Ace;
            Ace = pDacl->GetAce(i);
            if ( Ace == NULL )
                return WBEM_E_OUT_OF_MEMORY;

            (*ppAceList)->Add (Ace);
        }
    }
    return hRes;
}



//***************************************************************************
//
//  CWbemNamespace::PutAceList
//
//  Puts the ACEs associated with this namespace
//
//  <ppAceList>     Flexarray ACE list
//
//***************************************************************************
HRESULT CWbemNamespace::PutAceList(CFlexAceArray* pFlex)
{
    SCODE sc = S_OK;

    CNtAcl DestAcl;

    int iNumAces = pFlex->Size();
    for (int i=0; i<iNumAces; i++ )
    {
        if ( DestAcl.AddAce ((CNtAce*) pFlex->GetAt(i)) == FALSE )
        {
            return WBEM_E_INVALID_OBJECT;
        }
    }
    if ( m_sd.SetDacl (&DestAcl) == FALSE )
        return WBEM_E_INVALID_OBJECT;

    sc = StoreSDIntoNamespace(m_pSession, m_pNsHandle, m_sd);
    if ( !FAILED (sc) )
        sc = RecursiveSDMerge();
    return sc;
}

//***************************************************************************
//
//  CWbemNamespace::GetDynamicReferenceClasses
//
//  Asks the provider subsystem for dynamic association classes.
//
//
//***************************************************************************
HRESULT CWbemNamespace::GetDynamicReferenceClasses( long lFlags, IWbemContext
* pCtx, IWbemObjectSink* pSink )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    _IWmiProviderAssociatorsHelper* pAssocHelp = NULL;

    if ( m_pProvFact)
    {
        hRes = m_pProvFact->GetClassProvider(
                    0,                  // lFlags
                    pCtx,
                    m_wszUserName,
                    m_wsLocale,
                    m_pThisNamespace,                   
                    0,
                    IID__IWmiProviderAssociatorsHelper,
                    (LPVOID *) &pAssocHelp
                    );

        CReleaseMe  rm( pAssocHelp );

        if ( SUCCEEDED( hRes ) )
        {
            hRes = pAssocHelp->GetReferencesClasses( lFlags, pCtx, pSink );
        }

        if ( FAILED( hRes ) )
        {
            pSink->SetStatus( 0L, hRes, 0L, 0L );
        }
    }
    else
    {
        pSink->SetStatus( 0L, WBEM_S_NO_ERROR, 0L, 0L );
    }

    return hRes;

}


