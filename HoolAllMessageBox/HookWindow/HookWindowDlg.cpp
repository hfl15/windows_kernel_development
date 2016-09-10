
// HookWindowDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HookWindow.h"
#include "HookWindowDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  WM_HOOKMSG WM_USER + 106	// 自己定义消息


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


// CHookWindowDlg 对话框




CHookWindowDlg::CHookWindowDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHookWindowDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CHookWindowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX,IDC_LIST_HOOKWINDOW,m_list);
}

BEGIN_MESSAGE_MAP(CHookWindowDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &CHookWindowDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CHookWindowDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_MSGA, &CHookWindowDlg::OnBnClickedButtonMsga)
	ON_BN_CLICKED(IDC_BUTTON_MSGW, &CHookWindowDlg::OnBnClickedButtonMsgw)

	ON_MESSAGE(WM_HOOKMSG,CHookWindowDlg::OnLogHookWindows) //消息映射

END_MESSAGE_MAP()


// CHookWindowDlg 消息处理程序

BOOL CHookWindowDlg::OnInitDialog()
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

	// 列表初始化
	m_list.InsertColumn(0,_T("Hook窗口"));
	m_list.SetColumnWidth(0,250);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CHookWindowDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CHookWindowDlg::OnPaint()
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
HCURSOR CHookWindowDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 消息相应函数
LRESULT CHookWindowDlg::OnLogHookWindows(WPARAM wParam,LPARAM lParam)
{
	static CString str;
	static PMSLLHOOKSTRUCT mouseHookStruct;

	if (wParam)
	{
		TCHAR* str = (TCHAR*)wParam;
		CString msg;
		msg.Format(_T("%s"),str);
		m_list.InsertItem(m_list.GetItemCount(),msg);
		m_list.EnsureVisible(m_list.GetItemCount()-1,FALSE);
	}

	if (lParam)
	{
		HWND hwnd = (HWND)lParam;
		TCHAR title[MAX_PATH];
		::GetWindowText(hwnd,title,MAX_PATH);
		m_list.InsertItem(m_list.GetItemCount(),title);
		m_list.EnsureVisible(m_list.GetItemCount()-1,FALSE);
	}
	

	return 1;
}


HINSTANCE g_hinstDll = NULL;
//
// 开始 HOOK
//
void CHookWindowDlg::OnBnClickedButtonStart()
{
	// TODO: 在此添加控件通知处理程序代码

	g_hinstDll = LoadLibrary(_T("HookDll.dll"));
	if ( NULL == g_hinstDll)
	{
		AfxMessageBox(_T("加载 HookDll.dll 失败"));
	}

	typedef BOOL (CALLBACK *HookStart)(HWND hwnd);
	HookStart hookStart = NULL;
	hookStart = (HookStart)::GetProcAddress(g_hinstDll,"StartHook");
	if ( NULL == hookStart)
	{
		AfxMessageBox(_T("获取 StartHook 函数失败"));
		return;
	}
	bool ret = hookStart(m_hWnd);
	if (ret)
	{
		m_list.InsertItem(m_list.GetItemCount(),_T("启动钩子成功"));
		m_list.EnsureVisible(m_list.GetItemCount()-1,FALSE);
	}
	else
	{
		m_list.InsertItem(m_list.GetItemCount(),_T("启动钩子失败"));
		m_list.EnsureVisible(m_list.GetItemCount()-1,FALSE);
	}
}

//
// 终止 HOOK
//
void CHookWindowDlg::OnBnClickedButtonStop()
{
	// TODO: 在此添加控件通知处理程序代码

	typedef BOOL (CALLBACK* HookStop)();
	HookStop hookStop = NULL;
	if (NULL ==  g_hinstDll) // 一定要加这个判断，若不为空的话就不需要在重新加载，否则会是不同的实例
	{
		g_hinstDll = LoadLibrary(_T("HookDll.dll"));
		if (g_hinstDll == NULL)
		{
			AfxMessageBox(_T("加载 HookDll.dll 失败"));
			return;
		}

	}
	
	hookStop = ::GetProcAddress(g_hinstDll,"StopHook");
	if (hookStop == NULL)
	{
		AfxMessageBox(_T("获取 StopHook 失败"));
		FreeLibrary(g_hinstDll);  
		g_hinstDll=NULL;  
		return;
	}

	hookStop();
	if (g_hinstDll!= NULL)
	{
		::FreeLibrary(g_hinstDll);
	}
	m_list.InsertItem(m_list.GetItemCount(),_T("终止HOOK成功"));

}

// MessageBoxA
void CHookWindowDlg::OnBnClickedButtonMsga()
{
	// TODO: 在此添加控件通知处理程序代码
	
	MessageBoxA(m_hWnd,"这是正常的MessageBoxA...","哈哈",0);
}

// MessageBoxW
void CHookWindowDlg::OnBnClickedButtonMsgw()
{
	// TODO: 在此添加控件通知处理程序代码
	MessageBoxW(_T("这是正常的MessageBoxW..."),_T("呵呵"),0);
}
