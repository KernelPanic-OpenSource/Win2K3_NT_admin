/*---------------------------------------------------------------------------
  File: NT4Dom.h

  Comments: interface for the NT4 Domain class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#if !defined(AFX_NT4DOM_H__62E14C50_1AAC_11D3_8C81_0090270D48D1__INCLUDED_)
#define AFX_NT4DOM_H__62E14C50_1AAC_11D3_8C81_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Domain.h"
#include <map>

class CNT4Dom : public CDomain  
{
public:
	CNT4Dom();
	virtual ~CNT4Dom();
   HRESULT  GetEnumeration(BSTR sContainerName, BSTR sDomainName, BSTR m_sQuery, long attrCnt, LPWSTR * sAttr, ADS_SEARCHPREF_INFO prefInfo,BOOL  bMultiVal, IEnumVARIANT **& pVarEnum);
private:
	typedef std::map<_bstr_t,_bstr_t> CDCMap;
	CDCMap mDCMap;   //map to store domains and one of its DCs

	_bstr_t GetDC(_bstr_t sDomain);
};

#endif // !defined(AFX_NT4DOM_H__62E14C50_1AAC_11D3_8C81_0090270D48D1__INCLUDED_)
