#include "precomp.h"
#include <sddl.h>
#include "ssdlhelper.h"

SDDL sddl_wrapper;

SDDL::function_type 
SDDL::GetFunction(void)
{
    function_type return_function = DummyConvertStringSecurityDescriptorToSecurityDescriptor;
    if (lock_.acquire())
    {
        return_function = current_function_;
        if (current_function_==0)
        {
            HMODULE advapi = LoadLibrary(L"advapi32.dll");
            if (advapi)
            {
                current_function_ = (function_type)GetProcAddress(advapi,"ConvertStringSecurityDescriptorToSecurityDescriptorW");
                FreeLibrary(advapi);
            }

            if (current_function_==0)
            {
                current_function_ = DummyConvertStringSecurityDescriptorToSecurityDescriptor;
            }

            return_function = current_function_;
        }
        lock_.release();
    };
    return return_function;
};

BOOL SDDL::ConvertStringSecurityDescriptorToSecurityDescriptor(
  LPCTSTR StringSecurityDescriptor,          // security descriptor string
  DWORD StringSDRevision,                    // revision level
  PSECURITY_DESCRIPTOR *SecurityDescriptor,  // SD
  PULONG SecurityDescriptorSize              // SD size
)
{
  return (sddl_wrapper.GetFunction())(StringSecurityDescriptor, 
                                      StringSDRevision, 
                                      SecurityDescriptor, 
                                      SecurityDescriptorSize);
};


BOOL SDDL::DummyConvertStringSecurityDescriptorToSecurityDescriptor(
  LPCTSTR StringSecurityDescriptor,          // security descriptor string
  DWORD StringSDRevision,                    // revision level
  PSECURITY_DESCRIPTOR *SecurityDescriptor,  // SD
  PULONG SecurityDescriptorSize              // SD size
)
{
if (SecurityDescriptor==0)
  return ERROR_INVALID_PARAMETER;
if (SecurityDescriptorSize)
  *SecurityDescriptorSize = 0;
*SecurityDescriptor = 0;
return TRUE;
};


bool SDDL::hasSDDLSupport() 
{ 
    return sddl_wrapper.GetFunction() != DummyConvertStringSecurityDescriptorToSecurityDescriptor; 
}
