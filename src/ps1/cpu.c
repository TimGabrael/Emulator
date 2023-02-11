#include "cpu.h"
#include "bus.h"

static uint32_t CPU_Read(struct CPU* cpu, uint32_t addr)
{
	return PS1_BUS_CpuRead(cpu->bus, addr);
}
static void CPU_Write(struct CPU* cpu, uint32_t addr, uint32_t val)
{
	return PS1_BUS_CpuWrite(cpu->bus, addr, val);
}

#define IMM(i) (i & 0xFFFF)
#define IMM_SE(i) (uint32_t)(int16_t)(i & 0xFFFF)
#define IMM_SHIFT(i) ((i >> 6) & 0x1F)
#define IMM_JUMP(i) (i & 0x3FFFFFF)
#define TO(i) ((i >> 16) & 0x1F)
#define ST(i) ((i >> 21) & 0x1F)
#define DR(i) ((i >> 11) & 0x1F)
#define FUN(i) (i >> 26)
#define SUBFUN(i) (i & 0x3F)

#define WRITE_REG(reg, val) cpu->regs[t] = val; cpu->regs[0] = 0

static void CPU_LUI(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = imm << 16;
	WRITE_REG(t, val);
}
static void CPU_ORI(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t v = cpu->regs[s] | imm;
	WRITE_REG(t, v);
}
static void CPU_STW(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = cpu->regs[s] + imm;
	const uint32_t val = cpu->regs[t];
	CPU_Write(cpu, addr, val);
}
static void CPU_SLL(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t i = IMM_SHIFT(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t d = DR(instruction);
	const uint32_t val = cpu->regs[t] << i;
	WRITE_REG(d, val);
}
static void CPU_ADDIU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t i = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t val = cpu->regs[s] + i;
	WRITE_REG(t, val);
}

static void CPU_DecodeAndExecute(struct CPU* cpu, uint32_t instruction)
{
	switch (FUN(instruction))
	{
	case 0b000000:
		switch (SUBFUN(instruction))
		{
		case 0b000000:
			CPU_SLL(cpu, instruction);
			break;

		default:
			break;
		}
		break;
	case 0b001111:
		CPU_LUI(cpu, instruction);
		break;
	case 0b001101:
		CPU_ORI(cpu, instruction);
		break;
	case 0b101011:
		CPU_STW(cpu, instruction);
		break;
	default:
		LOG("UNHANDLED INSTRUCTION: %x\n", instruction);
		SDL_ShowSimpleMessageBox(0, "ERR", "UNHANDLED INSTRUCTION", NULL);

		break;
	}
}

static void CPU_Reset(struct CPU* cpu)
{
	cpu->zero = 0; // should always be 0 but just in case reset to 0
	cpu->pc = 0xbfc00000;
	
}

struct CPU* PS1_CPU_Alloc(struct Bus* bus)
{
	struct CPU* out = (struct CPU*)malloc(sizeof(struct CPU));
	if (!out) return 0;
	memset(out, 0, sizeof(struct CPU));
	out->bus = bus;
	CPU_Reset(out);
	return out;
}
void PS1_CPU_Free(struct CPU** cpu)
{
	if (cpu && *cpu)
	{
		struct CPU* c = *cpu;
		free(c);
		*cpu = 0;
	}
}

void PS1_CPU_Clock(struct CPU* cpu)
{
	const uint32_t instruction = CPU_Read(cpu, cpu->pc);
	CPU_DecodeAndExecute(cpu, instruction);
	cpu->pc = cpu->pc + 4;
}