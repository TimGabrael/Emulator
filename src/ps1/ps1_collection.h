#pragma once
#include "../Helper.h"
#include "../AppData.h"

struct PS1
{
	struct Bios* bios;
	struct Bus* bus;
	struct CPU* cpu;
};


struct PS1* PS1_Alloc(struct AppData* app);
void PS1_Free(struct PS1** ps1);

uint8_t PS1_Tick(struct AppData* app, struct PS1* ps1, float dt);