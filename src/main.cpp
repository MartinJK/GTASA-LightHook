/*****************************************************************************\
Copyright (C) 2013-2014 <martinjk at outloook dot com>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
\*****************************************************************************/

/* NOTE: 
	this project is outdated, still working, written in a dirty way, needs to be cleaned up!
	this executable does not inject itself into GTASA.exe, it modifies it's memory via ReadProcess/WriteProcessMemory!
	this code is written for SA:MP server who does not allow CLEO/ASI mods (usally they use their "anticheat" clients...)
*/
#include <Windows.h>
#include <iostream>
#include <string>

#include "../include/d3keys.h"

HANDLE hProc;

// Find other way to block it
unsigned int iBlock = 0;
unsigned int iStatus = 1;
bool bOn = false;
char AnsiBuffer[255]; 
const char * destPtr = (const char *)AnsiBuffer;
DWORD dwStatus = 1;
DWORD dwLight = 1;
HWND hwnd = NULL;
TCHAR szWindowText[100];
		
bool EnableDebugPrivilege() 
{ 
    HANDLE hThis = GetCurrentProcess(); 
    HANDLE hToken; 
    OpenProcessToken(hThis, TOKEN_ADJUST_PRIVILEGES, &hToken); 
    LUID luid; 
    LookupPrivilegeValue(0, TEXT("setDebugPrivilege"), &luid); 
    TOKEN_PRIVILEGES priv; 
    priv.PrivilegeCount = 1; 
    priv.Privileges[0].Luid = luid; 
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
    AdjustTokenPrivileges(hToken, false, &priv, 0, 0, 0); 
    CloseHandle(hToken); 
    CloseHandle(hThis); 
    return true; 
} 

DWORD dwVehicle = 0;
void setLightStatus(DWORD _dwLight = NULL, DWORD _dwStatus = NULL) 
{
	// Heavy CPU usage(10-15% on Intel i7 4790k due ReadProcessMemory)
    DWORD dwPlayer = 0;
    DWORD dwVehiclePlayer = 0;
    ReadProcessMemory(hProc,(LPCVOID)0xBA18FC,&dwVehicle,sizeof(dwVehicle), 0);

	// Check if our vehicle isn't a car
	BYTE dwVehicleType;
	ReadProcessMemory(hProc, (LPCVOID)(dwVehicle + 1424), &dwVehicleType, sizeof(dwVehicleType),0);
	if(dwVehicleType != 0)
		return;

    ReadProcessMemory(hProc, (LPCVOID)0xB6F5F0, &dwPlayer, sizeof(dwPlayer), 0);
    ReadProcessMemory(hProc, (LPCVOID)(dwVehicle + 0x460), &dwVehiclePlayer, sizeof(dwVehiclePlayer), 0); 
    if(dwVehiclePlayer == dwPlayer) 
	{
         if(dwVehicle != 0 && hProc) 
		 {
            DWORD currentLightState = 0;
            ReadProcessMemory(hProc,(LPCVOID)(dwVehicle + 1440 + 16), &currentLightState, sizeof(currentLightState), 0);
            int _iResult = (_dwStatus << 2 * _dwLight) | currentLightState & ~(3 << 2 * _dwLight);
            WriteProcessMemory(hProc,(LPVOID)(dwVehicle + 1440 + 16),&_iResult,sizeof(_iResult), 0);
         }
    }
}

void DXSendInput(WORD key)
{
    INPUT inp;

    inp.type = INPUT_KEYBOARD;
    inp.ki.wScan = key;

    SendInput(1, &inp, sizeof(INPUT)); //press
    Sleep(10);                        //wait short time (10 ms)

    inp.ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(1, &inp, sizeof(INPUT)); //release it
} 

void TCharToChar(const wchar_t* Src, char* Dest, int Size)
{
	WideCharToMultiByte(CP_ACP, 0, Src, wcslen(Src)+1, Dest , Size, NULL, NULL);
} 

void ForceLightWhile()
{
	if(strcpy_s(AnsiBuffer,"GTA:SA:MP")) 
	{
		if(GetAsyncKeyState(0x55) && iBlock == 0) 
		{
			if(bOn) 
			{
				iBlock = 2;
				Sleep(200);
				bOn = false;

				// Turn all lights on(old "true" state)
				setLightStatus(0, 0);
				setLightStatus(1, 0);
				setLightStatus(3, 0);
			}
			else if(!bOn && iBlock == 0) 
			{
				iBlock = 2;			
				Sleep(200);
				bOn = true;
			}
		}

		if(GetAsyncKeyState(0x4F))
		{
			switch (iStatus)
			{
				case 1:
					Sleep(200);
					iStatus = 2;
					break;
				case 2:
					Sleep(200);
					iStatus = 3;
					break;
				case 3:
					Sleep(200);
					iStatus = 4;
					break;
				case 4:
					Sleep(200);
					iStatus = 1;
					break;
				default:
					break;
			}
		} 
		if(iBlock > 0)
			iBlock--;

		if(bOn == true) 
		{
			setLightStatus(dwLight, dwStatus);
			if(dwStatus == 1) 
			{
				setLightStatus(0, 0);
				dwStatus = 0;
			} 
			else 
			{
				setLightStatus(0, 1);
				dwStatus = 1;
			}
                        
			setLightStatus(3, 0);

			switch (iStatus)
			{
				case 1:
					Sleep(50);
					break;
				case 2:
					Sleep(100);
					break;
				case 3:
					Sleep(200);
					break;
				case 4:
					Sleep(300);
					break;
				default:
					break;
			}

			setLightStatus(3, 1);
	
			switch (iStatus)
			{
				case 1:
					Sleep(50);
					break;
				case 2:
					Sleep(100);
					break;
				case 3:
					Sleep(200);
					break;
				case 4:
					Sleep(300);
					break;
				default:
					break;
			}

			setLightStatus(3, 0);
		}
	}
}

int main()
{
	// Color stuff
	CONSOLE_SCREEN_BUFFER_INFO csbiScreen;
	WORD wOldColAttr;	// For input process.
	GetConsoleScreenBufferInfo((HANDLE)GetStdHandle(STD_OUTPUT_HANDLE), &csbiScreen);
	wOldColAttr = csbiScreen.wAttributes;
	SetConsoleTextAttribute((HANDLE)GetStdHandle(STD_OUTPUT_HANDLE), wOldColAttr | FOREGROUND_INTENSITY);
	
	SetConsoleTitleA("[GTA:SA] Lightmod 1.0");

	SetConsoleTextAttribute((HANDLE)GetStdHandle(STD_OUTPUT_HANDLE), 0x006 | FOREGROUND_INTENSITY);
	printf("Initialisiere GTA:SA, (c) 2012 by Martin Kammersberger(mit Unterstuetzung von Alexander Guettler)\n\n");
    EnableDebugPrivilege();

    DWORD dwID;
    HWND sa = FindWindow(NULL,TEXT("GTA:SA:MP"));
	SetConsoleTextAttribute((HANDLE)GetStdHandle(STD_OUTPUT_HANDLE), 0x008 | FOREGROUND_INTENSITY);

    GetWindowThreadProcessId(sa, &dwID);
    hProc = OpenProcess(PROCESS_ALL_ACCESS, false, dwID);

	if (hProc) 
	{
		SetConsoleTextAttribute((HANDLE)GetStdHandle(STD_OUTPUT_HANDLE), 0x003 | FOREGROUND_INTENSITY);
		printf("\nGTA Prozess konnte erfolgreich gefunden werden, starte Injektion\n");
		ReadProcessMemory(hProc,(LPCVOID)0xBA18FC,&dwVehicle,sizeof(dwVehicle), 0);

		// Get time test
		DWORD dwTime;
		ReadProcessMemory(hProc,(LPCVOID)0xB70158,&dwTime,sizeof(dwTime), 0);
		printf("Timetest(0xB70158): %d\n",dwTime/1000);

		SetConsoleTextAttribute((HANDLE)GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		printf("Injektion ausgefuehrt, starte Lightmod\n");

		for (;;)
		{
			ForceLightWhile();
			hwnd = GetForegroundWindow(); // get active window
			GetWindowText(hwnd, szWindowText, 100); // get title
			TCharToChar(szWindowText, AnsiBuffer, sizeof(AnsiBuffer));
			destPtr = (const char *)AnsiBuffer;
		}
	}
	else 
	{
		SetConsoleTextAttribute((HANDLE)GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("\nKonnte keinen SA:MP/GTA Prozess finden, Programm wird pausiert!\n");
	}

	SetConsoleTextAttribute((HANDLE)GetStdHandle(STD_OUTPUT_HANDLE), 0x008 | FOREGROUND_INTENSITY);
	printf("Bitte eine Taste druecken, um das Programm zu stoppen....");
	getchar();
}

//int __thiscall sub_6C2100(int this, char a2, int a3) // <-- sub_6C2100 turns on the lights, grabed via IDA ; GTA SA v 1.1
//{
//  int result; // eax@1

//  result = (a3 << 2 * a2) | *(_DWORD *)(this + 16) & ~(3 << 2 * a2);
//  *(_DWORD *)(this + 16) = result;
//  return result;
//}
