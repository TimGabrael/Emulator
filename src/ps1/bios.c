#include "bios.h"
#include "../../data/PS1_BIOS.h"

struct Bios* PS1_BIOS_Alloc()
{
	struct Bios* out = (struct Bios*)malloc(sizeof(struct Bios));
	if (!out) return 0;
	memset(out, 0, sizeof(struct Bios));
	
	out->data_size = sizeof(PS1_BIOS);
	out->data = malloc(out->data_size);
	if (!out->data) {
		free(out);
		return 0;
	}
	memcpy(out->data, PS1_BIOS, out->data_size);

	return out;
}
void PS1_BIOS_Free(struct Bios** bios)
{
	if (bios && *bios)
	{
		struct Bios* b = *bios;
		if (b->data) free(b->data);
		b->data = 0;
		b->data_size = 0;
		free(b);
		*bios = 0;
	}
}

uint32_t PS1_BIOS_Read(struct Bios* bios, uint32_t offset)
{
	const uint32_t b0 = (uint32_t)bios->data[offset + 0];
	const uint32_t b1 = (uint32_t)bios->data[offset + 1];
	const uint32_t b2 = (uint32_t)bios->data[offset + 2];
	const uint32_t b3 = (uint32_t)bios->data[offset + 3];
	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}