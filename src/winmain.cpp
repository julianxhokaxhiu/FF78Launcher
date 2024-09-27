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
#include "globals.h"
#include "launcher.h"
#include "utils.h"

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
	char process_to_start[32]{0};

	// Find which process we need to start
	for (int i = 0; i < numProcesses; i++)
	{
		if (std::filesystem::exists(processes[i]))
			strcpy_s(process_to_start, 32, processes[i]);
	}

	// Is it FF8?
	if (startsWith("ff8", process_to_start)) ff8 = true;

	// Is it eStore?
	if (startsWith("ff7_ja", process_to_start) && std::filesystem::file_size("AF3DN.P") < 1_MB) ff7_estore_edition = true;

	// Get game language
	strncpy(game_lang, strchr(process_to_start, '_') + 1, 2);
	game_lang[2] = '\0';

	// Initialise the semaphores required to talk with the official driver
	gameCanReadMsgSem = CreateSemaphoreA(NULL, 0, 1, ff8 ? "ff8_gameCanReadMsgSem" : "ff7_gameCanReadMsgSem");
	gameDidReadMsgSem = CreateSemaphoreA(NULL, 0, 1, ff8 ? "ff8_gameDidReadMsgSem" : "ff7_gameDidReadMsgSem");
	launcherCanReadMsgSem = CreateSemaphoreA(NULL, 0, 1, ff8 ? "ff8_launcherCanReadMsgSem" : "ff7_launcherCanReadMsgSem");
	launcherDidReadMsgSem = CreateSemaphoreA(NULL, 0, 1, ff8 ? "ff8_launcherDidReadMsgSem" : "ff7_launcherDidReadMsgSem");
	sharedMemoryWithLauncher = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 0x20000, ff8 ? "ff8_sharedMemoryWithLauncher" : "ff7_sharedMemoryWithLauncher");
	viewOfSharedMemory = MapViewOfFile(sharedMemoryWithLauncher, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	launcher_memory_part = (uint32_t *)((uint8_t *)viewOfSharedMemory + 0x10000);
	processGameMessagesThread = CreateThread(NULL, 0, process_game_messages, NULL, NULL, NULL);

	// Initialize the process start information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the process
	if (!CreateProcessA(process_to_start, NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
	{
		MessageBoxA(NULL, "Something went wrong while launching the game.", "Error", MB_ICONERROR | MB_OK);
		return 1;
	}

	send_locale_data_dir();
	send_user_save_dir();
	send_user_doc_dir();
	send_install_dir();
	send_game_version();
	send_disable_cloud();
	send_launcher_completed();

	// Wait for the process to finish
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(processGameMessagesThread);

	// Close Semaphores and file mappings
	UnmapViewOfFile(viewOfSharedMemory);
	CloseHandle(sharedMemoryWithLauncher);
	CloseHandle(gameDidReadMsgSem);
	CloseHandle(gameCanReadMsgSem);
	CloseHandle(launcherDidReadMsgSem);
	CloseHandle(launcherCanReadMsgSem);

	return 0;
}
