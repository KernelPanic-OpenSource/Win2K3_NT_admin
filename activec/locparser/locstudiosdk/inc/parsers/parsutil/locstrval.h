//-----------------------------------------------------------------------------
//  
//  File: LocStrVal.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Written by: jstall
//  
//-----------------------------------------------------------------------------
 
#if !defined (PARSUTIL_LOCSTRVAL_H)
#define PARSUTIL_LOCSTRVAL_H

#include "LocChild.h"

#pragma warning(disable : 4275)

////////////////////////////////////////////////////////////////////////////////
class LTAPIENTRY CPULocStringValidation : public CPULocChild, public ILocStringValidation
{
// Construction
public:
	CPULocStringValidation(CPULocParser * pParent);

	DECLARE_CLUNKNOWN();

// COM Interfaces
public:
	//  Standard Debugging interface.
	void STDMETHODCALLTYPE AssertValidInterface() const;

	//  ILocStringValidation
	CVC::ValidationCode STDMETHODCALLTYPE ValidateString(
			const CLocTypeId &ltiType, const CLocTranslation &trTrans,
			CReporter *pReporter, const CContext &context);
};
////////////////////////////////////////////////////////////////////////////////

#pragma warning(default : 4275)

#endif
