/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SERVUTIL.H

Abstract:

	Declares a set of general purpose service utilitils.

History:

  a-davj  04-Mar-97   Created.

--*/

#ifndef _SERVUTIL_H_
#define _SERVUTIL_H_

BOOL StopService(LPCTSTR pServiceName, DWORD dwMaxWait=0);

#endif
