//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_MODULE_INFO
#define SIMC_MODULE_INFO

/*
 * This file contains the SIMCModuleInfoScanner and SIMCModuleInfoParser 
 * classes, which are light-weight scanners and parsers of a module, 
 * as opposed to the heavy-weights SIMCScanner and SIMCParser.
 * The difference is that the latter are compler scanners and parsers
 * of SNMP modules, while the former just parse a module enough to
 * get the module name, and the information in the IMPORT section
 */

typedef CList<CString, CString&> SIMCStringList;

// The light-weight scanner. This is derived from the ModuleInfo_scan 
// that is generated by the MKS LEX utility, from the information in 
// the file infoLex.l 
class SIMCModuleInfoScanner : public ModuleInfo_scan
{
	public:
		virtual void ModuleInfoerror(char *,...)	
		{}
		virtual void output(int x)
		{}
};

// The light-weight parser. This is derived from the ModuleInfo_parse 
// that is generated by the MKS YACC utility, from the information in 
// the file infoYacc.y. This parser uses the above scanner 

class SIMCModuleInfoParser  : public ModuleInfo_parse
{
		CString moduleName;
		SIMCStringList 	importModules;

	public:

		// Parse the module. Once this function is called, the other
		// functions make sense.
		BOOL GetModuleInfo(SIMCModuleInfoScanner *infoScanner);

		const SIMCStringList * GetImportModuleList() const
		{
			return &importModules;
		}
		void AddImportModule(CString name)
		{
			importModules.AddTail(name);
		}
		CString GetModuleName() const
		{
			return moduleName;
		}
		void SetModuleName(const CString& name) 
		{
			moduleName = name;
		}

};


#endif
