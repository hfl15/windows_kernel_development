
// HookMessageboxWindowDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HookMessageboxWindow.h"
#include "HookMessageboxWindowDlg.h"
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


// CHookMessageboxWindowDlg 对话框




CHookMessageboxWindowDlg::CHookMessageboxWindowDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHookMessageboxWindowDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CHookMessageboxWindowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX,IDC_STATIC_STATUS,m_status);
}

BEGIN_MESSAGE_MAP(CHookMessageboxWindowDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &CHookMessageboxWindowDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CHookMessageboxWindowDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_CALL, &CHookMessageboxWindowDlg::OnBnClickedButtonCall)
END_MESSAGE_MAP()


// CHookMessageboxWindowDlg 消息处理程序

BOOL CHookMessageboxWindowDlg::OnInitDialog()
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

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CHookMessageboxWindowDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CHookMessageboxWindowDlg::OnPaint()
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
HCURSOR CHookMessageboxWindowDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
//	定义API类型
#define CODE_LENGTH 5				//	入口指令长度
typedef int (WINAPI *TypeMessageBoxW)(HWND hwnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType);
TypeMessageBoxW OdlMsgBoxW = NULL;	// 指向函数原型指针
FARPROC pfOldMsgBoxW;				// 指向函数远指针
BYTE OldCode[CODE_LENGTH];			// 原系统API入口
BYTE NewCode[CODE_LENGTH];			// 自己实现的API的入口，（jmp xxxx），xxxx为新API入口地址

HANDLE hProcess = NULL;				// 本程序进程句柄
HINSTANCE hInst = NULL;				// API所在的dll文件句柄

VOID HookOn();			//	开始HOOK
VOID HookOff();			//	停止HOOK
VOID GetApiEntrancy();	//	获取API入口地址
bool AdjustPrivileges();//	提高权限

//
// 自己定义的，用于替换相应API的，假的API
//
int WINAPI MyMessageBoxW(HWND hwnd,LPCWSTR lpText,LPCWSTR lpCation,UINT uType)
{
	TRACE(lpText);
	
	/*
		调用原函数之前，先停止HOOK，也就是恢复原系统API函数的入口，
		否则无法调用到原API函数，而是继续调用自己的API，会造成死
		循环，进而造成堆栈溢出，崩溃。
	*/
	HookOff();	

	/*
		调用原来的MessageBoxW打印我们的信息。
	*/
	int ret = MessageBoxW(hwnd,_T("哈哈，被HOOK咯！！"),lpCation,uType);

	/*
		调用完原系统API后，记得恢复HOOK，也就是启动HOOK，将原API函数入口换成我们自己定义的函数入口，
		否则下一次调用MessageBoxW的时候就无法转到我们自己定义的API函数中，也就无法实现HOOK。
	*/
	HookOff();

	return ret;
}


/*
	启动HOOK，将原API的入口地址换成我们自己定义函数的入口地址
*/
VOID HookOn()
{
	//
	//	确保本程序进程句柄hProcess不为NULL
	//
	ASSERT(hProcess!=NULL);

	DWORD dwTemp;
	DWORD dwOldProtect;
	SIZE_T writedByte;

	//
	// 修改API入口的前5个字节，jmp xxxx
	//
	VirtualProtectEx(hProcess,pfOldMsgBoxW,CODE_LENGTH,PAGE_READWRITE,&dwOldProtect);
	WriteProcessMemory(hProcess,pfOldMsgBoxW,NewCode,CODE_LENGTH,&writedByte);
	if (writedByte == 0)
	{
		AfxMessageBox(_T("替换原API地址失败"));
	}
	VirtualProtectEx(hProcess,pfOldMsgBoxW,CODE_LENGTH,dwOldProtect,&dwTemp);
}

/*
	定制HOOK，将入口换成原来的API入口地址
*/
VOID HookOff()
{
	ASSERT(hProcess != NULL);

	DWORD dwTemp;
	DWORD dwOldProtect;

	SIZE_T wirtedByte;

	//	回复原API地址
	VirtualProtectEx(hProcess,pfOldMsgBoxW,CODE_LENGTH,PAGE_READWRITE,&dwOldProtect);
	WriteProcessMemory(hProcess,pfOldMsgBoxW,OldCode,CODE_LENGTH,&wirtedByte);
	if (wirtedByte == 0)
	{
		AfxMessageBox(_T("回复原API地址失败"));
	}
	VirtualProtectEx(hProcess,pfOldMsgBoxW,CODE_LENGTH,dwOldProtect,&dwTemp);
}

/*
	保存原API和新API的地址
*/
VOID GetApiEntrancy()
{
	//
	// 保存原来API地址
	//
	HMODULE hmod = LoadLibrary(_T("User32.dll"));
	if ( NULL == hmod)
	{
		AfxMessageBox(_T("加载User32.dll失败"));
		return;
	}
	OdlMsgBoxW = (TypeMessageBoxW)::GetProcAddress(hmod,"MessageBoxW");
	pfOldMsgBoxW = (FARPROC)OdlMsgBoxW;
	if ( pfOldMsgBoxW == NULL)
	{
		AfxMessageBox(_T("获取原API入口地址出错"));
		return;
	}

	//
	//	将原API的入口5个字节代码保存到OdeCode[]中
	//
	_asm
	{
			lea edi,OldCode			// 取数组OldCode[]地址，存放到edi中
			mov esi,pfOldMsgBoxW	// 获取原API入口地址，存入esi中
			cld						// 设置方向
			movsd					// 移动dword ，4 Byte
			movsb					// 移动 1 Byte
	}

	//
	// 新的API入口保存到NewCode[]中，即jmp xxxx，xxxx为新API地址，该指令总长度为5个字节
	//
	NewCode[0] = 0xe9; //	0xe9相当于jmp指令

	_asm
	{
		lea eax,MyMessageBoxW
		mov ebx,pfOldMsgBoxW
		sub eax,ebx
		sub eax,CODE_LENGTH
		mov dword ptr[NewCode+1],eax
	}

	//
	// 填充完毕，开始HOOK，即使用NewCode[]替换原API入口
	//
	HookOn();
}

bool AdjustPrivileges() {
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	TOKEN_PRIVILEGES oldtp;
	DWORD dwSize=sizeof(TOKEN_PRIVILEGES);
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		if (GetLastError()==ERROR_CALL_NOT_IMPLEMENTED) return true;
		else return false;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
		CloseHandle(hToken);
		return false;
	}
	ZeroMemory(&tp, sizeof(tp));
	tp.PrivilegeCount=1;
	tp.Privileges[0].Luid=luid;
	tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	/* Adjust Token Privileges */
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize)) {
		CloseHandle(hToken);
		return false;
	}
	// close handles
	CloseHandle(hToken);
	return true;
}

//
//	启动 HookMessageBoxW
//
void CHookMessageboxWindowDlg::OnBnClickedButtonStart()
{
	// TODO: 在此添加控件通知处理程序代码

	AdjustPrivileges();		//	提升权限，因为调用 OpenProcess() 需要合适的权限
	DWORD dwPid = ::GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_ALL_ACCESS,0,dwPid);
	if (hProcess == NULL)
	{
		CString logInfo;
		logInfo.Format(_T("获取进程句柄失败！！,进程 id = 0x%x ,错误代码 = 0x%x"),dwPid,GetLastError());
		AfxMessageBox(logInfo);
		return;
	}

	
	GetApiEntrancy();	//	获取新旧API入口，并开始HOOK

	m_status.SetWindowText(_T("Hook已启动"));
}

//
//	终止 HookMessageBoxW
//
void CHookMessageboxWindowDlg::OnBnClickedButtonStop()
{
	// TODO: 在此添加控件通知处理程序代码

	HookOff();
	m_status.SetWindowText(_T("Hook已停止"));
}

//
//	调用 HookMessageBoxW
//
void CHookMessageboxWindowDlg::OnBnClickedButtonCall()
{
	// TODO: 在此添加控件通知处理程序代码
	::MessageBoxW(m_hWnd,_T("这是正常的MessageBoxW"),_T("Hello"),0);
}
