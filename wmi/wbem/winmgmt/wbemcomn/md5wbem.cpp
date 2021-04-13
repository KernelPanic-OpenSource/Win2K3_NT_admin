/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "md5.h"
#include "md5wbem.h"




void MD5::Transform(
    IN  LPVOID  pInputValue,
    IN  UINT    uValueLength,
    OUT BYTE    MD5Buffer[16]
    )
{
    MD5_CTX Ctx;
    MD5Init(&Ctx);

    MD5Update(
        &Ctx,
        (unsigned char *) pInputValue,
        uValueLength
    );

    MD5Final(&Ctx);
    CopyMemory(MD5Buffer, Ctx.digest, 16);
}

void MD5::ContinueTransform(
    IN  LPVOID  pInputValue,
    IN  UINT    uValueLength,
    IN OUT BYTE    MD5Buffer[16]
    )
{
    MD5_CTX Ctx;
    MD5Init(&Ctx);      // zeros buffer, and counts

    CopyMemory( Ctx.buf, MD5Buffer, 16 );

    MD5Update(
        &Ctx,
        (unsigned char *) pInputValue,
        uValueLength
    );

    MD5Final(&Ctx);
    CopyMemory(MD5Buffer, Ctx.digest, 16);
}

