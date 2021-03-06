//-----------------------------------------------------------------------------
//
// chklist.h - Definitions and prototypes for the checklist pseudo-control
//
// Copyright 1996-2001, Microsoft Corporation
//
//
//-----------------------------------------------------------------------------

#ifndef _CHKLIST_H_
#define _CHKLIST_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//
// CheckList window class name
//
#define WC_CHECKLIST        "CHECKLIST_SSR"

//BOOL RegisterCheckListWndClass(void);


//
// CheckList check states
//
#define CLST_UNCHECKED      0   // == BST_UNCHECKED
#define CLST_CHECKED        1   // == BST_CHECKED
#define CLST_DISABLED       2   // == BST_INDETERMINATE
#define CLST_CHECKDISABLED  (CLST_CHECKED | CLST_DISABLED)

//
// CheckList window styles
//
#define CLS_1CHECK          0x0001
#define CLS_2CHECK          0x0002
//#define CLS_3CHECK          0x0003
//#define CLS_4CHECK          0x0004
#define CLS_CHECKMASK       0x000f
#define CLS_LEFTALIGN       0x0010		// Align the checkboxes at the left of the text (default = right)

//
// CheckList messages
//
// row is 0-based
// column is 1-based
//
#define CLM_SETCOLUMNWIDTH  (WM_USER + 1)   // lParam = width (dlg units) of a check column (default=32)
#define CLM_ADDITEM         (WM_USER + 2)   // wParam = pszName, lParam = item data, return = row
#define CLM_GETITEMCOUNT    (WM_USER + 3)   // no parameters
#define CLM_SETSTATE        (WM_USER + 4)   // wParam = row/column, lParam = state
#define CLM_GETSTATE        (WM_USER + 5)   // wParam = row/column, return = state
#define CLM_SETITEMDATA     (WM_USER + 6)   // wParam = row, lParam = item data
#define CLM_GETITEMDATA     (WM_USER + 7)   // wParam = row, return = item data
#define CLM_RESETCONTENT    (WM_USER + 8)   // no parameters
#define CLM_GETVISIBLECOUNT (WM_USER + 9)   // no parameters, return = # of visible rows
#define CLM_GETTOPINDEX     (WM_USER + 10)  // no parameters, return = index of top row
#define CLM_SETTOPINDEX     (WM_USER + 11)  // wParam = index of new top row
#define CLM_ENSUREVISIBLE   (WM_USER + 12)  // wParam = index of item to make fully visible

//
// CheckList notification messages
//
#define CLN_CLICK           (0U-1000U)      // lparam = PNM_CHECKLIST

typedef struct _NM_CHECKLIST
{
    NMHDR hdr;
    int iItem;                              // row (0-based)
    int iSubItem;                           // column (1-based)
    DWORD dwState;
    DWORD_PTR dwItemData;
} NM_CHECKLIST, *PNM_CHECKLIST;


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _CHKLIST_H_ */
