#include "bus.h"
#include "bios.h"
#include "cpu.h"
#include "ram.h"

struct Range
{
	uint32_t start;
	uint32_t length;
};


static const struct Range BIOS_RANGE = { 0x1FC00000, 512 * 1024 };
static const struct Range MEMCONTROL_RANGE = { 0x1F801000, 36 };
static const struct Range RAM_SIZE_RANGE = { 0x1F801060, 4 };
static const struct Range CACHECONTROL_RANGE = { 0xFFFE0130, 4 };
static const struct Range RAM_RANGE = { 0x00000000, RAM_SIZE };
static const struct Range SPU_RANGE = { 0x1F801C00, 640 };
static const struct Range EXPANSION_2_RANGE = { 0x1F802000, 66 };
static const struct Range EXPANSION_1_RANGE = { 0x1F000000, 512 * 1024 };
static const struct Range IRQ_CONTROL_RANGE = { 0x1F801070, 8 };
static const struct Range TIMERS_RANGE = { 0x1F801100, 0x30 };
static const struct Range DMA_RANGE = { 0x1F801080, 0x80 };
static const struct Range GPU_RANGE = { 0x1f801810, 8 };


const uint32_t REGION_MASK[8] = {
	// KUSEG: 2048MB
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	// KSEG0: 512MB
	0x7fffffff,
	// KSEG1: 512MB
	0x1fffffff,
	// KSEG2: 1024MB
	0xffffffff, 0xffffffff,
};
static uint32_t R_Mask(uint32_t addr)
{
	const uint32_t idx = (addr >> 29);
	return addr & REGION_MASK[idx];
}

static uint8_t R_Contains(const struct Range* range, uint32_t addr, uint32_t* out)
{
	if (addr >= range->start && addr < (range->start + range->length))
	{
		*out = (addr - range->start);
		return 1;
	}
	return 0;
}

struct Bus* PS1_BUS_Alloc(struct Bios* bios)
{
	struct Bus* out = (struct Bus*)malloc(sizeof(struct Bus));
	if (!out) return 0;
	memset(out, 0, sizeof(struct Bus));
	out->ram = PS1_RAM_Alloc();
	out->bios = bios;
	return out;
}
void PS1_BUS_Free(struct Bus** bus)
{
	if (bus && *bus)
	{
		struct Bus* b = *bus;
		if (b->ram) PS1_RAM_Free(&b->ram);
		free(b);
		*bus = 0;
	}
}

uint32_t PS1_BUS_CpuRead32(struct Bus* bus, uint32_t addr)
{
	if (addr % 4 != 0) {
		ERR_MSG("unaligned read");
	}
	addr = R_Mask(addr);
	uint32_t mapped = 0;
	if (R_Contains(&BIOS_RANGE, addr, &mapped)) return PS1_BIOS_Read32(bus->bios, mapped);
	else if (R_Contains(&RAM_RANGE, addr, &mapped)) return PS1_RAM_Read32(bus->ram, mapped);
	else if (R_Contains(&IRQ_CONTROL_RANGE, addr, &mapped)) {
		// TODO: read from IRQ_CONTROL
		LOG("read from IRQ_CONTROL\n");
		return 0;
	}
	else if (R_Contains(&DMA_RANGE, addr, &mapped)) {
		LOG("DMA_read: %x\n", addr);
		return 0;
	}
	else if (R_Contains(&GPU_RANGE, addr, &mapped)) {
		LOG("GPU_read: %x\n", addr);
		if (mapped == 4) return 0x10000000;
		else return 0;
	}
	// SHOULD NEVER REACH THIS POINT!
	ERR_MSG("read32 from unkown %x\n", addr);
	return -1;
}
void PS1_BUS_CpuWrite32(struct Bus* bus, uint32_t addr, uint32_t val)
{
	if (addr % 4 != 0) {
		ERR_MSG("unaligned write");
	}
	addr = R_Mask(addr);
	uint32_t mapped = 0;
	if (R_Contains(&RAM_RANGE, addr, &mapped)) PS1_RAM_Write32(bus->ram, mapped, val);
	else if (R_Contains(&MEMCONTROL_RANGE, addr, &mapped)) {
		if (mapped == 0) {
			if (val != 0x1F000000) ERR_MSG("BadExpansion_1_Base_address");
		}
		else if (mapped == 4) {
			if (val != 0x1F802000) ERR_MSG("BadExpansion_2_Base_address");
		}
		else {
			LOG("Unhandled_write_to_MEMCONTROL_register\n");
		}
	}
	else if (R_Contains(&RAM_SIZE_RANGE, addr, &mapped)) {
		LOG("write_to_ram_size: %x\n", mapped);
		// TODO: write to ram size
	}
	else if (R_Contains(&CACHECONTROL_RANGE, addr, &mapped)) {
		LOG("writing_to_cachecontrol: %x\n", mapped);
		// TODO: write to cache control
	}
	else if (R_Contains(&IRQ_CONTROL_RANGE, addr, &mapped)) {
		LOG("IRQ_control: %x <- %x\n", mapped, val);
	}
	else if (R_Contains(&DMA_RANGE, addr, &mapped)) {
		LOG("DMA_write: %x %x\n", addr, val);
	}
	else if (R_Contains(&GPU_RANGE, addr, &mapped)) {
		LOG("GPU_write: %x %x\n", addr, val);
	}
	else if (R_Contains(&TIMERS_RANGE, addr, &mapped)) {
		LOG("Timers_write: %x %x\n", addr, val);
	}
	else {
		ERR_MSG("write32 to unkown %x\n", addr);
	}
}

uint16_t PS1_BUS_CpuRead16(struct Bus* bus, uint32_t addr)
{
	addr = R_Mask(addr);
	uint32_t mapped = 0;
	if (R_Contains(&RAM_RANGE, addr, &mapped)) return PS1_RAM_Read16(bus->ram, mapped);
	else if (R_Contains(&SPU_RANGE, addr, &mapped)) {
		// TODO: read from spu
		return 0;
	}
	else if (R_Contains(&IRQ_CONTROL_RANGE, addr, &mapped)) {
		LOG("IRG_control_read: %x\n", addr);
		return 0;
	}
	ERR_MSG("unhandled read16 from: %x\n", addr);
}
void PS1_BUS_CpuWrite16(struct Bus* bus, uint32_t addr, uint16_t val)
{
	if (addr % 2 != 0)	{
		ERR_MSG("Unaliged write16 address %x\n", addr);
	}
	addr = R_Mask(addr);
	uint32_t mapped = 0;
	if (R_Contains(&RAM_RANGE, addr, &mapped)) PS1_RAM_Write16(bus->ram, mapped, val);
	else if (R_Contains(&SPU_RANGE, addr, &mapped)) {
		// TODO: write to sound processing unit
	}
	else if (R_Contains(&TIMERS_RANGE, addr, &mapped)) {
		// TODO: write to timers
	}
	else if (R_Contains(&IRQ_CONTROL_RANGE, addr, &mapped)) {
		LOG("IRQ_control_write: %x %x\n", addr, val);
	}
	else {
		ERR_MSG("unhandled write16 address %x\n", addr);
	}
}

uint8_t PS1_BUS_CpuRead8(struct Bus* bus, uint32_t addr)
{
	addr = R_Mask(addr);
	uint32_t mapped = 0;
	if (R_Contains(&RAM_RANGE, addr, &mapped)) return PS1_RAM_Read8(bus->ram, mapped);
	else if (R_Contains(&BIOS_RANGE, addr, &mapped))  return PS1_BIOS_Read8(bus->bios, mapped);
	else if (R_Contains(&EXPANSION_1_RANGE, addr, &mapped)) {
		// No expansion implemented
		return 0xFF;
	}
	// SHOULD NEVER REACH POINT!
	ERR_MSG("read8 from unkown %x\n", addr);
	return -1;
}
void PS1_BUS_CpuWrite8(struct Bus* bus, uint32_t addr, uint8_t val)
{
	addr = R_Mask(addr);
	uint32_t mapped = 0;
	if (R_Contains(&RAM_RANGE, addr, &mapped)) PS1_RAM_Write8(bus->ram, mapped, val);
	else if (R_Contains(&EXPANSION_2_RANGE, addr, &mapped)) {
		LOG("unhandeld write to expansion 2 register\n");
	}
	else {
		ERR_MSG("unhandled write8 %x\n", addr);
	}
}
