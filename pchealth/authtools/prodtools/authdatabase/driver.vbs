Option Explicit

const SKU_SRV = 4
const SKU_ADS = 8
const SKU_DTC = 16
const SKU_ADS64 = 64
const SKU_DTC64 = 128

const SERVER_MDB  = "d:\temp\Server.mdb"
const DESKTOP_MDB = "c:\temp\Desktop.mdb"
const WINME_MDB   = "c:\temp\WinMe.mdb"

Dim clsAuthDatabase
Set clsAuthDatabase = CreateObject("AuthDatabase.Main")

TestImportHHK

Sub TestKeywordifyTitles

    Dim clsTaxonomy

    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsTaxonomy = clsAuthDatabase.Taxonomy

    clsTaxonomy.KeywordifyTitles 1

End Sub

Sub TestImportHHK

    Dim clsImporter
    Dim FSO
    Dim Folder
    Dim File

    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsImporter = clsAuthDatabase.Importer
    Set FSO = CreateObject("Scripting.FileSystemObject")
    Set Folder = FSO.GetFolder("\\srvua\Latest\HelpDirs\SRV\Help\HHK")

    For Each File in Folder.Files
        clsImporter.ImportHHK File.Path, _
            "\\srvua\Latest\HelpDirs\SRV\Help", SKU_SRV, 0, "", 2
    Next

End Sub

Sub TestImportHHC

    Dim clsImporter

    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsImporter = clsAuthDatabase.Importer

    clsImporter.ImportHHC "\\srvua\Latest\HelpDirs\SRV\Help\HHC\windows.hhc", _
        "\\srvua\Latest\HelpDirs\SRV\Help", SKU_SRV, 0, ""

End Sub

Sub TestImportHHT

    Dim clsHHT

    clsAuthDatabase.SetDatabase DESKTOP_MDB
    Set clsHHT = clsAuthDatabase.HHT

    clsHHT.ImportHHT "c:\temp\foo.xml", 1234
 
End Sub

Sub UpdateChqAndHhk

    Dim clsChqsAndHhks

    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsChqsAndHhks = clsAuthDatabase.ChqsAndHhks

    clsChqsAndHhks.UpdateTable 4, "\\pietrino\HSCExpChms\Srv\winnt", _
        "\\pietrino\HlpImages\Srv\winnt"

End Sub

Sub TestChqAndHhk

    Dim clsChqsAndHhks
    Dim dictFilesAdded
    Dim dictFilesRemoved
    Dim dtmT0
    Dim dtmT1
    Dim vnt
    
    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsChqsAndHhks = clsAuthDatabase.ChqsAndHhks

    clsChqsAndHhks.UpdateTable 4, "c:\temp\CHQ\1"
    Sleep 2000
    clsChqsAndHhks.UpdateTable 8, "c:\temp\CHQ\1"
    dtmT0 = Now
    Sleep 5000
    clsChqsAndHhks.UpdateTable 4, "c:\temp\CHQ\2"
    Sleep 2000
    clsChqsAndHhks.UpdateTable 8, "c:\temp\CHQ\2"
    Sleep 5000
    clsChqsAndHhks.UpdateTable 4, "c:\temp\CHQ\3"
    dtmT1 = Now

    Set dictFilesAdded = CreateObject("Scripting.Dictionary")
    Set dictFilesRemoved = CreateObject("Scripting.Dictionary")

    clsChqsAndHhks.GetFileListDelta 4, dtmT0, dtmT1, dictFilesAdded, dictFilesRemoved

    WScript.Echo "Files added for SRV:"
    For Each vnt in dictFilesAdded.Keys
        WScript.Echo vnt
    Next

    WScript.Echo "Files removed for SRV:"
    For Each vnt in dictFilesRemoved.Keys
        WScript.Echo vnt
    Next

    dictFilesAdded.RemoveAll
    dictFilesRemoved.RemoveAll

    clsChqsAndHhks.GetFileListDelta 8, dtmT0, dtmT1, dictFilesAdded, dictFilesRemoved

    WScript.Echo "Files added for ADV:"
    For Each vnt in dictFilesAdded.Keys
        WScript.Echo vnt
    Next

    WScript.Echo "Files removed for ADV:"
    For Each vnt in dictFilesRemoved.Keys
        WScript.Echo vnt
    Next

End Sub

Sub TestDatabaseBackup

    clsAuthDatabase.CopyAndCompactDatabase SERVER_MDB, "c:\temp\ServerBack.mdb"
    clsAuthDatabase.CopyAndCompactDatabase DESKTOP_MDB, "c:\temp\DesktopBack.mdb"

End Sub

Sub TestServerCAB

    Dim clsHHT

    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsHHT = clsAuthDatabase.HHT

    clsHHT.GenerateCAB "c:\temp\SRV.cab",     4
    rem clsHHT.GenerateCAB "c:\temp\ADV.cab",     8
    rem clsHHT.GenerateCAB "c:\temp\DAT.cab",    16
    rem clsHHT.GenerateCAB "c:\temp\ADV64.cab",  64
    rem clsHHT.GenerateCAB "c:\temp\DAT64.cab", 128

End Sub

Sub TestDesktopCAB

    Dim clsHHT

    clsAuthDatabase.SetDatabase DESKTOP_MDB
    Set clsHHT = clsAuthDatabase.HHT

    clsHHT.GenerateCAB "c:\temp\STD.cab",    1
    clsHHT.GenerateCAB "c:\temp\PRO.cab",    2
    clsHHT.GenerateCAB "c:\temp\PRO64.cab", 32

End Sub

Sub TestWinMeCAB

    Dim clsHHT

    clsAuthDatabase.SetDatabase WINME_MDB
    Set clsHHT = clsAuthDatabase.HHT

    clsHHT.GenerateHHT "c:\temp\WINME.cab", 256, "Windows Me Update", False

End Sub

Sub TestSynonymSets()

    Dim clsSynonymSets
    Dim arrKeywords(3)
    Dim v

    clsAuthDatabase.SetDatabase WINME_MDB
    Set clsSynonymSets = clsAuthDatabase.SynonymSets

    arrKeywords(0) = 0
    arrKeywords(1) = 1
    arrKeywords(2) = 2
    arrKeywords(3) = 3
    
    v = arrKeywords

    clsSynonymSets.Create ".aaaaaaaaaahhhhhhh", v, 0, 0, "Test"

End Sub

Sub TestParameters()

    Dim clsParameters
    
    clsAuthDatabase.SetDatabase WINME_MDB
    Set clsParameters = clsAuthDatabase.Parameters

    clsParameters.Value("Foo") = 134
    Wscript.Echo clsParameters.Value("Foo") * 2

End Sub

Sub Sleep(intMilliSeconds)

    Dim WshShell

    Set WshShell = WScript.CreateObject("WScript.Shell")
    WScript.Sleep intMilliSeconds

End Sub
