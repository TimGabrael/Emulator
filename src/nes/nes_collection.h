#pragma once
#include "../Helper.h"

struct NES
{
	struct DataBus* bus;
	struct CPU* cpu;
	struct PPU* ppu;
	struct Cartridge* cart;

	struct SDL_Texture* texture;
	float timer;
};

struct NES* NES_Alloc(struct AppData* app);
void NES_Free(struct NES** nes);

uint8_t NES_Tick(struct AppData* app, struct NES* nes, float dt);

uint8_t NES_LoadFile(struct NES* nes, const char* filename);
uint8_t NES_Load(struct NES* nes, uint8_t* data, int sz);