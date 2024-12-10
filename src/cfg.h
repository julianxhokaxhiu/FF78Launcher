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

extern bool fullscreen;
extern long window_width;
extern long window_height;
extern long refresh_rate;
extern bool enable_linear_filtering;
extern bool keep_aspect_ratio;
extern bool original_mode;
extern bool pause_game_on_background;
extern long sfx_volume;
extern long music_volume;
extern bool launch_chocobo;

void read_cfg();
