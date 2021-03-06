//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       ScopeDelegation.h
//
//  Contents:   Delegation page declarations
//
//  Classes:    
//
//  History:    06-April-2001 JeffJon created
//
//-----------------------------------------------------------------------------

#ifndef __SCOPEDELEGATION_H_
#define __SCOPEDELEGATION_H_

#include "pch.h"
#include "proppage.h"

#include <list>

#ifdef DSADMIN

// Forware Declaration

class CServiceAllowedToDelegate;

typedef std::list<CServiceAllowedToDelegate*> AllowedServicesContainer;

//+----------------------------------------------------------------------------
//
//  Class:      CServiceAllowedToDelegate
//
//  Purpose:    Holds the data for each line in the Service to be delegate list
//
//-----------------------------------------------------------------------------
class CServiceAllowedToDelegate
{
public:
    
   // Constructor

   CServiceAllowedToDelegate() : m_pMasterService(0) {}

   // Destructor

   ~CServiceAllowedToDelegate();

   // Copy and assignment will be allowed

   CServiceAllowedToDelegate(const CServiceAllowedToDelegate& ref);
   
   // operators

   const CServiceAllowedToDelegate& 
   operator=(const CServiceAllowedToDelegate&);

   void 
   Assign(const CServiceAllowedToDelegate& ref);

   bool
   operator==(const CServiceAllowedToDelegate& rhs) const;


   // Initializer

   HRESULT Initialize(PCWSTR pszADSIValue);

   // Accessors

   PCWSTR GetColumn(int column) const;
   PCWSTR GetADSIValue() const { return m_strADSIValue; }

   void SetServiceType(PCWSTR pszServiceType);

   void AddDuplicate(CServiceAllowedToDelegate* pService);

   bool HasDuplicates() const { return !duplicateServices.empty(); }
   void SetDuplicate(CServiceAllowedToDelegate* pMasterService);
   bool IsDuplicate() { return m_pMasterService != 0; }
   CServiceAllowedToDelegate* GetMasterService() { return m_pMasterService; }
   void RemoveDuplicate(CServiceAllowedToDelegate* pService);
   AllowedServicesContainer& GetDuplicates() { return duplicateServices; }

private:

   CStr m_strADSIValue;
   CStr m_strServiceType;
   CStr m_strUserOrComputer;
   CStr m_strPort;
   CStr m_strServiceName;
   CStr m_strRealm;

   CServiceAllowedToDelegate* m_pMasterService;
   AllowedServicesContainer duplicateServices;
};

typedef std::list<CStr*> CStrList;

//+----------------------------------------------------------------------------
//
//  Class:      CFreebieService
//
//  Purpose:    Contains a mapping from aliased SPNs to the service name
//              which they are aliasing
//
//-----------------------------------------------------------------------------
class CFreebieService
{
public:

   // Constructor

   CFreebieService(PCWSTR pszAlias) : m_strAlias(pszAlias) {}

   // Desctructor

   ~CFreebieService() 
   { 
      for (CStrList::iterator itr = m_FreebiesList.begin();
           itr != m_FreebiesList.end();
           ++itr)
      {
         delete *itr;
      }
   }


   const CStrList&
   GetFreebies() { return m_FreebiesList; }

   void
   AddFreebie(PCWSTR pszFreebie) 
   { 
      CStr* pcstrAlias = new CStr(pszFreebie);
      if (pcstrAlias)
      {
         m_FreebiesList.push_back(pcstrAlias);
      }
   }

   PCWSTR
   GetAlias() { return m_strAlias; }

private:

   CStr m_strAlias;
   CStrList m_FreebiesList;
};

typedef std::list<CFreebieService*> FreebiesContainer;


class CSPNListView
{
public:

   // Constructor
   
   CSPNListView() : m_hWnd(0) {}

   // Desctructor

   ~CSPNListView();

   HRESULT 
   Initialize(HWND hwnd, bool bShowDuplicateEntries);

   const AllowedServicesContainer&
   GetSelectedServices() { return m_SelectedServices; }

   const AllowedServicesContainer&
   GetUnSelectedServices() { return m_UnSelectedServices; }

   const AllowedServicesContainer&
   GetAllServices() { return m_AllServices; }

   HWND
   GetHwnd() const { return m_hWnd; }

   UINT
   GetSelectedCount() const { return ListView_GetSelectedCount(m_hWnd); }

   void
   ClearSelectedServices();

   void
   ClearAll();

   void
   SelectAll();

   CServiceAllowedToDelegate* 
   FindService(CServiceAllowedToDelegate* pService, bool& isExactDuplicate);

   int
   AddServiceToUI(CServiceAllowedToDelegate* pService, bool selected = false);

   int 
   AddService(CServiceAllowedToDelegate* pService, bool selected = false);

   void
   AddServices(
      const AllowedServicesContainer& servicesToAdd, 
      bool selected = false,
      bool toUIOnly = false);

   void
   RemoveSelectedServices();

   void
   SetContainersFromSelection();

   void
   SetShowDuplicateEntries(bool bShowDuplicates);

private:

   void
   ClearUnSelectedServices();

   HWND m_hWnd;

   // If this is true all SPNs will be shown even if they are
   // a duplicate in another form. If it is false, the duplicates
   // will be added to a list of duplicates for the entry that is 
   // shown.

   bool m_bShowDuplicateEntries;

  // The dialog caller must clean up this container
  // and anything it contains

  AllowedServicesContainer m_SelectedServices;

  // The contents of these containers will be maintained
  // by this class

  AllowedServicesContainer m_UnSelectedServices;
  AllowedServicesContainer m_AllServices;
};

typedef enum
{
   SCOPE_DELEGATION_COMPUTER,
   SCOPE_DELEGATION_USER
} SCOPE_DELEGATION_TYPE;

HRESULT CreateUserDelegationPage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND,
                                 DWORD, const CDSSmartBasePathsInfo& basePathsInfo,
                                 HPROPSHEETPAGE *);

HRESULT CreateComputerDelegationPage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND,
                                     DWORD, const CDSSmartBasePathsInfo& basePathsInfo,
                                     HPROPSHEETPAGE *);

//+----------------------------------------------------------------------------
//
//  Class:      CDsScopeDelegationPage
//
//  Purpose:    property page object class for the computer and user delegaion page.
//
//-----------------------------------------------------------------------------
class CDsScopeDelegationPage : public CDsPropPageBase
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsScopeDelegationPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                           DWORD dwFlags, SCOPE_DELEGATION_TYPE scopeDelegationType);
    ~CDsScopeDelegationPage(void);

    //
    //  Instance specific wind proc
    //
    virtual INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void Init(PWSTR pwzADsPath, 
              PWSTR pwzClass, 
              const CDSSmartBasePathsInfo& basePathsInfo,
              bool hasSPNs = true,
              bool isTrusted = true);

private:
    HRESULT OnInitDialog(LPARAM lParam);
    LRESULT OnApply(void);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	 LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(void);

    void OnUpdateRadioSelection();
    void OnAddServices();
    void OnRemoveServices();
    void OnExpandCheck();
    void GetFreebiesList();
    void InitializeListColumns();
    void SetPageTextForType();
    void LoadDataFromObject();
    void SetUIFromData();
    void ResetUIFromData();
    void AddServicesToListViewFromData();

    HWND m_hList;
    SCOPE_DELEGATION_TYPE m_scopeDelegationType;

    BOOL m_fUACWritable;
    BOOL m_fA2D2Writable;

    DWORD m_oldUserAccountControl;
    DWORD m_newUserAccountControl;
    bool  m_bA2D2Dirty;

    bool m_bContainsSPNs;
    bool m_bIsTrustedForDelegation;

    CSPNListView m_ServicesList;
    FreebiesContainer m_FreebiesList;
};


//+----------------------------------------------------------------------------
//
//  Class:      CSelectServicesDialog
//
//  Purpose:    Dialog box that allows the admin to select services from
//              users or computers
//
//-----------------------------------------------------------------------------
class CSelectServicesDialog : public ICustomizeDsBrowser
{
public:
  CSelectServicesDialog(PCWSTR pszDC, HWND hParent, const FreebiesContainer& freebies);

  ~CSelectServicesDialog() {}

   //
   // IUknown methods
   //
   STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
   STDMETHOD_(ULONG, AddRef)(void);
   STDMETHOD_(ULONG, Release)(void);

   //
   // ICustomizeDsBrowser methods 
   //
   STDMETHOD(Initialize)(THIS_
                       HWND         hwnd,
                       PCDSOP_INIT_INFO pInitInfo,
                       IBindHelper *pBindHelper) { return S_OK; }

   STDMETHOD(GetQueryInfoByScope)(THIS_
             IDsObjectPickerScope *pDsScope,
             PDSQUERYINFO *ppdsqi);

   STDMETHOD(AddObjects)(THIS_
             IDsObjectPickerScope *pDsScope,
             IDataObject **ppdo) { return E_NOTIMPL; }

   STDMETHOD(ApproveObjects)(THIS_
             IDsObjectPickerScope*,
             IDataObject*,
             PBOOL);

   STDMETHOD(PrefixSearch)(THIS_
             IDsObjectPickerScope *pDsScope,
             PCWSTR pwzSearchFor,
             IDataObject **pdo) { return E_NOTIMPL; }

   STDMETHOD_(PSID, LookupDownlevelName)(THIS_
              PCWSTR) { return NULL; }


  static INT_PTR CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  virtual int 
  DoModal();
  
  virtual BOOL 
  OnInitDialog(HWND hDlg);
  
  virtual void 
  OnClose(int result);
  
  virtual void 
  OnOK();
  
  virtual void 
  ListItemClick(LPNMHDR pnmh);
  
  void 
  OnSelectAll();
  
  void 
  OnGetNewProvider();
  
  void 
  ProcessResults(IDataObject* pdoSelections);

  bool 
  AddNewServiceObjectToList(PCWSTR pszSPN);

  const AllowedServicesContainer& 
  GetSelectedServices() { return m_ServicesList.GetSelectedServices(); }

  void
  ClearSelectedServices() { m_ServicesList.ClearSelectedServices(); }

  HWND             m_hWnd;

private:
  HWND             m_hParent;

  CSPNListView m_ServicesList;
  const FreebiesContainer& m_FreebiesList;

  CStr m_strDC;

  // Reference counting
  ULONG m_uRefs;
};


#endif // DSADMIN

#endif // __SCOPEDELEGATION_H_
