
// HookWindowDlg.h : 头文件
//

#pragma once


// CHookWindowDlg 对话框
class CHookWindowDlg : public CDialogEx
{
// 构造
public:
	CHookWindowDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_HOOKWINDOW_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	CListCtrl m_list; // Hook 窗口列表

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnLogHookWindows(WPARAM wParam,LPARAM lParam); // 消息相应函数
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonMsga();
	afx_msg void OnBnClickedButtonMsgw();
};
