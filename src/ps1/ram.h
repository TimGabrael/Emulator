#pragma once
#include "../Helper.h"

#define RAM_SIZE 2 * 1024 * 1024

struct Ram
{
	uint8_t data[RAM_SIZE];
};

struct Ram* PS1_RAM_Alloc();
void PS1_RAM_Free(struct Ram** ram);

uint32_t PS1_RAM_Read32(struct Ram* ram, uint32_t offset);
void PS1_RAM_Write32(struct Ram* ram, uint32_t offset, uint32_t val);

uint16_t PS1_RAM_Read16(struct Ram* ram, uint32_t offset);
void PS1_RAM_Write16(struct Ram* ram, uint32_t offset, uint16_t val);

uint8_t PS1_RAM_Read8(struct Ram* ram, uint32_t offset);
void PS1_RAM_Write8(struct Ram* ram, uint32_t offset, uint8_t val);