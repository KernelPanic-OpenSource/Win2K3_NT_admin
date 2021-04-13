/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    SECURE.CPP

Abstract:

    Contains various routines used for ACL based security.
    It is defined in secure.h

History:

    a-davj    05-NOV-98  Created.

--*/

#include "precomp.h"
#include <wbemcore.h>
#include <oleauto.h>
#include <genutils.h>
#include <safearry.h>
#include <oahelp.inl>
#include <fcntl.h>
#define WBEM_WMISETUP       __TEXT("WMISetup")

// /////////////////////////////////////////////////

AutoRevertSecTlsFlag::AutoRevertSecTlsFlag ( LPVOID dir )
{
    m_bDir = dir ;
    TlsSetValue ( CCoreQueue::GetSecFlagTlsIndex(), (LPVOID)dir );
}

AutoRevertSecTlsFlag::AutoRevertSecTlsFlag()
{
    m_bDir = TlsGetValue ( CCoreQueue::GetSecFlagTlsIndex() ) ;
}

AutoRevertSecTlsFlag::~AutoRevertSecTlsFlag()
{
    TlsSetValue ( CCoreQueue::GetSecFlagTlsIndex(), (LPVOID)1 ) ;
}

VOID AutoRevertSecTlsFlag::SetSecTlsFlag ( LPVOID dir )
{
    TlsSetValue ( CCoreQueue::GetSecFlagTlsIndex(), (LPVOID)dir );
}


//***************************************************************************
//
//  SetOwnerAndGroup
//
//  Sets the owner and group of the SD to the Admininstrators group
//
//***************************************************************************

BOOL SetOwnerAndGroup(CNtSecurityDescriptor &sd)
{
    PSID pRawSid;
    BOOL bRet = FALSE;

    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
    if(AllocateAndInitializeSid( &id, 2,            // SEC:REVIEWED 2002-03-22 : OK
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidAdmins(pRawSid);
        FreeSid(pRawSid);
        if (CNtSid::NoError != SidAdmins.GetStatus()) return FALSE;
            
        bRet = sd.SetGroup(&SidAdmins);      // Access check doesnt really care what you put, so long as you
                                      // put something for the owner
        if(bRet)
            bRet = sd.SetOwner(&SidAdmins);

        return bRet;
    }
    return bRet;
}



//***************************************************************************
//
//  CFlexAceArray::~CFlexAceArray()
//
//  Cleans up safe array entries.
//
//***************************************************************************

CFlexAceArray::~CFlexAceArray()
{
    for(int iCnt = 0; iCnt < Size(); iCnt++)
    {
        CBaseAce * pace = (CBaseAce *)GetAt(iCnt);
        if(pace)
            delete pace;
    }
    Empty();
}

//***************************************************************************
//
//  SetStatusAndReturnOK
//
//  If there is an error, it dumps an error message.  It also sets the status
//
//***************************************************************************

HRESULT SetStatusAndReturnOK(SCODE sc, IWbemObjectSink* pSink, char * pMsg)
{
    if(sc != S_OK && pMsg)
        ERRORTRACE((LOG_WBEMCORE, "SecurityMethod failed doing %s, sc = 0x%x", pMsg, sc));
    pSink->SetStatus(0,sc, NULL, NULL);
    return S_OK;
}

//***************************************************************************
//
//  DumpErrorMsgAndReturn
//
//  Dumps out an error message
//
//***************************************************************************

HRESULT DumpErrorMsgAndReturn(SCODE sc, char * pMsg)
{
    if(pMsg)
        ERRORTRACE((LOG_WBEMCORE, "%s, sc = 0x%x", pMsg, sc));
    return sc;
}

//***************************************************************************
//
//  CWbemNamespace::GetSDMethod
//
//  Implements the GetSD method.  This method returns the security descriptor
//
//***************************************************************************

HRESULT CWbemNamespace::GetSDMethod(IWbemClassObject* pOutParams)
{
    // Load up the return object with the security descriptor

    SCODE sc = EnsureSecurity();
    if(sc != S_OK)
        return DumpErrorMsgAndReturn(sc, "GetSDMethod failed creating a SD");

    CNtSecurityDescriptor &sd = GetSDRef();

    sc = CopySDIntoProperty(L"SD", sd, pOutParams);
    return sc;

}

//***************************************************************************
//
//  SetSDMethod
//
//  Implements the SetSD method.  This method sets the security descriptor
//
//***************************************************************************
HRESULT CWbemNamespace::RecursiveSDMerge()
{
    // Enumerate the child namespaces

    CSynchronousSink* pSyncSink = CSynchronousSink::Create();
    if(pSyncSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pSyncSink->AddRef();
    CReleaseMe rm4(pSyncSink);

    HRESULT hres = CRepository::ExecQuery ( m_pSession, m_pNsHandle, L"select * from __Namespace", pSyncSink, WBEM_FLAG_DEEP );
    if(FAILED(hres))
        return hres;

    pSyncSink->Block();

    // For each child

    for(int i = 0; i < pSyncSink->GetObjects().GetSize(); i++)
    {

        // Get the child namespace

        CWbemNamespace* pNewNs = CWbemNamespace::CreateInstance();

        if (pNewNs == NULL)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        CReleaseMe rm2((IWbemServices *)pNewNs);

        VARIANT var;
        VariantInit(&var);
        CWbemObject* pObj = (CWbemObject*)pSyncSink->GetObjects().GetAt(i);
        hres = pObj->Get(L"name", 0, &var, NULL, NULL);
        if(SUCCEEDED(hres) && var.vt == VT_BSTR && var.bstrVal && m_pThisNamespace)
        {
            CClearMe cm(&var);
            DWORD dwLen = wcslen(m_pThisNamespace) + wcslen(var.bstrVal) + 2;    // SEC:REVIEWED 2002-03-22 : OK, Nulls are provably there
            WCHAR * pNewName = new WCHAR[dwLen];
            if(pNewName == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            CDeleteMe<WCHAR> dm(pNewName);
            StringCchCopyW(pNewName, dwLen, m_pThisNamespace);
            StringCchCatW(pNewName, dwLen, L"\\");
            StringCchCatW(pNewName, dwLen, var.bstrVal);

            hres = pNewNs->Initialize(pNewName,
                                NULL,
                                m_dwSecurityFlags, m_dwPermission, m_bForClient, FALSE,
                                NULL, 0xFFFFFFFF, FALSE, NULL);
            if(FAILED(hres))
                return hres;
            // Merge parents SD into the child

            if(pNewNs->IsNamespaceSDProtected())
                continue;

            hres = SetSecurityForNS(pNewNs->m_pSession, pNewNs->m_pNsHandle, m_pSession, m_pNsHandle, TRUE);
            if(FAILED(hres))
                return hres;
            // Recursively call the child
            
            hres = pNewNs->RecursiveSDMerge();
            if (FAILED(hres))
                return hres;
        }
    }
    return S_OK;
}

BOOL IsProtected(CNtSecurityDescriptor & sd)
{
    PSECURITY_DESCRIPTOR pActual = sd.GetPtr();
    if(pActual == NULL)
        return FALSE;

    SECURITY_DESCRIPTOR_CONTROL Control;
    DWORD dwRevision;
    BOOL bRet = GetSecurityDescriptorControl(pActual, &Control, &dwRevision);
    if(bRet == FALSE)
        return FALSE;

    if(Control & SE_DACL_PROTECTED)
        return TRUE;
    else
        return FALSE;

}

BOOL CWbemNamespace::IsNamespaceSDProtected()
{
    // Get the SD for this namespace

    HRESULT hRes = EnsureSecurity();
    if(FAILED(hRes))
        return FALSE;

    // check the control flag

    return IsProtected(m_sd);

}


HRESULT StripOutInheritedAces(CNtSecurityDescriptor &sd)
{

    // Get the DACL

    CNtAcl * DestAcl;
    DestAcl = sd.GetDacl();
    if(DestAcl == FALSE)
        return WBEM_E_INVALID_PARAMETER;
    CDeleteMe<CNtAcl> dm(DestAcl);

    // enumerate through the aces

    DWORD dwNumAces = DestAcl->GetNumAces();
    BOOL bChanged = FALSE;
    for(long nIndex = (long)dwNumAces-1; nIndex >= 0; nIndex--)
    {
        CNtAce *pAce = DestAcl->GetAce(nIndex);
        if(pAce && CNtAce::NoError == pAce->GetStatus())
        {
            long lFlags = pAce->GetFlags();
            if(lFlags & INHERITED_ACE)
            {
                DestAcl->DeleteAce(nIndex);
                bChanged = TRUE;
                delete pAce;
            }
        }
    }
    if(bChanged)
        sd.SetDacl(DestAcl);
    return S_OK;
}


HRESULT CWbemNamespace::GetParentsInheritableAces(CNtSecurityDescriptor &sd)
{
    // Get the parent namespace's SD

    if(m_pThisNamespace == NULL)
        return WBEM_E_CRITICAL_ERROR;

    // Start by figuring out what the parents name is.  Do this by copying the namespace name,
    // then nulling out the last back slash.

    int iLen = wcslen(m_pThisNamespace);   // SEC:REVIEWED 2002-03-22 : OK, Null is provably there
    WCHAR * pParentName = new WCHAR[iLen + 1];
    if(pParentName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CDeleteMe<WCHAR> dm(pParentName);
    StringCchCopyW(pParentName, iLen + 1, m_pThisNamespace);

    BOOL bFoundBackSlash = FALSE;
    WCHAR * pTest = pParentName+iLen-1;

    for (; pTest >= pParentName; pTest--)
    {
        if ( *pTest == '\\' || *pTest == '/' )
        {
            bFoundBackSlash = TRUE;
            *pTest = 0;
            break;
        }
    }
    if(!bFoundBackSlash)
        return S_OK;        // probably already in root

    // Open the parent namespace

    CWbemNamespace* pNewNs = CWbemNamespace::CreateInstance();

    if (pNewNs == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    IUnknown * pUnk = NULL;
    HRESULT hres = pNewNs->QueryInterface(IID_IUnknown, (void **)&pUnk);
    if(FAILED(hres))
        return hres;
    pNewNs->Release();      // ref count held by pUnk
    CReleaseMe rm2(pUnk);

    hres = pNewNs->Initialize(pParentName,
                            NULL,
                            m_dwSecurityFlags, m_dwPermission, m_bForClient, FALSE,
                            NULL, 0xFFFFFFFF, FALSE, NULL);
    if(FAILED(hres))
        return hres;

    hres = pNewNs->EnsureSecurity();
    if(FAILED(hres))
        return FALSE;

    // Go through the parents dacl and add and inheritiable aces to ours.

    hres = CopyInheritAces(sd, pNewNs->m_sd);
    return hres;
}


HRESULT CWbemNamespace::SetSDMethod(IWbemClassObject* pInParams)
{

    // Make sure that there is an input argument

    if(pInParams == NULL)
        return DumpErrorMsgAndReturn(WBEM_E_INVALID_PARAMETER, "SetSD failed due to null pInParams");

    // Get the security descriptor argument

    CNtSecurityDescriptor sd;
    HRESULT hr = GetSDFromProperty(L"SD", sd, pInParams);
    if(FAILED(hr))
        return hr;

    // Check to make sure the SD is valid before attempting to store it
    // CNtSecurityDescriptor does this via IsValidSecurityDescriptor so
    // all we need to do is check the status of sd before continuing.
    // NT RAID#:  152990        [marioh]
    if ( sd.GetStatus() != CNtSecurityDescriptor::NoError )
        return WBEM_E_INVALID_OBJECT;
    

    //
    // Reject SecurityDescriptors with NULL Owner or NULL group
    //
    // This is temporarily removed since _SOMEONE_ decided we need
    // to RI yesterday and test wasnt quite done with smoking this.
    //
    CNtSid *pTmpSid = sd.GetOwner ( ) ;
    CNtSid *pTmpSid2 = sd.GetGroup ( ) ;
    CDeleteMe<CNtSid> owner ( pTmpSid ) ;
    CDeleteMe<CNtSid> group ( pTmpSid2 ) ;

    if ( pTmpSid == NULL || pTmpSid2 == NULL )
    {
        return WBEM_E_FAILED ;
    }
    if (CNtSid::NoError != pTmpSid->GetStatus() || CNtSid::NoError != pTmpSid2->GetStatus())
    {
        return WBEM_E_FAILED;
    }
            
    // Some editors return inherited aces, and others dont.  Strip the inherited ones so
    // that we have a consistent SD.

    StripOutInheritedAces(sd);

    //
    // Make sure to order the ACEs that are in the pInparams.
    // NT Bug: 515545    [marioh]
    //
    CNtAcl* pAcl = sd.GetDacl ( ) ;
    CDeleteMe <CNtAcl> dacl ( pAcl ) ;
    CNtAcl* pOrderedAcl = NULL ;
    CDeleteMe <CNtAcl> ordDacl ( pOrderedAcl ) ;

    if ( pAcl != NULL )
    {
        //
        // Order the DACL.
        //
        pOrderedAcl = pAcl->OrderAces ( ) ;
        if ( pOrderedAcl == NULL )
        {
            return WBEM_E_FAILED ;
        }

        //
        // Now, set the DACL to the newly ordered one.
        //
        if ( sd.SetDacl ( pOrderedAcl ) == FALSE )
        {
            return WBEM_E_FAILED ;
        }
    }

    // Get the inherited aces from the parent

    if(!IsProtected(sd))
        GetParentsInheritableAces(sd);

    // Store the sd.
    hr = StoreSDIntoNamespace(m_pSession, m_pNsHandle, sd);
    if(FAILED(hr))
        return hr;

    hr = RecursiveSDMerge();    
    return hr;
}

//***************************************************************************
//
//  IsAceValid()
//
//  Does a sanity check on aces
//
//***************************************************************************

bool IsAceValid(DWORD dwMask, DWORD dwType, DWORD dwFlag)
{
    bool bOK = true;
    if(dwMask & WBEM_FULL_WRITE_REP && ((dwMask & WBEM_PARTIAL_WRITE_REP) == 0 ||
        (dwMask & WBEM_WRITE_PROVIDER) == 0))
    {
        bOK = false;
        return false;
    }

    // DONT allow INHERIT_ONLY_ACE with out CONTAINER_INHERIT

    DWORD dwTemp = dwFlag & (INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE);
    if(dwTemp == INHERIT_ONLY_ACE)
        bOK = false;

    DWORD dwBadAccess = dwMask & ~(FULL_RIGHTS);
    DWORD dwBadFlag = dwFlag & ~(CONTAINER_INHERIT_ACE | NO_PROPAGATE_INHERIT_ACE |
                        INHERIT_ONLY_ACE | INHERITED_ACE);
    if(dwBadFlag || dwBadAccess)
        bOK = false;

    if((dwType != 0) && (dwType != 1))
        bOK = false;

    if(!bOK)
        ERRORTRACE((LOG_WBEMCORE, "Got passed a bad ace, dwMask=0x%x, dwType=0x%x, dwFlag=0x%x",
            dwMask, dwType, dwFlag));

    return bOK;
}


//***************************************************************************
//
//  GetCallerAccessRightsMethod
//
//  Implements the GetCallerAccessRights methods.  It returns the access rignts
//  of the current caller.
//
//***************************************************************************

HRESULT CWbemNamespace::GetCallerAccessRightsMethod(IWbemClassObject* pOutParams)
{
    VARIANT var;
    var.vt = VT_I4;

    var.lVal = m_dwPermission;
    //
    // Instead of using the 'saved' permission set from original namespace connect
    // we get the caller access rights every time. This is to avoid the scenario
    // whereby user A connects and then sets the proxyblanket to user B and makes
    // a call to GetCallerAccessRights. Without getting the access rights each time
    // we would end of with A's access rights.
    //
    var.lVal = GetUserAccess ( ) ;
    //var.lVal = m_dwPermission;

    SCODE sc = pOutParams->Put(L"rights" , 0, &var, 0);
    if(sc != S_OK)
        return DumpErrorMsgAndReturn(sc, "GetCallerAccessRights failed putting the dwAccesMask property");
    return S_OK;
}

//***************************************************************************
//
//  SecurityMethod
//
//  Implements the security methods.
//
//***************************************************************************

HRESULT CWbemNamespace::SecurityMethod(LPWSTR wszMethodName, long lFlags,
                       IWbemClassObject *pInParams, IWbemContext *pCtx,
                       IWbemObjectSink* pSink)
{
    SCODE sc;

    // Do some parameter checking

    if(pSink == NULL || wszMethodName == NULL)
        return WBEM_E_INVALID_PARAMETER;

    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutClass = NULL;
    IWbemClassObject* pOutParams = NULL;

    // Get the class object

    sc = GetObject(L"__SystemSecurity", 0, pCtx, &pClass, NULL);
    if(sc != S_OK || pClass == NULL)
        return SetStatusAndReturnOK(sc, pSink, "getting the class object");

    // All the methods return data, so create an instance of the
    // output argument class.

    sc = pClass->GetMethod(wszMethodName, 0, NULL, &pOutClass);
    pClass->Release();
    if(sc != S_OK)
        return SetStatusAndReturnOK(sc, pSink, "getting the method");

    sc = pOutClass->SpawnInstance(0, &pOutParams);
    pOutClass->Release();
    if(sc != S_OK || pOutParams == NULL)
        return SetStatusAndReturnOK(sc, pSink, "spawning an instance of the output class");

    CReleaseMe rm(pOutParams);

    // Depending on the actual method, call the appropritate routine

    if(!wbem_wcsicmp(wszMethodName, L"GetSD"))
    {
        if (!Allowed(READ_CONTROL))
            sc = WBEM_E_ACCESS_DENIED;
        else
            sc = GetSDMethod(pOutParams);
    }
    else if(!wbem_wcsicmp(wszMethodName, L"Get9XUserList"))
    {
        sc = WBEM_E_METHOD_DISABLED;
    }
    else if(!wbem_wcsicmp(wszMethodName, L"SetSD"))
    {
        if (!Allowed(WRITE_DAC))
            sc = WBEM_E_ACCESS_DENIED;
        else
            sc = SetSDMethod(pInParams);
    }
    else if(!wbem_wcsicmp(wszMethodName, L"Set9XUserList"))
    {
        sc = WBEM_E_METHOD_DISABLED;
    }
    else if(!wbem_wcsicmp(wszMethodName, L"GetCallerAccessRights"))
    {
        sc = GetCallerAccessRightsMethod(pOutParams);
    }
    else
    {
        return SetStatusAndReturnOK(WBEM_E_INVALID_PARAMETER, pSink, "Invalid method name");
    }
    if(sc != S_OK)
        return SetStatusAndReturnOK(sc, pSink, "calling method");

    // Set the return code

    VARIANT var;
    var.vt = VT_I4;
    var.lVal = 0;    // special name for return value.
    sc = pOutParams->Put(L"ReturnValue" , 0, &var, 0);
    if(sc != S_OK)
        return SetStatusAndReturnOK(sc, pSink, "setting the ReturnCode property");

    // Send the output object back to the client via the sink. Then
    // release the pointers and free the strings.

    sc = pSink->Indicate(1, &pOutParams);

    // all done now, set the status
    sc = pSink->SetStatus(0,WBEM_S_NO_ERROR,NULL,NULL);

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//  GetUserAccess
//
//  Determines the allowed access for a user.
//
//***************************************************************************

DWORD CWbemNamespace::GetUserAccess()
{
    DWORD dwRet = 0;
    if(IsInAdminGroup())
        return FULL_RIGHTS;

    if(S_OK !=EnsureSecurity())
        return dwRet;   // nothing!

    dwRet = GetNTUserAccess();

    if((dwRet & WBEM_REMOTE_ACCESS) == 0)
    {
        HANDLE hAccessToken;
        if(SUCCEEDED(GetAccessToken(hAccessToken)))
        {
            BOOL bRemote = IsRemote(hAccessToken);
            CloseHandle(hAccessToken);
            if(bRemote)
                dwRet = 0;
        }
    }
    if(m_pThisNamespace && (wbem_wcsicmp(L"root\\security", m_pThisNamespace) == 0 ||
                            wbem_wcsicmp(L"root/security", m_pThisNamespace) == 0))
        if((dwRet  & READ_CONTROL) == 0)
            dwRet = 0;
    return dwRet;

}

//***************************************************************************
//
//  GetNTUserAccess
//
//  Determines the allowed access for a user.
//
//***************************************************************************

DWORD CWbemNamespace::GetNTUserAccess()
{

    HANDLE hAccessToken = INVALID_HANDLE_VALUE;
    if(S_OK != GetAccessToken(hAccessToken))
        return FULL_RIGHTS;       // Not having a token indicates an internal thread

    CCloseHandle cm(hAccessToken);

    DWORD dwMask = 0;

    if(IsAdmin(hAccessToken))
        return FULL_RIGHTS;

    // use the SD

    GENERIC_MAPPING map;
    map.GenericRead = 1;
    map.GenericWrite = 0x1C;
    map.GenericExecute = 2;
    map.GenericAll = 0x6001f;
    PRIVILEGE_SET ps[3];
    DWORD dwSize = 3 * sizeof(PRIVILEGE_SET);
    BOOL bResult;
    long testbit = 1;
    for(int iCnt = 0; iCnt < 26; iCnt++, testbit <<= 1)
    {
        // dont bother testing bits that we dont use

        DWORD dwGranted = 0;
        if(testbit & (FULL_RIGHTS))
        {
            BOOL bOK = AccessCheck(m_sd.GetPtr(), hAccessToken, testbit, &map, ps, &dwSize, &dwGranted, &bResult);
            if(bOK && bResult && dwGranted)
            {
                // if the right is full repository, make sure the user also gets the lesser write
                // access or else the logic for putting/deleting classes will have problems.

                if(testbit == WBEM_FULL_WRITE_REP)
                    dwMask |= (WBEM_PARTIAL_WRITE_REP|WBEM_WRITE_PROVIDER);
                dwMask |= testbit;
            }
        }
    }

    return dwMask;
}

//***************************************************************************
//
//  bool CWbemNamespace::Allowed(DWORD dwRequired)
//
//  Description.  Tests if the user has the requested permission.  This is
//  called before something like a WRITE is done.  Since nt supports
//  supports impersonation, this is always called.  For 9X, a simple check
//  of the permissions strored at the time of connection is OK.
//
//***************************************************************************

bool CWbemNamespace::Allowed(DWORD dwRequired)
{
    //
    // Check for admin first
    //

    GENERIC_MAPPING map;
    map.GenericRead = 1;
    map.GenericWrite = 0x1C;
    map.GenericExecute = 2;
    map.GenericAll = 0x6001f;
    PRIVILEGE_SET ps[3];
    DWORD dwSize = 3 * sizeof(PRIVILEGE_SET);
    BOOL bResult;
    DWORD dwGranted = 0;
    BOOL bOK;

    HANDLE hAccessToken = INVALID_HANDLE_VALUE;

    if(S_OK != GetAccessToken(hAccessToken))
        return true;
    CCloseHandle cm(hAccessToken);

    bOK = AccessCheck(m_sdCheckAdmin.GetPtr(), hAccessToken, 1,
                            &map, ps, &dwSize, &dwGranted, &bResult);
    if(bOK && bResult && dwGranted)
        return true;

    //
    // Not an admin. Continue
    //

    if(EnsureSecurity() != S_OK)
        return false;

    //
    // Always include the check for account enabled right.
    // NOTE: This is safe. We dont really care about the explicit
    // checks for PARTIAL of FULL write below if the account is disabled.
    //
    // NOTE: Why, oh, why did we go the anti-NT security path???????????????
    //
    DWORD dwRequiredCheck = dwRequired ;
    dwRequired |= WBEM_ENABLE ;


    // For nt, the current users priviledges are checked on the fly via access check

    CInCritSec ics(&m_cs);  // grab the cs since we are using the security desc.  // SEC:REVIEWED 2002-03-22 : Assumes entry
    if(IsRemote(hAccessToken))
    {
        //
        // Check to see if the user is remote enabled before continuing. If they are not
        // remote enabled, we fail (except in admin cases).
        //
        dwGranted = 0 ;
        bResult = FALSE ;
        bOK = AccessCheck(m_sd.GetPtr(), hAccessToken, WBEM_REMOTE_ACCESS, &map, ps, &dwSize,
                                                &dwGranted, &bResult);
        if ( !bOK || !bResult || !dwGranted )
        {
            return IsAdmin(hAccessToken) ? true : false ;
        }
    }

    bOK = AccessCheck(m_sd.GetPtr(), hAccessToken, dwRequired, &map, ps, &dwSize, &dwGranted, &bResult);
    bool bRet = (bOK && bResult && dwGranted);

    // Having full repository write gives access to the "lower" write capabilities.  So if the lower
    // right is rejected, double check for the full access right.

    if(bRet == false && (dwRequiredCheck == WBEM_PARTIAL_WRITE_REP || dwRequiredCheck == WBEM_WRITE_PROVIDER))
    {
        bOK = AccessCheck(m_sd.GetPtr(), hAccessToken, WBEM_FULL_WRITE_REP|WBEM_ENABLE, &map, ps, &dwSize,
                                                &dwGranted, &bResult);
        bRet = (bOK && bResult && dwGranted);
    }
    if(bRet == FALSE)
        bRet = TRUE == IsAdmin(hAccessToken);

    return bRet;
}


//***************************************************************************
//
//  HRESULT CWbemNamespace::InitializeSD()
//
//  Description.  Creates the SD
//
//***************************************************************************

HRESULT CWbemNamespace::InitializeSD(IWmiDbSession *pSession)
{
    HRESULT hr;
    if (pSession == NULL)
    {
        hr = CRepository::GetDefaultSession(&pSession);
        if (FAILED(hr))
            return hr;
    }
    else
        pSession->AddRef();
    CReleaseMe relMe2(pSession);
    IWbemClassObject * pThisNSObject = NULL;

    //AutoRevert av;          // switches to system and back to client
    BOOL bWasImpersonating = WbemIsImpersonating();
    if( bWasImpersonating )
    {
        if (FAILED(hr = CoRevertToSelf())) return hr;
    }
    
    //
    // Lets disable security checks here. This is a special case. If we didnt do this
    // a connection to a namespace would fail if the user didnt have the right to see
    // the security descriptor.
    //
    AutoRevertSecTlsFlag secFlag ( (LPVOID)0 ) ;
    hr = CRepository::GetObject(pSession, m_pNsHandle, L"__thisnamespace=@",
                                            WBEM_FLAG_USE_SECURITY_DESCRIPTOR | WMIDB_FLAG_ADMIN_VERIFIED,
                                            &pThisNSObject);
    if(FAILED(hr))
    {
        if(bWasImpersonating)
        {
            if (FAILED(CoImpersonateClient())) 
            {
                hr = WBEM_E_FAILED ;
            }
        }
        return hr;
    }
    CReleaseMe rm1(pThisNSObject);
    hr = GetSDFromProperty(L"SECURITY_DESCRIPTOR", m_sd, pThisNSObject);

    if(bWasImpersonating)
    {
        if ( FAILED (CoImpersonateClient()))   
        {
            hr = WBEM_E_FAILED ;
        }
    }
    return hr ;
}

//***************************************************************************
//
//  HRESULT CWbemNamespace::EnsureSecurity()
//
//  Description.  Generally doesnt do anything except for the first time
//
//***************************************************************************

HRESULT CWbemNamespace::EnsureSecurity()
{
    SCODE sc = S_OK;
    CInCritSec cs(&m_cs);  // SEC:REVIEWED 2002-03-22 : Assumes entry

    if(m_bSecurityInitialized)
        return S_OK;

    sc = InitializeSD(NULL);
    
    if(sc == S_OK)
        m_bSecurityInitialized = true;
    return sc;
}

CBaseAce * ConvertOldObjectToAce(IWbemClassObject * pObj, bool bGroup)
{
    // Get the properties out of the old object

    CVARIANT vName;
    pObj->Get(L"Name", 0, &vName, 0, 0);
    LPWSTR pName = NULL;
    if(vName.GetType() != VT_BSTR)
        return NULL;                // ignore this one.
    pName = LPWSTR(vName);

    CVARIANT vDomain;
    LPWSTR pDomain = L".";
    pObj->Get(L"Authority", 0, &vDomain, 0, 0);
    if(vDomain.GetType() == VT_BSTR)
        pDomain = LPWSTR(vDomain);

    bool bEditSecurity = false;
    bool bEnabled = false;
    bool bExecMethods = false;

    DWORD dwMask = 0;
    CVARIANT vEnabled;
    CVARIANT vEditSecurity;
    CVARIANT vExecMethods;
    CVARIANT vPermission;

    pObj->Get(L"Enabled", 0, &vEnabled, 0, 0);
    pObj->Get(L"EditSecurity", 0, &vEditSecurity, 0, 0);
    pObj->Get(L"ExecuteMethods", 0, &vExecMethods, 0, 0);
    pObj->Get(L"Permissions", 0, &vPermission, 0, 0);

    if (vEnabled.GetType() != VT_NULL && vEnabled.GetBool())
        bEnabled = true;

    if (vEditSecurity.GetType() != VT_NULL && vEditSecurity.GetBool())
        bEditSecurity = true;

    if (vExecMethods.GetType() != VT_NULL && vExecMethods.GetBool())
        bExecMethods = true;

    DWORD dwPermission = 0;
    if (vPermission.GetType() != VT_NULL && vPermission.GetLONG() > dwPermission)
            dwPermission = vPermission.GetLONG();

    // Now translate the old settings into new ones

    if(bEnabled)
        dwMask = WBEM_ENABLE | WBEM_REMOTE_ACCESS | WBEM_WRITE_PROVIDER;

    if(bEditSecurity)
        dwMask |= READ_CONTROL;

    if(bEditSecurity && dwPermission > 0)
        dwMask |= WRITE_DAC;

    if(bExecMethods)
        dwMask |= WBEM_METHOD_EXECUTE;

    if(dwPermission >= 1)
        dwMask |= WBEM_PARTIAL_WRITE_REP;

    if(dwPermission >= 2)
        dwMask |= WBEM_FULL_WRITE_REP | WBEM_PARTIAL_WRITE_REP | WBEM_WRITE_PROVIDER;


    // By default, CNtSid will look up the group name from either the local machine,
    // the domain, or a trusted domain.  So we need to be explicit

    WString wc;
    if(pDomain)
        if(wbem_wcsicmp(pDomain, L".") )
        {
            wc = pDomain;
            wc += L"\\";
        }
    wc += pName;

    // under m1, groups that were not enabled were just ignored.  Therefore the bits
    // cannot be transfer over since m3 has allows and denies, but no noops.  Also,
    // win9x doesnt have denies, do we want to noop those users also.

    if(!bEnabled && bGroup)
        dwMask = 0;

    // In general, m1 just supported allows.  However, a user entry that was not enabled was
    // treated as a deny.  Note that win9x does not allow actual denies.
    DWORD dwType = ACCESS_ALLOWED_ACE_TYPE;

    if(!bGroup && !bEnabled)
    {
        dwMask |= (WBEM_ENABLE | WBEM_REMOTE_ACCESS | WBEM_WRITE_PROVIDER);
        dwType = ACCESS_DENIED_ACE_TYPE;
    }
    
    CNtSid Sid(wc, NULL);
    if(Sid.GetStatus() != CNtSid::NoError)
    {
        ERRORTRACE((LOG_WBEMCORE, "Error converting m1 security ace, name = %S, error = 0x%x",
            wc, Sid.GetStatus()));
        return NULL;
    }
    CNtAce * pace = new CNtAce(dwMask, dwType, CONTAINER_INHERIT_ACE, Sid);
    if (pace && CNtAce::NoError != pace->GetStatus())
    {
        delete pace;
        pace = NULL;
    }
    return pace;
}

//***************************************************************************
//
//  BOOL IsRemote()
//
//  Description. returns true if the box is NT and the caller remote
//
//***************************************************************************


BOOL IsRemote(HANDLE hToken)
{
    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
    BOOL bRet = TRUE;
    
    if(AllocateAndInitializeSid( &id, 1,                    // SEC:REVIEWED 2002-03-22 : OK
        SECURITY_INTERACTIVE_RID, 0,
        0,0,0,0,0,0,&pRawSid))                             // S-1-5-4
    {
        CNtSid Sid(pRawSid);
        FreeSid(pRawSid);        
        if (CNtSid::NoError == Sid.GetStatus())
        {
            if(CNtSecurity::IsUserInGroup(hToken, Sid))
                bRet = FALSE;
        }
    }

    
    //
    //Add proper check for remotness. In addition to the INTERACTIVE group,
    //we also check NETWORK_RID membership
    //
    if ( bRet )
    {
        if(AllocateAndInitializeSid( &id, 1,                    // SEC:REVIEWED 2002-03-22 : OK
            SECURITY_NETWORK_RID, 0,
            0,0,0,0,0,0,&pRawSid))                             // S-1-5-4
        {
            CNtSid Sid(pRawSid);
            FreeSid(pRawSid);            
            if (CNtSid::NoError == Sid.GetStatus())
            {
                if(!CNtSecurity::IsUserInGroup(hToken, Sid))
                    bRet = FALSE;
            }
        }
    }
    return bRet;
}

HRESULT AddDefaultRootAces(CNtAcl * pacl)
{
    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;

    if(AllocateAndInitializeSid( &id, 2,                                 // SEC:REVIEWED 2002-03-22 : OK
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidAdmin(pRawSid);
        FreeSid(pRawSid);
        if (CNtSid::NoError != SidAdmin.GetStatus()) return WBEM_E_FAILED;
        DWORD dwMask = FULL_RIGHTS;
        wmilib::auto_ptr<CNtAce> pace( new CNtAce(dwMask, ACCESS_ALLOWED_ACE_TYPE,
                                                CONTAINER_INHERIT_ACE, SidAdmin));
        if ( NULL == pace.get() )   return WBEM_E_OUT_OF_MEMORY;
        if (CNtAce::NoError != pace->GetStatus()) return WBEM_E_OUT_OF_MEMORY;

        pacl->AddAce(pace.get());
    }

    //
    // Add ACE's for NETWORK_SERVICE ACCOUNT. These accounts have the following rights:
    // 1. WBEM_ENABLE
    // 2. WBEM_METHOD_EXECUTE
    // 3. WBEM_WRITE_PROVIDER
    //
    DWORD dwAccessMaskNetworkLocalService = WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER ;

    if(AllocateAndInitializeSid( &id, 1,                             // SEC:REVIEWED 2002-03-22 : OK
        SECURITY_NETWORK_SERVICE_RID,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidUsers(pRawSid);
        FreeSid(pRawSid);
        if (CNtSid::NoError != SidUsers.GetStatus()) return WBEM_E_FAILED;        
        wmilib::auto_ptr<CNtAce> pace( new CNtAce(dwAccessMaskNetworkLocalService, ACCESS_ALLOWED_ACE_TYPE,
                                                CONTAINER_INHERIT_ACE, SidUsers));
        if ( NULL == pace.get() ) return WBEM_E_OUT_OF_MEMORY;
        if (CNtAce::NoError != pace->GetStatus()) return WBEM_E_OUT_OF_MEMORY;

        pacl->AddAce(pace.get());
    }


    //
    // Add ACE's for NETWORK_SERVICE ACCOUNT. These accounts have the following rights:
    // 1. WBEM_ENABLE
    // 2. WBEM_METHOD_EXECUTE
    // 3. WBEM_WRITE_PROVIDER
    //
    if(AllocateAndInitializeSid( &id, 1,                                 // SEC:REVIEWED 2002-03-22 : OK
        SECURITY_LOCAL_SERVICE_RID,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidUsers(pRawSid);
        FreeSid(pRawSid);
        if (CNtSid::NoError != SidUsers.GetStatus()) return WBEM_E_FAILED;                
        wmilib::auto_ptr<CNtAce> pace( new CNtAce(dwAccessMaskNetworkLocalService, ACCESS_ALLOWED_ACE_TYPE,
                                                CONTAINER_INHERIT_ACE, SidUsers));
        if (NULL == pace.get())  return WBEM_E_OUT_OF_MEMORY;
        if (CNtAce::NoError != pace->GetStatus()) return WBEM_E_OUT_OF_MEMORY;

        pacl->AddAce(pace.get());
    }




    SID_IDENTIFIER_AUTHORITY id2 = SECURITY_WORLD_SID_AUTHORITY;   // SEC:REVIEWED 2002-03-22 : OK

    if(AllocateAndInitializeSid( &id2, 1,   // SEC:REVIEWED 2002-03-22 : OK
        0,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidUsers(pRawSid);
        FreeSid(pRawSid);
        if (CNtSid::NoError != SidUsers.GetStatus()) return WBEM_E_FAILED;                
        DWORD dwMask = WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER;
        wmilib::auto_ptr<CNtAce> pace( new CNtAce(dwMask, ACCESS_ALLOWED_ACE_TYPE,
                                                CONTAINER_INHERIT_ACE, SidUsers));
        if (NULL == pace.get())  return WBEM_E_OUT_OF_MEMORY;
        if (CNtAce::NoError != pace->GetStatus()) return WBEM_E_OUT_OF_MEMORY;

        pacl->AddAce(pace.get());
    }
    return S_OK;
}


HRESULT CopySDIntoProperty(LPWSTR pPropName, CNtSecurityDescriptor &sd, IWbemClassObject *pThisNSObject)
{
    if (sd.GetStatus() != CNtSecurityDescriptor::NoError)
        return WBEM_E_FAILED;

    // move the SD into a variant.
    SAFEARRAY FAR* psa;
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    long lSize = sd.GetSize();
    rgsabound[0].cElements = lSize;
    psa = SafeArrayCreate( VT_UI1, 1 , rgsabound );
    if(psa == NULL)
        return DumpErrorMsgAndReturn(WBEM_E_FAILED, "GetSDMethod failed creating a safe array");

    char * pData = NULL;
    SCODE sc = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pData);
    if(sc != S_OK)
        return DumpErrorMsgAndReturn(sc, "GetSDMethod failed accessing safe array data");

    memcpy(pData, sd.GetPtr(), lSize);    // SEC:REVIEWED 2002-03-22 : OK

    SafeArrayUnaccessData(psa);
    VARIANT var;
    var.vt = VT_UI1|VT_ARRAY;
    var.parray = psa;

    sc = pThisNSObject->Put(pPropName , 0, &var, 0);

    VariantClear(&var);
    return sc;
}

HRESULT GetSDFromProperty(LPWSTR pPropName, CNtSecurityDescriptor &sd, IWbemClassObject *pThisNSObject)
{
    // Get the security descriptor argument

    HRESULT hRes = S_OK ;

    _variant_t var;
    SCODE sc = pThisNSObject->Get(pPropName , 0, &var, NULL, NULL);
    if (sc != S_OK)
    {
        CVARIANT vPath;
        pThisNSObject->Get(L"__PATH", 0, &vPath, 0, 0);
        DEBUGTRACE((LOG_WBEMCORE, "Getting SD from %S failed due to code 0x%X\n", V_BSTR(&vPath), sc));
        return WBEM_E_CRITICAL_ERROR;
    }

    if(var.vt != (VT_ARRAY | VT_UI1))
    {
        CVARIANT vPath;
        pThisNSObject->Get(L"__PATH", 0, &vPath, 0, 0);
        DEBUGTRACE((LOG_WBEMCORE, "Getting SD from %S failed due to incorrect VARIANT type\n", V_BSTR(&vPath) ));
        return WBEM_E_INVALID_PARAMETER;
    }

    SAFEARRAY * psa = V_ARRAY(&var);
    PSECURITY_DESCRIPTOR pSD = NULL;
    sc = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pSD);
    
    if (FAILED(sc)) return DumpErrorMsgAndReturn(WBEM_E_INVALID_PARAMETER, "SetSD failed trying accessing SD property");

    OnDelete<SAFEARRAY *,HRESULT (*)(SAFEARRAY *),SafeArrayUnaccessData> unacc(psa);

    if (0 == psa->rgsabound[0].cElements) return WBEM_E_INVALID_PARAMETER;
    if (!IsValidSecurityDescriptor(pSD)) return WBEM_E_INVALID_PARAMETER;

    CNtSecurityDescriptor sdNew(pSD);

    CNtSid *pTmpSid = sdNew.GetOwner();
    if ( pTmpSid == NULL )
    {
        ERRORTRACE((LOG_WBEMCORE, "ERROR: Security descriptor was retrieved and it had no owner\n"));
    }
    delete pTmpSid;

    pTmpSid = sdNew.GetGroup();
    if (pTmpSid  == NULL )
    {
        ERRORTRACE((LOG_WBEMCORE, "ERROR: Security descriptor was retrieved and it had no group\n"));
    }
    delete pTmpSid;
    
    sd = sdNew;
    if ( sd.GetStatus ( ) != CNtSecurityDescriptor::NoError )
    {
        hRes = WBEM_E_OUT_OF_MEMORY ;
    }

    return hRes ;
}

HRESULT CopyInheritAces(CNtSecurityDescriptor & sd, CNtSecurityDescriptor & sdParent)
{
    // Get the acl list for both SDs

    CNtAcl * pacl = sd.GetDacl();
    if(pacl == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<CNtAcl> dm0(pacl);

    CNtAcl * paclParent = sdParent.GetDacl();
    if(paclParent == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<CNtAcl> dm1(paclParent);

    int iNumParent = paclParent->GetNumAces();
    for(int iCnt = 0; iCnt < iNumParent; iCnt++)
    {
        CNtAce *pParentAce = paclParent->GetAce(iCnt);
        if (pParentAce == NULL)
        	return WBEM_E_OUT_OF_MEMORY;
        CDeleteMe<CNtAce> dm2(pParentAce);
        if (CNtAce::NoError != pParentAce->GetStatus()) continue;

        long lFlags = pParentAce->GetFlags();
        if(lFlags & CONTAINER_INHERIT_ACE)
        {

            if(lFlags & NO_PROPAGATE_INHERIT_ACE)
                lFlags ^= CONTAINER_INHERIT_ACE;
            lFlags |= INHERITED_ACE;

            // If this is an inherit only ace we need to clear this
            // in the children.
            // NT RAID: 161761        [marioh]
            if ( lFlags & INHERIT_ONLY_ACE )
                lFlags ^= INHERIT_ONLY_ACE;

            pParentAce->SetFlags(lFlags);
            pacl->AddAce(pParentAce);
        }
    }
    sd.SetDacl(pacl);
    return S_OK;
}


HRESULT StoreSDIntoNamespace(IWmiDbSession * pSession, IWmiDbHandle *pNSToSet, CNtSecurityDescriptor & sd)
{
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Check to make sure the SD DACL is valid before attempting to put
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    CNtAcl* pAcl = sd.GetDacl ( );
    if ( NULL == pAcl ) return WBEM_E_INVALID_PARAMETER;

    CDeleteMe<CNtAcl> dm (pAcl);
    if ( !IsValidAclForNSSecurity ( pAcl ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    
    AutoRevertSecTlsFlag secFlag ( (LPVOID) 0 ) ;
    IWbemClassObject * pThisNSObject = NULL;
    HRESULT hr = CRepository::GetObject(pSession, pNSToSet, L"__thisnamespace=@",
                                            WBEM_FLAG_USE_SECURITY_DESCRIPTOR, &pThisNSObject);
    if(FAILED(hr))
        return hr;
    CReleaseMe rm1(pThisNSObject);

    hr = CopySDIntoProperty(L"SECURITY_DESCRIPTOR", sd, pThisNSObject);
    if(FAILED(hr))
        return hr;
    return CRepository::PutObject(pSession, pNSToSet, IID_IWbemClassObject, pThisNSObject,
        WMIDB_DISABLE_EVENTS | WBEM_FLAG_USE_SECURITY_DESCRIPTOR);
}

HRESULT    SetSecurityForNS(IWmiDbSession * pSession, IWmiDbHandle *pNSToSet,
                         IWmiDbSession * pParentSession, IWmiDbHandle * pNSParent, BOOL bExisting)
{
    IWbemClassObject * pThisNSObject = NULL;

    // Get the __thisnamespace object
    AutoRevertSecTlsFlag secFlag ( (LPVOID) 0 ) ;
    HRESULT hr = CRepository::GetObject(pSession, pNSToSet, L"__thisnamespace=@",
                                WBEM_FLAG_USE_SECURITY_DESCRIPTOR, &pThisNSObject);
    if(FAILED(hr))
    {
        ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failed to get __thisnamespace=@ object for current namespace <0x%X>!\n", hr));
        return hr;
    }
    CReleaseMe rm1(pThisNSObject);

    // Create the new SD

    CNtSecurityDescriptor sd;
    CNtAcl DestAcl;

    if(bExisting)
    {
        // Fill in the security descriptor
        hr = GetSDFromProperty(L"SECURITY_DESCRIPTOR", sd, pThisNSObject);
        if(FAILED(hr))
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure in GetSDFromProperty <0x%X>!\n", hr));
            return hr;
        }
    
        hr = StripOutInheritedAces (sd);
        if ( FAILED (hr) )
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure in StripOutInheritedAces <0x%X>!\n", hr));
            return hr;
        }

    }
    else
    {
        // NT RAID: 198935 Prefix    [marioh]
        if ( !SetOwnerAndGroup(sd) )
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure in SetOwnerAndGroup <0x%X>!\n", hr));
            return WBEM_E_FAILED;
        }

        if ( !sd.SetDacl(&DestAcl) )
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure in SetDacl <0x%X>!\n", hr));
            return WBEM_E_FAILED;
        }
    }


    CNtAcl * pacl = sd.GetDacl();
    if (pacl == NULL)
    {
        ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure in GetDacl <0x%X>!\n", hr));
        return WBEM_E_FAILED;
    }

    CDeleteMe<CNtAcl> del1(pacl);


    if(pNSParent == NULL)
    {
        // If there is no parent, this must be root.  Create a default one
        
        hr = AddDefaultRootAces(pacl);
        if (FAILED(hr))
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure for AddDefaultRootAces <0x%X>!\n", hr));
            return hr;
        }
        BOOL bRet = sd.SetDacl(pacl);
        if (bRet == FALSE)
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure for SetDacl (2) <0x%X>!\n", hr));
            return WBEM_E_FAILED ;
        }
    }
    else
    {
        // Get the parents __thisnamespace
        IWbemClassObject * pParentThisNSObject = NULL;
        hr = CRepository::GetObject(pParentSession, pNSParent, L"__thisnamespace=@",
                                WBEM_FLAG_USE_SECURITY_DESCRIPTOR, &pParentThisNSObject);
        if(FAILED(hr))
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failed to get __thisnamespace=@ object for parent namespace <0x%X>!\n", hr));
            return hr;
        }
        CReleaseMe rm11(pParentThisNSObject);
        CNtSecurityDescriptor sdParent;
        hr = GetSDFromProperty(L"SECURITY_DESCRIPTOR", sdParent, pParentThisNSObject);
        if(FAILED(hr))
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure for GetSDFromProperty <0x%X>!\n", hr));
            return hr;
        }
        hr = CopyInheritAces(sd, sdParent);
        if (FAILED(hr))
        {
            ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure for CopyInheritAces <0x%X>!\n", hr));
            return hr;
        }
    }
    if(FAILED(hr))
        return hr;

    hr = CopySDIntoProperty(L"SECURITY_DESCRIPTOR", sd, pThisNSObject);
    if(FAILED(hr))
    {
        ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure for CopySDIntoProperty <0x%X>!\n", hr));
        return hr;
    }

    // Extract sd property once more.
    // ==============================

    CNtSecurityDescriptor VerifiedSd;
    hr = GetSDFromProperty(L"SECURITY_DESCRIPTOR", VerifiedSd, pThisNSObject);

    if (FAILED(hr))
    {
        ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failure for GetSDFromProperty (2) <0x%X>!\n", hr));
        CVARIANT vPath;
        pThisNSObject->Get(L"__PATH", 0, &vPath, 0, 0);
        DEBUGTRACE((LOG_WBEMCORE, "Error creating security descriptor for new namespace %S", V_BSTR(&vPath) ));
        return WBEM_E_CRITICAL_ERROR;
    }

    // Go ahead and store the object.
    // ==============================

    hr = CRepository::PutObject(pSession, pNSToSet, IID_IWbemClassObject, pThisNSObject,
                        WMIDB_DISABLE_EVENTS  | WBEM_FLAG_USE_SECURITY_DESCRIPTOR);

    if (FAILED(hr))
    {
        ERRORTRACE((LOG_WBEMCORE, "SetSecurityForNS: Failed to put secured object back <0x%X>!\n", hr));
    }
    return hr;
}


//***************************************************************************
//
//  IsValidAclForNSSecurity
//
//    Checks the ACEs for the following:
//        2. Standard NT ACE correctness [IsValidAce]
//        1. ACE inheritance/propogation flag correctness for WMI namespace
//           security
//
//  Parameters:
//                <CNtAcl&>   ACL to be checked
//                
//  Return:
//                TRUE        if ACL is valid
//                FALSE        if ACL is invalid
//
//***************************************************************************
BOOL IsValidAclForNSSecurity (CNtAcl* pAcl)
{
    BOOL bRet = TRUE;

    // Standard NT ACL check
    if (!pAcl->IsValid()) return FALSE;
    
    // Loop through all the ACEs in the list
    ULONG ulNum = pAcl->GetNumAces( );
    for ( ULONG ulCnt = 0; ulCnt < ulNum; ulCnt++ )
    {
        CNtAce* pAce = pAcl->GetAce ( ulCnt );
        if (NULL == pAce)  return FALSE;
        CDeleteMe<CNtAce> autoDel ( pAce );
        if ( !IsAceValid ( pAce->GetAccessMask(), pAce->GetType(), pAce->GetFlags() ) )
        {
            return FALSE;
        }
    }

    return bRet;
}
