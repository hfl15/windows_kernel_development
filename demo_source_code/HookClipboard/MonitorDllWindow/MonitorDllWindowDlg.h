
// MonitorDllWindowDlg.h : 头文件
//

#pragma once


// CMonitorDllWindowDlg 对话框
class CMonitorDllWindowDlg : public CDialogEx
{
// 构造
public:
	CMonitorDllWindowDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MONITORDLLWINDOW_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	CListCtrl m_hookList;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnLogHook(WPARAM wParam,LPARAM lParam);	// 消息处理函数
	afx_msg void OnBnClickedButtonLoad();
	afx_msg void OnBnClickedButtonUnload();
	afx_msg void OnBnClickedButtonMsga();
	afx_msg void OnBnClickedButtonMsgw();
};
