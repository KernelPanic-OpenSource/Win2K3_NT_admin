/*Copyright (c) 1995-1999, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  SecTranslator.cpp
System      -  Domain Consolidation Toolkit.
Author      -  Christy Boles
Created     -  97/06/27
Description -  COM object that controls the security translation process.
               Reads the settings for the translation and performs the necessary
               operations.  

Updates     -
===============================================================================
*/
// SecTranslator.cpp : Implementation of CSecTranslator
#include "stdafx.h"
#include "WorkObj.h"
#include "SecTrans.h"

#include "Mcs.h"     
#include "EaLen.hpp"     
#include "BkupRstr.hpp"
#include "exchange.hpp"            
#include "ErrDct.hpp"

#include "SDStat.hpp" 
#include "sd.hpp"
#include "SecObj.hpp"
#include "LGTrans.h"
#include "RightsTr.h"
#include "RegTrans.h"
#include "BkupRstr.hpp"
#include "TReg.hpp"
#include "TxtSid.h"
#include "GetDcName.h"
#include "folders.h"

using namespace nsFolders;

#include "LSAUtils.h"
#include "Checker.h"
//#import "\bin\DBManager.tlb" no_namespace, named_guids
#import "DBMgr.tlb" no_namespace, named_guids
#import "WorkObj.tlb"
#include "varset_i.c"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if (_WIN32_WINNT < 0x0500)
#define LOGON32_LOGON_NEW_CREDENTIALS 9
#define LOGON32_PROVIDER_WINNT50 3
#endif

#ifndef IStatusObjPtr
_COM_SMARTPTR_TYPEDEF(IStatusObj, __uuidof(IStatusObj));
#endif


/////////////////////////////////////////////////////////////////////////////
// CSecTranslator

#define BACKUP_FAILED   5
#define BAD_PATH        6
#define BAD_LOG         2
#define LEN_SID         200

extern TErrorDct   err;
extern TErrorDct  errAlt;  // this is used for logging errors that occur after dispatcher is launched; use migration.log
extern bool useErrAlt;      // whether to use errAlt

// Defined in EnumVols.cpp
bool                                   // ret -true if name begins with "\\" has at least 3 total chars, and no other '\'
   IsMachineName(
      const LPWSTR           name      // in -possible machine name to check
   );

DWORD                                      // ret- OS return code
   GetProgramFilesDirectory(
      WCHAR                * directory,    // out- location of program files directory
      WCHAR          const * computer      // in - computer to find PF directory on
   )
{
   TRegKey                   hklm;
   TRegKey                   key;
   DWORD                     rc;

   rc = hklm.Connect(HKEY_LOCAL_MACHINE,computer);
   if ( ! rc )
   {
      rc = key.Open(REGKEY_CURRENT_VERSION,&hklm);
   }
   if ( !rc )
   {
      rc = key.ValueGetStr(REGVAL_PROGRAM_FILES_DIRECTORY,directory,MAX_PATH * (sizeof WCHAR));
   }
   return rc;
}


BOOL 
   IsLocallyInstalled()
{
   BOOL                      bFound;
   TRegKey                   key;
   DWORD                     rc;


   rc = key.Open(GET_STRING(IDS_HKLM_DomainAdmin_Key));
   if ( ! rc )
   {
      bFound = TRUE;
   }
   else
   {
      bFound = FALSE;
   }
   return bFound;
}


DWORD                                      // ret- OS return code
   GetLocalMachineName(WCHAR * computer)
{
   DWORD                     rc = 0;
   WKSTA_INFO_100          * buf = NULL;

   rc = NetWkstaGetInfo(NULL,100,(LPBYTE*)&buf);
   if ( ! rc )
   {
      UStrCpy(computer,L"\\\\");
      UStrCpy(computer+2,buf->wki100_computername);
      NetApiBufferFree(buf);
   }
   return rc;
}

BOOL 
   IsThisDispatcherMachine(IVarSet * pVarSet)
{
   BOOL                     bIsIt = FALSE;
   _bstr_t                  dispatcher = pVarSet->get(GET_BSTR(DCTVS_Options_Credentials_Server));
   WCHAR                    localComputer[LEN_Computer] = L"";
   
   GetLocalMachineName(localComputer);

   if ( ! UStrICmp(dispatcher,localComputer) )
   {
      bIsIt = TRUE;
   }
   return bIsIt;
}

class TSession : public TNode
{
   WCHAR                     server[LEN_Computer];
public:
   TSession(WCHAR const * s) { safecopy(server,s); }
   WCHAR             const * ServerName() { return server;} 
};


BOOL 
   CSecTranslator::EstablishASession(
      WCHAR          const * serverName   // in - computer to establish a session to
   )
{
   BOOL                      bSuccess = TRUE;
   TSession                * pSession = new TSession(serverName);

   if (!pSession)
	  return FALSE;

   if ( EstablishSession(serverName,m_domain,m_username,m_password,TRUE) )
   {
      m_ConnectionList.InsertBottom(pSession);
   }
   else
   {
	  delete pSession;
      bSuccess = FALSE;
      err.SysMsgWrite(ErrW,GetLastError(),DCT_MSG_NO_SESSION_SD,serverName,GetLastError());
   }
   return bSuccess;
}

void 
   CSecTranslator::CleanupSessions()
{
   TNodeListEnum             e;
   TSession                * s;
   TSession                * snext;

   for ( s = (TSession*)e.OpenFirst(&m_ConnectionList) ; s ; s = snext )
   {
      snext = (TSession*) e.Next();
      m_ConnectionList.Remove(s);
      // close the session
      EstablishSession(s->ServerName(),NULL,NULL,NULL,FALSE);
      delete s;
   }
   e.Close();


}

STDMETHODIMP 
   CSecTranslator::Process(
      IUnknown             * pWorkItem     // in - varset describing translation options
   )
{
    HRESULT hr = S_OK;

    try
    {
        IVarSetPtr                pVarSet = pWorkItem;
        IStatusObjPtr             pStatus = pVarSet->get(GET_BSTR(DCTVS_StatusObject));
        BOOL                      bReallyDoEverything = FALSE; // this (though not implemented yet) can be used
                                                              // to provide a way to override the default behavior 
                                                              // of only processing file, etc. security when running as 
                                                              // local system.  This would allow selective translation of items
                                                              // on the local machine
        _bstr_t                   text = pVarSet->get(GET_BSTR(DCTVS_Options_Logfile));
        _bstr_t text1 = pVarSet->get(GET_BSTR(DCTVS_Options_AlternativeLogfile));

        m_Args.LogFile(text);


        // Open the log file
        // use append mode since other processes may also be using this file
        if ( ! err.LogOpen(m_Args.LogFile(),1 /*append*/,0) )
        {
          return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        // open the alternative log file if necessary
        useErrAlt = false;
        if (!text1 == false && text1.length() != 0)
        {
            if (!errAlt.LogOpen(text1, 1, 0))
                return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            useErrAlt = true;
        }
            

        LoadSettingsFromVarSet(pVarSet);

        // Set up the cache
        TSDResolveStats     stat(m_Args.Cache(),m_Args.PathList(),pVarSet);

        if ( pStatus )
        {   
          m_Args.Cache()->SetStatusObject(pStatus);
        }

        if ( m_Args.Cache()->IsTree() )
        {
          m_Args.Cache()->ToSorted();
        }
        m_Args.Cache()->SortedToScrambledTree();
        m_Args.Cache()->Sort(&RidComp);
        m_Args.Cache()->Balance();
        m_Args.Cache()->UnCancel();

        //
        // Verify that the Cache got the source and target domain information it needs
        //
        // Note that source and target domain sids are not initialized when using a sid
        // mapping file therefore do not need to check the initialized status as the
        // initialized status is only checking whether the source and target domain
        // sids have been set to a non null value.
        //

        if ( !m_Args.UsingMapFile() && !m_Args.Cache()->IsInitialized() )
        {
          err.MsgWrite(ErrS,DCT_MSG_NO_CACHE_INFO);
          if (useErrAlt)
            errAlt.MsgWrite(ErrS,DCT_MSG_NO_CACHE_INFO);
        }
        else
        {
  
          if ( m_Args.IsLocalSystem() || bReallyDoEverything ) 
          {// Do the required translations
             if ( (m_Args.TranslateFiles() || m_Args.TranslateShares() || m_Args.TranslatePrinters() || m_Args.TranslateRecycler())
                    && !m_Args.Cache()->IsCancelled())
             {
                // This runs the old FST code
                pVarSet->put(GET_BSTR(DCTVS_CurrentOperation),GET_BSTR(IDS_FST_OPERATION_TEXT));
                DoResolution(&stat);
             }

             if ( m_Args.TranslateLocalGroups() && !m_Args.Cache()->IsCancelled() )
             {
                pVarSet->put(GET_BSTR(DCTVS_CurrentOperation),GET_BSTR(IDS_LGST_OPERATION_TEXT));
                DoLocalGroupResolution(&stat);
             }

             if ( m_Args.TranslateUserRights() && !m_Args.Cache()->IsCancelled() )
             {
                pVarSet->put(GET_BSTR(DCTVS_CurrentOperation),GET_BSTR(IDS_URST_OPERATION_TEXT));
                DoUserRightsTranslation(&stat);
             }

             if ( m_Args.TranslateRegistry() && !m_Args.Cache()->IsCancelled() )
             {
                pVarSet->put(GET_BSTR(DCTVS_CurrentOperation),GET_BSTR(IDS_REGST_OPERATION_TEXT));
			        //get needed privileges and keep them until the agent removes itself
                GetBkupRstrPriv((WCHAR*)NULL);
       	        GetPrivilege((WCHAR*)NULL,SE_SECURITY_NAME);
                TranslateRegistry(NULL,&m_Args,m_Args.Cache(),&stat);
             }
             if ( m_Args.TranslateUserProfiles() && !m_Args.Cache()->IsCancelled() )
             {
                // set the flag to indicate whether we want to allow switching from REPLACE to ADD
                // mode for profile translation while the user is logged on
                _bstr_t bstrFlag = pVarSet->get(GET_BSTR(DCTVS_Options_AllowSwitchingFromReplaceToAddInProfileTranslation));
                if (!bstrFlag == false && !UStrICmp(bstrFlag, GET_STRING(IDS_No)))
                {
                    m_Args.SetAllowingToSwitchFromReplaceToAddModeInProfileTranslation(FALSE);
                }

			    //get needed privileges and keep them until the agent removes itself
                GetBkupRstrPriv((WCHAR*)NULL);
                GetPrivilege((WCHAR*)NULL,SE_SECURITY_NAME);
                TranslateLocalProfiles(&m_Args,m_Args.Cache(),&stat);
             }
          }
          else
          {
             // do exchange translation

             if ( (m_Args.TranslateMailboxes() ||  m_Args.TranslateContainers()) && !m_Args.Cache()->IsCancelled() )
             {
                // This will run the old EST code
                pVarSet->put(GET_BSTR(DCTVS_CurrentOperation),GET_BSTR(IDS_EST_OPERATION_TEXT));
                DoExchangeResolution(&stat,pVarSet);
             }
          }
        }

        // indicate the operation is aborted if necessary
        if (m_Args.Cache()->IsCancelled())
            err.MsgWrite(0, DCT_MSG_OPERATION_ABORTED);
        
        pVarSet->put(GET_BSTR(DCTVS_CurrentOperation),"");
  
        ExportStatsToVarSet(pVarSet,&stat);

        if ( *m_CacheFile )
        {
          BuildCacheFile(m_CacheFile);
        }

        // Record whether errors occurred
        long                     level = pVarSet->get(GET_BSTR(DCTVS_Results_ErrorLevel));
        if ( level < err.GetMaxSeverityLevel() )
        {
          pVarSet->put(GET_BSTR(DCTVS_Results_ErrorLevel),(LONG)err.GetMaxSeverityLevel());
        }
         
        err.LogClose();
        if (useErrAlt)
        {
            errAlt.LogClose();
            useErrAlt = false;
        }
        CleanupSessions();
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
        err.SysMsgWrite(ErrS, hr, DCT_MSG_SECURITY_TRANSLATOR_UNABLE_TO_CONTINUE);
    }
    catch (...)
    {
        hr = E_UNEXPECTED;
        err.SysMsgWrite(ErrS, hr, DCT_MSG_SECURITY_TRANSLATOR_UNABLE_TO_CONTINUE);
    }

    return hr;
}


void 
   CSecTranslator::LoadSettingsFromVarSet(
      IVarSet              * pVarSet      // in - varset containing settings
   )
{
   MCSASSERT(pVarSet);
   
   _bstr_t                   text;
   _bstr_t                   text2;
   
   DWORD                     val;

   try 
   {
      m_Args.Reset();

      text = pVarSet->get(GET_BSTR(DCTVS_Options_LocalProcessingOnly));
      if ( !text == false && !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         err.MsgWrite(0,DCT_MSG_LOCAL_MODE);
         m_LocalOnly = TRUE;
         m_Args.SetLocalMode(TRUE);
         text = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomainSid));
         safecopy(m_SourceSid,(WCHAR const *)text);
         text = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomainSid));
         safecopy(m_TargetSid,(WCHAR const *)text);
      }

      text = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
      m_Args.Source(text);

      text = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
      m_Args.Target(text);

      val = (LONG)pVarSet->get(GET_BSTR(DCTVS_Options_LogLevel));
      if ( val )
         m_Args.SetLogging(val);
   
      val = (LONG)pVarSet->get(L"Security.DebugLogLevel");
      if ( val )
         m_Args.SetLogging(val);

      text = pVarSet->get(GET_BSTR(DCTVS_Options_NoChange));
      if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.SetWriteChanges(FALSE);
      }
      else
      {
         m_Args.SetWriteChanges(TRUE);
      }
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslationMode));

      if ( !UStrICmp(text,GET_STRING(IDS_Add)) )
      {
         m_Args.SetTranslationMode(ADD_SECURITY);
      }
      else if (! UStrICmp(text,GET_STRING(IDS_Replace)) )
      {
         m_Args.SetTranslationMode(REPLACE_SECURITY);
      }
      else if ( ! UStrICmp(text,GET_STRING(IDS_Remove)) )
      {
         m_Args.SetTranslationMode(REMOVE_SECURITY);
      }
      else
      {
         // Incorrect value - don't need to log this, just use replace
         // the log will show replace as the translation mode
         // err.MsgWrite(ErrE,DCT_MSG_BAD_TRANSLATION_MODE_S,(WCHAR*)text);
      }

      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateFiles));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.TranslateFiles(TRUE);
      }
      else
      {
         m_Args.TranslateFiles(FALSE);
      }

      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateShares));
      if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.TranslateShares(TRUE);
      }
      else
      {
         m_Args.TranslateShares(FALSE);
      }
   
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslatePrinters));
      if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.TranslatePrinters(TRUE);
      }
      else
      {
         m_Args.TranslatePrinters(FALSE);  
      }
   
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateUserProfiles));
      if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.TranslateUserProfiles(TRUE);
         m_Args.TranslateRecycler(TRUE);
      }
      else
      {
         m_Args.TranslateUserProfiles(FALSE); 
         m_Args.TranslateRecycler(FALSE);
      }
      
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateLocalGroups));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.TranslateLocalGroups(TRUE);
      }
      else
      {
         m_Args.TranslateLocalGroups(FALSE);
      }

      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateRegistry));
      if (! UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.TranslateRegistry(TRUE);
      }
      else
      {
         m_Args.TranslateRegistry(FALSE);
      }

      val = (LONG)pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));
      for ( int i = 0 ; i < (int)val ; i++ )
      {
         WCHAR                  key[MAX_PATH];
         DWORD                  flags = 0;
         _bstr_t                bStr;

         swprintf(key,GET_STRING(DCTVSFmt_Servers_D),i);

         bStr = key;

         text = pVarSet->get(bStr);
         if ( text.length() )
         {
            m_Args.PathList()->AddPath(text,flags);
         }
      }
   
      text = pVarSet->get(GET_BSTR(DCTVS_Security_GatherInformation));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.Cache()->AddIfNotFound(TRUE);
         m_Args.SetWriteChanges(FALSE);
         m_Args.SetLogging(m_Args.LogSettings() & ~FILESTATS);
      }
      else
      {
         m_Args.Cache()->AddIfNotFound(FALSE);
      }

      // Exchange Security Translation settings
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateMailboxes));
      if ( text.length() )
      {
         m_Args.TranslateMailboxes(TRUE);
         safecopy(m_Container,(WCHAR*)text);

         text = pVarSet->get(GET_BSTR(DCTVS_Security_MapiProfile));
         safecopy(m_Profile,(WCHAR*)text);

      }
      else
      {
         m_Args.TranslateMailboxes(FALSE);
      }

      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateContainers));
      if ( text.length() )
      {
         m_Args.TranslateContainers(TRUE);
         if ( ((WCHAR*)text)[0] == L'\\' && ((WCHAR*)text)[1] == L'\\' )
            safecopy(m_exchServer,(WCHAR*)text+2);
         else
            safecopy(m_exchServer,(WCHAR*)text);
      }
      else
      {
         m_Args.TranslateContainers(FALSE);
      }
      
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateUserRights));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         m_Args.TranslateUserRights(TRUE);
      }
      text = pVarSet->get(GET_BSTR(DCTVS_Security_BuildCacheFile));
      if ( text.length() )
      {
         safecopy(m_CacheFile,(WCHAR*)text);
      }
      // Check for inconsistent arguments
      
      // Load the cache
      // There are 4 possible ways to populate the cache
      // 1.  Use the list from the migrated objects table in our database
      // 2.  We are given a list of accounts in the VarSet, under "Accounts.".  This allows for renaming, but requires the most space
      // 3.  We are given an input file that was generated by AR, in "Accounts.InputFile".  This also allows for renaming, with less overall memory use.
	  // 4.  We are given a sid mapping, comma-seperated, file with source and target sids.
      
      text = pVarSet->get(GET_BSTR(DCTVS_Security_BuildCacheFile));
      text2 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT));
         //if list is in the migrated objects table
      if ((!m_LocalOnly) && (!UStrICmp(text2,GET_STRING(IDS_YES)))) 
      {
         LoadMigratedObjects(pVarSet);
      }
	     //else if a sid mapping file is being used
      else if ((!m_LocalOnly) && (UStrICmp(text2,GET_STRING(IDS_YES)))) 
      {
         m_Args.SetUsingMapFile(TRUE); //set the arg flag to indicate use of map file
         text2 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityMapFile));
         if (!LoadCacheFromMapFile(text2, pVarSet))
            _com_issue_error(E_FAIL);
      }
      //  took out the not because gather information sets this to false.
      //else if ( !m_Args.Cache()->AddIfNotFound() ) // skip loading the cache if we're gathering information
      else if ( m_Args.Cache()->AddIfNotFound() ) // skip loading the cache if we're gathering information
      {
         if ( m_LocalOnly )
         {
            m_Args.Cache()->SetSourceAndTargetDomainsWithSids(m_Args.Source(),m_SourceSid,m_Args.Target(),m_TargetSid);  
         }
         else
         {
            m_Args.Cache()->SetSourceAndTargetDomains(m_Args.Source(),m_Args.Target());
         }
      }
      else
      {
         text = pVarSet->get(GET_BSTR(DCTVS_Accounts_InputFile));
         if ( text.length() )
         {
            LoadCacheFromFile(text,pVarSet);
         }
         else
         {
            LoadCacheFromVarSet(pVarSet);
         }
      }
   }
   catch ( ... )
   {
      err.MsgWrite(ErrS,DCT_MSG_EXCEPTION_READING_VARSET);
      throw;
   }
}
 
void 
   CSecTranslator::ExportStatsToVarSet(
      IVarSet              * pVarSet,        // in - varset to write stats to
      TSDResolveStats      * stat            // in - object containing stats
   )
{
   _bstr_t                   filename;

   filename = pVarSet->get(GET_BSTR(DCTVS_Security_ReportAccountReferences));

   stat->Report(m_Args.LogSummary(),m_Args.LogAccountDetails(),m_Args.LogPathDetails());
   
   if ( m_Args.NoChange() )
   {
      err.MsgWrite(0,DCT_MSG_NO_CHANGE_MODE);
   }
   
   stat->ReportToVarSet(pVarSet,m_Args.LogSettings() & SUMMARYSTATS);
   
   if ( filename.length() )
   {
      err.MsgWrite(0,DCT_MSG_EXPORTING_ACCOUNT_REFS_S,(WCHAR*)filename);
      m_Args.Cache()->ReportAccountReferences((WCHAR*)filename);
   }

   long                     level = pVarSet->get(GET_BSTR(DCTVS_Results_ErrorLevel));
   if ( level < err.GetMaxSeverityLevel() )
   {
      pVarSet->put(GET_BSTR(DCTVS_Results_ErrorLevel),(LONG)err.GetMaxSeverityLevel());
   }
}

void 
   CSecTranslator::DoResolution(
      TSDResolveStats      * stat         // in - object to write translation stats to
   )
{
    // display confirmation message if writing changes 
    int                       result;
    if (!m_Args.Cache()->IsCancelled())
        result = ResolveAll(&m_Args,stat);
}

void 
   CSecTranslator::DoLocalGroupResolution(
      TSDResolveStats      * stat         // in - object to write translation stats to
  )
{
    DWORD                     rc;
    TNodeListEnum             tenum;
    TPathNode               * tnode;

    if (!m_Args.Cache()->IsCancelled())
    {
        if ( m_LocalOnly )
        {
          err.MsgWrite(0,DCT_MSG_TRANSLATING_LOCAL_GROUPS);
          rc = TranslateLocalGroups(NULL,&m_Args,m_Args.Cache(),stat);
        }
        else
        {   // Enumerate the machines in the pathlist
          for (tnode = (TPathNode *)tenum.OpenFirst((TNodeList *)m_Args.PathList()) 
             ; tnode && !m_Args.Cache()->IsCancelled()
             ; tnode = (TPathNode *)tenum.Next() )
          {
             if ( IsMachineName(tnode->GetPathName()) )
             {
                err.MsgWrite(0,DCT_MSG_TRANSLATING_LOCAL_GROUPS_ON_S,tnode->GetPathName());
                rc = TranslateLocalGroups(tnode->GetPathName(),&m_Args,m_Args.Cache(),stat);
             }
          }
        }
    }
    if (m_Args.Cache()->IsCancelled())
        err.MsgWrite(0, DCT_MSG_OPERATION_ABORTED_LOCAL_GROUPS);
    tenum.Close();
}              

void 
   CSecTranslator::DoUserRightsTranslation(
      TSDResolveStats      * stat         // in - object to write stats to
  )
{
    DWORD                     rc;
    TNodeListEnum             tenum;
    TPathNode               * tnode;

    if (!m_Args.Cache()->IsCancelled())
    {
        if ( m_LocalOnly )
        {
          err.MsgWrite(0,DCT_MSG_TRANSLATING_USER_RIGHTS);
          rc = TranslateUserRights(NULL,&m_Args,m_Args.Cache(),stat);
        }
        else
        {   // Enumerate the machines in the pathlist
          for (tnode = (TPathNode *)tenum.OpenFirst((TNodeList *)m_Args.PathList()) 
             ; tnode && !m_Args.Cache()->IsCancelled()
             ; tnode = (TPathNode *)tenum.Next() )
          {
             if ( IsMachineName(tnode->GetPathName()) )
             {
                err.MsgWrite(0,DCT_MSG_TRANSLATING_RIGHTS_ON_S,tnode->GetPathName());
                rc = TranslateUserRights(tnode->GetPathName(),&m_Args,m_Args.Cache(),stat);
             }
          }
        }
    }
    if (m_Args.Cache()->IsCancelled())
        err.MsgWrite(0, DCT_MSG_OPERATION_ABORTED_USER_RIGHTS);
    tenum.Close();
}


BOOL 
   CSecTranslator::LoadCacheFromVarSet(
      IVarSet              * pVarSet      // in - varset containing account mapping
   )
{
   BOOL                      bSuccess = TRUE;
   _bstr_t                   text;

   if ( m_LocalOnly )
   {
      m_Args.Cache()->SetSourceAndTargetDomainsWithSids(m_Args.Source(),m_SourceSid,m_Args.Target(),m_TargetSid);  
   }
   else
   {
      m_Args.Cache()->SetSourceAndTargetDomains(m_Args.Source(),m_Args.Target());
   }
   m_Args.Cache()->ToSorted();
   // no wildcard filter specified.  Use the explicit list of accounts
   long numAccounts = pVarSet->get(GET_BSTR(DCTVS_Accounts_NumItems));
   for ( int i = 0 ; i < numAccounts ; i++ )
   {
      WCHAR                  key[LEN_Path];
      WCHAR                  name[LEN_Account];
      WCHAR                  targetName[LEN_Account];
      WCHAR                  type[LEN_Path];
      short                  sType;

      swprintf(key,GET_STRING(DCTVSFmt_Accounts_D),i);
      text = pVarSet->get(key);
      safecopy(name,(WCHAR*)text);

      swprintf(key,GET_STRING(DCTVSFmt_Accounts_TargetName_D),i);
      text = pVarSet->get(key);
      safecopy(targetName,(WCHAR*)text);

      swprintf(key,GET_STRING(DCTVSFmt_Accounts_Type_D),i);
      text = pVarSet->get(key);
      safecopy(type,(WCHAR*)text);


      if (!UStrICmp(type,L"user") || !UStrICmp(type,L"inetOrgPerson"))
         sType = EA_AccountUser;
      else if (! UStrICmp(type,L"group") )
         sType = EA_AccountGroup;
      else
         sType = 0;
      m_Args.Cache()->InsertLast(name,0,targetName,0,sType);
   }
   
   if ( bSuccess && ! m_LocalOnly )
   {
      bSuccess = GetRIDsFromEA();
   }

   m_Args.Cache()->VerifyTargetSids();
   
   return bSuccess;
}


// this reads the migrated objects table from the database, and 
// constructs a mapping file to be used for security translation

HRESULT CSecTranslator::LoadMigratedObjects(IVarSet* pVarSetIn)
{
    HRESULT hr = S_OK;

    if ( m_LocalOnly )
    {
        m_Args.Cache()->SetSourceAndTargetDomainsWithSids(m_Args.Source(),m_SourceSid,m_Args.Target(),m_TargetSid);  
    }
    else
    {
        m_Args.Cache()->SetSourceAndTargetDomains(m_Args.Source(),m_Args.Target());
    }

    m_Args.Cache()->ToSorted();

    //
    // Create instance of database manager.
    //

    IIManageDBPtr spDB;
    hr = spDB.CreateInstance(CLSID_IManageDB);

    if  (SUCCEEDED(hr))
    {
        //
        // Retrieve objects that have been migrated from the source domain to the target domain.
        //

        IVarSetPtr spVarSet(CLSID_VarSet);

        hr = spDB->raw_GetMigratedObjectsForSecurityTranslation(
            _bstr_t(m_Args.Source()),
            _bstr_t(m_Args.Target()),
            IUnknownPtr(spVarSet)
        );

        if (SUCCEEDED(hr))
        {
            //
            // Retrieve data for each object and insert into cache.
            //

            WCHAR szKey[MAX_PATH];
            long lCount = spVarSet->get(L"MigratedObjects");

            for (long lIndex = 0; lIndex < lCount; lIndex++)
            {
                // retrieve object's data from VarSet

                swprintf(szKey, L"MigratedObjects.%ld.SourceSamName", lIndex);
                _bstr_t strSrcSam = spVarSet->get(szKey);

                swprintf(szKey, L"MigratedObjects.%ld.TargetSamName", lIndex);
                _bstr_t strTgtSam = spVarSet->get(szKey);

                swprintf(szKey, L"MigratedObjects.%ld.Type", lIndex);
                _bstr_t strType = spVarSet->get(szKey);

                swprintf(szKey, L"MigratedObjects.%ld.SourceRid", lIndex);
                long lSrcRid = spVarSet->get(szKey);

                swprintf(szKey, L"MigratedObjects.%ld.TargetRid", lIndex);
                long lTgtRid = spVarSet->get(szKey);

                // convert type to cache specific type

                short sType;

                if (UStrICmp(strType, L"user") == 0)
                {
                    sType = EA_AccountUser;
                }
                else if (UStrICmp(strType, L"group") == 0)
                {
                    sType = EA_AccountGroup;
                }
                else
                {
                    sType = 0;
                }

                // insert object's data into cache

                m_Args.Cache()->InsertLast(strSrcSam, lSrcRid, strTgtSam, lTgtRid, sType);
            }
        }
    }

    m_Args.Cache()->VerifyTargetSids();
    
    return hr;
}

BOOL 
   CSecTranslator::BuildCacheFile(
      WCHAR          const * filename        // in - file to write account mapping to
   )
{
   BOOL                      bSuccess = TRUE;
   FILE                    * pFile;
   
   m_Args.Cache()->ToSorted();

   TNodeListEnum             e;
   TRidNode                * node;
   WCHAR                     type[LEN_Path];


   pFile = _wfopen(filename,L"wb");
   
   if ( pFile )
   {
      for ( node = (TRidNode*) e.OpenFirst(m_Args.Cache() ); node ; node = (TRidNode*)e.Next() )
      {
      
         switch ( node->Type() )
         {
         case EA_AccountUser:
            safecopy(type,L"user");
            break;
         case EA_AccountGroup:
         case EA_AccountGGroup:
         case EA_AccountLGroup:
            safecopy(type,L"group");
            break;
        default:
            type[0] = 0;
            break;
         }

//         if (!UStrICmp(node->GetAcctName(),node->GetTargetAcctName()))
         if ((UStrICmp(node->GetSrcDomSid(),L"")) && (UStrICmp(node->GetTgtDomSid(),L"")))
         {
		       //account and domain names could be empty when using a sid
		       //mapping file for security translation.  A later scanf by
		       //the agent will fail on a NULL name, so we will store "(UnKnown)"
		       //instead and deal with that on the scanf-side
		    WCHAR ssname[MAX_PATH];
			wcscpy(ssname, node->GetAcctName());
		    if (!wcslen(ssname))
		       wcscpy(ssname, GET_STRING(IDS_UnknownSid));
		    WCHAR stname[MAX_PATH];
			wcscpy(stname, node->GetTargetAcctName());
		    if (!wcslen(stname))
		       wcscpy(stname, GET_STRING(IDS_UnknownSid));
		    WCHAR ssdname[MAX_PATH];
			wcscpy(ssdname, node->GetSrcDomName());
		    if (!wcslen(ssdname))
		       wcscpy(ssdname, GET_STRING(IDS_UnknownSid));
		    WCHAR stdname[MAX_PATH];
			wcscpy(stdname, node->GetTgtDomName());
		    if (!wcslen(stdname))
		       wcscpy(stdname, GET_STRING(IDS_UnknownSid));
         
            fwprintf(pFile,L"%s\t%s\t%s\t%lx\t%lx\t%lx\t%s\t%s\t%s\t%s\r\n",ssname,stname,type,
                                                   node->GetStatus(), node->SrcRid(), node->TgtRid(), node->GetSrcDomSid(), node->GetTgtDomSid(), 
												   ssdname, stdname);
         }
         else
         {
            fwprintf(pFile,L"%s\t%s\t%s\t%lx\t%lx\t%lx\r\n",node->GetAcctName(),node->GetTargetAcctName(),type,
                                                   (DWORD)node->GetStatus(), node->SrcRid(), node->TgtRid());
         }
      }
      e.Close();
      fclose(pFile);
   }
   else
   {
      bSuccess = FALSE;
//      DWORD rc = GetLastError();
   }
   return bSuccess;
}
BOOL 
   CSecTranslator::LoadCacheFromFile(
      WCHAR          const * filename,       // in - file to read account mapping from
      IVarSet              * pVarSet         // in - pointer to varset
   )
{
   BOOL                      bSuccess = TRUE;
   _bstr_t                   text;
   FILE                    * pFile;
   WCHAR                     sourceName[LEN_Account];
   WCHAR                     targetName[LEN_Account];
   WCHAR                     sourceDomSid[MAX_PATH];
   WCHAR                     targetDomSid[MAX_PATH];
   WCHAR                     sourceDomName[MAX_PATH];
   WCHAR                     targetDomName[MAX_PATH];
   WCHAR                     type[LEN_Account];
   DWORD                     status;
   int                       count = 0;
   BOOL                      bNeedRids = FALSE;
   WCHAR                     path[MAX_PATH];
   WCHAR                     temp[MAX_PATH];
   BOOL						 bUseMapFile = FALSE;
   WCHAR*                    pFullPathBuffer = NULL;
   
   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT));
   if (!text || UStrICmp(text,GET_STRING(IDS_YES)))
   {
      m_Args.SetUsingMapFile(TRUE); //set the arg flag to indicate use of map file
      bUseMapFile = TRUE;
   }
   
   if ( m_LocalOnly )
   {
      // we need to set source and target domain information for the cache
      // if we're not using migrated object table
      if (!bUseMapFile)
        m_Args.Cache()->SetSourceAndTargetDomainsWithSids(m_Args.Source(),m_SourceSid,m_Args.Target(),m_TargetSid);  
      
      // find the module path
      DWORD          rc = GetModuleFileName(NULL,temp,DIM(temp));
      if ( rc )
      {
         // Generally, our DCTCache file will be in the same directory as our EXE.
         // This is true 1) when agent is dispatched to clean machine (all will be in OnePointDomainAgent directory)
         // and also 2) when agent is dispatched to the local ADMT machine (all will be in Program Files\ADMT directory)
         // The exception is when the agent is dispatched to a remote machine where ADMT is also installed.
         WCHAR * slash = wcsrchr(temp,L'\\');
         UStrCpy(slash+1,filename);
         // Check whether ADMT is locally installed here
         if ( IsLocallyInstalled() && !IsThisDispatcherMachine(pVarSet) )
         {
            // ADMT is installed here, so we're running from the binaries
            // in the Program files\ADMT directory
            // However, our cache file should be in %Program Files%\\OnePOintDomainAgent
            GetProgramFilesDirectory(temp,NULL);
            UStrCpy(temp+UStrLen(temp),L"\\OnePointDomainAgent\\");
            UStrCpy(temp+UStrLen(temp),filename);
         }
      }
      else
      {
         rc = GetLastError();
         err.DbgMsgWrite(0,L"Couldn't get the module filename, rc=%ld",rc);
         swprintf(temp,L"..\\OnePointDomainAgent\\%ls",filename);
      }
      pFullPathBuffer = _wfullpath(path,temp,MAX_PATH);     
      if(!pFullPathBuffer)
      {
          // we should bail out here, since we could not get the file path name
          err.MsgWrite(0, DCT_MSG_GET_FULL_PATH_FAILED, temp);
          return FALSE;
       
      }
   }
   else
   {
      // we need to set source and target domain information for the cache
      // if we're not using migrated object table
      if (!bUseMapFile)
          m_Args.Cache()->SetSourceAndTargetDomains(m_Args.Source(),m_Args.Target());
      pFullPathBuffer = _wfullpath(path,filename,MAX_PATH);

      if(!pFullPathBuffer)
      {
          // we should bail out here, since we could not get the file path name
          err.MsgWrite(0, DCT_MSG_GET_FULL_PATH_FAILED, filename);
          return FALSE;
       
      }
   }
   m_Args.Cache()->ToSorted();
   
   // The input file should have the format:
   // SourceName, TargetName, Type, Status [,rid1, rid2]
   
   pFile = _wfopen(path,L"rb");
   if ( pFile )
   {
      int result;
      do 
      {
         DWORD rid1 = 0;
         DWORD rid2 = 0;
         if (!bUseMapFile)
		 {
            result = fwscanf(pFile,L"%[^\t]\t%[^\t]\t%[^\t]\t%lx\t%lx\t%lx\r\n",
			                 sourceName,targetName,type,&status,&rid1,&rid2);

            if ( result < 4 )
               break;
		 }
		 else
		 {
            result = fwscanf(pFile,L"%[^\t]\t%[^\t]\t%[^\t]\t%lx\t%lx\t%lx\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\r]\r\n",
			                 sourceName,targetName,type,&status,&rid1,&rid2,sourceDomSid,targetDomSid,
						     sourceDomName, targetDomName);

            if ( result < 8 )
               break;
		 }

         
         short lType = 0;
         if ( !UStrICmp(type,L"user") )
            lType = EA_AccountUser;
         else if ( ! UStrICmp(type,L"group") )
            lType = EA_AccountGroup;

         if (!bUseMapFile)
			m_Args.Cache()->InsertLast(sourceName,rid1,targetName,rid2,lType, status);
		 else
            m_Args.Cache()->InsertLastWithSid(sourceName,sourceDomSid,sourceDomName,rid1,
			                                  targetName,targetDomSid,targetDomName,rid2,lType, status);
         count++;
         if ( ! rid1 | ! rid2 )
         {
            bNeedRids = TRUE;
         }
      } while ( result >= 4 ); // 4 fields read and assigned

      if ( result )
      {
         err.MsgWrite(ErrS,DCT_MSG_ERROR_READING_INPUT_FILE_S,path);
      }
      err.MsgWrite(0,DCT_MSG_ACCOUNTS_READ_FROM_FILE_DS,count,path);
      fclose(pFile);
   }
   else
   {
      err.MsgWrite(ErrS,DCT_MSG_ERROR_OPENING_FILE_S,path);
      bSuccess = FALSE;
   }

   if ( bSuccess && bNeedRids && ! m_LocalOnly)
   {
      bSuccess = GetRIDsFromEA();
   }
  
   return bSuccess;
}

// This doesn't get RIDs from EA any more, since we have removed dependencies on MCS products.
// Instead, we use the Net APIs to get this information
BOOL CSecTranslator::GetRIDsFromEA()
{
   BOOL                      bSuccess = TRUE;

   // set the cache to a tree sorted by name
   m_Args.Cache()->SortedToScrambledTree();
   m_Args.Cache()->Sort(&CompN);

   // do NQDI to get RIDS for accounts
   DWORD                     rc = 0;
   NET_DISPLAY_USER        * pUser = NULL;
   NET_DISPLAY_GROUP       * pGroup = NULL;
   DWORD                     count = 0;
   DWORD                     resume = 0;
   TRidNode                * pNode = NULL;

   // Get source rids for users
   do 
   {
      rc = NetQueryDisplayInformation(m_Args.Cache()->GetSourceDCName(),1,resume,5000,100000,&count,(void**)&pUser);
      if ( 0 == rc || ERROR_MORE_DATA == rc )
      {
         for ( DWORD i = 0 ; i < count ; i++ )
         {
            // see if this account is in the cache
            pNode = (TRidNode*)m_Args.Cache()->Find(&vNameComp,pUser[i].usri1_name);
            if ( pNode )
            {
               pNode->SrcRid(pUser[i].usri1_user_id);   
            }
         }
         if ( count )
         {
            resume = pUser[count-1].usri1_next_index;
         }
         else
         {
            // no items were returned - get out of here
            break;
         }
         NetApiBufferFree(pUser);
      }
   } while ( rc == ERROR_MORE_DATA );

   count = 0;
   resume = 0;

   // Get source rids for global groups
   do 
   {
      rc = NetQueryDisplayInformation(m_Args.Cache()->GetSourceDCName(),3,resume,5000,100000,&count,(void**)&pGroup);
      if ( 0 == rc || ERROR_MORE_DATA == rc )
      {
         for ( DWORD i = 0 ; i < count ; i++ )
         {
            // see if this account is in the cache
            pNode = (TRidNode*)m_Args.Cache()->Find(&vNameComp,pGroup[i].grpi3_name);
            if ( pNode )
            {
               pNode->SrcRid(pGroup[i].grpi3_group_id);   
            }
         }
         if ( count )
         {
            resume = pGroup[count-1].grpi3_next_index;
         }
         else
         {
            // no items were returned - get out of here
            break;
         }
         NetApiBufferFree(pGroup);
      }
   } while ( rc == ERROR_MORE_DATA );

   count = 0;
   resume = 0;
   
   // Get target rids for users
   // set the cache to a tree sorted by target name
   m_Args.Cache()->ToSorted();
   m_Args.Cache()->SortedToScrambledTree();
   m_Args.Cache()->Sort(&CompTargetN);

   do 
   {
      rc = NetQueryDisplayInformation(m_Args.Cache()->GetTargetDCName(),1,resume,5000,100000,&count,(void**)&pUser);
      if ( 0 == rc || ERROR_MORE_DATA == rc )
      {
         for ( DWORD i = 0 ; i < count ; i++ )
         {
            // see if this account is in the cache
            pNode = (TRidNode*)m_Args.Cache()->Find(&vTargetNameComp,pUser[i].usri1_name);
            if ( pNode )
            {
               pNode->TgtRid(pUser[i].usri1_user_id);   
            }
         }
         if ( count )
         {
            resume = pUser[count-1].usri1_next_index;
         }
         else
         {
            // no items were returned - get out of here
            break;
         }
         NetApiBufferFree(pUser);
      }
   } while ( rc == ERROR_MORE_DATA );


   
   // TODO:  Add error message if rc != 0

   count = 0;
   resume = 0;
   // Get target rids for global groups
   do 
   {
      rc = NetQueryDisplayInformation(m_Args.Cache()->GetTargetDCName(),3,resume,5000,100000,&count,(void**)&pGroup);
      if ( 0 == rc || ERROR_MORE_DATA == rc )
      {
         for ( DWORD i = 0 ; i < count ; i++ )
         {
            // see if this account is in the cache
            pNode = (TRidNode*)m_Args.Cache()->Find(&vTargetNameComp,pGroup[i].grpi3_name);
            if ( pNode )
            {
               pNode->TgtRid(pGroup[i].grpi3_group_id);   
            }
         }
         if ( count )
         {
            resume = pGroup[count-1].grpi3_next_index;
         }
         else
         {
            // no items were returned - get out of here
            break;
         }
         NetApiBufferFree(pGroup);
      }
   } while ( rc == ERROR_MORE_DATA );


   // sort back to regular source name order
   m_Args.Cache()->ToSorted();
   m_Args.Cache()->SortedToScrambledTree();
   m_Args.Cache()->Sort(&CompN);


   // get source and target rids for local groups
   TNodeTreeEnum             tEnum;
   BYTE                      sid[LEN_SID];
   DWORD                     lenSid;
   WCHAR                     domain[LEN_Domain];
   DWORD                     lenDomain;
   SID_NAME_USE              snu;

   
   for ( pNode = (TRidNode*)tEnum.OpenFirst(m_Args.Cache()) ; pNode ; pNode = (TRidNode*) tEnum.Next() )
   {
      if ( ! pNode->SrcRid() )
      {
         // we don't have a rid for this account, possibly because it is a local group
         lenSid = DIM(sid);
         lenDomain = DIM(domain);
         if ( LookupAccountName(m_Args.Cache()->GetSourceDCName(),pNode->GetAcctName(),sid,&lenSid,domain,&lenDomain,&snu) )
         {
            if (! UStrICmp(m_Args.Source(),domain) )
            {
               // found the source SID
               // get the last sub-id
               PUCHAR        pCount = GetSidSubAuthorityCount(&sid);
               if ( pCount )
               {
                  LPDWORD    pRid = GetSidSubAuthority(&sid,(*pCount) - 1 );
                  if ( pRid )
                  {
                     pNode->SrcRid(*pRid);
                  }
               }
            }
         }
      }

      if ( pNode->SrcRid() && !pNode->TgtRid() )
      {
         // we got the source RID, now try to get the target RID
         lenSid = DIM(sid);
         lenDomain = DIM(domain);
         if ( LookupAccountName(m_Args.Cache()->GetTargetDCName(),pNode->GetTargetAcctName(),sid,&lenSid,domain,&lenDomain,&snu) )
         {
            if (! UStrICmp(m_Args.Target(),domain) )
            {
               // found the source SID
               // get the last sub-id
               PUCHAR        pCount = GetSidSubAuthorityCount(&sid);

               if ( pCount )
               {
                  LPDWORD    pRid = GetSidSubAuthority(&sid,(*pCount) - 1 );

                  if ( pRid )
                  {
                     pNode->TgtRid(*pRid);
                  }
               }
            }
         }
      }
   }
   tEnum.Close();

   return bSuccess;
}

// We remove the Exchange server service accont from the cache before translating, 
// since it is not recommended to change the service account from exchange
// in any event, the service account for exchange cannot be changed simply by granting
// exchange permissions to the new account.  It also requires configuration changes within
// exchange that must be performed manually
BOOL 
   CSecTranslator::RemoveExchangeServiceAccountFromCache()
{
   WCHAR          const    * exServiceName = L"MSExchangeDS";
   SC_HANDLE                 hSCM; 
   DWORD                     rc = 0;           // return code
   BOOL                      result = FALSE;
   BOOL						 bUseMapFile = m_Args.UsingMapFile();

   if ( m_Args.TranslateContainers() )
   {
      // get the service account name for the exchange directory service on exchServer
//      BOOL                      retval=FALSE; // returned value
      SC_HANDLE                 hSvc;         // Service handle
      DWORD                     lenQsc;       // required qsc info len
     
      union
      {
         QUERY_SERVICE_CONFIG   qsc;          // Exchange Directory service information
         BYTE                   padding[1000];
      }                         bufQsc;
      
      hSCM = OpenSCManager( m_exchServer, NULL, GENERIC_READ );
      if ( !hSCM )
      {
         rc = GetLastError();
         err.SysMsgWrite( ErrW, rc,
               DCT_MSG_SCM_OPEN_FAILED_SD, m_exchServer,rc );
         
      }
      else
      {
         hSvc = OpenService( hSCM, exServiceName, SERVICE_QUERY_CONFIG );
         if ( !hSvc )
         {
            rc = GetLastError();
            switch ( rc )
            {
               case ERROR_SERVICE_DOES_NOT_EXIST: // 1060
               default:
                  err.SysMsgWrite( ErrW, rc, DCT_MSG_OPEN_SERVICE_FAILED_SSD,
                       m_exchServer , exServiceName, rc );
                  break;
            }
         }
         else 
         {
            if ( !QueryServiceConfig( hSvc, &bufQsc.qsc, sizeof bufQsc, &lenQsc ) )
            {
               rc = GetLastError();
               err.SysMsgWrite( ErrW, rc, DCT_MSG_QUERY_SERVICE_CONFIG_FAILED_SSD,
                     m_exchServer, exServiceName, rc );
            }
            else
            {
               // We've found the account
               result = TRUE;
               // bufQsc.qsc.lpServiceStartName is DOMAIN\Account or .\Account
               WCHAR       * domAcct = bufQsc.qsc.lpServiceStartName;
               WCHAR       * domName;  // domain-name
               WCHAR       * acctName; // account-name

               for ( domName = domAcct ; *domName && *domName != _T('\\') ; domName++ )
                  ;
               if ( *domName == _T('\\') )
               {
                  *domName = 0;
                  acctName = domName+1;
                  domName = domAcct;
               }
               // Is the account from the source domain?
               WCHAR szSourceDomain[LEN_Domain];
               WCHAR wszAccountName[LEN_Account];
               
               safecopy(wszAccountName,acctName);
               
			      //use the domain name from the cache if we are not using a sID mapping
			      //file
               if (!bUseMapFile)
			   {
                  safecopy(szSourceDomain,m_Args.Cache()->GetSourceDomainName());
                  if ( !UStrICmp(domName,szSourceDomain ) )
				  {
                     // if so, is it in the cache?
                     TAcctNode * tnode;
                     TNodeTreeEnum  tEnum;
                     // the cache is a tree, sorted by RID
                     for ( tnode = (TAcctNode *)tEnum.OpenFirst(m_Args.Cache()) ; tnode ; tnode = (TAcctNode *)tEnum.Next() )
					 {
                        if ( !UStrICmp(tnode->GetAcctName(),wszAccountName) )
						{
                           // remove it from the cache, and notify the user
                           err.MsgWrite(ErrW,DCT_MSG_SKIPPING_EXCHANGE_ACCOUNT_SS,domName,acctName);
                           m_Args.Cache()->Remove(tnode);
						}
					 }
                     tEnum.Close();
				  }
			   }//end if not using mapping file
			   else //else using sID mapping file, get the source domain name from the
			   {    //node itself
                     //is this account in the cache?
                  TAcctNode * tnode;
                  TNodeTreeEnum  tEnum;
                     // the cache is a tree, sorted by RID
                  for ( tnode = (TAcctNode *)tEnum.OpenFirst(m_Args.Cache()) ; tnode ; tnode = (TAcctNode *)tEnum.Next() )
				  {
                     if (( !UStrICmp(tnode->GetAcctName(),wszAccountName) ) &&
						 ( !UStrICmp(((TRidNode*)tnode)->GetSrcDomName(),domName) ))
					 {
                           // remove it from the cache, and notify the user
                        err.MsgWrite(ErrW,DCT_MSG_SKIPPING_EXCHANGE_ACCOUNT_SS,domName,acctName);
                        m_Args.Cache()->Remove(tnode);
					 }
				  }
                  tEnum.Close();
			   }//end if using mapping file
               CloseServiceHandle( hSvc );
            }
         }
         CloseServiceHandle(hSCM);
      }
   }
   if ( !result ) 
   {
      // couldn't get the service account name
      err.SysMsgWrite(ErrW,rc,DCT_MSG_CANT_FIND_EXCHANGE_ACCOUNT_SD,
         m_exchServer,rc);
   }
   return result;
}

void 
   CSecTranslator::DoExchangeResolution(
      TSDResolveStats      * stat,          // in - stats object to record stats
      IVarSet              * pVarSet
   )
{

    if (!m_Args.Cache()->IsCancelled())
    {
      TGlobalDirectory          m_exDir;
      _bstr_t                   domain = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Domain));
      _bstr_t                   username = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_UserName));
      _bstr_t                   password = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password));
      _bstr_t                   mode = pVarSet->get(GET_BSTR(DCTVS_Security_TranslationMode));
      _bstr_t                   mbquery = pVarSet->get(L"ExchangeMigration.LdapQuery");
      WCHAR                     query[LEN_Path] = L"(objectClass=*)";
      if ( m_exchServer[0] )
      {
         if (! RemoveExchangeServiceAccountFromCache() )
            goto end;
      }

      if ( mbquery.length() )
      {
         UStrCpy(query,(WCHAR*)mbquery);
      }
      if ( m_Args.TranslateMailboxes() || m_Args.TranslateContainers() )
      {
          // make sure we have some accts in the cache
         m_exDir.SetStats(stat);
         m_Args.Cache()->UnCancel();      
         err.MsgWrite(0,DCT_MSG_EXCHANGE_TRANSLATION_MODE_S,(WCHAR*)mode);
         m_exDir.DoLdapTranslation(m_exchServer,domain,username,password,&m_Args,NULL,query);
         stat->DisplayPath(L"");
         
      } 
    }
    
    if (m_Args.Cache()->IsCancelled())
        err.MsgWrite(0, DCT_MSG_OPERATION_ABORTED_EXCHANGE);
end:
   return;
}


//----------------------------------------------------------------------------
// GetLine Function
//
// Retrieves a single line from a SID mapping file.
//
// Author
// moluper 2001-10-08
//
// Arguments
// IN fp      - pointer to stream i/o file structure
// IN pszLine - pointer to buffer in which a null terminated line from file
//              will be written
// IN cchLine - size of buffer in characters
//
// Return Value
// The return value is true if a valid line from the file was retrived
// otherwise the return value is false.
//----------------------------------------------------------------------------

inline bool __stdcall GetLine(FILE* fp, PTSTR pszLine, int cchLine)
{
    bool bGet = false;

    //
    // until a valid line is retrieved
    //

    while (bGet == false)
    {
        //
        // retrieve line from file
        //

        if (_fgetts(pszLine, cchLine, fp) == NULL)
        {
            break;
        }

        //
        // check if line length is valid
        //

        int cch = _tcslen(pszLine);

        if ((cch < (cchLine - 1)) || (pszLine[cch - 1] == L'\r') || (pszLine[cch - 1] == L'\n'))
        {
            //
            // the line is less than or equal to the maximum length
            // remove carriage return and linefeed characters from end of line
            //

            while ((cch > 0) && ((pszLine[cch - 1] == L'\r') || (pszLine[cch - 1] == L'\n')))
            {
                pszLine[--cch] = _T('\0');
            }

            bGet = true;
        }
        else
        {
            //
            // the line is greater than maximum length
            //
            // only display beginning of line with trailing ...
            //

            int ich = cchLine / 4;

            pszLine[ich++] = _T('.');
            pszLine[ich++] = _T('.');
            pszLine[ich++] = _T('.');
            pszLine[ich++] = _T('\0');

            err.MsgWrite(ErrW, DCT_MSG_SID_MAPPING_FILE_LINE_TOO_LONG_S, pszLine);

            if (useErrAlt)
            {
                errAlt.MsgWrite(ErrW, DCT_MSG_SID_MAPPING_FILE_LINE_TOO_LONG_S, pszLine);
            }

            // skip rest of line

            do
            {
                if (_fgetts(pszLine, cchLine, fp) != NULL)
                {
                    cch = _tcslen(pszLine);
                }
                else
                {
                    break;
                }
            }
            while ((pszLine[cch - 1] != L'\r') && (pszLine[cch - 1] != L'\n'));
        }
    }

    return bGet;
}

//----------------------------------------------------------------------------
// TrimTrailingSpaces
//
// Removes white space characters from end of string.
//
// Author
// moluper 2001-10-08
//
// Arguments
// IN psz - pointer to buffer in which trailing whitespace characters are
//          replaced with null characters in place
//----------------------------------------------------------------------------

inline void __stdcall TrimTrailingSpaces(PTSTR psz)
{
    for (PTSTR pch = psz + _tcslen(psz) - 1; (pch >= psz) && _istspace(*pch); pch--)
    {
        *pch = L'\0';
    }
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 4 OCT 2000                                                  *
 *                                                                   *
 *     This function is responsible for retrieving account sIDs from *
 * the given sID mapping file and adding these sids to the cache.    *
 *                                                                   *
 *********************************************************************/

//BEGIN LoadCacheFromMapFile
BOOL 
CSecTranslator::LoadCacheFromMapFile(
    WCHAR          const * filename,       // in - file to read sid mapping from
    IVarSet              * pVarSet         // in - pointer to varset
)
{
    FILE                    * pFile = NULL;
    WCHAR                     szLine[2 * MAX_PATH];
    WCHAR                     path[MAX_PATH];
    WCHAR                     szFmt1[64];
    WCHAR                     szFmt2[64];
    WCHAR                     sourceSid[MAX_PATH];
    WCHAR                     targetSid[MAX_PATH];
    int                       count = 0;
    BOOL                      bSuccess = TRUE;

    if (useErrAlt)
        errAlt.MsgWrite(0, DCT_MSG_PROCESSING_SID_MAPPING_FILE, filename);

    _wfullpath(path,filename,MAX_PATH);

    m_Args.Cache()->ToSorted();

    // The input file should have the format:
    // srcSid,tgtSid
    pFile = OpenMappingFile(path);   

    if ( pFile )
    {
        //
        // initialize format strings so that scanf does not exceed
        // size of sourceSid and targetSid buffers
        //

        wsprintf(szFmt1, L" %%%d[^,\r\n]", MAX_PATH - 1);
        wsprintf(szFmt2, L" %%%d[^,] , %%%d[^\r\n]", MAX_PATH - 1, MAX_PATH - 1);

        //
        // read account mapping entries until the end of the file is reached
        //

        bool bRemoveMode = (m_Args.TranslationMode() == REMOVE_SECURITY);

        for (;;) 
        {
            //
            // retrieve line from file
            //

            if (GetLine(pFile, szLine, sizeof(szLine) / sizeof(szLine[0]) - 1) == false)
            {
                // if an error reading the file occurred log an error message
                // and set return value to unsuccessful

                int nError = ferror(pFile);

                if (nError != 0)
                {
                    err.MsgWrite(ErrE, DCT_MSG_ERROR_READING_INPUT_FILE_S, path);
                    if (useErrAlt)
                        errAlt.MsgWrite(ErrE, DCT_MSG_ERROR_READING_INPUT_FILE_S, path);
                    bSuccess = FALSE;
                }
                break;
            }

            //
            // parse account name(s) or SID(s)
            // the comma character is the only valid separator between the source and target accounts
            //

            int cFields = 0;

            if (bRemoveMode)
            {
                //
                // remove mode only requires source account
                //

                cFields = swscanf(szLine, szFmt1, sourceSid);

                if (cFields >= 1)
                {
                    TrimTrailingSpaces(sourceSid);
                    targetSid[0] = L'\0';
                }
                else
                {
                    continue;
                }
            }
            else
            {
                //
                // add and replace modes require both source and target accounts
                //

                cFields = swscanf(szLine, szFmt2, sourceSid, targetSid);

                if (cFields >= 2)
                {
                    TrimTrailingSpaces(sourceSid);
                    TrimTrailingSpaces(targetSid);
                }
                else
                {
                    // only log error if not blank line

                    if (cFields == 1)
                    {
                        err.MsgWrite(ErrW, DCT_MSG_SID_MAPPING_FILE_UNABLE_TO_PARSE_S, szLine);
                        if (useErrAlt)
                            errAlt.MsgWrite(ErrW, DCT_MSG_SID_MAPPING_FILE_UNABLE_TO_PARSE_S, szLine);
                    }

                    continue;
                }
            }

            short lType = EA_AccountUser;

            //divide the sids into domain sids and rids
            WCHAR    srcDomainSid[MAX_PATH] = L"";
            WCHAR    tgtDomainSid[MAX_PATH] = L"";
            _bstr_t  srcDomainName = L"";
            _bstr_t  tgtDomainName = L"";
            DWORD    srcRid = 0;
            DWORD    tgtRid = 0;
            _bstr_t  srcName = L"";
            _bstr_t  tgtName = L"";
            WCHAR	  userName[MAX_PATH] = L"";
            WCHAR	  domainName[MAX_PATH] = L"";
            DWORD	  cb = MAX_PATH;
            DWORD    cbDomain = MAX_PATH;
            SID_NAME_USE	sid_Use;
            PSID     srcSid = NULL;
            PSID	  tgtSid = NULL;
            WCHAR  * slash;
            _bstr_t  DCName;
            BYTE     ssid[200];
            BYTE     tsid[200];
            DWORD    lenSid = DIM(ssid);
            BOOL	  bNeedToFreeSrc = FALSE;
            BOOL	  bNeedToFreeTgt = FALSE;

            //see if the source is given by domain\account format or 
            //decimal-style sid format
            if (wcschr(sourceSid,L'\\'))
            {
                //seperate domain and account names
                srcDomainName = sourceSid;
                srcName = sourceSid;
                slash = wcschr((WCHAR*)srcDomainName,L'\\');
                if ( slash )
                    *slash = 0;
 
                //get a DC for the given domain
                DCName = GetADC(srcDomainName); 
                if (!DCName.length())
                {
                    err.MsgWrite(0,DCT_MSG_SRC_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, sourceSid);
                    if (useErrAlt)
                        errAlt.MsgWrite(ErrW,DCT_MSG_SRC_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, sourceSid);
                    continue;
                }

                  //get the sid for this account
                if(!LookupAccountName(DCName,srcName,(PSID)ssid,&lenSid,domainName,&cbDomain,&sid_Use))
                {
                    err.MsgWrite(0,DCT_MSG_SRC_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, sourceSid);
                    if (useErrAlt)
                        errAlt.MsgWrite(ErrW,DCT_MSG_SRC_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, sourceSid);
                    continue;
                }

                srcSid = (PSID)ssid;

                if (sid_Use == SidTypeGroup)
                    lType = EA_AccountGroup;
                else
                    lType = EA_AccountUser;
            }//end if domain\account format
            else
            {
                srcSid = SidFromString(sourceSid);
                if (!srcSid)
                {
                    err.MsgWrite(0,DCT_MSG_SRC_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, sourceSid);
                    if (useErrAlt)
                        errAlt.MsgWrite(ErrW,DCT_MSG_SRC_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, sourceSid);
                    continue;
                }

                bNeedToFreeSrc = TRUE;

                if (LookupAccountSid(NULL, srcSid, userName, &cb, domainName, &cbDomain, &sid_Use))
                {
                    srcName = userName;
                    srcDomainName = domainName;
                    if (sid_Use == SidTypeGroup)
                        lType = EA_AccountGroup;
                    else
                        lType = EA_AccountUser;
                }
            }//end else sid format

            //if target given
            if (cFields > 1)
            {
                //see if the target is given by domain\account format or 
                //decimal-style sid format
                lenSid = DIM(tsid);
                cb = cbDomain = MAX_PATH;
                if (wcschr(targetSid,L'\\'))
                {
                    //seperate domain and account names
                    tgtDomainName = targetSid;
                    tgtName = targetSid;
                    slash = wcschr((WCHAR*)tgtDomainName,L'\\');
                    if ( slash )
                        *slash = 0;

                    //get a DC for the given domain
                    DCName = GetADC(tgtDomainName); 
                    if (!DCName.length())
                    {
                        if (bNeedToFreeSrc)
                            FreeSid(srcSid);
                        err.MsgWrite(0,DCT_MSG_TGT_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, targetSid);
                        if (useErrAlt)
                            errAlt.MsgWrite(ErrW,DCT_MSG_TGT_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, targetSid);
                        continue;
                    }

                    //get the sid for this account
                    if(!LookupAccountName(DCName,tgtName,(PSID)tsid,&lenSid,domainName,&cbDomain,&sid_Use))
                    {
                        if (bNeedToFreeSrc)
                            FreeSid(srcSid);
                        err.MsgWrite(0,DCT_MSG_TGT_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, targetSid);
                        if (useErrAlt)
                            errAlt.MsgWrite(ErrW,DCT_MSG_TGT_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, targetSid);
                        continue;
                    }

                    tgtSid = (PSID)tsid;

                    if (sid_Use == SidTypeGroup)
                        lType = EA_AccountGroup;
                    else
                        lType = EA_AccountUser;
                }//end if domain\account format
                else
                {
                    tgtSid = SidFromString(targetSid);
                    if (!tgtSid)
                    {
                        if (bNeedToFreeSrc)
                            FreeSid(srcSid);
                        err.MsgWrite(0,DCT_MSG_TGT_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, targetSid);
                        if (useErrAlt)
                            errAlt.MsgWrite(ErrW,DCT_MSG_TGT_ACCOUNT_NOT_READ_FROM_FILE_DS, sourceSid, targetSid, path, targetSid);
                        continue;
                    }

                    bNeedToFreeTgt = TRUE;

                    if (LookupAccountSid(NULL, tgtSid, userName, &cb, domainName, &cbDomain, &sid_Use))
                    {
                        tgtName = userName;
                        tgtDomainName = domainName;
                        if (sid_Use == SidTypeGroup)
                            lType = EA_AccountGroup;
                        else
                            lType = EA_AccountUser;
                    }
                }//end else sid format
            }//end if given target account
            else if (m_Args.TranslationMode() == REMOVE_SECURITY) //else if REMOVE mode set target to use source
            {
                tgtSid = srcSid;
                bNeedToFreeTgt = FALSE;
                wcscpy(targetSid, sourceSid);
                tgtName = srcName;
                tgtDomainName = srcDomainName;
            }

            //if the source account is not already in the cache, then add it
            if ((m_Args.Cache()->GetNumAccts() == 0) || (m_Args.Cache()->LookupWODomain(srcSid) == NULL))
            {
                //get the domain sids and account rids from the account sids
                SplitAccountSids(srcSid, srcDomainSid, &srcRid, tgtSid, tgtDomainSid, &tgtRid);

                //insert this node into the cache
                m_Args.Cache()->InsertLastWithSid(srcName,srcDomainSid,srcDomainName,srcRid,tgtName,
                                                                      tgtDomainSid,tgtDomainName,tgtRid,lType);
                count++;
            }
            else
                err.MsgWrite(0,DCT_MSG_SRC_ACCOUNT_DUPLICATE_IN_FILE_DS, sourceSid, targetSid, path, sourceSid);

            if (bNeedToFreeSrc)
                FreeSid(srcSid);
            if (bNeedToFreeTgt)
                FreeSid(tgtSid);
        }

        err.MsgWrite(0,DCT_MSG_ACCOUNTS_READ_FROM_FILE_DS,count,path);
        fclose(pFile);
    }
    else
    {
        err.MsgWrite(ErrS,DCT_MSG_ERROR_OPENING_FILE_S,path);
        if (useErrAlt)
            errAlt.MsgWrite(ErrS,DCT_MSG_ERROR_OPENING_FILE_S,path);
        bSuccess = FALSE;
    }

    return bSuccess;
}
//END LoadCacheFromMapFile 


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 11 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for opening the sid mapping file *
 * whether it is anm ANSI or UNICODE file and return the file        *
 * pointer.                                                          *
 *                                                                   *
 *********************************************************************/

//BEGIN OpenMappingFile
FILE* CSecTranslator::OpenMappingFile(LPCTSTR pszFileName)
{
	// open in binary mode first in order to check for UNICODE byte order
	// mark if the file is UNICODE then it must be read in binary mode
	// with the stream i/o functions

	FILE* fp = _tfopen(pszFileName, _T("rb"));

	if (fp == NULL)
	{
		return NULL;
//		_com_issue_error(E_INVALIDARG);
	}

	// check if file is ANSI or UNICODE or UTF-8

	BYTE byteSignature[3];

	if (fread(byteSignature, sizeof(BYTE), 3, fp) == 3)
	{
		static BYTE byteUtf8[] = { 0xEF, 0xBB, 0xBF };
		static BYTE byteUnicodeLE[] = { 0xFF, 0xFE };
		static BYTE byteUnicodeBE[] = { 0xFE, 0xFF };

		// check for signature or byte order mark

		if (memcmp(byteSignature, byteUtf8, sizeof(byteUtf8)) == 0)
		{
			// UTF-8 signature
			// TODO: not currently supported
		    return NULL;
//			_com_issue_error(E_INVALIDARG);
		}
		else if (memcmp(byteSignature, byteUnicodeLE, sizeof(byteUnicodeLE)) == 0)
		{
			// UNICODE Little Endian Byte Order Mark
			// supported
			// must read in binary mode
			// move file pointer back one byte because we read 3 bytes
			if (fseek(fp, -1, SEEK_CUR))
			{
			    // if unable to move file pointer back, we bail
			    fclose(fp);
			    fp = NULL;
			}
		}
		else if (memcmp(byteSignature, byteUnicodeBE, sizeof(byteUnicodeBE)) == 0)
		{
			// UNICODE Big Endian Byte Order Mark
			// TODO: not currently supported
		    return NULL;
//			_com_issue_error(E_INVALIDARG);
		}
		else
		{
			// assume ANSI
			// re-open file in text mode as the stream i/o functions will
			// treat the file as multi-byte characters and will convert them
			// to UNICODE

			fclose(fp);

			fp = _tfopen(pszFileName, _T("rt"));
		}
	}
	else
	{
		return NULL;
//		_com_issue_error(E_INVALIDARG);
	}

	return fp;
}
//END OpenMappingFile 


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 10 JUNE 2001                                                *
 *                                                                   *
 *     This function is responsible for retrieving a DC from the     *
 * given domain.                                                     *
 *     This function returns the DCName, if retrieved, or an empty   *
 * string.                                                           *
 *                                                                   *
 *********************************************************************/

//BEGIN GetADC
_bstr_t CSecTranslator::GetADC(_bstr_t sDomainName)
{
    _bstr_t strDcName;

    DWORD dwError = GetDcName4(sDomainName, DS_DIRECTORY_SERVICE_PREFERRED, strDcName);

    return (dwError == ERROR_SUCCESS) ? strDcName : _T("");
}
//END GetADC 
