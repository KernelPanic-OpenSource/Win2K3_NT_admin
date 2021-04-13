/*++
Module Name:

    MenuEnum.h

Abstract:

    Contains command ids for the context menus of the Dfs Admin snap-in
	and toolbar buttons of the Dfs Admin snap-in

--*/



enum STATIC_MENU_COMMANDS
{
	IDM_STATIC_TOP_NEW_DFSROOT = 1,
	IDM_STATIC_TOP_CONNECTTO,

	IDM_STATIC_MIN = IDM_STATIC_TOP_NEW_DFSROOT,
	IDM_STATIC_MAX = IDM_STATIC_TOP_CONNECTTO
};



enum ROOT_MENU_COMMANDS
{
	IDM_ROOT_TOP_NEW_DFS_LINK = 1,
	IDM_ROOT_TOP_NEW_ROOT_REPLICA,
	IDM_ROOT_TOP_CHECK_STATUS,
	IDM_ROOT_TOP_FILTER_DFS_LINKS,
	IDM_ROOT_TOP_DELETE_CONNECTION_TO_DFS_ROOT,
	IDM_ROOT_TOP_DELETE_DFS_ROOT,
	IDM_ROOT_TOP_DELETE_DISPLAYED_DFS_LINKS,
	IDM_ROOT_TOP_REPLICATION_TOPOLOGY,
	IDM_ROOT_TOP_SHOW_REPLICATION,
	IDM_ROOT_TOP_HIDE_REPLICATION,
	IDM_ROOT_TOP_STOP_REPLICATION,

	IDM_ROOT_MIN = IDM_ROOT_TOP_NEW_DFS_LINK,
	IDM_ROOT_MAX = IDM_ROOT_TOP_STOP_REPLICATION
};


enum JUNCTION_MENU_COMMANDS
{
	IDM_JUNCTION_TOP_NEW_DFS_REPLICA = 1,
	IDM_JUNCTION_TOP_CHECK_STATUS,
	IDM_JUNCTION_TOP_REMOVE_FROM_DFS,
	IDM_JUNCTION_TOP_REPLICATION_TOPOLOGY,
	IDM_JUNCTION_TOP_SHOW_REPLICATION,
	IDM_JUNCTION_TOP_HIDE_REPLICATION,
	IDM_JUNCTION_TOP_STOP_REPLICATION,

	IDM_JUNCTION_MIN = IDM_JUNCTION_TOP_NEW_DFS_REPLICA,
	IDM_JUNCTION_MAX = IDM_JUNCTION_TOP_STOP_REPLICATION
};


enum REPLICA_MENU_COMMANDS
{
	IDM_REPLICA_TOP_OPEN=1,
	IDM_REPLICA_TOP_CHECK_STATUS,
	IDM_REPLICA_TOP_TAKE_REPLICA_OFFLINE_ONLINE,
	IDM_REPLICA_TOP_REMOVE_FROM_DFS,
	IDM_REPLICA_TOP_REPLICATE,
	IDM_REPLICA_TOP_STOP_REPLICATION,

	IDM_REPLICA_MIN = IDM_REPLICA_TOP_OPEN,
	IDM_REPLICA_MAX = IDM_REPLICA_TOP_STOP_REPLICATION
};


// Toolbar command for the dfs admin static node
enum ADMIN_TOOLBAR_COMMANDS
{
	IDT_ADMIN_NEW_DFSROOT = 1,
	IDT_ADMIN_CONNECTTO,

	IDT_ADMIN_MIN = IDT_ADMIN_NEW_DFSROOT,
	IDT_ADMIN_MAX = IDT_ADMIN_CONNECTTO
};



// Toolbar command for the dfs admin root node
enum ROOT_TOOLBAR_COMMANDS
{
	IDT_ROOT_NEW_DFS_LINK = 1,
	IDT_ROOT_NEW_ROOT_REPLICA,
	IDT_ROOT_CHECK_STATUS,
	IDT_ROOT_FILTER_DFS_LINKS,
	IDT_ROOT_DELETE_CONNECTION_TO_DFS_ROOT,
	IDT_ROOT_DELETE_DFS_ROOT,
	IDT_ROOT_DELETE_DISPLAYED_DFS_LINKS,
	IDT_ROOT_REPLICATION_TOPOLOGY,
	IDT_ROOT_SHOW_REPLICATION,
	IDT_ROOT_HIDE_REPLICATION,
	IDT_ROOT_STOP_REPLICATION,

	IDT_ROOT_MIN = IDT_ROOT_NEW_DFS_LINK,
	IDT_ROOT_MAX = IDT_ROOT_STOP_REPLICATION
};


// Toolbar command for the dfs admin junction point(jp) node
enum JP_TOOLBAR_COMMANDS
{
	IDT_JP_NEW_DFS_REPLICA = 1,
	IDT_JP_CHECK_STATUS,
	IDT_JP_REMOVE_FROM_DFS,
	IDT_JP_REPLICATION_TOPOLOGY,
	IDT_JP_SHOW_REPLICATION,
	IDT_JP_HIDE_REPLICATION,
	IDT_JP_STOP_REPLICATION,

	IDT_JP_MIN = IDT_JP_NEW_DFS_REPLICA,
	IDT_JP_MAX = IDT_JP_STOP_REPLICATION
};



// Toolbar command for the dfs admin replica node
enum REPLICA_TOOLBAR_COMMANDS
{
	IDT_REPLICA_OPEN=1,
	IDT_REPLICA_CHECK_STATUS,
	IDT_REPLICA_TAKE_REPLICA_OFFLINE_ONLINE,
	IDT_REPLICA_REMOVE_FROM_DFS,
	IDT_REPLICA_REPLICATE,
	IDT_REPLICA_STOP_REPLICATION,

	IDT_REPLICA_MIN = IDT_REPLICA_OPEN,
	IDT_REPLICA_MAX = IDT_REPLICA_STOP_REPLICATION
};
