/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PSTRVAL.H

History:

--*/

 
#ifndef PBASE_PSTRVAL_H
#define PBASE_PSTRVAL_H

extern const IID IID_ILocStringValidation;

DECLARE_INTERFACE_(ILocStringValidation, IUnknown)
{
	//
	//  IUnknown standard Interface
	//
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR*ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	//
	//  Standard Debugging interfaces
	//
 	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD PURE;

	STDMETHOD_(CVC::ValidationCode, ValidateString)
		(THIS_ const CLocTypeId REFERENCE, const CLocString REFERENCE,
				CReporter *) PURE;
	
};

#endif
