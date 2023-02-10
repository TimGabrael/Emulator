#include "cpu.h"
#include "DataBus.h"

#define GetFlag(f) (((cpu->status & f) > 0) ? 1 : 0)
#define SetFlag(f, set) if(set) { cpu->status |= f; } else { cpu->status &= ~f; }

typedef uint8_t(__stdcall*PINSTRUCTIONFN)(struct CPU*);
struct Instruction
{
	const char* name;
	PINSTRUCTIONFN operate;
	PINSTRUCTIONFN addrmode;
	uint8_t cycles;
};







static uint8_t CPU_IMP(struct CPU* cpu);
static uint8_t CPU_IMM(struct CPU* cpu);
static uint8_t CPU_ZP0(struct CPU* cpu);
static uint8_t CPU_ZPX(struct CPU* cpu);
static uint8_t CPU_ZPY(struct CPU* cpu);
static uint8_t CPU_REL(struct CPU* cpu);
static uint8_t CPU_ABS(struct CPU* cpu);
static uint8_t CPU_ABX(struct CPU* cpu);
static uint8_t CPU_ABY(struct CPU* cpu);
static uint8_t CPU_IND(struct CPU* cpu);
static uint8_t CPU_IZX(struct CPU* cpu);
static uint8_t CPU_IZY(struct CPU* cpu);
static uint8_t CPU_ADC(struct CPU* cpu);
static uint8_t CPU_AND(struct CPU* cpu);
static uint8_t CPU_ASL(struct CPU* cpu);
static uint8_t CPU_BCC(struct CPU* cpu);
static uint8_t CPU_BCS(struct CPU* cpu);
static uint8_t CPU_BEQ(struct CPU* cpu);
static uint8_t CPU_BIT(struct CPU* cpu);
static uint8_t CPU_BMI(struct CPU* cpu);
static uint8_t CPU_BNE(struct CPU* cpu);
static uint8_t CPU_BPL(struct CPU* cpu);
static uint8_t CPU_BRK(struct CPU* cpu);
static uint8_t CPU_BVC(struct CPU* cpu);
static uint8_t CPU_BVS(struct CPU* cpu);
static uint8_t CPU_CLC(struct CPU* cpu);
static uint8_t CPU_CLD(struct CPU* cpu);
static uint8_t CPU_CLI(struct CPU* cpu);
static uint8_t CPU_CLV(struct CPU* cpu);
static uint8_t CPU_CMP(struct CPU* cpu);
static uint8_t CPU_CPX(struct CPU* cpu);
static uint8_t CPU_CPY(struct CPU* cpu);
static uint8_t CPU_DEC(struct CPU* cpu);
static uint8_t CPU_DEX(struct CPU* cpu);
static uint8_t CPU_DEY(struct CPU* cpu);
static uint8_t CPU_EOR(struct CPU* cpu);
static uint8_t CPU_INC(struct CPU* cpu);
static uint8_t CPU_INX(struct CPU* cpu);
static uint8_t CPU_INY(struct CPU* cpu);
static uint8_t CPU_JMP(struct CPU* cpu);
static uint8_t CPU_JSR(struct CPU* cpu);
static uint8_t CPU_LDA(struct CPU* cpu);
static uint8_t CPU_LDX(struct CPU* cpu);
static uint8_t CPU_LDY(struct CPU* cpu);
static uint8_t CPU_LSR(struct CPU* cpu);
static uint8_t CPU_NOP(struct CPU* cpu);
static uint8_t CPU_ORA(struct CPU* cpu);
static uint8_t CPU_PHA(struct CPU* cpu);
static uint8_t CPU_PHP(struct CPU* cpu);
static uint8_t CPU_PLA(struct CPU* cpu);
static uint8_t CPU_PLP(struct CPU* cpu);
static uint8_t CPU_ROL(struct CPU* cpu);
static uint8_t CPU_ROR(struct CPU* cpu);
static uint8_t CPU_RTI(struct CPU* cpu);
static uint8_t CPU_RTS(struct CPU* cpu);
static uint8_t CPU_SBC(struct CPU* cpu);
static uint8_t CPU_SEC(struct CPU* cpu);
static uint8_t CPU_SED(struct CPU* cpu);
static uint8_t CPU_SEI(struct CPU* cpu);
static uint8_t CPU_STA(struct CPU* cpu);
static uint8_t CPU_STX(struct CPU* cpu);
static uint8_t CPU_STY(struct CPU* cpu);
static uint8_t CPU_TAX(struct CPU* cpu);
static uint8_t CPU_TAY(struct CPU* cpu);
static uint8_t CPU_TSX(struct CPU* cpu);
static uint8_t CPU_TXA(struct CPU* cpu);
static uint8_t CPU_TXS(struct CPU* cpu);
static uint8_t CPU_TYA(struct CPU* cpu);

static uint8_t CPU_XXX(struct CPU* cpu);



static const struct Instruction OPCODE_LIST[256] = {
		{ "BRK", &CPU_BRK, &CPU_IMM, 7 },{ "ORA", &CPU_ORA, &CPU_IZX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 3 },{ "ORA", &CPU_ORA, &CPU_ZP0, 3 },{ "ASL", &CPU_ASL, &CPU_ZP0, 5 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "PHP", &CPU_PHP, &CPU_IMP, 3 },{ "ORA", &CPU_ORA, &CPU_IMM, 2 },{ "ASL", &CPU_ASL, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "ORA", &CPU_ORA, &CPU_ABS, 4 },{ "ASL", &CPU_ASL, &CPU_ABS, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },
		{ "BPL", &CPU_BPL, &CPU_REL, 2 },{ "ORA", &CPU_ORA, &CPU_IZY, 5 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "ORA", &CPU_ORA, &CPU_ZPX, 4 },{ "ASL", &CPU_ASL, &CPU_ZPX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "CLC", &CPU_CLC, &CPU_IMP, 2 },{ "ORA", &CPU_ORA, &CPU_ABY, 4 },{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 7 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "ORA", &CPU_ORA, &CPU_ABX, 4 },{ "ASL", &CPU_ASL, &CPU_ABX, 7 },{ "???", &CPU_XXX, &CPU_IMP, 7 },
		{ "JSR", &CPU_JSR, &CPU_ABS, 6 },{ "AND", &CPU_AND, &CPU_IZX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "BIT", &CPU_BIT, &CPU_ZP0, 3 },{ "AND", &CPU_AND, &CPU_ZP0, 3 },{ "ROL", &CPU_ROL, &CPU_ZP0, 5 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "PLP", &CPU_PLP, &CPU_IMP, 4 },{ "AND", &CPU_AND, &CPU_IMM, 2 },{ "ROL", &CPU_ROL, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "BIT", &CPU_BIT, &CPU_ABS, 4 },{ "AND", &CPU_AND, &CPU_ABS, 4 },{ "ROL", &CPU_ROL, &CPU_ABS, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },
		{ "BMI", &CPU_BMI, &CPU_REL, 2 },{ "AND", &CPU_AND, &CPU_IZY, 5 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "AND", &CPU_AND, &CPU_ZPX, 4 },{ "ROL", &CPU_ROL, &CPU_ZPX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "SEC", &CPU_SEC, &CPU_IMP, 2 },{ "AND", &CPU_AND, &CPU_ABY, 4 },{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 7 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "AND", &CPU_AND, &CPU_ABX, 4 },{ "ROL", &CPU_ROL, &CPU_ABX, 7 },{ "???", &CPU_XXX, &CPU_IMP, 7 },
		{ "RTI", &CPU_RTI, &CPU_IMP, 6 },{ "EOR", &CPU_EOR, &CPU_IZX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 3 },{ "EOR", &CPU_EOR, &CPU_ZP0, 3 },{ "LSR", &CPU_LSR, &CPU_ZP0, 5 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "PHA", &CPU_PHA, &CPU_IMP, 3 },{ "EOR", &CPU_EOR, &CPU_IMM, 2 },{ "LSR", &CPU_LSR, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "JMP", &CPU_JMP, &CPU_ABS, 3 },{ "EOR", &CPU_EOR, &CPU_ABS, 4 },{ "LSR", &CPU_LSR, &CPU_ABS, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },
		{ "BVC", &CPU_BVC, &CPU_REL, 2 },{ "EOR", &CPU_EOR, &CPU_IZY, 5 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "EOR", &CPU_EOR, &CPU_ZPX, 4 },{ "LSR", &CPU_LSR, &CPU_ZPX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "CLI", &CPU_CLI, &CPU_IMP, 2 },{ "EOR", &CPU_EOR, &CPU_ABY, 4 },{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 7 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "EOR", &CPU_EOR, &CPU_ABX, 4 },{ "LSR", &CPU_LSR, &CPU_ABX, 7 },{ "???", &CPU_XXX, &CPU_IMP, 7 },
		{ "RTS", &CPU_RTS, &CPU_IMP, 6 },{ "ADC", &CPU_ADC, &CPU_IZX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 3 },{ "ADC", &CPU_ADC, &CPU_ZP0, 3 },{ "ROR", &CPU_ROR, &CPU_ZP0, 5 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "PLA", &CPU_PLA, &CPU_IMP, 4 },{ "ADC", &CPU_ADC, &CPU_IMM, 2 },{ "ROR", &CPU_ROR, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "JMP", &CPU_JMP, &CPU_IND, 5 },{ "ADC", &CPU_ADC, &CPU_ABS, 4 },{ "ROR", &CPU_ROR, &CPU_ABS, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },
		{ "BVS", &CPU_BVS, &CPU_REL, 2 },{ "ADC", &CPU_ADC, &CPU_IZY, 5 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "ADC", &CPU_ADC, &CPU_ZPX, 4 },{ "ROR", &CPU_ROR, &CPU_ZPX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "SEI", &CPU_SEI, &CPU_IMP, 2 },{ "ADC", &CPU_ADC, &CPU_ABY, 4 },{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 7 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "ADC", &CPU_ADC, &CPU_ABX, 4 },{ "ROR", &CPU_ROR, &CPU_ABX, 7 },{ "???", &CPU_XXX, &CPU_IMP, 7 },
		{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "STA", &CPU_STA, &CPU_IZX, 6 },{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "STY", &CPU_STY, &CPU_ZP0, 3 },{ "STA", &CPU_STA, &CPU_ZP0, 3 },{ "STX", &CPU_STX, &CPU_ZP0, 3 },{ "???", &CPU_XXX, &CPU_IMP, 3 },{ "DEY", &CPU_DEY, &CPU_IMP, 2 },{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "TXA", &CPU_TXA, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "STY", &CPU_STY, &CPU_ABS, 4 },{ "STA", &CPU_STA, &CPU_ABS, 4 },{ "STX", &CPU_STX, &CPU_ABS, 4 },{ "???", &CPU_XXX, &CPU_IMP, 4 },
		{ "BCC", &CPU_BCC, &CPU_REL, 2 },{ "STA", &CPU_STA, &CPU_IZY, 6 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "STY", &CPU_STY, &CPU_ZPX, 4 },{ "STA", &CPU_STA, &CPU_ZPX, 4 },{ "STX", &CPU_STX, &CPU_ZPY, 4 },{ "???", &CPU_XXX, &CPU_IMP, 4 },{ "TYA", &CPU_TYA, &CPU_IMP, 2 },{ "STA", &CPU_STA, &CPU_ABY, 5 },{ "TXS", &CPU_TXS, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "???", &CPU_NOP, &CPU_IMP, 5 },{ "STA", &CPU_STA, &CPU_ABX, 5 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "???", &CPU_XXX, &CPU_IMP, 5 },
		{ "LDY", &CPU_LDY, &CPU_IMM, 2 },{ "LDA", &CPU_LDA, &CPU_IZX, 6 },{ "LDX", &CPU_LDX, &CPU_IMM, 2 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "LDY", &CPU_LDY, &CPU_ZP0, 3 },{ "LDA", &CPU_LDA, &CPU_ZP0, 3 },{ "LDX", &CPU_LDX, &CPU_ZP0, 3 },{ "???", &CPU_XXX, &CPU_IMP, 3 },{ "TAY", &CPU_TAY, &CPU_IMP, 2 },{ "LDA", &CPU_LDA, &CPU_IMM, 2 },{ "TAX", &CPU_TAX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "LDY", &CPU_LDY, &CPU_ABS, 4 },{ "LDA", &CPU_LDA, &CPU_ABS, 4 },{ "LDX", &CPU_LDX, &CPU_ABS, 4 },{ "???", &CPU_XXX, &CPU_IMP, 4 },
		{ "BCS", &CPU_BCS, &CPU_REL, 2 },{ "LDA", &CPU_LDA, &CPU_IZY, 5 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "LDY", &CPU_LDY, &CPU_ZPX, 4 },{ "LDA", &CPU_LDA, &CPU_ZPX, 4 },{ "LDX", &CPU_LDX, &CPU_ZPY, 4 },{ "???", &CPU_XXX, &CPU_IMP, 4 },{ "CLV", &CPU_CLV, &CPU_IMP, 2 },{ "LDA", &CPU_LDA, &CPU_ABY, 4 },{ "TSX", &CPU_TSX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 4 },{ "LDY", &CPU_LDY, &CPU_ABX, 4 },{ "LDA", &CPU_LDA, &CPU_ABX, 4 },{ "LDX", &CPU_LDX, &CPU_ABY, 4 },{ "???", &CPU_XXX, &CPU_IMP, 4 },
		{ "CPY", &CPU_CPY, &CPU_IMM, 2 },{ "CMP", &CPU_CMP, &CPU_IZX, 6 },{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "CPY", &CPU_CPY, &CPU_ZP0, 3 },{ "CMP", &CPU_CMP, &CPU_ZP0, 3 },{ "DEC", &CPU_DEC, &CPU_ZP0, 5 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "INY", &CPU_INY, &CPU_IMP, 2 },{ "CMP", &CPU_CMP, &CPU_IMM, 2 },{ "DEX", &CPU_DEX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "CPY", &CPU_CPY, &CPU_ABS, 4 },{ "CMP", &CPU_CMP, &CPU_ABS, 4 },{ "DEC", &CPU_DEC, &CPU_ABS, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },
		{ "BNE", &CPU_BNE, &CPU_REL, 2 },{ "CMP", &CPU_CMP, &CPU_IZY, 5 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "CMP", &CPU_CMP, &CPU_ZPX, 4 },{ "DEC", &CPU_DEC, &CPU_ZPX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "CLD", &CPU_CLD, &CPU_IMP, 2 },{ "CMP", &CPU_CMP, &CPU_ABY, 4 },{ "NOP", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 7 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "CMP", &CPU_CMP, &CPU_ABX, 4 },{ "DEC", &CPU_DEC, &CPU_ABX, 7 },{ "???", &CPU_XXX, &CPU_IMP, 7 },
		{ "CPX", &CPU_CPX, &CPU_IMM, 2 },{ "SBC", &CPU_SBC, &CPU_IZX, 6 },{ "???", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "CPX", &CPU_CPX, &CPU_ZP0, 3 },{ "SBC", &CPU_SBC, &CPU_ZP0, 3 },{ "INC", &CPU_INC, &CPU_ZP0, 5 },{ "???", &CPU_XXX, &CPU_IMP, 5 },{ "INX", &CPU_INX, &CPU_IMP, 2 },{ "SBC", &CPU_SBC, &CPU_IMM, 2 },{ "NOP", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_SBC, &CPU_IMP, 2 },{ "CPX", &CPU_CPX, &CPU_ABS, 4 },{ "SBC", &CPU_SBC, &CPU_ABS, 4 },{ "INC", &CPU_INC, &CPU_ABS, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },
		{ "BEQ", &CPU_BEQ, &CPU_REL, 2 },{ "SBC", &CPU_SBC, &CPU_IZY, 5 },{ "???", &CPU_XXX, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 8 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "SBC", &CPU_SBC, &CPU_ZPX, 4 },{ "INC", &CPU_INC, &CPU_ZPX, 6 },{ "???", &CPU_XXX, &CPU_IMP, 6 },{ "SED", &CPU_SED, &CPU_IMP, 2 },{ "SBC", &CPU_SBC, &CPU_ABY, 4 },{ "NOP", &CPU_NOP, &CPU_IMP, 2 },{ "???", &CPU_XXX, &CPU_IMP, 7 },{ "???", &CPU_NOP, &CPU_IMP, 4 },{ "SBC", &CPU_SBC, &CPU_ABX, 4 },{ "INC", &CPU_INC, &CPU_ABX, 7 },{ "???", &CPU_XXX, &CPU_IMP, 7 },
};















































static uint8_t CPU_Read(struct CPU* cpu, uint16_t addr)
{
	return DBus_CPURead(cpu->bus, addr, 0);
}
static void CPU_Write(struct CPU* cpu, uint16_t addr, uint8_t data)
{
	DBus_CPUWrite(cpu->bus, addr, data);
}
static uint8_t CPU_Fetch(struct CPU* cpu)
{
	if (!(OPCODE_LIST[cpu->opcode].addrmode == CPU_IMP)) cpu->fetched = CPU_Read(cpu, cpu->addr_abs);
	return cpu->fetched;
}


struct CPU* CPU_Alloc()
{
	struct CPU* added = malloc(sizeof(struct CPU));
	if (!added) return NULL;
	memset(added, 0, sizeof(struct CPU));
	return added;
}
void CPU_Free(struct CPU** cpu)
{
	if (cpu && *cpu)
	{
		free(*cpu);
		*cpu = 0;
	}
}

void CPU_Reset(struct CPU* cpu)
{
	cpu->addr_abs = 0xFFFC;
	const uint16_t lo = CPU_Read(cpu, cpu->addr_abs + 0);
	const uint16_t hi = CPU_Read(cpu, cpu->addr_abs + 1);
	cpu->pc = (hi << 8) | lo;

	cpu->a = 0;
	cpu->x = 0;
	cpu->y = 0;
	cpu->sp = 0xFD;
	cpu->status = 0x00 | UF;

	cpu->addr_rel = 0x0000;
	cpu->addr_abs = 0x0000;
	cpu->fetched = 0x00;

	cpu->cycles = 8;
}
void CPU_IRQ(struct CPU* cpu)
{
	if (GetFlag(ID) == 0)
	{
		CPU_Write(cpu, 0x0100 + cpu->sp, (cpu->pc >> 8) & 0x00FF);
		cpu->sp--;
		CPU_Write(cpu, 0x0100 + cpu->sp, cpu->pc & 0x00FF);
		cpu->sp--;

		SetFlag(BC, 0);
		SetFlag(UF, 1);
		SetFlag(ID, 1);
		CPU_Write(cpu, 0x0100 + cpu->sp, cpu->status);
		cpu->sp--;

		cpu->addr_abs = 0xFFFE;
		const uint16_t lo = CPU_Read(cpu, cpu->addr_abs + 0);
		const uint16_t hi = CPU_Read(cpu, cpu->addr_abs + 1);
		cpu->pc = (hi << 8) | lo;

		cpu->cycles = 7;
	}
}
void CPU_NMI(struct CPU* cpu)
{
	CPU_Write(cpu, 0x0100 + cpu->sp, (cpu->pc >> 8) & 0x00FF);
	cpu->sp--;
	CPU_Write(cpu, 0x0100 + cpu->sp, cpu->pc & 0x00FF);
	cpu->sp--;

	SetFlag(BC, 0);
	SetFlag(UF, 1);
	SetFlag(ID, 1);
	CPU_Write(cpu, 0x0100 + cpu->sp, cpu->status);
	cpu->sp--;

	cpu->addr_abs = 0xFFFA;
	const uint16_t lo = CPU_Read(cpu, cpu->addr_abs + 0);
	const uint16_t hi = CPU_Read(cpu, cpu->addr_abs + 1);
	cpu->pc = (hi << 8) | lo;

	cpu->cycles = 8;
}
void CPU_Clock(struct CPU* cpu)
{
	if (cpu->cycles == 0)
	{
		cpu->opcode = CPU_Read(cpu, cpu->pc);

		SetFlag(UF, 1);

		cpu->pc++;

		cpu->cycles = OPCODE_LIST[cpu->opcode].cycles;
		uint8_t additional_cycle1 = OPCODE_LIST[cpu->opcode].addrmode(cpu);
		uint8_t additional_cycle2 = OPCODE_LIST[cpu->opcode].operate(cpu);
		cpu->cycles += (additional_cycle1 & additional_cycle2);

		SetFlag(UF, 1);
	}
	cpu->clock_count++;
	cpu->cycles--;
}
int CPU_IsComplete(struct CPU* cpu)
{
	return cpu->cycles == 0;
}


static uint8_t CPU_IMP(struct CPU* cpu)
{
	cpu->fetched = cpu->a;
	return 0;
}
static uint8_t CPU_IMM(struct CPU* cpu)
{
	cpu->addr_abs = cpu->pc;
	cpu->pc = cpu->pc + 1;
	return 0;
}
static uint8_t CPU_ZP0(struct CPU* cpu)
{
	cpu->addr_abs = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	cpu->addr_abs = (cpu->addr_abs & 0xFF);
	return 0;
}
static uint8_t CPU_ZPX(struct CPU* cpu)
{
	cpu->addr_abs = (CPU_Read(cpu, cpu->pc) + cpu->x);
	cpu->pc = cpu->pc + 1;
	cpu->addr_abs = (cpu->addr_abs & 0xFF);
	return 0;
}
static uint8_t CPU_ZPY(struct CPU* cpu)
{
	cpu->addr_abs = (CPU_Read(cpu, cpu->pc) + cpu->y);
	cpu->pc = cpu->pc + 1;
	cpu->addr_abs = (cpu->addr_abs & 0xFF);
	return 0;
}
static uint8_t CPU_REL(struct CPU* cpu)
{
	cpu->addr_rel = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	if (cpu->addr_rel & 0x80) cpu->addr_rel |= 0xFF00;
	return 0;
}
static uint8_t CPU_ABS(struct CPU* cpu)
{
	const uint16_t lo = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	const uint16_t hi = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	cpu->addr_abs = (hi << 8) | lo;
	return 0;
}
static uint8_t CPU_ABX(struct CPU* cpu)
{
	const uint16_t lo = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	const uint16_t hi = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	cpu->addr_abs = (hi << 8) | lo;
	cpu->addr_abs = cpu->addr_abs + cpu->x;
	if ((cpu->addr_abs & 0xFF00) != (hi << 8)) return 1;
	return 0;
}
static uint8_t CPU_ABY(struct CPU* cpu)
{
	const uint16_t lo = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	const uint16_t hi = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	cpu->addr_abs = (hi << 8) | lo;
	cpu->addr_abs = cpu->addr_abs + cpu->y;
	if ((cpu->addr_abs & 0xFF00) != (hi << 8)) return 1;
	return 0;
}
static uint8_t CPU_IND(struct CPU* cpu)
{
	const uint16_t lo = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	const uint16_t hi = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	const uint16_t ptr = (hi << 8) | lo;
	if (lo == 0x00FF) cpu->addr_abs = (CPU_Read(cpu, ptr & 0xFF00) << 8) | CPU_Read(cpu, ptr);
	else cpu->addr_abs = (CPU_Read(cpu, ptr + 1) << 8) | CPU_Read(cpu, ptr);
	return 0;
}
static uint8_t CPU_IZX(struct CPU* cpu)
{
	const uint16_t read = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	const uint16_t lo = CPU_Read(cpu, (read + (uint16_t)cpu->x) & 0x00FF);
	const uint16_t hi = CPU_Read(cpu, (uint16_t)(read + (uint16_t)cpu->x + 1) & 0x00FF);
	cpu->addr_abs = (hi << 8) | lo;
	return 0;
}
static uint8_t CPU_IZY(struct CPU* cpu)
{
	const uint16_t read = CPU_Read(cpu, cpu->pc);
	cpu->pc = cpu->pc + 1;
	const uint16_t lo = CPU_Read(cpu, read & 0x00FF);
	const uint16_t hi = CPU_Read(cpu, (uint16_t)(read + 1) & 0x00FF);
	cpu->addr_abs = ((hi << 8) | lo) + cpu->y;
	if ((cpu->addr_abs & 0xFF00) != (hi << 8)) return 1;
	return 0;
}
static uint8_t CPU_ADC(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = (uint16_t)cpu->a + (uint16_t)cpu->fetched + (uint16_t)GetFlag(CF);
	SetFlag(CF, cpu->temp > 255);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(VF, (~((uint16_t)cpu->a ^ (uint16_t)cpu->fetched) & ((uint16_t)cpu->a ^ (uint16_t)cpu->temp)) & 0x0080);
	SetFlag(NF, cpu->temp & 0x80);
	cpu->a = cpu->temp & 0x00FF;
	return 1;
}
static uint8_t CPU_AND(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->a = cpu->a & cpu->fetched;
	SetFlag(ZF, cpu->a == 0);
	SetFlag(NF, cpu->a & 0x80);
	return 1;
}
static uint8_t CPU_ASL(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = (uint16_t)cpu->fetched << 1;
	SetFlag(CF, (cpu->temp & 0xFF00) > 0);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->temp & 0x80);
	if (OPCODE_LIST[cpu->opcode].addrmode == CPU_IMP) cpu->a = cpu->temp & 0xFF;
	else CPU_Write(cpu, cpu->addr_abs, cpu->temp & 0xFF);
	return 0;
}
static uint8_t CPU_BCC(struct CPU* cpu)
{
	if (GetFlag(CF) == 0)
	{
		cpu->cycles = cpu->cycles + 1;
		cpu->addr_abs = cpu->pc + cpu->addr_rel;
		if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) cpu->cycles = cpu->cycles + 1;
		cpu->pc = cpu->addr_abs;
	}
	return 0;
}
static uint8_t CPU_BCS(struct CPU* cpu)
{
	if (GetFlag(CF) == 1)
	{
		cpu->cycles = cpu->cycles + 1;
		cpu->addr_abs = cpu->pc + cpu->addr_rel;
		if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) cpu->cycles = cpu->cycles + 1;
		cpu->pc = cpu->addr_abs;
	}
	return 0;
}
static uint8_t CPU_BEQ(struct CPU* cpu)
{
	if (GetFlag(ZF) == 1)
	{
		cpu->cycles = cpu->cycles + 1;
		cpu->addr_abs = cpu->pc + cpu->addr_rel;
		if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) cpu->cycles = cpu->cycles + 1;
		cpu->pc = cpu->addr_abs;
	}
	return 0;
}
static uint8_t CPU_BIT(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = cpu->a & cpu->fetched;
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->fetched & (1 << 7));
	SetFlag(VF, cpu->fetched & (1 << 6));
	return 0;
}
static uint8_t CPU_BMI(struct CPU* cpu)
{
	if (GetFlag(NF) == 1)
	{
		cpu->cycles = cpu->cycles + 1;
		cpu->addr_abs = cpu->pc + cpu->addr_rel;
		if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) cpu->cycles = cpu->cycles + 1;
		cpu->pc = cpu->addr_abs;
	}
	return 0;
}
static uint8_t CPU_BNE(struct CPU* cpu)
{
	if (GetFlag(ZF) == 0)
	{
		cpu->cycles = cpu->cycles + 1;
		cpu->addr_abs = cpu->pc + cpu->addr_rel;
		if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) cpu->cycles = cpu->cycles + 1;
		cpu->pc = cpu->addr_abs;
	}
	return 0;
}
static uint8_t CPU_BPL(struct CPU* cpu)
{
	if (GetFlag(NF) == 0)
	{
		cpu->cycles = cpu->cycles + 1;
		cpu->addr_abs = cpu->pc + cpu->addr_rel;
		if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) cpu->cycles = cpu->cycles + 1;
		cpu->pc = cpu->addr_abs;
	}
	return 0;
}
static uint8_t CPU_BRK(struct CPU* cpu)
{
	cpu->pc = cpu->pc + 1;
	SetFlag(ID, 1);
	CPU_Write(cpu, 0x100 + cpu->sp, (cpu->pc >> 8) & 0xFF);
	cpu->sp = cpu->sp - 1;
	CPU_Write(cpu, 0x100 + cpu->sp, cpu->pc & 0xFF);
	cpu->sp = cpu->sp - 1;
	SetFlag(BC, 1);
	CPU_Write(cpu, 0x100 + cpu->sp, cpu->status);
	cpu->sp = cpu->sp - 1;
	SetFlag(BC, 0);
	cpu->pc = (uint16_t)CPU_Read(cpu, 0xFFFE) | ((uint16_t)CPU_Read(cpu, 0xFFFF) << 8);
	return 0;
}
static uint8_t CPU_BVC(struct CPU* cpu)
{
	if (GetFlag(VF) == 0)
	{
		cpu->cycles = cpu->cycles + 1;
		cpu->addr_abs = cpu->pc + cpu->addr_rel;
		if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) cpu->cycles = cpu->cycles + 1;
		cpu->pc = cpu->addr_abs;
	}
	return 0;
}
static uint8_t CPU_BVS(struct CPU* cpu)
{
	if (GetFlag(VF) == 1)
	{
		cpu->cycles = cpu->cycles + 1;
		cpu->addr_abs = cpu->pc + cpu->addr_rel;
		if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) cpu->cycles = cpu->cycles + 1;
		cpu->pc = cpu->addr_abs;
	}
	return 0;
}
static uint8_t CPU_CLC(struct CPU* cpu)
{
	SetFlag(CF, 0);
	return 0;
}
static uint8_t CPU_CLD(struct CPU* cpu)
{
	SetFlag(DM, 0);
	return 0;
}
static uint8_t CPU_CLI(struct CPU* cpu)
{
	SetFlag(ID, 0);
	return 0;
}
static uint8_t CPU_CLV(struct CPU* cpu)
{
	SetFlag(VF, 0);
	return 0;
}
static uint8_t CPU_CMP(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = (uint16_t)cpu->a - (uint16_t)cpu->fetched;
	SetFlag(CF, cpu->a >= cpu->fetched);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->temp & 0x80);
	return 1;
}
static uint8_t CPU_CPX(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = (uint16_t)cpu->x - (uint16_t)cpu->fetched;
	SetFlag(CF, cpu->x >= cpu->fetched);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->temp & 0x80);
	return 0;
}
static uint8_t CPU_CPY(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = (uint16_t)cpu->y - (uint16_t)cpu->fetched;
	SetFlag(CF, cpu->y >= cpu->fetched);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->temp & 0x80);
	return 0;
}
static uint8_t CPU_DEC(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = cpu->fetched - 1;
	CPU_Write(cpu, cpu->addr_abs, cpu->temp & 0xFF);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, (cpu->temp & 0x80));
	return 0;
}
static uint8_t CPU_DEX(struct CPU* cpu)
{
	cpu->x = cpu->x - 1;
	SetFlag(ZF, cpu->x == 0);
	SetFlag(NF, cpu->x & 0x80);
	return 0;
}
static uint8_t CPU_DEY(struct CPU* cpu)
{
	cpu->y = cpu->y - 1;
	SetFlag(ZF, cpu->y == 0);
	SetFlag(NF, cpu->y & 0x80);
	return 0;
}
static uint8_t CPU_EOR(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->a = cpu->a ^ cpu->fetched;
	SetFlag(ZF, cpu->a == 0);
	SetFlag(NF, cpu->a & 0x80);
	return 1;
}
static uint8_t CPU_INC(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = cpu->fetched + 1;
	CPU_Write(cpu, cpu->addr_abs, cpu->temp & 0xFF);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->temp & 0x80);
	return 0;
}
static uint8_t CPU_INX(struct CPU* cpu)
{
	cpu->x = cpu->x + 1;
	SetFlag(ZF, cpu->x == 0);
	SetFlag(NF, cpu->x & 0x80);
	return 0;
}
static uint8_t CPU_INY(struct CPU* cpu)
{
	cpu->y = cpu->y + 1;
	SetFlag(ZF, cpu->y == 0);
	SetFlag(NF, cpu->y & 0x80);
	return 0;
}
static uint8_t CPU_JMP(struct CPU* cpu)
{
	cpu->pc = cpu->addr_abs;
	return 0;
}
static uint8_t CPU_JSR(struct CPU* cpu)
{
	cpu->pc = cpu->pc - 1;
	CPU_Write(cpu, 0x100 + cpu->sp, (cpu->pc >> 8) & 0xFF);
	cpu->sp = cpu->sp - 1;
	CPU_Write(cpu, 0x100 + cpu->sp, cpu->pc & 0xFF);
	cpu->sp = cpu->sp - 1;
	cpu->pc = cpu->addr_abs;
	return 0;
}
static uint8_t CPU_LDA(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->a = cpu->fetched;
	SetFlag(ZF, cpu->a == 0);
	SetFlag(NF, cpu->a & 0x80);
	return 1;
}
static uint8_t CPU_LDX(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->x = cpu->fetched;
	SetFlag(ZF, cpu->x == 0);
	SetFlag(NF, cpu->x & 0x80);
	return 1;
}
static uint8_t CPU_LDY(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->y = cpu->fetched;
	SetFlag(ZF, cpu->y == 0);
	SetFlag(NF, cpu->y & 0x80);
	return 1;
}
static uint8_t CPU_LSR(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	SetFlag(CF, cpu->fetched & 0x1);
	cpu->temp = cpu->fetched >> 1;
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->temp & 0x80);
	if (OPCODE_LIST[cpu->opcode].addrmode == CPU_IMP) cpu->a = cpu->temp & 0xFF;
	else CPU_Write(cpu, cpu->addr_abs, cpu->temp & 0xFF);
	return 0;
}
static uint8_t CPU_NOP(struct CPU* cpu)
{
	if (cpu->opcode == 0x1C || cpu->opcode == 0x3C || cpu->opcode == 0x5C || cpu->opcode == 0x7C || cpu->opcode == 0xDC || cpu->opcode == 0xFC) return 1;
	return 0;
}
static uint8_t CPU_ORA(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->a = cpu->a | cpu->fetched;
	SetFlag(ZF, cpu->a == 0);
	SetFlag(NF, cpu->a & 0x80);
	return 1;
}
static uint8_t CPU_PHA(struct CPU* cpu)
{
	CPU_Write(cpu, 0x100 + cpu->sp, cpu->a);
	cpu->sp = cpu->sp - 1;
	return 0;
}
static uint8_t CPU_PHP(struct CPU* cpu)
{
	CPU_Write(cpu, 0x100 + cpu->sp, cpu->status | BC | UF);
	SetFlag(BC, 0);
	SetFlag(UF, 0);
	cpu->sp = cpu->sp - 1;
	return 0;
}
static uint8_t CPU_PLA(struct CPU* cpu)
{
	cpu->sp = cpu->sp + 1;
	cpu->a = CPU_Read(cpu, 0x100 + cpu->sp);
	SetFlag(ZF, cpu->a == 0);
	SetFlag(NF, cpu->a & 0x80);
	return 0;
}
static uint8_t CPU_PLP(struct CPU* cpu)
{
	cpu->sp = cpu->sp + 1;
	cpu->status = CPU_Read(cpu, 0x100 + cpu->sp);
	SetFlag(UF, 1);
	return 0;
}
static uint8_t CPU_ROL(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = (uint16_t)(cpu->fetched << 1) | GetFlag(CF);
	SetFlag(CF, cpu->temp & 0xFF00);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->temp & 0x80);
	if (OPCODE_LIST[cpu->opcode].addrmode == CPU_IMP) cpu->a = cpu->temp & 0xFF;
	else CPU_Write(cpu, cpu->addr_abs, cpu->temp & 0xFF);
	return 0;
}
static uint8_t CPU_ROR(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	cpu->temp = (uint16_t)(GetFlag(CF) << 7) | (cpu->fetched >> 1);
	SetFlag(CF, cpu->fetched & 1);
	SetFlag(ZF, (cpu->temp & 0xFF) == 0);
	SetFlag(NF, cpu->temp & 0x80);
	if (OPCODE_LIST[cpu->opcode].addrmode == CPU_IMP) cpu->a = cpu->temp & 0xFF;
	else CPU_Write(cpu, cpu->addr_abs, cpu->temp & 0xFF);
	return 0;
}
static uint8_t CPU_RTI(struct CPU* cpu)
{
	cpu->sp = cpu->sp + 1;
	cpu->status = CPU_Read(cpu, 0x100 + cpu->sp);
	cpu->status = cpu->status & (~BC);
	cpu->status = cpu->status & (~UF);
	cpu->sp = cpu->sp + 1;
	cpu->pc = (uint16_t)CPU_Read(cpu, 0x100 + cpu->sp);
	cpu->sp = cpu->sp + 1;
	cpu->pc |= ((uint16_t)CPU_Read(cpu, 0x100 + cpu->sp)) << 8;
	return 0;
}
static uint8_t CPU_RTS(struct CPU* cpu)
{
	cpu->sp = cpu->sp + 1;
	cpu->pc = (uint16_t)CPU_Read(cpu, 0x100 + cpu->sp);
	cpu->sp = cpu->sp + 1;
	cpu->pc |= ((uint16_t)CPU_Read(cpu, 0x100 + cpu->sp)) << 8;
	cpu->pc = cpu->pc + 1;
	return 0;
}
static uint8_t CPU_SBC(struct CPU* cpu)
{
	CPU_Fetch(cpu);
	const uint16_t val = ((uint16_t)cpu->fetched) ^ 0x00FF;
	cpu->temp = (uint16_t)cpu->a + val + (uint16_t)GetFlag(CF);
	SetFlag(CF, cpu->temp & 0xFF00);
	SetFlag(ZF, ((cpu->temp & 0x00FF) == 0));
	SetFlag(VF, (cpu->temp ^ (uint16_t)cpu->a) & (cpu->temp ^ val) & 0x0080);
	SetFlag(NF, cpu->temp & 0x0080);
	cpu->a = cpu->temp & 0xFF;
	return 1;
}
static uint8_t CPU_SEC(struct CPU* cpu)
{
	SetFlag(CF, 1);
	return 0;
}
static uint8_t CPU_SED(struct CPU* cpu)
{
	SetFlag(DM, 1);
	return 0;
}
static uint8_t CPU_SEI(struct CPU* cpu)
{
	SetFlag(ID, 1);
	return 0;
}
static uint8_t CPU_STA(struct CPU* cpu)
{
	CPU_Write(cpu, cpu->addr_abs, cpu->a);
	return 0;
}
static uint8_t CPU_STX(struct CPU* cpu)
{
	CPU_Write(cpu, cpu->addr_abs, cpu->x);
	return 0;
}
static uint8_t CPU_STY(struct CPU* cpu)
{
	CPU_Write(cpu, cpu->addr_abs, cpu->y);
	return 0;
}
static uint8_t CPU_TAX(struct CPU* cpu)
{
	cpu->x = cpu->a;
	SetFlag(ZF, cpu->x == 0);
	SetFlag(NF, cpu->x & 0x80);
	return 0;
}
static uint8_t CPU_TAY(struct CPU* cpu)
{
	cpu->y = cpu->a;
	SetFlag(ZF, cpu->y == 0);
	SetFlag(NF, cpu->y & 0x80);
	return 0;
}
static uint8_t CPU_TSX(struct CPU* cpu)
{
	cpu->x = cpu->sp;
	SetFlag(ZF, cpu->x == 0);
	SetFlag(NF, cpu->x & 0x80);
	return 0;
}
static uint8_t CPU_TXA(struct CPU* cpu)
{
	cpu->a = cpu->x;
	SetFlag(ZF, cpu->a == 0);
	SetFlag(NF, cpu->a & 0x80);
	return 0;
}
static uint8_t CPU_TXS(struct CPU* cpu)
{
	cpu->sp = cpu->x;
	return 0;
}
static uint8_t CPU_TYA(struct CPU* cpu)
{
	cpu->a = cpu->y;
	SetFlag(ZF, cpu->a == 0);
	SetFlag(NF, cpu->a & 0x80);
	return 0;
}

static uint8_t CPU_XXX(struct CPU* cpu)
{
	return 0;
}