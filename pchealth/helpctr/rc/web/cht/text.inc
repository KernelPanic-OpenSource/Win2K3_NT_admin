<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "�Ұʻ��ݨ�U"
	Const L_ProductName_Text			= "���ݨ�U"
	Const L_Title1Paragraph1_Text		= "�ϥλ��ݨ�U�������U"
	Const L_Paragraph1_Text				= "���ɭ����U�O�H�ѨM���D���̦n��k�N�O�ܽd�ѨM���D����k�C���ݨ�U�O�w�����s�u�A�i�H���z�ΨӦV�B�ͳv�B�ܽd�ѨM�q�����D����k�C"
	Const L_Paragraph2_Text				= "�p�G�o�O�z�Ĥ@���ϥΡA�Х��\Ū�H�U���`�N�ƶ��C"
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">�n�s�u��z�B�ͪ��q���A�Ы�</font><b><font color=""#000000"" size=""2""><b> �Ұʻ��ݨ�U�C</b></font></b>"
	Const L_StartRAButton_Text			= "�Ұʻ��ݨ�U"
	Const L_JScriptNotAvail1_Text		= "�z�ثe�� Internet Explorer �w���ʳ]�w�ȳ]�o�Ӱ��F�A�ҥH�L�k�ϥλ��ݨ�U�C�n�ϥλ��ݨ�U�A���ܧ�]�w�ȩαN�o�Ӻ����]�����H���������C"
	Const L_IEHelp_Text					= "�����ܧ�w���ʳ]�w�Ȫ��ԲӸ�T�A�аѾ\ Internet Explorer �����C"
	Const L_TicketError1_Text			= "�z�����ݨ�U�ܽФw�g�򥢩εL�ġC�z�������@�Ӧ��Ī��ܽФ~��ϥλ��ݨ�U�C"
	Const L_InstallIE_Text				= "�z�����w�� Internet Explorer 5.5 �~�౵�����ݨ�U�ܽСC"
	Const L_TicketErrorMsg_Text			= "�n�A�ѻ��ݨ�U���ԲӸ�T�A�Ы� Windows XP �� <B>[�}�l]</B>�A�� <B>[�����Τ䴩]</B>�A�M��b <B>[�j�M]</B> �������J <B>[���ݨ�U]</B>�C"
	
	Const L_NotesTitle_Text				= "�`�N�ƶ�"
	Const L_NotesLine1_Text				= "�Хu�����z�ҫH�����H�ҵo�X���ܽСC"
	
	Const L_NotesLine2_Text				= "�ϥλ��ݨ�U�ɡA�z�M�����z���U���H�A�������s�u����ں����C�p�G�o�X�ܽЪ̥i�H�����z���s�u�ܨD�A�z�N�i�H�P�L/�o�b�u�W�Y�ɲ�Ѩ��˵��L/�o���q���ù��C"
	
	Const L_NotesLine6_Text				= "�o�O�w������V�s�u�C�s�u��z���q�����H�L�צ�ɳ��L�k�H�L�Φo���q���Ӧs���z���q���C�p�G�z�b�ϰ�����W�A������i��|����z�ϥλ��ݨ�U�C�p�G�O�o�˪��ܡA�лP�z�������t�κ޲z���s���A���o�ԲӸ�T�C"
	
	Const L_LearnXP_Text				= "�n�A�ѻ��ݨ�U���ԲӸ�T�A�Ы� Windows XP �� <B>[�}�l]</B>�A�� <B>[�����Τ䴩]</B>�A�M��b <B>[�j�M]</B> �������J <B>[���ݨ�U]</B>�C"
	
	Const L_ContactUsMsg_Text			= "�s���覡"
	

	Const L_XPNewsletterMsg_Text		= "�K�O�q�\�q�l��"
	Const L_EmailPage_Text				= "�H�q�l�l��ǰe�o�ӵe��"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. All rights reserved. Terms of Use."
	
	Const L_PrivacyMsg_Text				= "Privacy Statement"

	Const L_AccMsg_Text					= "��U�u��"

	Const L_NotIEError1_Text			= "�z�����w�� Internet Explorer 5.5 �Χ�s�������~�౵�����ݨ�U�ܽСC"

	Const L_NotXPError1_Text			= "Microsoft ���ݨ�U�O Windows&nbsp;XP �䤤���@���\��C�z�������w�� Windows&nbsp;XP �~��ϥλ��ݨ�U�C"

	Const L_IELinkMsg_Text				= "�U���̷s���� Internet Explorer"
	Const L_XPLinkMsg_Text				= "�p�G�z�Q�A������ Windows XP ���ԲӸ�T�A���s��"
	Const L_XPLinkMsg2_Text				= "Microsoft Windows ����"
	Const L_XPLinkMsg3_Text				= "�C"
	
	Const L_IntroMsg1_Text              = "�U������ݭn�ϥλ��ݨ�U�C�p�G�z�� 28.8 kbps �ƾھ��A�U���u�n�@�����ɶ��C"
    Const L_IntroMsg2_Text              = "�����ҥ� Active Scripting �~��ϥλ��ݨ�U�C�p�G�W�L�w�p���U���ɶ��A�����ٵL�k�T�{�U���������ܡA���ˬd���O�X�O�_�w�g�ҥΡC"
    Const L_IntroMsg3_Text              = "�n�ҥ� Active Scripting ����"
    Const L_IntroMsg4_Text              = "�b Internet Explorer ��<B>�u��</B>�\����W�A��<B>���ں����ﶵ</B>�C"
    Const L_IntroMsg5_Text              = "��<B>�w����</B>���ҡC"
    Const L_IntroMsg6_Text              = "��<STRONG>���ں���</STRONG>�ϥܡA�M���<STRONG>�ۭq�h��</STRONG>�C"
    Const L_IntroMsg7_Text              = "�b���O�X�q���A<STRONG>Active Scripting</STRONG> ���U�A��<STRONG>�ҥ�</STRONG>�C"
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=A link from Microsoft&body=���s���Ӧ� Microsoft Windows XP ����"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>