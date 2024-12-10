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

#include <toml++/toml.h>

#include "cfg.h"
#include "globals.h"

#define FF78_CFG_FILE "FF78Launcher.toml"

bool fullscreen;
long window_width;
long window_height;
long refresh_rate;
bool enable_linear_filtering;
bool keep_aspect_ratio;
bool original_mode;
bool pause_game_on_background;
long sfx_volume;
long music_volume;
bool launch_chocobo;

void read_cfg()
{
  toml::parse_result config;

	try
	{
		config = toml::parse_file(FF78_CFG_FILE);
	}
	catch (const toml::parse_error &err)
	{
		PLOGW << "Parse error while opening the file " FF78_CFG_FILE ". Will continue with the default settings.";
		PLOGW.printf("%s (Line %u Column %u)\n", err.what(), err.source().begin.line, err.source().begin.column);

		char tmp[1024]{0};
		sprintf(tmp, "%s (Line %u Column %u)\n\nWill continue with safe default settings.", err.what(), err.source().begin.line, err.source().begin.column);
		MessageBoxA(NULL, tmp, "Configuration issue detected!", MB_ICONWARNING | MB_OK);

		config = toml::parse("");
	}

  // Read config values
	fullscreen = config["fullscreen"].value_or(false);
  window_width = config["window_width"].value_or(0);
	window_height = config["window_height"].value_or(0);
  refresh_rate = config["refresh_rate"].value_or(0);
	enable_linear_filtering = config["enable_linear_filtering"].value_or(false);
	keep_aspect_ratio = config["keep_aspect_ratio"].value_or(false);
	original_mode = config["original_mode"].value_or(false);
	pause_game_on_background = config["pause_game_on_background"].value_or(false);
  sfx_volume = config["sfx_volume"].value_or(100);
  music_volume = config["music_volume"].value_or(100);
	launch_chocobo = config["launch_chocobo"].value_or(false);

  // Window width or height size can't be less then 0
	if (window_width < 0) window_width = 0;
	if (window_height < 0) window_height = 0;

  // Ensure range is respected for volume settings
  if (sfx_volume < 0) sfx_volume = 0;
  if (sfx_volume > 100) sfx_volume = 100;
  if (music_volume < 0) music_volume = 0;
  if (music_volume > 100) music_volume = 100;

	// fetch current user screen settings
	DEVMODE dmCurrentScreenSettings;
	EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dmCurrentScreenSettings);

  // Autodetect current resolution if no width or height have been provided
  if (window_width == 0 || window_height == 0)
	{
		if (fullscreen)
		{
			window_width = dmCurrentScreenSettings.dmPelsWidth;
			window_height = dmCurrentScreenSettings.dmPelsHeight;
			// Autodetect refresh rate
			if (refresh_rate == 0) refresh_rate = dmCurrentScreenSettings.dmDisplayFrequency;
		}
		else
		{
			window_width = 640;
			window_height = 480;
			// If not set, force it to 60
			if (refresh_rate == 0) refresh_rate = 60;
		}
	}

	// FF7 does not support this flag
	if (!ff8)
	{
		pause_game_on_background = false;
		launch_chocobo = false;
	}
}
