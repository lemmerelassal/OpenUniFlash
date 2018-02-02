/*
OpenUniFlash - A universal NAND and NOR Flash programmer
Copyright (C) 2010-2018  Lemmer EL ASSAL, Axel GEMBE, Maël Blocteur

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
Also add information on how to contact you by electronic and paper mail.
*/

// FlasherDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// CFlasherDlg dialog
class CFlasherDlg : public CDialog
{
// Construction
public:
	CFlasherDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_FLASHER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
//	static UINT DumpNAND( LPVOID Param );
	CProgressCtrl vProgress;
	CButton btnDump;
	CButton btnFlash;
//	afx_msg void OnBnClickedButton1();
//	afx_msg void OnBnClickedButton2();
	CEdit edtProgress;
//	afx_msg void OnBnClickedButton3();
	CComboBox cbDevice1;
	CComboBox cbPageSize1;
	CComboBox cbBlockSize1;
	CComboBox cbBlockCount1;
//	afx_msg void OnCbnSelchangeCombo1();
//	afx_msg void OnBnClickedRadio1();
//	CButton cbDouble;
//	afx_msg void OnCbnSelchangeCombo2();
//	afx_msg void OnCbnSelchangeCombo3();
//	afx_msg void OnCbnSelchangeCombo4();
//	afx_msg void OnBnClickedNor();
//	CButton rbNor;
//	int rbNand;
//	afx_msg void OnBnClickedNand();
	CStatic gbNorSettings;
	CStatic gbNandSettings;
//	afx_msg void OnCbnSelchangeCbpagesize();
	CListBox lbPageSize;
//	afx_msg void OnCbnSelchangeCbblocksize();
//	afx_msg void OnCbnSelchangeCbblockcount();
//	afx_msg void OnBnClickedCheck1();
	CButton cboxNAND1;
	CButton cboxNAND2;
//	afx_msg void OnBnClickedCheck2();
	CComboBox cbDevice2;
	CComboBox cbPageSize2;
	CComboBox cbBlockSize2;
	CComboBox cbBlockCount2;
//	afx_msg void OnCbnSelchangeCbpagesize2();
//	afx_msg void OnCbnSelchangeCbblocksize2();
//	afx_msg void OnCbnSelchangeCbblockcount2();
//	afx_msg void OnBnClickedButton4();
	CStatic txtIdent1;
	CStatic txtIdent2;
	afx_msg void OnBnClickedCancel();
//	afx_msg void OnBnClickedRbdevice1();
//	afx_msg void OnBnClickedRadio4();
//	afx_msg void OnBnClickedRadio5();
//	CButton rbDevice1;
//	CButton rbDevice2;
//	afx_msg void OnNMThemeChangedRadio4(NMHDR *pNMHDR, LRESULT *pResult);
//	afx_msg void OnBnHotItemChangeRadio4(NMHDR *pNMHDR, LRESULT *pResult);
//	afx_msg void OnBnSetfocusRadio4();
	CComboBox cbNorSectorSize;
	CComboBox cbSectorCount;
	//afx_msg void OnBnClickedButton5();
//	afx_msg void OnBnClickedButton3();
//	afx_msg void OnBnClickedButton5();
//	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedNordump();
	afx_msg void OnBnClickedNorflash();
	afx_msg void OnBnClickedNanddump();
	afx_msg void OnBnClickedNandflash();
	afx_msg void OnBnClickedCboxnand1();
	afx_msg void OnBnClickedCboxnand2();
	afx_msg void OnBnClickedTestshorts();
	CButton cboxBigBlock1;
	CButton cboxRaw1;
	CButton cboxBigBlock2;
	CButton cboxRaw2;
	CButton cboxDiff1;
	CButton cboxDiff2;
	CButton cboxDiff3;
	CButton cboxNorVerify;
	CButton cboxNandVerify;
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	CButton cboxAlternate;
	CComboBox cbNorStartSector;
	CComboBox cbNorEndSector;
	afx_msg void OnCbnSelchangeNorsectorcount();
//	afx_msg void OnCbnDropdownNorsectorcount();
	afx_msg void OnCbnSelendokNorsectorcount();
	CButton cboxByteSwap;
};
