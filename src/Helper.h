#pragma once
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <assert.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#ifdef _WIN32
#include <Windows.h>
#include <intsafe.h>
#undef min
#undef max
#endif
#include <debugapi.h>
#define LOG(...) {char cad[1024]; sprintf(cad, __VA_ARGS__);  OutputDebugStringA(cad);}
#endif
#define ERR_MSG(...) {char cad[1024]; sprintf(cad, __VA_ARGS__); SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERR", cad, NULL); }


int i32_add_overflow(int32_t a1, int32_t a2, int32_t* res);