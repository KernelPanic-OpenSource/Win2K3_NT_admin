#ifndef _UTIL_H_8_25_2000
#define _UTIL_H_8_25_2000

#define INIT_SIZE 1024


//General Utility Functions
DWORD ResizeByTwo( LPTSTR *ppBuffer,
                   LONG *pSize );
BOOL StringCopy( LPWSTR *ppDest, LPWSTR pSrc);

LONG ReadFromIn(LPTSTR *ppBuffer);




//+----------------------------------------------------------------------------
//  Function: ConvertStringToInterger  
//  Synopsis: Converts string to integer. Returns false if string is outside
//				  the range on an integer  
//  Arguments:pszInput: integer in string format
//				 :pIntOutput:takes converted integer  
////  Returns:TRUE is successful.
//-----------------------------------------------------------------------------
BOOL ConvertStringToInterger(LPWSTR pszInput, int* pIntOutput);









#endif