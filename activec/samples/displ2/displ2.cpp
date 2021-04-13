//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       displ2.cpp
//
//--------------------------------------------------------------------------

// displ2.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f displ2ps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "displ2.h"

#include "DsplMgr2.h"
#include <atlimpl.cpp>

using namespace ATL;

CComModule _Module;

const CLSID CLSID_DsplMgr2 = {0x885B3BAE,0x43F9,0x11D1,{0x9F,0xD4,0x00,0x60,0x08,0x32,0xDB,0x4A}};

// cut from mmc_i.c (yuck) !!!
const IID IID_IComponentData = {0x955AB28A,0x5218,0x11D0,{0xA9,0x85,0x00,0xC0,0x4F,0xD8,0xD5,0x65}};
const IID IID_IComponent = {0x43136EB2,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const IID IID_IResultDataCompare = {0xE8315A52,0x7A1A,0x11D0,{0xA2,0xD2,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
const IID IID_IResultOwnerData = {0x9CB396D8,0xEA83,0x11d0,{0xAE,0xF1,0x00,0xC0,0x4F,0xB6,0xDD,0x2C}};
const IID IID_IConsole = {0x43136EB1,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const IID IID_IHeaderCtrl = {0x43136EB3,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const IID IID_IContextMenuCallback = {0x43136EB7,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const IID IID_IContextMenuProvider = {0x43136EB6,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const IID IID_IExtendContextMenu = {0x4F3B7A4F,0xCFAC,0x11CF,{0xB8,0xE3,0x00,0xC0,0x4F,0xD8,0xD5,0xB0}};
const IID IID_IImageList = {0x43136EB8,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const IID IID_IResultData = {0x31DA5FA0,0xE0EB,0x11cf,{0x9F,0x21,0x00,0xAA,0x00,0x3C,0xA9,0xF6}};
const IID IID_IQuickFilter = {0x9757abb8,0x1b32,0x11d1,{0xa7,0xce,0x00,0xc0,0x4f,0xd8,0xd5,0x65}};
const IID IID_IConsoleNameSpace = {0xBEDEB620,0xF24D,0x11cf,{0x8A,0xFC,0x00,0xAA,0x00,0x3C,0xA9,0xF6}};
const IID IID_IPropertySheetCallback = {0x85DE64DD,0xEF21,0x11cf,{0xA2,0x85,0x00,0xC0,0x4F,0xD8,0xDB,0xE6}};
const IID IID_IPropertySheetProvider = {0x85DE64DE,0xEF21,0x11cf,{0xA2,0x85,0x00,0xC0,0x4F,0xD8,0xDB,0xE6}};
const IID IID_IExtendPropertySheet = {0x85DE64DC,0xEF21,0x11cf,{0xA2,0x85,0x00,0xC0,0x4F,0xD8,0xDB,0xE6}};
const IID IID_IControlbar = {0x69FB811E,0x6C1C,0x11D0,{0xA2,0xCB,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
const IID IID_IExtendControlbar = {0x49506520,0x6F40,0x11D0,{0xA9,0x8B,0x00,0xC0,0x4F,0xD8,0xD5,0x65}};
const IID IID_IToolbar = {0x43136EB9,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const IID IID_IConsoleVerb = {0xE49F7A60,0x74AF,0x11D0,{0xA2,0x86,0x00,0xC0,0x4F,0xD8,0xFE,0x93}};
const IID IID_ISnapinAbout = {0x1245208C,0xA151,0x11D0,{0xA7,0xD7,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
const IID IID_IMenuButton = {0x951ED750,0xD080,0x11d0,{0xB1,0x97,0x00,0x00,0x00,0x00,0x00,0x00}};
const IID IID_ISnapinHelp = {0xA6B15ACE,0xDF59,0x11D0,{0xA7,0xDD,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};

const IID IID_IExtendTaskPad = {0x8dee6511,0x554d,0x11d1,{0x9f,0xea,0x00,0x60,0x08,0x32,0xdb,0x4a}};
const IID IID_IEnumTASK      = {0x338698b1,0x5a02,0x11d1,{0x9f,0xec,0x00,0x60,0x08,0x32,0xdb,0x4a}};

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_DsplMgr2, CDsplMgr2)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

long g_ref_DataObject = 0; // used in DataObj.cpp
HINSTANCE g_hinst = 0;     // used in DsplMgr2.cpp

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
      g_hinst = hInstance;
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	// registers object
	return _Module.RegisterServer();
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}


