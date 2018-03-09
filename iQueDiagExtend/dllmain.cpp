#include "stdafx.h"
#include <fstream>
#include <string>
#include "include\MinHook.h"

//#define DEVELOPER_MODE

typedef int(*CommandHandler_func)(char* input);
CommandHandler_func origCommandHandler;

const size_t CommandHandlerAddr = 0x401970;

int CmdDumpNandRaw()
{
	int* diag_handle = (int*)0x40E198;
	int* handlesBase = (int*)0x4326C0;
	const auto __BBC_CheckHandle = (int(*)(int handle))(0x403A20);
	const auto __bbc_direct_readblocks = (int(*)(int vtable, int blk, int count, unsigned char *buffer, unsigned char *spare))(0x40AD80);

	int res = __BBC_CheckHandle(*diag_handle);
	if (res)
	{
		printf("__BBC_CheckHandle failed, did you init with BBCInit (B)?\n");
		return 0;
	}

	int* handle2 = (int*)(handlesBase[*diag_handle]);

	int* direct_ptrs = *(int**)(handle2 + 0x110);

	FILE *f = fopen("nand.bin", "wb");
	FILE *f2 = fopen("spare.bin", "wb");
	unsigned char buff[0x4000];
	unsigned char spare[0x10];
	for (int i = 0; i < 0x1000; i++)
	{
		__bbc_direct_readblocks((int)direct_ptrs[0], i, 1, buff, spare);
		fwrite(buff, 1, 0x4000, f);
		fwrite(spare, 1, 0x10, f2);

		fflush(f);
		fflush(f2);
	}

	fclose(f);
	fclose(f2);

	return 0;
}

int CmdDumpFile(char *str)
{
	int* diag_handle = (int*)0x40E198;
	int* handlesBase = (int*)0x4326C0;
	const auto __BBC_CheckHandle = (int(*)(int handle))(0x403A20);
	const auto __BBC_ObjectSize = (int(*)(int vtable, char *path))(0x407460);
	const auto __BBC_GetObject = (int(*)(int vtable, char *path, unsigned char*buff, unsigned int size))(0x4074D0);

	int res = __BBC_CheckHandle(*diag_handle);
	if (res)
	{
		printf("__BBC_CheckHandle failed, did you init with BBCInit (B)?\n");
		return 0;
	}

	char filename[256];
	if (sscanf(str, "%s", &filename) >= 1)
	{
		int fileSize = __BBC_ObjectSize(*diag_handle, filename);
		printf("fileSize = %08X (%d) bytes\n", fileSize, fileSize);
		if (fileSize > 0)
		{
			unsigned char *buffer = (unsigned char*)malloc(fileSize);

			printf("__BBC_GetObject started\n", fileSize, fileSize);
			__BBC_GetObject(*diag_handle, filename, buffer, fileSize);
			printf("__BBC_GetObject finished\n", fileSize, fileSize);

			FILE *f = fopen(filename, "wb");
			fwrite(buffer, 1, fileSize, f);
			fclose(f);

		}
	}

	return 0;
}

int CmdWriteFile(char *str)
{
	int* diag_handle = (int*)0x40E198;
	int* handlesBase = (int*)0x4326C0;
	const auto __BBC_CheckHandle = (int(*)(int handle))(0x403A20);
	const auto __BBC_ObjectSize = (int(*)(int vtable, char *path))(0x407460);
	const auto __BBC_GetObject = (int(*)(int vtable, char *path, unsigned char*buff, unsigned int size))(0x4074D0);
	const auto __BBC_SetObject = (int(*)(int vtable, char *path, char *temp_path, unsigned char*buff, unsigned int size))(0x407550);

	int res = __BBC_CheckHandle(*diag_handle);
	if (res)
	{
		printf("__BBC_CheckHandle failed, did you init with BBCInit (B)?\n");
		return 0;
	}

	char filename[256];
	if (sscanf(str, "%s", &filename) >= 1)
	{
		FILE *f = fopen(filename, "rb");

		if (f)
		{
			fseek(f, 0, SEEK_END);
			int fileSize = ftell(f);
			fseek(f, 0, SEEK_SET);

			printf("__BBC_SetObject fileSize = %d bytes\n", fileSize);

			unsigned char *buffer = (unsigned char*)malloc(fileSize);
			fread(buffer, 1, fileSize, f);
			fclose(f);

			printf("__BBC_SetObject started\n");
			__BBC_SetObject(*diag_handle, filename, "temp.tmp", buffer, fileSize);
			printf("__BBC_SetObject finished\n");

			free(buffer);
		}
		else
		{
			printf("%s was not found.\n", filename);
		}
	}

	return 0;
}

int CmdGetDirListing()
{
	int* diag_handle = (int*)0x40E198;
	int* handlesBase = (int*)0x4326C0;
	const auto __BBC_CheckHandle = (int(*)(int handle))(0x403A20);
	const auto __BBC_FReadDir = (int(*)(int vtable, unsigned char*buff, unsigned int size))(0x4096B0);

	int res = __BBC_CheckHandle(*diag_handle);
	if (res)
	{
		printf("__BBC_CheckHandle failed, did you init with BBCInit (B)?\n");
		return 0;
	}

	unsigned char *buffer = (unsigned char*)calloc(0x2008, sizeof(unsigned char));
	if (buffer)
	{
		printf("__BBC_FReadDir started\n");
		__BBC_FReadDir(handlesBase[*diag_handle], buffer, 410);
		printf("__BBC_FReadDir finished\n");

		int idx = 0;
		while (buffer[idx] != 0)
		{
			printf("\"%s\" \t\t size = %d bytes\n", &buffer[idx], *(unsigned int*)&buffer[idx + 0x10]);
			idx += 0x14;
		}

		//FILE *f = fopen("readdir.bin", "wb");
		//fwrite(buffer, 1, 0x2008, f);
		//fclose(f);
	}
	free(buffer);

	return 0;
}


int __cdecl CommandHandlerHook(char* input)
{
	char inputChar = *input;

	switch (inputChar)
	{
	case 'X':
	case 'x':
		printf("\tiQueDiagExtend Commands:\n");
		printf("\t1\t - reads nand from ique to nand.bin/spare.bin\n");
		printf("\t3 [file] - reads [file] from ique [to file]\n");
#ifdef DEVELOPER_MODE
		printf("\t4 [file] - write [file] to ique\n");
#endif
		printf("\t5 List all files on ique\n");
		break;
	case '1':
		return CmdDumpNandRaw();
		break;
	case '3':
		return CmdDumpFile(input + 1);
		break;
#ifdef DEVELOPER_MODE
	case '4':
		return CmdWriteFile(input + 1);
		break;
#endif
	case '5':
		return CmdGetDirListing();
		break;

	default:
		return origCommandHandler(input);
	}

	return 0;
}

// this'll give our dll an export directory, so that we can hack this dll into ique_diag.exe as an import
__declspec(dllexport) void UselessExport()
{

}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		MH_Initialize();

		MH_CreateHook((void*)CommandHandlerAddr, CommandHandlerHook, (LPVOID*)&origCommandHandler);
		MH_EnableHook((void*)CommandHandlerAddr);
	}

	return TRUE;
}
