/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "perf.h"

#include <engine/shared/config.h>

#if defined(CONF_FAMILY_WINDOWS)
#include <windows.h>
#include <tlhelp32.h>
#endif

void ApplyPerformanceSettings()
{
#if defined(CONF_FAMILY_WINDOWS)
	SetPriorityClass(GetCurrentProcess(), g_Config.m_EcHighPriority ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);

	if(g_Config.m_EcDiscordLowPriority)
	{
		const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if(hSnapshot != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32W Entry{};
			Entry.dwSize = sizeof(Entry);
			if(Process32FirstW(hSnapshot, &Entry))
			{
				do
				{
					if(_wcsicmp(Entry.szExeFile, L"Discord.exe") != 0)
						continue;
					const HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, Entry.th32ProcessID);
					if(hProcess != nullptr)
					{
						SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS);
						CloseHandle(hProcess);
					}
				} while(Process32NextW(hSnapshot, &Entry));
			}
			CloseHandle(hSnapshot);
		}
	}
#endif
}
