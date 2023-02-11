#include "ram.h"


struct Ram* PS1_RAM_Alloc()
{
	struct Ram* out = (struct Ram*)malloc(sizeof(struct Ram));
	if (!out) return 0;
	memset(out, 0xCA, sizeof(struct Ram));
	return out;
}
void PS1_RAM_Free(struct Ram** ram)
{
	if (ram && *ram)
	{
		struct Ram* r = *ram;
		free(r);
		*ram = 0;
	}
}

uint32_t PS1_RAM_Read32(struct Ram* ram, uint32_t offset)
{
	const uint32_t b0 = (uint32_t)ram->data[offset + 0];
	const uint32_t b1 = (uint32_t)ram->data[offset + 1];
	const uint32_t b2 = (uint32_t)ram->data[offset + 2];
	const uint32_t b3 = (uint32_t)ram->data[offset + 3];

	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}
void PS1_RAM_Write32(struct Ram* ram, uint32_t offset, uint32_t val)
{
	const uint8_t b0 = (uint8_t)(val & 0xFF);
	const uint8_t b1 = (uint8_t)((val >> 8) & 0xFF);
	const uint8_t b2 = (uint8_t)((val >> 16) & 0xFF);
	const uint8_t b3 = (uint8_t)((val >> 24) & 0xFF);
	ram->data[offset + 0] = b0;
	ram->data[offset + 1] = b1;
	ram->data[offset + 2] = b2;
	ram->data[offset + 3] = b3;
}

uint16_t PS1_RAM_Read16(struct Ram* ram, uint32_t offset)
{
	const uint16_t b0 = (uint16_t)ram->data[offset + 0];
	const uint16_t b1 = (uint16_t)ram->data[offset + 1];
	return b0 | (b1 << 8);
}
void PS1_RAM_Write16(struct Ram* ram, uint32_t offset, uint16_t val)
{
	const uint8_t b0 = (uint8_t)(val & 0xFF);
	const uint8_t b1 = (uint8_t)((val >> 8) & 0xFF);
	ram->data[offset + 0] = b0;
	ram->data[offset + 1] = b1;
}

uint8_t PS1_RAM_Read8(struct Ram* ram, uint32_t offset)
{
	return ram->data[offset];
}
void PS1_RAM_Write8(struct Ram* ram, uint32_t offset, uint8_t val)
{
	ram->data[offset] = val;
}
