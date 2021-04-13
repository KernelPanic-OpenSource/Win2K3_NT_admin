<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "���� ���� ����"
	Const L_ProductName_Text			= "���� ����"
	Const L_Title1Paragraph1_Text		= "���� ������ ����� ���� ����"
	Const L_Paragraph1_Text				= "���δ� ���� �ذ� ����� ���濡�� �����ִ� ���� ���� ȿ������ ����Դϴ�. ���� ������ ����ڰ� ���濡�� ��ǻ�� ���� �ذ� ����� ������ �� �ְ� �ϴ� ���ȵ� �����Դϴ�."
	Const L_Paragraph2_Text				= "ó������ ����� ��� �Ʒ� ���� �׸��� �о�ʽÿ�."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">���� ��ǻ�Ϳ� �����Ϸ���</font><b><font color=""#000000"" size=""2""><b> ���� ���� ����</b>�� Ŭ���Ͻʽÿ�.</font></b>"
	Const L_StartRAButton_Text			= "���� ���� ����"
	Const L_JScriptNotAvail1_Text		= "Internet Explorer ���� ������ �ʹ� ���� �����߱� ������ ���� ������ ����� �� �����ϴ�. ���� ������ ����Ϸ��� ���� ������  �����ϰų� �� ����Ʈ�� �ŷ��� �� �ִ� ����Ʈ�� �߰��Ͻʽÿ�."
	Const L_IEHelp_Text					= "���� ���� ���濡 ���� ������ Internet Explorer ������ �����Ͻʽÿ�."
	Const L_TicketError1_Text			= "���� ���� �ʴ밡 ���ų� �ùٸ��� �ʽ��ϴ�. ���� ������ ����Ϸ��� �ùٸ� �ʴ밡 �ʿ��մϴ�."
	Const L_InstallIE_Text				= "���� ���� �ʴ븦 �޾Ƶ��̷��� Internet Explorer 5.5�� ��ġ�ؾ� �մϴ�."
	Const L_TicketErrorMsg_Text			= "���� ������ ���� �� �ڼ��� �˾ƺ����� Windows XP�� <B>����</B>�� Ŭ���� ���� <B>���� �� ����</B>�� Ŭ���ϰ� <B>�˻�</B> ���ڿ� <B>���� ����</B>�� �Է��Ͻʽÿ�."
	
	Const L_NotesTitle_Text				= "����"
	Const L_NotesLine1_Text				= "�ŷ��ϴ� ������κ��͸� ���� ��û�� �޾Ƶ��̽ʽÿ�."
	
	Const L_NotesLine2_Text				= "���� ������ ����ϴ� ���� ����ڿ� ������ ���ͳݿ� ����Ǿ� �־�� �մϴ�. �� �ʴ븦 ���� ������ ��ǻ�Ϳ� ������ �� �ֵ��� ����ϸ� ������ ��ǻ�� ȭ���� �� �� ������ �ǽð� ä�õ� �� �� �ֽ��ϴ�."
	
	Const L_NotesLine6_Text				= "�� ������ ���ȵ� �ܹ��� �����Դϴ�. ������ ��û�� ����ڴ� ��� ������ ����ڰ� ������ ��ǻ�ͷ� ������� ��ǻ�͸� �׼����� �� �����ϴ�. ���� ���� ��Ʈ��ũ���� �۾��� �ϴ� ��� ��ȭ�� ������ ���� ������ ������� ���� ���� �ֽ��ϴ�. �̷� ��� ��Ʈ��ũ �����ڿ��� �����Ͽ� �ʿ��� ������ �����ʽÿ�."
	
	Const L_LearnXP_Text				= "���� ������ ���� �� �ڼ��� �˾ƺ����� Windows XP�� <B>����</B>�� Ŭ���� ���� <B>���� �� ����</B>�� Ŭ���ϰ� <B>�˻�</B> ���ڿ� <B>���� ����</B>�� �Է��Ͻʽÿ�."
	
	Const L_ContactUsMsg_Text			= "����ó"
	

	Const L_XPNewsletterMsg_Text		= "�·� ��������"
	Const L_EmailPage_Text				= "���� ���Ϸ� �� ������ ������"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. All rights reserved. ���ǿ� ���� ���� ����."
	
	Const L_PrivacyMsg_Text				= "���� ���� ��ȣ ��å"

	Const L_AccMsg_Text					= "���� �ʿ��� �ɼ�"

	Const L_NotIEError1_Text			= "���� ���� �ʴ븦 �޾Ƶ��̷��� Internet Explorer 5.5 �̻��� ��ġ�ؾ� �մϴ�."

	Const L_NotXPError1_Text			= "Microsoft ���� ������ Windows&nbsp;XP�� ����Դϴ�. ���� ������ ����Ϸ��� Windows&nbsp;XP�� ���� ��ġ�ؾ� �մϴ�."

	Const L_IELinkMsg_Text				= "�ֽ� ������ Internet Explorer �ٿ�ε�"
	Const L_XPLinkMsg_Text				= "Windows XP�� ���� �� �ڼ��� �˾ƺ����� "
	Const L_XPLinkMsg2_Text				= "Microsoft Windows �� ����Ʈ"
	Const L_XPLinkMsg3_Text				= "�� �湮�Ͻʽÿ�."
	
	Const L_IntroMsg1_Text              = "���� ������ ����Ϸ��� ���� ��Ҹ� �ٿ�ε��ؾ� �մϴ�. 28.8 kbps ���� ����ϴ� ��� �ٿ�ε��ϴµ� 1�� ���� �ɸ��ϴ�."
    Const L_IntroMsg2_Text              = "���� ������ �����Ϸ��� Active ��ũ������ ����ؾ߸� �մϴ�. ���� �ٿ�ε� �ð��� ������ �ٿ�ε尡 �����ٴ� Ȯ�� �������� ������ ������, ��ũ������ ��� ������ �� Ȯ���Ͻʽÿ�."
    Const L_IntroMsg3_Text              = "Active ��ũ������ ����Ϸ���"
    Const L_IntroMsg4_Text              = "Internet Explorer�� <B>����</B> �޴����� <B>���ͳ� �ɼ�</B>�� Ŭ���Ͻʽÿ�."
    Const L_IntroMsg5_Text              = "<B>���� </B>���� Ŭ���Ͻʽÿ�."
    Const L_IntroMsg6_Text              = "<STRONG>���ͳ�</STRONG> �������� Ŭ���� ��, <STRONG>����� ���� ����</STRONG>�� Ŭ���Ͻʽÿ�."
    Const L_IntroMsg7_Text              = "��ũ���� ������ <STRONG>Active ��ũ����</STRONG> �Ʒ��� �ִ� <STRONG>���</STRONG>�� Ŭ���Ͻʽÿ�."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Microsoft�κ����� ��ũ&body=Microsoft Windows XP �� ����Ʈ�κ����� ��ũ�Դϴ�."
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>