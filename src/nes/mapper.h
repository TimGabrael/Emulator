#pragma once
#include "../Helper.h"


typedef uint8_t(__stdcall* PMAPPERFUNC1)(struct Mapper* map, uint16_t addr, uint32_t* mapped_addr);
typedef uint8_t(__stdcall* PMAPPERFUNC2)(struct Mapper* map, uint16_t addr, uint32_t* mapped_addr, uint8_t data);

struct Mapper
{

	PMAPPERFUNC1 cpu_map_read;
	PMAPPERFUNC2 cpu_map_write;
	PMAPPERFUNC1 ppu_map_read;
	PMAPPERFUNC1 ppu_map_write;

	uint8_t num_prog_banks;
	uint8_t num_chr_banks;
};

struct Mapper* NES_MAP_Alloc000(uint8_t prg_banks, uint8_t chr_banks);
void NES_MAP_Free(struct Mapper** mapper);