//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _BRANCHES_H
#define _BRANCHES_H

#include <mmc.h>
#include <crtdbg.h>
#include "globals.h"
#include "resource.h"
#include "LocalRes.h"

class CComponentData;

class CDelegationBase {
public:
    CDelegationBase();
    virtual ~CDelegationBase();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) = 0;
    virtual void GetChildColumnInfo(RESULTDATAITEM *rdi) { }
    virtual const GUID & getNodeType() { _ASSERT(FALSE); return IID_NULL; }
    
    virtual const LPARAM GetCookie() { return reinterpret_cast<LPARAM>(this); }
    virtual const int GetBitmapIndex() = 0;

	virtual CDelegationBase *GetChildPtr(int index) { return NULL; }
    
    virtual HRESULT GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions) { return S_FALSE; }

	virtual HRESULT SelectItem(IConsole *pConsole) 
	{
		return pConsole->SelectScopeItem(HSCOPEITEM(m_handle));
	}
    
public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent) { return S_FALSE; }
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem) { return S_FALSE; }
    virtual HRESULT OnAddImages(IImageList *pImageList, HSCOPEITEM hsi);
    virtual HRESULT OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect) { return S_FALSE; }
    virtual HRESULT OnRefresh(IConsole *pConsole) { return S_FALSE; }
	virtual HRESULT OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype) { return S_FALSE; }
    virtual HRESULT OnRename(LPOLESTR pszNewName) { return S_FALSE; }

	// cut / copy / paste implementation
	virtual HRESULT OnPaste(IConsole *pConsole, CComponentData *pComponentData, CDelegationBase *pPasted) { return S_FALSE; }
	virtual HRESULT OnQueryPaste(CDelegationBase *pPasted) { return S_FALSE; }
	virtual HRESULT OnDelete(IConsole *pConsole) { return S_FALSE; }
	virtual HRESULT OnDeleteScopeItem(IConsoleNameSpace *pConsoleNameSpace) { _ASSERT(FALSE); return S_FALSE; }

public:
    static HBITMAP m_pBMapSm;
    static HBITMAP m_pBMapLg;

	HANDLE GetHandle() { return m_handle; }
	void SetHandle(HANDLE handle) { m_handle = handle; }
    
protected:
    static void LoadBitmaps() {
        m_pBMapSm = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_SMICONS));
        m_pBMapLg = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_LGICONS));
    }
    
    BOOL bExpanded;
    
private:
    // {2974380B-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
	HANDLE m_handle;
};

#endif // _BRANCHES_H