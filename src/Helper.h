#pragma once
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#ifdef _WIN32
#include <Windows.h>
#undef min
#undef max
#endif
#include <debugapi.h>
#define LOG(...) {char cad[1024]; sprintf(cad, __VA_ARGS__);  OutputDebugStringA(cad);}
#endif
