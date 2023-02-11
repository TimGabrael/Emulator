#include "cartridge.h"
#include "mapper.h"

struct Cartridge* NES_Cart_AllocFromFile(const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if (!fp) return 0;

	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	uint8_t* data = malloc(sz);
	if (!data) {
		fclose(fp);
		return 0;
	}
	fread(data, sz, 1, fp);
	fclose(fp);

	struct Cartridge* cart = NES_Cart_Alloc(data, sz);
	free(data);
	return cart;
}
struct Cartridge* NES_Cart_Alloc(uint8_t* data, int sz)
{
	struct Cartridge* cart = (struct Cartridge*)malloc(sizeof(struct Cartridge));
	if (!cart) { return 0; }
	memset(cart, 0, sizeof(struct Cartridge));

	struct sHeader
	{
		char name[4];
		uint8_t prg_rom_chunks;
		uint8_t chr_rom_chunks;
		uint8_t mapper1;
		uint8_t mapper2;
		uint8_t prg_ram_size;
		uint8_t tv_system1;
		uint8_t tv_system2;
		char unused[5];
	};
	struct sHeader* header = (struct sHeader*)data;
	data += sizeof(struct sHeader);
	sz -= sizeof(struct sHeader);

	if (header->mapper1 & 0x4) {
		data += 512;
		sz -= 512;
	}
	cart->mapper_id = ((header->mapper2 >> 4) << 4) | (header->mapper1 >> 4);
	cart->mirror = (header->mapper1 & 1) ? VERTICAL : HORIZONTAL;

	// TODO CHECK FILETYPE
	uint8_t file_type = 1;
	if (file_type == 1)
	{
		cart->prog_banks = header->prg_rom_chunks;
		cart->v_prog_size = cart->prog_banks * 16384;
		cart->chr_banks = header->chr_rom_chunks;
		cart->v_chr_size = cart->chr_banks * 8192;
		cart->v_prog_memory = (uint8_t*)calloc(cart->v_prog_size, sizeof(uint8_t));
		cart->v_chr_memory = (uint8_t*)calloc(cart->v_chr_size, sizeof(uint8_t));
		if (!cart->v_prog_memory || !cart->v_chr_memory)
		{
			free(cart->v_prog_memory); free(cart->v_chr_memory);
			free(cart);
			return 0;
		}
		memcpy(cart->v_prog_memory, data, cart->v_prog_size);
		data += cart->v_prog_size;
		memcpy(cart->v_chr_memory, data, cart->v_chr_size);
		data += cart->v_chr_size;
	}

	switch (cart->mapper_id)
	{
	case 0:
		cart->mapper = NES_MAP_Alloc000(cart->prog_banks, cart->chr_banks);
		break;
	default:
		break;
	}

	cart->is_image_valid = 1;
	return cart;
}
void NES_Cart_Free(struct Cartridge** cart)
{
	if (cart && *cart)
	{
		struct Cartridge* c = *cart;
		if (c->mapper)
		{
			NES_MAP_Free(&c->mapper);
		}
		if (c->v_prog_memory) free(c->v_prog_memory);
		c->v_prog_memory = 0; c->v_prog_size = 0;
		if (c->v_chr_memory) free(c->v_chr_memory);
		c->v_chr_memory = 0; c->v_chr_memory = 0;

		free(c);
		*cart = 0;
	}
}

uint8_t NES_Cart_CpuRead(struct Cartridge* cart, uint16_t addr, uint8_t* data)
{
	uint32_t mapped_addr = 0;
	if (cart->mapper->cpu_map_read(cart->mapper, addr, &mapped_addr) && cart->v_prog_size > 0)
	{
		*data = cart->v_prog_memory[mapped_addr];
		return 1;
	}
	return 0;
}
uint8_t NES_Cart_CpuWrite(struct Cartridge* cart, uint16_t addr, uint8_t data)
{
	uint32_t mapped_addr = 0;
	if (cart->mapper->cpu_map_write(cart->mapper, addr, &mapped_addr, data) && cart->v_prog_size > 0)
	{
		cart->v_prog_memory[mapped_addr] = data;
		return 1;
	}
	return 0;
}

uint8_t NES_Cart_PpuRead(struct Cartridge* cart, uint16_t addr, uint8_t* data)
{
	uint32_t mapped_addr = 0;
	if (cart->mapper->ppu_map_read(cart->mapper, addr, &mapped_addr) && cart->v_chr_size > 0)
	{
		*data = cart->v_chr_memory[mapped_addr];
		return 1;
	}
	return 0;
}
uint8_t NES_Cart_PpuWrite(struct Cartridge* cart, uint16_t addr, uint8_t data)
{
	uint32_t mapped_addr = 0;
	if (cart->mapper->ppu_map_write(cart->mapper, addr, &mapped_addr) && cart->v_chr_size > 0)
	{
		cart->v_chr_memory[mapped_addr] = data;
		return 1;
	}
	return 0;
}