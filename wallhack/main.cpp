// I've already written all offsets that we'll use and copied function definitions
// I'll use functions from native api from this moment because they are a bit faster than WINAPI functions
#include <Windows.h>
#include <TlHelp32.h>
DWORD LocalPlayer = 11124188;
DWORD EntityList = 78089124;
DWORD glowObject = 83450352;
DWORD glowIndex = 41744;
DWORD oDormant = 0xE9;
DWORD oTeam = 0xF0;
DWORD bSpottedMask = 2428;
// offsets upper
typedef NTSTATUS(NTAPI* NtRVM)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToRead, PULONG NumberOfBytesReaded);
typedef NTSTATUS(NTAPI* NtWVM) (HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToRead, PULONG NumberOfBytesReaded);
// definitions
HANDLE process;
DWORD bClient, bEngine, LocalBase;
DWORD processid;
HMODULE lib = LoadLibrary("ntdll.dll");
// functions for reading/writing memory
NtRVM NtReadVirtualMemory = (NtRVM)GetProcAddress(lib, "NtReadVirtualMemory");
NtWVM NtWriteVirtualMemory = (NtWVM)GetProcAddress(lib, "NtWriteVirtualMemory");

//player struct
struct player
{
	float r, g, b, a;
	bool backlight;
};

DWORD GetProcess(LPCSTR name) // function to get processid and it's handle
{
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	do
	{
		if (!strcmp(pe.szExeFile, name))
			break;
	} while (Process32Next(snap, &pe));
	CloseHandle(snap);
	process = OpenProcess(PROCESS_ALL_ACCESS, 0, pe.th32ProcessID);
	return pe.th32ProcessID;
}

template<typename val>
val Read(DWORD value) // function for reading memory from process
{
	val num;
	NtReadVirtualMemory(process, (LPVOID)value, &num, sizeof(num), 0);
	return num;
}

template<typename val1>
void Write(DWORD value, val1 num) // function for writing memory into process
{
	NtWriteVirtualMemory(process, (LPVOID)value, &num, sizeof(val1), 0);
}

DWORD GetModule(LPCSTR modulename) // get process module
{
	MODULEENTRY32 mod;
	mod.dwSize = sizeof(mod);
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processid);
	do
	{
		if (!strcmp(mod.szModule, modulename))
			break;
	} while (Module32Next(snap, &mod));
	CloseHandle(snap);
	return (DWORD)mod.modBaseAddr;
}

void Startup() // do startup things
{
	processid = GetProcess("csgo.exe");
	bClient = GetModule("client.dll");
	bEngine = GetModule("engine.dll");
	LocalBase = Read<int>(bClient + LocalPlayer);
}

void makelight(int human, player man) // show player
{
	int GlowIndex = Read<int>(human + glowIndex);
	int GlowObject = Read<int>(bClient + glowObject);
	int calculation = GlowIndex * 0x38 + 0x4;
	Write<float>(GlowObject + calculation, man.r);
	calculation = GlowIndex * 0x38 + 0x8;
	Write<float>(GlowObject + calculation, man.g);
	calculation = GlowIndex * 0x38 + 0xC;
	Write<float>(GlowObject + calculation, man.b);
	calculation = GlowIndex * 0x38 + 0x10;
	Write<float>(GlowObject + calculation, man.a);
	calculation = GlowIndex * 0x38 + 0x24;
	Write<bool>(GlowObject + calculation, man.backlight);
}

int main() // main function
{
	// player structs
	player invisible = player{ 1, 0, 0, 1, true };
	player vis = player{ 0, 1, 0, 1, true };
	Startup(); // startup func
	while (true) // infinite loop
	{
		for (int i = 0; i <= 64; ++i) // player's loop
		{
			int human = Read<int>(bClient + EntityList + (i - 1) * 0x10); // get player
			int team = Read<int>(human + oTeam); // get player's team
			int myteam = Read<int>(LocalBase + oTeam); // get my team
			bool dormant = Read<bool>(human + oDormant); // check if object is player
			int visible = Read<int>(human + bSpottedMask); // if player is visible
			if (!dormant && myteam != team) // if enemy
			{
				if (visible) // if visible
					makelight(human, vis); // show with special color
				else // not visible
					makelight(human, invisible); // show with another color
			}
		}
		Sleep(10); // sleep for 10 ms to decrease cpu loading
	}
	return 0; // successfull end
}