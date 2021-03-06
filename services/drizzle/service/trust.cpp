//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//      No portion of this source code may be reproduced
//      without express written permission of Microsoft Corporation.
//
//      This source code is proprietary and confidential.
//
//  SYSTEM:     Industry Update
//
//  CLASS:      N/A
//  MODULE:     TRUST.LIB
//  FILE:       Trust.CPP
//
/////////////////////////////////////////////////////////////////////
//
//  DESC:   this file implements the functions used to make cabs
//          signed by certain providers trusted.
//
//
//  AUTHOR: Charles Ma, converted from WU CDMLIB
//  DATE:   10/4/2000
//
/////////////////////////////////////////////////////////////////////

//
// This file  is copied from the Windows AutoUpdate sources at \nt\enduser\windows.com\lib\trust\trust.cpp,
// modified to use BITS logging and remove UI.
//

#include "qmgrlib.h"

#include <wintrust.h>
#include <softpub.h>
#include "trust.h"

#if !defined(BITS_V12_ON_NT4)
#include "trust.tmh"
#endif

HMODULE WINAPI LoadLibraryFromSystemDir(LPCTSTR szModule);

/////////////////////////////////////////////////////////////////////////////
//
// typedefs for APIs used in the CheckTrust() function
//
//      Since some of these APIs are new and only available on IE5 we have to
//      try to dynamically use them when available and do without the extra checks
//      when we are on an OS that has not been upgraded to the new crypto code.
//
/////////////////////////////////////////////////////////////////////////////


#define WINTRUST _T("wintrust.dll")
#define CRYPT32  _T("crypt32.dll")

#if !defined(USES_IU_CONVERSION) && defined(USES_CONVERSION)
#define USES_IU_CONVERSION USES_CONVERSION
#endif

//
// declare a global crypt32.dll library handler, so we don't
// need to load the library every time these functions are called.
// NOTE: we do not release the library though. When the process of
// calling this feature exits, the library is released.
// same as wintrust.dll
//
static HINSTANCE shWinTrustDllInst = NULL;
static HINSTANCE shCrypt32DllInst = NULL;


//
// define prototype for function WinVerifyTrust()
// and declare a global variable to point to this function
//
typedef HRESULT
(WINAPI * PFNWinVerifyTrust)(
                        HWND hwnd, GUID *ActionID, LPVOID ActionData);
PFNWinVerifyTrust pfnWinVerifyTrust = NULL;


//
// define prototype for function WTHelperProvDataFromStateData()
// and declare a global variable to point to this function
//
typedef CRYPT_PROVIDER_DATA *
(WINAPI * PFNWTHelperProvDataFromStateData)(
                        HANDLE hStateData);
PFNWTHelperProvDataFromStateData pfnWTHelperProvDataFromStateData = NULL;


//
// define prototype for function WTHelperGetProvSignerFromChain()
// and declare a global variable to point to this function
//
typedef CRYPT_PROVIDER_SGNR *
(WINAPI * PFNWTHelperGetProvSignerFromChain)(
                        CRYPT_PROVIDER_DATA *pProvData,
                        DWORD idxSigner,
                        BOOL fCounterSigner,
                        DWORD idxCounterSigner);
PFNWTHelperGetProvSignerFromChain pfnWTHelperGetProvSignerFromChain = NULL;


//
// define prototype for function PFNWTHelperGetProvCertFromChain()
// and declare a global variable to point to this function
//
typedef CRYPT_PROVIDER_CERT *
(WINAPI * PFNWTHelperGetProvCertFromChain)(
                        CRYPT_PROVIDER_SGNR *pSgnr,
                        DWORD idxCert);
PFNWTHelperGetProvCertFromChain pfnWTHelperGetProvCertFromChain = NULL;


//
// define prototype for function CryptHashPublicKeyInfo()
// and declare a global variable to point to this function
//
typedef BOOL
(WINAPI * PFNCryptHashPublicKeyInfo)(
                        HCRYPTPROV hCryptProv,
                        ALG_ID Algid,
                        DWORD dwFlags,
                        DWORD dwCertEncodingType,
                        PCERT_PUBLIC_KEY_INFO pInfo,
                        BYTE *pbComputedHash,
                        DWORD *pcbComputedHash);
PFNCryptHashPublicKeyInfo pfnCryptHashPublicKeyInfo = NULL;


//
// define prototype for function CertGetCertificateContextProperty()
// and declare a global variable to point to this function
//
typedef BOOL
(WINAPI * PFNCertGetCertificateContextProperty)(
                        PCCERT_CONTEXT pCertContext,
                        DWORD dwPropId,
                        void *pvData,
                        DWORD *pcbData);
PFNCertGetCertificateContextProperty pfnCertGetCertificateContextProperty = NULL;



/////////////////////////////////////////////////////////////////////////////
//
// pre-defined cert data to check against
//
/////////////////////////////////////////////////////////////////////////////

//
// The following are sha1 key identifier for two Microsoft root certificates.
// Note that they are "subject" hashes, not thumbprints or serial numbers.
//
static const BYTE rgbSignerRootKeyIds[40] = {
    0x4A, 0x5C, 0x75, 0x22, 0xAA, 0x46, 0xBF, 0xA4, 0x08, 0x9D,     // the original MS root
    0x39, 0x97, 0x4E, 0xBD, 0xB4, 0xA3, 0x60, 0xF7, 0xA0, 0x1D,     // don't know its validity period

    0x0E, 0xAC, 0x82, 0x60, 0x40, 0x56, 0x27, 0x97, 0xE5, 0x25,     // the new "son of MS root".  For reference,
    0x13, 0xFC, 0x2A, 0xE1, 0x0A, 0x53, 0x95, 0x59, 0xE4, 0xA4      // subject is "Microsoft Root Certificate Authority", valid from May 2001 to May 2021

};


//
// define the size of each hash values in the known id buffer
// for special certs.
//
const size_t ExpectedKnownCertHashSize = 20;

//
// this is the size of buffer to receive the cert hash value
// it must be not less than the largest number in the
// above-defined array
//
const size_t ShaBufSize = 20;

//
// id buffer to store SH1 hashing values of known Microsoft
// certs (signature) that we should recognize.
// Warning: the size of this buffer should match the sum
// of size_t values defined above.
//

// The AutoUpdate code allowed MSNBC and MSN certificates but BITS does not.
//
static const BYTE rgbSpecialCertId[140] = {
    0xB1,0x59,0xA5,0x2E,0x3D,0xD8,0xCE,0xCD,0x3A,0x9A,0x4A,0x7A,0x73,0x92,0xAA,0x8D,0xA7,0xE7,0xD6,0x7F,    // MS cert
//  0xB1,0xC7,0x75,0xE0,0x4A,0x9D,0xFD,0x23,0xB6,0x18,0x97,0x11,0x5E,0xF6,0xEA,0x6B,0x99,0xEC,0x76,0x1D,    // MSN cert
//  0x11,0xC7,0x10,0xF3,0xCB,0x6C,0x43,0xE1,0x66,0xEC,0x64,0x1C,0x7C,0x01,0x17,0xC4,0xB4,0x10,0x35,0x30,    // MSNBC cert
//  0x95,0x25,0x58,0xD4,0x07,0xDE,0x4A,0xFD,0xAE,0xBA,0x13,0x72,0x83,0xC2,0xB3,0x37,0x04,0x90,0xC9,0x8A,    // MSN Europe
    0x72,0x54,0x14,0x91,0x1D,0x6E,0x10,0x84,0x8E,0x0F,0xFA,0xA0,0xB0,0xA1,0x65,0xBF,0x44,0x8F,0x9F,0x6D,    // MS Europe
    0x20,0x5E,0x48,0x43,0xAB,0xAD,0x54,0x77,0x71,0xBD,0x8D,0x1A,0x3C,0xE0,0xE5,0x9D,0xF5,0xBD,0x25,0xF9,    // Old MS cert: 97~98
    0xD6,0xCD,0x01,0x90,0xB3,0x1B,0x31,0x85,0x81,0x12,0x23,0x14,0xB5,0x17,0xA0,0xAA,0xCE,0xF2,0x7B,0xD5,    // Old MS cert: 98~99
    0x8A,0xA1,0x37,0xF5,0x03,0x9F,0xE0,0x28,0xC9,0x26,0xAA,0x55,0x90,0x14,0x19,0x68,0xFA,0xFF,0xE8,0x1A,    // Old MS cert: 99~00
    0xF3,0x25,0xF8,0x67,0x07,0x29,0xE5,0x27,0xF3,0x77,0x52,0x34,0xE0,0x51,0x57,0x69,0x0F,0x40,0xC6,0x1C,    // Old MS Europe cert: 99~00
    0x6A,0x71,0xFE,0x54,0x8A,0x51,0x08,0x70,0xF9,0x8A,0x56,0xCA,0x11,0x55,0xF6,0x76,0x45,0x92,0x02,0x5A     // Old MS Europe cert: 98~99

};




/////////////////////////////////////////////////////////////////////////////
//
// Private Function ULONG CompareMem(PVOID pBlock1, PVOID pBlock2, ULONG Length)
//
//      This function acts in the same way as RtlCompareMemory()
//
//
// Input:   two pointers to two memory blocks, and a byte size to compare
// Return:  the number of bytes that compared as equal.
//          If all bytes compare as equal, the input Length is returned.
//          If any pointer is NULL, 0 is returned.
//
/////////////////////////////////////////////////////////////////////////////
ULONG CompareMem(const BYTE* pBlock1, const BYTE* pBlock2, ULONG Length)
{
    ULONG uLen = 0L;
    if (pBlock1 != NULL && pBlock2 != NULL)
    {
        for (; uLen < Length; uLen++, pBlock1++, pBlock2++)
        {
            if (*pBlock1 != *pBlock2) return uLen;
        }
    }
    return uLen;
}






/////////////////////////////////////////////////////////////////////////////
//
// Private Function VerifyMSRoot()
//
//      This function takes the passed-in certificate as a root cert,
//      and verifies its public key hash value is the same as one of the
//      known "Microsoft Root Authority" cert values.
//
//
// Input:   hCrypt32DllInst - handle point to loaded crypt32.dll library
//          pRootCert - the certificate context of the root cert
//
// Return:  HRESULT - result of execution, S_OK if matched.
//          the result code, in case of error, are code retuned by
//          crypt32.dll, with these the exception of E_INVALIDARG if
//          the passed-in parameters are NULL.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT VerifyMSRoot(
                     HINSTANCE hCrypt32DllInst,         // handle points to loaded crypt32.dll library
                     PCCERT_CONTEXT pRootCert
                     )
{
    HRESULT hr = S_OK;
    BYTE    rgbKeyId[ExpectedKnownCertHashSize];
    DWORD   cbKeyId = sizeof(rgbKeyId);

    LogInfo("VerifyMSRoot()");

    //
    // valid parameter values
    //
    if (NULL == hCrypt32DllInst || NULL == pRootCert)
    {
        hr = E_INVALIDARG;
        goto ErrHandler;
    }

    //
    // get the function we need from the passed in library handle
    // If not available, return error
    //
    if (NULL == (pfnCryptHashPublicKeyInfo = (PFNCryptHashPublicKeyInfo)
        GetProcAddress(hCrypt32DllInst, "CryptHashPublicKeyInfo")))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
        goto ErrHandler;
    }

    //
    // get the public key hash value of this cert
    //
    ZeroMemory(rgbKeyId, sizeof(rgbKeyId));
    if (!pfnCryptHashPublicKeyInfo(
                            0,                      // use default crypto svc provider
                            CALG_SHA1,              // use SHA algorithm
                            0,                      // dwFlags
                            X509_ASN_ENCODING,
                            &pRootCert->pCertInfo->SubjectPublicKeyInfo,
                            rgbKeyId,
                            &cbKeyId
                            ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrHandler;
    }

    //
    // compare the hash value of public key of this root cert with the known MS root cert values
    //
    if (ExpectedKnownCertHashSize != cbKeyId ||
        (cbKeyId != CompareMem(rgbSignerRootKeyIds, rgbKeyId, cbKeyId) &&
         cbKeyId != CompareMem(rgbSignerRootKeyIds + ExpectedKnownCertHashSize, rgbKeyId, cbKeyId)
        )
       )
    {
        hr = S_FALSE;
    }


ErrHandler:

    if (FAILED(hr))
    {
        LogError("returning %x", hr);
    }
    else
    {
        LogInfo("returning %x", hr);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// Private Function VerifySpecialMSCerts()
//
//      This function takes the passed-in certificate as a leaf cert,
//      and verifies its hash value matches the hash value of one of
//      known Microsoft special certs that does not have MS root.
//
//      See definition of rgbSpecialCertId[] for a complete list of certs to match.
//
// Input:   hCrypt32DllInst - handle point to loaded crypt32.dll library
//          pRootCert - the certificate context of the root cert
//          pbSha1HashVal - if not NULL, compare to this one, instead of
//                          hard-coded hash values. this is the case
//                          of working on 3rd party package
//
// Return:  HRESULT - result of execution, S_OK if matched.
//          if not matched, CERT_E_UNTRUSTEDROOT, or
//          E_INVALIDARG if arguments not right, or
//          crypt32.dll error returned by API calls
//
/////////////////////////////////////////////////////////////////////////////
HRESULT VerifyKnownCerts(
                             HINSTANCE hCrypt32DllInst,         // handle point to loaded crypt32.dll librar
                             PCCERT_CONTEXT pLeafCert,
                             pCERT_HASH_ARRAY pKnownCertsData
                             )
{
    HRESULT hr = S_FALSE;
    BYTE    btShaBuffer[ShaBufSize];
    DWORD   dwSize = sizeof(btShaBuffer);
    BYTE const * pId;

    LogInfo("VerifyKnownCerts()");

    //
    // valid parameter values
    //
    if (NULL == hCrypt32DllInst || NULL == pLeafCert)
    {
        hr = E_INVALIDARG;
        goto ErrHandler;
    }

    //
    // get the function we need from the passed in library handle
    // If not available, return error
    //
    if (NULL == (pfnCertGetCertificateContextProperty = (PFNCertGetCertificateContextProperty)
        GetProcAddress(hCrypt32DllInst, "CertGetCertificateContextProperty")))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
        goto ErrHandler;
    }

    //
    // find out the id hash of leaf cert
    //
    ZeroMemory(btShaBuffer, dwSize);
    if (!pfnCertGetCertificateContextProperty(
                        pLeafCert,                  // pCertContext
                        CERT_SHA1_HASH_PROP_ID, // dwPropId
                        btShaBuffer,
                        &dwSize
                        ))
    {
        hr = GetLastError();
        goto ErrHandler;
    }


    if (NULL == pKnownCertsData)
    {
        int     i;
        //
        // iterrate through all known id hash values to see if this file is signed
        // with any of these special certs.
        //
        hr = S_FALSE;
        for (i = 0,pId = rgbSpecialCertId;
             i < sizeof(rgbSpecialCertId)/ExpectedKnownCertHashSize;
             i++, pId += ExpectedKnownCertHashSize)
        {
            if (ExpectedKnownCertHashSize == dwSize &&
                dwSize == CompareMem(btShaBuffer, pId, dwSize))
            {
                //
                // found a matching known cert!
                //
                hr = S_OK;
                LogInfo("Found hash matching on #%d of %d MS certs!", i, sizeof(rgbSpecialCertId)/ExpectedKnownCertHashSize);
                break;
            }
        }
    }
    else
    {
        //
        // check if the retrieved hashing value matches the one passed in.
        //
        UINT i;
        LogInfo("Comparing retrieved hash value with passed-in key");
        hr = S_FALSE;
        for (i = 0, pId = pKnownCertsData->pCerts; i < pKnownCertsData->uiCount;
            i++, pId += HASH_VAL_SIZE)
        {
            if (dwSize == HASH_VAL_SIZE &&
                HASH_VAL_SIZE == CompareMem(btShaBuffer, pId, HASH_VAL_SIZE))
            {
                hr = S_OK;
                LogInfo("Found hash matching #%d of %d passed-in certs!",
                            i, pKnownCertsData->uiCount);
                break;
            }
        }
    }

ErrHandler:

    if (FAILED(hr))
    {
        LogError("returning %x", hr);
    }
    else
    {
        LogInfo("returning %x", hr);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// Private Function CheckWinTrust()
//
//      This function will return the HRESULT for the trust state on the
//      specified file. The file can be pointing to any URL or local file.
//      The verification will be done by the wintrust.dll.
//
//      dwCheckRevocation is WTD_REVOKE_NONE (default) or WTD_REVOKE_WHOLE_CHAIN.
//
// Input:   Fully qualified filename, dwCheckRevocation
// Return:  HRESULT - result of execution
//
/////////////////////////////////////////////////////////////////////////////

HRESULT CheckWinTrust(LPCTSTR pszFileName, pCERT_HASH_ARRAY pCertsData, DWORD dwCheckRevocation)
{
    LogInfo("CheckWinTrust()");

    // Now verify the file
    WINTRUST_DATA               winData;
    WINTRUST_FILE_INFO          winFile;
    GUID                        gAction = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    CRYPT_PROVIDER_DATA const   *pProvData = NULL;
    CRYPT_PROVIDER_SGNR         *pProvSigner = NULL;    // recursively call this function if not in test mode so we can show
    // UI for this non-MS but good cert.

    CRYPT_PROVIDER_CERT         *pProvCert = NULL;
    HRESULT                     hr = S_OK;

    //
    // dynamically load the wintrust.dll
    //
    if (NULL == shWinTrustDllInst)
    {
        if (NULL == (shWinTrustDllInst = LoadLibraryFromSystemDir(WINTRUST)))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LogError("Failed to load library " WINTRUST ", hr %x.", hr);
            goto Done;
        }
    }

    //
    // dynamically load the crypt32.dll, which will be used by the two
    // helper functions to verify the cert is MS cert
    //
    if (NULL == shCrypt32DllInst)
    {
        if (NULL == (shCrypt32DllInst = LoadLibraryFromSystemDir(CRYPT32)))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LogError("Failed to load library " CRYPT32  ", hr %x.", hr);
            goto Done;
        }
    }
    //
    // find the functions we need
    //
    if (NULL == (pfnWinVerifyTrust = (PFNWinVerifyTrust)
                GetProcAddress(shWinTrustDllInst, "WinVerifyTrust")) ||
        NULL == (pfnWTHelperProvDataFromStateData = (PFNWTHelperProvDataFromStateData)
                GetProcAddress(shWinTrustDllInst, "WTHelperProvDataFromStateData")) ||
        NULL == (pfnWTHelperGetProvSignerFromChain = (PFNWTHelperGetProvSignerFromChain)
                GetProcAddress(shWinTrustDllInst, "WTHelperGetProvSignerFromChain")) ||
        NULL == (pfnWTHelperGetProvCertFromChain = (PFNWTHelperGetProvCertFromChain)
                GetProcAddress(shWinTrustDllInst, "WTHelperGetProvCertFromChain")))
    {
        //
        // at least one function was not found in the loaded cryp32.dll libary.
        // we can not continue, jsut quit.
        // NOTE: this shouldn't happen since we have tried to get
        // the least common denomination of different version of this dll
        // on both IE4 and IE5
        //
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
        LogError("Failed to load procs from " CRYPT32 ", hr %x.", hr);
        goto Done;
    }


    //
    // initialize the data structure used to verify trust
    //
    winFile.cbStruct       = sizeof(WINTRUST_FILE_INFO);
    winFile.hFile          = INVALID_HANDLE_VALUE;
    winFile.pcwszFilePath  = pszFileName;
    winFile.pgKnownSubject = NULL;

    winData.cbStruct            = sizeof(WINTRUST_DATA);
    winData.pPolicyCallbackData = NULL;
    winData.pSIPClientData      = NULL;
    winData.dwUIChoice          = WTD_UI_NONE;
    winData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winData.dwUnionChoice       = WTD_CHOICE_FILE;
    winData.dwStateAction       = WTD_STATEACTION_VERIFY;
    winData.hWVTStateData       = 0;
    winData.dwProvFlags         = WTD_REVOCATION_CHECK_NONE;
    winData.pFile               = &winFile;

    if (dwCheckRevocation == WTD_REVOKE_WHOLECHAIN)
    {
        winData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
        winData.dwProvFlags = WTD_REVOCATION_CHECK_CHAIN;
    }

    //
    // verify the signature
    //
    hr = pfnWinVerifyTrust( (HWND)0, &gAction, &winData);

    //
    // Ignore errors when retrieving Cert Revocation List (CRL). This
    // just means the list itself couldn't be retrieved, not that the
    // current cert was invalid or revoked. (KenSh, 2002/01/17)
    //
    if (hr == CERT_E_REVOCATION_FAILURE)
    {
        hr = S_OK;
    }

    if (FAILED(hr))
    {
        //
        // The object isn't signed, so just leave.
        //
        LogError("WinVerifyTrust on '%S' found error 0x%0x.", pszFileName, hr);
        goto Return;
    }

    //
    // if come to here, it means all above verified okay.
    //
    // the rest of code is used to verify the signed cert is a known cert.
    //

    hr = S_FALSE;

    pProvData = pfnWTHelperProvDataFromStateData(winData.hWVTStateData);

    pProvSigner = pfnWTHelperGetProvSignerFromChain(
                                    (PCRYPT_PROVIDER_DATA) pProvData,
                                    0,      // first signer
                                    FALSE,  // not a counter signer
                                    0);

    //
    // check root cert then check leaf (signing) cert if that fails
    //
    // 0 is signing cert, csCertChain-1 is root cert
    //


    if (NULL == pCertsData)
    {
        //
        // if caller does not specify a hash value, then it means we want
        // to verify if this cert is known MS cert. We will first
        // try to find out if it is signed with a cert that has MS as root.
        //
        pProvCert =  pfnWTHelperGetProvCertFromChain(pProvSigner, pProvSigner->csCertChain - 1);
        hr = VerifyMSRoot(shCrypt32DllInst, pProvCert->pCert);
    }

    if (S_OK != hr)
    {
        pProvCert =  pfnWTHelperGetProvCertFromChain(pProvSigner, 0);

        hr = VerifyKnownCerts(shCrypt32DllInst, pProvCert->pCert, pCertsData);
    }

Return:

    //
    // free the wintrust state that was used to get the cert in the chain
    //
    winData.dwStateAction = WTD_STATEACTION_CLOSE;
    pfnWinVerifyTrust( (HWND)0, &gAction, &winData);

    //
    // Only the two functions checking MS cert will return S_FALSE
    //
    if (S_OK != hr)
        {
        LogError("CheckWinTrust() found file not signed by a known cert!");
        LogError("Digital Signatures on file %S are not trusted, hr %x",  pszFileName, hr);
        hr = TRUST_E_SUBJECT_NOT_TRUSTED;
        }
    else
        {
        LogInfo("CheckWinTrust(%S) returns S_OK", pszFileName);
        }

Done:
    if (NULL != shWinTrustDllInst)
    {
        FreeLibrary(shWinTrustDllInst);
        shWinTrustDllInst = NULL;
    }
    if (NULL != shCrypt32DllInst)
    {
        FreeLibrary(shCrypt32DllInst);
        shCrypt32DllInst = NULL;
    }

    return (hr);
}



/////////////////////////////////////////////////////////////////////////////
//
// Public Function VerifyFileTrust()
//
// This is a wrapper function for CheckWinTrust that both Whistler
// and WU classic code should use.
//
// Input:
//      szFileName - the file with complete path
//      pCertsData - hash value of a known good cert, or NULL to use the default list
//      fCheckRevocation - TRUE to check against a CRL, FALSE to skip the CRL check
//
// Return:  HRESULT - S_OK the file is signed with a valid known cert
//                    or error code.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT VerifyFileTrust(
                        IN LPCTSTR szFileName,
                        IN pCERT_HASH_ARRAY pCertsData,
                        BOOL fCheckRevocation /*=FALSE*/
                        )
{
    DWORD dwCheckRevocation = fCheckRevocation ? WTD_REVOKE_WHOLECHAIN : WTD_REVOKE_NONE;

    return CheckWinTrust(szFileName, pCertsData, dwCheckRevocation);
}

// **************************************************************************
static
BOOL UseFullPath(void)
{
    static BOOL s_fUseFullPath = TRUE;
    static BOOL s_fInit        = FALSE;

    OSVERSIONINFO   osvi;

    if (s_fInit)
        return s_fUseFullPath;

    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (GetVersionEx(&osvi))
    {
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
            (osvi.dwMajorVersion > 5 ||
             (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1)))
        {
            s_fUseFullPath = FALSE;
        }

        s_fInit = TRUE;
    }

    return s_fUseFullPath;
}

// **************************************************************************
HMODULE WINAPI LoadLibraryFromSystemDir(LPCTSTR szModule)
/*
        Loads a module in a safe way, immune to fake DLLs placed in the current directory or other
        parts of the default search path.
*/
{
    HRESULT hr = NOERROR;
    HMODULE hmod = NULL;
    TCHAR   szModulePath[MAX_PATH + 1];

    if (szModule == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (UseFullPath())
        {
        DWORD cch;

        // if the function call fails, make the buffer an empty string, so
        //  we will just use the dll name in the append below.
        cch = GetSystemDirectory(szModulePath, RTL_NUMBER_OF(szModulePath));
        if (cch == 0 || cch >= RTL_NUMBER_OF(szModulePath))
            {
            szModulePath[0] = _T('\0');
            }
        else
            {
            hr = StringCchCat( szModulePath, RTL_NUMBER_OF(szModulePath), _T("\\") );
            if (FAILED(hr))
                {
                SetLastError(HRESULT_CODE(hr));
                goto done;
                }
            }
        }
    else
        {
        szModulePath[0] = _T('\0');
        }

    hr = StringCchCat( szModulePath, RTL_NUMBER_OF(szModulePath), szModule );
    if (FAILED(hr))
        {
        SetLastError(HRESULT_CODE(hr));
        goto done;
        }

    hmod = LoadLibraryEx(szModulePath, NULL, 0);
    if (hmod == NULL)
        goto done;

done:
    return hmod;
}
