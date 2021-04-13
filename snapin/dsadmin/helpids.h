//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       helpids.h
//
//--------------------------------------------------------------------------

#ifdef IDH_NO_HELP
#undef IDH_NO_HELP
#define IDH_NO_HELP static_cast<ULONG>(-1L)
#endif

#define DSADMIN_CONTEXT_HELP_FILE L"dsadmin.hlp"
#define DSADMIN_LINKED_HELP_FILE L"ADconcepts.chm"
#define DSADMIN_DEFAULT_TOPIC L"ADconcepts.chm::/dsadmin_top.htm"
#define DSSITES_DEFAULT_TOPIC L"ADconcepts.chm::/dssite_top.htm"

// FSMO transfer topics
#define DSADMIN_MOREINFO_RID_POOL_FSMO_TOPIC L"ADconcepts.chm::/FSMO_RID_POOL_ForcefulSeizure.htm"
#define DSADMIN_MOREINFO_PDC_FSMO_TOPIC L"ADconcepts.chm::/FSMO_PDC_ForcefulSeizure.htm"
#define DSADMIN_MOREINFO_INFRASTUCTURE_FSMO_TOPIC L"ADconcepts.chm::/FSMO_INFRASTRUCTURE_ForcefulSeizure.htm"

//
#define DSADMIN_MOREINFO_LOCAL_LOGIN_ERROR L"ADConcepts.chm::/sag_ADlocalNotAD.htm"
#define DSADMIN_MOREINFO_FSMO_TARGET_DC_IS_GC L"ADConcepts.chm::/sag_ADFSMOs.htm"

#define	IDH_BUILTIN_QUERY_CHECK_LIST	300000102
#define	IDH_CHANGE_FSMO_INFRA	300000552
#define	IDH_CHANGE_FSMO_PDC	300000202
#define	IDH_CHANGE_FSMO_RID	300000602
#define	IDH_CHECK_PASSWORD_MUST_CHANGE	300000052
#define	IDH_CONFIRM_PASSWORD	300000051
#define	IDH_DISP_NAME_EDIT	300000453
#define	IDH_EDIT_CURRENT_DC_INFRA	300000550
#define	IDH_EDIT_CURRENT_DC_PDC	300000200
#define	IDH_EDIT_CURRENT_DC_RID	300000600
#define	IDH_EDIT_CURRENT_FSMO_DC_INFRA	300000551
#define	IDH_EDIT_CURRENT_FSMO_DC_PDC	300000201
#define	IDH_EDIT_CURRENT_FSMO_DC_RID	300000601
#define	IDH_EDIT_CUSTOM_BUTTON	300000104
#define	IDH_EDIT_DISPLAY_NAME	300000153
#define	IDH_EDIT_OBJECT_NAME2	300000450
#define	IDH_EDIT_OBJECT_NAME3	300000500
#define	IDH_EDIT_OBJECT_NAME4	300000150
#define	IDH_FIRST_NAME_EDIT1	300000451
#define	IDH_FIRST_NAME_EDIT2	300000151
#define	IDH_LAST_NAME_EDIT1	300000452
#define	IDH_LAST_NAME_EDIT2	300000152
#define	IDH_MAX_ITEM_COUNT_EDIT	300000105
#define	IDH_NT4_DOMAIN_EDIT	300000156
#define	IDH_NT4_USER_EDIT1	300000501
#define	IDH_NT4_USER_EDIT3	300000157
#define	IDH_NT5_DOMAIN_COMBO	300000155
#define	IDH_NT5_USER_EDIT	300000154
#define	IDH_SELECTDC_DCEDIT	300000353
#define	IDH_SELECTDC_DCLISTVIEW	300000355
#define	IDH_SELECTDC_DOMAIN	300000350
#define	IDH_SELECTDOMAIN_BROWSE	300000401
#define	IDH_SELECTDOMAIN_DOMAIN	300000400
#define	IDH_SHOW_ALL_RADIO	300000100
#define	IDH_SHOW_BUILTIN_RADIO	300000101
#define	IDH_SHOW_CUSTOM_RADIO	300000103
#define	IDH_USER_NAME	300000300
#define IDH_BROWSE_FOR_OBJECT   300000650
#define IDH_SELECTDC_BROWSE	300000700
// for future expansion, use 300000709 to 300000725
#define IDH_SAVE_CURRENT_CHECK  300000707
#define	IDH_SELECTFOREST_EDIT	300000709
#define IDH_SAVE_FOREST_CHECK   300000710

//
// Name Mapping
//
#define IDH_EDIT_USER_ACCOUNT           300000701
#define IDH_LISTVIEW_X509               300000702
#define IDH_BUTTON_ADD                  300000703
#define IDH_BUTTON_EDIT                 300000704
#define IDH_BUTTON_REMOVE               300000705
#define IDH_LISTVIEW_KERBEROS           300000706
#define IDH_EDIT_KERBEROS_NAME          300000708

// Certificate Properties
#define IDH_SIMCERT_LISTVIEW            300000720
#define IDH_SIMCERT_CHECK_ISSUER        300000721
#define IDH_SIMCERT_CHECK_SUBJECT       300000722
 
//
// New Saved Query dialog
//
#define IDH_NAME_EDIT                   300000801
#define IDH_DESCRIPTION_EDIT            300000802
#define IDH_ROOT_EDIT                   300000803
#define IDH_BROWSE_BUTTON               300000804
#define IDH_MULTI_LEVEL_CHECK           300000805
#define IDH_QUERY_STRING_EDIT           300000806
#define IDH_EDIT_BUTTON                 300000807
#define IDH_OK_BUTTON                   300000808
#define IDH_CANCEL_BUTTON               300000809

//
// Saved Query forms
//
#define IDH_NAME_COMBO                  300000810
#define IDH_NAME_EDIT_QUERY_FORM        300000811
#define IDH_DESCRIPTION_COMBO           300000812
#define IDH_DESCRIPTION_EDIT_QUERY_FORM 300000813
#define IDH_DISABLED_ACCOUNTS_CHECK     300000814
#define IDH_NON_EXPIRING_PWD_CHECK      300000815
#define IDH_LASTLOGON_COMBO             300000816

//
// Favorites folder property page
//
#define IDH_CN                          300000817
#define IDH_DESCRIPTION_EDIT_FAVORITES  300000818


// NTRAID#NTBUG9-267769-2001/06/06-lucios - Begin
// IDD_DELETE_DC_COMPUTER
//
// JonN 12/19/01 267769 removed CDeleteDCDialog help
/*
#define IDH_DELETE_DC_BADREASON1	300000819
#define IDH_DELETE_DC_BADREASON2	300000820
#define IDH_DELETE_DC_GOODREASON	300000821
#define IDH_DELETE			300000822
#define IDH_CANCEL_DELETE		300000823
*/
// NTRAID#NTBUG9-267769-2001/06/06-lucios - End

//sites and services
#ifdef FIXUPDC
#define	IDH_FIXUP_DC_CHECK0	300000250
#define	IDH_FIXUP_DC_CHECK1	300000251
#define	IDH_FIXUP_DC_CHECK2	300000252
#define	IDH_FIXUP_DC_CHECK3	300000253
#define	IDH_FIXUP_DC_CHECK4	300000254
#define	IDH_FIXUP_DC_CHECK5	300000255
#endif // FIXUPDC

extern const DWORD g_aHelpIDs_IDD_CHANGE_PASSWORD[];
#ifdef FIXUPDC
extern const DWORD g_aHelpIDs_IDD_FIXUP_DC[];
#endif // FIXUPDC
extern const DWORD g_aHelpIDs_IDD_PASSWORD[];
extern const DWORD g_aHelpIDs_IDD_QUERY_FILTER[];
extern const DWORD g_aHelpIDs_IDD_RENAME_USER[];
extern const DWORD g_aHelpIDs_IDD_SELECT_DC[];
extern const DWORD g_aHelpIDs_IDD_SELECT_DOMAIN[];
extern const DWORD g_aHelpIDs_IDD_SELECT_FOREST[]; // 481964-JonN-2002/04/10
extern const DWORD g_aHelpIDs_IDD_RENAME_GROUP[];
extern const DWORD g_aHelpIDs_IDD_RENAME_CONTACT[];
extern const DWORD g_aHelpIDs_IDD_RID_FSMO_PAGE[];
extern const DWORD g_aHelpIDs_IDD_PDC_FSMO_PAGE[];
extern const DWORD g_aHelpIDs_IDD_INFRA_FSMO_PAGE[];
extern const DWORD g_aHelpIDs_IDD_RENAME_COMPUTER[];
extern const DWORD g_aHelpIDs_IDD_BROWSE_CONTAINER[];
extern const DWORD g_aHelpIDs_IDD_CREATE_NEW_QUERY[];
extern const DWORD g_aHelpIDs_IDD_QUERY_STD_PAGE[];
extern const DWORD g_aHelpIDs_IDD_QUERY_USER_PAGE[];
extern const DWORD g_aHelpIDs_IDD_FAVORITES_PROPERTY_PAGE[];
// JonN 12/19/01 267769 removed CDeleteDCDialog help
// extern const DWORD g_aHelpIDs_IDD_DELETE_DC_COMPUTER[];

