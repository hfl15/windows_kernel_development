#include "ntddk.h"

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((void)0))

VOID DriverUnload(PDRIVER_OBJECT driver)
{
	DbgPrint("firstFilter: our driver is unloading...\r\n");
}


//the function is entry.That is to say it's the first to excute the program.
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT driver,
    PUNICODE_STRING reg_path
    )
{
	DbgPrint("firstFilter: Hello everyone, welcome to Technical Sharon! \r\n");
	
	driver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}

