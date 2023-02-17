#pragma once
#include "../Helper.h"

struct Pulse
{
	uint16_t timer_period;
	uint16_t timer_val;
	uint8_t channel;
	uint8_t length_val;
	uint8_t duty_mode;
	uint8_t duty_val;
	uint8_t sweep_shift;
	uint8_t sweep_period;
	uint8_t sweep_val;
	uint8_t envelope_period;
	uint8_t envelope_val;
	uint8_t envelope_vol;
	uint8_t volume;
	uint8_t is_envelope_enabled;
	uint8_t is_envelope_loop;
	uint8_t is_envelope_start;
	uint8_t is_sweep_reload;
	uint8_t is_sweep_enabled;
	uint8_t is_sweep_negate;
	uint8_t is_length_enabled;
	uint8_t is_enabled;
};

struct Triangle 
{
	uint16_t timer_period;
	uint16_t timer_val;
	uint8_t length_val;
	uint8_t duty_val;
	uint8_t counter_period;
	uint8_t counter_val;
	uint8_t is_length_enabled;
	uint8_t is_counter_reload;
	uint8_t is_enabled;
};

struct Noise
{
	uint16_t shift_reg;
	uint16_t timer_period;
	uint16_t timer_val;
	uint8_t envelope_period;
	uint8_t envelope_val;
	uint8_t envelope_vol;
	uint8_t volume;
	uint8_t length_val;
	uint8_t is_mode;
	uint8_t is_length_enabled;
	uint8_t is_envelope_enabled;
	uint8_t is_envelope_start;
	uint8_t is_envelope_loop;
	uint8_t is_enabled;
};

struct DeltaModulationChannel
{
	uint16_t samp_addr;
	uint16_t samp_len;
	uint16_t cur_addr;
	uint16_t cur_len;
	uint8_t value;
	uint8_t shift_reg;
	uint8_t bit_count;
	uint8_t tick_period;
	uint8_t tick_val;
	uint8_t is_loop;
	uint8_t is_irq;
	uint8_t is_enabled;
};

struct NES_APU_Circbuf
{
	float* data;
	int cur_write;
	int cur_read;
	int size;
};

struct APU
{
	struct NES_APU_Circbuf buf;
	struct DataBus* bus;
	struct Pulse pulse1;
	struct Pulse pulse2;
	struct Triangle triangle;
	struct Noise noise;
	struct DeltaModulationChannel dmc;
	uint64_t cycle;
	uint8_t frame_period;
	uint8_t frame_val;
	uint8_t is_frame_irq;
};

struct APU* NES_APU_Alloc(struct DataBus* bus, int bufsize);
void NES_APU_Free(struct APU** apu);
void NES_APU_Clock(struct APU* apu);


void NES_APU_CPUWrite(struct APU* apu, uint16_t addr, uint8_t val);
void NES_APU_Reset(struct APU* apu);