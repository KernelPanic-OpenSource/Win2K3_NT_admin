/****************************************************************************
Copyright information		: Copyright (c) 1998-2002 Microsoft Corporation 
File Name					: helpers.cpp 
Project Name				: WMI Command Line
Author Name					: Ch.Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file has all the global function definitions 
Revision History			:
		Last Modified By	: Ch Sriramachandramurthy
		Last Modified Date  : 03-March-2001
*****************************************************************************/ 
#include "Precomp.h"
#include "CommandSwitches.h"
#include "GlobalSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"
#include "CmdTokenizer.h"
#include "CmdAlias.h"
#include "ErrorInfo.h"
#include "WmiCliXMLLog.h"
#include "ParserEngine.h"
#include "ExecEngine.h"
#include "FormatEngine.h"
#include "WmiCmdLn.h"

/*----------------------------------------------------------------------------
   Name				 :CompareTokens
   Synopsis	         :It compares the two tokens passed to it as input 
					  arguments and returns a BOOL value ,
					  TRUE if they are equal 
					  FALSE if not equal.
   Type	             :Global Function
   Input parameter(s):
			pszToken1- String type, Contains the first string of the two 
					   string to be compared
			pszToken2- String type, Contains the second string of the two 
					   string to be compared
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :CompareTokens(pszToken1,pszToken2)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CompareTokens(_TCHAR* pszToken1, _TCHAR* pszToken2)
{	
	return (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, 
					NORM_IGNORECASE | NORM_IGNOREWIDTH,	
					pszToken1, (pszToken1) ? lstrlen(pszToken1) : 0,
					pszToken2, (pszToken2) ? lstrlen(pszToken2) : 0)) 
					? TRUE : FALSE;
}

/*----------------------------------------------------------------------------
   Name				 :CompareTokensChars
   Synopsis	         :It compares the two tokens passed to it as input 
					  arguments and returns a BOOL value ,
					  TRUE if they are equal 
					  FALSE if not equal.
   Type	             :Global Function
   Input parameter(s):
			pszToken1- String type, Contains the first string of the two 
					   string to be compared
			pszToken2- String type, Contains the second string of the two 
					   string to be compared
		    cchToken-  number of characters to compare
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :CompareTokensChars(pszToken1,pszToken2,cchToken)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CompareTokensChars(_TCHAR* pszToken1, _TCHAR* pszToken2, DWORD cchToken)
{	
	DWORD cchSize = cchToken ;
	cchSize = min ( (pszToken1) ? lstrlen(pszToken1) : 0, cchSize ) ;
	cchSize = min ( (pszToken2) ? lstrlen(pszToken2) : 0, cchSize ) ;

	return	(CSTR_EQUAL == CompareString	(	LOCALE_SYSTEM_DEFAULT, 
												NORM_IGNORECASE | NORM_IGNOREWIDTH,	
												pszToken1,
												cchSize,
												pszToken2,
												cchSize
											) 
			) ? TRUE : FALSE;
}

/*----------------------------------------------------------------------------
   Name				 :Connect
   Synopsis	         :To connect to WMI namespace on a given server  with the
					  set of input user credentials.
   Type	             :Global Function
   Input parameter(s):
		pILocator	 -	Locator object 
		bstrNS		 -	Namespae to be connected to 
		bstrUser	 -	User name with which to connect 
		bstrPwd		 -	Password  for the user name  with which to connect
		bstrLocale	 -	Locale specified 
   Output parameters :
			pISvc	 -	WMI services object
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :Connect(pILocator, &pISvc, bstrNS, bstrUser, bstPwd,
							  bstrLocale)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT Connect(IWbemLocator* pILocator, IWbemServices** pISvc,
				BSTR bstrNS, BSTR bstrUser, BSTR bstrPwd, 
				BSTR bstrLocale, CParsedInfo& rParsedInfo)
{
	HRESULT hr						= S_OK;
	BSTR	bstrPass				= NULL;
	BSTR	bstrAuthorityPrinciple  = NULL;

	// If the user name is not NULL and the password is 
	// a) NULL, treat the password as BLANK
	// b) not NULL, treat the password as it is.
	try 
	{
		// Get the <authority principle> specified, and pass in with ConnectServer
		if(NULL != rParsedInfo.GetAuthorityPrinciple())
		{
			bstrAuthorityPrinciple =
				::SysAllocString(rParsedInfo.GetAuthorityPrinciple());
			if (bstrAuthorityPrinciple == NULL)
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}
		
		if (bstrUser)
		{
			if (!bstrPwd)
			{
				bstrPass = ::SysAllocString(L"");
				if (bstrPass == NULL)
					throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

				// Set the credentials flag to TRUE - BLANK password
				rParsedInfo.GetCmdSwitchesObject().SetCredentialsFlag(TRUE);
			}
		}
		
		if (pILocator != NULL)
		{	
			// Call the ConnectServer method of the IWbemLocator
			hr = pILocator->ConnectServer(bstrNS, 
							bstrUser, 
							(!bstrPass) ? bstrPwd : bstrPass, 
							bstrLocale,	
							0L,	bstrAuthorityPrinciple, NULL,	pISvc);

			// If the username is specified and the password is not 
			// specified (the remote machine also has the same password)
			if (FAILED(hr) && bstrUser && (hr == E_ACCESSDENIED))
			{

				hr = pILocator->ConnectServer(bstrNS, 
								bstrUser, 
								NULL, 
								bstrLocale,	
								0L,	bstrAuthorityPrinciple, NULL,	pISvc);

				// Set the credentials flag to FALSE - NULL password
				rParsedInfo.GetCmdSwitchesObject().SetCredentialsFlag(FALSE);
			}
		}
		else
			hr = E_FAIL;

		if (bstrPass)
			::SysFreeString(bstrPass);
		if (bstrAuthorityPrinciple)
			::SysFreeString(bstrAuthorityPrinciple);
	}
	catch(CHeap_Exception)
	{
		if (bstrAuthorityPrinciple)
			::SysFreeString(bstrAuthorityPrinciple);
		if (bstrPass)
			::SysFreeString(bstrPass);
		hr = E_FAIL;
	}

	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :SetSecurity
   Synopsis	         :To set the security privileges at the interface level
   Type	             :Global Function
   Input parameter(s):
		pIUnknown	 -	Interface pointer
		pszDomain	 -	domain name
		pszAuthority -	authority (always passed as NULL)
		pszUser		 -	Username
		pszPassword	 -	Password
		uAuthLevel	 -	Authentication Level
		uImpLevel	 -	Impersonation Level
   Output parameter(s):None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :SetSecurity(pIUnknown, pszDomain, pszUser, 
								  pszPassword, uAuthLevel, uImpLevel)
   Notes             :(partly taken from WMI VC samples 'utillib'
----------------------------------------------------------------------------*/
HRESULT SetSecurity(IUnknown* pIUnknown, _TCHAR* pszAuthority, 
					_TCHAR* pszDomain, _TCHAR* pszUser, 
					_TCHAR* pszPassword, UINT uAuthLevel, 
					UINT uImpLevel) throw(WMICLIINT)
{
	SCODE						sc				= S_OK;
	BSTR						bstrAuthArg		= NULL, 
								bstrUserArg		= NULL;
	SEC_WINNT_AUTH_IDENTITY_W*	pAuthIdentity	= NULL;
	try
	{
		if(pIUnknown == NULL)
			return E_INVALIDARG;

		// If we are lowering the security, no need to deal with the 
		// identification information
		if(uAuthLevel == RPC_C_AUTHN_LEVEL_NONE)
			return CoSetProxyBlanket(pIUnknown, RPC_C_AUTHN_WINNT, 
							RPC_C_AUTHZ_NONE,
							NULL, RPC_C_AUTHN_LEVEL_NONE, 
							RPC_C_IMP_LEVEL_IDENTIFY,
							NULL, EOAC_NONE);

		// If we are doing trivial case, just pass in a null authentication 
		// structure which is used if the current logged in user's credentials
		// are OK.
		if((pszAuthority == NULL || lstrlen((LPCTSTR)pszAuthority) < 1) && 
			(pszUser == NULL || lstrlen((LPCTSTR)pszUser) < 1) && 
			(pszPassword == NULL || lstrlen((LPCTSTR)pszPassword) < 1))
				return CoSetProxyBlanket(pIUnknown, RPC_C_AUTHN_WINNT, 
							RPC_C_AUTHZ_NONE, NULL, uAuthLevel,
							uImpLevel, NULL, EOAC_NONE);

		// If user, or Authority was passed in, the we need 
		// to create an authority argument for the login
		sc = ParseAuthorityUserArgs(bstrAuthArg, bstrUserArg, 
						pszAuthority, pszUser);
		if(FAILED(sc))
			return sc;
		
		pAuthIdentity = new SEC_WINNT_AUTH_IDENTITY_W;

		// Check whether the memory allocation has been successful
		if (pAuthIdentity == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		ZeroMemory(pAuthIdentity, sizeof(SEC_WINNT_AUTH_IDENTITY_W));

		if(bstrUserArg)
		{
			WMICLIULONG wulUserLen = (WMICLIULONG)
												 lstrlen((LPWSTR)bstrUserArg);
			pAuthIdentity->User = new WCHAR [wulUserLen + 1];
			if (pAuthIdentity->User == NULL)
				throw OUT_OF_MEMORY;
			wcscpy(pAuthIdentity->User, (LPWSTR)bstrUserArg);
			pAuthIdentity->UserLength = wulUserLen;
		}
		if(bstrAuthArg)
		{
			WMICLIULONG wulDomainLen = (WMICLIULONG)
												lstrlen((LPWSTR) bstrAuthArg);
			pAuthIdentity->Domain = new WCHAR [wulDomainLen + 1];
			if (pAuthIdentity->Domain == NULL)
				throw OUT_OF_MEMORY;
			wcscpy(pAuthIdentity->Domain, (LPWSTR)bstrAuthArg);
			pAuthIdentity->DomainLength = wulDomainLen;
		}

		if(pszPassword)
		{
			WMICLIULONG wulPasswordLen = (WMICLIULONG)
												lstrlen((LPWSTR) pszPassword);
			pAuthIdentity->Password = new WCHAR [wulPasswordLen + 1];
			if (pAuthIdentity->Password == NULL)
				throw OUT_OF_MEMORY;
			wcscpy(pAuthIdentity->Password, pszPassword);
			pAuthIdentity->PasswordLength= wulPasswordLen;
		}
		pAuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

		if( pszAuthority != NULL &&
			_tcslen(pszAuthority) > 9 &&
			_tcsnicmp(pszAuthority, _T("KERBEROS:"), 9) == 0)
		{
			// get the <principal name> that is passed with <authority type>
			// and send it to CoSetProxyBlanket()
			BSTR	bstrPrincipalName = ::SysAllocString(&pszAuthority[9]);

			sc = CoSetProxyBlanket(pIUnknown,
						   RPC_C_AUTHN_GSS_KERBEROS,
						   RPC_C_AUTHZ_NONE ,
						   bstrPrincipalName,
						   uAuthLevel,
						   uImpLevel,
						   pAuthIdentity,
						   EOAC_NONE);

			SAFEBSTRFREE(bstrPrincipalName);
		}
		else
		{
			sc = CoSetProxyBlanket(pIUnknown, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
							NULL, uAuthLevel, uImpLevel, pAuthIdentity, EOAC_NONE);
		}

		SAFEDELETE(pAuthIdentity->User);
		SAFEDELETE(pAuthIdentity->Domain);
		SAFEDELETE(pAuthIdentity->Password);
		delete pAuthIdentity;
		SAFEBSTRFREE(bstrUserArg);
		SAFEBSTRFREE(bstrAuthArg);
	}
	catch(WMICLIINT nErr)
	{
		if (nErr == OUT_OF_MEMORY)
			sc = WBEM_E_OUT_OF_MEMORY;
		SAFEDELETE(pAuthIdentity->User);
		SAFEDELETE(pAuthIdentity->Domain);
		SAFEDELETE(pAuthIdentity->Password);
		if (pAuthIdentity)
			delete pAuthIdentity;
		SAFEBSTRFREE(bstrUserArg);
		SAFEBSTRFREE(bstrAuthArg);
	}
    return sc;
}

/*----------------------------------------------------------------------------
   Name				 :ConvertWCToMBCS
   Synopsis	         :Converts the wide character string to MBCS string 
					  after applying the codepage settings
   Type	             :Global Function 
   Input parameters  :
			lpszMsg  - string  (widechar string)
			uCP		 - codepage value	
   Output parameters :
			lpszDisp - string  (multibyte string)
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ConvertWCToMBCS(lpszMsg, lpszDisp, uCP)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL ConvertWCToMBCS(LPTSTR lpszMsg, LPVOID* lpszDisp, UINT uCP)
{
	BOOL bRet = TRUE;

	if (lpszMsg != NULL && lpszDisp != NULL)
	{
		WMICLIINT nRet = 0;		
		nRet = WideCharToMultiByte(uCP,		// code page
						 0,					// performance and mapping flags
						 (LPCWSTR)lpszMsg,	// wide-character string
						 -1,				// number of chars in string
						 NULL,				// buffer for new string
						 0,					// size of buffer	
						 NULL,				// default for unmappable chars
						 NULL);	

		// allocate memory to hold the message
		*lpszDisp = (LPSTR) new char [nRet]; 

		if (*lpszDisp)
		{
			nRet = WideCharToMultiByte(uCP,		// code page
							 0,					// performance and mapping flags
							 (LPCWSTR)lpszMsg,	// wide-character string
							 -1,				// number of chars in string
							 (LPSTR) *lpszDisp,	// buffer for new string
							 nRet,				// size of buffer	
							 NULL,				// default for unmappable chars
							 NULL);	
		}
		else
			bRet = FALSE;
	}
	else
	{
		if ( lpszDisp )
		{
			*lpszDisp = NULL;
		}
	}

	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :ConvertMBCSToWC
   Synopsis	         :Converts the MBCS string to wide character string
					  after applying the codepage settings
   Type	             :Global Function 
   Input parameters  :
			lpszMsg  - string  (MBCS string)
			uCP		 - codepage value	
   Output parameters :
			lpszDisp - string  (multibyte string)
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ConvertMBCSToWC(lpszMsg, lpszDisp, uCP)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL ConvertMBCSToWC(LPSTR lpszMsg, LPVOID* lpszDisp, UINT uCP)
{
	BOOL bRet = TRUE;

	if (lpszMsg != NULL && lpszDisp != NULL)
	{
		WMICLIINT nRet = 0;		
		nRet = MultiByteToWideChar	(
										uCP,				// code page
										0,					// performance and mapping flags
										(LPCSTR)lpszMsg,	// wide-character string
										-1,					// number of chars in string
										NULL,				// buffer for new string
										0
									);	

		// allocate memory to hold the message
		*lpszDisp = (LPWSTR) new WCHAR [nRet]; 

		if (*lpszDisp)
		{
			nRet = MultiByteToWideChar	(
											uCP,				// code page
											0,					// performance and mapping flags
											(LPCSTR)lpszMsg,	// wide-character string
											-1,					// number of chars in string
											(LPWSTR) *lpszDisp,	// buffer for new string
											nRet				// size of buffer	
										);	
		}
		else
			bRet = FALSE;
	}
	else
	{
		if ( lpszDisp )
		{
			*lpszDisp = NULL;
		}
	}

	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :Revert_mbtowc
   Synopsis	         :Reverts string when created by mbtowc
   Type	             :Global Function 
   Input parameters  :
			wszBuffer	- string
   Output parameters :
			szBuffer	- output	
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :Revert_mbtowc(wszBuffer, &szBuffer)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL Revert_mbtowc ( LPCWSTR wszBuffer, LPSTR* szBuffer )
{
	BOOL bRet = FALSE ;

	LPWSTR help  = const_cast < LPWSTR > ( wszBuffer ) ;
	int    helpi = lstrlen ( wszBuffer ) ;

	(*szBuffer) = new char [ helpi + 1 ] ;
	if ( NULL != (*szBuffer) )
	{
		for ( int i = 0; i < helpi+1; i++ )
		{
			(*szBuffer)[ i ] = 0 ;
		}

		for ( int i = 0; i < helpi; i++ )
		{
			wctomb ( & (*szBuffer)[i], *help ) ;
			help++;
		}

		bRet = TRUE ;
	}

	return bRet ;
}

/*----------------------------------------------------------------------------
   Name				 :Find
   Synopsis	         :Searches for a given string in the vector list
   Input parameter(s):
		cvVector	 -	vector list
		pszStrToFind -  serach string
   Output parameter(s):None
   Return Type       :BOOL
   		TRUE		 - if match is found
		FALSE		 - if no match found
   Global Variables  :None
   Calling Syntax    :Find(cvVector, pszStrToFind)
   Notes             :overloaded function
----------------------------------------------------------------------------*/
BOOL Find(CHARVECTOR& cvVector,
		  _TCHAR* pszStrToFind,
		  CHARVECTOR::iterator& theIterator)
{
	BOOL bRet = FALSE;
	theIterator = cvVector.begin();
	CHARVECTOR ::iterator theEnd = cvVector.end();
	while (theIterator != theEnd)
	{
		if (CompareTokens(*theIterator, pszStrToFind))
		{
			bRet = TRUE;
			break;
		}
		theIterator++;
	}
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :Find
   Synopsis	         :Find a property name in the property details map
   Type	             :Global Function
   Input parameter(s):
		pdmPropDetMap   - property map
		pszPropToFind   - property to search for
		theIterator     - Iterator
		bExcludeNumbers - Boolean value
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :Find(pdmPropDetMap, pszPropToFind, tempIterator)
   Notes             :overloaded function, 
					  bExcludeNumbers = FALSE by default.
----------------------------------------------------------------------------*/
BOOL Find(PROPDETMAP& pdmPropDetMap, 
		  _TCHAR* pszPropToFind,
		  PROPDETMAP::iterator& theIterator,
		  BOOL bExcludeNumbers)
{
	BOOL bRet = FALSE;
	theIterator = pdmPropDetMap.begin();
	PROPDETMAP::iterator theEnd = pdmPropDetMap.end();
	while (theIterator != theEnd)
	{
		_TCHAR* pszPropName = (*theIterator).first;
		if ( bExcludeNumbers == TRUE )
			pszPropName = pszPropName + 5;

		if (CompareTokens(pszPropName, pszPropToFind))
		{
			bRet = TRUE;
			break;
		}
		theIterator++;
	}
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :Find
   Synopsis	         :Find a property name in the property details map
   Type	             :Global Function
   Input parameter(s):
		bmBstrMap    - BSTR map
		pszStrToFind - property to search for
		theIterator	 - Iterator. 	
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :Find(pdmPropDetMap, pszPropToFind)
   Notes             :overloaded function
----------------------------------------------------------------------------*/
BOOL Find(BSTRMAP& bmBstrMap, 
		  _TCHAR* pszStrToFind,
		  BSTRMAP::iterator& theIterator)
{
	BOOL bRet = FALSE;
	theIterator = bmBstrMap.begin();
	BSTRMAP::iterator theEnd = bmBstrMap.end();
	while (theIterator != theEnd)
	{
		if (CompareTokens((*theIterator).first, pszStrToFind))
		{
			bRet = TRUE;
			break;
		}
		theIterator++;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :FrameFileAndAddToXSLTDetVector
   Synopsis          :Frames the XSL File Path
   Type	             :Global Function
   Input parameter(s):
		stylesheet   - name of XSL File
		keyword      - name of keyword from mapping which could be possibly used
		rParsedInfo  - reference to object of CParsedInfo
   Output parameter(s): None
   Return Type       :BOOL  
   Global Variables  :None
   Calling Syntax    :FrameXSLFilePath(stylesheet, keyword, rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL FrameFileAndAddToXSLTDetVector(LPCWSTR stylesheet,
									LPCWSTR keyword,
									CParsedInfo& rParsedInfo)
{
	BOOL	bRet	  = FALSE;
	_TCHAR* pszBuffer = new _TCHAR [MAX_PATH+1];
	UINT	nSize	  = 0;

	try
	{
		if (pszBuffer != NULL)
		{
			// Obtain the windows system directory		
			nSize = GetSystemDirectory(pszBuffer, MAX_PATH+1);

			if (nSize)
			{
				if (nSize > MAX_PATH)
				{
					SAFEDELETE(pszBuffer);
					pszBuffer =	new _TCHAR [nSize + 1];
				
					if(pszBuffer == NULL)
					{
						throw OUT_OF_MEMORY;
					}

					if (!GetSystemDirectory(pszBuffer, nSize+1))
					{
						SAFEDELETE(pszBuffer);
						throw (::GetLastError());
					}
				}

				_bstr_t bstrPath = pszBuffer;
				SAFEDELETE(pszBuffer);
				bstrPath +=  WBEM_LOCATION;

				//
				// we have %windir%\system32\wbem in bstrPath now
				//

				BOOL bStyleSheet = TRUE;

				_bstr_t bstrFile = bstrPath;

				BSTRMAP::iterator theMapIterator = NULL;
				const BSTRMAP* pMap = g_wmiCmd.GetMappingsMap();
				if ( pMap )
				{
					if ( Find	(
									*( const_cast < BSTRMAP*> ( pMap ) ) ,
									const_cast < LPWSTR > ( keyword ) ,
									theMapIterator
								)
					   )
					{
						bstrFile += (*theMapIterator).second;

						//
						// check for existence
						//
						SmartCloseHandle hCheckFile = CreateFile	(
																		bstrFile,
																		0,
																		FILE_SHARE_READ ,
																		NULL,
																		OPEN_EXISTING,
																		0,
																		NULL
																	);
						if (hCheckFile != INVALID_HANDLE_VALUE)
						{
							bStyleSheet = FALSE;
							bRet = TRUE;
						}
						else
						{
							bstrFile = bstrPath;
						}
					}
				}
				else
				{
					//
					// there is no mapping ?
					// critical and very strange
					// we can still try shipped file
					//
				}

				if ( bStyleSheet && NULL != stylesheet )
				{
					bstrFile += stylesheet;

					//
					// check for existence
					//
					SmartCloseHandle hCheckFile = CreateFile	(
																	bstrFile,
																	0,
																	FILE_SHARE_READ ,
																	NULL,
																	OPEN_EXISTING,
																	0,
																	NULL
																);

					if (hCheckFile != INVALID_HANDLE_VALUE)
					{
						bRet = TRUE;
					}
					else
					{
						bstrFile = bstrPath;
					}
				}

				//
				// stylesheet structure to be added to vector
				// it is used by transform function
				//

				//
				// there is possibly adding "directory path" as path to file
				// hence: LOAD method of DOM will fail during transform
				//
				// this is intented as we are not really checking file validity
				// when /format has used and we wait for transform to deal with it
				//

				XSLTDET xdXSLTDet;
				xdXSLTDet.FileName = bstrFile;
				rParsedInfo.GetCmdSwitchesObject().AddToXSLTDetailsVector(xdXSLTDet);
			}
			else
			{
				SAFEDELETE(pszBuffer);	
				throw (::GetLastError());
			}
			SAFEDELETE(pszBuffer);
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
			bRet = FALSE;
		}
	}
	catch(_com_error& e)
	{
		SAFEDELETE(pszBuffer);
		bRet = FALSE;
		_com_issue_error(e.Error());
	}
	catch (DWORD dwError)
	{
		SAFEDELETE(pszBuffer);
		::SetLastError(dwError);
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(dwError);
		DisplayWin32Error();
		::SetLastError(dwError);
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :FrameFileAndAddToXSLTDetVector
   Synopsis          :Frames the XSL File Path
   Type	             :Global Function
   Input parameter(s):
		pszXSLFile   - the XSL File
		rParsedInfo  - reference to object of CParsedInfo
   Output parameter(s): None
   Return Type       :BOOL  
   Global Variables  :None
   Calling Syntax    :FrameXSLFilePath(pszXSLFile, rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL FrameFileAndAddToXSLTDetVector(XSLTDET& xdXSLTDet,
									CParsedInfo& rParsedInfo)
{
	BOOL	bRet	  = TRUE;
	_TCHAR* pszBuffer = new _TCHAR [MAX_PATH+1];
	UINT	nSize	  = 0;

	try
	{
		if (pszBuffer != NULL)
		{
			// Obtain the windows system directory		
			nSize = GetSystemDirectory(pszBuffer, MAX_PATH+1);

			if (nSize)
			{
				if (nSize > MAX_PATH)
				{
					SAFEDELETE(pszBuffer);
					pszBuffer =	new _TCHAR [nSize + 1];
				
					if(pszBuffer == NULL)
					{
						throw OUT_OF_MEMORY;
					}

					if (!GetSystemDirectory(pszBuffer, nSize+1))
					{
						SAFEDELETE(pszBuffer);
						throw (::GetLastError());
					}
				}

				_bstr_t bstrPath = _bstr_t(pszBuffer);
				SAFEDELETE(pszBuffer);
				// Append the following path
				// if (xdXSLTDet.FileName != NULL)
				if (!(!xdXSLTDet.FileName))
				{
					bstrPath +=  _bstr_t(WBEM_LOCATION) + _bstr_t(xdXSLTDet.FileName);
					xdXSLTDet.FileName = bstrPath;
					rParsedInfo.GetCmdSwitchesObject().AddToXSLTDetailsVector(
													   xdXSLTDet);
				}
			}
			else
			{
				SAFEDELETE(pszBuffer);	
				throw (::GetLastError());
			}
			SAFEDELETE(pszBuffer);
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
			bRet = FALSE;
		}
	}
	catch(_com_error& e)
	{
		SAFEDELETE(pszBuffer);
		bRet = FALSE;
		_com_issue_error(e.Error());
	}
	catch (DWORD dwError)
	{
		SAFEDELETE(pszBuffer);
		::SetLastError(dwError);
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(dwError);
		DisplayWin32Error();
		::SetLastError(dwError);
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :UnquoteString
   Synopsis          :Removes the starting and ending quotes of a string
					  enclosed in double quotes.
   Type	             :Global Function
   Input parameter(s):
		pszString	 - string input
   Output parameter(s): 
   		pszString	 - string input
   Return Type       :void  
   Global Variables  :None
   Calling Syntax    :UnQuoteString(pszString)
   Notes             :None
------------------------------------------------------------------------*/
void UnQuoteString(_TCHAR*& pszString)
{
	if ((lstrlen(pszString) - 1) > 0)
	{

		if(_tcsicmp(pszString, _T("\"NULL\"")) == 0)  return;

		// Check if the string is enclosed within quotes
		if ((pszString[0] == _T('"') && (pszString[lstrlen(pszString)-1] == _T('"'))) ||
			 (pszString[0] == _T('\'') && (pszString[lstrlen(pszString)-1] == _T('\''))))
		{
			WMICLIINT nLoop = 1, nLen = lstrlen(pszString)-1; 
			while (nLoop < nLen)
			{
				pszString[nLoop-1] = pszString[nLoop];
				nLoop++;
			}
			pszString[nLen-1] = _T('\0');
		}
	}
}

/*------------------------------------------------------------------------
   Name				 :ParseAuthorityUserArgs
   Synopsis          :Examines the Authority and User argument and 
					  determines the authentication type and possibly 
					  extracts the domain name from the user arugment in the 
					  NTLM case.  For NTLM, the domain can be at the end of
					  the authentication string, or in the front of the 
					  user name, ex:  "MSOFT\csriram"
   Type	             :Global Function
   Input parameter(s):
		   ConnType			-  Returned with the connection type, ie wbem,
							   ntlm
		   bstrAuthArg		-  Output, contains the domain name
		   bstrUserArg		-  Output, user name
		   bstrAuthority	-  Input
		   User				-  Input
   Output parameter(s):None 
   Return Type       :
			SCODE 
   Global Variables  :None
   Calling Syntax    :ParseAuthorityUserArgs(bstrAuthArg, bstrUserArg,
							bstrAuthority, bstrUser)
   Notes             :(taken from WMI VC samples 'utillib'
------------------------------------------------------------------------*/
SCODE ParseAuthorityUserArgs(BSTR& bstrAuthArg, BSTR& bstrUserArg,
							BSTR& bstrAuthority, BSTR& bstrUser)
{

    // Determine the connection type by examining the Authority string

    // The ntlm case is more complex.  There are four cases
    // 1)  Authority = NTLMDOMAIN:name" and User = "User"
    // 2)  Authority = NULL and User = "User"
    // 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
    // 4)  Authority = NULL and User = "domain\user"

    // first step is to determine if there is a backslash in the user 
	// name somewhere between the second and second to last character

	try
	{
		WCHAR * pSlashInUser = NULL;
		if(bstrUser)
		{
			WCHAR * pEnd = bstrUser + lstrlen((LPCTSTR)bstrUser) - 1;
			for(pSlashInUser = bstrUser; pSlashInUser <= pEnd; pSlashInUser++)
				// dont think forward slash is allowed!
				if(*pSlashInUser == L'\\')      
					break;
			if(pSlashInUser > pEnd)
				pSlashInUser = NULL;
		}

		if(bstrAuthority && lstrlen((LPCTSTR)bstrAuthority) > 11 &&
						_tcsnicmp((LPCTSTR)bstrAuthority, _T("NTLMDOMAIN:"), 11) == 0) 
		{
			if(pSlashInUser)
				return E_INVALIDARG;

			bstrAuthArg = SysAllocString(bstrAuthority + 11);
			if (bstrAuthArg == NULL)
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

			if(bstrUser) 
			{
				bstrUserArg = SysAllocString(bstrUser);
				if (bstrUserArg == NULL)
					throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
			return S_OK;
		}
		else if(pSlashInUser)
		{
			WMICLIINT iDomLen = pSlashInUser-bstrUser;
			WCHAR *pszTemp = new WCHAR [iDomLen+1];
			if ( pszTemp != NULL )
			{
				wcsncpy(pszTemp, bstrUser, iDomLen);
				pszTemp[iDomLen] = 0;
				bstrAuthArg = SysAllocString(pszTemp);
                if (bstrAuthArg == NULL){
                    delete pszTemp;
					throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                }

				if(lstrlen((LPCTSTR)pSlashInUser+1))
				{
					bstrUserArg = SysAllocString(pSlashInUser+1);
                    if (bstrAuthArg == NULL){
                        delete pszTemp;
						throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                    }
				}
				SAFEDELETE(pszTemp);
			}
			else
				throw OUT_OF_MEMORY;
		}
		else
			if(bstrUser)
			{
				bstrUserArg = SysAllocString(bstrUser);
				if (bstrUserArg == NULL)
					throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
	}
	catch(CHeap_Exception)
	{
        if(bstrAuthArg) SysFreeString(bstrAuthArg); bstrAuthArg = NULL;
        if(bstrUserArg) SysFreeString(bstrUserArg); bstrUserArg = NULL;
		throw OUT_OF_MEMORY;
	}

    return S_OK;
}

/*----------------------------------------------------------------------------
   Name				 :DisplayVARIANTContent
   Synopsis	         :Displays the content of a VARIANT type data object
   Type	             :Member Function
   Input Parameter(s):
		vtObject	 - VARIANT object
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :DisplayVARIANTContent(vtObject)
   Notes             :none
----------------------------------------------------------------------------*/
void DisplayVARIANTContent(VARIANT vtObject)
{
	_TCHAR szMsg[BUFFER255] = NULL_STRING;
	switch ( vtObject.vt )
	{
	case VT_UI1:
		_stprintf(szMsg, _T("%c"), vtObject.bVal);
		break;
	case VT_I2:
		_stprintf(szMsg, _T("%i"), vtObject.iVal);
		break;
	case VT_I4:
		_stprintf(szMsg, _T("%li"),	vtObject.lVal);
		break;
	case VT_R4:
		_stprintf(szMsg, _T("%f"),	vtObject.fltVal);
		break;
	case VT_R8:
		_stprintf(szMsg, _T("%e"),	vtObject.dblVal);
		break;
	case VT_BOOL:
		_stprintf(szMsg, _T("%i"),	vtObject.boolVal);
		break;
	case VT_I1:
		_stprintf(szMsg, _T("%c"),	vtObject.cVal);
		break;
	case VT_UI2:
		_stprintf(szMsg, _T("%i"),	vtObject.uiVal);
		break;
	case VT_UI4:
		_stprintf(szMsg, _T("%ld"),	vtObject.ulVal);
		break;
	case VT_INT:
		_stprintf(szMsg, _T("%i"),	vtObject.intVal);
		break;
	case VT_UINT:
		_stprintf(szMsg, _T("%i"),	vtObject.uintVal);
		break;
	}
	DisplayMessage(szMsg, CP_OEMCP, FALSE, TRUE);
}

/*----------------------------------------------------------------------------
   Name				 :GetPropertyAttributes
   Synopsis	         :This function obtains the information about the class
					  properties 
   Type	             :Member Function
   Input Parameter(s):
		pIObj	     - pointer to IWbemClassObject object
		bstrProp     - property name
		bTrace	     - trace flag
   Output parameter(s):
   		pdPropDet    - property details structure
   Return Type       :HRESULT 
   Global Variables  :None
   Calling Syntax    :GetPropertyAttributes(pIObj, bstrProp, 
											pdPropDet, bTrace)
   Notes             :none
----------------------------------------------------------------------------*/
HRESULT GetPropertyAttributes(IWbemClassObject* pIObj, 
							  BSTR bstrProp,
							  PROPERTYDETAILS& pdPropDet,
							  BOOL bTrace)
{
	HRESULT				hr			= S_OK;
	IWbemQualifierSet*	pIQualSet	= NULL;
	VARIANT				vtType, vtOper, vtDesc, vtTypeProp;
    CIMTYPE             ctCimType;
	VariantInit(&vtType);
	VariantInit(&vtOper);
	VariantInit(&vtDesc);
	VariantInit(&vtTypeProp);
	try
	{
		// Obtain the property qualifier set for the property
   		hr = pIObj->GetPropertyQualifierSet(bstrProp, &pIQualSet);
		if ( pIQualSet != NULL )
		{
			// Obtain the CIM type of the property
			hr = pIQualSet->Get(_bstr_t(L"CIMTYPE"), 0L, &vtType, NULL);

			if (SUCCEEDED(hr))
			{
				if ( vtType.vt == VT_BSTR )
                {
					pdPropDet.Type = _bstr_t(vtType.bstrVal);
	
                    // Obtain the CIM type of the property
		            hr = pIObj->Get(bstrProp, 0L, &vtTypeProp, &ctCimType, NULL);
			        if (SUCCEEDED(hr))
                    {
                        if ( ctCimType & VT_ARRAY )
                        {
                            pdPropDet.Type = _bstr_t("array of ") + 
                                             pdPropDet.Type;
                        }
			            VARIANTCLEAR(vtTypeProp);
                    }
                }
				else
					pdPropDet.Type = _bstr_t("Not Found");
			}
			else
				pdPropDet.Type = _bstr_t("Not Found");
			// Should not break here, hence the HRESULT should be set to S_OK
			hr = S_OK;
			VARIANTCLEAR(vtType);

			// Check whether the property has 'read' flag set
			_bstr_t bstrOper;
			hr = pIQualSet->Get(_bstr_t(L"read"), 0L, &vtOper, NULL);
			if (SUCCEEDED(hr))
			{
				if (vtOper.vt == VT_BOOL && vtOper.boolVal)
					bstrOper = _bstr_t(L"Read");
			}
			VARIANTCLEAR(vtOper);
			// Should not break here, hence the HRESULT should be set to S_OK
			hr = S_OK;

			// Check whether the property has 'write' flag set
			VariantInit(&vtOper);
			hr = pIQualSet->Get(_bstr_t(L"write"), 0L, &vtOper, NULL);
			if (SUCCEEDED(hr))
			{
				if ((vtOper.vt == VT_BOOL) && vtOper.boolVal)
				{
					if (!bstrOper)
						bstrOper = _bstr_t(L"Write");
					else
						bstrOper += _bstr_t(L"/Write");	
				}
			}
			VARIANTCLEAR(vtOper);
			// Should not break here, hence the HRESULT should be set to S_OK
			hr = S_OK;

			if (!bstrOper)
				pdPropDet.Operation = _bstr_t(TOKEN_NA);
			else
				pdPropDet.Operation = _bstr_t(bstrOper);

			// Retrieve the 'Description' text for the property
			if (FAILED(pIQualSet->Get(_bstr_t(L"Description"), 0L, 
						&vtDesc,NULL)))
				pdPropDet.Description = _bstr_t("Not Available");
			else
				pdPropDet.Description = _bstr_t(vtDesc.bstrVal);
			VARIANTCLEAR(vtDesc);
			SAFEIRELEASE(pIQualSet);
		}
		else
			hr = S_OK;
	}
	catch(_com_error& e)
	{
		VARIANTCLEAR(vtType);
		VARIANTCLEAR(vtOper);
		VARIANTCLEAR(vtDesc);
		SAFEIRELEASE(pIQualSet);
		hr = e.Error();
	}
	return hr;
}  

/*----------------------------------------------------------------------------
   Name				 :GetNumber
   Synopsis	         :converts string to number
   Input Parameter(s):string 
   Output parameters :
   Return Type       :WMICLIINT 
   Global Variables  :None
   Calling Syntax    :GetNumber(string)
   Notes             :none
----------------------------------------------------------------------------*/
WMICLIINT GetNumber ( WCHAR* wsz )
{
	WMICLIINT iResult = -1;

	if ( wsz )
	{
		WCHAR* wszTemp = wsz;

		if ( *wszTemp == L'0' )
		{
			wszTemp++;

			if ( *wszTemp )
			{
				if ( towlower ( *wszTemp ) == L'x' )
				{
					WMICLIINT i = 0;
					i = swscanf ( wsz, L"%x", &iResult );
					if ( !i || i == EOF )
					{
						iResult = -1;
					}
				}
				else
				{
					// do not support octal strings
				}
			}
			else
			{
				// this is plain 0
				iResult = 0;
			}
		}
		else
		{
			WMICLIINT i = 0;
			i = swscanf ( wsz, L"%d", &iResult );
			if ( !i || i == EOF )
			{
				iResult = -1;
			}
		}
	}

	return iResult;
}

/*----------------------------------------------------------------------------
   Name				 :ReturnFileType
   Synopsis	         :type of file
   Input Parameter(s):FILE* 
   Output parameters :none
   Return Type       :FILETYPE 
   Global Variables  :None
   Calling Syntax    :ReturnFileType(file)
   Notes             :none
----------------------------------------------------------------------------*/
FILETYPE ReturnFileType ( FILE* fpFile )
{
	FILETYPE eftFileType = ANSI_FILE ;

	// Indentifing the file type whether Unicode or ANSI.
	char szFirstTwoBytes [2] = { '\0' } ;

    if( fread(szFirstTwoBytes, 2, 1, fpFile) )
	{
		if ( memcmp(szFirstTwoBytes, UNICODE_SIGNATURE, 2)	== 0 )
		{
			eftFileType = UNICODE_FILE;
		} 
		else if (memcmp(szFirstTwoBytes, UNICODE_BIGEND_SIGNATURE, 2) == 0 )
		{
			eftFileType = UNICODE_BIGENDIAN_FILE;
		}
		else if( memcmp(szFirstTwoBytes, UTF8_SIGNATURE, 2) == 0 )
		{
			eftFileType = UTF8_FILE;
		}

		fseek(fpFile, 0, SEEK_SET);
    }

	return eftFileType ;
}

/*----------------------------------------------------------------------------
   Name				 :ReturnVarType
   Synopsis	         :Does the CIMTYPE to VARIANT conversion
   Input Parameter(s):
		bstrCIMType	 - CIMTYPE 
   Output parameters :
   Return Type       :VARTYPE 
   Global Variables  :None
   Calling Syntax    :ReturnVarType(bstrCIMType)
   Notes             :none
----------------------------------------------------------------------------*/
VARTYPE ReturnVarType( _TCHAR* bstrCIMType )
{
	if (CompareTokens(bstrCIMType, _T("")))
		return VT_NULL;
	else if (CompareTokens(bstrCIMType,_T("string")))
		return VT_BSTR;
	else if (CompareTokens(bstrCIMType,_T("Sint16")))
		return VT_I2;
	else if (CompareTokens(bstrCIMType,_T("Sint8")))
		return VT_I2;
	else if ( CompareTokens(bstrCIMType,_T("Sint32")))
		return VT_I4;
	else if ( CompareTokens(bstrCIMType,_T("Real32")))
		return VT_R4;
	else if ( CompareTokens(bstrCIMType,_T("Sint64")))
		return VT_BSTR;
	else if ( CompareTokens(bstrCIMType,_T("Uint64")))
		return VT_BSTR;
	else if ( CompareTokens(bstrCIMType,_T("Real64")))
		return VT_R8;
	else if ( CompareTokens(bstrCIMType,_T("Boolean")))
		return VT_BOOL;
	else if ( CompareTokens(bstrCIMType,_T("Object")))
		return VT_DISPATCH;
	else if ( CompareTokens(bstrCIMType,_T("Sint8")))
		return VT_INT;
	else if ( CompareTokens(bstrCIMType,_T("Uint8")))
		return VT_UI1;
	else if ( CompareTokens(bstrCIMType,_T("Uint16")))
		return VT_I4;
	else if ( CompareTokens(bstrCIMType,_T("Uint32")))
		return VT_I4;
	else if ( CompareTokens(bstrCIMType,_T("Datetime")))
		return VT_BSTR; // In WMI Date is treated as string
	else if ( CompareTokensChars(bstrCIMType,_T("ref:"),lstrlen(_T("ref:"))))
		return VT_BSTR; // In WMI ref is treated as string
	else if ( CompareTokens(bstrCIMType,_T("Char16")))
		return VT_I2;
	else // (CIM_OBJECT)
		return VT_NULL;

	return VT_EMPTY;
}

/*----------------------------------------------------------------------------
   Name				 :ConvertCIMTYPEToVarType
   Synopsis	         :Does the CIMTYPE to VARIANT conversion
   Type	             :Member Function
   Input Parameter(s):
		varSrc		 - VARIANT source 
		bstrCIMType	 - CIMTYPE 
   Output parameters :
		varDest		 - VARIANT destination
   Return Type       :HRESULT 
   Global Variables  :None
   Calling Syntax    :ConvertCIMTYPEToVarType(varDest, varSrc, bstrCIMType)
   Notes             :none
----------------------------------------------------------------------------*/
HRESULT ConvertCIMTYPEToVarType( VARIANT& varDest, VARIANT& varSrc,
								_TCHAR* bstrCIMType )
{
	HRESULT hr = S_OK;

	if (CompareTokens(bstrCIMType,_T("string")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_BSTR);
	else if (CompareTokens(bstrCIMType,_T("Sint16")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_I2);
	else if (CompareTokens(bstrCIMType,_T("Sint8")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_I2);
	else if ( CompareTokens(bstrCIMType,_T("Sint32")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_I4);
	else if ( CompareTokens(bstrCIMType,_T("Real32")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_R4);
	else if ( CompareTokens(bstrCIMType,_T("Sint64")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_BSTR);
	else if ( CompareTokens(bstrCIMType,_T("Uint64")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_BSTR);
	else if ( CompareTokens(bstrCIMType,_T("Real64")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_R8);
	else if ( CompareTokens(bstrCIMType,_T("Boolean")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_BOOL);
	else if ( CompareTokens(bstrCIMType,_T("Object")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_DISPATCH);
	else if ( CompareTokens(bstrCIMType,_T("Sint8")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_INT);
	else if ( CompareTokens(bstrCIMType,_T("Uint8")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_UI1);
	else if ( CompareTokens(bstrCIMType,_T("Uint16")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_I4);
	else if ( CompareTokens(bstrCIMType,_T("Uint32")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_I4);
	else if ( CompareTokens(bstrCIMType,_T("Datetime")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_BSTR); // In WMI Date is treated as string
	else if ( CompareTokensChars(bstrCIMType,_T("ref:"),lstrlen(_T("ref:"))))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_BSTR); // In WMI ref is treated as string
	else if ( CompareTokens(bstrCIMType,_T("Char16")))
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_I2);
	else // (CIM_OBJECT)
		hr = VariantChangeType(&varDest, &varSrc, 0, VT_NULL);
	
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :DisplayMessage
   Synopsis	         :Displays localized string 
   Type	             :Global Function 
   Input parameter(s):
			lszpMsg		- string  
			uCP		- codepage value
			bIsError	- Boolean type specifying error message or not.
			bIsLog		- Boolean type specifying message to be logged or not .
   Output parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayMessage(lpszMsg, uCP, TRUE, TRUE)
   Notes             :bIsError = FALSE, bIsLog = FALSE by default.	
----------------------------------------------------------------------------*/
void DisplayMessage ( LPTSTR lpszMsg, UINT uCP, BOOL bIsError, BOOL bIsLog, BOOL bIsStream )
{
	LPSTR	lpszDisp = NULL;
	try
	{
		// Convert the widechar to MBCS string
		if ( lpszMsg && lpszMsg[0] )
		{
			OUTPUTSPEC opsOutOpt = g_wmiCmd.GetParsedInfoObject().
								GetGlblSwitchesObject().
								GetOutputOrAppendOption(TRUE);

			if ( bIsError || ( STDOUT == opsOutOpt ) )
			{
				if ( ! ConvertWCToMBCS ( lpszMsg, (LPVOID*) &lpszDisp, uCP ) )
				{
					_com_issue_error(WBEM_E_OUT_OF_MEMORY);
				}
			}

			if (bIsLog)
			{
				// Append to the output string
				g_wmiCmd.GetFormatObject().AppendtoOutputString ( lpszMsg ) ;
			}

			if ( bIsError == TRUE )
			{
				fprintf(stderr, "%s", lpszDisp);
				fflush(stderr);
			}
			else
			{
				// OUT for getting append file pointer.
				FILE* fpOutFile = g_wmiCmd.GetParsedInfoObject().
											GetGlblSwitchesObject().
											GetOutputOrAppendFilePointer(TRUE);

				if ( opsOutOpt == CLIPBOARD )
				{
					g_wmiCmd.AddToClipBoardBuffer ( lpszMsg ) ;
				}
				else if ( opsOutOpt == FILEOUTPUT )
				{
					if (fpOutFile != NULL)
					{
						fwprintf( fpOutFile, L"%s", lpszMsg ) ;
					}
				}
				else
				{
					if ( FALSE == bIsStream )
					{
						fprintf(stdout, "%s", lpszDisp);
						fflush(stdout);
					}
				}

				// FALSE for getting append file pointer.
				FILE* fpAppendFile = g_wmiCmd.GetParsedInfoObject().
											GetGlblSwitchesObject().
											GetOutputOrAppendFilePointer(FALSE);
											
				if ( fpAppendFile != NULL )
				{
					FILETYPE eftOpt =  g_wmiCmd.GetParsedInfoObject().
												GetGlblSwitchesObject().
												GetFileType () ;

					if ( ANSI_FILE == eftOpt )
					{
						fprintf ( fpAppendFile, "%S", lpszMsg ) ;
					}
					else
					{
						fwprintf( fpAppendFile, L"%s", lpszMsg ) ;
					}
				}
			}

			SAFEDELETE(lpszDisp);
		}
	}
	catch(_com_error& e)
	{
		SAFEDELETE(lpszDisp);
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		SAFEDELETE(lpszDisp);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	
}

/*----------------------------------------------------------------------------
   Name				 :CleanUpCharVector
   Synopsis	         :Clears the character vector
   Type	             :Global Function 
   Input parameter(s):
			cvCharVector - reference to character vector.
   Output parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :CleanUpCharVector(cvCharVector)
   Notes             :None
----------------------------------------------------------------------------*/
void CleanUpCharVector(CHARVECTOR& cvCharVector)
{
	if ( !cvCharVector.empty() )
	{
		CHARVECTOR::iterator theIterator;
		for (	theIterator = cvCharVector.begin();	
				theIterator < cvCharVector.end(); theIterator++  )
		{
			SAFEDELETE( *theIterator );
		}
		cvCharVector.clear();
	}
}

/*----------------------------------------------------------------------------
   Name				 :FindAndReplaceAll
   Synopsis	         :Search and replace all the occurences of pszFromStr 
					  with pszToStr in the given string strString
   Type	             :Global Function 
   Input parameter   :
			strString  - string buffer
			pszFromStr - string to searched and replaced
			pszToStr   - string to be replaced by
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :CleanUpCharVector(cvCharVector)
   Notes             :None
----------------------------------------------------------------------------*/
void FindAndReplaceAll(STRING& strString, _TCHAR* pszFromStr,_TCHAR* pszToStr)
{
	if ( pszFromStr != NULL && pszToStr != NULL )
	{
		STRING::size_type stPos = 0;
		STRING::size_type stFromPos = 0;
		STRING::size_type stFromStrLen = lstrlen(pszFromStr);
		STRING::size_type stToStrLen = lstrlen(pszToStr);
		while( TRUE )
		{
			stPos = strString.find(pszFromStr, stFromPos, stFromStrLen); 
			if ( stPos == STRING::npos )
				break;
			strString.replace(stPos, stFromStrLen, pszToStr);
			stFromPos = stPos + stToStrLen;
		}
	}
}

/*----------------------------------------------------------------------------
   Name				 :IsSysProp
   Synopsis	         :Returns true if the input property is a WMI system 
					  property
   Type	             :Global Function 
   Input parameter   :
			pszProp    - property name
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Notes             :IsSysProp(pszProp)
----------------------------------------------------------------------------*/
BOOL IsSysProp(_TCHAR* pszProp)
{
	BOOL bRet = FALSE;
	if (CompareTokens(pszProp, WMISYSTEM_CLASS) ||
		CompareTokens(pszProp, WMISYSTEM_DERIVATION) ||
		CompareTokens(pszProp, WMISYSTEM_DYNASTY) ||
		CompareTokens(pszProp, WMISYSTEM_GENUS) ||
		CompareTokens(pszProp, WMISYSTEM_NAMESPACE) ||
		CompareTokens(pszProp, WMISYSTEM_PATH) ||
		CompareTokens(pszProp, WMISYSTEM_PROPERTYCOUNT) ||
		CompareTokens(pszProp, WMISYSTEM_REPLATH) ||
		CompareTokens(pszProp, WMISYSTEM_SERVER) ||
		CompareTokens(pszProp, WMISYSTEM_SUPERCLASS))
	{
		bRet = TRUE;
	}
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :EraseConsoleString
   Synopsis	         :Displays white spaces at the current cursor position 
					  and sets the cursor column to zero
   Type	             :Global Function 
   Input parameter   :String ID wich has to be wiped out, Console information prior to 
                       writing the string.
   Output parameters :None
   Return Type       :None
   Global Variables  :None
----------------------------------------------------------------------------*/

void EraseConsoleString(CONSOLE_SCREEN_BUFFER_INFO* csbiInfo)
{
	COORD						coord; 
	HANDLE						hStdOut		= NULL;
	WMICLIINT					nHeight		= 0;
	DWORD dWritten = 0;	
	CONSOLE_SCREEN_BUFFER_INFO lcsbiInfo;
	DWORD						XCordW = 0;
	DWORD						YCordW = 0;
	
	LPTSTR	lpszMsg = new _TCHAR [BUFFER1024];
    
	if( NULL == lpszMsg ) return; // NO MEMORY

	// Obtain the standard output handle
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(hStdOut, &lcsbiInfo);

	XCordW = lcsbiInfo.dwCursorPosition.X - csbiInfo->dwCursorPosition.X;
	YCordW = lcsbiInfo.dwCursorPosition.Y - csbiInfo->dwCursorPosition.Y;

	dWritten =  XCordW + ( YCordW  * lcsbiInfo.dwMaximumWindowSize.X);

	if( dWritten > BUFFER1024 ) dWritten = BUFFER1024;

	FillMemory(lpszMsg, dWritten * sizeof(_TCHAR), '\0');

	// Get the screen buffer size. 
	if (hStdOut != INVALID_HANDLE_VALUE )
		nHeight = csbiInfo->srWindow.Bottom - csbiInfo->srWindow.Top;
	else
		nHeight = 0;

	// if console size is positive (to address redirection)
	if (nHeight > 0)
	{
		coord.X = 0;
		coord.Y = csbiInfo->dwCursorPosition.Y;
		SetConsoleCursorPosition(hStdOut, coord);
		WriteConsole(hStdOut,lpszMsg,dWritten,&dWritten,NULL);
		SetConsoleCursorPosition(hStdOut, coord);
	}
	SAFEDELETE(lpszMsg);
}

/*----------------------------------------------------------------------------
   Name				 :IsRedirection
   Synopsis	         :Returns true if the the output is being redirected 
   Type	             :Global Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Notes             :IsRedirection()
----------------------------------------------------------------------------*/
BOOL IsRedirection()
{
	HANDLE						hStdOut		= NULL;
	CONSOLE_SCREEN_BUFFER_INFO	csbiInfo; 
	WMICLIINT					nHeight		= 0;
	BOOL						bRet		= FALSE;

	// Obtain the standard output handle
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStdOut != INVALID_HANDLE_VALUE)
	{
		// Get the screen buffer size. 
		if ( GetConsoleScreenBufferInfo(hStdOut, &csbiInfo) == TRUE )
			nHeight = csbiInfo.srWindow.Bottom - csbiInfo.srWindow.Top;
		else
			nHeight = 0;

		if (nHeight <= 0)
			bRet = TRUE;
	}
	else
		bRet = TRUE;
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :WMITRACEORERRORLOG
   Synopsis	         :To display the trace of COM methods invoked
   Type	             :Global Function 
   Input parameter   :
			hr		- HRESULT value
			nLine	- the line number of the file.
			pszFile	- the file name
			pszMsg  - message to be displayed.
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax	 :WMITRACEORERRORLOG(hr, nLine, pszFile, (LPCWSTR)chsMsg, 
										 eloErrLgOpt, bTrace)
   Note				 : Default values dwError = 0, pszResult = NULL. 	
----------------------------------------------------------------------------*/
void WMITRACEORERRORLOG(HRESULT hr, INT nLine, char* pszFile, _bstr_t bstrMsg, 
						DWORD dwThreadId, CParsedInfo& rParsedInfo, 
						BOOL bTrace, DWORD dwError, _TCHAR* pszResult)
{
	_TCHAR* pszMsg = bstrMsg;

	if ( pszMsg )
	{
		try
		{
			if ( bTrace == TRUE )
			{
				if (_tcsnicmp(pszMsg,_T("COMMAND:"),8) != 0)
				{
					CHString chsMessage;
					CHString chsSInfo;
					if (FAILED (hr))
						chsMessage.Format(L"FAIL: %s\n", pszMsg?pszMsg:L"NULL");
					else
						chsMessage.Format(L"SUCCESS: %s\n",pszMsg?pszMsg:L"NULL");			
						
					_fputts((_TCHAR*)_bstr_t((LPCWSTR)chsMessage), stderr);
					fflush(stderr);
					chsSInfo.Format(L"Line: %6d File: %s\n", nLine, 
													  (LPCWSTR)CHString(pszFile));
					_fputts((_TCHAR*)_bstr_t((LPCWSTR)chsSInfo), stderr);
					fflush(stderr);

					if ( pszResult != NULL )
					{
						chsMessage.Format(L"Result:  %s\n\n", pszResult);			
						_fputts((_TCHAR*)_bstr_t((LPCWSTR)chsMessage), stderr);
						fflush(stderr);
					}
					else
					{
						_fputts(_T("\n"), stderr);
						fflush(stderr);
					}
				}
			}
		}
		catch(_com_error& e)
		{
			_com_issue_error(e.Error());
		}
		catch(CHeap_Exception)
		{
			_com_issue_error(WBEM_E_OUT_OF_MEMORY);
		}

		if ( rParsedInfo.GetErrorLogObject().GetErrLogOption() != NO_LOGGING )
		{
			try
			{
				rParsedInfo.GetErrorLogObject().
					LogErrorOrOperation(hr, pszFile, nLine, pszMsg, 
								dwThreadId, dwError); 
			}
			catch(DWORD dwError)
			{
				::SetLastError(dwError);
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(dwError);
			}
		}
	}
}
	
/*----------------------------------------------------------------------------
   Name				 :DisplayWin32Error
   Synopsis	         :Displays the formatted error message for the Win32
					  function calls failure
   Type	             :Global Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax	 :DisplayWin32Error()
   Notes			 :None
----------------------------------------------------------------------------*/
void DisplayWin32Error()
{
	LPVOID	lpMessage	= NULL;
	DWORD	dwError		= ::GetLastError();

	try
	{
		// If there was an error, create a text message for it
		DWORD dwRet = FormatMessage	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
										FORMAT_MESSAGE_FROM_SYSTEM |
										FORMAT_MESSAGE_IGNORE_INSERTS,
										NULL,
										dwError,
										0, 
										(LPTSTR) &lpMessage,
										0,
										NULL
									);

		if ( 0 != dwRet )
		{
			_bstr_t bstrMsg;
			WMIFormatMessage(IDS_I_ERROR_WIN32, 1, bstrMsg, (LPWSTR) lpMessage);
			
			// Free the memory used up the error message
			// and then exit
			if ( NULL != lpMessage )
			{
				LocalFree(lpMessage);
				lpMessage = NULL ;
			}

			// Display the error message
			DisplayMessage((LPWSTR)bstrMsg, CP_OEMCP, TRUE, TRUE);
		}

		// Used for returning the error level
		::SetLastError(dwError);
	}
	catch(_com_error& e)
	{
		if ( lpMessage != NULL )
		{
			LocalFree(lpMessage);
			lpMessage = NULL ;
		}

		_com_issue_error(e.Error());
	}
}

/*----------------------------------------------------------------------------
   Name				 :AcceptPassword
   Synopsis	         :Prompts for the password when user name alone is 
					  specified with the command
   Type	             :Global Function 
   Input parameter   :None
   Output parameters :
			pszPassword  - password string
   Return Type       :void
   Global Variables  :None
   Calling Syntax	 :AcceptPassword(pszPassword)
   Notes             :None
----------------------------------------------------------------------------*/
void AcceptPassword(_TCHAR* pszPassword)
{
	// local variables
	TCHAR	ch;
	DWORD	dwIndex				= 0;
	DWORD	dwCharsRead			= 0;
	DWORD	dwCharsWritten		= 0;
	DWORD	dwPrevConsoleMode	= 0;
	HANDLE	hStdIn				= NULL;
	_TCHAR	szBuffer[BUFFER32]	= NULL_STRING;		

	// Get the handle for the standard input
	hStdIn = GetStdHandle( STD_INPUT_HANDLE );

	// Get the current input mode of the input buffer
	GetConsoleMode( hStdIn, &dwPrevConsoleMode );
	
	// Set the mode such that the control keys are processed by the system
	SetConsoleMode( hStdIn, ENABLE_PROCESSED_INPUT );
	
	//	Read the characters until a carriage return is hit
	while( TRUE )
	{
		if ( !ReadConsole( hStdIn, &ch, 1, &dwCharsRead, NULL ))
		{
			// Set the original console settings
			SetConsoleMode( hStdIn, dwPrevConsoleMode );
			return;
		}
		
		// Check for carraige return
		if ( ch == CARRIAGE_RETURN )
		{
			// break from the loop
			break;
		}

		// Check id back space is hit
		if ( ch == BACK_SPACE )
		{
			if ( dwIndex != 0 )
			{
				//
				// Remove a asterik from the console

				// move the cursor one character back
				FORMAT_STRING( szBuffer, _T( "%c" ), BACK_SPACE );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );
				
				// replace the existing character with space
				FORMAT_STRING( szBuffer, _T( "%c" ), BLANK_CHAR );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );

				// now set the cursor at back position
				FORMAT_STRING( szBuffer, _T( "%c" ), BACK_SPACE );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );

				// decrement the index 
				dwIndex--;
			}
			
			// process the next character
			continue;
		}

		// if the max password length has been reached then sound a beep
		if ( dwIndex == ( MAXPASSWORDSIZE - 1 ) )
		{
			WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), BEEP_SOUND, 1, 
				&dwCharsWritten, NULL );
		}
		else
		{
			// store the input character
			*( pszPassword + dwIndex ) = ch;
			dwIndex++;

			// display asterix onto the console
			WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), ASTERIX, 1, 
				&dwCharsWritten, NULL );
		}
	}

	// Add the NULL terminator
	*( pszPassword + dwIndex ) = NULL_CHAR;

	//Set the original console settings
	SetConsoleMode( hStdIn, dwPrevConsoleMode );

	// display the character ( new line character )
	FORMAT_STRING( szBuffer, _T( "%s" ), _T( "\n\n" ) );
	WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 2, 
		&dwCharsWritten, NULL );
}

/*----------------------------------------------------------------------------
   Name				 :IsValueSet
   Synopsis	         :Checks string passed in pszFromValue, Is a value set
					  or not.	
   Type	             :Global Function 
   Input parameter   :
			pszFromValue - string to be checked.
   Output parameters :
			cValue1		 - <value1> of value set.
			cValue2		 - <value2> of value set.
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax	 :IsValueSet(pszFromValue,cValue1,cValue2)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL IsValueSet(_TCHAR* pszFromValue, _TCHAR& cValue1, _TCHAR& cValue2)
{
	BOOL bValueSet = FALSE;
	
	if ( pszFromValue != NULL )
	{
		_TCHAR cV1 = _T('\0'), cV2 = _T('\0');

		if ( lstrlen(pszFromValue) == 3 && pszFromValue[1] == _T('-') )
		{
			bValueSet = TRUE;
			cV1 = pszFromValue[0];
			cV2 = pszFromValue[2];
			cValue1 = ( cV1 < cV2 ) ? cV1 : cV2 ;
			cValue2 = ( cV1 > cV2 ) ? cV1 : cV2 ;
		}
	}
	else
		cValue1 = cValue2 = _T('\0');

	return bValueSet;
}

/*----------------------------------------------------------------------------
   Name				 :DisplayString
   Synopsis	         :Displays localized string
   Type	             :Global Function
   Input parameter(s):
			uID		  - string table identifier
			uCP		  - codepage value
			lpszParam - String to be used as parameter in resource string.
			bIsError  - Boolean type specifying error message or not.
			bIsLog    - Boolean type specifying message to be logged or not.
   Output parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayString(uID, CP_OEMCP, NULL, TRUE, TRUE)
   Notes             :lpszParam = NULL, bIsError = FALSE, and bIsLog = FALSE
					  by default.	
----------------------------------------------------------------------------*/
void DisplayString(UINT uID, UINT uCP, LPTSTR lpszParam,
				   BOOL bIsError, BOOL bIsLog) throw(WMICLIINT)
{
	LPVOID lpMsgBuf = NULL;
	LPTSTR	lpszMsg		= NULL;
	lpszMsg = new _TCHAR [BUFFER1024];
	try
	{
		if (lpszMsg)
		{
			LoadString(NULL, uID, lpszMsg, BUFFER1024);
			if (lpszParam)
			{
				char* pvaInsertStrs[1];
				pvaInsertStrs[0] = (char*)	lpszParam;

				DWORD dwRet = FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_STRING | 
						FORMAT_MESSAGE_ARGUMENT_ARRAY,
						lpszMsg,
						0, 
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						(LPTSTR) &lpMsgBuf,
						0,
						pvaInsertStrs);

				if (dwRet == 0)
				{
					SAFEDELETE(lpszMsg);
					throw (::GetLastError());
				}
			}

			DisplayMessage	(
								( lpMsgBuf ) ? ( LPTSTR ) lpMsgBuf : lpszMsg,
								uCP,
								bIsError,
								bIsLog
							) ;

			SAFEDELETE(lpszMsg);

			// Free the memory used up the error message
			// and then exit
			if ( lpMsgBuf != NULL )
			{
				LocalFree(lpMsgBuf);
				lpMsgBuf = NULL ;
			}
		}
		else
			throw OUT_OF_MEMORY;
	}
	catch(_com_error& e)
	{
		if ( lpMsgBuf != NULL )
		{
			LocalFree(lpMsgBuf);
			lpMsgBuf = NULL ;
		}

		SAFEDELETE(lpszMsg);
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		if ( lpMsgBuf != NULL )
		{
			LocalFree(lpMsgBuf);
			lpMsgBuf = NULL ;
		}

		SAFEDELETE(lpszMsg);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	
}

/*----------------------------------------------------------------------------
   Name				 :SubstituteEscapeChars
   Synopsis	         :Substitue escape character i.e '\' before the 
					  specified substring
   Type	             :Global Function
   Input parameter   :
			sSource	- source string
			lpszSub	- substring to be searched for
   Output parameters :
			sSource	- source string
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SubstituteEscapeChars(sTemp, lpszSub)
   Notes             :None
----------------------------------------------------------------------------*/
void SubstituteEscapeChars(CHString& sTemp, LPCWSTR lpszSub)
{
	try
	{
		CHString str(sTemp);
		sTemp.Empty();
		while ( str.GetLength() > 0 )
		{
			//LONG lPos = str.Find( L"\"" );
			LONG lPos = str.Find(lpszSub);
			if ( lPos != -1 )
			{
				sTemp += str.Left( lPos ) + L"\\\"";
				str = str.Mid( lPos + 1 );
			}
			else 
			{
				sTemp += str;
				str.Empty();
			}
		}
	}
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}

/*----------------------------------------------------------------------------
   Name				 :RemoveEscapeChars
   Synopsis	         :Remove escape character i.e '\' before any of the 
					  following characters 
					  	
   Type	             :Global Function
   Input parameter(s):
			sSource	- source string
   Output parameter(s):
			sSource	- source string
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :RemoveEscapeChars(sTemp)
   Notes             :None
----------------------------------------------------------------------------*/
void RemoveEscapeChars(CHString& sTemp)
{
	try
	{
		CHString str(sTemp);
		sTemp.Empty();
		while ( str.GetLength() > 0 )
		{
			LONG lPos = str.Find(L"\\");
			if ( lPos != -1 )
			{
				if (str.GetAt(lPos+1) == L'"') 
				{
					sTemp += str.Left( lPos );				
				}
				else
				{
					sTemp += str.Left( lPos ) + L"\\";
				}
				str = str.Mid( lPos + 1 );
			}
			else 
			{
				sTemp += str;
				str.Empty();
			}
		}
	}
	catch(CHeap_Exception)
	{
		throw OUT_OF_MEMORY;
	}
}

/*----------------------------------------------------------------------------
   Name				 :FrameNamespace
   Synopsis	         :Frame the new namespace 
   Type	             :Global Function
   Input parameter(s):
		pszRoleOrNS			- old namespace
		pszRoleOrNSToUpdate	- string to be appended/replaced
   Output parameter(s):
		pszRoleOrNSToUpdate	- new namespace
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FrameNamespace(pszRoleOrNS, pszRoleOrNSToUpdate)
   Notes             :None
----------------------------------------------------------------------------*/
void FrameNamespace(_TCHAR* pszRoleOrNS, _TCHAR* pszRoleOrNSToUpdate)
{
	if ( pszRoleOrNS != NULL && pszRoleOrNSToUpdate != NULL )
	{
		LONG lRoleOrNSLen = lstrlen(pszRoleOrNS);
		LONG lRoleOrNSToUpdate = lstrlen(pszRoleOrNSToUpdate);

		_TCHAR *pszTemp = new _TCHAR[lRoleOrNSLen + lRoleOrNSToUpdate +
									 MAX_BUFFER]; 
		if ( pszTemp != NULL )
		{
			if (!CompareTokens(pszRoleOrNS, CLI_TOKEN_NULL))
			{
				//if the role does not begin with a '\\' it should be assumed
				//to be relative to the current role

				if ( _tcsncmp(pszRoleOrNS, CLI_TOKEN_2BSLASH, 2) == 0 )
					lstrcpy(pszTemp, pszRoleOrNS+2);
				else if (_tcsncmp(pszRoleOrNS, CLI_TOKEN_2DOT, 2) == 0 )
				{
					_TCHAR *lp = NULL;
					for (lp = &pszRoleOrNSToUpdate[lstrlen(pszRoleOrNSToUpdate) - 1]; 
						lp > pszRoleOrNSToUpdate; lp--)
					{
						if (_tcsncmp(lp,CLI_TOKEN_2BSLASH,1) == 0)
						{	lstrcpy(lp,NULL_STRING);
							break;
						}
					}
					lstrcpy(pszTemp, pszRoleOrNSToUpdate);
					if (_tcsncmp(pszRoleOrNS + 2, NULL_STRING, 1))
					{
						lstrcat(pszTemp, pszRoleOrNS + 2);
					}
				}
				else
				{			
					lstrcpy(pszTemp, pszRoleOrNSToUpdate);
					lstrcat(pszTemp, CLI_TOKEN_BSLASH);
					lstrcat(pszTemp, pszRoleOrNS);
				}
				//if the last character in the string pszRoleOrNS terminates 
				//with '\' terminate the string.
				//this case occurs when namespace is specified as "xyz\"
				if(CompareTokens(pszTemp + (WMICLIINT)lstrlen(pszTemp)-1, 
					CLI_TOKEN_BSLASH ) && 
						!CompareTokens(pszTemp, CLI_TOKEN_2BSLASH))
				{
					pszTemp[lstrlen(pszTemp) - 1] = _T('\0');
				}
			}
			else
				lstrcpy(pszTemp, CLI_TOKEN_NULL);

			lstrcpy(pszRoleOrNSToUpdate, pszTemp);
			SAFEDELETE(pszTemp);
		}
		else
			throw OUT_OF_MEMORY;
	}
	else
		throw OUT_OF_MEMORY;
}

/*----------------------------------------------------------------------------
   Name				 :SetScreenBuffer
   Synopsis	         :Set the buffer size of the command line to the 
					  following:
					  1) Width	- 500
					  2) Height - 3000	
   Type	             :Global Function
   Input parameter(s):
			nHeight	 - height of the console buffer
			nWidth	 - width of the console buffer
   Output parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetScreenBuffer(nHeight, nWidth)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL SetScreenBuffer(SHORT nHeight, SHORT nWidth)
{
	BOOL bResult = FALSE;

	COORD	coord;
	coord.X = nWidth;
	coord.Y = nHeight;
    HANDLE	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE); 
	if ( hStdOut != INVALID_HANDLE_VALUE &&
		 hStdOut != (HANDLE)0x00000013 ) // For telnet
	{
		// Set the console screen buffer info
		bResult = SetConsoleScreenBufferSize(hStdOut, coord);
	}

	return bResult;
}

/*----------------------------------------------------------------------------
   Name				 :GetScreenBuffer
   Synopsis	         :Get the buffer size of the command line
   Type	             :Global Function
   Input parameter(s):None
   Output parameter(s):
			nHeight	 - height of the console buffer
			nWidth	 - width of the console buffer
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetScreenBuffer(nHeight, nWidth)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL GetScreenBuffer(SHORT& nHeight, SHORT& nWidth)
{
	BOOL bResult = FALSE;

	HANDLE	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if ( hStdOut != INVALID_HANDLE_VALUE &&
		 hStdOut != (HANDLE)0x00000013 ) // For telnet
	{
		CONSOLE_SCREEN_BUFFER_INFO csbConsoleScreenBufferInfo;
		// Set the console screen buffer info
		if ( GetConsoleScreenBufferInfo(hStdOut, &csbConsoleScreenBufferInfo) == TRUE )
		{
			nHeight = csbConsoleScreenBufferInfo.dwSize.Y;
			nWidth = csbConsoleScreenBufferInfo.dwSize.X;

			bResult = TRUE;
		}
		else
		{
			nHeight = 0;
			nWidth = 0;
		}
	}

	return bResult;
}

/*----------------------------------------------------------------------------
   Name				 :WMIFormatMessage
   Synopsis	         :This function loads the resource string using the 
					  ID of the string and does parameter substituion using
					  the FormatMessage() function.	
   Type	             :Global Function
   Input parameter(s):
			uID			- resource ID
			nParamCount - no. of. parameter(s) to be substituted.
			lpszParam	- first parameter. (%1)
			...			- variable number of arguments (%2, %3, ...)
   Output parameter(s):
			bstrMSG		- formatted message
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :WMIFormatMessage(uID, nParamCount, bstrMsg, 
							lpszParam,)
   Notes             :None
----------------------------------------------------------------------------*/
void WMIFormatMessage(UINT uID, WMICLIINT nParamCount, _bstr_t& bstrMsg, 
					  LPTSTR lpszParam, ...)
{
	// Load the resource string 
	_TCHAR	pszMsg[BUFFER1024];
	LoadString(NULL, uID, pszMsg, BUFFER1024);

	// If parameters are specified.
	if (lpszParam)
	{
		LPTSTR lpszTemp				= lpszParam;
		INT		nLoop				= 0;
		char*	pvaInsertStrs[5];

		va_list marker;
		va_start(marker, lpszParam);
	
		while (TRUE)
		{
			pvaInsertStrs[nLoop++] = (char*) lpszTemp;
			lpszTemp = va_arg(marker, LPTSTR);

			if (nLoop == nParamCount)
				break;
		}
		va_end(marker);
			
		LPVOID lpMsgBuf = NULL;
		DWORD dwRet = FormatMessage	(
										FORMAT_MESSAGE_ALLOCATE_BUFFER | 
										FORMAT_MESSAGE_FROM_STRING | 
										FORMAT_MESSAGE_ARGUMENT_ARRAY,
										pszMsg,
										0, 
										MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
										(LPTSTR) &lpMsgBuf,
										0,
										pvaInsertStrs
									);

		if ( 0 != dwRet )
		{
			if ( lpMsgBuf )
			{
				bstrMsg = (WCHAR*)lpMsgBuf;

				// Free the memory used for the message and then exit
				LocalFree(lpMsgBuf);
				lpMsgBuf = NULL ;
			}
		}
		else
		{
			bstrMsg = pszMsg;
		}
	}
	else
	{
		bstrMsg = pszMsg;
	}
}

/*----------------------------------------------------------------------------
   Name				 :InitWinsock
   Synopsis	         :This function initiates the windows sockets interface.
   Type	             :Global Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :InitWinsock ()
   Notes             :None
----------------------------------------------------------------------------*/
BOOL InitWinsock ()
{
	BOOL	bSuccess	= TRUE;
	WMICLIINT nRes;
	WSADATA wsaData;
	WORD wVerRequested = 0x0101; // ver 1.1

	//	The Windows Sockets WSAStartup function initiates use of Ws2_32.dll 
	//  by a process.
	// Init the sockets interface
	nRes = WSAStartup (wVerRequested, &wsaData);
	if (nRes)
		bSuccess = FALSE;

	return bSuccess;
}

/*----------------------------------------------------------------------------
   Name				 :TermWinsock
   Synopsis	         :This function uninitializes the windows sockets 
					  interface.
   Type	             :Global Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :TermWinsock ()
   Notes             :None
----------------------------------------------------------------------------*/
BOOL TermWinsock ()
{
	// Uninitailize windows socket interface.
	BOOL	bSuccess = TRUE;
	if (SOCKET_ERROR == WSACleanup ())
		bSuccess = FALSE;

	return bSuccess;
}

/*----------------------------------------------------------------------------
   Name				 :PingNode
   Synopsis	         :Pings a node to validate availibility of node using
					  windows socket functions.	 
   Type	             :Global Function
   Input parameter   :
			pszNode  - Pointer to a string specifing node name.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :PingNode(pszNode)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL PingNode(_TCHAR* pszNode)
{
	BOOL	bRet				= TRUE;
	HOSTENT	*pNodeEnt			= NULL;
	HANDLE	hIcmpHandle			= NULL;
    char    *pszSendBuffer		= NULL;
	char    *pszRcvBuffer		= NULL;
    UINT    nSendSize			= DEFAULT_SEND_SIZE;
    UINT    nRcvSize			= DEFAULT_BUFFER_SIZE;
	ULONG	nTimeout			= PING_TIMEOUT;
    UCHAR   *pszOpt				= NULL;
    UINT    nOptLength			= 0;
	ULONG	ulINAddr			= INADDR_NONE;
    UCHAR   uFlags				= 0;
	DWORD	dwNumOfReplies		= 1; // Set 1 for finding error
    IP_OPTION_INFORMATION ioiSendOpts;

	try
	{
		if ( pszNode )
		{
			// pszNode can be IPAddress of node or node name itself. 
			_bstr_t	bstrNodeNameOrIPAddr(pszNode);

			// Initialize windows socket interface.
			if ( g_wmiCmd.GetInitWinSock() == FALSE )
			{
				bRet = InitWinsock();
				g_wmiCmd.SetInitWinSock(bRet);
			}
			
			if ( bRet == TRUE )
			{
				// Get IPAddress. 
				ulINAddr = inet_addr((char*)bstrNodeNameOrIPAddr);
				// If not an IP address. then it may be computername.
				if ( ulINAddr == INADDR_NONE )
				{
					pNodeEnt = gethostbyname((char*)bstrNodeNameOrIPAddr);
					if ( pNodeEnt == NULL)
					{
						bRet = FALSE; // "computername" is not found.
					}
					else
					{
						ulINAddr  = *(DWORD *)pNodeEnt->h_addr ; 
					}
				}
			}

			if ( bRet == TRUE )
			{
				// Create IcmpFile
				hIcmpHandle	= IcmpCreateFile();
				if ( hIcmpHandle == INVALID_HANDLE_VALUE )
					throw GetLastError();

				// Alloc memory to send buffer
				pszSendBuffer = (char*)LocalAlloc(LMEM_FIXED, nSendSize);
				if ( pszSendBuffer == NULL )
					throw GetLastError();

				// Fill Some data in send buffer
				for (WMICLIINT i = 0; i < nSendSize; i++) 
				{
					pszSendBuffer[i] = 'a' + (i % 23);
				}

				// Initialize the send options
				ioiSendOpts.OptionsData = pszOpt;
				ioiSendOpts.OptionsSize = (UCHAR)nOptLength;
				ioiSendOpts.Ttl = DEFAULT_TTL;
				ioiSendOpts.Tos = DEFAULT_TOS;
				ioiSendOpts.Flags = uFlags;

				// Alloc memory to receive buffer
				pszRcvBuffer = (char*)LocalAlloc(LMEM_FIXED, nRcvSize);
				if ( pszRcvBuffer == NULL )
					throw GetLastError();

				dwNumOfReplies = IcmpSendEcho(hIcmpHandle,
											  (IPAddr)ulINAddr,
											  pszSendBuffer,
											  (unsigned short) nSendSize,
											  &ioiSendOpts,
											  pszRcvBuffer,
											  nRcvSize,
											  nTimeout);
				if ( dwNumOfReplies == 0 )
					throw GetLastError();

				// Free memory
				LocalFree(pszSendBuffer);
				LocalFree(pszRcvBuffer);
				IcmpCloseHandle(hIcmpHandle);
			}
		}
		else
			bRet = FALSE; // Null nodename pointer.
	}
	catch(_com_error& e)
	{
		// Free memory
		if ( hIcmpHandle != NULL )
			IcmpCloseHandle(hIcmpHandle);
		if ( pszSendBuffer != NULL )
			LocalFree(pszSendBuffer);
		if ( pszRcvBuffer != NULL )
			LocalFree(pszRcvBuffer);
		bRet = FALSE;
		_com_issue_error(e.Error());
	}
	catch (DWORD dwError)
	{
		// if ping failed then don't display win32 error
		if ( dwNumOfReplies != 0 )
		{
			::SetLastError(dwError);
			DisplayWin32Error();
			::SetLastError(dwError);
		}
		bRet = FALSE;
		// Free memory
		if ( hIcmpHandle != NULL )
			IcmpCloseHandle(hIcmpHandle);
		if ( pszSendBuffer != NULL )
			LocalFree(pszSendBuffer);
		if ( pszRcvBuffer != NULL )
			LocalFree(pszRcvBuffer);
	}
	catch (CHeap_Exception)
	{
		// Free memory
		if ( pszSendBuffer != NULL )
			LocalFree(pszSendBuffer);
		if ( hIcmpHandle != NULL )
			IcmpCloseHandle(hIcmpHandle);
		if ( pszRcvBuffer != NULL )
			LocalFree(pszRcvBuffer);
		bRet = FALSE;
	}

	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :IsFailFastAndNodeExist
   Synopsis	         :Validates node if FailFast is on, If pszNodeName == NULL
					  then check for GetNode() else pszNodeName itself. 
   Type	             :Global Function
   Input parameter(s):
		rParsedInfo  - reference to object of CParsedInfo.
		pszNode		 - Pointer to a string specifing node name.
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :IsFailFastAndNodeExist(rParsedInfo, pszNode)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL IsFailFastAndNodeExist(CParsedInfo& rParsedInfo, _TCHAR* pszNode)
{
	BOOL	bRet	= TRUE;
	
	// If FailFast is on.
	if ( rParsedInfo.GetGlblSwitchesObject().GetFailFast() == TRUE )
	{
		// Form the appropriate node name. If pszNodeName != NULL pszNode 
		// should be validated. Else Node stored should be validated.
		_TCHAR*		pszNodeName = NULL;
		if (pszNode == NULL)
			pszNodeName = rParsedInfo.GetGlblSwitchesObject().GetNode();
		else
			pszNodeName = pszNode;

		// "." node name specifies local machine and need not to be validated.
		if ( CompareTokens(pszNodeName, CLI_TOKEN_DOT) == FALSE )
		{
			// If pinging node fails then node is unavialable. 
			if ( PingNode(pszNodeName) == FALSE )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_RPC_SERVER_NOT_AVAILABLE);
				bRet = FALSE;
			}
		}
	}

	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :GetBstrTFromVariant
   Synopsis	         :Get _bstr_t object equivalent to Varaint passed.
   Type	             :Global Function
   Input parameter(s):
			vtVar	 - variant object
			pszType	 - Pointer to string specifying type of the object passed.
   Output parameter(s):
			bstrObj	- BSTR object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetBstrTFromVariant(vtVar, bstrObj)
   Notes             :None
----------------------------------------------------------------------------*/
void GetBstrTFromVariant(VARIANT& vtVar, _bstr_t& bstrObj,
						 _TCHAR* pszType)
{
	VARTYPE	vType;
	VARIANT vtDest;
	VariantInit(&vtDest);

	try
	{
		if ( vtVar.vt != VT_NULL && vtVar.vt != VT_EMPTY )
		{
			if ( VariantChangeType(&vtDest, &vtVar,0 , VT_BSTR) == S_OK )
            {
                bstrObj = _bstr_t(vtDest);
            }
            else
            {
                // The following line assures that
                // if an exception is thrown (say
                // in the bstr_t allocation below)
                // VariantClear in the catch statement
                // won't try to clear anything.
                V_VT(&vtDest) = VT_EMPTY;

			    if ( vtVar.vt == VT_UNKNOWN )
			    {
				    if ( pszType != NULL )
                    {
					    bstrObj = _bstr_t(pszType);
                    }
				    else
                    {
					    bstrObj = _bstr_t("<Embeded Object>");
                    }
			    }
			    else if ( SafeArrayGetVartype(vtVar.parray, &vType) == S_OK )
			    {
				    if ( pszType != NULL )
                    {
					    bstrObj = _bstr_t("<Array of ") + _bstr_t(pszType) + 
							      _bstr_t(">");
                    }
				    else
                    {
					    bstrObj = _bstr_t("<Array>");
                    }
			    }
			    else
                {
				    bstrObj = _bstr_t("UNKNOWN");
                }
            }
		}
		else
        {
			bstrObj = _bstr_t(_T("<null>"));
        }

		VARIANTCLEAR(vtDest);
	}
	catch(_com_error& e)
	{
		VARIANTCLEAR(vtDest);
		_com_issue_error(e.Error());
	}
}
/*----------------------------------------------------------------------------
   Name				 :IsValidFile
   Synopsis	         :This functions checks if a given file name is
					  Valid.
   Type	             :Global Function
   Input parameter(s):
		pszFileName  - String type, Name of the file to be validated
   Output parameter(s):None
   Return Type       :RETCODE
   Global Variables  :None
   Calling Syntax    :IsValidFile(pszFileName)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE IsValidFile(_TCHAR* pszFileName)
{
	RETCODE bRet = PARSER_ERROR;
	BOOL	bValidFileName	= TRUE;

	LONG lFileNameLen	= lstrlen(pszFileName);
	LONG lCount			= lFileNameLen;
	while( lCount >= 0 )
	{
		if (pszFileName[lCount] == _T('\\'))
			break;

		lCount--;
	}

	if(lCount != 0)
		lCount++;

	while( lCount <= lFileNameLen )
	{
		if( pszFileName[lCount] == _T('/') ||
			pszFileName[lCount] == _T('\\') ||
			pszFileName[lCount] == _T(':') ||
			pszFileName[lCount] == _T('*') ||
			pszFileName[lCount] == _T('?') ||
			pszFileName[lCount] == _T('\"') ||
			pszFileName[lCount] == _T('<') ||
			pszFileName[lCount] == _T('>') ||
			pszFileName[lCount] == _T('|') )
		{
			bValidFileName = FALSE;
			break;
		}

		lCount++;
	}

	if ( pszFileName != NULL && bValidFileName == TRUE)
	{
		FILE *fpFile = _tfopen(pszFileName, _T("a"));

		if ( fpFile != NULL )
		{
			LONG lFileHandle = _fileno(fpFile);
			if ( _filelength(lFileHandle) == 0)
			{
				if ( fclose(fpFile) == 0 )
				{
					if ( _tremove(pszFileName) == 0 )
						bRet = PARSER_CONTINUE;
					else
					{
						DisplayWin32Error();
						bRet = PARSER_ERRMSG;
					}
				}
			}
			else if ( fclose(fpFile) == 0 )
					bRet = PARSER_CONTINUE;
		}

	}

	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :FindAndReplaceEntityReferences
   Synopsis	         :Search and replace all the occurences of entity 
					  references.
   Type	             :Global Function 
   Input parameter(s):
			strString  - string buffer
   Output parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FindAndReplaceEntityReferences(strString);
   Notes             :None
----------------------------------------------------------------------------*/
void FindAndReplaceEntityReferences(_bstr_t& bstrString)
{
	STRING strString((_TCHAR*)bstrString);

	FindAndReplaceAll(strString, _T("&"), _T("&amp;"));
	FindAndReplaceAll(strString, _T("<"), _T("&lt;"));
	FindAndReplaceAll(strString, _T(">"), _T("&gt;"));
	FindAndReplaceAll(strString, _T("\'"), _T("&apos;"));
	FindAndReplaceAll(strString, _T("\""), _T("&quot;"));
	try
	{
		bstrString = _bstr_t((LPTSTR)strString.data());
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*----------------------------------------------------------------------------
   Name				 :IsOption
   Synopsis	         :It checks whether the current token indicates option. An
					  option can start with '/' or '-'
   Type	             :Global Function
   Input Parameter(s):
		pszToken	 - pointer to token.
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :IsOption(pszToken)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL IsOption(_TCHAR* pszToken)
{
	BOOL bRet = TRUE;
	if ( pszToken != NULL )
	{
		bRet = (CompareTokens(pszToken, CLI_TOKEN_FSLASH) 
				|| CompareTokens(pszToken, CLI_TOKEN_HYPHEN))
				? TRUE : FALSE;
	}
	else
		bRet = FALSE;
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :IsClassOperation
   Synopsis	         :It checks whether the current operation is class 
						level operation or instance level operation
   Type	             :Global Function
   Input Parameter(s):
	    rParsedInfo		 - reference to CParsedInfo class object
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :IsClassOperation(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL IsClassOperation(CParsedInfo& rParsedInfo)
{
	BOOL bClass = FALSE;
	if ( rParsedInfo.GetCmdSwitchesObject().GetAliasName() == NULL 
		&& (rParsedInfo.GetCmdSwitchesObject().
									GetWhereExpression() == NULL)
		&& (rParsedInfo.GetCmdSwitchesObject().
									GetPathExpression() == NULL))
	{
		bClass = TRUE;
	}

	return bClass;
}

/*-------------------------------------------------------------------------
   Name				 :ModifyPrivileges
   Synopsis	         :This function enables/disables all the token privileges
					  for current process token.
   Type	             :Global Function
   Input Parameter(s):
			bEnable	 -  Enable|Disable privileges flag
   Output Parameter(s):None
   Return Type       :HRESULT 
   Global Variables  :None
   Calling Syntax    :ModifyPrivileges(bEnable)
   Notes             :none
-------------------------------------------------------------------------*/
HRESULT ModifyPrivileges(BOOL bEnable)
{
	HANDLE		hToken		= NULL;
	DWORD		dwError		= ERROR_SUCCESS, 
				dwLen		= 0;	
	BOOL		bRes		= TRUE;
	TOKEN_USER	tu;
	HRESULT		hr			= WBEM_S_NO_ERROR;
	BYTE		*pBuffer	= NULL;

	// Open the access token associated with the current process. 
    bRes = OpenProcessToken(GetCurrentProcess(), 
							TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, 
							&hToken);

	if (bRes) 
	{
		// If disable privilges
		if (!bEnable)
		{
			// Store the information back into the token,after disabling all 
			// the privileges
			bRes = AdjustTokenPrivileges(hToken, TRUE, NULL, 0, NULL, NULL);
			if (!bRes)
				hr = WBEM_E_ACCESS_DENIED;
		}
		else // If enable privileges
		{
			// Get the privileges
			memset(&tu,0,sizeof(TOKEN_USER));
			bRes = GetTokenInformation(hToken, TokenPrivileges, &tu, 
					sizeof(TOKEN_USER), &dwLen); 
			pBuffer = new BYTE[dwLen];
			if(pBuffer != NULL)
			{
				bRes = GetTokenInformation(hToken, TokenPrivileges, 
									pBuffer, dwLen, &dwLen);
				if (bRes)
				{
					// Iterate through all the privileges and enable them all
					TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
					for (DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
					{
						pPrivs->Privileges[i].Attributes 
											|= SE_PRIVILEGE_ENABLED;
					}
					// Store the information back into the token
					bRes = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, 
													NULL, NULL);
					if (!bRes)
						hr = WBEM_E_ACCESS_DENIED;				
				}
				else
					hr = WBEM_E_ACCESS_DENIED;
				SAFEDELETE(pBuffer);
			}
			else
				hr = WBEM_E_OUT_OF_MEMORY;
		}
		CloseHandle(hToken); 
	}
	else
		hr = WBEM_E_ACCESS_DENIED;
	return hr;
}

/*-------------------------------------------------------------------------
   Name				 :RemoveParanthesis
   Synopsis	         :This function removes the opening and closing
					  paranthesis in the given string.
   Type	             :Global Function
   Input Parameter(s):
			pszString	 -  String which has paranthesis.
   Output Parameter(s):None
   Return Type       :void 
   Global Variables  :None
   Calling Syntax    :RemoveParanthesis(pszString)
   Notes             :none
-------------------------------------------------------------------------*/
void RemoveParanthesis(_TCHAR*& pszString)
{
	if ((lstrlen(pszString) - 1) > 0)
	{
		// Check if the string is enclosed within quotes
		if ((pszString[0] == _T('(')) 
				&& (pszString[lstrlen(pszString)-1] == _T(')')))
		{
			WMICLIINT nLoop = 1, nLen = lstrlen(pszString)-1; 
			while (nLoop < nLen)
			{
				pszString[nLoop-1] = pszString[nLoop];
				nLoop++;
			}
			pszString[nLen-1] = _T('\0');
		}
	}
}

/*-------------------------------------------------------------------------
   Name				 :TrimBlankSpaces
   Synopsis	         :This function removes the leading and trailing blank
					  spaces in the given string.
   Type	             :Global Function
   Input Parameter(s):
			pszString	 -  String in which leading and trailing spaces to be
							removed..
   Output Parameter(s):None
   Return Type       :void 
   Global Variables  :None
   Calling Syntax    :TrimBlankSpaces(pszString)
   Notes             :none
-------------------------------------------------------------------------*/
void TrimBlankSpaces(_TCHAR	*pszString)
{
	if ((lstrlen(pszString) - 1) > 0)
	{
		WMICLIINT		nLengthOfString = lstrlen(pszString);
		WMICLIINT		nNoOfBlanksAtBegin = 0;
		WMICLIINT		nNoOfBlanksAtEnd = 0;

		// find the noof blank chars at begining
		for(WMICLIINT i=0; i<nLengthOfString; ++i)
		{
			if( pszString[i] != _T(' ') ) 
				break;
			else
				nNoOfBlanksAtBegin++;
		}

		// find the noof blank chars at end
		for(WMICLIINT i=nLengthOfString - 1; i>=0; --i)
		{
			if( pszString[i] != _T(' ') ) 
				break;
			else
				nNoOfBlanksAtEnd++;
		}

		// Remove the blanks at begining
		if( nNoOfBlanksAtBegin > 0 )
		{
			// Shift the chars front
			WMICLIINT nLoop = nNoOfBlanksAtBegin;

			while ( nLoop < nLengthOfString )
			{
				pszString[nLoop - nNoOfBlanksAtBegin] = pszString[nLoop];
				nLoop++;
			}
			pszString[nLengthOfString-nNoOfBlanksAtBegin] = _T('\0');
		}

		// Remove the blanks at end
		if ( nNoOfBlanksAtEnd > 0)
			pszString[lstrlen(pszString) - nNoOfBlanksAtEnd] = _T('\0');

	}
}