#pragma once
#include "../Helper.h"

enum CartridgeMirrorMode
{
	HORIZONTAL,
	VERTICAL,
	ONESCREEN_LO,
	ONESCREEN_HI,
};

struct Cartridge
{
	enum CartridgeMirrorMode mirror;
	uint8_t* v_prog_memory;
	uint8_t* v_chr_memory;
	int v_prog_size;
	int v_chr_size;
	uint8_t mapper_id;
	uint8_t prog_banks;
	uint8_t chr_banks;
	uint8_t is_image_valid;

	struct Mapper* mapper;
};

struct Cartridge* Cart_AllocFromFile(const char* filename);
struct Cartridge* Cart_Alloc(uint8_t* data, int sz);
void Cart_Free(struct Cartridge** cart);

uint8_t Cart_CpuRead(struct Cartridge* cart, uint16_t addr, uint8_t* data);
uint8_t Cart_CpuWrite(struct Cartridge* cart, uint16_t addr, uint8_t data);

uint8_t Cart_PpuRead(struct Cartridge* cart, uint16_t addr, uint8_t* data);
uint8_t Cart_PpuWrite(struct Cartridge* cart, uint16_t addr, uint8_t data);