#pragma once

#define APP_NAME_LITERAL "Uptooda"

inline constexpr auto APP_NAME_A = APP_NAME_LITERAL;

#ifdef _WIN32
#include <tchar.h>

inline constexpr auto APP_NAME = _T(APP_NAME_LITERAL);
#endif 

#undef APP_NAME_LITERAL

constexpr auto MAX_RETRIES_PER_FILE = 3;
constexpr auto MAX_RETRIES_PER_ACTION = 2;
constexpr auto ENV_A_E_K = "R1htayd0aW1VSlYkLyxqOw==";
