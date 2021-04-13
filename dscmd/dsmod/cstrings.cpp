//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      cstrings.cpp
//
//  Contents:  Defines the global strings that are used in the parser
//
//  History:   07-Sep-2000    JeffJon  Created
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "commonstrings.cpp"

//
// The command line executable name
//
PCWSTR g_pszDSCommandName           = L"dsmod";

//
// Object types as are typed on the command line
//
PCWSTR g_pszOU                      = L"ou";
PCWSTR g_pszUser                    = L"user";
PCWSTR g_pszContact                 = L"contact";
PCWSTR g_pszComputer                = L"computer";
PCWSTR g_pszGroup                   = L"group";
PCWSTR g_pszServer                  = L"server";
PCWSTR g_pszQuota                   = L"quota";
PCWSTR g_pszPartition               = L"partition";

//
// User and Contact switches
//
PCWSTR g_pszArg1UserUPN             = L"upn"; 
PCWSTR g_pszArg1UserFirstName       = L"fn";
PCWSTR g_pszArg1UserMiddleInitial   = L"mi";
PCWSTR g_pszArg1UserLastName        = L"ln";
PCWSTR g_pszArg1UserDisplayName     = L"display";
PCWSTR g_pszArg1UserEmpID           = L"empid";
PCWSTR g_pszArg1UserPassword        = L"pwd";
PCWSTR g_pszArg1UserOffice          = L"office";
PCWSTR g_pszArg1UserTelephone       = L"tel"; 
PCWSTR g_pszArg1UserEmail           = L"email";
PCWSTR g_pszArg1UserHomeTelephone   = L"hometel";
PCWSTR g_pszArg1UserPagerNumber     = L"pager"; 
PCWSTR g_pszArg1UserMobileNumber    = L"mobile"; 
PCWSTR g_pszArg1UserFaxNumber       = L"fax";
PCWSTR g_pszArg1UserIPTel           = L"iptel";
PCWSTR g_pszArg1UserWebPage         = L"webpg";
PCWSTR g_pszArg1UserTitle           = L"title";
PCWSTR g_pszArg1UserDepartment      = L"dept"; 
PCWSTR g_pszArg1UserCompany         = L"company";
PCWSTR g_pszArg1UserManager         = L"mgr";
PCWSTR g_pszArg1UserHomeDirectory   = L"hmdir";
PCWSTR g_pszArg1UserHomeDrive       = L"hmdrv";
PCWSTR g_pszArg1UserProfilePath     = L"profile";
PCWSTR g_pszArg1UserScriptPath      = L"loscr";
PCWSTR g_pszArg1UserMustChangePwd   = L"mustchpwd";
PCWSTR g_pszArg1UserCanChangePwd    = L"canchpwd";
PCWSTR g_pszArg1UserReversiblePwd   = L"reversiblepwd";
PCWSTR g_pszArg1UserPwdNeverExpires = L"pwdneverexpires";
PCWSTR g_pszArg1UserAccountExpires  = L"acctexpires";
PCWSTR g_pszArg1UserDisableAccount  = L"disabled";

//
// Computer switches
//
PCWSTR g_pszArg1ComputerLocation    = L"loc";
PCWSTR g_pszArg1ComputerDisabled    = L"disabled";
PCWSTR g_pszArg1ComputerReset       = L"reset";

//
// Group switches
//
PCWSTR g_pszArg1GroupSAMName        = L"samid";
PCWSTR g_pszArg1GroupSec            = L"secgrp";
PCWSTR g_pszArg1GroupScope          = L"scope";
PCWSTR g_pszArg1GroupAddMember      = L"addmbr";
PCWSTR g_pszArg1GroupRemoveMember   = L"rmmbr";
PCWSTR g_pszArg1GroupChangeMember   = L"chmbr";

//
// Server switches
//
PCWSTR g_pszArg1ServerIsGC          = L"isgc";

//
// Quota switches
//
PCWSTR g_pszArg1QuotaQLimit      = L"qlimit";

//
// Partition switches
//
PCWSTR g_pszArg1PartitionQDefault   = L"qdefault";
PCWSTR g_pszArg1PartitionQtmbstnwt  = L"qtmbstnwt";
