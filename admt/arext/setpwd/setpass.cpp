//---------------------------------------------------------------------------
// SetPass.cpp
//
// Comment: This is a COM object extension for the MCS DCTAccountReplicator.
//          This object implements the IExtendAccountMigration interface. 
//          The process method of this object sets the password for the 
//          target account according to the users specification.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "ResStr.h"
#include <lm.h>
#include "rpc.h"
#include "EaLen.hpp"
#include <activeds.h>
#include "ARExt.h"
#include "ARExt_i.c"
#include "ErrDCT.hpp"
#include "PWGen.hpp"
#include "TReg.hpp"

#import "DBMgr.tlb" no_namespace

StringLoader                 gString;
TErrorDct   err;

#include "SetPwd.h"
#include "SetPass.h"
#include "pwdfuncs.h"
#include "PwRpcUtl.h"
#include "PwdSvc.h"

#define AR_Status_Created           (0x00000001)
#define AR_Status_PasswordCopied    (0x00000200)
#define AR_Status_PasswordError     (0x00000400)

#define SECONDS_PER_DAY 86400.0

/////////////////////////////////////////////////////////////////////////////
// CSetPassword

//---------------------------------------------------------------------------
// Get and set methods for the properties.
//---------------------------------------------------------------------------
STDMETHODIMP CSetPassword::get_sName(BSTR *pVal)
{
   *pVal = m_sName;
    return S_OK;
}

STDMETHODIMP CSetPassword::put_sName(BSTR newVal)
{
   m_sName = newVal;
    return S_OK;
}

STDMETHODIMP CSetPassword::get_sDesc(BSTR *pVal)
{
   *pVal = m_sDesc;
    return S_OK;
}

STDMETHODIMP CSetPassword::put_sDesc(BSTR newVal)
{
   m_sDesc = newVal;
    return S_OK;
}

//---------------------------------------------------------------------------
// ProcessObject : This method currently records the setting of the "User
//                 cannot change password" flag for intra-forest user migrations
//---------------------------------------------------------------------------
STDMETHODIMP CSetPassword::PreProcessObject(
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet,    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                             EAMAccountStats* pStats
                                          )
{
/* local variables */
   IVarSetPtr  pVS = pMainSettings;
   CString     sSAMName = L"";

/* function body */
   _bstr_t sIntraforest = pVS->get(GET_BSTR(DCTVS_Options_IsIntraforest));
   _bstr_t sType = pVS->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   _bstr_t sSrc = pVS->get(GET_BSTR(DCTVS_Options_SourceServer));
   _bstr_t sAccount = pVS->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
   if ((!sIntraforest.length()) || (!sType.length()) || 
       (!sSrc.length()) || (!sAccount.length()))
       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   sSAMName = (WCHAR*)sAccount;
   if (!UStrICmp((WCHAR*)sIntraforest,GET_STRING(IDS_YES)))
   {
      if (!UStrICmp((WCHAR*)sType,L"user") || !UStrICmp((WCHAR*)sType,L"inetOrgPerson"))
      {
            //record the password flags set on the source account
         RecordPwdFlags(sSrc, sAccount);
            //we only want to record the "User cannot change password" flag, so
            //clear the others
         m_bUMCPNLFlagSet = false;
         m_bPNEFlagSet = false;

            //store the flag setting in a map as a string
         if (m_bUCCPFlagSet)
            mUCCPMap.SetAt(sSAMName, L"TRUE");
         else
            mUCCPMap.SetAt(sSAMName, L"FALSE");
      }
   }
      //if previously migrated, store that time in a map
   _bstr_t sSrcDom = pVS->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t sTgtDom = pVS->get(GET_BSTR(DCTVS_Options_TargetDomain));
   IVarSetPtr       pVSMig(__uuidof(VarSet));
   IUnknown       * pUnk;
   IIManageDBPtr    pDb(__uuidof(IManageDB));
   _variant_t       varDate;
   
   pVSMig->QueryInterface(IID_IUnknown, (void**) &pUnk);
   HRESULT hrFind = pDb->raw_GetAMigratedObject(sAccount, sSrcDom, sTgtDom, &pUnk);
   pUnk->Release();
      //if migrated previously, store that time and date in a class map in string
      //format
   if (hrFind == S_OK)
   {
      //
      // if object's password was previously copied
      //

      long lStatusOld = pVSMig->get(L"MigratedObjects.status");

      if (lStatusOld & AR_Status_PasswordCopied)
      {
          //
          // set password copied flag in account status
          // as the account replicator does not preserve
          // status for objects being replaced
          //

          _bstr_t strStatus = GET_BSTR(DCTVS_CopiedAccount_Status);
          long lStatusNew = pVS->get(strStatus);
          lStatusNew |= AR_Status_PasswordCopied;
          pVS->put(strStatus, lStatusNew);

          //
          // retrieve time when object was migrated and save
          //

          varDate = pVSMig->get(L"MigratedObjects.Time");
          if (varDate.vt & VT_DATE)
          {
             COleDateTime aDate = varDate.date;
                //save the date and time in a string
             CString sDate = aDate.Format(L"%B %d, %Y %H:%M:%S");
                //store the date\time string in a map
             mMigTimeMap.SetAt(sSAMName, sDate);
          }
      }
   }

   return S_OK;
}
//---------------------------------------------------------------------------
// ProcessObject : This method sets the password of the target object
//                 by looking at the settings in the varset.
//---------------------------------------------------------------------------
STDMETHODIMP CSetPassword::ProcessObject(
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet,    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                             EAMAccountStats* pStats
                                          )
{
    _bstr_t                   sType;
    IVarSetPtr                pVS = pMainSettings;
    WCHAR                     password[LEN_Path];
    bool                      bGenerate = false;
    bool                      bGenerated = false;
    DWORD                     dwMinUC = 0, dwMinLC = 0, dwMinDigits = 1, dwMinSpecial = 0, dwMaxAlpha = 10, dwMinLen = 4;
    _variant_t                var;
    WCHAR                     fileName[LEN_Path];
    bool                      bCopiedPwd = false;
    HRESULT                   hrPwd = ERROR_SUCCESS;

    sType = pVS->get(GET_BSTR(DCTVS_CopiedAccount_Type));

    //
    // Group objects don't have a password so do not attempt to process.
    //

    if (UStrICmp((WCHAR*)sType, L"group") == 0)
    {
        return S_OK;
    }

    // Get the Error log filename from the Varset
    var = pVS->get(GET_BSTR(DCTVS_Options_Logfile));
    wcscpy(fileName, (WCHAR*)V_BSTR(&var));
    VariantInit(&var);
    // Open the error log
    err.LogOpen(fileName, 1);

    _bstr_t sSrc = pVS->get(GET_BSTR(DCTVS_Options_SourceServer));
    _bstr_t sSrcDom = pVS->get(GET_BSTR(DCTVS_Options_SourceDomain));
    _bstr_t sMach = pVS->get(GET_BSTR(DCTVS_Options_TargetServer));
    _bstr_t sTgtName = pVS->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
    _bstr_t sAccount = pVS->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
    _bstr_t sSkip = pVS->get(GET_BSTR(DCTVS_CopiedAccount_DoNotUpdatePassword));
    _bstr_t sTgtCN = pVS->get(GET_BSTR(DCTVS_CopiedAccount_TargetName));
    _bstr_t sIntraforest = pVS->get(GET_BSTR(DCTVS_Options_IsIntraforest));
    _bstr_t sCopyPwd = pVS->get(GET_BSTR(DCTVS_AccountOptions_CopyPasswords));
    _bstr_t sPwdDC = pVS->get(GET_BSTR(DCTVS_AccountOptions_PasswordDC));

    if ( !UStrICmp((WCHAR*)sSkip,(WCHAR*)sAccount) )
    {
        return S_OK;
    }

    //
    // Retrieve the operating system version of the password server. If it is
    // NT 4 or earlier then the flat or NetBIOS name of the target domain
    // controller must be specified.
    //

    if (m_nPasswordServerVersion == 0)
    {
        PWKSTA_INFO_100 pwiInfo;

        NET_API_STATUS nasStatus = NetWkstaGetInfo(sPwdDC, 100, (LPBYTE*)&pwiInfo);

        if (nasStatus == NERR_Success)
        {
            m_nPasswordServerVersion = pwiInfo->wki100_ver_major;

            NetApiBufferFree(pwiInfo);
        }
        else
        {
            err.SysMsgWrite(ErrW, nasStatus, DCT_MSG_PW_CANT_GET_PES_OS_VERSION_S, (WCHAR*)sPwdDC);
        }
    }

    if ((m_nPasswordServerVersion > 0) && (m_nPasswordServerVersion < 5))
    {
        sMach = pVS->get(GET_BSTR(DCTVS_Options_TargetServerFlat));
    }

    if ( sTgtCN.length() == 0 ) 
        sTgtCN = sTgtName;
    // strip off the CN= from the beginning of the name, if necessary
    if ( !UStrICmp(sTgtCN,"CN=",3) )
    {
        sTgtCN = _bstr_t(((WCHAR*)sTgtCN)+3);
    }

    dwMaxAlpha = (LONG)pVS->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MaxConsecutiveAlpha));
    dwMinDigits = (LONG)pVS->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinDigit));
    dwMinLC = (LONG)pVS->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinLower));
    dwMinUC = (LONG)pVS->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinUpper));
    dwMinSpecial = (LONG)pVS->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinSpecial));
    dwMinLen = (LONG)pVS->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinLength));

    // if the values are all 0 s then we make up something
    if ( (dwMaxAlpha + dwMinDigits + dwMinLC + dwMinUC + dwMinSpecial) == 0 ) 
    {
        dwMinDigits = 3;
        dwMinSpecial = 3;
        dwMinUC = 3;
        dwMinLC = 3;
    }

    //for intra-forest migration of a user, reset the user's
    //original "User cannot change password" flag, whose value is
    //stored in the map as a string
    if ( ! UStrICmp((WCHAR*)sIntraforest,GET_STRING(IDS_YES)) )
    {
        if ( !UStrICmp((WCHAR*)sType,L"user") || !UStrICmp((WCHAR*)sType,L"inetOrgPerson") )
        {
            //get the UCCP string from the map
            CString sFlag;
            if (mUCCPMap.Lookup((WCHAR*)sAccount, sFlag))
            {
                if (sFlag == L"TRUE")
                    m_bUCCPFlagSet = true;
                else
                    m_bUCCPFlagSet = false;
            }//end if found in map
            else
                m_bUCCPFlagSet = false;
            m_bUMCPNLFlagSet = false;
            m_bPNEFlagSet = false;

            ResetPwdFlags(pTarget, sMach, sTgtName);
            return S_OK;
        }
    }
    // Set the password for this account.
    if ( (_wcsicmp((WCHAR*)sType,L"user") == 0) || (_wcsicmp((WCHAR*)sType,L"inetOrgPerson") == 0) || (_wcsicmp((WCHAR*)sType, L"computer") == 0)  )
    {
        if  ( !_wcsicmp((WCHAR*)sType,L"user") || !_wcsicmp((WCHAR*)sType,L"inetOrgPerson") )
        {
            //we will not migrate passwords if replacing and password reuse policy on the target is 2 or greater
            UINT uMsgId = 0;
            if (!CanCopyPassword(pVS, sSrc, sAccount, uMsgId))
            {
                switch (uMsgId)
                {
                    // The source account password has not changed since the last migration.
                case 0:
                    err.MsgWrite(0, DCT_MSG_PW_COPY_NOT_TRIED_S, (WCHAR*)sTgtCN);
                    break;
                    // The 'User Must Change Password at Next Logon' is selected for this user.
                case DCT_MSG_PW_COPY_NOT_TRIED_USER_MUST_CHANGE_S:
                    err.MsgWrite(0, DCT_MSG_PW_COPY_NOT_TRIED_USER_MUST_CHANGE_S, (WCHAR*)sTgtCN);
                    break;
                    // Unable to determine the password age as the password last
                    // changed time is later than the current time on this machine.
                case DCT_MSG_PW_COPY_NOT_TRIED_TIMEDIFF_SS:
                    err.MsgWrite(0, DCT_MSG_PW_COPY_NOT_TRIED_TIMEDIFF_SS, (WCHAR*)sTgtCN, (WCHAR*)sMach);
                    break;
                    // This case should not occur but generate default message in case.
                default:
                    _ASSERT(FALSE);
                    err.MsgWrite(0, DCT_MSG_PW_COPY_NOT_TRIED_S, (WCHAR*)sTgtCN);
                    break;
                }

                return S_OK;
            }

            _bstr_t bstrGenerate = pVS->get(GET_BSTR(DCTVS_AccountOptions_GenerateStrongPasswords));

            if (bstrGenerate == GET_BSTR(IDS_YES))
            {
                bGenerate = true;
            }

            if (bGenerate)
            {
                // generate a strong password
                DWORD dwError = EaPasswordGenerate(dwMinUC, dwMinLC, dwMinDigits, dwMinSpecial, dwMaxAlpha, dwMinLen, password, LEN_Path);
                bGenerated = ( dwError == ERROR_SUCCESS );

                if ( !bGenerated )
                {
                    //
                    // If unable to generate password then mark account to be disabled.
                    //
                    MarkAccountError(pVS);
                    if (pStats != NULL)
                        pStats->errors.users++;
                    err.SysMsgWrite(ErrE, dwError, DCT_MSG_PW_STRONG_GENERATE_FAILED_S, (WCHAR*)sTgtCN);
                    err.LogClose();
                    return S_OK;
                }

                // ensure that the password is NULL terminated
                password[14] = 0;

            }
            else
            {
                // set the password to the first 14 characters of the username
                wcsncpy(password,(WCHAR*)(sTgtName),15);
                // Convert the password to lower case.

                // ensure that the password is NULL terminated
                password[14] = 0;
            }
        }
        else
        {
            // computer, set the password to the first 14 characters of the computer name, in lower case,
            // without the trailing $
            UStrCpy(password, (WCHAR*)sTgtName, DIM(password));
            if ( password[UStrLen(password) - 1] == L'$' )
                password[UStrLen(password) - 1] = L'\0';  // remove trailing $ from machine name
            password[14] = L'\0';                     // truncate to max password length of 14

            // Convert the password to lower case.
            for ( DWORD i = 0; i < wcslen(password); i++ )
                password[i] = towlower(password[i]);

        }

        // We are going to use the Net API to set the password.
        USER_INFO_1003                  pInfo;
        DWORD                           pDw;
        WCHAR                           server[MAX_PATH];
        bool                            bFailedCopyPwd = false;

        //place the new password in the info structure
        pInfo.usri1003_password = password;

        long rc = NetUserSetInfo((WCHAR*)sMach, 
            (WCHAR*)sTgtName, 1003, (LPBYTE) &pInfo, &pDw);

        if ( rc != 0 ) 
        {
            if ( bGenerated )
            {
                //
                // If unable to set password then mark account to be disabled.
                //
                MarkAccountError(pVS);
                if (pStats != NULL)
                    pStats->errors.users++;
                err.SysMsgWrite(ErrE,rc,DCT_MSG_PW_GENERATE_FAILED_S,(WCHAR*)sTgtCN);
                err.LogClose();
                return S_OK;
            }
            else
            {
                if (pStats != NULL)
                    pStats->warnings.users++;
                err.SysMsgWrite(ErrW,rc,DCT_MSG_FAILED_SET_PASSWORD_TO_USERNAME_SD,(WCHAR*)sTgtCN,rc);
                if ( rc == NERR_PasswordTooShort )
                {
                    // try to generate a password
                    DWORD dwError = EaPasswordGenerate(dwMinUC, dwMinLC, dwMinDigits, dwMinSpecial, dwMaxAlpha, dwMinLen, password, LEN_Path);

                    if (dwError != ERROR_SUCCESS)
                    {
                        //
                        // If unable to generate password then mark account to be disabled.
                        //
                        MarkAccountError(pVS);
                        if (pStats != NULL)
                            pStats->errors.users++;
                        err.SysMsgWrite(ErrE, dwError, DCT_MSG_PW_STRONG_GENERATE_FAILED_S, (WCHAR*)sTgtCN);
                        err.LogClose();
                        return S_OK;
                    }

                    rc = NetUserSetInfo((WCHAR*)sMach,(WCHAR*)sTgtName,1003,(LPBYTE)&pInfo,&pDw);
                    if ( rc )
                    {
                        //
                        // If unable to set password then mark account to be disabled.
                        //
                        MarkAccountError(pVS);
                        if (pStats != NULL)
                            pStats->errors.users++;
                        err.SysMsgWrite(ErrE,rc,DCT_MSG_PW_GENERATE_FAILED_S,(WCHAR*)sTgtCN);
                        err.LogClose();
                        return S_OK;
                    }
                    else //else complex password generated, if requested to copy password
                    {    //we now try to do that and only log that complex pwd generated if copy fails
                        //if we are migrating the user's password, then set it here
                        if ( !UStrICmp((WCHAR*)sCopyPwd,GET_STRING(IDS_YES)) )
                        {
                            //record the password flags set on the source account
                            RecordPwdFlags(sSrc, sAccount);
                            //clear the "User cannot change password" flag if it is set
                            ClearUserCanChangePwdFlag(sMach, sTgtName);
                            //set the change password flag to get past the minimum pwd age policy
                            SetUserMustChangePwdFlag(pTarget);
                            //prepare the server name
                            server[0] = L'\\';
                            server[1] = L'\\';
                            UStrCpy(server+2,(WCHAR*)sPwdDC);
                            //call the member function to copy the password.  If success, set flag, else
                            //failed, log the generated password message
                            if ((hrPwd = CopyPassword(_bstr_t(server), sMach, sAccount, sTgtName, _bstr_t(password), pStats)) == ERROR_SUCCESS)
                            {
                                err.MsgWrite(0,DCT_MSG_PWCOPIED_S,(WCHAR*)sTgtCN);
                                bCopiedPwd = true;
                                //reset the password flags as were on the source account 
                                ResetPwdFlags(pTarget, sMach, sTgtName);
                            }
                            else
                            {
                                bFailedCopyPwd = true;
                            }
                        }//end if migrate password
                        else //else not copy password, so post the complex password generated message
                            err.MsgWrite(0,DCT_MSG_PWGENERATED_S,(WCHAR*)sTgtCN);
                    }
                }
            }
        }
        else //else success
        {
            //if complex password generated, if requested to copy password
            //we now try to do that and only log that complex pwd generated if copy fails
            if ( bGenerated )
            {
                //if we are migrating the user's password, then set it here
                if ( !UStrICmp((WCHAR*)sCopyPwd,GET_STRING(IDS_YES)) )
                {
                    //record the password flags set on the source account
                    RecordPwdFlags(sSrc, sAccount);
                    //clear the "User cannot change password" flag if it is set
                    ClearUserCanChangePwdFlag(sMach, sTgtName);
                    //set the change password flag to get past the minimum pwd age policy
                    SetUserMustChangePwdFlag(pTarget);
                    //prepare the server name
                    server[0] = L'\\';
                    server[1] = L'\\';
                    UStrCpy(server+2,(WCHAR*)sPwdDC);
                    //call the member function to copy the password.  If success, set flag, else
                    //failed, log the generated password message
                    if ((hrPwd = CopyPassword(_bstr_t(server), sMach, sAccount, sTgtName, _bstr_t(password), pStats)) == ERROR_SUCCESS)
                    {
                        err.MsgWrite(0,DCT_MSG_PWCOPIED_S,(WCHAR*)sTgtCN);
                        bCopiedPwd = true;
                        //reset the password flags as were on the source account 
                        ResetPwdFlags(pTarget, sMach, sTgtName);
                    }
                    else
                    {
                        bFailedCopyPwd = true;
                    }
                }//end if migrate password
                else //else not copy password, so post the complex password generated message
                    err.MsgWrite(0,DCT_MSG_PWGENERATED_S,(WCHAR*)sTgtCN);
            }
            else
            {
                err.MsgWrite(0,DCT_MSG_SET_PASSWORD_TO_USERNAME_S,(WCHAR*)sTgtCN);
            }
        }
        //if user being migrated and that user's password was not copied, write
        //password to the password file
        if (((_wcsicmp((WCHAR*)sType,L"user") == 0) || (_wcsicmp((WCHAR*)sType,L"inetOrgPerson") == 0)) && (bCopiedPwd == false))
        {
            //
            // Open password log file if it has not been already opened.
            //

            if (!m_bTriedToOpenFile)
            {
                m_bTriedToOpenFile = true;

                _bstr_t strPasswordFile = pVS->get(GET_BSTR(DCTVS_AccountOptions_PasswordFile));

                if (m_passwordLog.LogOpen(strPasswordFile) == FALSE)
                {
                    if (pStats != NULL)
                    {
                        pStats->errors.users++;
                    }
                }
            }

            // Log the new password in the password log.
            if ( m_passwordLog.IsOpen() )
            {
                m_passwordLog.MsgWrite(L"%ls,%ls",(WCHAR*)(pVS->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam)).bstrVal),password);
            }
        }//end if migrating user
    }//end if migrating user or computer

    //change password flags on the account if we did not copy the password
    if (( pTarget ) && (!bCopiedPwd))
    {
        // We want to clear "user cannot change password" and "password never expire flag"
        USER_INFO_1008               usr1008;
        USER_INFO_20               * usr20;
        DWORD                        errParam = 0;
        DWORD rc = NetUserGetInfo((WCHAR*) sSrc, (WCHAR*)sAccount, 20, (LPBYTE *)&usr20);
        _bstr_t                      strDisable = pVS->get(GET_BSTR(DCTVS_AccountOptions_DisableCopiedAccounts));
        _bstr_t                      strSame = pVS->get(GET_BSTR(DCTVS_AccountOptions_TgtStateSameAsSrc));
        long                         val = pVS->get(GET_WSTR(DCTVS_CopiedAccount_UserFlags));
        BOOL                         bDisable = FALSE;
        BOOL                         bSame = FALSE;

        if ( ! UStrICmp(strDisable,GET_STRING(IDS_YES)) )
            bDisable = TRUE;
        if ( ! UStrICmp(strSame,GET_STRING(IDS_YES)) )
            bSame = TRUE;
        if ( !rc ) 
        {
            usr1008.usri1008_flags = usr20->usri20_flags & ~UF_DONT_EXPIRE_PASSWD;
            // we won't turn off the user cannot change password, we will just expire the password below.
            // This will lock out any account with user cannot change password set, and the admin will
            // have to manually unlock the account.

            //usr1008.usri1008_flags &= ~UF_PASSWD_CANT_CHANGE;

            // for the computer account we need to set the UF_PASSWD_NOTREQD
            if ( !_wcsicmp((WCHAR*)sType,L"computer") )
            {
                usr1008.usri1008_flags |= UF_PASSWD_NOTREQD;
                // make sure the disable state for the computer account is the same as for the source computer
                if ( usr20->usri20_flags & UF_ACCOUNTDISABLE )
                {
                    usr1008.usri1008_flags |= UF_ACCOUNTDISABLE;
                }
                else
                {
                    usr1008.usri1008_flags &= ~UF_ACCOUNTDISABLE;
                }
            }
            else
            {
                // for user accounts, the disable flag is set based on the disable option
                // make sure that the disable flag is set properly!
                if ((bDisable) || (bSame && (val & UF_ACCOUNTDISABLE)))
                {
                    usr1008.usri1008_flags |= UF_ACCOUNTDISABLE;
                }
                else 
                {
                    usr1008.usri1008_flags &= ~UF_ACCOUNTDISABLE;
                }
            }         

            NetUserSetInfo((WCHAR*) sMach, (WCHAR*) sTgtName, 1008, (LPBYTE)&usr1008, &errParam);
            NetApiBufferFree(usr20);
        }
        else
        {
            MarkAccountError(pVS);
            if (pStats != NULL)
                pStats->errors.users++;
            err.SysMsgWrite(ErrE, rc, DCT_MSG_PW_UNABLE_RETRIEVE_FLAGS_SS, (WCHAR*)sTgtName, (WCHAR*)sAccount);
        }
        // Require the users to change the password at next logon since we created new passwords for them
        SetUserMustChangePwdFlag(pTarget);
    }

    //
    // set or clear password copied flag in the persisted object status
    // this flag is used to determine whether the password was previously copied
    //

    _bstr_t strStatus = GET_BSTR(DCTVS_CopiedAccount_Status);

    long lStatus = pVS->get(strStatus);

    if (bCopiedPwd)
    {
        lStatus |= AR_Status_PasswordCopied;
    }
    else
    {
        lStatus &= ~AR_Status_PasswordCopied;
    }

    pVS->put(strStatus, lStatus);

    err.LogClose();

    SecureZeroMemory(password, sizeof(password));
    return S_OK;
}

//---------------------------------------------------------------------------
// ProcessUndo : Since we cant undo the password setting we will ignore this.
//---------------------------------------------------------------------------
STDMETHODIMP CSetPassword::ProcessUndo(                                             
                                          IUnknown *pSource,         //in- Pointer to the source AD object
                                          IUnknown *pTarget,         //in- Pointer to the target AD object
                                          IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                          IUnknown **ppPropsToSet,    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                     //         once all extension objects are executed.
                                          EAMAccountStats* pStats
                                       )
{
   _bstr_t                   sType;
   _bstr_t                   sSam;
   _bstr_t                   sServer;
   _bstr_t                   sIntraforest;
   WCHAR                     password[LEN_Path];
   DWORD                     rc = 0;
   DWORD                     pDw = 0;
   IVarSetPtr                pVs = pMainSettings;

   sIntraforest = pVs->get(GET_BSTR(DCTVS_Options_IsIntraforest));
   sSam  = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
   sType = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   sServer = pVs->get(GET_BSTR(DCTVS_Options_TargetServer));
   if ((!sIntraforest.length()) || (!sType.length()) || 
       (!sSam.length()) || (!sServer.length()))
       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

      //if this is user intra-forest, call preprocess the first time in and
      //precess the second
   if (!UStrICmp((WCHAR*)sIntraforest,GET_STRING(IDS_YES)))
   {
      if (!UStrICmp((WCHAR*)sType,L"user") || !UStrICmp((WCHAR*)sType,L"inetOrgPerson"))
      {
            //see if this SAM is in the undone list (every entry in the list has a ","
            //on each side of it
         _bstr_t sTemp = _bstr_t(L",") + sSam + _bstr_t(L",");
            //if found, that means it is post migration, so call process
         if (wcsstr((PCWSTR)m_sUndoneUsers, sTemp))
         {
            ProcessObject(pSource, pTarget, pMainSettings, ppPropsToSet, pStats);
         }
         else //else pre-migration, so add to the list and call preprocess
         {
               //add to the list with an ending ","
            m_sUndoneUsers += sSam;
            m_sUndoneUsers += L",";
            PreProcessObject(pSource, pTarget, pMainSettings, ppPropsToSet, pStats);
         }
      }
   }
   
   if (!_wcsicmp((WCHAR*) sType, L"computer"))
   {
      USER_INFO_1003                      buf;
      rc = 0;
      if ( !rc ) 
      {
         // Create a lower case password from the sam account name. Do not include the trailing $.
         // Password to be maximum of 14 characters.
         UStrCpy(password, (WCHAR*)sSam, DIM(password));
         if ( password[UStrLen(password) - 1] == L'$' )
            password[UStrLen(password) - 1] = L'\0';  // remove trailing $ from machine name
         password[14] = L'\0';                        // truncate to max password length of 14
         for ( DWORD i = 0; i < wcslen(password); i++ )
            password[i] = towlower(password[i]);
         buf.usri1003_password = password;
         rc = NetUserSetInfo((WCHAR*) sServer, (WCHAR*) sSam, 1003, (LPBYTE) &buf, &pDw);
         if ( rc == 2221 )
         {
            WCHAR             sam[300];
            UStrCpy(sam,(WCHAR*)sSam);

            // remove the $ from the sam account name
            sam[UStrLen(sam)-1] = 0;
            rc = NetUserSetInfo((WCHAR*) sServer, sam, 1003, (LPBYTE) &buf, &pDw);
         }
      }
   }
   return HRESULT_FROM_WIN32(rc);
}

BOOL                                       // ret - TRUE if directory found
   CSetPassword::GetDirectory(
      WCHAR                * filename      // out - string buffer to store directory name
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;


   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);


   if ( ! rc )
   {

       rc = key.ValueGetStr(L"Directory",filename,MAX_PATH);

       if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   key.Close();


   return bFound;
}


//---------------------------------------------------------------------------
// IsValidPassword Method
//
// This method validates a password by converting the Unicode password string
// to an ANSI string.
//
// If any Unicode characters cannot be directly translated to an ANSI
// character and a default character must be used then the password is
// invalid.
//
// If any Unicode characters translate to multi-byte characters then the
// password is invalid.
//---------------------------------------------------------------------------


#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS 0x00000400
#endif


bool CSetPassword::IsValidPassword(LPCWSTR pwszPassword)
{
    bool bValid = false;

    BOOL bUsedDefaultChar;
    CHAR szPassword[PWLEN + 1];

    // convert Unicode string to ANSI string

    int cch = WideCharToMultiByte(
        CP_ACP,                     // use system ANSI code page
        WC_NO_BEST_FIT_CHARS,       // do not use best fit characters
        pwszPassword,
        -1,                         // assume password string is zero terminated
        szPassword,
        sizeof (szPassword),
        NULL,
        &bUsedDefaultChar           // will be true if default character used
    );

    // if no error occurred

    if (cch > 0)
    {
        // if default character was not used

        if (bUsedDefaultChar == FALSE)
        {
            CPINFOEX cpie;
            GetCPInfoEx(CP_ACP, 0, &cpie);

            // if code page defines a SBCS then password is valid
            // otherwise code page defines a DBCS and the password
            // must be searched for a multi-byte character

            if ((cpie.LeadByte[0] == 0) && (cpie.LeadByte[1] == 0))
            {
                bValid = true;
            }
            else
            {
                // search for multi-byte character

                bool bLeadByteFound = false;

                for (int ich = 0; ich < cch; ich++)
                {
                    if (IsDBCSLeadByteEx(CP_ACP, szPassword[ich]))
                    {
                        bLeadByteFound = true;
                        break;
                    }
                }

                // if no multi-byte character found
                // then password is valid

                if (!bLeadByteFound)
                {
                    bValid = true;
                }
            }
        }
    }

    SecureZeroMemory(szPassword, sizeof(szPassword));
    return bValid;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 4 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for copying the user's password  *
 * in the source domain to its new account in the target domain.     *
 *     We use the Password Migration COM Wrapper to first check the  *
 * configuration and establish a session with the given Password     *
 * Export server.  Then we copy the password.  The configuration     *
 * check, which establishes a session for this series of operations, *
 * is only done once per set of accounts.                            *
 *                                                                   *
 *********************************************************************/

//BEGIN CopyPassword
HRESULT CSetPassword::CopyPassword(_bstr_t srcServer, _bstr_t tgtServer, _bstr_t srcName, 
                                   _bstr_t tgtName, _bstr_t password, EAMAccountStats* pStats)
{
/* local variables */
    HRESULT     hr = S_OK;

/* function body */

    if (m_pPwdMig == NULL)
    {
        hr = m_pPwdMig.CreateInstance(__uuidof(PasswordMigration));

        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // If session has not been established or needs to be re-established
    // then establish a session with the password export server. Note that
    // EstablishSession will un-establish an existing session first before
    // re-establishing a new session.
    //

    if (m_bEstablishedSession == false)
    {
        hr = m_pPwdMig->raw_EstablishSession(srcServer, tgtServer);

        if (SUCCEEDED(hr))
        {
            m_bEstablishedSession = true;
        }
    }

    //
    // Copy the password.
    // If unsuccessful then force session to be re-established.
    //

    if (SUCCEEDED(hr))
    {
        hr = m_pPwdMig->raw_CopyPassword(srcName, tgtName, password);

        if (FAILED(hr))
        {
            m_bEstablishedSession = false;
        }
    }

    //
    // The only time copy password can fail with ERROR_PASSWORD_RESTRICTION is when password history
    // has been enabled on the target domain and ADMT is attempting to recopy a password that has
    // been previously copied to the target. This scenario can happen if the adminstrator migrates
    // users with the copy password option, then migrates users with the generate complex password
    // option and then finally migrates users with the copy password option again.
    //
    // The only way out of this situation is to simply use the brute force technique of setting
    // the user's password to a series of complex passwords in order to push any known passwords
    // out of the password history before finally attempting to copy the source password again.
    //

    if (hr == HRESULT_FROM_WIN32(ERROR_PASSWORD_RESTRICTION))
    {
        //
        // This solution uses the set password API to push the new passwords into the
        // password history list. The set password API is preferred because the new
        // password is not subject to any password policies in effect. Note that the
        // set password API only works if the caller has adminstrator privileges.
        //

        WCHAR szPasswordOld[LEN_Path];
        WCHAR szPasswordNew[LEN_Path];

        USER_INFO_1003 uiInfo = { szPasswordNew };

        wcscpy(szPasswordOld, password);

        for (long lCount = m_lPwdHistoryLength; lCount > 0; lCount--)
        {
            if (EaPasswordGenerate(5, 5, 2, 2, 10, 14, szPasswordNew, LEN_Path) == ERROR_SUCCESS)
            {
                NET_API_STATUS nasStatus = NetUserSetInfo(tgtServer, tgtName, 1003, (LPBYTE)&uiInfo, NULL);

                if (nasStatus == ERROR_SUCCESS)
                {
                    wcscpy(szPasswordOld, szPasswordNew);
                }
                else
                {
                    break;
                }
            }
        }

        //
        // Must expire password so that the password export server may change password otherwise
        // the minimum password age policy may prevent the password change.
        //
        // Attempt to retrieve user information at level 4 which is only supported on .NET and later
        // but note that level 3 may not be supported on a .NET server when the SAM SID compatibility
        // mode is SAM_SID_COMPATIBILITY_STRICT. If failed to retrieve level 4 then retrieve level 3.
        //
        // Note that an error attempting to expire password is ignored as the copy password call will
        // fail if the minimum password age policy is in effect but will succeed otherwise.
        //

        PUSER_INFO_4 pui4 = NULL;

        NET_API_STATUS nasStatus = NetUserGetInfo(tgtServer, tgtName, 4, (LPBYTE*)&pui4);

        if (nasStatus == ERROR_SUCCESS)
        {
            pui4->usri4_password_expired = TRUE;

            NetUserSetInfo(tgtServer, tgtName, 4, (LPBYTE)pui4, NULL);

            NetApiBufferFree(pui4);
        }
        else
        {
            if (nasStatus == ERROR_INVALID_LEVEL)
            {
                PUSER_INFO_3 pui3 = NULL;

                nasStatus = NetUserGetInfo(tgtServer, tgtName, 3, (LPBYTE*)&pui3);

                if (nasStatus == ERROR_SUCCESS)
                {
                    pui3->usri3_password_expired = TRUE;

                    NetUserSetInfo(tgtServer, tgtName, 3, (LPBYTE)pui3, NULL);

                    NetApiBufferFree(pui3);
                }
            }
        }

        //
        // Attempt to copy password.
        //

        hr = m_pPwdMig->raw_CopyPassword(srcName, tgtName, szPasswordOld);
        
        SecureZeroMemory(szPasswordOld, sizeof(szPasswordOld));
        SecureZeroMemory(szPasswordNew, sizeof(szPasswordNew));
    }

       //if either failed, print warning in the migration.log
    if (FAILED(hr))
    {
        IErrorInfoPtr       pErrorInfo = NULL;
        BSTR                bstrDescription;
        _bstr_t             sText = GET_BSTR(IDS_Unspecified_Failure);

            //get the rich error information on the failure
        if (SUCCEEDED(GetErrorInfo(0, &pErrorInfo)))
        {
            HRESULT hrTmp = pErrorInfo->GetDescription(&bstrDescription);
            if (SUCCEEDED(hrTmp)) //if got rich error info, use it
                sText = _bstr_t(bstrDescription, false);
        }
            //print message in the log
        if (pStats != NULL)
            pStats->warnings.users++;
        err.MsgWrite(ErrW,DCT_MSG_PW_COPY_FAILED_S,(WCHAR*)tgtName, (WCHAR*)sText);
    }

    return hr;
}
//END CopyPassword


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 5 DEC 2000                                                  *
 *                                                                   *
 *     This function is responsible for setting the "User must change*
 * password at next logon" flag for a given user.  We use this prior *
 * to copying a user's password and after we have just set it to a   *
 * new complex password so that we get around the minimum password   *
 * age policy for the target domain.                                 *
 *                                                                   *
 *********************************************************************/

//BEGIN SetUserMustChangePwdFlag
void CSetPassword::SetUserMustChangePwdFlag(IUnknown *pTarget)
{
/* local variables */
   IADs  * pAds = NULL;

/* function body */
      //set the new pwdLastSet value
   HRESULT hr = pTarget->QueryInterface(IID_IADs, (void**) &pAds);
   if ( SUCCEEDED(hr) )
   {
         // Require the users to change the password at next logon since we created new passwords for them
      VARIANT var;
      VariantInit(&var);
      V_I4(&var)=0;
      V_VT(&var)=VT_I4;
      hr = pAds->Put(L"pwdLastSet",var);
      hr = pAds->SetInfo();
      VariantClear(&var);
      if ( pAds ) pAds->Release();
   }
}
//END SetUserMustChangePwdFlag


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 11 JAN 2001                                                 *
 *                                                                   *
 *     This function is responsible for setting and clearing the     *
 * "User cannot change password" flag for a given user, if it was    *
 * originally set.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN ClearUserCanChangePwdFlag
void CSetPassword::ClearUserCanChangePwdFlag(LPCWSTR pwszMach, LPCWSTR pwszUser)
{
/* local variables */
   USER_INFO_3                   * pInfo;
   DWORD                           pDw;
   long                            rc;

/* function body */
      //get the current flag info for this user
   rc = NetUserGetInfo(pwszMach, pwszUser, 3, (LPBYTE *)&pInfo);
   if (rc == 0)
   {
         //clear the "User cannot change password" flag if it is set
      if (pInfo->usri3_flags & UF_PASSWD_CANT_CHANGE)
      {
         pInfo->usri3_flags &= !(UF_PASSWD_CANT_CHANGE);
         NetUserSetInfo(pwszMach, pwszUser, 3, (LPBYTE)pInfo, &pDw);
      }

      NetApiBufferFree((LPVOID) pInfo);
   }
}
//END ClearUserCanChangePwdFlag


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 JAN 2001                                                 *
 *                                                                   *
 *     This function is responsible for recording the password flags *
 * from a given user's source domain account.                        *
 *                                                                   *
 *********************************************************************/

//BEGIN RecordPwdFlags
void CSetPassword::RecordPwdFlags(LPCWSTR pwszMach, LPCWSTR pwszUser)
{
/* local variables */
   USER_INFO_3                   * pInfo;
   long                            rc;

/* function body */
      //get the user password flags
   rc = NetUserGetInfo(pwszMach, pwszUser, 3, (LPBYTE *)&pInfo);
   if (rc == 0)
   {
         //record whether the "User cannot change password" flag is set
      if (pInfo->usri3_flags & UF_PASSWD_CANT_CHANGE)
         m_bUCCPFlagSet = true;//store whether the flag was set
      else
         m_bUCCPFlagSet = false;

         //record whether the "Password never expires" flag is set
      if (pInfo->usri3_flags & UF_DONT_EXPIRE_PASSWD)
         m_bPNEFlagSet = true;//store whether the flag was set
      else
         m_bPNEFlagSet = false;

         //record whether the "User must change password at next logon" flag is set
      if (pInfo->usri3_password_expired)
         m_bUMCPNLFlagSet = true;//store whether the flag was set
      else
         m_bUMCPNLFlagSet = false;

      NetApiBufferFree((LPVOID) pInfo);
   }
}
//END RecordPwdFlags


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 JAN 2001                                                 *
 *                                                                   *
 *     This function is responsible for recording the password flags *
 * from a given user's source domain account.                        *
 *                                                                   *
 *********************************************************************/

//BEGIN ResetPwdFlags
void CSetPassword::ResetPwdFlags(IUnknown *pTarget, LPCWSTR pwszMach, LPCWSTR pwszUser)
{
/* local variables */
   USER_INFO_3                   * pInfo = NULL;
   DWORD                           pDw;
   long                            rc;

/* function body */
      //if the "User cannot change password" or "Password never expires" 
      //flag was original set, reset it
   if ((m_bUCCPFlagSet) || (m_bPNEFlagSet))
   {
         //get the current flag info for this user
      rc = NetUserGetInfo(pwszMach, pwszUser, 3, (LPBYTE *)&pInfo);
      if (rc == 0)
      {
         if (m_bUCCPFlagSet)
            pInfo->usri3_flags |= UF_PASSWD_CANT_CHANGE;
         if (m_bPNEFlagSet)
            pInfo->usri3_flags |= UF_DONT_EXPIRE_PASSWD;
         NetUserSetInfo(pwszMach, pwszUser, 3, (LPBYTE)pInfo, &pDw);

         NetApiBufferFree((LPVOID) pInfo);
      }
   }

      //if the "User must change password at next logon" flag was original set, reset it
   if (m_bUMCPNLFlagSet)
      SetUserMustChangePwdFlag(pTarget);

}
//END ResetPwdFlags


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 MAY 2001                                                  *
 *                                                                   *
 *     This function is responsible for determining if we will indeed*
 * set the password.  If the user is being re-migrated, copy password*
 * was selected, the password reuse policy on the target domain is   *
 * greater than 1, and the password on the source object has not been*
 * changed since the last migration of that user, then we will not   *
 * touch the password on the existing object on the target domain.   *
 *                                                                   *
 *********************************************************************/

//BEGIN CanCopyPassword
BOOL CSetPassword::CanCopyPassword(IVarSet * pVarSet, LPCWSTR pwszMach, LPCWSTR pwszUser, UINT& uMsgId)
{
/* local constants */
    const long      MAX_REUSE_NUM_ALLOWED = 1;

/* local variables */
    BOOL            bCanCopy = TRUE;
    _variant_t      varDate;
    long            rc;

/* function body */
        //init the message id to 0
    uMsgId = 0;

        //if not copying passwords, return TRUE
    _bstr_t sCopyPwd = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyPasswords));
    if (UStrICmp((WCHAR*)sCopyPwd,GET_STRING(IDS_YES)))
        return bCanCopy;

    //
    // Always retrieve the target domain's password history length as
    // the copy password code may need the value even if the account
    // was previously migrated without the copy password option.
    //

    /* if not already done, check password reuse policy on the target domain */
    if (m_lPwdHistoryLength == -1)
    {
        IADsDomain              * pDomain;
        _bstr_t                   sDom( L"WinNT://" );
   
        _bstr_t sTgtDom = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
        sDom += sTgtDom;

        HRESULT hr = ADsGetObject(sDom, IID_IADsDomain, (void **) &pDomain);
        if (SUCCEEDED(hr))
        {
            //Get the password reuse policy
            long lReuse;
            hr = pDomain->get_PasswordHistoryLength(&lReuse);
            pDomain->Release();
                //if successful, store it in a class member variable
            if (SUCCEEDED(hr))
                m_lPwdHistoryLength = lReuse;
        }
    }

        //if not previously migrated, return TRUE
    _bstr_t sAccount = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
        //get the previous migration time from the class map
    CString sDate;
        //if previously migrated, get the date this user was last migrated
    if (mMigTimeMap.Lookup((WCHAR*)sAccount, sDate))
    {
          //convert the date string to a variant (VT_DATE)
        COleDateTime aDate;
        if (aDate.ParseDateTime((LPCTSTR)sDate))
        {
            varDate.vt = VT_DATE;
            varDate.date = DATE(aDate);
        }
        else
            return bCanCopy;
    }
    else //else return TRUE
        return bCanCopy;

    if ((m_lPwdHistoryLength != -1) && (m_lPwdHistoryLength <= MAX_REUSE_NUM_ALLOWED))
        return bCanCopy;

    /* if target account was created, we still want to copy password if object was created (Stystus == 1) */
        //get the migration status for this user
    long lStatus = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_Status));
        //if the status field has the first bit set, then this user was
        //created and we should return TRUE to copy the password
    if (lStatus & AR_Status_Created)
        return bCanCopy;

    //
    // Check whether source account password has been
    // changed since the object has been migrated.
    //

    DWORD dwPasswordAge = 0;
    DWORD dwPasswordExpired = 0;

    //
    // Attempt to retrieve user information at level 4 which is only supported on .NET and later
    // but note that level 3 may not be supported on a .NET server when the SAM SID compatibility
    // mode is SAM_SID_COMPATIBILITY_STRICT. If failed to retrieve level 4 then retrieve level 3.
    //

    PUSER_INFO_4 pui4 = NULL;

    rc = NetUserGetInfo(pwszMach, pwszUser, 4, (LPBYTE*)&pui4);

    if (rc == ERROR_SUCCESS)
    {
        dwPasswordAge = pui4->usri4_password_age;
        dwPasswordExpired = pui4->usri4_password_expired;

        NetApiBufferFree(pui4);
    }
    else
    {
        if (rc == ERROR_INVALID_LEVEL)
        {
            PUSER_INFO_3 pui3 = NULL;

            rc = NetUserGetInfo(pwszMach, pwszUser, 3, (LPBYTE*)&pui3);

            if (rc == ERROR_SUCCESS)
            {
                dwPasswordAge = pui3->usri3_password_age;
                dwPasswordExpired = pui3->usri3_password_expired;

                NetApiBufferFree(pui3);
            }
       }
    }

    if (rc == ERROR_SUCCESS)
    {
        //
        // Retrieve password age in seconds.
        //

        if (dwPasswordAge > 0)
        {
            //
            // Retrieve current system time and then subtract password age
            // from the current system time to arrive at the time that the
            // password was last set. Note that for the DATE type that whole
            // numbers represent days and the fractional part represents a
            // fraction of one day.
            //

            DATE dateCurrent;
            SYSTEMTIME stCurrent;
            GetLocalTime(&stCurrent);
            SystemTimeToVariantTime(&stCurrent, &dateCurrent);

            DATE datePassword = dateCurrent - (double(dwPasswordAge) / SECONDS_PER_DAY);

            //
            // If the time the object was last migrated is later than or equal
            // to the time the password was last set then don't copy password.
            //

            if (varDate.date >= datePassword)
            {
                bCanCopy = FALSE;
            }
        }
        else
        {
            //
            // The password age cannot be determined. There are two possible
            // reasons that the password age cannot be determined. The 'User
            // Must Change Password at Next Logon' might be checked on the
            // source account or the system time on this computer is less
            // than the time the password was last changed.
            //

            //
            // If the password expired value is not zero than we will assume that
            // the 'User Must Change Password at Next Logon' must be checked and
            // there is not a time difference issue.
            //

            if (dwPasswordExpired)
            {
                uMsgId = DCT_MSG_PW_COPY_NOT_TRIED_USER_MUST_CHANGE_S;
            }
            else
            {
                uMsgId = DCT_MSG_PW_COPY_NOT_TRIED_TIMEDIFF_SS;
            }

            bCanCopy = FALSE;
        }
    }

    return bCanCopy;
}
//END CanCopyPassword


//-----------------------------------------------------------------------------
// MarkAccountError
//
// Synopsis
// Sets the error status bit in the account status. This is used by the
// disable account component as an indication that the account should remain
// disabled.
//
// Paramters
// pVarSet - varset containing account information
//-----------------------------------------------------------------------------

void CSetPassword::MarkAccountError(IVarSet* pVarSet)
{
    //
    // set or clear password copied flag in the persisted object status
    // this flag is used to determine whether the password was previously copied
    //

    _bstr_t strStatus = GET_BSTR(DCTVS_CopiedAccount_Status);

    long lStatus = pVarSet->get(strStatus);

    lStatus |= AR_Status_PasswordError;

    pVarSet->put(strStatus, lStatus);
}
