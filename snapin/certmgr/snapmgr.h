//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2002.
//
//  File:       SnapMgr.h
//
//  Contents:   header file for Snapin Manager property page
//
//----------------------------------------------------------------------------

#ifndef __SNAPMGR_H__
#define __SNAPMGR_H__

#endif // ~__SNAPMGR_H__
/////////////////////////////////////////////////////////////////////////////
// CCertMgrChooseMachinePropPage dialog

class CCertMgrChooseMachinePropPage : public CChooseMachinePropPage
{
// Construction
public:
	CCertMgrChooseMachinePropPage();
	virtual ~CCertMgrChooseMachinePropPage();
	void AssignLocationPtr (DWORD* pdwLocation);

// Dialog Data
	//{{AFX_DATA(CCertMgrChooseMachinePropPage)
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCertMgrChooseMachinePropPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCertMgrChooseMachinePropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	virtual LRESULT OnWizardNext();
    virtual BOOL OnKillActive();

private:
	DWORD* m_pdwLocation;
};
/////////////////////////////////////////////////////////////////////////////
// CCertMgrChooseMachinePropPage property page

CCertMgrChooseMachinePropPage::CCertMgrChooseMachinePropPage() : 
	CChooseMachinePropPage(),
	m_pdwLocation (0)
{
	//{{AFX_DATA_INIT(CCertMgrChooseMachinePropPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CCertMgrChooseMachinePropPage::~CCertMgrChooseMachinePropPage()
{
}

void CCertMgrChooseMachinePropPage::AssignLocationPtr(DWORD * pdwLocation)
{
	m_pdwLocation = pdwLocation;
}

void CCertMgrChooseMachinePropPage::DoDataExchange(CDataExchange* pDX)
{
	CChooseMachinePropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCertMgrChooseMachinePropPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCertMgrChooseMachinePropPage, CChooseMachinePropPage)
	//{{AFX_MSG_MAP(CCertMgrChooseMachinePropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCertMgrChooseMachinePropPage message handlers
BOOL CCertMgrChooseMachinePropPage::OnSetActive() 
{
	if ( m_pdwLocation && CERT_SYSTEM_STORE_SERVICES == *m_pdwLocation )
		GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_NEXT);
	else
		GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_FINISH);
	
	// Do not call CChooseMachinePropPage here because it will disable the back button. 
	// Call the grandparent instead.
	return CPropertyPage::OnSetActive();
}

BOOL CCertMgrChooseMachinePropPage::OnKillActive()
{
	ASSERT_VALID(this);

    // override CPropertyPage::OnKillActive () so that UpdateData is not called
    return TRUE;
}

LRESULT CCertMgrChooseMachinePropPage::OnWizardNext()
{
	if ( !UpdateData (TRUE) )
        return -1;

	if ( m_pstrMachineNameOut )
    {
        // Store the machine name into its output buffer
        // NTRAID# 487794 Certificates/Service account: cannot add snapin 
        // targeted to local computer if illegal chars present in "Another 
        // computer" textbox
        if ( m_fIsRadioLocalMachine )
		    *m_pstrMachineNameOut = L"";
        else
            *m_pstrMachineNameOut = m_strMachineName;

        if (m_pfAllowOverrideMachineNameOut != NULL)
			*m_pfAllowOverrideMachineNameOut = m_fAllowOverrideMachineName;
    }

	return CChooseMachinePropPage::OnWizardNext ();
}

