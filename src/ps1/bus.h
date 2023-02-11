#pragma once
#include "../Helper.h"

struct Bus
{
	struct Bios* bios;
	struct Ram* ram;
};




struct Bus* PS1_BUS_Alloc(struct Bios* bios);
void PS1_BUS_Free(struct Bus** bus);

uint32_t PS1_BUS_CpuRead32(struct Bus* bus, uint32_t addr);
void PS1_BUS_CpuWrite32(struct Bus* bus, uint32_t addr, uint32_t val);

void PS1_BUS_CpuWrite16(struct Bus* bus, uint32_t addr, uint16_t val);

uint8_t PS1_BUS_CpuRead8(struct Bus* bus, uint32_t addr);
void PS1_BUS_CpuWrite8(struct Bus* bus, uint32_t addr, uint8_t val);
