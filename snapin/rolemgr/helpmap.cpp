#define BEGIN_HELP_ARRAY(x)	const DWORD g_aHelpIDs_##x[]={ ,
#define END_HELP_ARRAY		 0, 0 };
#define ARRAY_ENTRY(x,y) y, IDH_##x_##y,

//IDD_ADD_GROUP
BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    ARRAY_ENTRY(IDD_ADD_GROUP,IDC_LIST)
END_HELP_ARRAY

/*
//IDD_ADD_OPERATION
BEGIN_HELP_ARRAY(IDD_ADD_OPERATION)
    IDC_LIST,				IDH_LIST_ADD_OPERATION,
END_HELP_ARRAY

//IDD_ADD_ROLE_DEFINITION
BEGIN_HELP_ARRAY(IDD_ADD_ROLE_DEFINITION)
    IDC_LIST,				IDH_LIST_ADD_ROLE_DEFINITION,
END_HELP_ARRAY

//IDD_ADD_TASK
BEGIN_HELP_ARRAY(IDD_ADD_TASK)
    IDC_LIST,				IDH_LIST_ADD_TASK,
END_HELP_ARRAY

//IDD_ADMIN_MANAGER_ADVANCED_PROPERTY
BEGIN_HELP_ARRAY(IDD_ADMIN_MANAGER_ADVANCED_PROPERTY)
    IDC_EDIT_DOMAIN_TIMEOUT,				IDH_EDIT_DOMAIN_TIMEOUT,
	IDC_EDIT_SCRIPT_ENGINE_TIMEOUT,			IDH_EDIT_SCRIPT_ENGINE_TIMEOUT,
	IDC_EDIT_MAX_SCRIPT_ENGINE,				IDH_EDIT_MAX_SCRIPT_ENGINE,
END_HELP_ARRAY

//IDD_ADMIN_MANAGER_GENERAL_PROPPERTY
BEGIN_HELP_ARRAY(IDD_ADMIN_MANAGER_GENERAL_PROPPERTY)
    IDC_EDIT_NAME,				IDH_EDIT_NAME,
	IDC_EDIT_DESCRIPTION,		IDH_EDIT_DESCRIPTION,
	IDC_EDIT_STORE_TYPE,		IDH_EDIT_STORE_TYPE,
END_HELP_ARRAY


BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY

BEGIN_HELP_ARRAY(IDD_ADD_GROUP)
    IDC_LIST,				IDH_LIST_ADD_GROUP,
END_HELP_ARRAY
*/