#pragma once
#include "../Helper.h"

enum CPU_FLAGS
{
	CF = (0x1 << 0x0), // CarryFlag
	ZF = (0x1 << 0x1), // ZeroFlag
	ID = (0x1 << 0x2), // Interrupts Disable
	DM = (0x1 << 0x3), // Decimal Mode
	BC = (0x1 << 0x4), // Break Command
	UF = (0x1 << 0x5), // Unused Flag
	VF = (0x1 << 0x6), // Overflow Flag
	NF = (0x1 << 0x7), // Negativ Flag
};

struct CPU
{
	uint16_t pc;

	uint8_t sp;
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t status;


	uint8_t fetched;
	uint8_t opcode;
	uint8_t cycles;
	uint16_t temp;
	uint16_t addr_abs;
	uint32_t clock_count;
	uint16_t addr_rel;

	struct DataBus* bus;

};

struct CPU* NES_CPU_Alloc();
void NES_CPU_Free(struct CPU** cpu);

void NES_CPU_Reset(struct CPU* cpu);

// interrupt request
void NES_CPU_IRQ(struct CPU* cpu);
// NonMaskable interrupt
void NES_CPU_NMI(struct CPU* cpu);

void NES_CPU_Clock(struct CPU* cpu);
int NES_CPU_IsComplete(struct CPU* cpu);