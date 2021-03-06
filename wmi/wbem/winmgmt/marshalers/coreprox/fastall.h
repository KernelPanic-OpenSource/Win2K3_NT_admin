/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTALL.H

Abstract:

    This is the include file for all clients of fastobj functionality.
    See member header files for documentation.

History:

    3/10/97     a-levn  Fully documented

--*/

#ifndef _FASTALL_H_
#define _FASTALL_H_

#include <corex.h>

// Parameter flow indicators.
// ==========================

#define READONLY
    // The value should be treated as read-only

#define ACQUIRED
    // Ownership of the object/pointer is acquired.

#define COPIED
    // The function makes a copy of the object/pointer.

#define PREALLOCATED
    // The out-param uses caller's memory.

#define NEWOBJECT
    // The return value or out parameter is a new
    // allocation which must be deallocated by
    // the caller if the call succeeds.

#define READWRITE
    // The in-param is will be treated as read-write,
    // but will not be deallocated.

#define INTERNAL
    // Returns a pointer to internal memory object
    // which should not be deleted.

#define ADDREF
    // On a parameter, indicates that the called
    // function will do an AddRef() on the interface
    // and retain it after the call completes.

#define TYPEQUAL L"CIMTYPE"

#include "fastcls.h"
#include "fastinst.h"

#endif
