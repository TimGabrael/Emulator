#pragma once
#include "../Helper.h"

struct Bios
{
	uint8_t* data;
	int data_size;
};

struct Bios* PS1_BIOS_Alloc();
void PS1_BIOS_Free(struct Bios** bios);

uint32_t PS1_BIOS_Read32(struct Bios* bios, uint32_t offset);

uint8_t PS1_BIOS_Read8(struct Bios* bios, uint32_t offset);