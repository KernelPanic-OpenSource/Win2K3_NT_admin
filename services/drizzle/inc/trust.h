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
//  FILE:       TRUST.H
//
/////////////////////////////////////////////////////////////////////
//
//  DESC:   this header file declares functions used to make cabs
//          signed by certain providers trusted.
//
//  AUTHOR: Charles Ma, converted from WU CDMLIB
//  DATE:   10/4/2000
//
/////////////////////////////////////////////////////////////////////
//
//  Revision History:
//
//  Date        Author    Description
//  ~~~~        ~~~~~~    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  2002-01-18  KenSh     Added revocation check param to VerifyFileTrust
//
/////////////////////////////////////////////////////////////////////
//

//
// This file  is copied from the Windows AutoUpdate sources at \nt\enduser\windows.com\inc\trust.h
// modified to use BITS logging and remove UI.
//

#pragma once


//
// define the number of bytes needed to store a SHA1 hashing value
// of the public key
//
const UINT HASH_VAL_SIZE = 20;      

//
// define structure used to pass in the hash values to the following
// function in order to detect if one of the hash matches the
// public key of the leaf cert of a file.
//
typedef struct _HASH_STRUCT {
    UINT uiCount;
    PBYTE pCerts;
} CERT_HASH_ARRAY, *pCERT_HASH_ARRAY;


/////////////////////////////////////////////////////////////////////////////
//
// Public Function VerifyFileTrust()
//
// This is a wrapper function for CheckWinTrust that both Whistler
// and WU classic code should use.
//
// Input:   szFileName - the file with complete path
//          pbSha1HashVae - a pointer to a 20 byte long buffer, containing
//                          the signature SHA1 hashing value that should
//                          be used to check this file, or NULL for checking
//                          known Microsoft cert.
//          fCheckRevocation - whether the certificat revocation list (CRL) is
//                             checked to see whether any of the certs in the chain
//                             have been revoked. Never prompts the user to initiate
//                             a dial-up connection. Default = FALSE.
//
// Return:  HRESULT - S_OK the file is signed with a valid cert
//                    or error code.
//                    If the file is signed correctly but cert is not
//                    a known Microsoft cert, or it's SHA1 hash does not match
//                    the one passed in, then CERT_UNTRUSTED_ROOT is returned.
//
// Good Cert: Here is the deifnition of a good cert, in addition to the fact
//            that the signature must be valid and not expired.
//              (1) The signature was signed with a cert that has
//                  "Microsoft Root Authority" as root, or
//              (2) Parameter pbSha1HashVal is not NULL, and the file's SHA1
//                  hashing value of signature matches this value, or
//              (3) The signature was signed with one of the following known
//                  Microsoft cert's (they are not rooted to MS) and
//                  pbSha1HashVal is NULL.
//                  * Microsoft Corporation
//                  * Microsoft Corporation MSN (Europe)
//                  * Microsoft Corporation (Europe)
//
/////////////////////////////////////////////////////////////////////////////
HRESULT VerifyFileTrust(
                        IN LPCTSTR          szFileName,
                        IN pCERT_HASH_ARRAY pHashArray,
                        BOOL                fCheckRevocation = FALSE
                        );

