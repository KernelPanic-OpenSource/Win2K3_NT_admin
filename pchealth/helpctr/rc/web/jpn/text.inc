<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "�����[�g �A�V�X�^���X�̊J�n"
	Const L_ProductName_Text			= "�����[�g �A�V�X�^���X"
	Const L_Title1Paragraph1_Text		= "�����[�g �A�V�X�^���X���g�����w���v"
	Const L_Paragraph1_Text				= "�R���s���[�^�̖�������Ă���l��������ꍇ�A���ۂɖڂ̑O�ł��̉������@�������Ă�����ƕ�����₷�����Ƃ��悭����܂��B�����[�g �A�V�X�^���X���g���ƁA�F�l�̃R���s���[�^�Ɉ��S�ɐڑ����A�������̎菕�������Ă����邱�Ƃ��ł��܂��B"
	Const L_Paragraph2_Text				= "���߂ă����[�g �A�V�X�^���X���g���ꍇ�́A�ȉ��̎��������ǂ݂��������B"
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">�F�l�̃R���s���[�^�ɐڑ�����ɂ́A</font><b><font color=""#000000"" size=""2""><b>�����[�g �A�V�X�^���X�̊J�n</b></font></b>���N���b�N���Ă��������B"
	Const L_StartRAButton_Text			= "�����[�g �A�V�X�^���X�̊J�n"
	Const L_JScriptNotAvail1_Text		= "Internet Explorer �̃Z�L�����e�B�����ɐݒ肳��Ă��邽�߁A�����[�g �A�V�X�^���X���g�p���邱�Ƃ��ł��܂���B�����[�g �A�V�X�^���X���g�p����ɂ́A�ݒ��ύX���邩���̃T�C�g��M���ς݃T�C�g�Ƃ��Ēǉ����Ă��������B"
	Const L_IEHelp_Text					= "�Z�L�����e�B�ݒ�̕ύX�Ɋւ���ڂ������́AInternet Explorer �̃w���v���Q�Ƃ��Ă��������B"
	Const L_TicketError1_Text			= "�����[�g �A�V�X�^���X�̏��҂��Ȃ����A�����ł��B�����[�g �A�V�X�^���X���g�p����ɂ́A�L���ȏ��҂��K�v�ł��B"
	Const L_InstallIE_Text				= "�����[�g �A�V�X�^���X�̏��҂��󂯓����ɂ́AInternet Explorer 5.5 ���C���X�g�[�����Ă��������B"
	Const L_TicketErrorMsg_Text			= "�����[�g �A�V�X�^���X�Ɋւ���ڂ��������Q�Ƃ���ɂ́AWindows XP �� <B>[�X�^�[�g]</B> ���j���[�ɂ��� <B>[�w���v�ƃT�|�[�g]</B> ���N���b�N���Ă���A<B>[����]</B> �{�b�N�X�� <B>�u�����[�g �A�V�X�^���X�v</B> �Ɠ��͂��Ă��������B"
	
	Const L_NotesTitle_Text				= "����"
	Const L_NotesLine1_Text				= "�M���ł���l����̏��҂̂ݎ󂯓���Ă��������B"
	
	Const L_NotesLine2_Text				= "�����[�g �A�V�X�^���X���g���Ă���Ԃ́A���Ȃ����A�����Ă��Ȃ����菕�����郆�[�U�[���A�C���^�[�l�b�g�ɐڑ����Ă���K�v������܂��B���̏��ҏ�𑗐M�������[�U�[���A�����̃R���s���[�^�ւ��Ȃ����ڑ����邱�Ƃ�������ƁA���Ȃ��͑���̃R���s���[�^�̉�ʂ����Ȃ��烊�A���^�C���Ń`���b�g���邱�Ƃ��ł��܂��B"
	
	Const L_NotesLine6_Text				= "����̓Z�L�����e�B�ŕی삳�ꂽ�A����ʍs�̐ڑ��ł��B����͂��Ȃ��̃R���s���[�^�ɃA�N�Z�X���邱�Ƃ͂ł��܂���B���[�J�� �G���A �l�b�g���[�N��ō�Ƃ��Ă���ꍇ�A�t�@�C�A�E�H�[���������Ń����[�g �A�V�X�^���X�𗘗p�ł��Ȃ����Ƃ�����܂��B���̂悤�ȏꍇ�́A�l�b�g���[�N�Ǘ��҂ɖ₢���킹�ďڂ���������肵�Ă��������B"
	
	Const L_LearnXP_Text				= "�����[�g �A�V�X�^���X�Ɋւ���ڂ��������Q�Ƃ���ɂ́AWindows XP �� <B>[�X�^�[�g]</B> ���j���[�ɂ��� <B>[�w���v�ƃT�|�[�g]</B> ���N���b�N���Ă���A<B>[����]</B> �{�b�N�X�� <B>�u�����[�g �A�V�X�^���X�v</B> �Ɠ��͂��Ă��������B"
	
	Const L_ContactUsMsg_Text			= "���₢���킹"
	

	Const L_XPNewsletterMsg_Text		= "�����j���[�X���^�["
	Const L_EmailPage_Text				= "���̃y�[�W��d�q���[���ő��M����"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. All rights reserved. Terms of Use."
	
	Const L_PrivacyMsg_Text				= "���q�l�̃v���C�o�V�["

	Const L_AccMsg_Text					= "�A�N�Z�V�r���e�B"

	Const L_NotIEError1_Text			= "�����[�g �A�V�X�^���X�̏��҂��󂯓����ɂ́AInternet Explorer 5.5 �܂��͂���ȍ~�̃o�[�W�������C���X�g�[������K�v������܂��B"

	Const L_NotXPError1_Text			= "�����[�g �A�V�X�^���X�� Windows XP �̋@�\�ł��B�����[�g �A�V�X�^���X�𗘗p����ɂ́A�܂� Windows XP ���C���X�g�[������K�v������܂��B"

	Const L_IELinkMsg_Text				= "�ŐV�� Internet Explorer ���_�E�����[�h����"
	Const L_XPLinkMsg_Text				= "Windows XP �Ɋւ���ڂ������́A"
	Const L_XPLinkMsg2_Text				= "Microsoft Windows Web �T�C�g"
	Const L_XPLinkMsg3_Text				= "���Q�Ƃ��Ă��������B"
	
	Const L_IntroMsg1_Text              = "�����[�g �A�V�X�^���X�ɕK�v�ȃR���|�[�l���g���_�E�����[�h���Ă��܂��B28.8 kbps ���f�����g�p���Ă���ꍇ�A�_�E�����[�h�ɖ� 1 ��������܂��B"
    Const L_IntroMsg2_Text              = "�����[�g �A�V�X�^���X�����s����ɂ́A�A�N�e�B�u �X�N���v�g���L���ɂȂ��Ă��Ȃ���΂Ȃ�܂���B�_�E�����[�h�̏��v���Ԃ��o�߂��Ă��_�E�����[�h�����̃��b�Z�[�W���\������Ȃ��ꍇ�́A�A�N�e�B�u �X�N���v�g���L���ɂȂ��Ă��邩�ǂ������m�F���Ă��������B"
    Const L_IntroMsg3_Text              = "�A�N�e�B�u �X�N���v�g��L���ɂ���ɂ�"
    Const L_IntroMsg4_Text              = "Internet Explorer �� <B>[�c�[��]</B> ���j���[�ɂ��� <B>[�C���^�[�l�b�g �I�v�V����]</B> ���N���b�N���܂��B"
    Const L_IntroMsg5_Text              = "<B>[�Z�L�����e�B]</B> �^�u���N���b�N���܂��B"
    Const L_IntroMsg6_Text              = "<STRONG>�C���^�[�l�b�g</STRONG> �A�C�R�����N���b�N���Ă���A<STRONG>[���x���̃J�X�^�}�C�Y]</STRONG> ���N���b�N���܂��B"
    Const L_IntroMsg7_Text              = "[�X�N���v�g] �Z�N�V������ <STRONG>[�A�N�e�B�u �X�N���v�g]</STRONG> �� <STRONG>[�L���ɂ���]</STRONG> �ɐݒ肵�܂��B"
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=�}�C�N���\�t�g����̃����N&body=Microsoft Windows XP Web �T�C�g����̃����N�ł� - "
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>