#pragma once
#include "../Helper.h"

typedef struct _color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
}color;




struct PPU
{
	struct Cartridge* cart;
	uint8_t* pOAM;

	uint8_t oam_addr;

	uint8_t is_frame_complete;
	uint8_t is_nmi;

	uint8_t tbl_name[2][1024];
	uint8_t tbl_palette[32];
	uint8_t tbl_pattern[2][4096];

	color pallette[0x40];
	color out_screen[256 * 240];
	color out_name_table[2][256 * 240];
	color out_pattern_table[2][128 * 128];

	int16_t scanline;
	int16_t cycle;

	union
	{
		struct
		{
			uint8_t unused : 5;
			uint8_t sprite_overflow : 1;
			uint8_t sprite_zero_hit : 1;
			uint8_t vertical_blank : 1;
		};
		uint8_t reg;
	}status;

	union
	{
		struct
		{
			uint8_t grayscale : 1;
			uint8_t render_background_left : 1;
			uint8_t render_sprites_left : 1;
			uint8_t render_background : 1;
			uint8_t render_sprites : 1;
			uint8_t enhance_red : 1;
			uint8_t enhance_green : 1;
			uint8_t enhance_blue : 1;
		};
		uint8_t reg;
	}mask;

	union PPUCTRL
	{
		struct
		{
			uint8_t nametable_x : 1;
			uint8_t nametable_y : 1;
			uint8_t increment_mode : 1;
			uint8_t pattern_sprite : 1;
			uint8_t pattern_background : 1;
			uint8_t sprite_size : 1;
			uint8_t slave_mode : 1;
			uint8_t enable_nmi : 1;
		};
		uint8_t reg;
	}control;

	uint8_t addr_latch;
	uint8_t ppu_buffer;

	union loopy_register
	{
		struct
		{
			uint16_t coarse_x : 5;
			uint16_t coarse_y : 5;
			uint16_t nametable_x : 1;
			uint16_t nametable_y : 1;
			uint16_t fine_y : 3;
			uint16_t unused : 1;
		};
		uint16_t reg;
	};

	union loopy_register vram_addr;
	union loopy_register tram_addr;

	uint8_t fine_x;

	uint8_t bg_next_tile_id;
	uint8_t bg_next_tile_attrib;
	uint8_t bg_next_tile_lsb;
	uint8_t bg_next_tile_msb;

	uint16_t bg_shifter_pattern_lo;
	uint16_t bg_shifter_pattern_hi;
	uint16_t bg_shifter_attrib_lo;
	uint16_t bg_shifter_attrib_hi;

	struct sObjectAttributeEntry
	{
		uint8_t y;
		uint8_t id;
		uint8_t attribute;
		uint8_t x;
	}OAM[64];

	struct sObjectAttributeEntry sprite_scanline[8];
	uint8_t sprite_count;
	uint8_t sprite_shifter_pattern_lo[8];
	uint8_t sprite_shifter_pattern_hi[8];

	uint8_t is_sprite_zero_hit_possible;
	uint8_t is_sprite_zero_being_rendered;
};




struct PPU* NES_PPU_Alloc();
void NES_PPU_Free(struct PPU** ppu);

uint8_t NES_PPU_CPURead(struct PPU* ppu, uint16_t addr, uint8_t isReadOnly);
void NES_PPU_CPUWrite(struct PPU* ppu, uint16_t addr, uint8_t data);

uint8_t NES_PPU_Read(struct PPU* ppu, uint16_t addr, uint8_t isReadOnly);
void NES_PPU_Write(struct PPU* ppu, uint16_t addr, uint8_t data);

void NES_PPU_Clock(struct PPU* ppu);
void NES_PPU_Reset(struct PPU* ppu);


color* NES_PPU_GetPatternTable(struct PPU* ppu, uint8_t i, uint8_t palett);