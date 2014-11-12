#ifndef         __XBOXKRNL_H__
#define         __XBOXKRNL_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include "cromwell_types.h"
#include "types.h"

/* partially based on RectOS source code */

#define OBJ_CASE_INSENSITIVE		0x00000040L
#define PAGE_READWRITE			0x4
#define PAGE_EXECUTE_READWRITE		0x40
#define STATUS_NO_MEMORY		0xc0000017
#define STATUS_SUCCESS  		0x00000000
#define MEM_RESERVE			0x00002000
#define MEM_COMMIT			0x00001000

/* NT Data Types */

typedef VOID
(*PIO_APC_ROUTINE)(PVOID ApcContext,
                   struct _IO_STATUS_BLOCK* IoStatusBlock,
                   ULONG Reserved);

/* Xbox Kernel Functions */
extern VOID __attribute__((__stdcall__))
(*RtlInitAnsiString)(PANSI_STRING, PCSZ);
extern NTSTATUS __attribute__((__stdcall__))
(*NtClose)(HANDLE);
extern PVOID __attribute__((__stdcall__))
(*MmAllocateContiguousMemoryEx)(ULONG, PHYSICAL_ADDRESS, PHYSICAL_ADDRESS, ULONG, ULONG);
extern NTSTATUS __attribute__((__stdcall__))
(*NtReadFile)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER);
extern VOID __attribute__((__stdcall__))
(*MmFreeContiguousMemory)(PVOID);
extern PHYSICAL_ADDRESS __attribute__((__stdcall__))
(*MmGetPhysicalAddress)(PVOID);
extern NTSTATUS __attribute__((__stdcall__))
(*NtAllocateVirtualMemory)(PVOID*, ULONG, PULONG, ULONG, ULONG);
extern PANSI_STRING XeImageFileName;
extern VOID __attribute__((__stdcall__))
(*RtlZeroMemory)(PVOID Destination,ULONG Length);
extern ULONG __attribute__((__stdcall__))
(*MmQueryAllocationSize)(PVOID   BaseAddress);
extern VOID __attribute__((__stdcall__))
(*MmPersistContiguousMemory)(PVOID BaseAddress,ULONG NumberOfBytes,BOOLEAN Persist);
extern VOID __attribute__((__stdcall__))
(*RtlMoveMemory)(VOID *Destination,CONST VOID *Source,SIZE_T Length);
extern NTSTATUS __attribute__((__stdcall__))(*NtWriteFile)(
        HANDLE  FileHandle,
        PVOID   Event,
        PVOID   ApcRoutine,
        PVOID   ApcContext,
        PVOID   IoStatusBlock,
        PVOID   Buffer,
        ULONG   Length,
        PVOID   ByteOffset
);
extern NTSTATUS __attribute__((__stdcall__))(*NtCreateFile) (
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize OPTIONAL,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions
);
typedef enum _FILE_INFORMATION_CLASS
{
    FileDirectoryInformation        = 1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileCopyOnWriteInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileObjectIdInformation,
    FileTrackingInformation,
    FileOleDirectoryInformation,
    FileContentIndexInformation,
    FileInheritContentIndexInformation,
    FileOleInformation,
    FileMaximumInformation
}
FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

extern NTSTATUS __attribute__((__stdcall__))(*NtQueryInformationFile)(
        HANDLE FileHandle,
        PIO_STATUS_BLOCK IoStatusBlock,
        PVOID FileInformation,
        ULONG FileInformationLength,
        FILE_INFORMATION_CLASS FileInformationClass
);
extern NTSTATUS __attribute__((__stdcall__))(*NtSetInformationFile)(
        HANDLE  FileHandle,
        PVOID   IoStatusBlock,
        PVOID   FileInformation,
        ULONG   Length,
        ULONG   FileInformationClass
);
extern NTSTATUS __attribute__((__stdcall__))(*IoCreateSymbolicLink)(
	PANSI_STRING SymbolicLinkName,
	PANSI_STRING DeviceName
);
extern NTSTATUS __attribute__((__stdcall__))(*IoDeleteSymbolicLink)(
	PANSI_STRING SymbolicLinkName
);
extern NTSTATUS __attribute__((__stdcall__))(*IoDismountVolumeByName)(
	PANSI_STRING DeviceName
);

#ifdef  __cplusplus
};
#endif

#endif          //__XBOXKRNL_H__

