
// hookWindowDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "hookWindow.h"
#include "hookWindowDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_MOUSEMSG WM_USER + 106

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// ChookWindowDlg 对话框




ChookWindowDlg::ChookWindowDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(ChookWindowDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void ChookWindowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX,IDC_LIST,m_List);
}

BEGIN_MESSAGE_MAP(ChookWindowDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &ChookWindowDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_HOOK, &ChookWindowDlg::OnBnClickedButtonHook)
	ON_BN_CLICKED(IDC_CHECK_MOVE, &ChookWindowDlg::OnBnClickedCheckMove)
	ON_BN_CLICKED(IDC_CHECK_WHEEL, &ChookWindowDlg::OnBnClickedCheckWheel)

	ON_MESSAGE(WM_MOUSEMSG,&ChookWindowDlg::OnMouseMsg) //消息映射

END_MESSAGE_MAP()


// ChookWindowDlg 消息处理程序

BOOL ChookWindowDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_List.InsertColumn(0,_T("wParam"));
	m_List.InsertColumn(1,_T("lParam"));
	m_List.InsertColumn(2,_T("鼠标消息"));
	m_List.InsertColumn(3,_T("鼠标坐标"));
	m_List.InsertColumn(4,_T("所在窗口"));
	m_List.SetColumnWidth(0,100);
	m_List.SetColumnWidth(1,100);
	m_List.SetColumnWidth(2,150);
	m_List.SetColumnWidth(3,100);
	m_List.SetColumnWidth(4,250);
	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// 得到复选框状态信息
	CButton* pbton = (CButton*)GetDlgItem(IDC_CHECK_MOVE);
	pbton->SetCheck(TRUE);
	pbton = (CButton*)GetDlgItem(IDC_CHECK_WHEEL);
	pbton->SetCheck(TRUE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void ChookWindowDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void ChookWindowDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR ChookWindowDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




HINSTANCE g_hInstanceDll = NULL;
//
//	启动鼠标钩子
//
void ChookWindowDlg::OnBnClickedButtonStart()
{
	// TODO: 在此添加控件通知处理程序代码
	g_hInstanceDll = LoadLibrary(_T("hookDll.dll"));
	if (NULL == g_hInstanceDll)
	{
		AfxMessageBox(_T("加载hookDll.dll失败"));
		return;
	}
	typedef BOOL (CALLBACK *StartHookMouse)(HWND hwnd);
	StartHookMouse startHook;
	startHook = (StartHookMouse) ::GetProcAddress(g_hInstanceDll,"StartHookMouse");
	if ( NULL == startHook )
	{
		AfxMessageBox(_T("获取 StartHookMouse 函数失败"));
		return;
	}

	if (startHook(this->m_hWnd))
	{
		m_List.InsertItem(m_List.GetItemCount(),_T("0"));
		m_List.SetItemText(m_List.GetItemCount()-1,1,_T("0"));
		m_List.SetItemText(m_List.GetItemCount()-1,2,_T("启动鼠标钩子成功"));
	}
	else
	{
		m_List.InsertItem(m_List.GetItemCount(),_T("0"));
		m_List.SetItemText(m_List.GetItemCount()-1,1,_T("0"));
		m_List.SetItemText(m_List.GetItemCount()-1,2,_T("启动鼠标钩子失败"));
	}
	
}

//
// 停止鼠标钩子HOOK
//
void ChookWindowDlg::OnBnClickedButtonHook()
{
	// TODO: 在此添加控件通知处理程序代码
	typedef VOID (CALLBACK *StopHookMouse)();
	StopHookMouse stopHook;
	g_hInstanceDll = LoadLibrary(_T("hookDll.dll"));
	if ( g_hInstanceDll == NULL)
	{
		AfxMessageBox(_T("加载DLL失败"));
		return;
	}

	stopHook = (StopHookMouse) ::GetProcAddress(g_hInstanceDll,"StopHookMouse");
	if (stopHook == NULL)
	{
		m_List.InsertItem(m_List.GetItemCount(),_T("0"));
		m_List.SetItemText(m_List.GetItemCount()-1,1,_T("0"));
		m_List.SetItemText(m_List.GetItemCount()-1,2,_T("获取函数 StopHookMouse 失败"));
		return;
	}
	else
	{
		stopHook();
		m_List.InsertItem(m_List.GetItemCount(),_T("0"));
		m_List.SetItemText(m_List.GetItemCount()-1,1,_T("0"));
		m_List.SetItemText(m_List.GetItemCount()-1,2,_T("停止HOOKMOUSE成功"));
	}

	if (g_hInstanceDll != NULL)
	{
		::FreeLibrary(g_hInstanceDll);
	}

	// 确保list control 最后一行可见
	m_List.EnsureVisible(m_List.GetItemCount()-1,FALSE);

}

// 消息相应函数
LRESULT ChookWindowDlg::OnMouseMsg(WPARAM wParam,LPARAM lParam)
{
	static CString str;
	static PMSLLHOOKSTRUCT mouseHookStruct;

	// 鼠标参数
	str.Format(_T("0x%X"),wParam);
	m_List.InsertItem(m_List.GetItemCount(),str);
	str.Format(_T("0x%X"),lParam);
	m_List.SetItemText(m_List.GetItemCount()-1,1,str);

	if (wParam == WM_LBUTTONDBLCLK)	// 鼠标左键按下
	{
		str = _T("WM_LBUTTONDBLCLK");
		m_List.SetItemText(m_List.GetItemCount()-1,2,str);
	}
	else if (wParam == WM_LBUTTONUP) // 鼠标左键弹起
	{
		str = _T("WM_LOBUTTONUP");
		m_List.SetItemText(m_List.GetItemCount()-1,2,str);
	}
	else if ( wParam == WM_RBUTTONDBLCLK) // 鼠标右键按下
	{
		str = _T("WM_RBUTTONDELCLK");
		m_List.SetItemText(m_List.GetItemCount()-1,2,str);
	}
	else if ( wParam == WM_RBUTTONUP) // 鼠标右键弹起
	{
		str = _T("WM_RBUTTONUP");
		m_List.SetItemText(m_List.GetItemCount()-1,2,str);
	}
	else if ( wParam == WM_MOUSEMOVE) // 鼠标移动
	{
		if ( !m_bIsShowMouseMoveMsg && m_List.GetItemCount()>0 )
		{
			m_List.DeleteItem(m_List.GetItemCount()-1);
			return 1;
		}
		
		str = _T("WM_MOUSEMOVE");
		m_List.SetItemText(m_List.GetItemCount()-1,2,str);
	}
	else if ( wParam == WM_MOUSEWHEEL) // 鼠标滚动
	{
		if ( !m_bIsSHowMouseWheelMsg && m_List.GetItemCount()>0)
		{
			m_List.DeleteItem(m_List.GetItemCount()-1);
			return 1;
		}
		str = _T("WM_MOUSEWHEEL");
		m_List.SetItemText(m_List.GetItemCount()-1,2,str);
	}

	// 鼠标位置
	mouseHookStruct = (PMSLLHOOKSTRUCT)lParam;
	str.Format(_T("%d,%d"),mouseHookStruct->pt.x,mouseHookStruct->pt.y);
	m_List.SetItemText(m_List.GetItemCount()-1,3,str);

	// 所在窗口
	HWND hwnd = ::WindowFromPoint(mouseHookStruct->pt);
	TCHAR title[MAX_PATH];
	::GetWindowText(hwnd,title,MAX_PATH);
	m_List.SetItemText(m_List.GetItemCount()-1,4,title);

	m_List.EnsureVisible(m_List.GetItemCount()-1,FALSE);

	return 1;
}

//	不显示鼠标移动消息
void ChookWindowDlg::OnBnClickedCheckMove()
{
	// TODO: 在此添加控件通知处理程序代码

	if (BST_CHECKED == IsDlgButtonChecked(IDC_CHECK_MOVE))
	{
		m_bIsShowMouseMoveMsg = FALSE;
	}
	else
	{
		m_bIsShowMouseMoveMsg = TRUE;
	}
}

//	不显示鼠标滚动轮消息
void ChookWindowDlg::OnBnClickedCheckWheel()
{
	// TODO: 在此添加控件通知处理程序代码

	if (BST_CHECKED == IsDlgButtonChecked(IDC_CHECK_WHEEL))
	{
		m_bIsSHowMouseWheelMsg = FALSE;
	}
	else
	{
		m_bIsSHowMouseWheelMsg = TRUE;
	}
}
