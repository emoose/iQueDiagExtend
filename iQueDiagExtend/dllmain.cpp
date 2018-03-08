#include "stdafx.h"
#include <fstream>
#include <string>
#include "include\MinHook.h"

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

int __cdecl CommandHandlerHook(char* input)
{
	char inputChar = *input;

	switch (inputChar)
	{
	case 'X':
	case 'x':
		printf("\tiQueDiagExtend Commands:\n");
		printf("\t1\t - reads nand from ique to nand.bin/spare.bin\n");
		break;
	case '1':
		return CmdDumpNandRaw();
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
