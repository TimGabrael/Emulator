#pragma once
#include "../Helper.h"



struct LoadInfo
{
	uint32_t reg;
	uint32_t val;
};

struct CPU
{
	struct Bus* bus;
	uint32_t pc;
	uint32_t next_pc;
	uint32_t regs[32];
	uint32_t out_regs[32];
	struct LoadInfo load;
	uint32_t sr; // cop0 status register

	uint32_t current_pc;// used in exceptions
	uint32_t cause; // cop0-13
	uint32_t epc; // cop0-14

	
	uint32_t hi;
	uint32_t lo;

	uint8_t is_branch;
	uint8_t is_delay_spot;
};



struct CPU* PS1_CPU_Alloc(struct Bus* bus);
void PS1_CPU_Free(struct CPU** cpu);
void PS1_CPU_Reset(struct CPU* cpu);
void PS1_CPU_Clock(struct CPU* cpu);
