#include "cpu.h"
#include "bus.h"

static uint32_t CPU_Read32(struct CPU* cpu, uint32_t addr)
{
	return PS1_BUS_CpuRead32(cpu->bus, addr);
}
static void CPU_Write32(struct CPU* cpu, uint32_t addr, uint32_t val)
{
	PS1_BUS_CpuWrite32(cpu->bus, addr, val);
}
static uint16_t CPU_Read16(struct CPU* cpu, uint32_t addr)
{
	return PS1_BUS_CpuRead16(cpu->bus, addr);
}
static void CPU_Write16(struct CPU* cpu, uint32_t addr, uint16_t val)
{
	PS1_BUS_CpuWrite16(cpu->bus, addr, val);
}
static uint8_t CPU_Read8(struct CPU* cpu, uint32_t addr)
{
	return PS1_BUS_CpuRead8(cpu->bus, addr);
}
static void CPU_Write8(struct CPU* cpu, uint32_t addr, uint8_t val)
{
	PS1_BUS_CpuWrite8(cpu->bus, addr, val);
}

static enum Exception
{
	EX_LOAD_ADDRESS_ERROR = 0x4,
	EX_STORE_ADDRESS_ERROR = 0x5,
	EX_SYSCALL = 0x8,
	EX_BREAK = 0x9,
	EX_ILLEGAL_INSTRUCTION = 0xA,
	EX_COPROCESSOR_ERROR = 0xB,
	EX_OVERFLOW = 0xC,
};

#define IMM(i) (i & 0xFFFF)
#define IMM_SE(i) ((uint32_t)(int16_t)(i & 0xFFFF))
#define IMM_SHIFT(i) ((i >> 6) & 0x1F)
#define IMM_JUMP(i) (i & 0x3FFFFFF)
#define TO(i) ((i >> 16) & 0x1F)
#define ST(i) ((i >> 21) & 0x1F)
#define DR(i) ((i >> 11) & 0x1F)
#define FUN(i) (i >> 26)
#define SUBFUN(i) (i & 0x3F)
#define COP_OP(i) ST(i)

#define WRITE_REG(r, val) cpu->out_regs[r] = val; cpu->out_regs[0] = 0
#define READ_REG(r) cpu->regs[r]

static void CPU_BRANCH(struct CPU* cpu, uint32_t offset)
{
	// offset immediates are always shifted two places to the right since PC 
	// is always aligned on 32 bits
	offset = offset << 2;
	cpu->next_pc = cpu->pc + offset;
	cpu->is_branch = 1;
}
static void CPU_Exception(struct CPU* cpu, enum Exception cause)
{
	uint32_t handler = 0x80000080;
	if ((cpu->sr & (1 << 22)) != 0) handler = 0xBFC00180;
	
	const uint32_t mode = cpu->sr & 0x3F;
	cpu->sr &= ~0x3F;
	cpu->sr |= (mode << 2) & 0x3F;
	cpu->cause = ((uint32_t)cause) << 2;
	cpu->epc = cpu->current_pc;
	if (cpu->is_delay_spot) {
		cpu->epc = cpu->epc - 4;
		cpu->cause |= (1 << 31);
	}
	cpu->pc = handler;
	cpu->next_pc = cpu->pc + 4;
}

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
	const uint32_t v = READ_REG(s) | imm;
	WRITE_REG(t, v);
}
static void CPU_STW(struct CPU* cpu, uint32_t instruction)
{
	if (cpu->sr & 0x10000) return;
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint32_t val = READ_REG(t);
	if ((addr % 4) == 0) CPU_Write32(cpu, addr, val);
	else CPU_Exception(cpu, EX_STORE_ADDRESS_ERROR);
}
static void CPU_SLL(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t i = IMM_SHIFT(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t d = DR(instruction);
	const uint32_t val = READ_REG(t) << i;
	WRITE_REG(d, val);
}
static void CPU_ADDIU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t i = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t val = READ_REG(s) + i;
	WRITE_REG(t, val);
}
static void CPU_JMP(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_JUMP(instruction);
	cpu->next_pc = (cpu->next_pc & 0xF0000000) | (imm << 2);
	cpu->is_branch = 1;
}
static void CPU_OR(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = READ_REG(s) | READ_REG(t);
	WRITE_REG(d, val);
}


static void COP0_MTC0(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t cpu_reg = TO(instruction);
	const uint32_t cop_reg = DR(instruction);
	const uint32_t val = READ_REG(cpu_reg);
	switch (cop_reg)
	{
	case 3: case 5: case 6: case 7: case 9: case 11: // breakpoint registers
		if (val != 0) ERR_MSG("writing non 0 in bp register %x %x\n", cop_reg, val);
		break;
	case 12:
		cpu->sr = val;
		break;
	case 13:
		cpu->cause &= ~0x300;
		cpu->cause |= (val & 0x300);
		break;
	default:
		ERR_MSG("Unhandled COP0 register %x\n", cop_reg);
		break;
	}
}
static void COP0_MFC0(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t cpu_r = TO(instruction);
	const uint32_t cop_r = DR(instruction);
	uint32_t val = 0;
	switch (cop_r)
	{
	case 12:
		val = cpu->sr;
		break;
	case 13:
		val = cpu->cause;
		break;
	case 14:
		val = cpu->epc;
		break;
	default:
		ERR_MSG("unhandled read from cop0 reg %x\n", cop_r);
		break;
	}

	cpu->load.reg = cpu_r;
	cpu->load.val = val;
}
static void COP0_RFE(struct CPU* cpu, uint32_t instruction)
{
	if ((instruction & 0x3F) != 0b010000) ERR_MSG("invalid_cop0 instruction");
	const uint32_t mode = cpu->sr & 0x3F;
	cpu->sr &= ~0x3F;
	cpu->sr |= mode >> 2;

}
static void CPU_COP0(struct CPU* cpu, uint32_t instruction)
{
	switch (COP_OP(instruction))
	{
	case 0b00000:
		COP0_MFC0(cpu, instruction);
		break;
	case 0b00100:
		COP0_MTC0(cpu, instruction);
		break;
	case 0b10000:
		COP0_RFE(cpu, instruction);
		break;
	default:
		ERR_MSG("Unhandled COP0 instruction: %x %x\n", instruction, COP_OP(instruction));
		break;
	}
}

static void CPU_BNE(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	if (READ_REG(s) != READ_REG(t)) CPU_BRANCH(cpu, imm);
}
static void CPU_ADDI(struct CPU* cpu, uint32_t instruction)
{
	const int32_t imm = (int32_t)IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const int32_t is = (int32_t)READ_REG(s);
	int32_t val = 0;
	if (i32_add_overflow(imm, is, &val)) { WRITE_REG(t, (uint32_t)val); }
	else CPU_Exception(cpu, EX_OVERFLOW);
}
static void CPU_LW(struct CPU* cpu, uint32_t instruction)
{
	if (cpu->sr & 0x10000) return;
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	if ((addr % 4) == 0) {
		const uint32_t val = CPU_Read32(cpu, addr);
		cpu->load.reg = t;
		cpu->load.val = val;
	}
	else CPU_Exception(cpu, EX_LOAD_ADDRESS_ERROR);
}
static void CPU_STLU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = READ_REG(s) < READ_REG(t);
	WRITE_REG(d, val);
}
static void CPU_ADDU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t d = DR(instruction);
	const uint32_t val = READ_REG(s) + READ_REG(t);
	WRITE_REG(d, val);
}
static void CPU_SH(struct CPU* cpu, uint32_t instruction)
{
	if ((cpu->sr & 0x10000) != 0) {
		LOG("ignore store while cache is isolated");
		return;
	}
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint32_t val = READ_REG(t);
	if ((addr % 2) == 0) CPU_Write16(cpu, addr, (uint16_t)val);
	else CPU_Exception(cpu, EX_STORE_ADDRESS_ERROR);
}
static void CPU_JAL(struct CPU* cpu, uint32_t instruction)
{
	WRITE_REG(31, cpu->next_pc); // ra = cpu->pc
	CPU_JMP(cpu, instruction);
}
static void CPU_ANDI(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t val = READ_REG(s) & imm;
	WRITE_REG(t, val);
}
static void CPU_SB(struct CPU* cpu, uint32_t instruction)
{
	if ((cpu->sr & 0x10000) != 0) {
		LOG("ignoring store while cache is isolated");
		return;
	}
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint32_t val = READ_REG(t);
	CPU_Write8(cpu, addr, (uint8_t)val);
}
static void CPU_JR(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	cpu->next_pc = READ_REG(s);
	cpu->is_branch = 1;
}
static void CPU_LB(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint8_t val = CPU_Read8(cpu, addr);
	cpu->load.reg = t;
	cpu->load.val = (uint32_t)val;
}
static void CPU_BEQ(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	
	if (READ_REG(s) == READ_REG(t)) CPU_BRANCH(cpu, imm);
}
static void CPU_AND(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = READ_REG(s) & READ_REG(t);
	WRITE_REG(d, val);
}
static void CPU_ADD(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t d = DR(instruction);
	const int32_t si = (int32_t)READ_REG(s);
	const int32_t ti = (int32_t)READ_REG(t);
	int32_t val = 0;
	if (i32_add_overflow(si, ti, &val)) { WRITE_REG(d, (uint32_t)val); }
	else CPU_Exception(cpu, EX_OVERFLOW); 
}
static void CPU_BGTZ(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t s = ST(instruction);
	const int32_t val = (int32_t)READ_REG(s);
	if (val > 0) CPU_BRANCH(cpu, imm);
}
static void CPU_BLEZ(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t s = ST(instruction);
	const int32_t val = (int32_t)READ_REG(s);
	if (val <= 0) CPU_BRANCH(cpu, imm);
}
static void CPU_LBU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint8_t val = CPU_Read8(cpu, addr);
	cpu->load.reg = t;
	cpu->load.val = (uint32_t)val;
}
static void CPU_JALR(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	WRITE_REG(d, cpu->next_pc);
	cpu->next_pc = READ_REG(s);
	cpu->is_branch = 1;
}
static void CPU_BXX(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t s = ST(instruction);
	
	const uint32_t is_bgez = (instruction >> 16) & 1;
	const uint32_t is_link = ((instruction >> 20) & 1) != 0;

	const int32_t val = (int32_t)READ_REG(s);
	uint32_t test = (val < 0);
	test = test ^ is_bgez;
	if (test) {
		if (is_link) {
			WRITE_REG(31, cpu->next_pc); // write pc to ra register
		}
		CPU_BRANCH(cpu, imm);
	}
}
static void CPU_SLTI(struct CPU* cpu, uint32_t instruction)
{
	const int32_t imm = (int32_t)IMM_SE(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = (((int32_t)READ_REG(s)) < imm);
	WRITE_REG(t, val);
}
static void CPU_SUBU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t d = DR(instruction);
	const uint32_t val = READ_REG(s) - READ_REG(t);
	WRITE_REG(d, val);
}
static void CPU_SRA(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SHIFT(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t d = DR(instruction);
	const int32_t val = ((int32_t)READ_REG(t)) >> imm;
	WRITE_REG(d, (uint32_t)val);
}
static void CPU_DIV(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const int32_t n = (int32_t)READ_REG(s);
	const int32_t d = (int32_t)READ_REG(t);

	if (d == 0) {
		cpu->hi = (uint32_t)n;
		if (n >= 0) cpu->lo = 0xFFFFFFFF;
		else cpu->lo = 1;
	}
	else if (((uint32_t)n) == 0x80000000 && d == -1) {
		cpu->hi = 0;
		cpu->lo = 0x80000000;
	}
	else {
		cpu->hi = (uint32_t)(n % d);
		cpu->lo = (uint32_t)(n / d);
	}
}
static void CPU_MFLO(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	WRITE_REG(d, cpu->lo);
}
static void CPU_SRL(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SHIFT(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t d = DR(instruction);
	const uint32_t val = READ_REG(t) >> imm;
	WRITE_REG(d, val);
}
static void CPU_SLTIU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = READ_REG(s) < imm;
	WRITE_REG(t, val);
}
static void CPU_DIVU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t n = READ_REG(s);
	const uint32_t d = READ_REG(t);
	if (d == 0) {
		cpu->hi = n;
		cpu->lo = 0xFFFFFFFF;
	}
	else {
		cpu->hi = n % d;
		cpu->lo = n / d;
	}
}
static void CPU_MFHI(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	WRITE_REG(d, cpu->hi);
}
static void CPU_SLT(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const int32_t si = (int32_t)READ_REG(s);
	const int32_t ti = (int32_t)READ_REG(t);
	const uint32_t val = si < ti;
	WRITE_REG(d, val);
}
static void CPU_SYSCALL(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_SYSCALL);
}
static void CPU_MTLO(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	cpu->lo = READ_REG(s);
}
static void CPU_MTHI(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	cpu->hi = READ_REG(s);
}
static void CPU_LHU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	if ((addr % 2) == 0) {
		const uint16_t val = CPU_Read16(cpu, addr);
		cpu->load.reg = t;
		cpu->load.val = (uint32_t)val;
	}
	else CPU_Exception(cpu, EX_LOAD_ADDRESS_ERROR);
}
static void CPU_SLLV(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);

	const uint32_t val = READ_REG(t) << (READ_REG(s) & 0x1F);
	WRITE_REG(d, val);
}
static void CPU_LH(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const int16_t val = (int16_t)CPU_Read16(cpu, addr);
	cpu->load.reg = t;
	cpu->load.val = (uint32_t)val;
}
static void CPU_NOR(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = !(READ_REG(s) | READ_REG(t));
	WRITE_REG(d, val);
}
static void CPU_SRAV(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = (uint32_t)(((int32_t)READ_REG(t)) >> (READ_REG(s) & 0x1F));
	WRITE_REG(d, val);
}
static void CPU_SRLV(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = READ_REG(t) >> (READ_REG(s) & 0x1F);
	WRITE_REG(d, val);
}
static void CPU_MULTU(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint64_t v1 = (uint64_t)READ_REG(s);
	const uint64_t v2 = (uint64_t)READ_REG(t);
	const uint64_t val = v1 * v2;
	cpu->hi = (uint32_t)(val >> 32);
	cpu->lo = (uint32_t)val;
}
static void CPU_XOR(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t d = DR(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t val = READ_REG(s) ^ READ_REG(t);
	WRITE_REG(d, val);
}
static void CPU_BREAK(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_BREAK);
}
static void CPU_MULT(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const int64_t v1 = (int64_t)((int32_t)READ_REG(s));
	const int64_t v2 = (int64_t)((int32_t)READ_REG(t));
	const uint64_t val = (uint64_t)(v1 * v2);
	cpu->hi = (uint32_t)(val >> 32);
	cpu->lo = (uint32_t)val;
}
static void CPU_SUB(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t s = ST(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t d = DR(instruction);
	const int32_t si = (int32_t)READ_REG(s);
	const int32_t ti = (int32_t)READ_REG(t);
	int32_t val = 0;
	if (i32_add_overflow(si, -ti, &val))
	{
		WRITE_REG(d, (uint32_t)val);
	}
	else
	{
		CPU_Exception(cpu, EX_OVERFLOW);
	}
}
static void CPU_XORI(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t val = READ_REG(s) ^ imm;
	WRITE_REG(t, val);
}
static void CPU_LWL(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint32_t cur_val = cpu->out_regs[t];
	const uint32_t aligned_addr = addr & ~3;
	const uint32_t aligned_word = CPU_Read32(cpu, aligned_addr);
	uint32_t val = 0;
	switch (addr & 3)
	{
	case 0:
		val = (cur_val & 0x00FFFFFF) | (aligned_word << 24);
		break;
	case 1:
		val = (cur_val & 0x0000FFFF) | (aligned_word << 16);
		break;
	case 2:
		val = (cur_val & 0x000000FF) | (aligned_word << 8);
		break;
	case 3:
		val = aligned_word;
		break;
	default:
		break;
	}
	cpu->load.reg = t;
	cpu->load.val = val;
}
static void CPU_LWR(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint32_t cur_val = cpu->out_regs[t];
	const uint32_t aligned_addr = addr & ~3;
	const uint32_t aligned_word = CPU_Read32(cpu, aligned_addr);
	uint32_t val = 0;
	switch (addr & 3)
	{
	case 0:
		val = aligned_word;
		break;
	case 1:
		val = (cur_val & 0xFF000000) | (aligned_word >> 8);
		break;
	case 2:
		val = (cur_val & 0xFFFF0000) | (aligned_word >> 16);
		break;
	case 3:
		val = (cur_val & 0xFFFFFF00) | (aligned_word >> 24);
		break;
	default:
		break;
	}
	cpu->load.reg = t;
	cpu->load.val = val;
}
static void CPU_SWL(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint32_t val = READ_REG(t);
	const uint32_t aligned_addr = addr & ~3;
	const uint32_t cur = CPU_Read32(cpu, aligned_addr);
	uint32_t write_val = 0;
	switch (addr & 3)
	{
	case 0:
		write_val = (cur & 0xFFFFFF00) | (val >> 24);
		break;
	case 1:
		write_val = (cur & 0xFFFF0000) | (val >> 16);
		break;
	case 2:
		write_val = (cur & 0xFF000000) | (val >> 8);
		break;
	case 3:
		write_val = val;
		break;
	default:
		break;
	}
	CPU_Write32(cpu, addr, write_val);
}
static void CPU_SWR(struct CPU* cpu, uint32_t instruction)
{
	const uint32_t imm = IMM_SE(instruction);
	const uint32_t t = TO(instruction);
	const uint32_t s = ST(instruction);
	const uint32_t addr = READ_REG(s) + imm;
	const uint32_t val = READ_REG(t);
	const uint32_t aligned_addr = addr & ~3;
	const uint32_t cur = CPU_Read32(cpu, aligned_addr);
	uint32_t write_val = 0;
	switch (addr & 3)
	{
	case 0:
		write_val = val;
		break;
	case 1:
		write_val = (cur & 0x000000FF) | (val << 8);
		break;
	case 2:
		write_val = (cur & 0x0000FFFF) | (val << 16);
		break;
	case 3:
		write_val = (cur & 0x00FFFFFF) | (val << 24);
		break;
	default:
		break;
	}
	CPU_Write32(cpu, addr, write_val);
}
static void CPU_COP1(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_COPROCESSOR_ERROR);
}
static void CPU_COP3(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_COPROCESSOR_ERROR);
}
static void CPU_LWC0(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_COPROCESSOR_ERROR);
}
static void CPU_LWC1(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_COPROCESSOR_ERROR);
}
static void CPU_LWC3(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_COPROCESSOR_ERROR);
}
static void CPU_SWC0(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_COPROCESSOR_ERROR);
}
static void CPU_SWC1(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_COPROCESSOR_ERROR);
}
static void CPU_SWC3(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_COPROCESSOR_ERROR);
}
static void CPU_ILLEGAL(struct CPU* cpu, uint32_t instruction)
{
	CPU_Exception(cpu, EX_ILLEGAL_INSTRUCTION);
}
static void CPU_COP2(struct CPU* cpu, uint32_t instruction)
{
	ERR_MSG("unhandled GTE instruction: %x\n", instruction);
}
static void CPU_LWC2(struct CPU* cpu, uint32_t instruction)
{
	ERR_MSG("unhandled GTE read instruction: %x", instruction);
}
static void CPU_SWC2(struct CPU* cpu, uint32_t instruction)
{
	ERR_MSG("unhandled GTE write instruction: %x", instruction);
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
		case 0b000010:
			CPU_SRL(cpu, instruction);
			break;
		case 0b000011:
			CPU_SRA(cpu, instruction);
			break;
		case 0b000100:
			CPU_SLLV(cpu, instruction);
			break;
		case 0b000110:
			CPU_SRLV(cpu, instruction);
			break;
		case 0b000111:
			CPU_SRAV(cpu, instruction);
			break;
		case 0b001000:
			CPU_JR(cpu, instruction);
			break;
		case 0b001001:
			CPU_JALR(cpu, instruction);
			break;
		case 0b001100:
			CPU_SYSCALL(cpu, instruction);
			break;
		case 0b001101:
			CPU_BREAK(cpu, instruction);
			break;
		case 0b010000:
			CPU_MFHI(cpu, instruction);
			break;
		case 0b010001:
			CPU_MTHI(cpu, instruction);
			break;
		case 0b010010:
			CPU_MFLO(cpu, instruction);
			break;
		case 0b010011:
			CPU_MTLO(cpu, instruction);
			break;
		case 0b011000:
			CPU_MULT(cpu, instruction);
			break;
		case 0b011001: 
			CPU_MULTU(cpu, instruction);
			break;
		case 0b011010:
			CPU_DIV(cpu, instruction);
			break;
		case 0b011011:
			CPU_DIVU(cpu, instruction);
			break;
		case 0b100000:
			CPU_ADD(cpu, instruction);
			break;
		case 0b100001:
			CPU_ADDU(cpu, instruction);
			break;
		case 0b100010:
			CPU_SUB(cpu, instruction);
			break;
		case 0b100011:
			CPU_SUBU(cpu, instruction);
			break;
		case 0b100100:
			CPU_AND(cpu, instruction);
			break;
		case 0b100101:
			CPU_OR(cpu, instruction);
			break;
		case 0b100110:
			CPU_XOR(cpu, instruction);
			break;
		case 0b100111:
			CPU_NOR(cpu, instruction);
			break;
		case 0b101010:
			CPU_SLT(cpu, instruction);
			break;
		case 0b101011:
			CPU_STLU(cpu, instruction);
			break;
		default:
			CPU_ILLEGAL(cpu, instruction);
			break;
		}
		break;
	case 0b000001:
		CPU_BXX(cpu, instruction);
		break;
	case 0b000010:
		CPU_JMP(cpu, instruction);
		break;
	case 0b000011:
		CPU_JAL(cpu, instruction);
		break;
	case 0b000100:
		CPU_BEQ(cpu, instruction);
		break;
	case 0b000101:
		CPU_BNE(cpu, instruction);
		break;
	case 0b000110:
		CPU_BLEZ(cpu, instruction);
		break;
	case 0b000111:
		CPU_BGTZ(cpu, instruction);
		break;
	case 0b001000:
		CPU_ADDI(cpu, instruction);
		break;
	case 0b001001:
		CPU_ADDIU(cpu, instruction);
		break;
	case 0b001010:
		CPU_SLTI(cpu, instruction);
		break;
	case 0b001011:
		CPU_SLTIU(cpu, instruction);
		break;
	case 0b001100:
		CPU_ANDI(cpu, instruction);
		break;
	case 0b001101:
		CPU_ORI(cpu, instruction);
		break;
	case 0b001110:
		CPU_XORI(cpu, instruction);
		break;
	case 0b001111:
		CPU_LUI(cpu, instruction);
		break;
	case 0b010000:
		CPU_COP0(cpu, instruction);
		break;
	case 0b010001:
		CPU_COP1(cpu, instruction);
		break;
	case 0b010010:
		CPU_COP2(cpu, instruction);
		break;
	case 0b010011:
		CPU_COP3(cpu, instruction);
		break;
	case 0b100000:
		CPU_LB(cpu, instruction);
		break;
	case 0b100001:
		CPU_LH(cpu, instruction);
		break;
	case 0b100010:
		CPU_LWL(cpu, instruction);
		break;
	case 0b100011:
		CPU_LW(cpu, instruction);
		break;
	case 0b100100:
		CPU_LBU(cpu, instruction);
		break;
	case 0b100101:
		CPU_LHU(cpu, instruction);
		break;
	case 0b100110:
		CPU_LWR(cpu, instruction);
		break;
	case 0b101000:
		CPU_SB(cpu, instruction);
		break;
	case 0b101001:
		CPU_SH(cpu, instruction);
		break;
	case 0b101010:
		CPU_SWL(cpu, instruction);
		break;
	case 0b101011:
		CPU_STW(cpu, instruction);
		break;
	case 0b101110:
		CPU_SWR(cpu, instruction);
		break;
	case 0b110000:
		CPU_LWC0(cpu, instruction);
		break;
	case 0b110001:
		CPU_LWC1(cpu, instruction);
		break;
	case 0b110010:
		CPU_LWC2(cpu, instruction);
		break;
	case 0b110011:
		CPU_LWC3(cpu, instruction);
		break;
	case 0b111000:
		CPU_SWC0(cpu, instruction);
		break;
	case 0b111001:
		CPU_SWC1(cpu, instruction);
		break;
	case 0b111010:
		CPU_SWC2(cpu, instruction);
		break;
	case 0b111011:
		CPU_SWC3(cpu, instruction);
		break;
	default:
		CPU_ILLEGAL(cpu, instruction);
		break;
	}
}


struct CPU* PS1_CPU_Alloc(struct Bus* bus)
{
	struct CPU* out = (struct CPU*)malloc(sizeof(struct CPU));
	if (!out) return 0;
	memset(out, 0, sizeof(struct CPU));
	out->bus = bus;
	PS1_CPU_Reset(out);
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

void PS1_CPU_Reset(struct CPU* cpu)
{
	cpu->load.reg = 0; cpu->load.val = 0;
	cpu->regs[0] = 0; // should always be 0 but just in case reset to 0
	cpu->out_regs[0] = 0; // should always be 0 but just in case reset to 0
	cpu->pc = 0xbfc00000;
	cpu->next_pc = cpu->pc + 4;
	cpu->sr = 0;
	cpu->is_branch = 0;
	cpu->is_delay_spot = 0;
}

void PS1_CPU_Clock(struct CPU* cpu)
{
	cpu->current_pc = cpu->pc;
	if ((cpu->current_pc % 4) != 0) {
		CPU_Exception(cpu, EX_LOAD_ADDRESS_ERROR);
		return;
	}
	const uint32_t instruction = CPU_Read32(cpu, cpu->pc);
	
	cpu->is_delay_spot = cpu->is_branch;
	cpu->is_branch = 0;

	cpu->pc = cpu->next_pc;
	cpu->next_pc = cpu->next_pc + 4;
	WRITE_REG(cpu->load.reg, cpu->load.val);
	cpu->load.reg = 0; cpu->load.val = 0;
	
	CPU_DecodeAndExecute(cpu, instruction);
	
	memcpy(cpu->regs, cpu->out_regs, sizeof(cpu->regs));
}
