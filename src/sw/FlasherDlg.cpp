// FlasherDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Flasher.h"
#include "FlasherDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DWORD g_dwProgress;
CString g_csMessage;
PCHAR g_pbDest;
usb_dev_handle *g_dev;



CHAR g_bLarge1, g_bLarge2;
CHAR g_byAddressCycles1, g_byAddressCycles2;
WORD g_wPageSize1, g_wPageSize2;
DWORD g_dwPageCount1, g_dwPageCount2;
DWORD g_dwBlockCount1, g_dwBlockCount2;
BYTE g_bNAND1, g_bNAND2;



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CFlasherDlg dialog




CFlasherDlg::CFlasherDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFlasherDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFlasherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, vProgress);
	DDX_Control(pDX, IDC_BUTTON1, btnDump);
	DDX_Control(pDX, IDC_BUTTON2, btnFlash);
	DDX_Control(pDX, IDC_EDIT1, edtProgress);
	DDX_Control(pDX, IDC_CBDEVICE1, cbDevice1);
	//	DDX_Control(pDX, IDC_CBPAGESIZE1, cbPageSize1);
	DDX_Control(pDX, IDC_CBBLOCKSIZE1, cbBlockSize1);
	DDX_Control(pDX, IDC_CBBLOCKCOUNT1, cbBlockCount1);
	//	DDX_Control(pDX, IDC_CHECK1, cbDouble);
	//	DDX_Control(pDX, IDC_NOR, rbNor);
	//	DDX_Control(pDX, IDC_NORSETTINGS, gbNorSettings);
	//	DDX_Control(pDX, IDC_NANDSETTINGS, gbNandSettings);
	//	DDX_Control(pDX, IDC_LIST1, lbPageSize);
	DDX_Control(pDX, IDC_CHECK1, cboxNAND1);
	DDX_Control(pDX, IDC_CHECK2, cboxNAND2);
	DDX_Control(pDX, IDC_CBDEVICE2, cbDevice2);
	//	DDX_Control(pDX, IDC_CBPAGESIZE2, cbPageSize2);
	DDX_Control(pDX, IDC_CBBLOCKSIZE2, cbBlockSize2);
	DDX_Control(pDX, IDC_CBBLOCKCOUNT2, cbBlockCount2);
	DDX_Control(pDX, IDC_IDENT1, txtIdent1);
	DDX_Control(pDX, IDC_IDENT2, txtIdent2);
	//	DDX_Control(pDX, IDC_RADIO4, rbDevice1);
	//	DDX_Control(pDX, IDC_RADIO6, rbDevice2);
	DDX_Control(pDX, IDC_NORSECTORSIZE, cbNorSectorSize);
	DDX_Control(pDX, IDC_NORSECTORCOUNT, cbSectorCount);
	DDX_Control(pDX, IDC_CBOXBIGBLOCK1, cboxBigBlock1);
	DDX_Control(pDX, IDC_RAW1, cboxRaw1);
	DDX_Control(pDX, IDC_CBOXBIGBLOCK2, cboxBigBlock2);
	DDX_Control(pDX, IDC_RAW2, cboxRaw2);
	DDX_Control(pDX, IDC_DIFF1, cboxDiff1);
	DDX_Control(pDX, IDC_DIFF2, cboxDiff2);
	DDX_Control(pDX, IDC_DIFF3, cboxDiff3);
	DDX_Control(pDX, IDC_VERIFYNOR, cboxNorVerify);
	DDX_Control(pDX, IDC_VERIFYNAND, cboxNandVerify);
	DDX_Control(pDX, IDC_CBOXALTERNATE, cboxAlternate);
	DDX_Control(pDX, IDC_CBNORSTARTSECTOR, cbNorStartSector);
	DDX_Control(pDX, IDC_CBNORENDSECTOR, cbNorEndSector);
	DDX_Control(pDX, IDC_CBOXBYTESWAP, cboxByteSwap);
}

BEGIN_MESSAGE_MAP(CFlasherDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
//	ON_BN_CLICKED(IDC_BUTTON1, &CFlasherDlg::OnBnClickedButton1)
//	ON_BN_CLICKED(IDC_BUTTON2, &CFlasherDlg::OnBnClickedButton2)
//	ON_BN_CLICKED(IDC_BUTTON3, &CFlasherDlg::OnBnClickedButton3)
//	ON_CBN_SELCHANGE(IDC_COMBO1, &CFlasherDlg::OnCbnSelchangeCombo1)
//	ON_CBN_SELCHANGE(IDC_COMBO2, &CFlasherDlg::OnCbnSelchangeCombo2)
//	ON_CBN_SELCHANGE(IDC_COMBO3, &CFlasherDlg::OnCbnSelchangeCombo3)
//	ON_CBN_SELCHANGE(IDC_COMBO4, &CFlasherDlg::OnCbnSelchangeCombo4)
//ON_BN_CLICKED(IDC_NOR, &CFlasherDlg::OnBnClickedNor)
//ON_BN_CLICKED(IDC_NAND, &CFlasherDlg::OnBnClickedNand)
//ON_CBN_SELCHANGE(IDC_CBPAGESIZE1, &CFlasherDlg::OnCbnSelchangeCbpagesize)
//ON_CBN_SELCHANGE(IDC_CBBLOCKSIZE1, &CFlasherDlg::OnCbnSelchangeCbblocksize)
//ON_CBN_SELCHANGE(IDC_CBBLOCKCOUNT1, &CFlasherDlg::OnCbnSelchangeCbblockcount)
//ON_BN_CLICKED(IDC_CHECK1, &CFlasherDlg::OnBnClickedCheck1)
//ON_BN_CLICKED(IDC_CHECK2, &CFlasherDlg::OnBnClickedCheck2)
//ON_CBN_SELCHANGE(IDC_CBPAGESIZE2, &CFlasherDlg::OnCbnSelchangeCbpagesize2)
//ON_CBN_SELCHANGE(IDC_CBBLOCKSIZE2, &CFlasherDlg::OnCbnSelchangeCbblocksize2)
//ON_CBN_SELCHANGE(IDC_CBBLOCKCOUNT2, &CFlasherDlg::OnCbnSelchangeCbblockcount2)
//ON_BN_CLICKED(IDC_BUTTON4, &CFlasherDlg::OnBnClickedButton4)
ON_BN_CLICKED(IDCANCEL, &CFlasherDlg::OnBnClickedCancel)
//ON_BN_CLICKED(IDC_RBDEVICE1, &CFlasherDlg::OnBnClickedRbdevice1)
//ON_BN_CLICKED(IDC_RADIO4, &CFlasherDlg::OnBnClickedRadio4)
//ON_BN_CLICKED(IDC_RADIO5, &CFlasherDlg::OnBnClickedRadio5)
//ON_NOTIFY(NM_THEMECHANGED, IDC_RADIO4, &CFlasherDlg::OnNMThemeChangedRadio4)
//ON_NOTIFY(BCN_HOTITEMCHANGE, IDC_RADIO4, &CFlasherDlg::OnBnHotItemChangeRadio4)
//ON_BN_SETFOCUS(IDC_RADIO4, &CFlasherDlg::OnBnSetfocusRadio4)
//ON_BN_CLICKED(IDC_BUTTON5, &CFlasherDlg::OnBnClickedButton5)
//ON_BN_CLICKED(IDC_BUTTON3, &CFlasherDlg::OnBnClickedButton3)
//ON_BN_CLICKED(IDC_BUTTON5, &CFlasherDlg::OnBnClickedButton5)
//ON_BN_CLICKED(IDC_BUTTON2, &CFlasherDlg::OnBnClickedButton2)
ON_BN_CLICKED(IDC_NORDUMP, &CFlasherDlg::OnBnClickedNordump)
ON_BN_CLICKED(IDC_NORFLASH, &CFlasherDlg::OnBnClickedNorflash)
ON_BN_CLICKED(IDC_NANDDUMP, &CFlasherDlg::OnBnClickedNanddump)
ON_BN_CLICKED(IDC_NANDFLASH, &CFlasherDlg::OnBnClickedNandflash)
ON_BN_CLICKED(IDC_CBOXNAND1, &CFlasherDlg::OnBnClickedCboxnand1)
ON_BN_CLICKED(IDC_CBOXNAND2, &CFlasherDlg::OnBnClickedCboxnand2)
ON_BN_CLICKED(IDC_TESTSHORTS, &CFlasherDlg::OnBnClickedTestshorts)
ON_WM_WINDOWPOSCHANGED()
ON_CBN_SELCHANGE(IDC_NORSECTORCOUNT, &CFlasherDlg::OnCbnSelchangeNorsectorcount)
//ON_CBN_DROPDOWN(IDC_NORSECTORCOUNT, &CFlasherDlg::OnCbnDropdownNorsectorcount)
ON_CBN_SELENDOK(IDC_NORSECTORCOUNT, &CFlasherDlg::OnCbnSelendokNorsectorcount)
END_MESSAGE_MAP()


/*void ParseLine(CHAR *pbyLine, CHAR *psName, CHAR *psLarge, CHAR *psAddressCycles, CHAR *psPageSize, CHAR *psPageCount)
{
	while(*pbyLine != 0)
	{
		while(*pbyLine != ';')
			*(psName++) = *(pbyLine++);
		while(*pbyLine != ';')
			*(psLarge++) = *(pbyLine++);
		while(*pbyLine != ';')
			*(psAddressCycles++) = *(pbyLine++);
		while(*pbyLine != ';')
			*(psPageSize++) = *(pbyLine++);
		while(*pbyLine != ';')
			*(psPageCount++) = *(pbyLine++);
	}
}*/

// CFlasherDlg message handlers

BOOL CFlasherDlg::OnInitDialog()
{
	int something = MessageBox("Dear user,\nBy proceeding, you acknowledge that this software is still in beta phase.\nIt may contain bugs and they should be reported in the forum.\nClick YES to continue or NO to close.", "ProgSkeet Flasher", MB_YESNO);
	if(something == 7)
		OnCancel();
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

//	cbPageSize1.SetCurSel(0);
	cbBlockSize1.SetCurSel(0);
	cbBlockCount1.SetCurSel(0);
//	cbPageSize2.SetCurSel(0);
	cbBlockSize2.SetCurSel(0);
	cbBlockCount2.SetCurSel(0);
	cbNorSectorSize.SetCurSel(0);
	cbSectorCount.SetCurSel(0);

	CString csTemp = "";

	// With CString
	char    szAppPath[MAX_PATH] = "";
	CString strAppDirectory;

	::GetModuleFileName(0, szAppPath, sizeof(szAppPath) - 1);

	// Extract directory
	strAppDirectory = szAppPath;
	strAppDirectory = strAppDirectory.Left(strAppDirectory.ReverseFind('\\'));

	theApp.Debug_Setup(strAppDirectory+"\\ProgSkeet.log");

	CString temp = "";
	cbSectorCount.GetLBText(cbSectorCount.GetCurSel(), temp);
	int dwSectorCount = atoi(temp.GetString());
	int i = 0;
	
	
	for(i=0; i<4096; i++)
	{
		temp.Format("%i",i);
		cbNorStartSector.AddString(temp);
		cbNorEndSector.AddString(temp);
	}
	
	cbNorStartSector.SetCurSel(0);
	cbNorEndSector.SetCurSel(dwSectorCount-1);
	
	

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFlasherDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFlasherDlg::OnPaint()
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
		CDialog::OnPaint();
	}
	//CFlasherDlg::RedrawWindow();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFlasherDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//UINT WorkerThreadProc( LPVOID Param )
//{
//	g_dwProgress = 0;
//	theApp.ReadBlock(g_dev, g_pbDest, 0, 0x400, &g_dwProgress);
//	return TRUE;
//}


//UINT CFlasherDlg::DumpNAND( LPVOID Param )
//{
//	CHAR result = theApp.CreateDevice();
//	if(!result){
//		//MessageBox("Device not found!");
//		return 1;
//	}
//	CFileDialog FD(FALSE, 0, "dump.bin", 4|2, "*.bin", 0, 0, 1);
//	DWORD dwProgress = 0;
//	double totalspeed = 0;
//	DWORD timer;
//	double speed;
//	CHAR addresscycles = 5;
//	//PCHAR pbVerify;
//
//	CString temp = "";
//
//
//	if(FD.DoModal() == IDOK)
//	{
//		//AfxBeginThread(WorkerThreadProc,NULL,THREAD_PRIORITY_NORMAL,0,0,NULL);
//		
//		CString text;
//		if(g_bNAND1)
//		{
//			
//			g_wPageSize1 = cboxBigBlock1.GetCheck()?2048:512;
//			g_wPageSize1 += cboxRaw1.GetCheck()?((g_wPageSize1/512)*16):0;
//			cbBlockSize1.GetLBText(cbBlockSize1.GetCurSel(), temp);
//			g_dwPageCount1 = atoi(temp.GetString());
//			cbBlockCount1.GetLBText(cbBlockCount1.GetCurSel(), temp);
//			g_dwBlockCount1 = atoi(temp.GetString());
//
//			theApp.NAND_Configure(g_wPageSize1, g_dwPageCount1, g_dwBlockCount1, cboxBigBlock1.GetCheck(), 0);
//			vProgress.SetRange(1, (short) g_dwBlockCount1);
//			vProgress.SetStep(1);
//			
//			CString path = FD.GetPathName();
//			if(g_bNAND2)
//				path += ".0";
//			CFile temp(path, CFile::modeCreate | CFile::modeWrite);
//			temp.SeekToBegin();
//			RedrawWindow();
//			PCHAR pbDest = (PCHAR) malloc(g_dwPageCount1*g_wPageSize1);
//			PCHAR pbVerify = (PCHAR) malloc(g_dwPageCount1*g_wPageSize1);
//			memset(pbVerify, 0, g_dwPageCount1*g_wPageSize1);
//
//			for(DWORD i=0; i < g_dwBlockCount1; i++)
//			{
//				timer = GetTickCount();
//				do {
//					theApp.NAND_ReadBlock(i, pbDest, 0);
//					if(cboxNandVerify.GetCheck())
//						theApp.NAND_ReadBlock(i, pbVerify, 0);
//				} while (memcmp(pbVerify, pbDest, g_dwPageCount1*g_wPageSize1) && cboxNandVerify.GetCheck()); 
//
//				//theApp.NAND_ReadBlock(1, addresscycles, g_wPageSize1, g_dwPageCount1, i*g_dwPageCount1, pbDest, 0);
//				timer = GetTickCount() - timer;
//				speed = (g_dwPageCount1*g_wPageSize1) / (double) timer;
//				totalspeed += speed;
//				vProgress.SetPos(i+1);
//				text.Format("Dumping NAND 1 (%i%% / Currently: %.2f kB/s / Average: %.2f kB/s)", (int) ((((double)(i+1))/(double)g_dwBlockCount1)*100), speed, totalspeed/(i+1));
//				edtProgress.SetWindowTextA(text);
//				edtProgress.RedrawWindow();
//				//if((i%4)==0)
//					RedrawWindow();
//				
//				temp.Write(pbDest, g_dwPageCount1*g_wPageSize1);
//
//			}
//			temp.Close();
//			free(pbDest);
//			free(pbVerify);
//		}
//
//		if(g_bNAND2)
//		{
//			
//			g_wPageSize2 = cboxBigBlock2.GetCheck()?2048:512;
//			g_wPageSize2 += cboxRaw2.GetCheck()?16:0;
//
//			cbBlockSize2.GetLBText(cbBlockSize2.GetCurSel(), temp);
//			g_dwPageCount2 = atoi(temp.GetString());
//			cbBlockCount2.GetLBText(cbBlockCount2.GetCurSel(), temp);
//			g_dwBlockCount2 = atoi(temp.GetString());
//
//
//			theApp.NAND_Configure(g_wPageSize2, g_dwPageCount2, g_dwBlockCount2, cboxBigBlock2.GetCheck(), 1);
//			vProgress.SetRange(1, (short) g_dwBlockCount2);
//			vProgress.SetStep(1);
//			
//			CString path = FD.GetPathName();
//			if(g_bNAND1)
//				path += ".1";
//			CFile temp(path, CFile::modeCreate | CFile::modeWrite);
//			temp.SeekToBegin();
//			RedrawWindow();
//			PCHAR pbDest = (PCHAR) malloc(g_dwPageCount2*g_wPageSize2);
//			PCHAR pbVerify = (PCHAR) malloc(g_dwPageCount2*g_wPageSize2);
//
//			for(DWORD i=0; i < g_dwBlockCount2; i++)
//			{
//				timer = GetTickCount();
//
//				do {
//					theApp.NAND_ReadBlock(i, pbDest, 1);
//					if(cboxNandVerify.GetCheck())
//						theApp.NAND_ReadBlock(i, pbVerify, 1);
//				} while (memcmp(pbVerify, pbDest, g_dwPageCount2*g_wPageSize2) && cboxNandVerify.GetCheck());
//
//				//theApp.NAND_ReadBlock(1, addresscycles, g_wPageSize2, g_dwPageCount2, i*g_dwPageCount2, pbDest, 1);
//				timer = GetTickCount() - timer;
//				speed = (g_dwPageCount2*g_wPageSize2) / (double) timer;
//				totalspeed += speed;
//				vProgress.SetPos(i+1);
//				text.Format("Dumping NAND 2 (%i%% / Currently: %.2f kB/s / Average: %.2f kB/s)", (int) ((((double)(i+1))/(double)g_dwBlockCount2)*100), speed, totalspeed/(i+1));
//				edtProgress.SetWindowTextA(text);
//				edtProgress.RedrawWindow();
//				//if((i%4)==0)
//					RedrawWindow();
//				
//				temp.Write(pbDest, g_dwPageCount2*g_wPageSize2);
//
//			}
//			temp.Close();
//			free(pbDest);
//		}
//	}
//
//
//	theApp.RemoveDevice();
//
//	return TRUE;
//}


//void CFlasherDlg::OnCbnSelchangeCbpagesize()
//{
//	
//}

//void CFlasherDlg::OnCbnSelchangeCbblocksize()
//{
//	CString temp = "";
//	
//
//}

//void CFlasherDlg::OnCbnSelchangeCbblockcount()
//{
//	CString temp = "";
//
//}


//void CFlasherDlg::OnCbnSelchangeCbpagesize2()
//{
//	CString temp = "";
//
//}

//void CFlasherDlg::OnCbnSelchangeCbblocksize2()
//{
//	CString temp = "";
//}

//void CFlasherDlg::OnCbnSelchangeCbblockcount2()
//{
//	CString temp = "";
//
//}


void CFlasherDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}







void CFlasherDlg::OnBnClickedNordump()
{
	
	CHAR result = theApp.CreateDevice();
	if(!result){
		MessageBox("Device not found!");
		return;
	}
	theApp.NOR_Reset();
	CFileDialog FD(FALSE, 0, "dump.bin", 4|2, "*.bin", 0, 0, 1);
	if(FD.DoModal())
	{
		CString path = FD.GetPathName();
		CString temp = "";
		CFile filetemp(path, CFile::modeCreate | CFile::modeWrite);
		filetemp.SeekToBegin();

		cbNorSectorSize.GetLBText(cbNorSectorSize.GetCurSel(), temp);
		DWORD dwSectorSize = atoi(temp.GetString());
		cbSectorCount.GetLBText(cbSectorCount.GetCurSel(), temp);
		DWORD dwSectorCount = atoi(temp.GetString());

		cbNorStartSector.GetLBText(cbNorStartSector.GetCurSel(), temp);
		DWORD dwStartSector = atoi(temp.GetString());

		cbNorEndSector.GetLBText(cbNorEndSector.GetCurSel(), temp);
		DWORD dwEndSector = atoi(temp.GetString());


		PCHAR pbDest = (PCHAR) malloc(dwSectorSize*1024);
		PCHAR pbVerify = (PCHAR) malloc(dwSectorSize*1024);

		vProgress.SetRange((short) dwStartSector, (short) dwEndSector);
		vProgress.SetStep(1);
		DWORD timer, totaltime = 0;
		double speed, totalspeed = 0;
		CString text = "";
		for(DWORD i=dwStartSector; i<=dwEndSector; i++)
		{
			timer = GetTickCount();
			do {
				theApp.NOR_ReadSector(pbDest, i, dwSectorSize*512, cboxByteSwap.GetCheck()?1:0);
				if(cboxNorVerify.GetCheck())
					theApp.NOR_ReadSector(pbVerify, i, dwSectorSize*512, cboxByteSwap.GetCheck()?1:0);
			} while(memcmp(pbVerify, pbDest, dwSectorSize*1024) && cboxNorVerify.GetCheck());

			timer = GetTickCount() - timer;
			speed = (dwSectorSize*1024) / (double) timer;
			totaltime += timer;
			totalspeed = ((i-dwStartSector)*dwSectorSize*1024)/totaltime;
			vProgress.SetPos(i);
			text.Format("Dumping NOR (%i%% / Currently: %.2f kB/s / Average: %.2f kB/s)", (int) ((((double)(i+1-dwStartSector))/(double) (dwEndSector-dwStartSector))*100), speed, totalspeed);
			edtProgress.SetWindowTextA(text);
			edtProgress.RedrawWindow();

			filetemp.Write(pbDest, dwSectorSize*1024);
		}
		filetemp.Close();
		free(pbDest);
		free(pbVerify);
	}
	theApp.RemoveDevice();

}

void CFlasherDlg::OnBnClickedNorflash()
{
	CHAR result = theApp.CreateDevice();
	if(!result){
		MessageBox("Device not found!");
		return;
	}
	DWORD dwProgress = 0;
	double totalspeed = 0;
	DWORD timer, totaltime = 0;
	double speed = 0;


	CString temp = "";

		//AfxBeginThread(WorkerThreadProc,NULL,THREAD_PRIORITY_NORMAL,0,0,NULL);
		
	CString text, progress = "";
//	PCHAR pbSrc, pbDest;

	CString path = "*.bin"; //FD.GetPathName();
	CFileDialog FD(TRUE, 0, path , 4|2, path, 0, 0, 1);
	if(FD.DoModal())
	{


		cbNorSectorSize.GetLBText(cbNorSectorSize.GetCurSel(), temp);
		DWORD dwSectorSize = atoi(temp.GetString());
		cbSectorCount.GetLBText(cbSectorCount.GetCurSel(), temp);
		DWORD dwSectorCount = atoi(temp.GetString());

		cbNorStartSector.GetLBText(cbNorStartSector.GetCurSel(), temp);
		DWORD dwStartSector = atoi(temp.GetString());

		cbNorEndSector.GetLBText(cbNorEndSector.GetCurSel(), temp);
		DWORD dwEndSector = atoi(temp.GetString());

		PCHAR pbDest = (PCHAR) malloc(dwSectorSize*1024);
		PCHAR pbSrc = (PCHAR) malloc(dwSectorSize*1024);
		PCHAR pbVerify = (PCHAR) malloc(dwSectorSize*1024);
		PCHAR pbCheckAgainst = (PCHAR) malloc(dwSectorSize*1024);

		memset(pbCheckAgainst, 0xFF, dwSectorSize*1024);

		memset(pbVerify, 0, dwSectorSize*1024);

		vProgress.SetRange((short)dwStartSector, (short)dwEndSector);
		vProgress.SetStep(1);

		path = FD.GetPathName();
		CFile temp(path, CFile::modeRead);
		temp.SeekToBegin();
		RedrawWindow();

		double timeleft = 0;
		
		theApp.NOR_Reset();
	
		for(DWORD i=dwStartSector; i <= dwEndSector; i++)
		{
			temp.Read(pbDest, dwSectorSize*1024);
			timer = GetTickCount();
			int tries = 5;
			//do {
				if(cboxDiff3.GetCheck())
					theApp.NOR_ReadSector(pbSrc, i, dwSectorSize*512, cboxByteSwap.GetCheck()?1:0);
				else
				{
					text.Format("Erasing Sector %i", i);
					edtProgress.SetWindowTextA(text+progress);
					edtProgress.RedrawWindow();
					theApp.NOR_EraseSector(i, dwSectorSize*512);
					memset(pbSrc, 0xFF, dwSectorSize*1024);
				}
				
				if(memcmp(pbSrc, pbDest, dwSectorSize*1024))
				{
					if(memcmp(pbSrc, pbCheckAgainst, dwSectorSize*1024))
					{
						text.Format("Erasing Sector %i", i);
						edtProgress.SetWindowTextA(text+progress);
						edtProgress.RedrawWindow();
						theApp.NOR_EraseSector(i, dwSectorSize*512);
						memset(pbSrc, 0xFF, dwSectorSize*1024);
					}

					if(memcmp(pbDest, pbCheckAgainst, dwSectorSize*1024))
					{
						text.Format("Programming Sector %i", i);
						edtProgress.SetWindowTextA(text+progress);
						edtProgress.RedrawWindow();
						theApp.NOR_ProgramSector(i, pbDest, pbSrc, dwSectorSize*512, cboxAlternate.GetCheck()?1:0, cboxByteSwap.GetCheck()?1:0);
					}
				}
				else
				{
					text.Format("Flashing Sector %i skipped (source = destination).\r\n", i);
					edtProgress.SetWindowTextA(text+progress);
					theApp.Debug_Write(text);
					tries = 0;
				}
			//} while(tries);

			timer = GetTickCount() - timer;
			speed = (dwSectorSize*1024) / (double) timer;
			totaltime += timer;
			totalspeed = ((i-dwStartSector)*dwSectorSize*1024)/totaltime;
			vProgress.SetPos(i);
				
			timeleft = ((dwEndSector-i-1)*1024)/totalspeed;
			progress.Format("(%i%% / %.2f kB/s); Approx. %.0fs left. ", (int) ((((double)(i+1-dwStartSector))/(double)(dwEndSector-dwStartSector))*100), totalspeed, timeleft);
			edtProgress.SetWindowTextA(text);
			edtProgress.RedrawWindow();
			RedrawWindow();
		}

		if(cboxNorVerify.GetCheck())
		{
			vProgress.SetPos(dwStartSector);
			temp.SeekToBegin();
			for(DWORD i=dwStartSector; i <= dwEndSector; i++)
			{
				text.Format("Verifying Sector %i", i);
				edtProgress.SetWindowTextA(text);
				edtProgress.RedrawWindow();

				temp.Read(pbDest, dwSectorSize*1024);
				timer = GetTickCount();
				theApp.NOR_ReadSector(pbSrc, i, dwSectorSize*512, cboxByteSwap.GetCheck()?1:0);
				if(memcmp(pbDest, pbSrc, dwSectorSize*1024))
				{
					text.Format("Verification of Sector %i failed.\r\n", i);
					theApp.Debug_Write(text);
					edtProgress.SetWindowTextA(text);
					edtProgress.RedrawWindow();
				}
				vProgress.StepIt();
			}
		}

		text.Format("Flashing NOR DONE.(%.2f kB/s)", totalspeed);
		edtProgress.SetWindowTextA(text);
		edtProgress.RedrawWindow();

		temp.Close();
		free(pbDest);
		free(pbSrc);
		free(pbVerify);
		free(pbCheckAgainst);
	}
	theApp.RemoveDevice();
}

void CFlasherDlg::OnBnClickedNanddump()
{
	CHAR result = theApp.CreateDevice();
	if(!result){
		MessageBox("Device not found!");
		return;
	}
	CFileDialog FD(FALSE, 0, "dump.bin", 4|2, "*.bin", 0, 0, 1);
	DWORD dwProgress = 0;
	double totalspeed = 0;
	DWORD timer;
	double speed;
	CHAR addresscycles = 5;
	//PCHAR pbVerify;

	CString temp = "";


	if(FD.DoModal() == IDOK)
	{
		//AfxBeginThread(WorkerThreadProc,NULL,THREAD_PRIORITY_NORMAL,0,0,NULL);
		
		CString text;
		if(g_bNAND1)
		{
			
			g_wPageSize1 = cboxBigBlock1.GetCheck()?2048:512;
			g_wPageSize1 += cboxRaw1.GetCheck()?((g_wPageSize1/512)*16):0;
			cbBlockSize1.GetLBText(cbBlockSize1.GetCurSel(), temp);
			g_dwPageCount1 = atoi(temp.GetString());
			cbBlockCount1.GetLBText(cbBlockCount1.GetCurSel(), temp);
			g_dwBlockCount1 = atoi(temp.GetString());

			theApp.NAND_Configure(g_wPageSize1, g_dwPageCount1, g_dwBlockCount1, cboxBigBlock1.GetCheck(), 0);
			vProgress.SetRange(1, (short) g_dwBlockCount1);
			vProgress.SetStep(1);
			
			CString path = FD.GetPathName();
			if(g_bNAND2)
				path += ".0";
			CFile temp(path, CFile::modeCreate | CFile::modeWrite);
			temp.SeekToBegin();
			RedrawWindow();
			PCHAR pbDest = (PCHAR) malloc(g_dwPageCount1*g_wPageSize1);
			PCHAR pbVerify = (PCHAR) malloc(g_dwPageCount1*g_wPageSize1);
			memset(pbVerify, 0, g_dwPageCount1*g_wPageSize1);

			for(DWORD i=0; i < g_dwBlockCount1; i++)
			{
				timer = GetTickCount();
				do {
					theApp.NAND_ReadBlock(i, pbDest, 0);
					if(cboxNandVerify.GetCheck())
						theApp.NAND_ReadBlock(i, pbVerify, 0);
				} while (memcmp(pbVerify, pbDest, g_dwPageCount1*g_wPageSize1) && cboxNandVerify.GetCheck()); 

				//theApp.NAND_ReadBlock(1, addresscycles, g_wPageSize1, g_dwPageCount1, i*g_dwPageCount1, pbDest, 0);
				timer = GetTickCount() - timer;
				speed = (g_dwPageCount1*g_wPageSize1) / (double) timer;
				totalspeed += speed;
				vProgress.SetPos(i+1);
				text.Format("Dumping NAND 1 (%i%% / Currently: %.2f kB/s / Average: %.2f kB/s)", (int) ((((double)(i+1))/(double)g_dwBlockCount1)*100), speed, totalspeed/(i+1));
				edtProgress.SetWindowTextA(text);
				edtProgress.RedrawWindow();
				//if((i%4)==0)
					//RedrawWindow();
				
				temp.Write(pbDest, g_dwPageCount1*g_wPageSize1);

			}
			temp.Close();
			free(pbDest);
			free(pbVerify);
		}

		if(g_bNAND2)
		{
			
			g_wPageSize2 = cboxBigBlock2.GetCheck()?2048:512;
			g_wPageSize2 += cboxRaw2.GetCheck()?((g_wPageSize2/512)*16):0;

			cbBlockSize2.GetLBText(cbBlockSize2.GetCurSel(), temp);
			g_dwPageCount2 = atoi(temp.GetString());
			cbBlockCount2.GetLBText(cbBlockCount2.GetCurSel(), temp);
			g_dwBlockCount2 = atoi(temp.GetString());


			theApp.NAND_Configure(g_wPageSize2, g_dwPageCount2, g_dwBlockCount2, cboxBigBlock2.GetCheck(), 1);
			vProgress.SetRange(1, (short) g_dwBlockCount2);
			vProgress.SetStep(1);
			
			CString path = FD.GetPathName();
			if(g_bNAND1)
				path += ".1";
			CFile temp(path, CFile::modeCreate | CFile::modeWrite);
			temp.SeekToBegin();
			RedrawWindow();
			PCHAR pbDest = (PCHAR) malloc(g_dwPageCount2*g_wPageSize2);
			PCHAR pbVerify = (PCHAR) malloc(g_dwPageCount2*g_wPageSize2);

			for(DWORD i=0; i < g_dwBlockCount2; i++)
			{
				timer = GetTickCount();

				do {
					theApp.NAND_ReadBlock(i, pbDest, 1);
					if(cboxNandVerify.GetCheck())
						theApp.NAND_ReadBlock(i, pbVerify, 1);
				} while (memcmp(pbVerify, pbDest, g_dwPageCount2*g_wPageSize2) && cboxNandVerify.GetCheck());

				//theApp.NAND_ReadBlock(1, addresscycles, g_wPageSize2, g_dwPageCount2, i*g_dwPageCount2, pbDest, 1);
				timer = GetTickCount() - timer;
				speed = (g_dwPageCount2*g_wPageSize2) / (double) timer;
				totalspeed += speed;
				vProgress.SetPos(i+1);
				text.Format("Dumping NAND 2 (%i%% / Currently: %.2f kB/s / Average: %.2f kB/s)", (int) ((((double)(i+1))/(double)g_dwBlockCount2)*100), speed, totalspeed/(i+1));
				edtProgress.SetWindowTextA(text);
				edtProgress.RedrawWindow();
				//if((i%4)==0)
					//RedrawWindow();
				
				temp.Write(pbDest, g_dwPageCount2*g_wPageSize2);

			}
			temp.Close();
			free(pbDest);
		}
	}


	theApp.RemoveDevice();

//	AfxBeginThread(CFlasherDlg::DumpNAND,NULL,THREAD_PRIORITY_NORMAL,0,0,NULL);

}

void CFlasherDlg::OnBnClickedNandflash()
{
	CHAR result = theApp.CreateDevice();
	if(!result)
	{
		MessageBox("Device not found!");
		return;
	}
	

	DWORD dwProgress = 0;
	double totalspeed = 0;
	DWORD timer;
	double speed;
	CHAR addresscycles = 5;


	CString temp = "";
	CString csBadBlocks = "";

		//AfxBeginThread(WorkerThreadProc,NULL,THREAD_PRIORITY_NORMAL,0,0,NULL);
		
	CString text;
	PCHAR pbSrc, pbDest;

	if(g_bNAND1)
	{
		CString path = ".bin"; //FD.GetPathName();
		if(g_bNAND2)
			path += ".0";
		CFileDialog FD(TRUE, 0, "dump"+path , 4|2, "*"+path, 0, 0, 1);
		if(FD.DoModal())
		{
			g_wPageSize1 = cboxBigBlock1.GetCheck()?2048:512;
			g_wPageSize1 += cboxRaw1.GetCheck()?(g_wPageSize1>>5):0;
			cbBlockSize1.GetLBText(cbBlockSize1.GetCurSel(), temp);
			g_dwPageCount1 = atoi(temp.GetString());
			cbBlockCount1.GetLBText(cbBlockCount1.GetCurSel(), temp);
			g_dwBlockCount1 = atoi(temp.GetString());

			theApp.NAND_Configure(g_wPageSize1, g_dwPageCount1, g_dwBlockCount1, cboxBigBlock1.GetCheck()?1:0, 0);
			vProgress.SetRange(1, (short) g_dwBlockCount1);
			vProgress.SetStep(1);

			path = FD.GetPathName();
			CFile temp(path, CFile::modeRead);
			temp.SeekToBegin();
			RedrawWindow();
			pbDest = (PCHAR) malloc(g_dwPageCount1*g_wPageSize1);
			pbSrc = (PCHAR) malloc(g_dwPageCount1*g_wPageSize1);
			
			PCHAR pbVerify = (PCHAR) malloc(g_dwPageCount1*g_wPageSize1);

			csBadBlocks = "";

			for(DWORD i=0; i < g_dwBlockCount1; i++)
			{
				memset(pbVerify, 0xFF, g_dwPageCount1*g_wPageSize1);
				temp.Read(pbDest, g_dwPageCount1*g_wPageSize1);
				timer = GetTickCount();
				if(cboxDiff1.GetCheck())
					theApp.NAND_ReadBlock(i, pbSrc, 0);
				else
					memset(pbSrc, 0, g_dwPageCount1*g_wPageSize1);

				if(memcmp(pbSrc, pbDest, g_dwPageCount1*g_wPageSize1))
				{
					for(DWORD j=0; j < g_dwPageCount1; j++)
						if(memcmp(pbSrc+(g_wPageSize1*j), pbVerify, g_wPageSize1))
						{
							if(theApp.NAND_EraseBlock(i, 0) & 0x1)
							{
								text.Format("Bad Block on NAND 1 (Erase failed): %i \r\n", i);
								theApp.Debug_Write(text);
							}
							j=g_dwPageCount1;
						}
				}

				if(theApp.NAND_ProgramBlock(i, pbDest, 0) & 0x1)
				{
					text.Format("Bad Block on NAND 1 (Program failed): %i \r\n", i);
					theApp.Debug_Write(text);
				}
				else 
				{
					if(cboxNandVerify.GetCheck())
					{
							theApp.NAND_ReadBlock(i, pbVerify, 0);
							if(memcmp(pbVerify, pbDest, g_dwPageCount1*g_wPageSize1))
								MessageBox("Error in programming Block %i on NAND 1...", "Error", MB_ICONWARNING);
							else
							{
								text.Format("Block matched on NAND 1: %i\r\n", i);
								theApp.Debug_Write(text);
							}
					}
				}


				timer = GetTickCount() - timer;
				speed = (g_dwPageCount1*g_wPageSize1) / (double) timer;
				totalspeed += speed;
				vProgress.SetPos(i+1);
				text.Format("Flashing NAND 1 (%i%% / Currently: %.2f kB/s / Average: %.2f kB/s); ", (int) ((((double)(i+1))/(double)g_dwBlockCount1)*100), speed, totalspeed/(i+1));
				edtProgress.SetWindowTextA(text);
				edtProgress.RedrawWindow();
				RedrawWindow();
			}
			temp.Close();
			free(pbDest);
			free(pbSrc);
			free(pbVerify);
		}
	}

	if(g_bNAND2)
	{
		CString path = ".bin"; //FD.GetPathName();
		if(g_bNAND1)
			path += ".1";
		CFileDialog FD(TRUE, 0, "dump"+path , 4|2, "*"+path, 0, 0, 1);
		if(FD.DoModal())
		{
			g_wPageSize2 = cboxBigBlock2.GetCheck()?2048:512;
			g_wPageSize2 += cboxRaw2.GetCheck()?(g_wPageSize2>>5):0;
			cbBlockSize2.GetLBText(cbBlockSize2.GetCurSel(), temp);
			g_dwPageCount2 = atoi(temp.GetString());
			cbBlockCount2.GetLBText(cbBlockCount2.GetCurSel(), temp);
			g_dwBlockCount2 = atoi(temp.GetString());

			theApp.NAND_Configure(g_wPageSize2, g_dwPageCount2, g_dwBlockCount2, cboxBigBlock2.GetCheck()?1:0, 1);
			vProgress.SetRange(1, (short) g_dwBlockCount2);
			vProgress.SetStep(1);

			path = FD.GetPathName();
			CFile temp(path, CFile::modeRead);
			temp.SeekToBegin();
			RedrawWindow();
			pbDest = (PCHAR) malloc(g_dwPageCount2*g_wPageSize2);
			pbSrc = (PCHAR) malloc(g_dwPageCount2*g_wPageSize2);
			
			PCHAR pbVerify = (PCHAR) malloc(g_dwPageCount2*g_wPageSize2);

			csBadBlocks = "";

			for(DWORD i=0; i < g_dwBlockCount2; i++)
			{
				memset(pbVerify, 0xFF, g_dwPageCount2*g_wPageSize2);
				temp.Read(pbDest, g_dwPageCount2*g_wPageSize2);
				timer = GetTickCount();
				if(cboxDiff2.GetCheck())
					theApp.NAND_ReadBlock(i, pbSrc, 1);
				else
					memset(pbSrc, 0, g_dwPageCount2*g_wPageSize2);

				if(memcmp(pbSrc, pbDest, g_dwPageCount2*g_wPageSize2))
				{
					for(DWORD j=0; j < g_dwPageCount2; j++)
						if(memcmp(pbSrc+(g_wPageSize2*j), pbVerify, g_wPageSize2))
						{
							if(theApp.NAND_EraseBlock(i,1) & 0x1)
							{
								text.Format("Bad Block on NAND 2 (Erase failed): %i \r\n", i);
								theApp.Debug_Write(text);
							}
							j=g_dwPageCount2;
						}
				}

				if(theApp.NAND_ProgramBlock(i, pbDest, 1) & 0x1)
				{
					text.Format("Bad Block on NAND 2 (Program failed): %i \r\n", i);
					theApp.Debug_Write(text);
				}
				else 
				{
					if(cboxNandVerify.GetCheck())
					{
							theApp.NAND_ReadBlock(i, pbVerify, 1);
							if(memcmp(pbVerify, pbDest, g_dwPageCount2*g_wPageSize2))
								MessageBox("Error in programming Block %i on NAND 2...", "Error", MB_ICONWARNING);
							else
							{
								text.Format("Block matched on NAND 2: %i\r\n", i);
								theApp.Debug_Write(text);
							}
					}
				}


				timer = GetTickCount() - timer;
				speed = (g_dwPageCount2*g_wPageSize2) / (double) timer;
				totalspeed += speed;
				vProgress.SetPos(i+1);
				text.Format("Flashing NAND 2 (%i%% / Currently: %.2f kB/s / Average: %.2f kB/s); ", (int) ((((double)(i+1))/(double)g_dwBlockCount2)*100), speed, totalspeed/(i+1));
				edtProgress.SetWindowTextA(text);
				edtProgress.RedrawWindow();
				RedrawWindow();
			}
			temp.Close();
			free(pbDest);
			free(pbSrc);
			free(pbVerify);
		}
	}


	theApp.RemoveDevice();
}

void CFlasherDlg::OnBnClickedCboxnand1()
{
	RedrawWindow();
	CHAR result = theApp.CreateDevice();
	if(!result)
	{
		MessageBox("Device not found!");
		//OnCancel();
		return;
	}

//	theApp.SetConfig(10, 1, 0, 0, 0);
//	theApp.SetDirections(0xFFFF);
//	theApp.SetOutputs(0);
//	theApp.TxStart();
	//PCHAR blabla = (PCHAR) malloc(16*1024);
	//memset(blabla, 0, 16*1024);
//	theApp.SetConfig(10, 0, 0, 1, 0);
	
//	MessageBox("Switch PS3 on now.");

	//for(int i=0; i<1000; i++)
	//{
	//	theApp.Write(8*1024, blabla);
	//	theApp.TxStart();
	//}



	DWORD temp = 0;

	PCHAR ident = (PCHAR) malloc(5);
	CString csTemp;
	if(cboxNAND1.GetCheck())
	{
		g_bNAND1 = 1;
//		cbPageSize1.EnableWindow(1);
		cbBlockSize1.EnableWindow(1);
		cbBlockCount1.EnableWindow(1);
		theApp.NAND_ReadID(ident, 5, 0);
//		if(!memcmp(ident, ident+2, 2))
//		{
//			memset(ident+2, 0, 3);
//			cboxBigBlock1.EnableWindow(0);
//		}
//		else
//		{
//			cboxBigBlock1.EnableWindow(1);
//			cboxBigBlock1.SetCheck(1);
//			temp = ident[3];
//			temp >>= 4;
//			temp &= 0x11;
//			cbBlockSize1.SetCurSel(2 + (int)temp);
//		}


		csTemp.Format("ID: %.02X %.02X %.02X %.02X %.02X", ident[0] & 0xFF, ident[1] & 0xFF, ident[2]&0xFF, ident[3]&0xFF, ident[4]&0xFF);
		txtIdent1.SetWindowTextA(csTemp);
	}
	else
	{
		g_bNAND1 = 0;
//		cbPageSize1.EnableWindow(0);
		cbBlockSize1.EnableWindow(0);
		cbBlockCount1.EnableWindow(0);
	}
	
	if(cboxNAND2.GetCheck())
	{
		g_bNAND2 = 1;
//		cbPageSize2.EnableWindow(1);
		cbBlockSize2.EnableWindow(1);
		cbBlockCount2.EnableWindow(1);
		//txtIdent2.SetWindowTextA();
		theApp.NAND_ReadID(ident, 5, 1);
		//if(!memcmp(ident, ident+2, 2))
		//{
		//	memset(ident+2, 0, 3);
		//	cboxBigBlock2.EnableWindow(0);
		//}
		//else
		//{
		//	cboxBigBlock2.EnableWindow(1);
		//	cboxBigBlock2.SetCheck(1);
		//	temp = ident[3];
		//	temp >>= 4;
		//	temp &= 0x11;
		//	cbBlockSize2.SetCurSel(2 + (int)temp);
		//}

		csTemp.Format("ID: %.02X %.02X %.02X %.02X %.02X", ident[0]&0xFF, ident[1]&0xFF, ident[2]&0xFF, ident[3]&0xFF, ident[4]&0xFF);
		txtIdent2.SetWindowTextA(csTemp);
	}
	else
	{
		g_bNAND2 = 0;
//		cbPageSize2.EnableWindow(0);
		cbBlockSize2.EnableWindow(0);
		cbBlockCount2.EnableWindow(0);
	}

	free(ident);
	theApp.RemoveDevice();
}

void CFlasherDlg::OnBnClickedCboxnand2()
{
	OnBnClickedCboxnand1();
	// TODO: Add your control notification handler code here
}

void CFlasherDlg::OnBnClickedTestshorts()
{
	CHAR result = theApp.CreateDevice();
	if(!result){
		MessageBox("Device not found! Please check all leads on U2 and leads 50 through 75 on U1.", "Shorts information", MB_ICONERROR);
		return;
	}
	result = theApp.Debug_TestShorts();
	theApp.RemoveDevice();

	if(result == 0)
		MessageBox("No shorts found! Device is properly working.", "Shorts information", MB_ICONINFORMATION);
	else if(result == 4)
		MessageBox("Shorts found! Please check D0..D15. (Leads between 76 and 100 on U1; Top side of U1)", "Shorts information", MB_ICONWARNING);
	else if(result == 3)
		MessageBox("Shorts found! Please check GP0..GP6. (Leads between 51 and 75 on U1; Right-hand side of U1)", "Shorts information", MB_ICONWARNING);
	else
		MessageBox("Shorts found! Please check GP7..GP15. (Leads between 1 and 25 on U1; Left-hand side of U1)", "Shorts information", MB_ICONWARNING);
}

void CFlasherDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialog::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	RedrawWindow();
}

void CFlasherDlg::OnCbnSelchangeNorsectorcount()
{
}

//void CFlasherDlg::OnCbnDropdownNorsectorcount()
//{
//	OnCbnSelchangeNorsectorcount();
//}

void CFlasherDlg::OnCbnSelendokNorsectorcount()
{
	CString temp = "";
	cbSectorCount.GetLBText(cbSectorCount.GetCurSel(), temp);
	int dwSectorCount = atoi(temp.GetString());
	cbNorStartSector.SetCurSel(0);
	cbNorEndSector.SetCurSel(dwSectorCount-1);

	// TODO: Add your control notification handler code here
}
