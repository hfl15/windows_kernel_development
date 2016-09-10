
// MyWindowDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MyWindow.h"
#include "MyWindowDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CMyWindowDlg 对话框




CMyWindowDlg::CMyWindowDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMyWindowDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMyWindowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMyWindowDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CMyWindowDlg 消息处理程序

BOOL CMyWindowDlg::OnInitDialog()
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

	HookLoad();	//	加载HOOK

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}


void CMyWindowDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	HookUnload();	// 退出窗口，要卸载HOOK
	CDialogEx::OnClose();
}

void CMyWindowDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMyWindowDlg::OnPaint()
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
HCURSOR CMyWindowDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMyWindowDlg::HookLoad()
{
	m_hinstHookDll = ::LoadLibrary(_T("C:\\testProject\\MonitorDll.dll"));
	CString loginfo;
	if ( NULL == m_hinstHookDll)
	{
		loginfo.Format(_T("加载 MonitorDll.dll失败，错误代码 = [%d] "),GetLastError());
		AfxMessageBox(loginfo);
		return;
	}


	typedef BOOL (WINAPI* LoadMonitor)(HWND hwnd,DWORD dwProcessId);
	LoadMonitor loadMonitor = NULL;
	loadMonitor = (LoadMonitor)::GetProcAddress(m_hinstHookDll,"HookLoad");
	if (NULL == loadMonitor)
	{
		loginfo.Format(_T("获取函数 HookLoad 失败，错误代码 = [%d]"),GetLastError());
		AfxMessageBox(loginfo);
	}
	if (loadMonitor(m_hWnd,GetCurrentProcessId()))
	{
		loginfo.Format(_T("HOOK加载成功"));
		AfxMessageBox(loginfo);
	}
	else
	{
		loginfo.Format(_T("HOOK加载失败"));
		AfxMessageBox(loginfo);
	}
}

/*
	卸载HOOKDLL
*/

void CMyWindowDlg::HookUnload()
{
	CString logInfo;
	if (m_hinstHookDll == NULL)
	{
		m_hinstHookDll = LoadLibrary(_T("MonitorDll.dll"));
		if ( NULL == m_hinstHookDll)
		{
			logInfo.Format(_T("加载 MonitorDll.dll失败，错误代码 = [%d]"),GetLastError());
			AfxMessageBox(logInfo);
			return;
		}

	}

	typedef VOID (WINAPI* UnloadHook)();
	UnloadHook unloadHook = NULL;
	unloadHook = (UnloadHook)::GetProcAddress(m_hinstHookDll,"HookUnload");
	if (NULL == unloadHook)
	{
		logInfo.Format(_T("获取函数 HookUnload 失败，错误代码 = [%d]"),GetLastError());
		AfxMessageBox(logInfo);
		return;
	}

	unloadHook();
}


