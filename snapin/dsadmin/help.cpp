//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       help.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "helpids.h"

const DWORD g_aHelpIDs_IDD_CHANGE_PASSWORD[]=
{
	IDC_NEW_PASSWORD, IDH_CONFIRM_PASSWORD,
	IDC_CONFIRM_PASSWORD,IDH_CONFIRM_PASSWORD,
	IDC_CHECK_PASSWORD_MUST_CHANGE,IDH_CHECK_PASSWORD_MUST_CHANGE,
	0, 0
};

const DWORD g_aHelpIDs_IDD_RID_FSMO_PAGE[]=
{
	IDC_EDIT_CURRENT_DC,IDH_EDIT_CURRENT_DC_RID,
	IDC_EDIT_CURRENT_FSMO_DC,IDH_EDIT_CURRENT_FSMO_DC_RID,
	IDC_CHANGE_FSMO,IDH_CHANGE_FSMO_RID,
  IDC_STATIC_FSMO_STATUS, IDH_NO_HELP,
  static_cast<ULONG>(IDC_STATIC), IDH_NO_HELP,
	IDC_STATIC_FSMO_DESC, IDH_NO_HELP,
	0,0
};

const DWORD g_aHelpIDs_IDD_PDC_FSMO_PAGE[]=
{
	IDC_CHANGE_FSMO,IDH_CHANGE_FSMO_PDC,
	IDC_EDIT_CURRENT_FSMO_DC,IDH_EDIT_CURRENT_FSMO_DC_PDC,
	IDC_EDIT_CURRENT_DC,IDH_EDIT_CURRENT_DC_PDC,
  IDC_STATIC_FSMO_STATUS, IDH_NO_HELP,
  static_cast<ULONG>(IDC_STATIC), IDH_NO_HELP,
	IDC_STATIC_FSMO_DESC, IDH_NO_HELP,
	0,0
};

const DWORD g_aHelpIDs_IDD_INFRA_FSMO_PAGE[]=
{
	IDC_EDIT_CURRENT_DC,IDH_EDIT_CURRENT_DC_INFRA,
	IDC_EDIT_CURRENT_FSMO_DC,IDH_EDIT_CURRENT_FSMO_DC_INFRA,
	IDC_CHANGE_FSMO,IDH_CHANGE_FSMO_INFRA,
  IDC_STATIC_FSMO_STATUS, IDH_NO_HELP,
  static_cast<ULONG>(IDC_STATIC), IDH_NO_HELP,
	IDC_STATIC_FSMO_DESC, IDH_NO_HELP,
  0,0
};

const DWORD g_aHelpIDs_IDD_PASSWORD[]=
{
	IDC_USER_NAME,IDH_USER_NAME,
	IDC_PASSWORD, IDH_CONFIRM_PASSWORD,
	0,0
};

const DWORD g_aHelpIDs_IDD_QUERY_FILTER[]=
{
	IDC_BUILTIN_QUERY_CHECK_LIST,IDH_BUILTIN_QUERY_CHECK_LIST,
	IDC_SHOW_BUILTIN_RADIO,IDH_SHOW_BUILTIN_RADIO,
	IDC_SHOW_CUSTOM_RADIO,IDH_SHOW_CUSTOM_RADIO,
	IDC_EDIT_CUSTOM_BUTTON,IDH_EDIT_CUSTOM_BUTTON,
	IDC_MAX_ITEM_COUNT_EDIT,IDH_MAX_ITEM_COUNT_EDIT,
	IDC_SHOW_ALL_RADIO,IDH_SHOW_ALL_RADIO,
	0,0
};

const DWORD g_aHelpIDs_IDD_RENAME_CONTACT[]=
{
	IDC_EDIT_OBJECT_NAME,IDH_EDIT_OBJECT_NAME2,
	IDC_FIRST_NAME_EDIT,IDH_FIRST_NAME_EDIT1,
	IDC_LAST_NAME_EDIT,IDH_LAST_NAME_EDIT1,
	IDC_DISP_NAME_EDIT,IDH_DISP_NAME_EDIT,
	0,0
};

const DWORD g_aHelpIDs_IDD_RENAME_GROUP[]=
{
	IDC_EDIT_OBJECT_NAME,IDH_EDIT_OBJECT_NAME3,
	IDC_NT4_USER_EDIT,IDH_NT4_USER_EDIT1,
	0,0
};

const DWORD g_aHelpIDs_IDD_RENAME_USER[]=
{
	IDC_NT5_DOMAIN_COMBO,IDH_NT5_DOMAIN_COMBO,
	IDC_LAST_NAME_EDIT,IDH_LAST_NAME_EDIT2,
	IDC_NT5_USER_EDIT,IDH_NT5_USER_EDIT,
	IDC_EDIT_OBJECT_NAME,IDH_EDIT_OBJECT_NAME4,
	IDC_NT4_DOMAIN_EDIT,IDH_NT4_DOMAIN_EDIT,
	IDC_NT4_USER_EDIT,IDH_NT4_USER_EDIT3,
	IDC_FIRST_NAME_EDIT,IDH_FIRST_NAME_EDIT2,
	IDC_EDIT_DISPLAY_NAME,IDH_EDIT_DISPLAY_NAME,
	0,0
};

const DWORD g_aHelpIDs_IDD_RENAME_COMPUTER[]= 
{
        0,0
};

const DWORD g_aHelpIDs_IDD_SELECT_DC[]=
{
	IDC_SELECTDC_DCLISTVIEW,IDH_SELECTDC_DCLISTVIEW,
	IDC_SELECTDC_DCEDIT,IDH_SELECTDC_DCEDIT,
	IDC_SELECTDC_DOMAIN,IDH_SELECTDC_DOMAIN,
	IDC_SELECTDC_BROWSE,IDH_SELECTDC_BROWSE,
  IDC_SELECTDC_DCEDIT_TITLE, IDH_NO_HELP,
  static_cast<ULONG>(IDC_STATIC),IDH_NO_HELP,
  0,0
};

const DWORD g_aHelpIDs_IDD_SELECT_DOMAIN[]=
{
	IDC_SELECTDOMAIN_DOMAIN,IDH_SELECTDOMAIN_DOMAIN,
	IDC_SELECTDOMAIN_BROWSE,IDH_SELECTDOMAIN_BROWSE,
	IDC_SELECTDOMAIN_LABEL,IDH_SELECTDOMAIN_DOMAIN,
	IDC_SELECTDOMAIN_LABEL,IDH_NO_HELP,
	IDC_SAVE_CURRENT_CHECK,IDH_SAVE_CURRENT_CHECK,
	0,0
};

// 481964-JonN-2002/04/10
const DWORD g_aHelpIDs_IDD_SELECT_FOREST[]=
{
	IDC_SELECTDOMAIN_DOMAIN,IDH_SELECTFOREST_EDIT,
	IDC_SELECTDOMAIN_LABEL,IDH_SELECTFOREST_EDIT,
	IDC_SELECTDOMAIN_LABEL,IDH_NO_HELP,
	IDC_SAVE_CURRENT_CHECK,IDH_SAVE_FOREST_CHECK,
	0,0
};

const DWORD g_aHelpIDs_IDD_BROWSE_CONTAINER[]=
{
  DSBID_CONTAINERLIST, IDH_BROWSE_FOR_OBJECT,
	0,0
};

const DWORD g_aHelpIDs_IDD_CREATE_NEW_QUERY[]=
{
  IDC_NAME_EDIT,              IDH_NAME_EDIT,
  IDC_DESCRIPTION_EDIT,       IDH_DESCRIPTION_EDIT,
  IDC_ROOT_EDIT,              IDH_ROOT_EDIT,
  IDC_BROWSE_BUTTON,          IDH_BROWSE_BUTTON,
  IDC_MULTI_LEVEL_CHECK,      IDH_MULTI_LEVEL_CHECK,
  IDC_QUERY_STRING_EDIT,      IDH_QUERY_STRING_EDIT,
  IDC_EDIT_BUTTON,            IDH_EDIT_BUTTON,
  IDOK,                       IDH_OK_BUTTON,
  IDCANCEL,                   IDH_CANCEL_BUTTON,
  0,0
};

const DWORD g_aHelpIDs_IDD_QUERY_STD_PAGE[]=
{
  IDC_NAME_COMBO,             IDH_NAME_COMBO,
  IDC_NAME_EDIT,              IDH_NAME_EDIT_QUERY_FORM,
  IDC_DESCRIPTION_COMBO,      IDH_DESCRIPTION_COMBO,
  IDC_DESCRIPTION_EDIT,       IDH_DESCRIPTION_EDIT_QUERY_FORM,
  0,0
};

const DWORD g_aHelpIDs_IDD_QUERY_USER_PAGE[]=
{
  IDC_NAME_COMBO,             IDH_NAME_COMBO,
  IDC_NAME_EDIT,              IDH_NAME_EDIT_QUERY_FORM,
  IDC_DESCRIPTION_COMBO,      IDH_DESCRIPTION_COMBO,
  IDC_DESCRIPTION_EDIT,       IDH_DESCRIPTION_EDIT_QUERY_FORM,
  IDC_DISABLED_ACCOUNTS_CHECK,IDH_DISABLED_ACCOUNTS_CHECK,
  IDC_NON_EXPIRING_PWD_CHECK, IDH_NON_EXPIRING_PWD_CHECK,
  IDC_LASTLOGON_COMBO,        IDH_LASTLOGON_COMBO,
  0,0
};

const DWORD g_aHelpIDs_IDD_FAVORITES_PROPERTY_PAGE[]=
{
  IDC_CN,                     IDH_CN,
  IDC_DESCRIPTION_EDIT,       IDH_DESCRIPTION_EDIT_FAVORITES,
  0,0
};

// NTRAID#NTBUG9-267769-2001/06/06-lucios - Begin
// JonN 12/19/01 267769 removed CDeleteDCDialog help
/*
const DWORD g_aHelpIDs_IDD_DELETE_DC_COMPUTER[]=
{
   IDC_DELETE_DC_BADREASON1,  IDH_DELETE_DC_BADREASON1,
   IDC_DELETE_DC_BADREASON2,  IDH_DELETE_DC_BADREASON2,
   IDC_DELETE_DC_GOODREASON,  IDH_DELETE_DC_GOODREASON,
   IDOK,                      IDH_DELETE,
   IDCANCEL,                  IDH_CANCEL_DELETE,
   0,0
};
*/
// NTRAID#NTBUG9-267769-2001/06/06-lucios - End


//Sites and Services
#ifdef FIXUPDC
const DWORD g_aHelpIDs_IDD_FIXUP_DC[]=
{
	IDC_FIXUP_DC_CHECK5,IDH_FIXUP_DC_CHECK5,
	IDC_FIXUP_DC_CHECK4,IDH_FIXUP_DC_CHECK4,
	IDC_FIXUP_DC_CHECK3,IDH_FIXUP_DC_CHECK3,
	IDC_FIXUP_DC_CHECK2,IDH_FIXUP_DC_CHECK2,
	IDC_FIXUP_DC_CHECK1,IDH_FIXUP_DC_CHECK1,
	IDC_FIXUP_DC_CHECK0,IDH_FIXUP_DC_CHECK0,
	0,0
};
#endif // FIXUPDC