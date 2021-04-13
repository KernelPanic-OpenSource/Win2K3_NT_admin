//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       job.cxx
//
//  Contents:
//
//  Classes:    None.
//
//  Functions:
//
//  History:    14-Mar-96 EricB  created
//              21-Jun-96 MarkBl Renamed from jobedit.cxx since now Save() &
//                               Get/SetAccountInformation members require
//                               client-side rpc. Their implementations must
//                               reside here so we don't have to include the
//                               client-side RPC in the service. Also added
//                               Get/SetAccountInformation members.
//
//  Notes:      Disabled security code completely for Win95. This will be
//              enabled in the next release of the scheduling agent.
//
//              BUGBUG : The classes should be split into a core base class,
//                       then have an OLE-supporting class inherit from it.
//                       The core class would exist in the service. The sub-
//                       class in the dll.
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "network.hxx"
#include "misc.hxx"
#include "job_cls.hxx"
#include "debug.hxx"
#include "defines.hxx"
#include "SASecRPC.h"     // Get/SetAccountInformation RPC definition.
#include <StrSafe.h>

typedef DWORD (WINAPI * PWNETGETUNIVERSALNAMEW)(LPCWSTR, DWORD, LPVOID,
                                                    LPDWORD);

PWNETGETUNIVERSALNAMEW  gpWNetGetUniversalNameW = NULL;

HRESULT DisplayJobProperties(LPTSTR, ITask *);
//
// This operation is not supported locally on Win95, and for the first
// release of the scheduling agent, neither remotely from Win95 to NT.
//

void    ResetAccountInfo(PJOB_ACCOUNT_INFO pAccountInfo);


//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::EditWorkItem
//
//  Synopsis:   Invoke the edit job property sheet.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::EditWorkItem(HWND hParent, DWORD dwReserved)
{
    if (m_ptszFileName != NULL && m_ptszFileName[0] != TEXT('\0'))
    {
        return DisplayJobProperties(m_ptszFileName, (ITask *)this);
    }

    return STG_E_NOTFILEBASEDSTORAGE;
}

//+----------------------------------------------------------------------------
//
//      CJob::IProvideTaskPage::GetPage method
//
//-----------------------------------------------------------------------------

STDMETHODIMP
I_GetTaskPage(
    ITask              * pITask,
    TASKPAGE             tpType,
    BOOL                 fPersistChanges,
    HPROPSHEETPAGE    *  phPage);


STDMETHODIMP
CJob::GetPage(
    TASKPAGE            tpType,
    BOOL                fPersistChanges,
    HPROPSHEETPAGE    * phPage)
{
    return I_GetTaskPage((ITask *)this, tpType, fPersistChanges, phPage);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetAccountInformation
//
//  Synopsis:   Set the name and password of the account to be used for running
//              this job.
//
//  Arguments:  [pwszAccountName] -- Account name.
//              [pwszPassword]    -- Account password.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//
//  Notes:      Both strings are caller allocated and freed.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetAccountInformation(LPCWSTR pwszAccountName,
                            LPCWSTR pwszPassword)
{
    TRACE(CJob, SetAccountInformation)

	if( NULL == pwszAccountName )
	{
		return E_INVALIDARG;
	}
    //
    // This operation is not supported locally on Win95, and for the first
    // release of the scheduling agent, neither remotely from Win95 to NT.
    //

    HRESULT hr;

    //
    // Need to allocate the private data member structure containing copies
    // of the account arguments.
    //
    // Note, could allocate everything within a single buffer, but since
    // this operation is performed so rarely, it really isn't worth it.
    //

    PJOB_ACCOUNT_INFO pAccountInfo = new JOB_ACCOUNT_INFO;

    if (pAccountInfo == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    pAccountInfo->pwszPassword = NULL;
    size_t acctLength = wcslen(pwszAccountName) + 1;
    pAccountInfo->pwszAccount  = new WCHAR[acctLength];

    if (pAccountInfo->pwszAccount != NULL)
    {
        StringCchCopy(pAccountInfo->pwszAccount, acctLength, pwszAccountName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto ErrorExit;
    }

    if (pwszPassword != NULL)
    {
        size_t pwLength = wcslen(pwszPassword) + 1;
        pAccountInfo->pwszPassword = new WCHAR[pwLength];

        if (pAccountInfo->pwszPassword != NULL)
        {
            StringCchCopy(pAccountInfo->pwszPassword, pwLength, pwszPassword);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            goto ErrorExit;
        }
    }

    if (m_pAccountInfo != NULL)
    {
        ResetAccountInfo(m_pAccountInfo);
        delete m_pAccountInfo;
    }
    m_pAccountInfo = pAccountInfo;

    //
    // Setting this flag will result in the RPC call to set the account
    // information on object save (IPersistFile::Save()).
    //

    this->SetFlag(JOB_I_FLAG_SET_ACCOUNT_INFO);

    return(S_OK);

ErrorExit:
    if (pAccountInfo != NULL)
    {
        ResetAccountInfo(pAccountInfo);
        delete pAccountInfo;
    }

    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetAccountInformation
//
//  Synopsis:   Get the name of the account to be used for this job.
//
//  Arguments:  [ppwszAccountName] - the returned string buffer
//
//  Returns:    HRESULTS
//
//  Notes:      The string is callee allocated and caller freed with
//              CoTaskMemFree.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetAccountInformation(LPWSTR * ppwszAccountName)
{
    TRACE3(CJob, GetAccountInformation)

    //
    // This operation is not supported locally on Win95, and for the first
    // release of the scheduling agent, neither remotely from Win95 to NT.
    //

    HRESULT hr;

    //
    // Unexpected, but possible should the caller CoCreateInstance a new
    // job, then call this member w/o IPersistFile::Load/Save.
    //

    if (m_ptszFileName == NULL)
    {
        schDebugOut((DEB_ERROR,
                     "GetAccountInformation called with no filename\n"));
        return(E_UNEXPECTED);
    }

    //
    // Return cached account name. If it hasn't been obtained, we'll
    // need to RPC to the server to get it.
    //

    WCHAR   wszAccountName[MAX_USERNAME + 1];
    WCHAR * pwszAccountName = NULL;

    if (m_pAccountInfo == NULL)
    {
        //
        // RPC to the server local to this job to fetch the account name
        // associated with this job.
        //
        // First, figure out if this is a remote job. If so, fetch the server
        // name on which the job resides.
        //

        WCHAR   wszFileName[MAX_PATH + 1] = L"";
        WCHAR   wszUNCPath[MAX_PATH + 1];
        WCHAR * pwszFileName;
        WCHAR * pwszServerName;

        pwszFileName = m_ptszFileName;

        //
        // Fetch the server name associated with the network path. If the path
        // is local, the server name returned will be NULL.
        //

        hr = GetServerNameFromPath(pwszFileName,
                                   (MAX_PATH + 1) * sizeof(WCHAR),
                                   wszUNCPath,
                                   &pwszServerName);

        if (FAILED(hr))
        {
            return(hr);
        }


        //
        // RPC to the service to fetch the account name.
        //
        // First, isolate the relative job name from the remaining path.
        //

        WCHAR * pwszRelativeFileName;

        if (pwszFileName != NULL)
        {
            pwszRelativeFileName = wcsrchr(pwszFileName, L'\\');

            if (pwszRelativeFileName != NULL)
            {
                pwszRelativeFileName++;
            }
            else
            {
                pwszRelativeFileName = pwszFileName;
            }
        }

        DWORD ccAccountName = MAX_USERNAME;

        RpcTryExcept
        {
            hr = SAGetAccountInformation(pwszServerName,
                                         pwszRelativeFileName,
                                         ccAccountName,
                                         wszAccountName);
        }
        RpcExcept(1)
        {
            DWORD Status = RpcExceptionCode();
            schDebugOut((DEB_ERROR,
                "GetAccountInformation exception(0x%x)\n",
                Status));
            hr = SchedMapRpcError(Status);
        }
        RpcEndExcept;

        if (SUCCEEDED(hr))
        {
            pwszAccountName = wszAccountName;
        }
    }
    else
    {
        schAssert(m_pAccountInfo->pwszAccount != NULL);
        pwszAccountName = m_pAccountInfo->pwszAccount;
        hr = S_OK;
    }

    //
    // Allocate returned name.
    //

    if (pwszAccountName != NULL)
    {
        LPWSTR pwszAccountNameCopy;

        size_t pwLength = (wcslen(pwszAccountName) + 1);
        pwszAccountNameCopy = (LPWSTR)CoTaskMemAlloc(
                         pwLength * sizeof(WCHAR));

        if (pwszAccountNameCopy == NULL)
        {
            return(E_OUTOFMEMORY);
        }

        StringCchCopy(pwszAccountNameCopy, pwLength, pwszAccountName);
        *ppwszAccountName = pwszAccountNameCopy;
    }

    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IPersistFile::Save
//
//  Synopsis:   Save job properties to the job object. Upon successful save,
//              if account credentials have been specified, set them via
//              RPC to the service.
//
//  Arguments:  [pwszFileName] - if null, save to the previously loaded file.
//              [fRemember]    - if TRUE, the object becomes associated with
//                               the new filename.
//
//  Notes:      This member must now be split into two versions with the
//              addition of security: one for the .dll in
//              sched\client\job.cxx, and for the .exe in
//              sched\svc_core\job.cxx. This was necessary as Save now
//              includes client-side rpc code. A single version of this
//              member would require the client-side rpc code to be included
//              in the service.
//
//              All OLE32 strings are UNICODE, including the filename passed
//              in the IPersistFile methods.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::Save(LPCOLESTR pwszFileName, BOOL fRemember)
{
    HRESULT hr = S_OK;


    //
    // Always save fixed and variable length data, but Never alter the running
    // instance count from the COM interface method.
    //

    hr = SaveWithRetry(pwszFileName, fRemember, SAVEP_VARIABLE_LENGTH_DATA);
    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Now that job changes are completely saved, set security information,
    // if specified. This order is important as the application name is tied
    // to the credentials for security reasons. If the application changes,
    // the existing credentials are invalidated.
    //
    // ** Important ** Maintain file save, security setting order!
    //

    if (IsFlagSet(JOB_I_FLAG_SET_ACCOUNT_INFO))
    {
        //
        // RPC to the server local to this job to fetch the account name
        // associated with this job.
        //
        // First, figure out if this is a remote job. If so, fetch the server
        // name on which the job resides.
        //

        WCHAR   wszUNCPath[MAX_PATH + 1];
        WCHAR * pwszFileNameLocal;
        WCHAR * pwszServerName;

        if (pwszFileName == NULL)
        {
            pwszFileNameLocal = m_ptszFileName;
        }
        else
        {
            pwszFileNameLocal = (WCHAR *)pwszFileName;
        }

        //
        // Fetch the server name associated with the network path. If the path
        // is local, the server name returned will be NULL.
        //

        hr = GetServerNameFromPath(pwszFileNameLocal,
                                   (MAX_PATH + 1) * sizeof(WCHAR),
                                   wszUNCPath,
                                   &pwszServerName);

        if (FAILED(hr))
        {
            return(hr);
        }


        //
        // RPC to the service to set the account information.
        //
        // First, isolate the relative job name from the remaining path.
        //

        WCHAR * pwszRelativeFileName;

        if (pwszFileNameLocal != NULL)
        {
            pwszRelativeFileName = wcsrchr(pwszFileNameLocal, L'\\');

            if (pwszRelativeFileName != NULL)
            {
                pwszRelativeFileName++;
            }
            else
            {
                pwszRelativeFileName = pwszFileNameLocal;
            }
        }

        RpcTryExcept
        {
            //
            // Note:  We pass the flags via RPC in order to let
            // the server side do the access checks for the NULL
            // password case.  These checks could technically be
            // done on the client side, but it's more convenient
            // (and smaller codesize) to do it this way
            //
            hr = SASetAccountInformation(pwszServerName,
                                         pwszRelativeFileName,
                                         m_pAccountInfo->pwszAccount,
                                         m_pAccountInfo->pwszPassword,
                                         m_rgFlags);
        }
        RpcExcept(1)
        {
            DWORD Status = RpcExceptionCode();
            schDebugOut((DEB_ERROR,
                "SetAccountInformation exception(0x%x)\n",
                Status));
            hr = SchedMapRpcError(Status);
        }
        RpcEndExcept;

        this->ClearFlag(JOB_I_FLAG_SET_ACCOUNT_INFO);

        //
        // NB : After successful save of the security information, the
        //      cached values are reset.
        //

        ResetAccountInfo(m_pAccountInfo);
        delete m_pAccountInfo;
        m_pAccountInfo = NULL;
    }

    return hr;
}

//
// This operation is not supported locally on Win95, and for the first
// release of the scheduling agent, neither remotely from Win95 to NT.
//

//+----------------------------------------------------------------------------
//
//  Function:   ResetAccountInfo
//
//  Synopsis:   Simple helper to zero the password and deallocate struct
//              JOB_ACCOUNT_INFO fields.
//
//  Arguments:  [pAccountInfo] -- Account info struct to reset.
//
//  Returns:    None.
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
void
ResetAccountInfo(PJOB_ACCOUNT_INFO pAccountInfo)
{
    delete pAccountInfo->pwszAccount;
    pAccountInfo->pwszAccount = NULL;

    if (pAccountInfo->pwszPassword != NULL)
    {
        ZERO_PASSWORD(pAccountInfo->pwszPassword);
        delete pAccountInfo->pwszPassword;
        pAccountInfo->pwszPassword = NULL;
    }
}
