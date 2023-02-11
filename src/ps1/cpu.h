#pragma once
#include "../Helper.h"



struct CPU
{
	struct Bus* bus;
	uint32_t pc;
	union {
		struct
		{
			uint32_t zero;
			uint32_t at;
			uint32_t v0; uint32_t v1;
			uint32_t a0; uint32_t a1; uint32_t a2; uint32_t a3; 
			uint32_t t0; uint32_t t1; uint32_t t2; uint32_t t3;
			uint32_t t4; uint32_t t5; uint32_t t6; uint32_t t7;
			uint32_t s0; uint32_t s1; uint32_t s2; uint32_t s3;
			uint32_t s4; uint32_t s5; uint32_t s6; uint32_t s7;
			uint32_t k0; uint32_t k1;
			uint32_t gp;
			uint32_t sp;
			uint32_t fp;
			uint32_t ra;
		};
		uint32_t regs[32];
	};
};



struct CPU* PS1_CPU_Alloc(struct Bus* bus);
void PS1_CPU_Free(struct CPU** cpu);

void PS1_CPU_Clock(struct CPU* cpu);