/****************************************************************************/
//    Copyright (C) 2024 Julian Xhokaxhiu                                   //
//                                                                          //
//    This file is part of FF78Launcher                                     //
//                                                                          //
//    FF78Launcher is free software: you can redistribute it and/or modify  //
//    it under the terms of the GNU General Public License as published by  //
//    the Free Software Foundation, either version 3 of the License         //
//                                                                          //
//    FF78Launcher is distributed in the hope that it will be useful,       //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of        //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
//    GNU General Public License for more details.                          //
/****************************************************************************/

#include <filesystem>
#include <windows.h>
#include "resource.h"

const char processes[][32]{
	// FF7
	"ff7_de.exe",
	"ff7_en.exe",
	"ff7_es.exe",
	"ff7_fr.exe",
	"ff7_ja.exe",
	// FF8
	"ff8_de.exe",
	"ff8_en.exe",
	"ff8_es.exe",
	"ff8_fr.exe",
	"ff8_it.exe",
	"ff8_ja.exe",
};
const int numProcesses = sizeof(processes)/sizeof(processes[0]);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Initialize the process start information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Find one of the supported processes and attempt to start
	for (int i = 0; i < numProcesses; i++)
	{
		if (std::filesystem::exists(processes[i]))
		{
			if (!CreateProcessA(processes[i], NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			{
				MessageBoxA(NULL, "Something went wrong while launching the game.", "Error", MB_ICONERROR | MB_OK);
				return 1;
			}
		}
	}

	// Wait for the process to finish
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}
