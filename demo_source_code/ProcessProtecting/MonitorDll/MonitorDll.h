// MonitorDll.h : MonitorDll DLL 的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// CMonitorDllApp
// 有关此类实现的信息，请参阅 MonitorDll.cpp
//

class CMonitorDllApp : public CWinApp
{
public:
	CMonitorDllApp();

// 重写
public:
	virtual BOOL InitInstance();
	int ExitInstance();

	DECLARE_MESSAGE_MAP()
};
