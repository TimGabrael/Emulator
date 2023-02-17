#include "DataBus.h"
#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"
#include "apu.h"

void NES_DBus_Init(struct DataBus* bus, struct PPU* ppu, struct CPU* cpu)
{
	memset(bus, 0, sizeof(struct DataBus));

	bus->cpu = cpu;
	bus->ppu = ppu;
	bus->apu = NES_APU_Alloc(bus, 100000);
	bus->is_dma_dummy = 1;
	cpu->bus = bus;
}
void NES_DBus_Uninit(struct DataBus* bus)
{
	if (bus)
	{
		if (bus->apu) NES_APU_Free(&bus->apu);
		bus->cpu->bus = 0;
	}
}

uint8_t NES_DBus_CPURead(struct DataBus* bus, uint16_t addr, uint8_t isReadOnly)
{
	uint8_t data = 0x00;

	if (bus->cart && NES_Cart_CpuRead(bus->cart, addr, &data))
	{

	}
	else if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		data = bus->cpu_ram[addr & 0x07FF];
	}
	else if (addr >= 0x2000 && addr <= 0x3FFF)
	{
		data = NES_PPU_CPURead(bus->ppu, addr & 0x0007, isReadOnly);
	}
	else if (addr >= 0x4016 && addr <= 0x4017)
	{
		data = (bus->controller_state[addr & 0x0001] & 0x80) > 0;
		bus->controller_state[addr & 0x0001] <<= 1;
	}
	else
	{
		//ERR_MSG("read from invalid address\n");
	}

	return data;
}

void NES_DBus_CPUWrite(struct DataBus* bus, uint16_t addr, uint8_t data)
{
	if (bus->cart && NES_Cart_CpuWrite(bus->cart, addr, data))
	{

	}
	else if (addr >= 0x0000 && addr < 0x2000)
	{
		bus->cpu_ram[addr & 0x07FF] = data;
	}
	else if (addr <= 0x3FFF)
	{
		NES_PPU_CPUWrite(bus->ppu, addr & 0x0007, data);
	}
	else if (addr < 0x4014)
	{
		NES_APU_CPUWrite(bus->apu, addr, data);
	}
	else if (addr == 0x4014)
	{
		bus->dma_page = data;
		bus->dma_addr = 0x00;
		bus->is_dma_transfer = 1;
	}
	else if (addr == 0x4015)
	{
		NES_APU_CPUWrite(bus->apu, addr, data);
	}
	else if (addr >= 0x4016 && addr <= 0x4017)
	{
		bus->controller_state[addr & 0x0001] = bus->controller[addr & 0x0001];
	}
	else if (addr == 0x4017)
	{
		NES_APU_CPUWrite(bus->apu, addr, data);
	}
	else
	{
		//ERR_MSG("write to invalid address\n");
	}
}

void NES_DBus_InsertCartridge(struct DataBus* bus, struct Cartridge* cart)
{
	bus->cart = cart;
	bus->ppu->cart = cart;
}
void NES_DBus_Reset(struct DataBus* bus)
{
	NES_CPU_Reset(bus->cpu);
	NES_PPU_Reset(bus->cpu);
	NES_APU_Reset(bus->apu);
	bus->clock_counter = 0;
	bus->dma_page = 0;
	bus->dma_addr = 0;
	bus->dma_data = 0;
	bus->is_dma_dummy = 1;
	bus->is_dma_transfer = 0;
}
void NES_DBus_Clock(struct DataBus* bus)
{
	NES_PPU_Clock(bus->ppu);
	if (bus->clock_counter % 3 == 0)
	{
		NES_APU_Clock(bus->apu);
		if (bus->is_dma_transfer)
		{
			if (bus->is_dma_dummy)
			{
				if ((bus->clock_counter % 2) == 1) bus->is_dma_dummy = 0;
			}
			else
			{
				if ((bus->clock_counter % 2) == 0) bus->dma_data = NES_DBus_CPURead(bus, (bus->dma_page << 8) | bus->dma_addr, 0);
				else {
					bus->ppu->pOAM[bus->dma_addr] = bus->dma_data;
					bus->dma_addr = bus->dma_addr + 1;
					if (bus->dma_addr == 0)
					{
						bus->is_dma_transfer = 0;
						bus->is_dma_dummy = 1;
					}
				}
			}
		}
		else
		{
			NES_CPU_Clock(bus->cpu);
		}
	}

	if (bus->ppu->is_nmi)
	{
		bus->ppu->is_nmi = 0;
		NES_CPU_NMI(bus->cpu);
	}
	bus->clock_counter = bus->clock_counter + 1;
}