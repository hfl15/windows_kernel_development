//	This contains all of the global definitions for mini-filters.
//	Kernel mode
#include <fltKernel.h>

//This module deprecates Banned APIs
#include <dontuse.h>	

// Auto-generated from directory %SDXROOT%\sdktools\analysis\prefast\suppression\inc 
//   using file %SDXROOT%\tools\analysis\x86\OACR\inc\pftwarnings.h
#include <suppress.h>

//This is the include file that defines all constants and types for
//    accessing the SCSI port adapters.

#include <ntddscsi.h>		

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

#define MINISPY_PORT_NAME								L"\\NPMiniPort"

/**
 * the introduction of parameters：
 *
 * @param  	UniName			IN UNICODE_STRING
 * @param  	Name				IN OUT char pointor
 * @return  BOOLEAN 		TURE,FALSE
 */
BOOLEAN NPUnicodeStringToChar(PUNICODE_STRING UniName, char Name[]);


/*************************************************************************
    Prototypes
*************************************************************************/

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    );

NTSTATUS
NPInstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
NPInstanceTeardownStart (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
NPInstanceTeardownComplete (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
NPUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
NPInstanceQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
NPPreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );
    
FLT_POSTOP_CALLBACK_STATUS
NPPostCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );
        
NTSTATUS
NPMiniMessage (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    );

NTSTATUS
NPMiniConnect(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    );

VOID
NPMiniDisconnect(
    __in_opt PVOID ConnectionCookie
    );
            
//  Assign text sections for each routine.
/*
			#pragma alloc_text：编译时控制分页能力
			说明：http://hi.baidu.com/billbeggar/item/c378e2ea39a5daeffa42bada
			
			如果某些代码在驱动程序完成初始化后不再需要，可以直接把它插入到INIT段：
			#pragma alloc_text(INIT, DriverEntry)
			像入口函数代码在驱动程序完成初始化后往往不再需要，可以直接把它插入到INIT段。 
			
			加个头无非就是说明该函数是分在 分页内存 非分页内存，初始内存等等...
			例如：
	    #pragma alloc_text(PAGE, a) 
				
*/
#ifdef ALLOC_PRAGMA
		#pragma alloc_text(INIT, DriverEntry)
		#pragma alloc_text(PAGE, NPUnload)
		#pragma alloc_text(PAGE, NPInstanceQueryTeardown)
		#pragma alloc_text(PAGE, NPInstanceSetup)
		#pragma alloc_text(PAGE, NPInstanceTeardownStart)
		#pragma alloc_text(PAGE, NPInstanceTeardownComplete)
		#pragma alloc_text(PAGE, NPPreCreate)
    #pragma alloc_text(PAGE, NPMiniConnect)				//for port comunication
    #pragma alloc_text(PAGE, NPMiniDisconnect)			//for port comunication
    #pragma alloc_text(PAGE, NPMiniMessage)    		//for port comunication		
#endif    

//  operation registration
const FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,	//	请求主功能号
      0,							//	这个标志仅仅对读/写回调有用，所以对生成请求直接设置为0
      NPPreCreate,		//	生成预操作回调
      NPPostCreate },	//	生成后操作回调

    { IRP_MJ_OPERATION_END }	// 必须作为之后一个元素，否则过滤器无法知道到底有多少个元素
};

//  This defines what we want to filter with FltMgr
const FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version，可直接按照惯例写：FLT_REGISTRATION_VERSION
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    NPUnload,                           //  MiniFilterUnload

    NPInstanceSetup,                    //  InstanceSetup
    NPInstanceQueryTeardown,            //  InstanceQueryTeardown
    NPInstanceTeardownStart,            //  InstanceTeardownStart
    NPInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};

//  Defines the commands between the utility and the filter
typedef enum _NPMINI_COMMAND {
    ENUM_PASS = 0,
    ENUM_BLOCK
} NPMINI_COMMAND;	

//  Defines the command structure between the utility and the filter.
typedef struct _COMMAND_MESSAGE {
    NPMINI_COMMAND 	Command;  
} COMMAND_MESSAGE, *PCOMMAND_MESSAGE;





