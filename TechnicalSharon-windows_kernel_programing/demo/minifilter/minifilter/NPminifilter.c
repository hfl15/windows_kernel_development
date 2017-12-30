#include "NPminifilter.h"
//  Global variables

ULONG gTraceFlags = 0;

PFLT_FILTER gFilterHandle;
PFLT_PORT 	gServerPort;
PFLT_PORT 	gClientPort;

NPMINI_COMMAND gCommand = ENUM_BLOCK;

//	使用这个定义似乎是看不到输出的。。。
#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((void)0))

/*
	功能说明：该函数实现了将 UNICODE 字符串转换为 全是大写的 char 数组，以便于查找“NOTEPAD.EXE”。
	参数：
	返回值：TRUE OR FALSE
*/
BOOLEAN NPUnicodeStringToChar(PUNICODE_STRING UniName, char Name[])
{
    ANSI_STRING	AnsiName;			//	ANSI_STRING 中的 Buffer 是 PCHAR 类型，而UNICODE_STRING 中的 Buffer 是 PWSTR 类型
    NTSTATUS	ntstatus;
    char*		nameptr;			
    
   DbgPrint("NPminifilter!NPUnicodeStringToChar: Entered\n"); 
    
    __try {	    		   		    		
			ntstatus = RtlUnicodeStringToAnsiString(&AnsiName, UniName, TRUE);		
			
			//	260 是FileName 缓冲区的大小				
			if (AnsiName.Length < 260) {
				
				nameptr = (PCHAR)AnsiName.Buffer;
				//Convert into upper case and copy to buffer
				
				strcpy(Name, _strupr(nameptr));						    	    
			//	DbgPrint("NPUnicodeStringToChar : %s\n", Name);	
			}		  	
			RtlFreeAnsiString(&AnsiName);		 
	} 
	__except(EXCEPTION_EXECUTE_HANDLER) {
		DbgPrint("NPUnicodeStringToChar EXCEPTION_EXECUTE_HANDLER\n");	
		return FALSE;
	}
    return TRUE;
}      
/*
	对于本程序，该函数并没实际意义，仅仅是作为示例。
	
	该函数存在的目的，主要是让驱动开发者来决定哪个卷需要绑定，哪个卷不需要绑定。
	当有新卷被挂载是触发该回调函数。

*/
NTSTATUS
NPInstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();
    
    
		 DbgPrint("NPminifilter!NPInstanceSetup: Entered\n");
	
    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("NPminifilter!NPInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}

/*
	对于本程序来说，该回调函数没有任何意义，其中并没有做什么工作，仅作为示例。
	
	InstanceQueryTeardown为控制实例销毁函数。只函数只有手工解绑定的时候被触发。

*/

NTSTATUS
NPInstanceQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

		 DbgPrint("NPminifilter!NPInstanceQueryTeardown: Entered\n");
    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("NPminifilter!NPInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}

/*
		对本程序没有什么实际意义。
		
		InstanceTeardownStart为实例解绑定回调函数。当调用时已经决定解除绑定。


*/
VOID
NPInstanceTeardownStart (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();
    
    DbgPrint("NPminifilter!NPInstanceTeardownStart: Entered\n");

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("NPminifilter!NPInstanceTeardownStart: Entered\n") );
}

/*
	对本程序，没有什么实际意义
	
	InstanceTeardownComplete为实例解绑定的完成函数，当确定时调用解除绑定后的完成函数。

*/
VOID
NPInstanceTeardownComplete (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();
		DbgPrint("NPminifilter!NPInstanceTeardownComplete: Entered\n");
    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("NPminifilter!NPInstanceTeardownComplete: Entered\n") );
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    PSECURITY_DESCRIPTOR sd;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uniString;		//for communication port name
    
    UNREFERENCED_PARAMETER( RegistryPath );

		 DbgPrint("NPminifilter!DriverEntry: Entered\n");  

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("NPminifilter!DriverEntry: Entered\n") );

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,	//	本驱动的驱动对象
                                &FilterRegistration,//	i一个宣告注册的结构，在NPminifilter.h中有定义
                                &gFilterHandle );	//	返回注册成功后的过滤器句柄

    ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );	//	在这个函数之前，过滤是不起作用的。这个函数起到了开启的作用。

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }
    //
		//Communication Port
		//
		
		//	FltBuildDefaultSecurityDescriptor 以 FLT_RORT_ALL_ACCESS 权限来产生一个安全性的叙述子
    status  = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );
    
    if (!NT_SUCCESS( status )) {
       	goto final;
    }
                  
    RtlInitUnicodeString( &uniString, MINISPY_PORT_NAME );//	初始化通信端口名
    
		// InitializeObjectAttributes 来初始化对象属性（OBJECT_ATTRIBUTES）
    InitializeObjectAttributes( &oa,
                                &uniString,
                                OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                NULL,
                                sd );
		
		//	创建通信端口
    status = FltCreateCommunicationPort( gFilterHandle,
                                         &gServerPort,
                                         &oa,
                                         NULL,
                                         NPMiniConnect,
                                         NPMiniDisconnect,
                                         NPMiniMessage,
                                         1 );

    FltFreeSecurityDescriptor( sd );

    if (!NT_SUCCESS( status )) {
        goto final;
    }            

final :
    
    if (!NT_SUCCESS( status ) ) {

         if (NULL != gServerPort) {
             FltCloseCommunicationPort( gServerPort );
         }

         if (NULL != gFilterHandle) {
             FltUnregisterFilter( gFilterHandle );
         }       
    }
    
    DbgPrint("driver entry end!\n");  
     
    return status;
}

/*
		卸载回调函数。关闭通信端口，取消注册。
*/
NTSTATUS
NPUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

		DbgPrint("NPminifilter!NPUnload: Entered\n");  
    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("NPminifilter!NPUnload: Entered\n") );

		FltCloseCommunicationPort( gServerPort );
		
    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/

/*
	生成预操作回调函数：
		
			1、通过 FltGetFileNameInformation 函数获得文件名信息
			2、若获取成功，则通过 FltParseFileNameInformation 函数解析文件名
			3、若解析成功，则通过 NPUnicodeStringToChar 函数将文件名转换成大写字符串
			4、若转换成功，则使用 strstr 匹配 NOTEPADE.EXE 
			5、若返回值非空，则代表是，那么返回错误 

*/
FLT_PREOP_CALLBACK_STATUS
NPPreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
		//
		//	缓冲区，用来获取文件名
		//			
		char FileName[260] = "X:";
		
		NTSTATUS status;
		PFLT_FILE_NAME_INFORMATION nameInfo;

		//
		//	未使用参数，用宏掩盖使之不发生编译警告
		//
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

		//
		//	检测分页代码
		//
    PAGED_CODE();	        

		__try {	                             								
		    status = FltGetFileNameInformation( Data,
		                                        FLT_FILE_NAME_NORMALIZED |
		                                            FLT_FILE_NAME_QUERY_DEFAULT,
		                                        &nameInfo );
		    if (NT_SUCCESS( status )) {
		    		//如果成功，则解析文件名信息，然后比较其中是否有NOTEPAD.EXE
		    	//	 DbgPrint("NPPreCreate：FilGetFileNameInformation success\n");
		    		if (gCommand == ENUM_BLOCK) {
		    			
						    FltParseFileNameInformation( nameInfo );
								if (NPUnicodeStringToChar(&nameInfo->Name, FileName)) {		
								//	DbgPrint("NPPreCreate:FilGetFileNameInformation success %s",FileName);
								//
								//	strstr() 函数搜索一个字符串在另一个字符串中的第一次出现。
								//	找到所搜索的字符串，则该函数返回第一次匹配的字符串的地址；如果未找到所搜索的字符串，则返回NULL。
								//	函数原型：extern char *strstr(char *str1, const char *str2);
								//	str1: 被查找目标，str2: 要查找对象
								//	返回值：若str2是str1的子串，则先确定str2在str1的第一次出现的位置，并返回此str1在str2首位置的地址。
								//	如果str2不是str1的子串，则返回NULL。
								//
								//	自己加了个禁止 EXPLORER.EXE	运行
								//
								    if (strstr(FileName, "NOTEPAD.EXE") != NULL || strstr(FileName,"EXPLORER.EXE") != NULL) {

												Data->IoStatus.Status = STATUS_ACCESS_DENIED;
												Data->IoStatus.Information = 0;
												FltReleaseFileNameInformation( nameInfo );
												return FLT_PREOP_COMPLETE;						    		
								    }																			
								}						 
						}   			    
				    //release resource
				    FltReleaseFileNameInformation( nameInfo );        
		    }                                    
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
		    DbgPrint("NPPreCreate EXCEPTION_EXECUTE_HANDLER\n");				
		}

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

/*

	生成后操作回调：对本程序没有什么实际意义，仅作示例

*/

FLT_POSTOP_CALLBACK_STATUS
NPPostCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
			
		
    FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
    PFLT_FILE_NAME_INFORMATION nameInfo;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );
		
//		DbgPrint("NPPostCreate enter\n");
		
    //
    //  If this create was failing anyway, don't bother scanning now.
    //

    if (!NT_SUCCESS( Data->IoStatus.Status ) ||
        (STATUS_REPARSE == Data->IoStatus.Status)) {

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  Check if we are interested in this file.
    //

    status = FltGetFileNameInformation( Data,
                                        FLT_FILE_NAME_NORMALIZED |
                                            FLT_FILE_NAME_QUERY_DEFAULT,
                                        &nameInfo );

    if (!NT_SUCCESS( status )) {

        return FLT_POSTOP_FINISHED_PROCESSING;
    }
    		
    return returnStatus;
}

//
//	是用户态与内核态建立链接时会调用到的函数
//
NTSTATUS
NPMiniConnect(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    )
{
		DbgPrint("[mini-filter] NPMiniConnect\n");
    PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie );

    ASSERT( gClientPort == NULL );
    gClientPort = ClientPort;
    return STATUS_SUCCESS;
}

//
//	是用户态与内核态断开链接时会调用到的函数
//
VOID
NPMiniDisconnect(
    __in_opt PVOID ConnectionCookie
   )
{		
    PAGED_CODE();
    UNREFERENCED_PARAMETER( ConnectionCookie );
		DbgPrint("[mini-filter] NPMiniDisconnect\n");
    
    //  Close our handle
    FltCloseClientPort( gFilterHandle, &gClientPort );
}

//
//	是用户态和内核态传送数据时内核态会调用到的函数
//
NTSTATUS
NPMiniMessage (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    )
{
		
    NPMINI_COMMAND command;
    NTSTATUS status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( ConnectionCookie );
    UNREFERENCED_PARAMETER( OutputBufferSize );
    UNREFERENCED_PARAMETER( OutputBuffer );
    
		DbgPrint("[mini-filter] NPMiniMessage\n");
    
    //                      **** PLEASE READ ****    
    //  The INPUT and OUTPUT buffers are raw user mode addresses.  The filter
    //  manager has already done a ProbedForRead (on InputBuffer) and
    //  ProbedForWrite (on OutputBuffer) which guarentees they are valid
    //  addresses based on the access (user mode vs. kernel mode).  The
    //  minifilter does not need to do their own probe.
    //  The filter manager is NOT doing any alignment checking on the pointers.
    //  The minifilter must do this themselves if they care (see below).
    //  The minifilter MUST continue to use a try/except around any access to
    //  these buffers.    

    if ((InputBuffer != NULL) &&
        (InputBufferSize >= (FIELD_OFFSET(COMMAND_MESSAGE,Command) +
                             sizeof(NPMINI_COMMAND)))) {

        try  {
            //  Probe and capture input message: the message is raw user mode
            //  buffer, so need to protect with exception handler
            command = ((PCOMMAND_MESSAGE) InputBuffer)->Command;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return GetExceptionCode();
        }

        switch (command) {
            //	通过
            case ENUM_PASS:
            {            		
            		DbgPrint("[mini-filter] ENUM_PASS");
								gCommand = ENUM_PASS;
								status = STATUS_SUCCESS;         
            		break;
            }
            //	禁止
            case ENUM_BLOCK:
            {            		
            		DbgPrint("[mini-filter] ENUM_BLOCK");
								gCommand = ENUM_BLOCK;
								status = STATUS_SUCCESS;                          
            		break;
            }               		
            
            default:
            		DbgPrint("[mini-filter] default");
                status = STATUS_INVALID_PARAMETER;
                break;
        }
    } else {

        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}