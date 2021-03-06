/*
Launchy: Application Launcher
Copyright (C) 2005  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// LaunchyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Launchy.h"
#include "LaunchyDlg.h"
#include "HotkeyDialog.h"
#include "Skin.h"
#include "SkinChooser.h"
#include "DirectoryChooser.h"
#include "AdvancedOptions.h"
#include "AboutDialog.h"
#include ".\launchydlg.h"
#include "plugin.h"
#include "PluginDialog.h"
#include <afxinet.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLaunchyDlg dialog

#define DELAY_TIMER 100
#define UPDATE_TIMER 101

CLaunchyDlg::CLaunchyDlg(CWnd* pParent /*=NULL*/)
: CDialogSK(CLaunchyDlg::IDD, pParent)
, DelayTimer(0), initialized(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);

	atLaunch = true;

	DelayTimer = 100;


	m_FontInput = NULL;
	m_FontResult = NULL;
	ShowLaunchyAtStart = false;
	ShowingDialog = false;
}

void CLaunchyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogSK::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_Input, InputBox);
	DDX_Control(pDX, IDC_PREVIEW, Preview);
	DDX_Control(pDX, IDC_PIC, IconPreview);
}

BEGIN_MESSAGE_MAP(CLaunchyDlg, CDialogSK)
	ON_MESSAGE(WM_HOTKEY,OnHotKey)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_Input, &CLaunchyDlg::OnCbnSelchangeInput)
	ON_WM_TIMER()
	ON_WM_ENDSESSION()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(LAUNCHY_DB_DONE, OnDBDone)
END_MESSAGE_MAP()

UINT CheckForUpdate (LPVOID pParam) { 

	CInternetSession session;
	CHttpFile* file = NULL;
	CHttpConnection* pServer = NULL;

	try {

		CHttpFile* file = reinterpret_cast<CHttpFile*>( session.OpenURL( L"http://www.launchy.net/version.html"));
		DWORD infocode;
		file->QueryInfoStatusCode(infocode);
		if (infocode != 200) return 0;
		char* buff = (char*) malloc(sizeof(char) * 256);
		file->Read(buff,10);
		int latest = atoi(buff);
		free(buff);

		if (LAUNCHY_VERSION < latest) {
			AfxMessageBox(L"A new version of Launchy is available.\n\nYou can download it at http://www.launchy.net/");
		}

		delete file;
		file = NULL;


		// Now get the stats
		//http://m1.webstats.motigo.com/n?id=AEJV3A4l/cDSX3qBPvhGeIRGerIg
		CHttpConnection* pServer = NULL;
		pServer = session.GetHttpConnection(L"m1.webstats.motigo.com");

		file = pServer->OpenRequest(NULL, 
			L"n?id=AEJV3A4l/cDSX3qBPvhGeIRGerIg",
			L"http://www.launchy.net/stats.html");

		CString szHeaders = L"Accept: image/gif, text/plain, text/html, text/htm\r\n";
		file->AddRequestHeaders(szHeaders);
		file->SendRequest();

	} 	catch (CInternetException* pEx)
	{
	}
	if (file != NULL) delete file;
	if (pServer != NULL) delete pServer;

	
	return 0;
}


// CLaunchyDlg message handlers

BOOL CLaunchyDlg::OnInitDialog()
{

	AfxInitRichEdit();

	ShowWindows(false);

	CDialogSK::OnInitDialog();



	// Change the directory to Launchy's executable
	int numArgs;
	LPWSTR*  strings = CommandLineToArgvW(GetCommandLine(), &numArgs);
	if (strings == NULL) {
		AfxMessageBox(L"There was an error with Launchy initialization, quitting");
		return 0;
	}
	if (numArgs == 2) {
		CString arg1;
		arg1 = strings[1];		
		if (arg1 == L"/wait")
			ShowLaunchyAtStart = true;
	}



	CString tmp = strings[0];
	tmp = tmp.Left(tmp.ReverseFind(L'\\')+1);
	_wchdir(tmp.GetBuffer());
	LocalFree(strings);



	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon




	SetWindowLong(GetSafeHwnd(), GWL_EXSTYLE, GetWindowLong(GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_TOOLWINDOW);


	// BEGIN SKINNING FUNCTIONS
	EnableEasyMove();                       // enable moving of
	// the dialog by
	// clicking
	// anywhere in
	// the dialog


	options.reset(new Options());
	plugins.reset(new Plugin());

	smarts.reset(new LaunchySmarts());

	applySkin();


	// In order to subclass the combobox list and edit controls
	// we have to first paint the controls to make sure the message
	// mapping is setup before we use the controls.

//	InputBox.ShowDropDown(1);
//	InputBox.ShowDropDown(0);
	//	InputBox.DoSubclass();

	if (options->stickyWindow || ShowLaunchyAtStart) {
		//		HideLaunchy();
		ShowLaunchy();
	}
	else {
		HideLaunchy();
	}
	//	InputBox.doSubclass();

	BOOL m_isKeyRegistered = RegisterHotKey(GetSafeHwnd(), 100,
		options->mod_key, options->vkey);

	ASSERT(m_isKeyRegistered != FALSE);

	if (options->indexTime != 0)
		SetTimer(UPDATE_TIMER, 60000, NULL);




	if (options->chkupdate)
		AfxBeginThread(CheckForUpdate, NULL, THREAD_PRIORITY_IDLE);


	initialized = true;
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLaunchyDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect); 
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogSK::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CLaunchyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



LRESULT CLaunchyDlg::OnHotKey(WPARAM wParam, LPARAM lParam) {
	if (wParam == 100) {
		if (atLaunch)
		{
			this->Visible = false;
			atLaunch = false;
		}
		if (ShowingDialog) return 1;
		if (options->stickyWindow) {
			this->ShowLaunchy();
		} else {
			this->Visible = !this->Visible;
			if (Visible)
			{
				this->ShowLaunchy();
			}
			else {
				HideLaunchy();
			}
		}
	}
	return 1;
}

void CLaunchyDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	if(this->atLaunch) {

		// This can either get called before or after OnInitDlg (maybe?)

		// If before oninitdialog, then it's safe to hide
		if (!this->initialized) {
			lpwndpos->flags &= ~SWP_SHOWWINDOW;
		} else {	
			// If after oninitdialog, only hide if supposed to
			if (options != NULL && !options->stickyWindow && !ShowLaunchyAtStart) {
				lpwndpos->flags &= ~SWP_SHOWWINDOW;		
			}

		}
	}


	CDialogSK::OnWindowPosChanging(lpwndpos);


	// TODO: Add your message handler code here
}

void CLaunchyDlg::OnClose()
{
	// Wait for all threads to finish 

	// Save our settings before the database
	options->Store();
	// Must close smarts before options!  
	smarts.reset();
	plugins.reset();
	options.reset();
	CoUninitialize();
	//	border.OnClose();
	// TODO: Add your message handler code here and/or call default
	CDialogSK::OnClose();
}

void CLaunchyDlg::OnDestroy()
{
	OnClose();
	CDialogSK::OnDestroy();

	// TODO: Add your message handler code here
}




void CLaunchyDlg::OnCbnSelchangeInput()
{
	// TODO: Add your control notification handler code here
}




BOOL CLaunchyDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_SYSKEYDOWN) {
			if (pMsg->wParam==VK_BACK) {
				InputBox.DeleteLine();
				return true;
			}
	}

	if(pMsg->message==WM_KEYDOWN)
	{

		if (GetKeyState(VK_CONTROL) < 0) {
			// Control is pressed down
			if (pMsg->wParam==VK_BACK) {	 
				InputBox.DeleteWord();
				return true;
			}
		}	
		
		else if (pMsg->wParam==VK_DOWN) {
			
			if (!InputBox.GetDroppedState() && InputBox.typed != _T("")) {
				InputBox.ShowDropDown(true);
			}
		} 
		else if (pMsg->wParam==VK_UP) {
			if (!InputBox.GetDroppedState() && InputBox.typed != _T("")) {
				InputBox.ShowDropDown(true);
			}
		}
		else if (pMsg->wParam==VK_TAB) {
			if ( searchTxt != L"")
				InputBox.TabSearchTxt();
			return true;
		}

		else {

			if (InputBox.GetDroppedState()) {
				CString typed = InputBox.typed;
				InputBox.ShowDropDown(false);
				InputBox.SetEditSel(InputBox.GetWindowTextLengthW(), InputBox.GetWindowTextLengthW());
			}
		}
		SetTimer(DELAY_TIMER, 1000, NULL);
		if(pMsg->wParam==VK_RETURN) {
			HideLaunchy();

			CString MatchPath = smarts->GetMatchPath(0);
			if (smarts->GetMatchPath(0) != InputBox.searchPath && SearchStrings.GetCount() == 0) {


				//			if (InputBox.typed != searchTxt) {
				CString x;
				options->Associate(InputBox.typed, smarts->GetMatchPath(0)); 	
				options->Store();
			}

			KillTimer(DELAY_TIMER);
			smarts->Launch();
			pMsg->wParam = NULL;
		}

		if (pMsg->wParam==VK_ESCAPE) {
			HideLaunchy();
			pMsg->wParam = NULL;
		}

		InputBox.CleanText();
	} 

	if (pMsg->message == WM_CHAR) {
		SetTimer(DELAY_TIMER, 1000, NULL);
		InputBox.CleanText();
	}
	return CDialogSK::PreTranslateMessage(pMsg);
}

void CLaunchyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	if (nIDEvent == DELAY_TIMER) {
		if (Visible && !InputBox.GetDroppedState() && InputBox.GetWindowTextLengthW() > 0 &&
			InputBox.GetCount() > 1) {
				InputBox.SetCurSel(-1);
				InputBox.ShowDropDown(true);
				//				InputBox.m_edit.SetWindowText(InputBox.typed);
				InputBox.ReformatDisplay();
				InputBox.ParseSearchTxt();
				//				InputBox.SetEditSel(InputBox.typed.GetLength(), InputBox.typed.GetLength());
				InputBox.CleanText();
			}
			KillTimer(DELAY_TIMER);
			CDialogSK::OnTimer(nIDEvent);
	}
	else if (nIDEvent == UPDATE_TIMER) {
		smarts->LoadCatalog();
		KillTimer(UPDATE_TIMER);
		if (options->indexTime != 0)
			SetTimer(UPDATE_TIMER, options->indexTime * 60000, NULL);
	}
}

void CLaunchyDlg::OnEndSession(BOOL bEnding)
{
	if (options != NULL) { options->Store(); }
	CDialogSK::OnEndSession(bEnding);

	// TODO: Add your message handler code here
}

void CLaunchyDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// TODO: Add your message handler code here
	// Load the desired menu
	CMenu mnuPopupSubmit;
	mnuPopupSubmit.LoadMenu(IDR_MENU1);


	// Get a pointer to the first item of the menu
	CMenu *mnuPopupMenu = mnuPopupSubmit.GetSubMenu(0);
	ASSERT(mnuPopupMenu);

	DWORD selection = mnuPopupMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON| TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, this);

	if (selection == ID_SETTINGS_SKINS) {
		SkinChooser dlg;
		ShowingDialog = true;
		dlg.DoModal();
		ShowingDialog = false;
		applySkin();
		options->Store();
	}
	else if (selection == ID_SETTINGS_PLUGINS) {
		// We may have unloaded a dll responsible for the currently displayed launchy query, 
		// So we need to clear the current entry!

		this->ClearEntry();
		CPluginDialog dlg;
		ShowingDialog = true;
		dlg.DoModal();
		ShowingDialog = false;
		smarts->LoadCatalog();
	}
	else if (selection == ID_SETTINGS_HOTKEY) {
		CHotkeyDialog dlg;
		ShowingDialog = true;
		dlg.DoModal();
		ShowingDialog = false;
		options->Store();
	}

	else if (selection == ID_SETTINGS_DIRECTORIES) {
		ShowingDialog = true;
		DirectoryChooser dlg;
		dlg.DoModal();
		ShowingDialog = false;
		options->Store();
		smarts->LoadCatalog();
	}

	else if (selection == ID_SETTINGS_ADVANCED) {
		AdvancedOptions dlg;
		ShowingDialog = true;
		dlg.DoModal();
		ShowingDialog = false;
		options->Store();
		// Apply any changes to the windows, such as always on top
		this->applySkin();
	}

	else if (selection == ID_SETTINGS_REBUILD) {
		smarts->LoadCatalog();
	}

	else if (selection == ID_SETTINGS_ABOUT) {
		AboutDialog dlg;
		ShowingDialog = true;
		dlg.DoModal();
		ShowingDialog = false;
	}


	else if (selection == ID_EXIT) {
		this->EndDialog(1);
	}
}

void CLaunchyDlg::applySkin()
{
	RECT previousPos;
	this->GetWindowRect(&previousPos);


	if (options->skin == NULL) {
		options->SetSkin(_T("Default"));
		if (options->skin == NULL) {
			return;
		}
	}

	if (border.inuse) {
		border.inuse = false;
		border.DestroyWindow();
	}



	if (options->skin->resultBorder) {
		SetWindowLong(Preview.GetSafeHwnd(), GWL_EXSTYLE, GetWindowLong(Preview.GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_CLIENTEDGE);
	} else {
		SetWindowLong(Preview.GetSafeHwnd(), GWL_EXSTYLE, GetWindowLong(Preview.GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
	}

	InputBox.m_RemoveFrame = !options->skin->inputBorder;
	InputBox.m_Transparent = options->skin->inputTransparent;

	// After changing the windowlong, the window needs to get moved to update it.. so send
	// it to the top left corner for now and we'll move it back into position later.
	RECT r;
	r.top = 0;
	r.left = 0;

	Preview.MoveWindow(&r,1);

	SetBitmap(options->skin->bgFile);
	SetStyle (LO_STRETCH);                   // resize dialog to
	if (options->skin->translucensy != -1) {
		SetTransparent(options->skin->translucensy);
	} else {
		SetTransparentColor(options->skin->trans_rgb);    // set red as
	}

	if (options->skin->alphaBorderFile == "") {
		border.inuse = false;
	} else {
		if (!border.inuse) {
			int ret = border.Create(IDD_ALPHA_BORDER, this);
		}
		
		border.ShowWindow(false);
		border.SetImage(options->skin->alphaBorderFile);
		border.inuse = true;

		RECT r;
		GetWindowRect(&r);
		border.MoveWindow(r.left + options->skin->alphaRect.left, r.top + options->skin->alphaRect.top, r.right, r.bottom, 1);
		ShowWindows(IsWindowVisible() != 0);
	}




	//	InputBox.SetFont(options->skin->m_FontInput,1);
	//	Preview.SetFont(options->skin->m_FontResult,1);



	CFont* old1;
	CFont* old2;
	CFont* old3;

	old1 = m_FontInput;
	old2 = m_FontResult;
	old3 = m_FontInputSmall;

	m_FontInput = new CFont;
	m_FontResult = new CFont;
	m_FontInputSmall = new CFont;

	m_FontInputSmall->CreateFontW(
		options->skin->inputSmall_fontSize,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		options->skin->inputSmall_bold,						// nWeight
		options->skin->inputSmall_italics,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		options->skin->inputSmall_fontName);                 // lpszFacename	

	m_FontInput->CreateFontW(
		options->skin->input_fontSize,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		options->skin->input_bold,						// nWeight
		options->skin->input_italics,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		options->skin->input_fontName);                 // lpszFacename	


	// Fonts
	m_FontResult->CreateFontW(
		options->skin->results_fontSize,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		options->skin->results_bold,                 // nWeight
		options->skin->results_italics,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		options->skin->results_fontName);                 // lpszFacename	

	InputBox.SetSmallFont(m_FontInputSmall,options->skin->inputSmallFontRGB);
	InputBox.SetFont(m_FontInput,1);
	Preview.SetFont(m_FontResult,1);

	// Free the old fonts!
	if (old1 != NULL && old2 != NULL) {
		old1->DeleteObject();
		old2->DeleteObject();
		old3->DeleteObject();
		delete(old1);
		delete(old2);
		delete(old3);
	}



	InputBox.SetTextColor(options->skin->inputFontRGB);
	Preview.SetTextColor(options->skin->resultFontRGB);



	Preview.m_isBackTransparent = options->skin->resultTransparent;

	InputBox.SetBackColor(options->skin->inputRGB);

	Preview.SetBackColor(options->skin->resultRGB);	


	IconPreview.m_GrabBkgnd = true;

	// Set always on top or not
	this->SetWindowLayer(options->aot);

	if (previousPos.top != 0 || previousPos.left != 0)
		MoveWindow(previousPos.left, previousPos.top, options->skin->backRect.Width(), options->skin->backRect.Height(),1);
	else
		MoveWindow(options->posX, options->posY, options->skin->backRect.Width(), options->skin->backRect.Height(),1);
	InputBox.MoveWindow(options->skin->inputRect,1);
	Preview.MoveWindow(options->skin->resultRect,1);
	IconPreview.MoveWindow(options->skin->iconRect,1);	
	
	RedrawWindow();




}
LRESULT CLaunchyDlg::OnDBDone(UINT wParam, LONG lParam) {
	options->set_Indexing(false);
	smarts->Update(searchTxt, false);
	return true;
}
void CLaunchyDlg::HideLaunchy(void)
{
	if (!options->stickyWindow) {
		this->ShowWindows(SW_HIDE);
		this->Visible = false;
	}
	KillTimer(DELAY_TIMER);
	InputBox.ShowDropDown(false);
}

bool CLaunchyDlg::DoDonate(void)
{
	CTime time = CTime::GetCurrentTime();
	CTimeSpan span(21,0,0,0);
	if (options->installTime != 0 && options->installTime + span <= time) {
		smarts->exeLauncher.Run(_T("http://www.launchy.net/donate.html"));
		options->installTime = CTime(0);
		options->Store();
		return true;
	}
	return false;
}

void CLaunchyDlg::AdjustPostionIfOffscreen(void) {
	// width
	int cx = GetSystemMetrics(SM_CXSCREEN);
	// height
	int cy = GetSystemMetrics(SM_CYSCREEN);
	RECT location;
	GetWindowRect(&location);
	if (location.left > cx || location.top > cy) {
		if (options == NULL) return;
		MoveWindow(0, 0, options->skin->backRect.Width(), options->skin->backRect.Height(),1);		
	}
}

void CLaunchyDlg::ShowLaunchy(void)
{
	m_Foreground = GetForegroundWindow();
	smarts->SetActiveProgram(m_Foreground);
	this->DoDonate();	
	this->AdjustPostionIfOffscreen();
	this->Visible = true;
	this->ShowWindows((bool) SW_SHOW);
	this->ActivateTopParent();
	this->InputBox.SetFocus();
}

void CLaunchyDlg::ClearEntry(void) {
	InputBox.DeleteLine();
}

void CLaunchyDlg::SetWindowLayer(bool AlwaysOnTop) {
	RECT r;
	this->GetWindowRect(&r);

	if (AlwaysOnTop) {
		this->SetWindowPos(&CWnd::wndTopMost, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOREPOSITION);
		border.SetWindowPos(&CWnd::wndTopMost, r.left + options->skin->alphaRect.left, r.top + options->skin->alphaRect.top, r.right, r.bottom, SWP_NOMOVE | SWP_NOREPOSITION);
	} else {
		this->SetWindowPos(&CWnd::wndNoTopMost, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOREPOSITION);
		border.SetWindowPos(&CWnd::wndNoTopMost, r.left + options->skin->alphaRect.left, r.top + options->skin->alphaRect.top, r.right, r.bottom, SWP_NOMOVE | SWP_NOREPOSITION);
	}
}