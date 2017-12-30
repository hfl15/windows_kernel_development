
// hookWindowDlg.h : 头文件
//

#pragma once


// ChookWindowDlg 对话框
class ChookWindowDlg : public CDialogEx
{
// 构造
public:
	ChookWindowDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_HOOKWINDOW_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg LRESULT OnMouseMsg(WPARAM wParam,LPARAM lParam);	// 鼠标HOOL DLL 返回消息处理函数
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonHook();
	afx_msg void OnBnClickedCheckMove();
	afx_msg void OnBnClickedCheckWheel();

	CEdit m_ediInfo;
	CListCtrl m_List;
	bool m_bIsShowMouseMoveMsg;
	bool m_bIsSHowMouseWheelMsg;
};
