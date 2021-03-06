//+-------------------------------------------------------------------
//
//  File:        DUMPSEC.hxx
//
//  Contents:    class encapsulating file security.
//
//  Classes:     CDumpSecurity
//
//  History:     Nov-93        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __DUMPSEC__
#define __DUMPSEC__

#include "t2.hxx"

//+-------------------------------------------------------------------
//
//  Class:      CDumpSecurity
//
//  Purpose:    encapsulation of NT File security descriptor with functions
//              to get SIDs and iterate through the ACES in the DACL.
//
//--------------------------------------------------------------------
class CDumpSecurity
{
public:

    CDumpSecurity(LPWSTR filename);

   ~CDumpSecurity();

ULONG Init();
ULONG GetSDOwner(SID **psid);
ULONG GetSDGroup(SID **pgsid);
VOID  ResetAce(SID *psid);
LONG  GetNextAce(ACE_HEADER **paceh);
BOOL  IsDaclNull() const { return _bNullDacl;}

private:

    BYTE       * _psd      ;
    LPWSTR     _pfilename  ;
    LPWSTR     _pwfilename ;
    ACL        * _pdacl      ;
    ACE_HEADER * _pah        ;
    SID        * _psid       ;
    ULONG        _cacethissid;  // a dinosaur from the cretaceous
    BOOL       _bNullDacl;
};

#endif // __DUMPSEC__





