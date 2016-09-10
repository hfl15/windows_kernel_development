// MonitorDll.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "MonitorDll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/************************************************************************/
/*          变量的定义与函数声明                                        */

// 共享数据
#pragma data_seg("Share")

HWND g_hwnd = NULL;			//	主窗口句柄，加载HOOK的时候传入
HINSTANCE hInstance = NULL;	//	本DLL的实例句柄
HHOOK hhook = NULL;			//	鼠标钩子句柄

#pragma data_seg()
#pragma comment(linker,"/section:Share,rws")

//其它变量定义

#define WM_HOOKMSG WM_USER + 106	// 自己定义的消息
HANDLE hProcess = NULL;		//	当前进程句柄
BOOL bIsInjected = FALSE;	//	保证只注入一次


typedef HANDLE ( WINAPI *TypeSetClipboardData )(_In_ UINT uFormat,_In_opt_ HANDLE hMem);//User32.dll
typedef HANDLE ( WINAPI *TypeGetClipboardData )(_In_ UINT uFormat);
typedef HRESULT (WINAPI *TypeOleSetClipboard)(_In_ LPDATAOBJECT pDataObj);		//Ole32.dll
typedef HRESULT (WINAPI *TypeOleGetClipboard)(_Out_ LPDATAOBJECT *ppDataObje); 

TypeSetClipboardData oldSetClipboardData = NULL;	// 保存原函数地址
TypeGetClipboardData oldGetClipboardData = NULL;
TypeOleSetClipboard  oldOleSetClipboard = NULL;
TypeOleGetClipboard  oldOleGetClipboard = NULL;

FARPROC pfOldSetClipboardData = NULL;	// 指向原函数地址的远指针
FARPROC pfOldGetClipboardData = NULL;
FARPROC pfOldOleSetClipboard = NULL;
FARPROC pfOldOleGetClipboard = NULL;

HANDLE WINAPI MySetClipboardData(_In_ UINT uFormat,_In_opt_ HANDLE hMem);	// 自己的函数声明
HANDLE WINAPI MyGetClipboardData(_In_ UINT uFormat);
HRESULT WINAPI MyOleSetClipboard(_In_ LPDATAOBJECT pDataObj);
HRESULT WINAPI MyOleGetClipboard(_Out_ LPDATAOBJECT *ppDataObje);



#define CODE_LENGTH	5			//	入口指令长度
BYTE oldCodeSet[CODE_LENGTH];	//	原函数入口
BYTE newCodeSet[CODE_LENGTH];	//	替换函数入口指令

BYTE oldCodeGet[CODE_LENGTH];
BYTE newCodeGet[CODE_LENGTH];

BYTE oldCodeSetOle[CODE_LENGTH];
BYTE newCodeSetOle[CODE_LENGTH];

BYTE oldCodeGetOle[CODE_LENGTH];
BYTE newCodeGetOle[CODE_LENGTH];


BOOL WINAPI HookLoad(HWND hwnd);	// 关于dll hook 操作
VOID WINAPI HookUnload();
VOID Inject();
VOID HookOn();
VOID HookOff();
BOOL AdjustPrivileges();	// 提升权限
LRESULT CALLBACK MouseProc(		// 鼠标钩子子过程调用
	int nCode,	// hook code
	WPARAM wParam,// message identifier
	LPARAM lParam // mouse coordinates
	);
BOOL WriteMemory(LPVOID lpAddress,BYTE* pcode,size_t length); //将长度为 length 的 pcode 写入地址 lpAddress 的进程内存中


/************************************************************************/


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

	hInstance = AfxGetInstanceHandle();

	AdjustPrivileges();
	DWORD dwPid = ::GetCurrentProcessId();
	hProcess = ::OpenProcess(PROCESS_ALL_ACCESS,0,dwPid);

	if (hProcess == NULL)
	{
		CString str;
		str.Format(_T("OpenProcess fail!!, error code = [%d]"),GetLastError());
		AfxMessageBox(str);
		return FALSE;
	}

	Inject();
	return TRUE;
}

//	退出时，一定要记得恢复原函数地址！！！

int CMonitorDllApp::ExitInstance()
{
	HookOff();	//要记得恢复原函数地址
	
	return CWinApp::ExitInstance();
}

/************************************************************************/
/*                      DLL相关操作函数                                */

/*
	鼠标钩子子过程，目的是加载本dll到使用鼠标的程序.
	鼠标钩子的作用：当鼠标在某程序窗口中时，就会加载我们这个dll
*/
LRESULT CALLBACK MouseProc(		
	int nCode,	// hook code
	WPARAM wParam,// message identifier
	LPARAM lParam // mouse coordinates
	)
{
	/*	if (nCode == HC_ACTION)
	{
		// 将钩子所在窗口句柄发给主程序
		::SendMessage(g_hWnd,WM_HOOKMSG,wParam,(LPARAM)(((PMOUSEHOOKSTRUCT)lParam)->hwnd));
	}*/

	return CallNextHookEx(hhook,nCode,wParam,lParam);
}

/* 
	安装钩子
*/
BOOL WINAPI HookLoad(HWND hwnd)
{
	BOOL ret = FALSE;

	g_hwnd = hwnd;
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
	卸载钩子
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

	// 加载User32.dll 和 Ole32.dll

	HMODULE hmodleUser32,hmodleOle32;
	hmodleUser32 = ::LoadLibrary(_T("User32.dll"));
	if ( NULL == hmodleUser32)
	{
		TRACE("Inject :: 加载 User32.dll 失败");
		return;
	}
	hmodleOle32 = ::LoadLibrary(_T("Ole32.dll"));
	if ( NULL == hmodleOle32)
	{
		AfxMessageBox(_T("Inject :: 加载Ole32.dll 失败"));
		return;
	}

	//	获取原函数地址
	oldGetClipboardData = (TypeGetClipboardData)::GetProcAddress(hmodleUser32,"GetClipboardData");
	pfOldGetClipboardData = (FARPROC)oldGetClipboardData;
	if (NULL == pfOldGetClipboardData)
	{
		AfxMessageBox(_T("Inject :: 获取函数 GetClipboardData 失败"));
		return;
	}
	oldSetClipboardData = (TypeSetClipboardData)::GetProcAddress(hmodleUser32,"SetClipboardData");
	pfOldSetClipboardData = (FARPROC) oldSetClipboardData;
	if (NULL == pfOldGetClipboardData)
	{
		AfxMessageBox(_T("Inject :: 获取函数 SetClipboardData 失败"));
		return;
	}
	oldOleGetClipboard = (TypeOleGetClipboard)::GetProcAddress(hmodleOle32,"OleGetClipboard");
	pfOldOleGetClipboard = (FARPROC)oldOleGetClipboard;
	if (NULL == pfOldGetClipboardData)
	{
		AfxMessageBox(_T("Inject :: 获取函数 OleGetClipboard 失败"));
		return;
	}
	oldOleSetClipboard = (TypeOleSetClipboard)::GetProcAddress(hmodleOle32,"OleSetClipboard");
	pfOldOleSetClipboard = (FARPROC) oldOleSetClipboard;
	if (NULL == pfOldGetClipboardData)
	{
		AfxMessageBox(_T("Inject :: 获取函数 OleSetClipboard 失败"));
		return;
	}


	//
	//	保存原函数入口
	//
	_asm
	{
		lea edi, oldCodeGet
		mov esi,pfOldGetClipboardData
		cld
		mov ecx,CODE_LENGTH
		rep movsb
	}

	_asm
	{
		lea edi,oldCodeSet
		mov esi,pfOldSetClipboardData
		cld
		mov ecx,CODE_LENGTH
		rep movsb
	}

	_asm
	{
		lea edi,oldCodeGetOle
		mov esi,pfOldOleGetClipboard
		cld
		mov ecx,CODE_LENGTH
		rep movsb
	}

	_asm
	{
		lea edi,oldCodeSetOle
		mov esi,pfOldOleSetClipboard
		cld
		mov ecx,CODE_LENGTH
		rep movsb
	}

	//
	//	获取新入口 : 新相对地址 = 新函数地址 - 原函数地址 - 指令长度
	//
	newCodeGet[0] = 0xe9;	//	jmp 指令, 1 BYTE
	_asm
	{
		lea eax,MyGetClipboardData
		mov ebx,pfOldGetClipboardData
		sub eax,ebx
		sub eax,CODE_LENGTH
		mov dword ptr [newCodeGet+1],eax	// 4 BYTE
	}

	newCodeSet[0] = 0xe9;
	_asm
	{
		lea eax,MySetClipboardData
		mov ebx,pfOldSetClipboardData
		sub eax,ebx
		sub eax,CODE_LENGTH
		mov dword ptr [newCodeSet+1],eax
	}

	newCodeGetOle[0] = 0xe9;
	_asm
	{
		lea eax,MyOleGetClipboard
		mov ebx,pfOldOleGetClipboard
		sub eax,ebx
		sub eax,CODE_LENGTH
		mov dword ptr [newCodeGetOle+1],eax
	}

	newCodeSetOle[0] = 0xe9;
	_asm
	{
		lea eax,MyOleSetClipboard
		mov ebx,pfOldOleSetClipboard
		sub eax,ebx
		sub eax,CODE_LENGTH
		mov dword ptr [newCodeSetOle+1],eax
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
	ret = WriteMemory(pfOldGetClipboardData,newCodeGet,CODE_LENGTH);
	if (FALSE == ret)
	{
		TRACE("HookOn ::　Fail to write pfOldGetClipboardData \n\n");
	}
	ret = WriteMemory(pfOldSetClipboardData,newCodeSet,CODE_LENGTH);
	if (FALSE == ret)
	{
		TRACE("HookOn :: Fail to wirte pfOldSetClipboardData \n\n");
	}
	ret = WriteMemory(pfOldOleGetClipboard,newCodeGetOle,CODE_LENGTH);
	if (FALSE == ret)
	{
		TRACE("HookOn :: Fail to wirte pfOldOleGetClipboard \n\n");
	}
	ret = WriteMemory(pfOldOleSetClipboard,newCodeSetOle,CODE_LENGTH);
	if (FALSE == ret)
	{
		TRACE("HookOn :: Fail to write pfOldOleSetClipboard \n\n");
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
	ret = WriteMemory(pfOldGetClipboardData,oldCodeGet,CODE_LENGTH);
	if (FALSE == ret)
	{
		TRACE("HookOff :: fail to recover pfOldGetClipboardData oldCode \n\n");
	}
	ret = WriteMemory(pfOldSetClipboardData,oldCodeSet,CODE_LENGTH);
	if ( FALSE == ret)
	{
		TRACE("Hookoff :: fail to recover pfOldSetClipboardData oldCode \n\n");
	}
	ret = WriteMemory(pfOldOleGetClipboard,oldCodeGetOle,CODE_LENGTH);
	if (FALSE == ret)
	{
		TRACE("HookOff :: fail to recover pfOldOleGetClipboard \n\n");
	}
	ret = WriteMemory(pfOldOleSetClipboard,oldCodeSetOle,CODE_LENGTH);
	if (FALSE == ret)
	{
		TRACE("HookOff :: fail to recover pfOldOleSetClipboard \n\n");
	}
}
/*
	提升权限
*/
BOOL AdjustPrivileges()
{
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

/************************************************************************/




/************************************************************************/
/*                            替换函数实现                              */

CString getCurProcessName()
{
	TCHAR* procName = new TCHAR[MAX_PATH];
	GetModuleFileName(NULL,procName,MAX_PATH);  // 这里可以取得进程名，那么就可以判断是否是机密进程啦。。
	CString info;
	info.Format(_T("%s"),procName);
	return info;
}

HANDLE WINAPI MyGetClipboardData(_In_ UINT uFormat)
{
	HANDLE handle = NULL;

	WriteMemory(pfOldGetClipboardData,oldCodeGet,CODE_LENGTH);//恢复原函数地址

	CString processName = getCurProcessName();
	CString info;
	info.Format(_T("HOOK  GetClipboardData 咯 ^_^，当前进程：[%s]"),processName);// 调用原函数
	AfxMessageBox(info);

	handle = oldGetClipboardData(uFormat);
	WriteMemory(pfOldGetClipboardData,newCodeGet,CODE_LENGTH);//替换原函数，继续HOOK
	return handle;
}

HANDLE WINAPI MySetClipboardData(_In_ UINT uFormat,_In_opt_ HANDLE hMem)
{
	HANDLE handle = NULL;

	WriteMemory(pfOldSetClipboardData,oldCodeSet,CODE_LENGTH);
	CString processName = getCurProcessName();
	CString info;
	info.Format(_T("HOOK  SetClipboardData 咯 ^_^，当前进程：[%s]"),processName);
	AfxMessageBox(info);

	handle = oldSetClipboardData(uFormat,hMem);

	WriteMemory(pfOldSetClipboardData,newCodeSet,CODE_LENGTH);
	return handle;
}

HRESULT WINAPI MyOleGetClipboard(_Out_ LPDATAOBJECT *ppDataObje)
{
	HRESULT hresult = S_FALSE;

	HookOff();

	CString processName = getCurProcessName();
	CString info;
	info.Format(_T("HOOK  OleGetClipboard 咯 ^_^，当前进程：[%s]"),processName);
	AfxMessageBox(info);

	hresult = OleGetClipboard(ppDataObje);

	HookOn();

	return hresult;
}

HRESULT WINAPI MyOleSetClipboard(_In_ LPDATAOBJECT pDataObj)
{
	HRESULT hresult = S_FALSE;

	HookOff();

	CString processName = getCurProcessName();
	CString info;
	info.Format(_T("HOOK  OleSetClipboard 咯 ^_^，当前进程：[%s]"),processName);
	AfxMessageBox(info);

	hresult = OleSetClipboard(pDataObj);

	HookOn();

	return hresult;
}

/************************************************************************/


