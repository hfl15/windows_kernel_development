// HookDll.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "HookDll.h"
#include <Windows.h>
#include <psapi.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/*
	 全局共享变量
*/
#pragma data_seg("Share")
HWND g_hWnd = NULL ;			// 主窗口句柄
HINSTANCE g_hInstance = NULL;	// 本dll实例句柄
HHOOK hhk = NULL;				// 鼠标钩子句柄
#pragma data_seg()
#pragma comment(linker,"/section:Share,rws")

HANDLE hProcess = NULL;				//	当前进程
BOOL bIsInJected = FALSE;			//	是否已注入标记
TCHAR* msgToMain = new TCHAR[200];	//	发给主调程序的信息


/*
	原函数定义
*/
typedef int (WINAPI *TypeMsgBoxA)(HWND hWnd,LPCSTR lpText, LPCSTR lpCaption,UINT uType);
typedef int (WINAPI *TypeMsgBoxW)(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType);

TypeMsgBoxA oldMsgBoxA = NULL;	// 用于保存原函数地址
TypeMsgBoxW oldMsgBoxW = NULL;	// 用于保存原楷书地址
FARPROC pfMsgBoxA = NULL;		// 指向原函数地址的远指针
FARPROC pfMsgBoxW = NULL;		// 指向原函数地址的远指针

#define CODE_LENGTH 5
BYTE oldCodeA[CODE_LENGTH];	// 保存原来API入口代码
BYTE oldCodeW[CODE_LENGTH];	// 保存原来API入口代码
BYTE newCodeA[CODE_LENGTH];	// 保存新API入口代码，jmp xxxx
BYTE newCodeW[CODE_LENGTH];	// 保存新API入口代码，jmp xxxx

/* 
	自己编写的API
*/
int WINAPI MyMessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCation,UINT uType);
int WINAPI MyMessageBoxW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCation,UINT uType);

/* 
	其它函数原型声明
*/
void HookOn();			//	开始HOOK
void HookOff();			//	关闭HOOK
void Inject();			//	注入
BOOL WINAPI StartHook(HWND hWnd);	// 加载钩子
BOOL WINAPI StopHook();				// 卸载钩子
bool AdjustPrivileges();			// 提升权限



//
//TODO: 如果此 DLL 相对于 MFC DLL 是动态链接的，
//		则从此 DLL 导出的任何调入
//		MFC 的函数必须将 AFX_MANAGE_STATE 宏添加到
//		该函数的最前面。
//
//		例如:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// 此处为普通函数体
//		}
//
//		此宏先于任何 MFC 调用
//		出现在每个函数中十分重要。这意味着
//		它必须作为函数中的第一个语句
//		出现，甚至先于所有对象变量声明，
//		这是因为它们的构造函数可能生成 MFC
//		DLL 调用。
//
//		有关其他详细信息，
//		请参阅 MFC 技术说明 33 和 58。
//

// CHookDllApp

BEGIN_MESSAGE_MAP(CHookDllApp, CWinApp)
END_MESSAGE_MAP()


// CHookDllApp 构造

CHookDllApp::CHookDllApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CHookDllApp 对象

CHookDllApp theApp;


// CHookDllApp 初始化
/*
	dll程序入口，当程序加载dll时，会执行InitInstance()  
*/
BOOL CHookDllApp::InitInstance()
{
	CWinApp::InitInstance();
	g_hInstance = AfxGetInstanceHandle();//	获取当前DLL实例句柄
	
	AdjustPrivileges();	//	提高权限
	DWORD dwPid = ::GetCurrentProcessId();
	hProcess = ::OpenProcess(PROCESS_ALL_ACCESS,0,dwPid);
/*	
	TCHAR* procName = new TCHAR[MAX_PATH];
	GetModuleFileName(NULL,procName,MAX_PATH);  // 这里可以取得进程名，那么就可以判断是否是机密进程啦。。
	CString info;
	info.Format(_T("进程id = %d , 进程名 %s"),dwPid,procName);
	AfxMessageBox(info);
*/	
	if (hProcess == NULL)
	{
		CString str;
		str.Format(_T("OpenProcess fail， and error code = %d"),GetLastError());
		AfxMessageBox(str);
		return FALSE;
	}

	Inject();	// 开始注入
	
	return TRUE;
}
int CHookDllApp::ExitInstance()
{
	/*
		dll退出时，一定要记得恢复原API的入口！！！
		我们编写的dll会被注入到所有目标进程中，若dll退出时，没有恢复原API入口，
		那么被挂钩的程序再次调用该API时，会发生错误。
		因为我们的dll程序已经退出，但原API的入口仍为我们所定义的API的入口，这
		时被挂钩的程序无法找到我们实现的API，然而原API的地址又没有被恢复，也就
		调用不到原API，这时程序自然会发生崩溃了。 
	*/

	HookOff();

	return CWinApp::ExitInstance();
}

/*
	 提升权限
*/
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


/*
	 鼠标钩子子过程，目的是加载本dll到使用鼠标的程序中。
	 鼠标钩子的作用：当鼠标在某程序窗口中时，就会加载我们这个dll。
*/
LRESULT CALLBACK MouseProc(
						int nCode,	  // hook code
						WPARAM wParam,// message identifier
						LPARAM lParam // mouse coordinates
						)
{

	return CallNextHookEx(hhk,nCode,wParam,lParam);
}

/*
	将长度为length的pcode写入到地址lpAddress中。
*/
void WriteMemory(LPVOID lpAddress,BYTE* pcode,int length)
{
	//
	//	保证本进程句柄不为NULL
	//
	ASSERT(hProcess != NULL);

	DWORD dwTemp,dwOldProtect,dwRet,dwWrited;

	//
	// 修改API入口前length个字节为 jmp xxxx
	//
	VirtualProtectEx(hProcess,lpAddress,length,PAGE_READWRITE,&dwOldProtect);
	dwRet = WriteProcessMemory(hProcess,lpAddress,pcode,length,&dwWrited);
	if ( 0 == dwRet || 0 == dwWrited)
	{
		AfxMessageBox(_T("哭！！写入失败"));
	}
	VirtualProtectEx(hProcess,lpAddress,length,dwOldProtect,&dwTemp);
}

/*
	用新API地址替换原API地址
*/
void HookOn()
{
	ASSERT(hProcess != NULL);

	DWORD dwTemp,dwOldProtect,dwRet,dwWrited;
	
	WriteMemory(pfMsgBoxA,newCodeA,CODE_LENGTH);
	WriteMemory(pfMsgBoxW,newCodeW,CODE_LENGTH);

}

/*	
	恢复原API地址
*/
void HookOff()
{
	ASSERT(hProcess != NULL);

	DWORD dwTemp,dwOldProtect,dwRet,dwWrited;

	WriteMemory(pfMsgBoxA,oldCodeA,CODE_LENGTH);
	WriteMemory(pfMsgBoxW,oldCodeW,CODE_LENGTH);
}

/*
	注入
*/
void Inject()
{
	if ( TRUE == bIsInJected)
	{
		return;
	}
	bIsInJected = TRUE;	// 保证只调用一次


	//
	// 获取函数
	//
	HMODULE hmodle = ::LoadLibrary(_T("User32.dll"));
	oldMsgBoxA = (TypeMsgBoxA) ::GetProcAddress(hmodle,"MessageBoxA");
	pfMsgBoxA = (FARPROC)oldMsgBoxA;

	oldMsgBoxW = (TypeMsgBoxW) ::GetProcAddress(hmodle,"MessageBoxW");
	pfMsgBoxW = (FARPROC)oldMsgBoxW;

	if (pfMsgBoxA == NULL)
	{
		AfxMessageBox(_T("获取 MessageBoxA 函数失败"));
		return;
	}
	if ( pfMsgBoxW == NULL)
	{
		AfxMessageBox(_T("获取 MessageBoxW 函数失败"));
		return;
	}

	//
	// 保存原API地址
	//
	_asm
	{
		lea edi,oldCodeA	// 取数组基地址
		mov esi,pfMsgBoxA	// API地址
		cld					// 设置方向
		mov ecx,CODE_LENGTH
		rep movsb
	}

	_asm
	{
		lea edi,oldCodeW
		mov esi,pfMsgBoxW
		cld
		mov ecx,CODE_LENGTH
		rep movsb
	}

	//
	// 将新地址复制到入口
	//
	newCodeA[0] = newCodeW [0] = 0xe9;	// jmp 指定代码
	_asm
	{
		lea eax,MyMessageBoxA		// 新API地址
		mov ebx,pfMsgBoxA			// 原API地址
		sub eax,ebx				
		sub eax,CODE_LENGTH			// 跳转地址 = 新API地址 - 原API地址 - 指令长度
		mov dword ptr [newCodeA+1],eax // eax 32bit =  4 BYTE
	}

	_asm
	{
		lea eax,MyMessageBoxW
		mov ebx,pfMsgBoxW
		sub eax,ebx
		sub eax,CODE_LENGTH
		mov dword ptr [newCodeW + 1],eax
	}

	HookOn();	//	开始HOOK

}

//
// 安装钩子
//
BOOL WINAPI StartHook(HWND hWnd)
{
	g_hWnd = hWnd;
	hhk = ::SetWindowsHookEx(WH_MOUSE,MouseProc,g_hInstance,0);

	if (hhk == NULL)
	{
		return FALSE;
	}
	else
	{
		
		return TRUE;
	}
}

//
// 卸载钩子
//
BOOL WINAPI StopHook()
{	
	/*
		卸载钩子时，一定要记得恢复原API入口。
		这里恢复的只是主程序的原API入口，其它程序的API入口还没有被恢复。
		因此我们必须处理dll退出过程，即在函数ExitInstance()中，调用恢复  
		API入口的函数HookOff(),只有这样，其它程序再次调用原API时，才不
		会发生错误。
		当我们HOOK所有程序的某个系统API时，千万要注意在ExitInstance()中
		调用HookOff()！！！！！
	*/
	HookOff();
	if (hhk!=NULL)
	{
		UnhookWindowsHookEx(hhk);
		FreeLibrary(g_hInstance);
	}

	return TRUE;
}


/*
	自己用于替换的API
*/
int WINAPI MyMessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCation,UINT uType)
{
	int nRet = 0;

	HookOff();
/*
	TCHAR* procName = new TCHAR[MAX_PATH];
	GetModuleFileName(NULL,procName,MAX_PATH);  // 这里可以取得进程名，那么就可以判断是否是机密进程啦。。
	CString info;
	info.Format(_T("进程名 %s"),procName);
	AfxMessageBox(info);*/
	nRet = ::MessageBoxA(hWnd,"哈哈 ^_^，MessageBoxA 被 HOOK 咯",lpCation,uType);
	nRet = ::MessageBoxA(hWnd,lpText,lpCation,uType);
/*	
	info.Format(_T("MyMessageBoxA 被 HOOK 啦"));
	//msgToMain = (TCHAR *)malloc(info.GetLength() * sizeof(TCHAR) + 1);
	memset(msgToMain,0,150);
	msgToMain = info.GetBuffer(info.GetLength());
	::SendMessage(g_hWnd,WM_HOOKMSG,(WPARAM)msgToMain,0);
	*/
	HookOn();

	return nRet;
}

int WINAPI MyMessageBoxW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCation,UINT uType)
{
	int nRet = 0;

	HookOff();
	
/*	TCHAR* procName = new TCHAR[MAX_PATH];
	GetModuleFileName(NULL,procName,MAX_PATH);  // 这里可以取得进程名，那么就可以判断是否是机密进程啦。。
	CString info;
	info.Format(_T("进程名 %s"),procName);
	AfxMessageBox(info);*/
	nRet = ::MessageBoxW(hWnd,_T("O(∩_∩)O哈哈~，MMessageBoxW 被 HOOK 咯"),lpCation,uType);
	nRet = ::MessageBoxW(hWnd,lpText,lpCation,uType);
/*	
	info.Format(_T("MyMessageBoxW 被 HOOK 啦"));
	//msgToMain = (TCHAR *)malloc(info.GetLength() * sizeof(TCHAR) + 1);
	memset(msgToMain,0,150);
	msgToMain = info.GetBuffer(info.GetLength());
	
	::SendMessage(g_hWnd,WM_HOOKMSG,(WPARAM)msgToMain,0);*/


	HookOn();

	return nRet;
}