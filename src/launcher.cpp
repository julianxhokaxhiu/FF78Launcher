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
#include <shlwapi.h>
#include <shlobj.h>
#include "globals.h"
#include "utils.h"

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
			CoTaskMemFree(outPath);

			if (ff8)
				payload.append(LR"(\Square Enix\FINAL FANTASY VIII Steam)");
			else
				payload.append(LR"(\Square Enix\FINAL FANTASY VII Steam)");
		}
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

	// Wait for the game
	ReleaseSemaphore(gameCanReadMsgSem, 1, nullptr);
	WaitForSingleObject(gameDidReadMsgSem, INFINITE);
}

void write_ffvideo()
{
	std::wstring payload;

	payload.append(getDocumentPath());
	if (payload.size() > 0) payload.append(LR"(\)");
	if (ff8) payload.append(L"ff8video.cfg");
	else payload.append(L"ff7video.cfg");

	FILE* handle = _wfopen(payload.c_str(), L"wb");
	fwrite_byteswap_ulong(window_width, handle);
	fwrite_byteswap_ulong(window_height, handle);
	fwrite_byteswap_ulong(refresh_rate, handle);
	fwrite_byteswap_ulong(fullscreen, handle);
	fwrite_byteswap_ulong(enable_linear_filtering, handle);
	fwrite_byteswap_ulong(keep_aspect_ratio, handle);
	fwrite_byteswap_ulong(pause_game_on_background, handle);
	fwrite_byteswap_ulong(original_mode, handle);
	fclose(handle);
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