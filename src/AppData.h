#pragma once
#include <SDL2/SDL.h>

struct AppData
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_AudioDeviceID audio_device;
	SDL_GameController* controller;
	uint8_t keys[322];
};


struct AppData* AD_Alloc(int w, int h);
void AD_Free(struct AppData** app);