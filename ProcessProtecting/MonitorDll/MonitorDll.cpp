// MonitorDll.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "MonitorDll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/*
	全局变量
*/

//	共享变量
#pragma data_seg("Share")
HWND g_hwnd = NULL;			//	主窗口句柄，加载HOOK时传入
HINSTANCE hInstance = NULL;	//	本DLL的实例句柄
HHOOK hhook = NULL;			//	鼠标钩子句柄
DWORD g_dwProcessId;		//	进程id
HANDLE g_hProcess = NULL;	//	保存本进程在远进程中的句柄	
#pragma data_seg()
#pragma comment(linker,"/section:Share,rws")

//	其他变量定义
HANDLE hProcess = NULL;				//	当前进程句柄
bool bIsInjected = false;			//	保证只注入一次

#define CODE_LENGTH	5				//	入口指令长度

// TerminateProcess
typedef BOOL (WINAPI *TypeTerminateProcess)(_In_ HANDLE hProcess, _In_ UINT uExitCode); //Kernel32.dll
TypeTerminateProcess oldTerminateProcess = NULL;
FARPROC pfOldTerminateProcess = NULL;
BOOL WINAPI MyTerminateProcess(_In_ HANDLE hProcess, _In_ UINT uExitCode);
BYTE oldCodeTermPro[CODE_LENGTH];	//	原API入口
BYTE newCodeTermpro[CODE_LENGTH];	//	新API入口

//	OpenProcess
typedef HANDLE(WINAPI *TypeOpenProcess)( _In_ DWORD dwDesiredAccess,_In_ BOOL  bInheritHandle,_In_ DWORD dwProcessId);
TypeOpenProcess oldOpenProcess = NULL;
FARPROC pfOldOpenProcess = NULL;
HANDLE WINAPI MyOpenProcess(_In_ DWORD dwDesiredAccess,_In_ BOOL  bInheritHandle,_In_ DWORD dwProcessId);
BYTE oldCodeOpenPro[CODE_LENGTH];
BYTE newCodeOpenPro[CODE_LENGTH];

BOOL WINAPI HookLoad(HWND hwnd,DWORD  dwProcessId);	// 关于dll hook 操作
VOID WINAPI HookUnload();
VOID Inject();	
VOID HookOn();
VOID HookOff();
BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
	) ;
LRESULT CALLBACK MouseProc(		// 鼠标钩子子过程调用
	int nCode,	// hook code
	WPARAM wParam,// message identifier
	LPARAM lParam // mouse coordinates
	);
BOOL WriteMemory(LPVOID lpAddress,BYTE* pcode,size_t length); //将长度为 length 的 pcode 写入地址 lpAddress 的进程内存中


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

// CMonitorDllApp

BEGIN_MESSAGE_MAP(CMonitorDllApp, CWinApp)
END_MESSAGE_MAP()


// CMonitorDllApp 构造

CMonitorDllApp::CMonitorDllApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CMonitorDllApp 对象

CMonitorDllApp theApp;


// CMonitorDllApp 初始化

BOOL CMonitorDllApp::InitInstance()
{
	CWinApp::InitInstance();
	
	hInstance = AfxGetInstanceHandle();			// 获取本dll句柄

	/*
		先提高权限，再获取进程句柄。
		因为只有权限足够，才能获取到当前进程的句柄。
	*/
	HANDLE hToken;
	BOOL bRet = OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken);
	if (bRet == FALSE)
	{
		AfxMessageBox(_T("权限提升失败"));
	}
	SetPrivilege(hToken,SE_DEBUG_NAME,TRUE);

	DWORD dwPid = ::GetCurrentProcessId();
	hProcess = ::OpenProcess(PROCESS_ALL_ACCESS,0,dwPid);
	if (hProcess == NULL)
	{
		CString str;
		str.Format(_T("OpenProcess fail!!, error code = [%d]"),GetLastError());
		AfxMessageBox(str);
		return FALSE;
	}

	Inject();		//	开始注入
	return TRUE;
}

//
//	实例退出函数。退出时，一定要记得恢复原函数地址！！！
//
int CMonitorDllApp::ExitInstance()
{
	HookOff();	//要记得恢复原函数地址

	return CWinApp::ExitInstance();
}


/*
	鼠标钩子子过程，目的是加载本dll到使用鼠标的程序.
	鼠标钩子的作用：当鼠标在某程序窗口中时，就会加载我们这个dll。
	即使本DLL随着鼠标钩子注入到目标进程中。
*/
LRESULT CALLBACK MouseProc(		
	int nCode,		// hook code
	WPARAM wParam,	// message identifier
	LPARAM lParam	// mouse coordinates
	)
{

	return CallNextHookEx(hhook,nCode,wParam,lParam);
}


/* 
	安装钩子。
	主调程序传入窗口句柄和进程id。
*/

BOOL WINAPI HookLoad(HWND hwnd,DWORD  dwProcessId)
{
	BOOL ret = FALSE;

	g_hwnd = hwnd;
	g_dwProcessId = dwProcessId;
	hhook = ::SetWindowsHookEx(WH_MOUSE,MouseProc,hInstance,0);

	if (hhook == NULL)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

/*
	卸载钩子。
	注：卸载钩子之前，一定要记得恢复原函数地址！！！
*/
VOID WINAPI HookUnload()
{
	HookOff();	// 恢复原函数地址
	if (hhook != NULL)
	{
		UnhookWindowsHookEx(hhook);
	}
	if (hInstance != NULL)
	{
		FreeLibrary(hInstance);
	}
}


/*
	注入函数。
	主要完成原函数地址的保存，保存到 oldCode_[]中；
	新入口地址的计算，保存到newCode_[]中，即 jmp xxxx 指令。
	新入口地址 = 新函数地址 - 原函数地址 - 指令长度
	最后一定要记得HookOn！！
*/
VOID Inject()
{
	if (bIsInjected == TRUE)	
	{
		return;
	}
	bIsInjected = TRUE;// 保证只注入一次


	//	TerminateProcess
	HMODULE hmodleKernel32;
	hmodleKernel32 = ::LoadLibrary(_T("Kernel32.dll"));
	if (NULL ==  hmodleKernel32)
	{
		AfxMessageBox(_T("加载Kernel32.dll失败"));
		return;
	}
	//	获取原函数地址
	oldTerminateProcess = (TypeTerminateProcess)GetProcAddress(hmodleKernel32,"TerminateProcess");
	if (NULL == oldTerminateProcess)
	{
		AfxMessageBox(_T("获取TerminateProcess函数失败"));
		return;
	}
	pfOldTerminateProcess = (FARPROC)oldTerminateProcess;
	//	保存原函数入口
	_asm
	{
		lea edi,oldCodeTermPro
		mov esi,pfOldTerminateProcess
		cld
		mov ecx,CODE_LENGTH
		rep movsb
	}
	//	替换新函数入口
	newCodeTermpro[0] = 0xe9;
	_asm
	{
		lea eax,MyTerminateProcess
		mov ebx,pfOldTerminateProcess
		sub eax,ebx
		sub eax,CODE_LENGTH
		mov dword ptr [newCodeTermpro+1],eax
	}

	// OpenProcess
	oldOpenProcess = (TypeOpenProcess)GetProcAddress(hmodleKernel32,"OpenProcess");
	if (NULL == oldOpenProcess)
	{
		AfxMessageBox(_T("获取OpenProcess地址失败"));
		return;
	}
	pfOldOpenProcess = (FARPROC)oldOpenProcess;
	_asm
	{
		lea edi,oldCodeOpenPro
			mov esi,pfOldOpenProcess
			cld
			mov ecx,CODE_LENGTH
			rep movsb
	}
	newCodeOpenPro[0] = 0xe9;
	_asm
	{
		lea eax,MyOpenProcess
			mov ebx,pfOldOpenProcess
			sub eax,ebx
			sub eax,CODE_LENGTH
			mov dword ptr [newCodeOpenPro+1],eax
	}

	HookOn();	//填充完毕，开始HOOK

}

/*
	将长度为 length 的 pcode 写入地址 lpAddress 的进程内存中
*/
BOOL WriteMemory(LPVOID lpAddress,BYTE* pcode,size_t length)
{
	ASSERT(hProcess != NULL);

	DWORD dwtemp,dwOldProtect,dwRet,dwWrited;

	dwRet = VirtualProtectEx(hProcess,lpAddress,length,PAGE_READWRITE,&dwOldProtect);
	CString logInfo;
	if ( 0 == dwRet)
	{
		logInfo.Format(_T("WriteMemory :: Call VirtualProtectEx fail, eror code = [%d]\n\n"),GetLastError());
		AfxMessageBox(logInfo);
		return FALSE;
	}
	dwRet = WriteProcessMemory(hProcess,lpAddress,pcode,length,&dwWrited);
	if ( 0 == dwRet || 0 == dwWrited)
	{
		logInfo.Format(_T("WriteMemory :: Call WriteProcessMomory fail, error code = [%d]\n\n"),GetLastError());
		AfxMessageBox(logInfo);
		return FALSE;
	}
	dwRet = VirtualProtectEx(hProcess,lpAddress,length,dwOldProtect,&dwtemp);
	if ( 0 == dwRet )
	{
		logInfo.Format(_T("WriteMemory :: Recover Protect fail, error code = [%d]\n\n"),GetLastError());
		AfxMessageBox(logInfo);
		return FALSE;
	}
	return TRUE;
}

/*
	开始HOOK。
	即，将Inject 初始化好的入口地址进行写入进程内存中。
	这里，将 新函数入口 newCode_[]，写入内存中。
	这样一来，在原函数被调用的时候，就会跳转到我们新函数的位置。
	
	注: 这里处理的函数，是当前需要替换的所有函数，所以只在Inject()函数中调用，
	即进行初始化的时候用到该函数。
*/
VOID HookOn()
{
	
	BOOL ret;
	
	ret = WriteMemory(pfOldTerminateProcess,newCodeTermpro,CODE_LENGTH);
	if (FALSE == ret)
	{
		AfxMessageBox(_T("HookOn :: Fail to write pfOldTerminateProcess"));
	}
	ret = WriteMemory(pfOldOpenProcess,newCodeOpenPro,CODE_LENGTH);
	if (FALSE == ret)
	{
		AfxMessageBox(_T("HookOn :: Fail to write pfOldOpenProcess"));
	}
}

/*
	停止HOOK。
	恢复原函数地址。

	注：这里处理的是所有替换的函数，所以一般情况下只有在卸载HOOK函数中调用
*/
VOID HookOff()
{
	
	ASSERT(hProcess != NULL);

	BOOL ret;
	ret = WriteMemory(pfOldTerminateProcess,oldCodeTermPro,CODE_LENGTH);
	if (FALSE == ret)
	{
		AfxMessageBox(_T("HookOff :: fail to recover pfOldTerminateProcess \n\n"));
	}
	ret = WriteMemory(pfOldOpenProcess,oldCodeOpenPro,CODE_LENGTH);
	if (FALSE == ret)
	{
		AfxMessageBox(_T("HookOff :: fail to recover pfOldOpenProcess"));
	}
}

/*
	提升进程权限。
*/
BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
	) 
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	CString info;
	if ( !LookupPrivilegeValue( 
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid ) )        // receives LUID of privilege
	{
		info.Format(_T("LookupPrivilegeValue error: %u\n"), GetLastError() ); 
		AfxMessageBox(info);
		return FALSE; 
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if ( !AdjustTokenPrivileges(
		hToken, 
		FALSE, 
		&tp, 
		sizeof(TOKEN_PRIVILEGES), 
		(PTOKEN_PRIVILEGES) NULL, 
		(PDWORD) NULL) )
	{ 
		info.Format(_T("AdjustTokenPrivileges error: %u\n"), GetLastError() ); 
		AfxMessageBox(info);
		return FALSE; 
	} 

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		info.Format(_T("The token does not have the specified privilege. \n"));
		AfxMessageBox(info);
		return FALSE;
	} 

	return TRUE;
}


//
//	自己重新定义的进程终止函数。
//	检查当前要终止的进程是否是受保护进程，若是则禁止关闭。
//
BOOL WINAPI MyTerminateProcess(_In_ HANDLE hProcess, _In_ UINT uExitCode)
{
	BOOL ret;
	if (g_hProcess == hProcess)
	{
		AfxMessageBox(_T("不能关闭受保护进程哦！！"));
		ret = TRUE;
	}
	else
	{
		WriteMemory(pfOldTerminateProcess,oldCodeTermPro,CODE_LENGTH);
		ret = oldTerminateProcess(hProcess,uExitCode);
		WriteMemory(pfOldTerminateProcess,newCodeTermpro,CODE_LENGTH);
	}

	return ret;
}


//
//	自己定义的打开进程函数。
//	若当前打开进程为受保护进程，则记录下该远程调用句柄。
//
HANDLE WINAPI MyOpenProcess(_In_ DWORD dwDesiredAccess,_In_ BOOL  bInheritHandle,_In_ DWORD dwProcessId)
{
	HANDLE hProcess = NULL;

	WriteMemory(pfOldOpenProcess,oldCodeOpenPro,CODE_LENGTH);
	hProcess = oldOpenProcess(dwDesiredAccess,bInheritHandle,dwProcessId);
	if ( dwProcessId == g_dwProcessId)
	{
		g_hProcess = hProcess;

	}
	WriteMemory(pfOldOpenProcess,newCodeOpenPro,CODE_LENGTH);

	return hProcess;

}
