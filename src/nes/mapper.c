#include "mapper.h"
#include <memory.h>



static uint8_t __stdcall Mapper000CpuRead(struct Mapper* map, uint16_t addr, uint32_t* mapped_addr)
{
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		*mapped_addr = addr & (map->num_prog_banks > 1 ? 0x7FFF : 0x3FFF);
		return 1;
	}

	return 0;
}
static uint8_t __stdcall Mapper000CpuWrite(struct Mapper* map, uint16_t addr, uint32_t* mapped_addr, uint8_t data)
{
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		*mapped_addr = addr & (map->num_prog_banks > 1 ? 0x7FFF : 0x3FFF);
		return 1;
	}
	return 0;
}
static uint8_t __stdcall Mapper000PpuRead(struct Mapper* map, uint16_t addr, uint32_t* mapped_addr)
{
	if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		*mapped_addr = addr;
		return 1;
	}
	return 0;
}
static uint8_t __stdcall Mapper000PpuWrite(struct Mapper* map, uint16_t addr, uint32_t* mapped_addr)
{
	if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		if (map->num_chr_banks == 0)
		{
			*mapped_addr = addr;
			return 1;
		}
	}

	return 0;
}

struct Mapper* NES_MAP_Alloc000(uint8_t prg_banks, uint8_t chr_banks)
{
	struct Mapper* result = (struct Mapper*)malloc(sizeof(struct Mapper));
	if (!result) return 0;

	result->num_prog_banks = prg_banks;
	result->num_chr_banks = chr_banks;
	result->cpu_map_read = Mapper000CpuRead;
	result->cpu_map_write = Mapper000CpuWrite;
	result->ppu_map_read = Mapper000PpuRead;
	result->ppu_map_write = Mapper000PpuWrite;
	return result;
}
void NES_MAP_Free(struct Mapper** mapper)
{
	if (mapper && *mapper)
	{
		memset(*mapper, 0, sizeof(struct Mapper));
		free(*mapper);
		*mapper = 0;
	}
}