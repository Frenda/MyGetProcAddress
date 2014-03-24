#include "MyGetProcAddress.h"

PIMAGE_DATA_DIRECTORY GetDataDirectory(DWORD ImageBase, DWORD Index)
{
	PIMAGE_DOS_HEADER DosHead;
	PIMAGE_NT_HEADERS PeHead;
	DosHead = (PIMAGE_DOS_HEADER)ImageBase;
	PeHead = (PIMAGE_NT_HEADERS)((DWORD)DosHead + DosHead->e_lfanew);
	return (PIMAGE_DATA_DIRECTORY)(&PeHead->OptionalHeader.DataDirectory[Index]);
}

PDWORD  GetApiSetMapHead()
{
	DWORD *SetMapHead = 0;
	__asm
	{
		mov eax, fs:[0x30]
		mov eax, [eax + 0x38]
		mov SetMapHead, eax
	}
	return SetMapHead;
}

FARPROC GetExportByName(HMODULE hModule, char *ProcName)
{
	int cmp;
	char *ApiName;
	FARPROC ApiAddress = 0;
	WORD Ordinal, *NameOrd;
	DWORD ExportSize, *Ent, *Eat, HigthIndex, LowIndex = 0, MidIndex;
	PIMAGE_EXPORT_DIRECTORY ExportTable;
	PIMAGE_DATA_DIRECTORY DataDirec;
	DataDirec = GetDataDirectory((DWORD)hModule, IMAGE_DIRECTORY_ENTRY_EXPORT);
	ExportTable = (PIMAGE_EXPORT_DIRECTORY)((DWORD)hModule + DataDirec->VirtualAddress);
	ExportSize = DataDirec->Size;
	if (ExportTable)
	{
		Eat = (DWORD *)((DWORD)hModule + ExportTable->AddressOfFunctions);
		Ent = (DWORD *)((DWORD)hModule + ExportTable->AddressOfNames);
		NameOrd = (WORD *)((DWORD)hModule + ExportTable->AddressOfNameOrdinals);
		HigthIndex = ExportTable->NumberOfNames;
		while (LowIndex <= HigthIndex)
		{
			MidIndex = (LowIndex + HigthIndex) / 2;
			ApiName = (char *)((DWORD)hModule + Ent[MidIndex]);
			cmp = strcmp(ProcName, ApiName);
			if (cmp < 0)
			{
				HigthIndex = MidIndex;
				continue;
			}
			if (cmp > 0)
			{
				LowIndex = MidIndex;
				continue;
			}
			if (cmp == 0)
			{
				Ordinal = NameOrd[MidIndex];
				break;
			}
		}
		if (LowIndex > HigthIndex)
			return 0;
		ApiAddress = (FARPROC)((DWORD)hModule + Eat[Ordinal]);
		if ((DWORD)ApiAddress >= (DWORD)ExportTable && ((DWORD)ApiAddress < ((DWORD)ExportTable + ExportSize)))
			ApiAddress = FileNameRedirection(hModule, (char *)ApiAddress);
	}
	return ApiAddress;
}

FARPROC GetExportByOrdinal(HMODULE hModule, WORD Ordinal)
{
	FARPROC ApiAddress = 0;
	WORD *NameOrd;
	DWORD ExportSize, *Eat;
	PIMAGE_DATA_DIRECTORY DataDirec;
	PIMAGE_EXPORT_DIRECTORY ExportTable;
	DataDirec = GetDataDirectory((DWORD)hModule, IMAGE_DIRECTORY_ENTRY_EXPORT);
	ExportTable = (PIMAGE_EXPORT_DIRECTORY)((DWORD)hModule + DataDirec->VirtualAddress);
	ExportSize = DataDirec->Size;
	if (ExportTable)
	{
		Eat = (DWORD *)((DWORD)hModule + ExportTable->AddressOfFunctions);
		NameOrd = (WORD *)((DWORD)hModule + ExportTable->AddressOfNameOrdinals);
		ApiAddress = (FARPROC)((Eat[Ordinal - ExportTable->Base] != 0) ? ((DWORD)hModule + Eat[Ordinal - ExportTable->Base]) : 0);
		if (((DWORD)ApiAddress >= (DWORD)ExportTable) && ((DWORD)ApiAddress < ((DWORD)ExportTable + ExportSize)))
			ApiAddress = FileNameRedirection(hModule, (char *)ApiAddress);
	}
	return ApiAddress;
}

 void ResolveApiSet(WCHAR *ApiSetName, WCHAR *HostName)
{
	 WCHAR *NameBuffer;
	 WCHAR LibName[64];
	 DWORD LibNameSize, HostNameSize, *Version;
	 PAPI_SET_NAMESPACE_ARRAY_V2 SetMapHead_v2;
	 PAPI_SET_VALUE_ARRAY_V2 SetMapHost_v2;
	 PAPI_SET_NAMESPACE_ARRAY_V4 SetMapHead_v4;
	 PAPI_SET_VALUE_ARRAY_V4 SetMapHost_v4;
	 Version = GetApiSetMapHead();
	 if (Version)
	 {
		 switch (*Version)
		 {
		 case 2:
		 {
				   SetMapHead_v2 = (PAPI_SET_NAMESPACE_ARRAY_V2)Version;
				   for (DWORD i = 0; i < SetMapHead_v2->Count; ++i)
				   {
					   NameBuffer = (WCHAR *)((DWORD)SetMapHead_v2 + SetMapHead_v2->Entry[i].NameOffset);
					   LibNameSize = SetMapHead_v2->Entry[i].NameLength;
					   wcsncpy_s(LibName, 64, NameBuffer, LibNameSize / sizeof(WCHAR));
					   if (!_wcsicmp((WCHAR *)(ApiSetName + 4), LibName))
					   {
						   SetMapHost_v2 = (PAPI_SET_VALUE_ARRAY_V2)((DWORD)SetMapHead_v2 + SetMapHead_v2->Entry[i].DataOffset);
						   NameBuffer = (WCHAR *)((DWORD)SetMapHead_v2 + SetMapHost_v2->Entry[SetMapHost_v2->Count - 1].ValueOffset);
						   HostNameSize = SetMapHost_v2->Entry[SetMapHost_v2->Count - 1].ValueLength;
						   wcsncpy_s(HostName, 64, NameBuffer, HostNameSize / sizeof(WCHAR));
						   return;
					   }
				   }
				   break;
		 }
		 case 4:
		 {
				   SetMapHead_v4 = (PAPI_SET_NAMESPACE_ARRAY_V4)Version;
				   for (DWORD i = 0; i < SetMapHead_v4->Count; ++i)
				   {
					   NameBuffer = (WCHAR *)((DWORD)SetMapHead_v4 + SetMapHead_v4->Entry[i].NameOffset);
					   LibNameSize = SetMapHead_v4->Entry[i].NameLength;
					   wcsncpy_s(LibName, 64, NameBuffer, LibNameSize / sizeof(WCHAR));
					   if (!_wcsicmp((WCHAR *)(ApiSetName + 4), LibName))
					   {
						   SetMapHost_v4 = (PAPI_SET_VALUE_ARRAY_V4)((DWORD)SetMapHead_v4 + SetMapHead_v4->Entry[i].DataOffset);
						   HostNameSize = SetMapHost_v4->Entry[SetMapHost_v4->Count - 1].ValueLength;
						   NameBuffer = (WCHAR *)((DWORD)SetMapHead_v4 + SetMapHost_v4->Entry[SetMapHost_v4->Count - 1].ValueOffset);
						   wcsncpy_s(HostName, 64, NameBuffer, HostNameSize / sizeof(WCHAR));
						   return;
					   }
				   }
				   break;
		 }
		 default:
			 break;
		 }
	 }
}

FARPROC FileNameRedirection(HMODULE hModule, char *RedirectionName)
{
	char *ptr, *ProcName;
	char Buffer[64];
	WCHAR DllName[64];
	FARPROC ApiAddress = 0;
	strcpy_s(Buffer, 64, RedirectionName);
	ptr = strchr(Buffer, '.');
	if (ptr)
	{
		*ptr = 0;
		MultiByteToWideChar(CP_ACP, 0, Buffer, sizeof(Buffer), DllName, 64);
		if (!_wcsnicmp(DllName, L"api-", 4))
		{
			ResolveApiSet(DllName, DllName);
			goto get_api_address;
		}
		else
		{
		get_api_address:
			hModule = LoadLibraryW(DllName);
			if (hModule)
			{
				ProcName = (char *)(ptr + 1);
				ApiAddress = GetExportByName(hModule, ProcName);
				FreeLibrary(hModule);
			}
		}
	}
	return ApiAddress;
}

FARPROC MyGetProcAddress(HMODULE hModule, char *ProcName)
{
	FARPROC ProcAddress = 0;
	DWORD Ordinal = (DWORD)ProcName;
	if (hModule)
	{
		if (HIWORD((DWORD)ProcName))
		{
			ProcAddress = GetExportByName(hModule, ProcName);
		}
		else
		{
			Ordinal &= 0x0000FFFF;
			ProcAddress = GetExportByOrdinal(hModule, (WORD)Ordinal);
		}
	}
	return ProcAddress;
}