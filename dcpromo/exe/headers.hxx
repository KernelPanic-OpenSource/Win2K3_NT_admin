// Copyright (C) 1997 Microsoft Corporation
// 
// All header files used by this project; for the purpose of creating a
// pre-compiled header file
// 
// 12-12-97 sburns
//
// This file must be named with a .hxx extension due to some weird and
// inscrutible requirement of BUILD.



#ifndef HEADERS_HXX_INCLUDED 
#define HEADERS_HXX_INCLUDED



#include <burnslib.hpp>
#include <comdef.h>
#include <process.h>

#include <regstr.h>
#include <msi.h>
#include <lmaccess.h>
#include <dsrolep.h>
#include <iphlpapi.h>
#include <shlobjp.h>
#include <winldap.h>
#include <ntldap.h>
#include <wincrui.h>
#include <Rpcndr.h>
#include <initguid.h>
#include <dsclient.h>
#include <scesetup.h>
#include <dnsmgr.h>  // DNS_SETUP_*


extern Popup popup; 



const int DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY      = 64;  // 106840
const int DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8 = 155; // 54054


// CODEWORK: Need this until throws are removed.

#pragma warning (disable: 4702)


#pragma hdrstop



#endif   // HEADERS_HXX_INCLUDED
