#include "bus.h"
#include "bios.h"
#include "cpu.h"

struct Range
{
	uint32_t start;
	uint32_t length;
};


static const struct Range BIOS_RANGE = { 0xBFC00000, 512 * 1024 };
static const struct Range MEMCONTROL_RANGE = { 0x1F801000, 36 };
static const struct Range RAM_SIZE_RANGE = { 0x1F801060, 4 };

static uint8_t R_Contains(const struct Range* range, uint32_t addr, uint32_t* out)
{
	if (addr >= range->start && addr < (range->start + range->length))
	{
		*out = addr - range->start;
		return 1;
	}
	return 0;
}

struct Bus* PS1_BUS_Alloc(struct Bios* bios)
{
	struct Bus* out = (struct Bus*)malloc(sizeof(struct Bus));
	if (!out) return 0;
	memset(out, 0, sizeof(struct Bus));
	out->bios = bios;
	return out;
}
void PS1_BUS_Free(struct Bus** bus)
{
	if (bus && *bus)
	{
		struct Bus* b = *bus;

		free(b);
		*bus = 0;
	}
}

uint32_t PS1_BUS_CpuRead(struct Bus* bus, uint32_t addr)
{
	if (addr % 4 != 0) {
		SDL_ShowSimpleMessageBox(0, "WARNING", "UNALIGNED READ", NULL);
	}
	uint32_t mapped = 0;
	if (R_Contains(&BIOS_RANGE, addr, &mapped)) return PS1_BIOS_Read(bus->bios, mapped);

	// NEVER SHOULD REACH THIS POINT!
	return -1;
}
void PS1_BUS_CpuWrite(struct Bus* bus, uint32_t addr, uint32_t val)
{
	if (addr % 4 != 0) {
		SDL_ShowSimpleMessageBox(0, "WARNING", "UNALIGNED WRITE", NULL);
	}
	uint32_t mapped = 0;
	if (R_Contains(&MEMCONTROL_RANGE, addr, &mapped)) {
		if (mapped == 0) {
			if(val != 0x1F000000) SDL_ShowSimpleMessageBox(0, "ERR", "BadExpansion_1_Base_address", NULL);
		}
		else if (mapped == 4) {
			if(val != 0x1F802000) SDL_ShowSimpleMessageBox(0, "ERR", "BadExpansion_2_Base_address", NULL);
		}
		else {
			SDL_ShowSimpleMessageBox(0, "ERR", "Unhandled_write_to_MEMCONTROL_register", NULL);
		}
		return;
	}
	else if (R_Contains(&RAM_SIZE_RANGE, addr, &mapped)) {
		return;
	}
	SDL_ShowSimpleMessageBox(0, "WRITING TO UNKOWN", "WRITING TO UNKOWN", NULL);
}