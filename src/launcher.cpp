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
#include <vector>
#include <cstring>
#include <shlwapi.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include "globals.h"
#include "utils.h"

// ---------------------------------------------------------------------------
// In-memory bypass of the FF7/FF8 keyboard prompt (replaces editing AF3DN.P)
//
// AF3DN.P contains a one-time flag byte (initialised to 1) that is checked in
// its render code. FF7 example:
//
//   38 1D <flag>        cmp  byte [flag], bl
//   74 48               je   skip
//   39 3D <mode>        cmp  dword [mode], edi
//   75 40               jne  skip
//   6A 48               push 0x48
//   88 1D <flag>        mov  byte [flag], bl
//
// FF8 uses the same flag with slightly different surrounding code (see the
// signature table below). Setting the flag to 0 makes the driver skip that block, which is exactly
// what the one-byte file patch does. Here we find the code by signature (so it
// also works if the DLL is relocated), and write 0 to the flag in the running
// game's memory.
// ---------------------------------------------------------------------------
static bool patch_prompt_flag(HANDLE hProcess, DWORD pid)
{
	// Find the AF3DN.P module in the game process
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap == INVALID_HANDLE_VALUE) return false;

	MODULEENTRY32 me;
	ZeroMemory(&me, sizeof(me));
	me.dwSize = sizeof(me);

	BYTE *base = nullptr;
	DWORD size = 0;
	if (Module32First(snap, &me))
	{
		do
		{
			if (_stricmp(me.szModule, "AF3DN.P") == 0)
			{
				base = me.modBaseAddr;
				size = me.modBaseSize;
				break;
			}
		} while (Module32Next(snap, &me));
	}
	CloseHandle(snap);

	if (!base || size == 0) return false;

	// Copy the module image out of the game
	std::vector<BYTE> img(size);
	SIZE_T got = 0;
	if (!ReadProcessMemory(hProcess, base, img.data(), size, &got) || got < size) return false;

	// Code signatures (mask 0 = wildcard). "ref2" is the offset of the second
	// reference to the flag, which must equal the one at offset 2.
	struct Signature
	{
		const char *name;
		size_t len;
		size_t ref2;
		BYTE pat[32];
		BYTE mask[32];
	};

	static const Signature signatures[] = {
		// FF7: cmp [flag],bl / je / cmp [mode],edi / jne / push 48h / mov [flag],bl
		{ "FF7", 24, 20,
			{ 0x38,0x1D,0,0,0,0, 0x74,0x48, 0x39,0x3D,0,0,0,0, 0x75,0x40, 0x6A,0x48, 0x88,0x1D,0,0,0,0 },
			{ 1,1,0,0,0,0, 1,1, 1,1,0,0,0,0, 1,1, 1,1, 1,1,0,0,0,0 } },
		// FF8: cmp [flag],bl / jz / cmp [other],bl / jz / mov [flag],bl
		{ "FF8", 30, 26,
			{ 0x38,0x1D,0,0,0,0, 0x0F,0x84,0,0,0,0, 0x38,0x1D,0,0,0,0, 0x0F,0x84,0,0,0,0, 0x88,0x1D,0,0,0,0 },
			{ 1,1,0,0,0,0, 1,1,0,0,0,0, 1,1,0,0,0,0, 1,1,0,0,0,0, 1,1,0,0,0,0 } },
	};

	for (const Signature &sig : signatures)
	for (size_t i = 0; i + sig.len <= img.size(); i++)
	{
		bool ok = true;
		for (size_t j = 0; j < sig.len; j++)
		{
			if (sig.mask[j] && img[i + j] != sig.pat[j]) { ok = false; break; }
		}
		if (!ok) continue;

		// Both references must point to the same flag
		if (memcmp(&img[i + 2], &img[i + sig.ref2], 4) != 0) continue;

		uint32_t flagAddr;
		memcpy(&flagAddr, &img[i + 2], 4);

		// Not (yet) relocated, or not what we expect: try again later
		if (flagAddr < (uint32_t)(uintptr_t)base || flagAddr >= (uint32_t)(uintptr_t)base + size) return false;

		BYTE value = 0xFF;
		if (!ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)flagAddr, &value, 1, &got) || got != 1) return false;

		if (value == 0)
		{
			PLOGI.printf("Keyboard prompt flag already cleared");
			return true;
		}
		if (value != 1) return false;

		BYTE zero = 0;
		if (!WriteProcessMemory(hProcess, (LPVOID)(uintptr_t)flagAddr, &zero, 1, &got) || got != 1) return false;

		PLOGI.printf("Keyboard prompt bypassed (%s driver, flag at 0x%08X)", sig.name, flagAddr);
		return true;
	}

	return false;
}

static HANDLE g_bypassProcess = NULL;

static DWORD WINAPI prompt_bypass_thread(LPVOID lpParam)
{
	DWORD pid = GetProcessId(g_bypassProcess);
	ULONGLONG deadline = GetTickCount64() + 60000;

	// Keep trying until the driver is loaded, the game exits or we time out
	while (WaitForSingleObject(g_bypassProcess, 0) == WAIT_TIMEOUT && GetTickCount64() < deadline)
	{
		if (patch_prompt_flag(g_bypassProcess, pid)) return 0;
		Sleep(5);
	}

	PLOGW.printf("Could not bypass the keyboard prompt (AF3DN.P not found, or unsupported version)");
	return 1;
}

HANDLE start_prompt_bypass(HANDLE hProcess)
{
	g_bypassProcess = hProcess;
	return CreateThread(NULL, 0, prompt_bypass_thread, NULL, 0, NULL);
}

std::wstring getDocumentPath()
{
	std::wstring payload;

	if (!ff7_estore_edition && !std::filesystem::exists("data/music_2"))
	{
		PWSTR outPath = NULL;
		HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &outPath);

		if (SUCCEEDED(hr))
		{
			payload.append(outPath);

			if (ff8)
				payload.append(LR"(\Square Enix\FINAL FANTASY VIII Steam)");
			else
				payload.append(LR"(\Square Enix\FINAL FANTASY VII Steam)");
		}

		CoTaskMemFree(outPath);
	}
	else
	{
		WCHAR basedir[512];
		GetCurrentDirectoryW(512, basedir);
		payload.append(basedir);
	}

	return payload;
}

DWORD WINAPI process_game_messages( LPVOID lpParam )
{
	PLOGI.printf("Starting game message queue thread...");

	// Release semaphores when raised
	while(true)
	{
		WaitForSingleObject(launcherCanReadMsgSem, INFINITE);
		ReleaseSemaphore(launcherDidReadMsgSem, 1, nullptr);
	}

	return 1;
}

void send_locale_data_dir()
{
	std::wstring payload(L"lang-");
	payload.append(std::wstring(game_lang, game_lang + sizeof(game_lang)));

	if (ff8)
		*launcher_memory_part = FF8_LOCALE_DATA_DIR;
	else if(ff7_estore_edition)
		*launcher_memory_part = ESTORE_LOCALE_DATA_DIR;
	else
		*launcher_memory_part = FF7_LOCALE_DATA_DIR;

	*(launcher_memory_part + 1) = uint32_t(payload.size());
	memcpy((void *)(launcher_memory_part + 2), payload.data(), payload.size() * sizeof(wchar_t));

	PLOGI.printf("%s -> %ls", __func__, payload.c_str());

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void send_user_save_dir()
{
	std::wstring payload;

	// Get current document path
	payload.append(getDocumentPath());

	if (std::filesystem::exists("save"))
	{
		payload.append(LR"(\save)");
	}
	else
	{
		// Search for the first "user_" match in the game path
		WCHAR searchPath[260];
		WIN32_FIND_DATAW pathFound;
		HANDLE hFind;

		StrCpyW(searchPath, payload.c_str());
		StrCatW(searchPath, LR"(\user_*)");
		if (hFind = FindFirstFileW(searchPath, &pathFound))
		{
			payload.append(LR"(\)");
			payload.append(pathFound.cFileName);
			FindClose(hFind);
		}
	}

	if (ff8)
		*launcher_memory_part = FF8_USER_SAVE_DIR;
	else if(ff7_estore_edition)
		*launcher_memory_part = ESTORE_USER_SAVE_DIR;
	else
		*launcher_memory_part = FF7_USER_SAVE_DIR;
	*(launcher_memory_part + 1) = uint32_t(payload.size());
	memcpy((void *)(launcher_memory_part + 2), payload.data(), payload.size() * sizeof(wchar_t));

	PLOGI.printf("%s -> %ls", __func__, payload.c_str());

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void send_user_doc_dir()
{
	std::wstring payload;

	// Get current document path
	payload.append(getDocumentPath());

	if (ff8)
		*launcher_memory_part = FF8_DOC_DIR;
	else if(ff7_estore_edition)
		*launcher_memory_part = ESTORE_DOC_DIR;
	else
		*launcher_memory_part = FF7_DOC_DIR;
	*(launcher_memory_part + 1) = uint32_t(payload.size());
	memcpy((void *)(launcher_memory_part + 2), payload.data(), payload.size() * sizeof(wchar_t));

	PLOGI.printf("%s -> %ls", __func__, payload.c_str());

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void send_install_dir()
{
	WCHAR basedir[512];
	GetCurrentDirectoryW(512, basedir);
	std::wstring payload(basedir);

	if (ff8)
		*launcher_memory_part = FF8_INSTALL_DIR;
	else if(ff7_estore_edition)
		*launcher_memory_part = ESTORE_INSTALL_DIR;
	else
		*launcher_memory_part = FF7_INSTALL_DIR;
	*(launcher_memory_part + 1) = uint32_t(payload.size());
	memcpy((void *)(launcher_memory_part + 2), payload.data(), payload.size() * sizeof(wchar_t));

	PLOGI.printf("%s -> %ls", __func__, payload.c_str());

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void send_game_version()
{
	std::wstring payload(L"" APP_RELEASE_NAME " " APP_RELEASE_VERSION);

	if (ff8)
		*launcher_memory_part = FF8_GAME_VERSION;
	else if(ff7_estore_edition)
		*launcher_memory_part = ESTORE_GAME_VERSION;
	else
		*launcher_memory_part = FF7_GAME_VERSION;
	*(launcher_memory_part + 1) = uint32_t(payload.size());
	memcpy((void *)(launcher_memory_part + 2), payload.data(), payload.size() * sizeof(wchar_t));

	PLOGI.printf("%s -> %ls", __func__, payload.c_str());

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void send_disable_cloud()
{
	if (ff7_estore_edition) return;

	if (ff8)
		*launcher_memory_part = FF8_DISABLE_CLOUD;
	else
		*launcher_memory_part = FF7_DISABLE_CLOUD;

	PLOGI.printf("%s", __func__);

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void send_bg_pause_enabled()
{
	if (!ff8) return;

	*launcher_memory_part = FF8_BG_PAUSE_ENABLED;
	*(launcher_memory_part + 1) = uint32_t(1);

	PLOGI.printf("%s", __func__);

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void send_launcher_completed()
{
	if (ff8)
		*launcher_memory_part = FF8_END_USER_INFO;
	else if(ff7_estore_edition)
		*launcher_memory_part = ESTORE_END_USER_INFO;
	else
		*launcher_memory_part = FF7_END_USER_INFO;

	PLOGI.printf("%s", __func__);

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void write_ffvideo()
{
	std::wstring payload;

	payload.append(getDocumentPath());
	if (payload.size() > 0) payload.append(LR"(\)");

	if (ff8)
	{
		payload.append(L"ff8video.cfg");

		FILE* handle = _wfopen(payload.c_str(), L"wb");
		fwrite_ulong(window_width, handle);
		fwrite_ulong(window_height, handle);
		fwrite_ulong(refresh_rate, handle);
		fwrite_ulong(fullscreen, handle);
		fwrite_ulong(0, handle);
		fwrite_ulong(keep_aspect_ratio, handle);
		fwrite_ulong(enable_linear_filtering, handle);
		fwrite_ulong(original_mode, handle);
		fwrite_ulong(pause_game_on_background, handle);
		fclose(handle);
	}
	else
	{
		payload.append(L"ff7video.cfg");

		FILE* handle = _wfopen(payload.c_str(), L"wb");
		fwrite_byteswap_ulong(window_width, handle);
		fwrite_byteswap_ulong(window_height, handle);
		fwrite_byteswap_ulong(refresh_rate, handle);
		fwrite_byteswap_ulong(fullscreen, handle);
		fwrite_byteswap_ulong(0, handle);
		fwrite_byteswap_ulong(keep_aspect_ratio, handle);
		fwrite_byteswap_ulong(enable_linear_filtering, handle);
		fwrite_byteswap_ulong(original_mode, handle);
		fclose(handle);
	}
}

void write_ffinput()
{
	// The eStore edition uses a different input format that is not supported here
	if (ff7_estore_edition) return;

	const std::string &mode = ff8 ? ff8_input_mode : ff7_input_mode;
	if (mode != "missing" && mode != "always") return;

	std::wstring payload;

	payload.append(getDocumentPath());
	if (payload.size() > 0) payload.append(LR"(\)");
	payload.append(ff8 ? L"ff8input.cfg" : L"ff7input.cfg");

	if (mode == "missing" && std::filesystem::exists(payload))
	{
		PLOGI.printf("%s -> %ls already exists, keeping it", __func__, payload.c_str());
		return;
	}

	FILE* handle = _wfopen(payload.c_str(), L"wb");
	if (!handle)
	{
		PLOGW.printf("%s -> could not open %ls for writing", __func__, payload.c_str());
		return;
	}

	if (ff8)
	{
		std::string data = inputcfg::build_ff8(ff8_input);
		fwrite(data.data(), 1, data.size(), handle);
	}
	else
	{
		std::vector<uint8_t> data = inputcfg::build_ff7(ff7_input);
		fwrite(data.data(), 1, data.size(), handle);
	}
	fclose(handle);

	PLOGI.printf("%s -> %ls", __func__, payload.c_str());
}

void write_ffsound()
{
	std::wstring payload;

	payload.append(getDocumentPath());
	if (payload.size() > 0) payload.append(LR"(\)");
	if (ff8) payload.append(L"ff8sound.cfg");
	else payload.append(L"ff7sound.cfg");

	FILE* handle = _wfopen(payload.c_str(), L"wb");
	fwrite(&sfx_volume, sizeof(DWORD), 1, handle);
	fwrite(&music_volume, sizeof(DWORD), 1, handle);
	fclose(handle);
}