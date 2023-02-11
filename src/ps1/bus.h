#pragma once
#include "../Helper.h"

struct Bus
{
	struct Bios* bios;
};




struct Bus* PS1_BUS_Alloc(struct Bios* bios);
void PS1_BUS_Free(struct Bus** bus);

uint32_t PS1_BUS_CpuRead(struct Bus* bus, uint32_t addr);
void PS1_BUS_CpuWrite(struct Bus* bus, uint32_t addr, uint32_t val);