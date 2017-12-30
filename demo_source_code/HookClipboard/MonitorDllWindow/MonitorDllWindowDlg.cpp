
// MonitorDllWindowDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MonitorDllWindow.h"
#include "MonitorDllWindowDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  WM_HOOK_LOG WM_USER + 106	// 自己定义的消息处理


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


// CMonitorDllWindowDlg 对话框




CMonitorDllWindowDlg::CMonitorDllWindowDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMonitorDllWindowDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMonitorDllWindowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX,IDC_LIST_LOG,m_hookList);
}

BEGIN_MESSAGE_MAP(CMonitorDllWindowDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &CMonitorDllWindowDlg::OnBnClickedButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_UNLOAD, &CMonitorDllWindowDlg::OnBnClickedButtonUnload)

	ON_MESSAGE(WM_HOOK_LOG,&CMonitorDllWindowDlg::OnLogHook)	// 消息映射

	ON_BN_CLICKED(IDC_BUTTON_MSGA, &CMonitorDllWindowDlg::OnBnClickedButtonMsga)
	ON_BN_CLICKED(IDC_BUTTON_MSGW, &CMonitorDllWindowDlg::OnBnClickedButtonMsgw)
END_MESSAGE_MAP()


// CMonitorDllWindowDlg 消息处理程序

BOOL CMonitorDllWindowDlg::OnInitDialog()
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

	// 初始化 log list
	m_hookList.InsertColumn(0,_T("HOOK信息"));
	m_hookList.SetColumnWidth(0,250);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMonitorDllWindowDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMonitorDllWindowDlg::OnPaint()
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
HCURSOR CMonitorDllWindowDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


/*
	消息处理函数
*/

LRESULT CMonitorDllWindowDlg::OnLogHook(WPARAM wParam,LPARAM lParam)
{

	return 1;
}

HINSTANCE g_hinsDll = NULL;

/*
	加载HOOK
*/
void CMonitorDllWindowDlg::OnBnClickedButtonLoad()
{
	// TODO: 在此添加控件通知处理程序代码
	g_hinsDll = LoadLibrary(_T("MonitorDll.dll"));
	CString logInfo;
	if ( NULL == g_hinsDll )
	{
		logInfo.Format(_T("加载 MonitorDll.dll 失败，错误代码：%d"),GetLastError());
		m_hookList.InsertItem(m_hookList.GetItemCount(),logInfo);
		m_hookList.EnsureVisible(m_hookList.GetItemCount()-1,FALSE);
		return;
	}

	typedef BOOL (WINAPI* HookLoad)(HWND hwnd);
	HookLoad hookLoad = NULL;
	hookLoad = (HookLoad)::GetProcAddress(g_hinsDll,"HookLoad");
	if ( NULL == hookLoad)
	{
		m_hookList.InsertItem(m_hookList.GetItemCount(),_T("获取函数 HookLoad 失败"));
		m_hookList.EnsureVisible(m_hookList.GetItemCount()-1,FALSE);
		return;
	}

	BOOL ret =FALSE;
	ret = hookLoad(m_hWnd);
	if ( FALSE == ret )
	{
		m_hookList.InsertItem(m_hookList.GetItemCount(),_T("加载HOOK失败"));
		m_hookList.EnsureVisible(m_hookList.GetItemCount()-1,FALSE);
	}
	else
	{
		m_hookList.InsertItem(m_hookList.GetItemCount(),_T("加载HOOK成功"));
		m_hookList.EnsureVisible(m_hookList.GetItemCount()-1,FALSE);
	}

}

/*
	卸载HOOK
*/
void CMonitorDllWindowDlg::OnBnClickedButtonUnload()
{
	// TODO: 在此添加控件通知处理程序代码
	if (g_hinsDll != NULL)
	{
		g_hinsDll = LoadLibrary(_T("MonitorDll.dll"));
		if(NULL == g_hinsDll)
		{
			m_hookList.InsertItem(m_hookList.GetItemCount(),_T("加载 MonitorDll.dll 失败"));
			m_hookList.EnsureVisible(m_hookList.GetItemCount()-1,FALSE);
			return;
		}
	}

	typedef VOID (WINAPI* HookUnload)();
	HookUnload hookUnload = NULL;
	hookUnload = (HookUnload) ::GetProcAddress(g_hinsDll,"HookUnload");
	if (NULL == hookUnload)
	{
		m_hookList.InsertItem(m_hookList.GetItemCount(),_T("获取函数 HookUnload 失败"));
		m_hookList.EnsureVisible(m_hookList.GetItemCount()-1,FALSE);
		return;
	}

	hookUnload();
	FreeLibrary(g_hinsDll);

	m_hookList.InsertItem(m_hookList.GetItemCount(),_T("卸载HOOK成功"));
	m_hookList.EnsureVisible(m_hookList.GetItemCount()-1,FALSE);
}


void CMonitorDllWindowDlg::OnBnClickedButtonMsga()
{
	// TODO: 在此添加控件通知处理程序代码
	MessageBoxA(m_hWnd,"这是正常的MessageBoxA...","哈哈",0);
}


void CMonitorDllWindowDlg::OnBnClickedButtonMsgw()
{
	// TODO: 在此添加控件通知处理程序代码
	MessageBoxW(_T("这是正常的MessageBoxW..."),_T("呵呵"),0);
}
