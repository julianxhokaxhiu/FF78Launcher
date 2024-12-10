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

#include <string.h>

inline bool startsWith(const char *pre, const char *str)
{
  return strncmp(pre, str, strlen(pre)) == 0;
}

auto operator""_MB( unsigned long long const x ) -> long { return 1024L*1024L*x; }


// trim from start (in place)
static inline void ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
  rtrim(s);
  ltrim(s);
}

inline size_t fwrite_byteswap_ulong(unsigned long value, FILE* handle)
{
  auto swapped = _byteswap_ulong(value);
  return fwrite(&swapped, sizeof(DWORD), 1, handle);
}

inline size_t fwrite_ulong(unsigned long value, FILE* handle)
{
  return fwrite(&value, sizeof(DWORD), 1, handle);
}
