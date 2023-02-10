#pragma once
#include "../Helper.h"

struct DataBus
{
	struct PPU* ppu;
	struct CPU* cpu;

	struct Cartridge* cart;
	uint8_t controller[2];

	uint8_t cpu_ram[2048];

	uint8_t controller_state[2];
	uint32_t clock_counter;

	uint8_t dma_page;
	uint8_t dma_addr;
	uint8_t dma_data;

	uint8_t is_dma_transfer;
	uint8_t is_dma_dummy;

};


void DBus_Init(struct DataBus* bus, struct PPU* ppu, struct CPU* cpu);
void DBus_Uninit(struct DataBus* bus);

uint8_t DBus_CPURead(struct DataBus* bus, uint16_t addr, uint8_t isReadOnly);
void DBus_CPUWrite(struct DataBus* bus, uint16_t addr, uint8_t data);

void DBus_InsertCartridge(struct DataBus* bus, struct Cartridge* cart);
void DBus_Reset(struct DataBus* bus);
void DBus_Clock(struct DataBus* bus);