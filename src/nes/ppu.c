#include "ppu.h"
#include "cartridge.h"
#include "../Helper.h"

const color DEFAULT_PALLET[64] = {
	{ 84,  84,  84 },{ 0,  30, 116 },{ 8 , 16, 144 },{ 48 ,  0, 136 },{ 68,   0, 100 },{ 92,   0,  48 },{ 84,   4,   0 },{ 60,  24,   0 },{ 32,  42,   0 },{ 8,  58,   0 },{ 0,  64,   0 },{ 0,  60,   0 },{ 0,  50,  60 },
	{ 0,   0,   0 },{ 0,   0,   0 },{ 0,   0,   0 },{ 152, 150, 152 },{ 8,  76, 196 },{ 48,  50, 236 },{ 92,  30, 228 },{ 136,  20, 176 },{ 160,  20, 100 },{ 152,  34,  32 },{ 120,  60,   0 },{ 84,  90,   0 },{ 40, 114,   0 },
	{ 8, 124,   0 },{ 0, 118,  40 },{ 0, 102, 120 },{ 0,   0,   0 },{ 0,   0,   0 },{ 0,   0,   0 },{ 236, 238, 236 },{ 76, 154, 236 },{ 120, 124, 236 },{ 176,  98, 236 },{ 228,  84, 236 },{ 236,  88, 180 },{ 236, 106, 100 },{ 212, 136,  32 },
	{ 160, 170,   0 },{ 116, 196,   0 },{ 76, 208,  32 },{ 56, 204, 108 },{ 56, 180, 204 },{ 60,  60,  60 },{ 0,0,0 },{ 0,0,0 },{ 236, 238, 236 },{ 168, 204, 236 },{ 188, 188, 236 },{ 212, 178, 236 },{ 236, 174, 236 },
	{ 236, 174, 212 },{ 236, 180, 176 },{ 228, 196, 144 },{ 204, 210, 120 },{ 180, 222, 120 },{ 168, 226, 144 },{ 152, 226, 180 },{ 160, 214, 228 },{ 160, 162, 160 },{ 0, 0, 0 },{ 0, 0, 0 }
};

static color PPU_GetColorFromPaletteRam(struct PPU* ppu, uint8_t pallett, uint8_t pixel)
{
	return ppu->pallette[PPU_Read(ppu, (0x3F00 + (pallett << 2) + pixel), 0) & 0x3F];
}


struct PPU* PPU_Alloc()
{
	struct PPU* result = (struct PPU*)malloc(sizeof(struct PPU));
	if (!result) return 0;
	memset(result, 0, sizeof(struct PPU));
	memcpy(result->pallette, DEFAULT_PALLET, sizeof(DEFAULT_PALLET));
	result->pOAM = (uint8_t*)result->OAM;
	PPU_Reset(result);
	return result;
}
void PPU_Free(struct PPU** ppu)
{
	if (ppu && *ppu)
	{
		struct PPU* p = *ppu;
		free(p);
		*ppu = 0;
	}
}

uint8_t PPU_CPURead(struct PPU* ppu, uint16_t addr, uint8_t isReadOnly)
{
	uint8_t data = 0x00;

	if (isReadOnly)
	{
		switch (addr)
		{
		case 0x0000: // Control
			data = ppu->control.reg;
			break;
		case 0x0001: // Mask
			data = ppu->mask.reg;
			break;
		case 0x0002: // Status
			data = ppu->status.reg;
			break;
		case 0x0003: // OAM Address
			break;
		case 0x0004: // OAM Data
			break;
		case 0x0005: // Scroll
			break;
		case 0x0006: // PPU Address
			break;
		case 0x0007: // PPU Data
			break;
		}
	}
	else
	{
		switch (addr)
		{
		case 0x0000: // Control
			break;
		case 0x0001: // Mask
			break;
		case 0x0002: // Status
			data = (ppu->status.reg & 0xE0) | (ppu->ppu_buffer & 0x1F);
			ppu->status.vertical_blank = 0;
			ppu->addr_latch = 0;
			break;
		case 0x0003: // OAM Address
			break;
		case 0x0004: // OAM Data
			data = ppu->pOAM[ppu->oam_addr];
			break;
		case 0x0005: // Scroll
			break;
		case 0x0006: // PPU Address
			break;
		case 0x0007: // PPU Data
			data = ppu->ppu_buffer;
			ppu->ppu_buffer = PPU_Read(ppu, ppu->vram_addr.reg, 0);

			if (ppu->vram_addr.reg > 0x3F00)
			{
				data = ppu->ppu_buffer;
			}
			ppu->vram_addr.reg += (ppu->control.increment_mode ? 32 : 1);
			break;
		}
	}
	return data;
}
void PPU_CPUWrite(struct PPU* ppu, uint16_t addr, uint8_t data)
{
	switch (addr)
	{
	case 0x0000: // Control
		ppu->control.reg = data;
		ppu->tram_addr.nametable_x = ppu->control.nametable_x;
		ppu->tram_addr.nametable_y = ppu->control.nametable_y;
		break;
	case 0x0001: // Mask
		ppu->mask.reg = data;
		break;
	case 0x0002: // Status
		break;
	case 0x0003: // OAM Address
		ppu->oam_addr = data;
		break;
	case 0x0004: // OAM Data
		ppu->pOAM[ppu->oam_addr] = data;
		break;
	case 0x0005: // Scroll
		if (ppu->addr_latch == 0)
		{
			ppu->fine_x = data & 0x07;
			ppu->tram_addr.coarse_x = data >> 3;
			ppu->addr_latch = 1;
		}
		else
		{
			ppu->tram_addr.fine_y = data & 0x07;
			ppu->tram_addr.coarse_y = data >> 3;
			ppu->addr_latch = 0;
		}
		break;
	case 0x0006: // PPU Address
		if (ppu->addr_latch == 0)
		{
			ppu->tram_addr.reg = (uint16_t)((data & 0x3F) << 8) | (ppu->tram_addr.reg & 0x00FF);
			ppu->addr_latch = 1;
		}
		else
		{
			ppu->tram_addr.reg = (ppu->tram_addr.reg & 0xFF00) | data;
			ppu->vram_addr = ppu->tram_addr;
			ppu->addr_latch = 0;
		}
		break;
	case 0x0007: // PPU Data
		PPU_Write(ppu, ppu->vram_addr.reg, data);
		ppu->vram_addr.reg += (ppu->control.increment_mode ? 32 : 1);
		break;
	}
}

uint8_t PPU_Read(struct PPU* ppu, uint16_t addr, uint8_t isReadOnly)
{
	uint8_t data = 0x00;
	addr &= 0x3FFF;

	if (Cart_PpuRead(ppu->cart, addr, &data))
	{

	}
	else if (addr >= 0x00 && addr <= 0x1FFF)
	{
		data = ppu->tbl_pattern[(addr & 0x1000) >> 12][addr & 0x0FFF];
	}
	else if (addr >= 0x2000 && addr <= 0x3EFF)
	{
		addr &= 0x0FFF;
		if (ppu->cart->mirror == VERTICAL)
		{
			if (addr >= 0x0000 && addr <= 0x03FF)
			{
				data = ppu->tbl_name[0][addr & 0x03FF];
			}
			if (addr >= 0x0400 && addr <= 0x07FF)
			{
				data = ppu->tbl_name[1][addr & 0x03FF];
			}
			if (addr >= 0x0800 && addr <= 0xBFF)
			{
				data = ppu->tbl_name[0][addr & 0x03FF];
			}
			if (addr >= 0x0C00 && addr <= 0x0FFF)
			{
				data = ppu->tbl_name[1][addr & 0x03FF];
			}
		}
		else if (ppu->cart->mirror == HORIZONTAL)
		{
			if (addr >= 0x0 && addr <= 0x03FF)
			{
				data = ppu->tbl_name[0][addr & 0x03FF];
			}
			if (addr >= 0x0400 && addr <= 0x07FF)
			{
				data = ppu->tbl_name[0][addr & 0x03FF];
			}
			if (addr >= 0x0800 && addr <= 0x0BFF)
			{
				data = ppu->tbl_name[1][addr & 0x03FF];
			}
			if (addr >= 0x0C00 && addr <= 0x0FFF)
			{
				data = ppu->tbl_name[1][addr & 0x03FF];
			}
		}
	}
	else if (addr >= 0x3F00 && addr <= 0x3FFF)
	{
		addr &= 0x001F;
		if (addr == 0x010)
		{
			addr = 0x0;
		}
		if (addr == 0x014)
		{
			addr = 0x04;
		}
		if (addr == 0x018)
		{
			addr = 0x08;
		}
		if (addr == 0x01C)
		{
			addr = 0x0C;
		}
		data = ppu->tbl_palette[addr] & (ppu->mask.grayscale ? 0x30 : 0x3F);
	}

	return data;
}
void PPU_Write(struct PPU* ppu, uint16_t addr, uint8_t data)
{
	addr &= 0x3FFF;
	
	if (Cart_PpuWrite(ppu->cart, addr, data))
	{

	}
	else if (addr >= 0x00 && addr <= 0x1FFF)
	{
		ppu->tbl_pattern[(addr & 0x1000) >> 12][addr & 0x0FFF] = data;
	}
	else if (addr >= 0x2000 && addr <= 0x3EFF)
	{
		addr &= 0x0FFF;
		if (ppu->cart->mirror == VERTICAL)
		{
			if (addr >= 0x0000 && addr <= 0x03FF)
			{
				ppu->tbl_name[0][addr & 0x03FF] = data;
			}
			if (addr >= 0x0400 && addr <= 0x07FF)
			{
				ppu->tbl_name[1][addr & 0x03FF] = data;
			}
			if (addr >= 0x0800 && addr <= 0xBFF)
			{
				ppu->tbl_name[0][addr & 0x03FF] = data;
			}
			if (addr >= 0x0C00 && addr <= 0x0FFF)
			{
				ppu->tbl_name[1][addr & 0x03FF] = data;
			}
		}
		else if (ppu->cart->mirror == HORIZONTAL)
		{
			if (addr >= 0x0 && addr <= 0x03FF)
			{
				ppu->tbl_name[0][addr & 0x03FF] = data;
			}
			if (addr >= 0x0400 && addr <= 0x07FF)
			{
				ppu->tbl_name[0][addr & 0x03FF] = data;
			}
			if (addr >= 0x0800 && addr <= 0x0BFF)
			{
				ppu->tbl_name[1][addr & 0x03FF] = data;
			}
			if (addr >= 0x0C00 && addr <= 0x0FFF)
			{
				ppu->tbl_name[1][addr & 0x03FF] = data;
			}
		}
	}
	else if (addr >= 0x3F00 && addr <= 0x3FFF)
	{
		addr &= 0x001F;
		if (addr == 0x010)
		{
			addr = 0x0;
		}
		if (addr == 0x014)
		{
			addr = 0x04;
		}
		if (addr == 0x018)
		{
			addr = 0x08;
		}
		if (addr == 0x01C)
		{
			addr = 0x0C;
		}
		ppu->tbl_palette[addr] = data;
	}
}


static uint8_t FlipByte(uint8_t b)
{
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

void PPU_Clock(struct PPU* ppu)
{
#define IncrementScrollX()	if(ppu->mask.render_background || ppu->mask.render_sprites)\
							{\
								if(ppu->vram_addr.coarse_x == 31)\
								{\
									ppu->vram_addr.coarse_x = 0;\
									ppu->vram_addr.nametable_x = ~ppu->vram_addr.nametable_x;\
								}\
								else\
								{\
									ppu->vram_addr.coarse_x++;\
								}\
							}
#define IncrementScrollY()	if(ppu->mask.render_background || ppu->mask.render_sprites)\
							{\
								if(ppu->vram_addr.fine_y < 7)\
								{\
									ppu->vram_addr.fine_y++;\
								}\
								else\
								{\
									ppu->vram_addr.fine_y = 0;\
									if(ppu->vram_addr.coarse_y == 29)\
									{\
										ppu->vram_addr.coarse_y = 0;\
										ppu->vram_addr.nametable_y = ~ppu->vram_addr.nametable_y;\
									}\
									else if(ppu->vram_addr.coarse_y == 31)\
									{\
										ppu->vram_addr.coarse_y = 0;\
									}\
									else\
									{\
										ppu->vram_addr.coarse_y++;\
									}\
								}\
							}
#define TransferAddressX()	if(ppu->mask.render_background || ppu->mask.render_sprites)\
							{\
								ppu->vram_addr.nametable_x = ppu->tram_addr.nametable_x;\
								ppu->vram_addr.coarse_x = ppu->tram_addr.coarse_x;\
							}
#define TransferAddressY()	if(ppu->mask.render_background || ppu->mask.render_sprites)\
							{\
								ppu->vram_addr.fine_y = ppu->tram_addr.fine_y;\
								ppu->vram_addr.nametable_y = ppu->tram_addr.nametable_y;\
								ppu->vram_addr.coarse_y = ppu->tram_addr.coarse_y;\
							}
#define LoadBackgroundShifters(){\
									ppu->bg_shifter_pattern_lo = (ppu->bg_shifter_pattern_lo & 0xFF00) | ppu->bg_next_tile_lsb;\
									ppu->bg_shifter_pattern_hi = (ppu->bg_shifter_pattern_hi & 0xFF00) | ppu->bg_next_tile_msb;\
									ppu->bg_shifter_attrib_lo = (ppu->bg_shifter_attrib_lo & 0xFF00) | ((ppu->bg_next_tile_attrib & 0b01) ? 0xFF : 0);\
									ppu->bg_shifter_attrib_hi = (ppu->bg_shifter_attrib_hi & 0xFF00) | ((ppu->bg_next_tile_attrib & 0b10) ? 0xFF : 0);\
								}
#define UpdateShifters()	if(ppu->mask.render_background)\
							{\
								ppu->bg_shifter_pattern_lo <<= 1;\
								ppu->bg_shifter_pattern_hi <<= 1;\
								ppu->bg_shifter_attrib_lo <<= 1;\
								ppu->bg_shifter_attrib_hi <<= 1;\
							}\
							if(ppu->mask.render_sprites && ppu->cycle >= 1 && ppu->cycle < 258)\
							{\
								for(int macro_i = 0; macro_i < ppu->sprite_count; macro_i++)\
								{\
									if(ppu->sprite_scanline[macro_i].x > 0)\
									{\
										ppu->sprite_scanline[macro_i].x--;\
									}\
									else\
									{\
										ppu->sprite_shifter_pattern_lo[macro_i] <<= 1;\
										ppu->sprite_shifter_pattern_hi[macro_i] <<= 1;\
									}\
								}\
							}

	if (ppu->scanline >= -1 && ppu->scanline < 240)
	{
		if (ppu->scanline == 0 && ppu->cycle == 0) ppu->cycle = 1;
		if (ppu->scanline == -1 && ppu->cycle == 1) 
		{
			ppu->status.vertical_blank = 0;
			ppu->status.sprite_overflow = 0;
			ppu->status.sprite_zero_hit = 0;
			for (int i = 0; i < 8; i++)
			{
				ppu->sprite_shifter_pattern_lo[i] = 0;
				ppu->sprite_shifter_pattern_hi[i] = 0;
			}
		}
		if ((ppu->cycle >= 2 && ppu->cycle < 258) || (ppu->cycle >= 321 && ppu->cycle < 338))
		{
			UpdateShifters();
			
			switch ((ppu->cycle - 1) % 8)
			{
			case 0:
				LoadBackgroundShifters();
				ppu->bg_next_tile_id = PPU_Read(ppu, 0x2000 | (ppu->vram_addr.reg & 0xFFF), 0);
				break;
			case 2:
				ppu->bg_next_tile_attrib = PPU_Read(ppu, 0x23C0 | (ppu->vram_addr.nametable_y << 11) | (ppu->vram_addr.nametable_x << 10) | ((ppu->vram_addr.coarse_y >> 2) << 3) | (ppu->vram_addr.coarse_x >> 2), 0);
				if (ppu->vram_addr.coarse_y & 0x2) ppu->bg_next_tile_attrib >>= 4;
				if (ppu->vram_addr.coarse_x & 0x2) ppu->bg_next_tile_attrib >>= 2;
				ppu->bg_next_tile_attrib &= 0x3;
				break;
			case 4:
				ppu->bg_next_tile_lsb = PPU_Read(ppu, (ppu->control.pattern_background << 12) + ((uint16_t)ppu->bg_next_tile_id << 4) + (ppu->vram_addr.fine_y), 0);
				break;
			case 6:
				ppu->bg_next_tile_msb = PPU_Read(ppu, (ppu->control.pattern_background << 12) + ((uint16_t)ppu->bg_next_tile_id << 4) + (ppu->vram_addr.fine_y) + 8, 0);
				break;
			case 7:
				IncrementScrollX();
				break;
			}
		}
		if (ppu->cycle == 256)
		{
			IncrementScrollY();
		}
		if (ppu->cycle == 257)
		{
			LoadBackgroundShifters();
			TransferAddressX();
		}
		if (ppu->cycle == 338 || ppu->cycle == 340)
		{
			ppu->bg_next_tile_id = PPU_Read(ppu, 0x2000 | (ppu->vram_addr.reg & 0x0FFF), 0);
		}
		if (ppu->scanline == -1 && ppu->cycle >= 280 && ppu->cycle < 305)
		{
			TransferAddressY();
		}

		if (ppu->cycle == 257 && ppu->scanline >= 0)
		{
			memset(ppu->sprite_scanline, 0xFF, 8 * sizeof(struct sObjectAttributeEntry));
			ppu->sprite_count = 0;

			for (uint8_t i = 0; i < 8; i++)
			{
				ppu->sprite_shifter_pattern_lo[i] = 0;
				ppu->sprite_shifter_pattern_hi[i] = 0;
			}

			uint8_t nOAMEntry = 0;
			ppu->is_sprite_zero_hit_possible = 0;
			while (nOAMEntry < 64 && ppu->sprite_count < 9)
			{
				int16_t diff = ((int16_t)ppu->scanline - (int16_t)ppu->OAM[nOAMEntry].y);
				if (diff >= 0 && diff < (ppu->control.sprite_size ? 16 : 8))
				{
					if (ppu->sprite_count < 8)
					{
						if (nOAMEntry == 0)
						{
							ppu->is_sprite_zero_hit_possible = 1;
						}
						memcpy(&ppu->sprite_scanline[ppu->sprite_count], &ppu->OAM[nOAMEntry], sizeof(struct sObjectAttributeEntry));
						ppu->sprite_count++;
					}
				}
				nOAMEntry++;
			}
			ppu->status.sprite_overflow = (ppu->sprite_count > 8);
		}


		if (ppu->cycle == 340)
		{

			for (uint8_t i = 0; i < ppu->sprite_count; i++)
			{
				uint8_t sprite_pattern_bits_lo = 0;
				uint8_t sprite_pattern_bits_hi = 0;
				uint16_t sprite_pattern_addr_lo = 0;
				uint16_t sprite_pattern_addr_hi = 0;


				if (!ppu->control.sprite_size)
				{
					//8x8 mode
					if (!(ppu->sprite_scanline[i].attribute & 0x80))
					{
						sprite_pattern_addr_lo = (ppu->control.pattern_sprite << 12) | (ppu->sprite_scanline[i].id << 4) | (ppu->scanline - ppu->sprite_scanline[i].y);
					}
					else
					{
						//flipped Vertically
						sprite_pattern_addr_lo = (ppu->control.pattern_sprite << 12) | (ppu->sprite_scanline[i].id << 4) | (7 - (ppu->scanline - ppu->sprite_scanline[i].y));
					}
				}
				else
				{
					//8x16 mode
					if (!(ppu->sprite_scanline[i].attribute & 0x80))
					{
						if ((ppu->scanline - ppu->sprite_scanline[i].y) < 8)
						{
							//topHalf
							sprite_pattern_addr_lo = ((ppu->sprite_scanline[i].id & 0x01) << 12) | ((ppu->sprite_scanline[i].id & 0xFE) << 4) | ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07);
						}
						else
						{
							//bottomHalf
							sprite_pattern_addr_lo = ((ppu->sprite_scanline[i].id & 0x01) << 12) | (((ppu->sprite_scanline[i].id & 0xFE) + 1) << 4) | ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07);
						}
					}
					else
					{
						if ((ppu->scanline - ppu->sprite_scanline[i].y) < 8)
						{
							sprite_pattern_addr_lo = ((ppu->sprite_scanline[i].id & 0x01) << 12) | (((ppu->sprite_scanline[i].id & 0xFE) + 1) << 4) | (7 - (ppu->scanline - ppu->sprite_scanline[i].y) & 0x07);
						}
						else
						{
							sprite_pattern_addr_lo = ((ppu->sprite_scanline[i].id & 0x01) << 12) | ((ppu->sprite_scanline[i].id & 0xFE) << 4) | (7 - (ppu->scanline - ppu->sprite_scanline[i].y) & 0x07);
						}
					}
				}

				sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;
				sprite_pattern_bits_lo = PPU_Read(ppu, sprite_pattern_addr_lo, 0);
				sprite_pattern_bits_hi = PPU_Read(ppu, sprite_pattern_addr_hi, 0);

				if (ppu->sprite_scanline[i].attribute & 0x40)
				{
					//Horizontal flip
					sprite_pattern_bits_lo = FlipByte(sprite_pattern_bits_lo);
					sprite_pattern_bits_hi = FlipByte(sprite_pattern_bits_hi);
				}
				ppu->sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
				ppu->sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;

			}
		}
	}

	if (ppu->scanline >= 241 && ppu->scanline < 261)
	{
		if (ppu->scanline == 241 && ppu->cycle == 1)
		{
			ppu->status.vertical_blank = 1;
			if (ppu->control.enable_nmi)
			{
				ppu->is_nmi = 1;
			}
		}
	}
	uint8_t bg_pixel = 0x00;
	uint8_t bg_palette = 0x00;

	if (ppu->mask.render_background)
	{
		uint16_t bit_mux = 0x8000 >> ppu->fine_x;

		uint8_t p0_pixel = (ppu->bg_shifter_pattern_lo & bit_mux) > 0;
		uint8_t p1_pixel = (ppu->bg_shifter_pattern_hi & bit_mux) > 0;
		bg_pixel = (p1_pixel << 1) | p0_pixel;

		uint8_t bg_pal0 = (ppu->bg_shifter_attrib_lo & bit_mux) > 0;
		uint8_t bg_pal1 = (ppu->bg_shifter_attrib_hi & bit_mux) > 0;
		bg_palette = (bg_pal1 << 1) | bg_pal0;
	}

	uint8_t fg_pixel = 0x00;
	uint8_t fg_palette = 0x00;
	uint8_t fg_priority = 0x00;

	if (ppu->mask.render_sprites)
	{
		ppu->is_sprite_zero_being_rendered = 0;
	
		for (uint8_t i = 0; i < ppu->sprite_count; i++)
		{
			if (ppu->sprite_scanline[i].x == 0)
			{
				uint8_t fg_pixel_lo = (ppu->sprite_shifter_pattern_lo[i] & 0x80) > 0;
				uint8_t fg_pixel_hi = (ppu->sprite_shifter_pattern_hi[i] & 0x80) > 0;
				fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;
	
				fg_palette = (ppu->sprite_scanline[i].attribute & 0x03) + 0x04;
				fg_priority = (ppu->sprite_scanline[i].attribute & 0x20) == 0;
				if (fg_pixel != 0)
				{
					if (i == 0)
					{
						ppu->is_sprite_zero_being_rendered = 1;
					}
					break;
				}
			}
		}
	}

	uint8_t pixel = 0x00;
	uint8_t pltte = 0x00;

	if (bg_pixel == 0 && fg_pixel == 0)
	{
		pixel = 0x00;
		pltte = 0x00;
	}
	else if (bg_pixel == 0 && fg_pixel > 0)
	{
		pixel = fg_pixel;
		pltte = fg_palette;
	}
	else if (bg_pixel > 0 && fg_pixel == 0)
	{
		pixel = bg_pixel;
		pltte = bg_palette;
	}
	else if (bg_pixel > 0 && fg_pixel > 0)
	{
		if (fg_priority)
		{
			pixel = fg_pixel;
			pltte = fg_palette;
		}
		else
		{
			pixel = bg_pixel;
			pltte = bg_palette;
		}

		if (ppu->is_sprite_zero_hit_possible && ppu->is_sprite_zero_being_rendered)
		{
			if (ppu->mask.render_background & ppu->mask.render_sprites)
			{
				if (~(ppu->mask.render_background_left | ppu->mask.render_sprites_left))
				{
					if (ppu->cycle >= 9 && ppu->cycle < 258)
					{
						ppu->status.sprite_zero_hit = 1;
					}

				}
				else
				{
					if (ppu->cycle >= 1 && ppu->cycle < 258)
					{
						ppu->status.sprite_zero_hit = 1;
					}
				}
			}
		}
	}

	if (ppu->cycle - 1 >= 0 && ppu->cycle < 256 && ppu->scanline >= 0 && ppu->scanline < 240)
	{
		ppu->out_screen[ppu->cycle - 1 + ppu->scanline * 256] = PPU_GetColorFromPaletteRam(ppu, pltte, pixel);
	}

	ppu->cycle++;
	if (ppu->cycle >= 341)
	{
		ppu->cycle = 0;
		ppu->scanline++;
		if (ppu->scanline >= 261)
		{
			ppu->scanline = -1;
			ppu->is_frame_complete = 1;
		}
	}
}

void PPU_Reset(struct PPU* ppu)
{
	ppu->fine_x = 0x00;
	ppu->addr_latch = 0x00;
	ppu->ppu_buffer = 0x00;
	ppu->scanline = 0x0;
	ppu->cycle = 0x0;

	ppu->bg_next_tile_id = 0x00;
	ppu->bg_next_tile_attrib = 0x00;
	ppu->bg_next_tile_lsb = 0x00;
	ppu->bg_next_tile_msb = 0x00;
	ppu->bg_shifter_pattern_lo = 0x00;
	ppu->bg_shifter_pattern_hi = 0x00;
	ppu->bg_shifter_attrib_lo = 0x00;
	ppu->bg_shifter_attrib_hi = 0x00;


	ppu->status.reg = 0x0;
	ppu->mask.reg = 0x0;
	ppu->control.reg = 0x0;
	ppu->vram_addr.reg = 0x00;
	ppu->tram_addr.reg = 0x00;
}

color* PPU_GetPatternTable(struct PPU* ppu, uint8_t i, uint8_t palett)
{
	for (uint16_t y = 0; y < 16; y++)
	{
		for (uint16_t x = 0; x < 16; x++)
		{
			uint16_t offset = y * 256 + x * 16;
			for (uint16_t row = 0; row < 8; row++)
			{
				uint8_t tilelsb = PPU_Read(ppu, i * 0x1000 + offset + row + 0x00, 0);
				uint8_t tilemsb = PPU_Read(ppu, i * 0x1000 + offset + row + 0x08, 0);

				for (uint16_t col = 0; col < 8; col++)
				{
					uint8_t pixel = ((tilelsb & 0x01) << 1) | (tilemsb & 0x01);
					tilelsb >>= 1;
					tilemsb >>= 1;

					ppu->out_pattern_table[i][x * 8 + (7 - col) + 128 * (y * 8 + row)] = PPU_GetColorFromPaletteRam(ppu, palett, pixel);
				}

			}
		}
	}
	return ppu->out_pattern_table[i];
}
