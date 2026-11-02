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

#pragma once

// Builders for ff7input.cfg / ff8input.cfg (header only, standard C++ only).
//
// Keyboard values are DirectInput scan codes (e.g. 0x2D = X, 0xC8 = Up).
// Joystick values are button numbers 1-10, as shown in the official launcher.
// With the default values, both builders reproduce the files written by the
// official launcher byte for byte.

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace inputcfg
{
	// ------------------------------------------------------------------ FF7
	enum Ff7Action
	{
		FF7_CAMERA, FF7_TARGET, FF7_PAGE_UP, FF7_PAGE_DOWN, FF7_MENU,
		FF7_OK, FF7_CANCEL, FF7_SWITCH, FF7_ASSIST, FF7_START,
		FF7_ACTION_COUNT
	};

	struct Ff7
	{
		long kb[FF7_ACTION_COUNT] = { 0xC7, 0xCF, 0xC9, 0xD1, 0x2F, 0x2D, 0x2E, 0xD2, 0xD3, 0x30 };
		long kb_up = 0xC8, kb_down = 0xD0, kb_left = 0xCB, kb_right = 0xCD;
		long js[FF7_ACTION_COUNT] = { 7, 8, 5, 6, 4, 3, 2, 1, 9, 10 };
	};

	// Joystick button N (1-10) is stored as 0xEA + N in ff7input.cfg
	inline uint32_t ff7_js_code(long button) { return uint32_t(0xEA + button); }

	// Layout: 1 byte (0x01) followed by 75 little-endian 32-bit values.
	// Slots that are not configurable keep the values the official launcher writes.
	inline std::vector<uint8_t> build_ff7(const Ff7 &c)
	{
		uint32_t d[75] = { 0 };

		// Keyboard, 12 slots (slots 9 and 10 are unused)
		for (int i = 0; i < 9; i++) d[i] = uint32_t(c.kb[i]);
		d[11] = uint32_t(c.kb[FF7_START]);

		// Keyboard directions + fixed keys (Esc, Enter)
		d[12] = uint32_t(c.kb_up);
		d[13] = uint32_t(c.kb_right);
		d[14] = uint32_t(c.kb_down);
		d[15] = uint32_t(c.kb_left);
		d[16] = 0x01;
		d[17] = uint32_t(c.kb_up);
		d[18] = uint32_t(c.kb_down);
		d[19] = uint32_t(c.kb_left);
		d[20] = uint32_t(c.kb_right);
		d[21] = 0x1C;

		// Joystick: fixed leading slots, then the same 12 slots as the keyboard
		d[22] = 0xDF;
		d[23] = 0xEB;
		d[24] = 0xED;
		for (int i = 0; i < 9; i++) d[25 + i] = ff7_js_code(c.js[i]);
		d[36] = ff7_js_code(c.js[FF7_START]);

		// Joystick directions (not rebindable) + fixed slots
		d[37] = 0xE3; d[38] = 0xE6; d[39] = 0xE4; d[40] = 0xE5;
		d[41] = 0xEC;
		d[42] = 0xE3; d[43] = 0xE4; d[44] = 0xE5; d[45] = 0xE6;
		d[47] = 0xE0;

		std::vector<uint8_t> out;
		out.push_back(0x01);
		for (uint32_t v : d)
			for (int b = 0; b < 4; b++) out.push_back(uint8_t(v >> (8 * b)));
		return out;
	}

	// ------------------------------------------------------------------ FF8
	// Names are the ones used inside ff8input.cfg, in file order.
	static const char *const FF8_NAMES[14] = {
		"Select", "Exit", "Misc", "Menu", "Toggle", "Trigger", "RotLt", "RotRt",
		"Start", "Select", "Up", "Down", "Left", "Right"
	};

	struct Ff8
	{
		// 10 configurable entries followed by Up, Down, Left, Right
		long kb[14] = { 47, 45, 31, 46, 35, 34, 32, 33, 30, 36, 200, 208, 203, 205 };
		// Joystick button numbers 1-10 for the first 10 entries
		long js[10] = { 3, 2, 1, 4, 5, 6, 7, 8, 9, 10 };
	};

	inline std::string ff8_line(int n, const char *name, long value)
	{
		std::string s = std::to_string(n) + ". \"" + name + "\"";
		// The value always starts at column 14
		do { s.push_back(' '); } while (s.size() < 14);
		s += std::to_string(value) + "\n";
		return s;
	}

	inline std::string build_ff8(const Ff8 &c)
	{
		std::string s = "Keyboard\n";
		for (int i = 0; i < 14; i++) s += ff8_line(i + 1, FF8_NAMES[i], c.kb[i]);

		s += "Joystick\n";
		static const long dirs[4] = { 252, 253, 254, 255 };
		for (int i = 0; i < 14; i++)
			s += ff8_line(i + 1, FF8_NAMES[i], i < 10 ? 223 + c.js[i] : dirs[i - 10]);

		return s;
	}
}
