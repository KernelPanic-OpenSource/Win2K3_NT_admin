/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MRSHBASE.H

Abstract:

    Marshaling base classes.

History:

--*/

#ifndef __MRSHBASE_H__
#define __MRSHBASE_H__

#include <unk.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <sync.h>
#include <fastall.h>
#include <wbemclasscache.h>
#include <wbemclasstoidmap.h>
#include <objindpacket.h>
#include <winntsec.h>

//***************************************************************************
//
//  class CBaseProxyBuffer
//
//  DESCRIPTION:
//
//  This class provides a base class implementation for an IRpcProxyBuffer.  As
//	the code necessary for performing this operation isn't necessarily so
//	obvious, but we use it in several places, this encapsulation is intended
//	to try and keep all of this maintainable.
//
//    Trick #1: This object is derived from IRpcProxyBuffer since IRpcProxyBuffer
//    is its "internal" interface --- the interface that does not delegate to the
//    aggregator. (Unlike in normal objects, where that interface is IUnknown)
//
//***************************************************************************

class CBaseProxyBuffer : public IRpcProxyBuffer
{
protected:
    CLifeControl* m_pControl;
    IUnknown* m_pUnkOuter;
	IRpcProxyBuffer*	m_pOldProxy;
    long m_lRef;
	REFIID m_riid;
	bool		m_fRemote;

protected:
    IRpcChannelBuffer* m_pChannel;
	IRpcChannelBuffer* GetChannel( void ) { return m_pChannel; };

	virtual void*	GetInterface( REFIID riid ) = 0;
	virtual void**	GetOldProxyInterfacePtr( void ) = 0;
	virtual void	ReleaseOldProxyInterface( void ) = 0;

public:
    CBaseProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter, REFIID riid);
    virtual ~CBaseProxyBuffer();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release(); 
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    STDMETHOD(Connect)(IRpcChannelBuffer* pChannel);
    STDMETHOD_(void, Disconnect)();
};

//***************************************************************************
// 
// class CProxySinkSecurity
//
// DESCRIPTION:
//
// facelets use this class to obtain the principal sid of the server 
// they are proxies to.  This is so async callback sinks passed through the
// facelets to the server can be set up to expect only calls from that 
// server identity.  It is intended that facelets construct this instance once
// as part of their state and make calls to it on each method involving a 
// callback sink being handed out. 
//
//***************************************************************************
class CProxySinkSecurity
{
    //
    // Init must be deferred in this class because usually when constructed,
    // the proxy does not yet have an IClientSecurity implementation which is 
    // required by this implementation.   So, we wait until the 
    // EnsureSinkSecurity() call is made to perform init.  
    //
    CCritSec m_cs;
    BOOL m_bInit;
    CNtSid m_PrincipalSid;
    IUnknown* m_pOwnerProxy; // it owns us, so don't hold ref.

    HRESULT EnsureInitialized();

public:

    //
    // users must be aware the CTOR can throw CX_MemoryException (m_cs CTOR)
    //
    CProxySinkSecurity( IUnknown* pOwnerProxy ) 
      : m_bInit( FALSE ), m_pOwnerProxy( pOwnerProxy ) {} 

    //
    // When a callback sink is handed to a facelet, about to be handed off to
    // a server ( e.g. GetObjectAsync ), the facelet ensures that the sink 
    // knows about the server principal its being handed off to so it can 
    // optionally perform access checks on the callback.  This method is a 
    // No-op if the sink does not implement _IWmiObjectSinkSecurity
    //
    HRESULT EnsureSinkSecurity( IWbemObjectSink* pSink );
};


//***************************************************************************
//
//  class CBaseStubBuffer
//
//  DESCRIPTION:
//
//  This class provides the stublet for the IWbemObjectSink interface.
//
//***************************************************************************

// Forward the definition
class CBaseStublet;

class CBaseStubBuffer : public CUnk
{
    friend CBaseStublet;

protected:

	IRpcStubBuffer*	m_pOldStub;

protected:

public:
    CBaseStubBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CUnk(pControl, pUnkOuter), m_pOldStub( NULL )
    {}

};

class CBaseStublet : public CImpl<IRpcStubBuffer, CBaseStubBuffer>
{
protected:
	REFIID	m_riid;
	long	m_lConnections;

protected:

	virtual IUnknown*	GetServerInterface( void ) = 0;
	virtual void**	GetServerPtr( void ) = 0;
	virtual void	ReleaseServerPointer( void ) = 0;

	IRpcStubBuffer*	GetOldStub( void )
	{	return m_pObject->m_pOldStub; }

public:
    CBaseStublet(CBaseStubBuffer* pObj, REFIID riid);
    ~CBaseStublet();

    STDMETHOD(Connect)(IUnknown* pUnkServer);
    STDMETHOD_(void, Disconnect)();
    STDMETHOD(Invoke)(RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer);
    STDMETHOD_(IRpcStubBuffer*, IsIIDSupported)(REFIID riid);
    STDMETHOD_(ULONG, CountRefs)();
    STDMETHOD(DebugServerQueryInterface)(void** ppv);
    STDMETHOD_(void, DebugServerRelease)(void* pv);
};

#endif
        
