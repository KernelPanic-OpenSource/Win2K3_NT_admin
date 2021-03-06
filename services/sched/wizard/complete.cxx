//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       complete.cxx
//
//  Contents:   Task wizard completion (final) property page implementation.
//
//  Classes:    CCompletionPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"




//+--------------------------------------------------------------------------
//
//  Member:     CCompletionPage::CCompletionPage
//
//  Synopsis:   ctor
//
//  Arguments:  [ptszFolderPath] - full path to tasks folder with dummy
//                                          filename appended
//              [phPSP]                - filled with prop page handle
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CCompletionPage::CCompletionPage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        CWizPage(MAKEINTRESOURCE(IDD_COMPLETION), ptszFolderPath)
{
    TRACE_CONSTRUCTOR(CCompletionPage);

    _pParent = pParent;
    _hIcon = NULL;
    _pJob = NULL;

#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97

    *phPSP = CreatePropertySheetPage(&m_psp);

    if (!*phPSP)
    {
        DEBUG_OUT_LASTERROR;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CCompletionPage::~CCompletionPage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CCompletionPage::~CCompletionPage()
{
    TRACE_DESTRUCTOR(CCompletionPage);

    if (_pJob)
    {
        _pJob->Release();
    }

    if (_hIcon)
    {
        VERIFY(DestroyIcon(_hIcon));
    }
}



//===========================================================================
//
// CWizPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CCompletionPage::_OnInitDialog
//
//  Synopsis:   Perform initialization that should only occur once.
//
//  Arguments:  [lParam] - LPPROPSHEETPAGE used to create this page
//
//  Returns:    TRUE (let windows set focus)
//
//  History:    5-20-1997   DavidMun   Created
//              4-14-1998   CameronE   Added Policy Support
//
//---------------------------------------------------------------------------

LRESULT
CCompletionPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE_METHOD(CCompletionPage, _OnInitDialog);

    //
    // Policy Support - remove "open advanced" checkbox if
    // we find a registry key for that policy
    //
	
    if (RegReadPolicyKey(TS_KEYPOLICY_DISABLE_ADVANCED))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy DISABLE_ADVANCED active to remove checkbox\n"));
        EnableWindow(_hCtrl(complete_advanced_ckbox), FALSE);
        ShowWindow(_hCtrl(complete_advanced_ckbox), SW_HIDE);
    }

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CCompletionPage::_OnPSNSetActive
//
//  Synopsis:   Create a task object (in-memory only) and update the
//              summary information on this page.
//
//  Arguments:  [lParam] - LPNMHDR (unused)
//
//  Returns:    TRUE
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CCompletionPage::_OnPSNSetActive(
    LPARAM lParam)
{
    TRACE_METHOD(CCompletionPage, _OnPSNSetActive);

    HRESULT hr = S_OK;
    LPWSTR pwszTrigger = NULL;

    do
    {
        //
        // Update the summary info to reflect the user's latest
        // choices.
        //

        CSelectProgramPage *pSelProg = GetSelectProgramPage(_pParent);
        CSelectTriggerPage *pSelTrig = GetSelectTriggerPage(_pParent);

        HICON hIcon;

        hIcon = pSelProg->GetSelectedAppIcon();

        SendDlgItemMessage(Hwnd(),
                           complete_task_icon,
                           STM_SETICON,
                           (WPARAM) hIcon,
                           0L);
        if (_hIcon)
        {
            VERIFY(DestroyIcon(_hIcon));
        }
        _hIcon = hIcon;

        Static_SetText(_hCtrl(complete_taskname_lbl), pSelTrig->GetTaskName());
        Static_SetText(_hCtrl(complete_trigger_lbl), TEXT(""));

        //
        // Create the task object so we can ask it for its trigger string.
        // The object won't be persisted until the user hits the Finish
        // button.
        //

        hr = _UpdateTaskObject();

        if (FAILED(hr))
        {
            _SetWizButtons(PSWIZB_BACK | PSWIZB_DISABLEDFINISH);
            break;
        }

        //
        // _pJob is now valid, so enable the finish button.
        //

        _SetWizButtons(PSWIZB_BACK | PSWIZB_FINISH);

        //
        // Put the trigger string in the ui so the user can see a
        // description of when the task will run.
        //

        hr = _pJob->GetTriggerString(0, &pwszTrigger);

        if (FAILED(hr))
        {
            DEBUG_OUT_HRESULT(hr);
            break;
        }
        Static_SetText(_hCtrl(complete_trigger_lbl), pwszTrigger);
    } while (0);

    CoTaskMemFree(pwszTrigger);
    return CPropPage::_OnPSNSetActive(lParam);
}




//+--------------------------------------------------------------------------
//
//  Member:     CCompletionPage::_OnWizBack
//
//  Synopsis:   Set the current page to the one that should precede this.
//
//  Returns:    -1
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CCompletionPage::_OnWizBack()
{
    TRACE_METHOD(CCompletionPage, _OnWizBack);
    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, IDD_PASSWORD);
    return -1;
}



//+--------------------------------------------------------------------------
//
//  Member:     CCompletionPage::_OnWizFinish
//
//  Synopsis:   Persist the task object.
//
//  Returns:    0
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CCompletionPage::_OnWizFinish()
{
    TRACE_METHOD(CCompletionPage, _OnWizFinish);

    HRESULT             hr = S_OK;
    LPCTSTR             ptszJob = NULL;
    CSelectTriggerPage *pSelTrig = GetSelectTriggerPage(_pParent);
    CPasswordPage      *pPasswdPage = GetPasswordPage(_pParent);
    BOOL                fSaveSucceeded = FALSE;

    do
    {
        if (!_pJob)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Persist the new job object.
        //

        ptszJob = pSelTrig->GetJobObjectFullPath();
        CWaitCursor HourGlass;

        if (FileExists((LPTSTR)ptszJob, MAX_PATH + 1))
        {
            if (!DeleteFile(ptszJob))
            {
                //
                // Complain but leave hr alone so we don't pop up a second
                // error
                //

                DEBUG_OUT_LASTERROR;
                SchedUIErrorDialog(Hwnd(),
                                   IDS_CANT_DELETE_EXISTING,
                                   (LPTSTR) pSelTrig->GetTaskName());
                break;
            }
        }

        hr = _pJob->Save(ptszJob, TRUE);
        BREAK_ON_FAIL_HRESULT(hr);

        fSaveSucceeded = TRUE;

        //
        // set the account information.  Caller must ensure service is running.
        //

        //
        // Translate input account prior to saving, then save it
        //
        DWORD cchAccount = MAX_USERNAME + 1;
        WCHAR wszAccount[MAX_USERNAME + 1] = L"";
        LPWSTR pwszPassword = (LPWSTR) pPasswdPage->GetPassword();
        hr = TranslateAccount(TRANSLATE_FOR_INTERNAL, pPasswdPage->GetAccountName(), wszAccount, cchAccount, &pwszPassword);
        if (SUCCEEDED(hr))
        {
            hr = _pJob->SetAccountInformation(wszAccount, pwszPassword);
        }
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _pJob->Save(NULL, FALSE);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    //
    // Don't leave account name & password in memory
    //
    pPasswdPage->ZeroCredentials();

    //
    // If advanced checkbox is checked, indicate to DoTaskWizard.  Also,
    // give it a reference to the job object so it can do any terminal
    // processing necessary (e.g., displaying the property pages).
    //

    if (fSaveSucceeded)
    {
        BOOL fAdvanced = IsDlgButtonChecked(Hwnd(), complete_advanced_ckbox);

        _pParent->SetAdvancedMode(fAdvanced);

        _pJob->AddRef();
        _pParent->SetTaskObject(_pJob);

        _pParent->SetJobObjectPath(ptszJob);
    }

    //
    // Notify the user if anything went wrong.
    //

    if (FAILED(hr))
    {
        if (fSaveSucceeded)
        {
            SchedUIErrorDialog(Hwnd(), IDS_WIZFINISH_NONFATAL, hr);
        }
        else
        {
            SchedUIErrorDialog(Hwnd(), IDS_WIZFINISH_FATAL, hr);
        }
    }
    return 0;
}



//+--------------------------------------------------------------------------
//
//  Member:     CCompletionPage::_UpdateTaskObject
//
//  Synopsis:   Create a task object in memory that matches all the
//              settings the user made on previous pages.
//
//  Returns:    HRESULT
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      If a task object already exists, it is freed and replaced
//              with a new one.
//
//              The task object is not persisted until the user hits the
//              finish button.
//
//---------------------------------------------------------------------------

HRESULT
CCompletionPage::_UpdateTaskObject()
{
    TRACE_METHOD(CCompletionPage, _CreateTaskObject);

    HRESULT             hr = S_OK;
    ITaskTrigger       *pTrigger = NULL;
    CSelectTriggerPage *pSelTrig = GetSelectTriggerPage(_pParent);
    CSelectProgramPage *pSelProg = GetSelectProgramPage(_pParent);

    do
    {
        //
        // If there's already a task object, get rid of it.  This would
        // be the case if the user got to the finish page, then hit
        // the back button.
        //

        if (_pJob)
        {
            _pJob->Release();
            _pJob = NULL;
        }

        //
        // Create the task object
        //

        _pJob = CJob::Create();

        if (_pJob == NULL)
        {
            hr = E_OUTOFMEMORY;
            DEBUG_OUT_HRESULT(hr);
            break;
        }

        //
        // Add default flags
        //

        DWORD dwAddFlags = TASK_FLAG_DONT_START_IF_ON_BATTERIES |
                           TASK_FLAG_KILL_IF_GOING_ON_BATTERIES;

        hr = _pJob->SetFlags(dwAddFlags);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Fill in the trigger struct
        //

        TASK_TRIGGER Trigger;
        SecureZeroMemory(&Trigger, sizeof(Trigger));
        Trigger.cbTriggerSize = sizeof(Trigger);

        CTriggerPage *pTriggerPage = pSelTrig->GetSelectedTriggerPage();

        if (pTriggerPage)
        {
            pTriggerPage->FillInTrigger(&Trigger);
        }
        else
        {
            ULONG idTrigger = pSelTrig->GetSelectedTriggerType();

            switch (idTrigger)
            {
            case seltrig_startup_rb:
                Trigger.TriggerType = TASK_EVENT_TRIGGER_AT_SYSTEMSTART;
                break;

            case seltrig_logon_rb:
                Trigger.TriggerType = TASK_EVENT_TRIGGER_AT_LOGON;
                break;

            default:
                DEBUG_ASSERT(FALSE);
                hr = E_UNEXPECTED;
                break;
            }
            BREAK_ON_FAIL_HRESULT(hr);

            SYSTEMTIME   stStart;
            GetSystemTime(&stStart);

            Trigger.wBeginYear = stStart.wYear;
            Trigger.wBeginMonth = stStart.wMonth;
            Trigger.wBeginDay = stStart.wDay;
        }

        //
        // Create a trigger object and init it with the struct
        //

        WORD iTrigger = (WORD)-1;

        hr = _pJob->CreateTrigger(&iTrigger, &pTrigger);
        BREAK_ON_FAIL_HRESULT(hr);

        DEBUG_ASSERT(iTrigger == 0);

        hr = pTrigger->SetTrigger(&Trigger);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Set the application name
        //

        TCHAR tszExeFullPath[MAX_PATH + 1];

        pSelProg->GetExeFullPath(tszExeFullPath, ARRAYLEN(tszExeFullPath));

        hr = _pJob->SetApplicationName(tszExeFullPath);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Set the arguments
        //
        hr = _pJob->SetParameters(pSelProg->GetArgs());
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Set the working directory
        //

        TCHAR tszWorkingDir[MAX_PATH + 1];

        pSelProg->GetExeDir(tszWorkingDir, ARRAYLEN(tszWorkingDir));

        hr = _pJob->SetWorkingDirectory(tszWorkingDir);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    if (pTrigger)
    {
        pTrigger->Release();
    }

    return hr;
}
