#include "pch.h"
#include "Utils.h"
#include "Service.h"

void Utils::PrintServerStatus(ServiceRef service)
{
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, 0 });

	int32 sessionCount = service->GetCurrentSessionCount();

	PROCESS_MEMORY_COUNTERS_EX pmc;
	::GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
	size_t memUsed = pmc.PrivateUsage / 1024 / 1024;  // MB

	wcout << L"============================================" << endl;
	wcout << L"[MONITOR] Time   : " << Utils::GetCurrentTime() << endl;
	wcout << L"   - Users       : " << service->GetCurrentSessionCount() << endl;
	wcout << L"   - Logic Queue : " << GLogicJobCount << endl;
	wcout << L"   - DB Queue    : " << GDBJobCount << endl;
	wcout << L"   - Memory      : " << memUsed << L"MB" << endl;
	wcout << L"============================================" << endl;
}
