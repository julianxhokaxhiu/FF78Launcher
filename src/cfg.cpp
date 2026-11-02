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

#include <algorithm>
#include <cctype>
#include <toml++/toml.h>

#include "cfg.h"
#include "globals.h"

#define FF78_CFG_FILE "FF78Launcher.toml"

std::string language;
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
bool skip_keyboard_prompt;
std::string ff7_input_mode;
std::string ff8_input_mode;
inputcfg::Ff7 ff7_input;
inputcfg::Ff8 ff8_input;

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
	language = std::string(config["language"].value_or("auto"));
	std::transform(language.begin(), language.end(), language.begin(), [](unsigned char c) { return (char)std::tolower(c); });
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
	skip_keyboard_prompt = config["skip_keyboard_prompt"].value_or(true);

	// Controller settings. Values outside the valid range fall back to the defaults.
	auto get_value = [&](const char *game, const char *group, const char *key, long def, long lo, long hi) -> long
	{
		long v = config[game][group][key].value_or(def);
		return (v < lo || v > hi) ? def : v;
	};

	auto get_mode = [&](const char *game) -> std::string
	{
		// write_config = "off" | "missing" | "always" (true is the same as "always")
		if (config[game]["write_config"].is_boolean())
			return config[game]["write_config"].value_or(false) ? "always" : "off";

		std::string mode = std::string(config[game]["write_config"].value_or("off"));
		std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		if (mode != "missing" && mode != "always") mode = "off";
		return mode;
	};

	ff7_input_mode = get_mode("ff7_controller");
	ff8_input_mode = get_mode("ff8_controller");

	{
		static const char *const kb_keys[inputcfg::FF7_ACTION_COUNT] = { "camera", "target", "page_up", "page_down", "menu", "ok", "cancel", "switch", "assist", "start" };
		const inputcfg::Ff7 def{};
		for (int i = 0; i < inputcfg::FF7_ACTION_COUNT; i++)
		{
			ff7_input.kb[i] = get_value("ff7_controller", "keyboard", kb_keys[i], def.kb[i], 0, 255);
			ff7_input.js[i] = get_value("ff7_controller", "joystick", kb_keys[i], def.js[i], 1, 10);
		}
		ff7_input.kb_up = get_value("ff7_controller", "keyboard", "up", def.kb_up, 0, 255);
		ff7_input.kb_down = get_value("ff7_controller", "keyboard", "down", def.kb_down, 0, 255);
		ff7_input.kb_left = get_value("ff7_controller", "keyboard", "left", def.kb_left, 0, 255);
		ff7_input.kb_right = get_value("ff7_controller", "keyboard", "right", def.kb_right, 0, 255);
	}

	{
		// Names follow the labels of the official launcher, in the order the entries are stored in ff8input.cfg.
		// The last two entries are labelled differently on the keyboard and joystick tabs of the official launcher.
		static const char *const kb_keys[14] = { "menu", "select", "card_game", "cancel", "rotation_left", "rotation_right", "escape_battle", "switch_pov", "pause", "toggle_display", "up", "down", "left", "right" };
		static const char *const js_keys[10] = { "menu", "select", "card_game", "cancel", "rotation_left", "rotation_right", "escape_battle", "switch_pov", "toggle_display", "pause" };
		const inputcfg::Ff8 def{};
		for (int i = 0; i < 14; i++)
			ff8_input.kb[i] = get_value("ff8_controller", "keyboard", kb_keys[i], def.kb[i], 0, 255);
		for (int i = 0; i < 10; i++)
			ff8_input.js[i] = get_value("ff8_controller", "joystick", js_keys[i], def.js[i], 1, 10);
	}

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
