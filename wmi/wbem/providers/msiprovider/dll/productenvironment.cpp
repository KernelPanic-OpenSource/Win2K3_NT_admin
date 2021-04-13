// ProductEnvironment.cpp: implementation of the CProductEnvironment class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ProductEnvironment.h"

#include "ExtendString.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProductEnvironment::CProductEnvironment(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CProductEnvironment::~CProductEnvironment()
{

}

HRESULT CProductEnvironment::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProduct[BUFF_SIZE];
    WCHAR wcQuery[BUFF_SIZE];
    WCHAR wcProductCode[39];
    DWORD dwBufSize;
    UINT uiStatus;
    bool bMatch = false;

    bool bResource, bProduct;

	CStringExt wcResource;

	// safe operation
	// lenght is smaller than BUFF_SIZE ( 512 )
    wcscpy(wcQuery, L"select distinct `Environment`, `Component_` from Environment");

	LPWSTR Buffer = NULL;
	LPWSTR dynBuffer = NULL;

	DWORD dwDynBuffer = 0L;

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if(CreateProductString(wcProductCode, wcProduct)){

			//Open our database
            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Environment", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

						// safe operation
                        wcResource.Copy ( L"Win32_EnvironmentSpecification.CheckID=\"" );

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
						GetBufferToPut ( hRecord, 1, dwBufSize, wcBuf, dwDynBuffer, dynBuffer, Buffer );

                        if ( Buffer && Buffer [ 0 ] != 0 )
						{
							wcResource.Append ( 3, Buffer, wcProductCode, L"\"" );

							if ( dynBuffer && dynBuffer [ 0 ] != 0 )
							{
								dynBuffer [ 0 ] = 0;
							}

                            dwBufSize = BUFF_SIZE;
							GetBufferToPut ( hRecord, 2, dwBufSize, wcBuf, dwDynBuffer, dynBuffer, Buffer );

                            if ( ValidateComponentName ( msidata.GetDatabase (), wcProductCode, Buffer ) )
							{
								if ( dynBuffer && dynBuffer [ 0 ] != 0 )
								{
									dynBuffer [ 0 ] = 0;
								}

                                PutKeyProperty(m_pObj, pCheck, wcResource, &bResource, m_pRequest);
                                PutKeyProperty(m_pObj, pProduct, wcProduct, &bProduct, m_pRequest);
                            //====================================================

                            //----------------------------------------------------

                                if(bResource && bProduct) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
                                }
                            }
							else
							{
								if ( dynBuffer && dynBuffer [ 0 ] != 0 )
								{
									dynBuffer [ 0 ] = 0;
								}
							}
                        }

                        m_pObj->Release();
                        m_pObj = NULL;

                        g_fpMsiCloseHandle(hRecord);

                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);
                    }
                }
            }
			catch(...)
			{
				if ( dynBuffer )
				{
					delete [] dynBuffer;
					dynBuffer = NULL;
				}

                g_fpMsiCloseHandle(hRecord);
                g_fpMsiViewClose(hView);
                g_fpMsiCloseHandle(hView);

				msidata.CloseDatabase ();

				if(m_pObj)
				{
					m_pObj->Release();
					m_pObj = NULL;
				}

                throw;
            }

            g_fpMsiCloseHandle(hRecord);
            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

			msidata.CloseDatabase ();
        }
    }

    if ( dynBuffer )
	{
		delete [] dynBuffer;
		dynBuffer = NULL;
	}
	
	return hr;
}