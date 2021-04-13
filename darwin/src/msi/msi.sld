<?xml version="1.0" encoding="UTF-16"?>
<!DOCTYPE DCARRIER SYSTEM "mantis.dtd">
<DCARRIER CarrierRevision="1">
	<TOOLINFO ToolName="iCat"><![CDATA[<?xml version="1.0"?>
<!DOCTYPE TOOL SYSTEM "tool.dtd">
<TOOL>
	<CREATED><NAME>iCat</NAME><VSGUID>{e61c9051-ac57-48e4-90bc-27b21a575689}</VSGUID><VERSION>1.0.0.424</VERSION><BUILD>424</BUILD><DATE>11/13/2000</DATE></CREATED><LASTSAVED><NAME>iCat</NAME><VSGUID>{97b86ee0-259c-479f-bc46-6cea7ef4be4d}</VSGUID><VERSION>1.0.0.452</VERSION><BUILD>452</BUILD><DATE>7/16/2001</DATE></LASTSAVED></TOOL>
]]></TOOLINFO><COMPONENT Revision="5" Visibility="1000" MultiInstance="0" Released="1" Editable="1" HTMLFinal="0" ComponentVSGUID="{38F0FDC2-3C7D-4AF4-AA6F-282A4031E99F}" ComponentVIGUID="{94835EE0-F09D-44BA-B3B7-E2BC6D21D203}" PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}" RepositoryVSGUID="{8E0BE9ED-7649-47F3-810B-232D36C430B4}"><DISPLAYNAME>Windows installer</DISPLAYNAME><VERSION>5.0</VERSION><DESCRIPTION>Windows installer component</DESCRIPTION><COPYRIGHT>2000 Microsoft Corp.</COPYRIGHT><VENDOR>Microsoft Corp.</VENDOR><OWNERS>dkays</OWNERS><AUTHORS>dkays</AUTHORS><DATECREATED>11/13/2000</DATECREATED><DATEREVISED>7/16/2001</DATEREVISED><RESOURCE ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}" BuildTypeMask="819" Name="File(819):&quot;%11%&quot;,&quot;msi.dll&quot;"><PROPERTY Name="DstPath" Format="String">%11%</PROPERTY><PROPERTY Name="DstName" Format="String">msi.dll</PROPERTY><PROPERTY Name="NoExpand" Format="Boolean">0</PROPERTY><DISPLAYNAME>msi.dll</DISPLAYNAME><DESCRIPTION>msi core logic</DESCRIPTION></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;ntdll.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">ntdll.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;KERNEL32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">KERNEL32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;ADVAPI32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">ADVAPI32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;USER32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">USER32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;GDI32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">GDI32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;RPCRT4.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">RPCRT4.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}" BuildTypeMask="819" Name="File(819):&quot;%11%&quot;,&quot;msiexec.exe&quot;"><PROPERTY Name="DstPath" Format="String">%11%</PROPERTY><PROPERTY Name="DstName" Format="String">msiexec.exe</PROPERTY><PROPERTY Name="NoExpand" Format="Boolean">0</PROPERTY><DISPLAYNAME>msiexec.exe</DISPLAYNAME><DESCRIPTION>msi service and custom action server</DESCRIPTION></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;MSVCRT.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">MSVCRT.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;ole32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">ole32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;msi.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">msi.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}" BuildTypeMask="819" Name="File(819):&quot;%11%&quot;,&quot;msihnd.dll&quot;"><PROPERTY Name="DstPath" Format="String">%11%</PROPERTY><PROPERTY Name="DstName" Format="String">msihnd.dll</PROPERTY><PROPERTY Name="NoExpand" Format="Boolean">0</PROPERTY><DISPLAYNAME>msihnd.dll</DISPLAYNAME><DESCRIPTION>msi ui handler</DESCRIPTION></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;COMCTL32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">COMCTL32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;IMM32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">IMM32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;SHELL32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">SHELL32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}" BuildTypeMask="819" Name="File(819):&quot;%11%&quot;,&quot;msimsg.dll&quot;"><PROPERTY Name="DstPath" Format="String">%11%</PROPERTY><PROPERTY Name="DstName" Format="String">msimsg.dll</PROPERTY><PROPERTY Name="NoExpand" Format="Boolean">0</PROPERTY><DISPLAYNAME>msimsg.dll</DISPLAYNAME><DESCRIPTION>msi message dll</DESCRIPTION></RESOURCE><RESOURCE ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}" BuildTypeMask="819" Name="File(819):&quot;%11%&quot;,&quot;msisip.dll&quot;"><PROPERTY Name="DstPath" Format="String">%11%</PROPERTY><PROPERTY Name="DstName" Format="String">msisip.dll</PROPERTY><PROPERTY Name="NoExpand" Format="Boolean">0</PROPERTY><DISPLAYNAME>msisip.dll</DISPLAYNAME><DESCRIPTION>msi digital signature SIP handler</DESCRIPTION></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;sfc.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">sfc.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;oleaut32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">oleaut32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;mpr.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">mpr.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;urlmon.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">urlmon.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;cscdll.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">cscdll.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;shfolder.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">shfolder.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;wintrust.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">wintrust.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;sxs.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">sxs.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;crypt32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">crypt32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;userenv.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">userenv.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;wininet.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">wininet.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;mspatcha.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">mspatcha.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;ODBCCP32.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">ODBCCP32.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{90D8E195-E710-4AF6-B667-B1805FFC9B8F}" BuildTypeMask="819" Name="RawDep(819):&quot;File&quot;,&quot;version.dll&quot;"><PROPERTY Name="RawType" Format="String">File</PROPERTY><PROPERTY Name="Value" Format="String">version.dll</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package&quot;,&quot;&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">Windows Installer Package</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package&quot;,&quot;EditFlags&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package</PROPERTY><PROPERTY Name="ValueName" Format="String">EditFlags</PROPERTY><PROPERTY Name="RegValue" Format="Binary">00001000</PROPERTY><PROPERTY Name="RegType" Format="Integer">3</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\DefaultIcon&quot;,&quot;&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\DefaultIcon</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">%SystemRoot%\system32\msiexec.exe,0</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell&quot;,&quot;&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">Open,Repair,Uninstall</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Open&quot;,&quot;&quot;" Localize="1"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Open</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">&amp;Install</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Open\command&quot;,&quot;&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Open\command</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">"%SystemRoot%\System32\msiexec.exe" /i "%1" %*</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Repair&quot;,&quot;&quot;" Localize="1"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Repair</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">Re&amp;pair</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Repair\command&quot;,&quot;&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Repair\command</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">"%SystemRoot%\System32\msiexec.exe" /f "%1" %*</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Uninstall&quot;,&quot;&quot;" Localize="1"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Uninstall</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">&amp;Uninstall</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Uninstall\command&quot;,&quot;&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Uninstall\command</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">"%SystemRoot%\System32\msiexec.exe" /x "%1" %*</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch&quot;,&quot;&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">Windows Installer Patch</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch&quot;,&quot;EditFlags&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch</PROPERTY><PROPERTY Name="ValueName" Format="String">EditFlags</PROPERTY><PROPERTY Name="RegValue" Format="Binary">00001000</PROPERTY><PROPERTY Name="RegType" Format="Integer">3</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\DefaultIcon&quot;,&quot;&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\DefaultIcon</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">%SystemRoot%\system32\msiexec.exe,0</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\shell&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\shell</PROPERTY><PROPERTY Name="RegValue" Format="Binary"/><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\shell\Open&quot;,&quot;&quot;" Localize="1"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\shell\Open</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">&amp;Apply Patch</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\shell\Open\command&quot;,&quot;&quot;"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\shell\Open\command</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">"%SystemRoot%\System32\msiexec.exe" /p "%1" %*</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\MSIServer&quot;,&quot;Description&quot;" Localize="1"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\MSIServer</PROPERTY><PROPERTY Name="ValueName" Format="String">Description</PROPERTY><PROPERTY Name="RegValue" Format="String">Installs, repairs and removes software according to instructions contained in .MSI files.</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{322D2CA9-219E-4380-989B-12E8A830DFFA}" BuildTypeMask="819" Name="FBRegDLL(819):&quot;%11%\msisip.dll&quot;" Localize="0"><PROPERTY Name="Arguments" Format="String"></PROPERTY><PROPERTY Name="DLLEntryPoint" Format="String"></PROPERTY><PROPERTY Name="DLLInstall" Format="Boolean">False</PROPERTY><PROPERTY Name="DLLRegister" Format="Boolean">True</PROPERTY><PROPERTY Name="FilePath" Format="String">%11%\msisip.dll</PROPERTY><PROPERTY Name="Flags" Format="Integer">0</PROPERTY><PROPERTY Name="Timeout" Format="Integer">0</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{322D2CA9-219E-4380-989B-12E8A830DFFA}" BuildTypeMask="819" Name="FBRegDLL(819):&quot;%11%\msiexec.exe&quot;" Localize="0"><PROPERTY Name="Arguments" Format="String">/regserver</PROPERTY><PROPERTY Name="DLLEntryPoint" Format="String"></PROPERTY><PROPERTY Name="DLLInstall" Format="Boolean">False</PROPERTY><PROPERTY Name="DLLRegister" Format="Boolean">False</PROPERTY><PROPERTY Name="FilePath" Format="String">%11%\msiexec.exe</PROPERTY><PROPERTY Name="Flags" Format="Integer">0</PROPERTY><PROPERTY Name="Timeout" Format="Integer">0</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\Software\Classes\.msi&quot;,&quot;&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\Software\Classes\.msi</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">Msi.Package</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\Software\Classes\.msp&quot;,&quot;&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\Software\Classes\.msp</PROPERTY><PROPERTY Name="ValueName" Format="String"></PROPERTY><PROPERTY Name="RegValue" Format="String">Msi.Patch</PROPERTY><PROPERTY Name="RegType" Format="Integer">1</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package&quot;,&quot;FriendlyTypeName&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package</PROPERTY><PROPERTY Name="ValueName" Format="String">FriendlyTypeName</PROPERTY><PROPERTY Name="RegValue" Format="String">@%SystemRoot%\System32\msi.dll,-34</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Open&quot;,&quot;MUIVerb&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Open</PROPERTY><PROPERTY Name="ValueName" Format="String">MUIVerb</PROPERTY><PROPERTY Name="RegValue" Format="String">@%SystemRoot%\System32\msi.dll,-36</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Repair&quot;,&quot;MUIVerb&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Repair</PROPERTY><PROPERTY Name="ValueName" Format="String">MUIVerb</PROPERTY><PROPERTY Name="RegValue" Format="String">@%SystemRoot%\System32\msi.dll,-37</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Uninstall&quot;,&quot;MUIVerb&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Package\shell\Uninstall</PROPERTY><PROPERTY Name="ValueName" Format="String">MUIVerb</PROPERTY><PROPERTY Name="RegValue" Format="String">@%SystemRoot%\System32\msi.dll,-38</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch&quot;,&quot;FriendlyTypeName&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch</PROPERTY><PROPERTY Name="ValueName" Format="String">FriendlyTypeName</PROPERTY><PROPERTY Name="RegValue" Format="String">@%SystemRoot%\System32\msi.dll,-35</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><RESOURCE ResTypeVSGUID="{2C10DB69-39AB-48A4-A83F-9AB3ACBA7C45}" BuildTypeMask="819" Name="RegKey(819):&quot;HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\shell\Open&quot;,&quot;MUIVerb&quot;" Localize="0"><PROPERTY Name="KeyPath" Format="String">HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Msi.Patch\shell\Open</PROPERTY><PROPERTY Name="ValueName" Format="String">MUIVerb</PROPERTY><PROPERTY Name="RegValue" Format="String">@%SystemRoot%\System32\msi.dll,-39</PROPERTY><PROPERTY Name="RegType" Format="Integer">2</PROPERTY><PROPERTY Name="RegOp" Format="Integer">1</PROPERTY><PROPERTY Name="RegCond" Format="Integer">1</PROPERTY></RESOURCE><GROUPMEMBER GroupVSGUID="{E01B4103-3883-4FE8-992F-10566E7B796C}"/><GROUPMEMBER GroupVSGUID="{64668FB9-9289-45F0-BEF9-23745D272E3D}"/><DEPENDENCY Class="Include" Type="All" DependOnGUID="{5B3A5EB5-C23D-4B64-8C55-AC779B4A1A56}"/><DEPENDENCY Class="Include" Type="All" DependOnGUID="{D4E8DC20-E3F1-478B-B6C8-3A5BA3779A4B}"/><DEPENDENCY Class="Include" Type="All" DependOnGUID="{061AB9C1-9BBD-425E-89BC-D55FF69BF330}"/><DEPENDENCY Class="Include" Type="All" DependOnGUID="{F691200A-F5EA-49CE-8760-434723AF33D9}"/><DEPENDENCY Class="Include" Type="All" DependOnGUID="{89155B6B-7692-424B-978B-37296B32E091}"/><DEPENDENCY Class="Include" Type="All" DependOnGUID="{6A6A12B5-17B6-4066-8C68-138415AF0BA6}"/></COMPONENT></DCARRIER>
