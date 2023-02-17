#include "apu.h"
#include "DataBus.h"
#include "cpu.h"

static const uint8_t length_table[] = {
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};
static const uint8_t duty_table[] = {
	0, 1, 0, 0, 0, 0, 0, 0,
	0, 1, 1, 0, 0, 0, 0, 0,
	0, 1, 1, 1, 1, 0, 0, 0,
	1, 0, 0, 1, 1, 1, 1, 1,
};
static const uint8_t triangle_table[] = {
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};
static const uint16_t noise_table[] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};
static const uint8_t dmc_table[] = {
	214, 190, 170, 160, 143, 127, 113, 107, 95, 80, 71, 64, 53, 42, 36, 27,
};

static const float pulse_table[] = { 
	0.0f, 0.01160913892f, 0.02293948084f, 0.03400094807f, 0.04480300099f, 0.05535465851f, 0.0656645298f, 0.07574082166f,
	0.08559139818f, 0.09522374719f, 0.1046450436f, 0.1138621494f, 0.1228816435f, 0.1317097992f, 0.1403526366f, 0.1488159597f, 0.1571052521f, 0.1652258784f,
	0.1731829196f, 0.1809812635f, 0.188625589f, 0.1961204559f, 0.2034701705f, 0.2106789351f, 0.2177507579f, 0.2246894985f, 0.2314988673f, 0.2381824702f,
	0.2447437793f, 0.2511860728f, 0.2575125694f, 0.2637263834f
};
static const float tnd_table[] = { 
	0.0f, 0.006699823774f, 0.01334501989f, 0.01993625611f, 0.0264741797f, 0.03295944259f, 0.0393926762f, 0.04577450082f,
	0.05210553482f, 0.05838638172f, 0.06461763382f, 0.07079987228f, 0.07693368942f, 0.08301962167f, 0.08905825764f, 0.09505013376f,
	0.1009957939f, 0.1068957672f, 0.1127505824f, 0.1185607538f, 0.1243267879f, 0.130049184f, 0.1357284486f, 0.1413650513f, 0.1469594985f,
	0.1525122225f, 0.1580237001f, 0.1634943932f, 0.1689247638f, 0.174315244f, 0.1796662807f, 0.1849783063f, 0.1902517378f, 0.1954869777f,
	0.2006844729f, 0.2058446258f, 0.210967809f, 0.2160544395f, 0.2211049199f, 0.2261195928f, 0.2310988754f, 0.2360431105f, 0.2409527153f,
	0.2458280027f, 0.2506693602f, 0.2554771006f, 0.2602516413f, 0.2649932802f, 0.2697023749f, 0.2743792236f, 0.2790241838f, 0.2836375833f,
	0.2882197201f, 0.292770952f, 0.2972915173f, 0.3017818034f, 0.3062421083f, 0.3106726706f, 0.3150738478f, 0.3194458783f, 0.3237891197f,
	0.3281037807f, 0.3323901892f, 0.3366486132f, 0.3408792913f, 0.3450825512f, 0.3492586315f, 0.3534077704f, 0.357530266f, 0.3616263568f,
	0.3656963408f, 0.3697403669f, 0.3737587631f, 0.3777517378f, 0.3817195594f, 0.3856624365f, 0.3895806372f, 0.3934743702f, 0.3973438442f,
	0.4011892974f, 0.4050109982f, 0.4088090658f, 0.412583828f, 0.4163354635f, 0.4200641513f, 0.4237701297f, 0.4274536073f, 0.431114763f,
	0.4347538352f, 0.4383709729f, 0.4419664443f, 0.4455403984f, 0.449093014f, 0.4526245296f, 0.4561350644f, 0.4596248865f, 0.4630941153f,
	0.4665429294f, 0.4699715674f, 0.4733801484f, 0.4767689407f, 0.4801379442f, 0.4834875166f, 0.4868176877f, 0.4901287258f, 0.4934206903f,
	0.4966938794f, 0.4999483228f, 0.5031842589f, 0.5064018369f, 0.5096011758f, 0.5127824545f, 0.5159458518f, 0.5190914273f, 0.5222194791f,
	0.5253300667f, 0.5284232497f, 0.5314993262f, 0.5345583558f, 0.5376005173f, 0.5406259298f, 0.5436347723f, 0.5466270447f, 0.549603045f,
	0.5525628328f, 0.5555064678f, 0.5584343076f, 0.5613462329f, 0.5642424822f, 0.5671232343f, 0.5699884892f, 0.5728384256f, 0.5756732225f,
	0.5784929395f, 0.5812976956f, 0.5840876102f, 0.5868628025f, 0.5896234512f, 0.5923695564f, 0.5951013565f, 0.5978189111f, 0.6005222797f,
	0.6032115817f, 0.6058869958f, 0.6085486412f, 0.6111965775f, 0.6138308048f, 0.6164515615f, 0.6190590262f, 0.6216531396f, 0.6242340207f,
	0.6268018484f, 0.6293566823f, 0.6318986416f, 0.6344277263f, 0.6369441748f, 0.6394480467f, 0.641939342f, 0.6444182396f, 0.6468848586f,
	0.6493391991f, 0.6517813802f, 0.6542115211f, 0.6566297412f, 0.6590360403f, 0.6614305973f, 0.6638134122f, 0.6661846638f, 0.6685443521f,
	0.6708925962f, 0.6732294559f, 0.6755550504f, 0.6778694391f, 0.6801727414f, 0.6824649572f, 0.6847462058f, 0.6870166063f, 0.6892762184f,
	0.6915250421f, 0.6937633157f, 0.6959909201f, 0.698208034f, 0.7004147768f, 0.7026110888f, 0.7047972083f, 0.7069730759f, 0.7091388106f,
	0.7112944722f, 0.7134401202f, 0.7155758739f, 0.7177017927f, 0.7198178768f, 0.7219242454f, 0.7240209579f, 0.7261080146f, 0.7281856537f,
	0.7302538157f, 0.7323125601f, 0.7343619466f, 0.7364020944f, 0.7384331226f, 0.7404549122f, 0.7424675822f
};


const double FRAME_CTR_FREQ = 1789773.0 / 240.0;
const double SAMPLE_RATE = 1789773.0 / 44100.0;

struct APU* NES_APU_Alloc(struct DataBus* bus, int bufsz)
{
	struct APU* apu = malloc(sizeof(struct APU));
	if (!apu) return NULL;
	memset(apu, 0, sizeof(struct APU));
	apu->bus = bus;
	apu->buf.data = malloc(sizeof(float) * bufsz);
	if (!apu->buf.data) {
		free(apu);
		return NULL;
	}
	memset(apu->buf.data, 0, sizeof(float) * bufsz);
	apu->buf.size = bufsz;
	return apu;
}
void NES_APU_Free(struct APU** apu)
{
	if (apu && *apu)
	{
		struct APU* a = *apu;
		if (a->buf.data) free(a->buf.data);
		a->buf.data = NULL;
		free(a);
		*apu = NULL;
	}
}

static void Pulse_Clock(struct Pulse* p)
{
	if(p->timer_val == 0)
	{
		p->timer_val = p->timer_period;
		p->duty_val = (p->duty_val + 1) & 7;
	}
	else
	{
		p->timer_val = p->timer_val - 1;
	}
}
static uint8_t Pulse_Output(struct Pulse* p)
{
	if (!p->is_enabled || p->length_val == 0 || duty_table[p->duty_mode * 8 + p->duty_val] == 0 || p->timer_period < 8 || p->timer_period > 0x7FF) return 0;
	else if (p->is_envelope_enabled) return p->envelope_vol;
	else return p->volume;
}
static void Pulse_Env_Clock(struct Pulse* p)
{
	if (p->is_envelope_start)
	{
		p->envelope_vol = 15;
		p->envelope_val = p->envelope_period;
		p->is_envelope_start = 0;
	}
	else if (p->envelope_val > 0) p->envelope_val = p->envelope_val - 1;
	else
	{
		if (p->envelope_vol > 0) p->envelope_vol = p->envelope_vol - 1;
		else if (p->is_envelope_loop) p->envelope_vol = 15;
		p->envelope_val = p->envelope_period;
	}
}
static void Pulse_Sweep_Clock(struct Pulse* p)
{
	if (p->is_sweep_reload)
	{
		if (p->is_sweep_enabled && p->sweep_val == 0)
		{
			const uint16_t delta = p->timer_period >> p->sweep_shift;
			if (p->is_sweep_negate)
			{
				p->timer_period = p->timer_period - delta;
				if (p->channel == 1) p->timer_period = p->timer_period - 1;
			}
			else
			{
				p->timer_period = p->timer_period + delta;
			}
		}
		p->sweep_val = p->sweep_period;
		p->is_sweep_reload = 0;
	}
	else if (p->sweep_val > 0) p->sweep_val = p->sweep_val - 1;
	else
	{
		if (p->is_sweep_enabled)
		{
			const uint16_t delta = p->timer_period >> p->sweep_shift;
			if (p->is_sweep_negate)
			{
				p->timer_period = p->timer_period - delta;
				if (p->channel == 1) p->timer_period = p->timer_period - 1;
			}
			else
			{
				p->timer_period = p->timer_period + delta;
			}
		}
		p->sweep_val = p->sweep_period;
	}
}
static void Env_Clock(struct APU* apu)
{
	Pulse_Env_Clock(&apu->pulse1);
	Pulse_Env_Clock(&apu->pulse2);
	struct Triangle* t = &apu->triangle;
	if (t->is_counter_reload) t->counter_val = t->counter_period;
	else if (t->counter_val > 0) t->counter_val = t->counter_val - 1;
	if (t->is_length_enabled) t->is_counter_reload = 0;
	struct Noise* n = &apu->noise;
	if (n->is_envelope_start)
	{
		n->envelope_vol = 15;
		n->envelope_val = n->envelope_period;
		n->is_envelope_start = 0;
	}
	else if (n->envelope_val > 0) n->envelope_val = n->envelope_val - 1;
	else
	{
		if (n->envelope_vol > 0) n->envelope_vol = n->envelope_vol - 1;
		else if (n->is_envelope_loop) n->envelope_vol = 15;
		n->envelope_val = n->envelope_period;
	}
}
static void Sweep_Clock(struct APU* apu)
{
	Pulse_Sweep_Clock(&apu->pulse1);
	Pulse_Sweep_Clock(&apu->pulse2);
}
static void Length_Clock(struct APU* apu)
{
	if (apu->pulse1.is_length_enabled && apu->pulse1.length_val > 0) apu->pulse1.length_val = apu->pulse1.length_val - 1;
	if (apu->pulse2.is_length_enabled && apu->pulse2.length_val > 0) apu->pulse2.length_val = apu->pulse2.length_val - 1;
	if (apu->triangle.is_length_enabled && apu->triangle.length_val > 0) apu->triangle.length_val = apu->triangle.length_val - 1;
	if (apu->noise.is_length_enabled && apu->noise.length_val > 0) apu->noise.length_val = apu->noise.length_val - 1;
}


void NES_APU_Clock(struct APU* apu)
{
	const uint64_t cycle1 = apu->cycle;
	apu->cycle = apu->cycle + 1;
	const uint64_t cycle2 = apu->cycle;

	if ((apu->cycle & 1) == 0) 
	{
		Pulse_Clock(&apu->pulse1);
		Pulse_Clock(&apu->pulse2);
		struct Noise* n = &apu->noise;
		if (n->timer_val == 0) 
		{
			n->timer_val = n->timer_period;
			uint8_t shift = n->is_mode ? 6 : 1;
			uint16_t b1 = n->shift_reg & 1;
			uint16_t b2 = (n->shift_reg >> shift) & 1;
			n->shift_reg = (n->shift_reg >> 1) | ((b1 ^ b2) << 14);
		}
		else 
		{
			n->timer_val = n->timer_val - 1;
		}
		struct DeltaModulationChannel* d = &apu->dmc;
		if (d->is_enabled)
		{
			if (d->cur_len > 0 && d->bit_count == 0)
			{
				d->shift_reg = NES_DBus_CPURead(apu->bus, d->cur_addr, 0);
				d->bit_count = 8;
				d->cur_addr = d->cur_addr + 1;
				if (d->cur_addr == 0) d->cur_addr = 0x8000;
				d->cur_len = d->cur_len - 1;
				if (d->cur_len == 0 && d->is_loop) {
					d->cur_addr = d->samp_addr;
					d->cur_len = d->samp_len;
				}
			}

			if (d->tick_val == 0)
			{
				d->tick_val = d->tick_period;
				if (d->bit_count != 0)
				{
					if ((d->shift_reg & 1) == 1)
					{
						if (d->value <= 125)
						{
							d->value = d->value + 2;
						}
					}
					else if (d->value >= 2)
					{
						d->value = d->value - 2;
					}
					d->shift_reg = d->shift_reg >> 1;

					d->bit_count = d->bit_count - 1;
				}
			}
			else
			{
				d->tick_val = d->tick_val - 1;
			}
		}
	}
	struct Triangle* t = &apu->triangle;
	if (t->timer_val == 0)
	{
		t->timer_val = t->timer_period;
		if (t->length_val > 0 && t->counter_val > 0) t->duty_val = (t->duty_val + 1) & 31;
	}
	else
	{
		t->timer_val = t->timer_val - 1;
	}
	
	const int f1 = (int)(((double)cycle1) / FRAME_CTR_FREQ);
	const int f2 = (int)(((double)cycle2) / FRAME_CTR_FREQ);
	if (f1 != f2)
	{
		const uint8_t fp = apu->frame_period;
		if (fp == 4)
		{
			apu->frame_val = (apu->frame_val + 1) & 3;
			switch (apu->frame_val)
			{
			case 0:
			case 2:
				Env_Clock(apu);
				break;
			case 1:
				Env_Clock(apu);
				Sweep_Clock(apu);
				Length_Clock(apu);
				break;
			case 3:
				Env_Clock(apu);
				Sweep_Clock(apu);
				Length_Clock(apu);
				if (apu->is_frame_irq) NES_CPU_IRQ(apu->bus->cpu);
				break;
			default:
				break;
			}
		}
		else if (fp == 5)
		{
			apu->frame_val = (apu->frame_val + 1) % 5;
			switch (apu->frame_val)
			{
			case 1:
			case 3:
				Env_Clock(apu);
				break;
			case 0:
			case 2:
				Env_Clock(apu);
				Sweep_Clock(apu);
				Length_Clock(apu);
				break;
			}
		}
	}
	const int s1 = (int)(((double)cycle1) / SAMPLE_RATE);
	const int s2 = (int)(((double)cycle2) / SAMPLE_RATE);

	if (s1 != s2)
	{
		const uint8_t p1_output = Pulse_Output(&apu->pulse1);
		const uint8_t p2_output = Pulse_Output(&apu->pulse2);
		const uint8_t tri_output = (!t->is_enabled || t->length_val == 0 || t->counter_val == 0) ? 0 : triangle_table[t->duty_val];
		struct Noise* n = &apu->noise;
		uint8_t noise_out;
		if (!n->is_enabled || n->length_val == 0 || (n->shift_reg & 1) == 1) noise_out = 0;
		else if (n->is_envelope_enabled) noise_out = n->envelope_vol;
		else noise_out = n->volume;
		const uint8_t dmc_out = apu->dmc.value;
		
		const float output = tnd_table[(3 * tri_output) + (2 * noise_out) + dmc_out] + pulse_table[p1_output + p2_output];
		
		apu->buf.data[apu->buf.cur_write] = output;
		apu->buf.cur_write = (apu->buf.cur_write + 1) % apu->buf.size;

	}
}


static void Pulse_WriteControl(struct Pulse* p, uint8_t val)
{
	p->duty_mode = (val >> 6) & 3;
	p->is_length_enabled = ((val >> 5) & 1) == 0;
	p->is_envelope_loop = ((val >> 5) & 1) == 1;
	p->is_envelope_enabled = ((val >> 4) & 1) == 0;
	p->envelope_period = val & 15;
	p->volume = val & 15;
	p->is_envelope_start = 1;
}
static void Pulse_WriteSweep(struct Pulse* p, uint8_t val)
{
	p->is_sweep_enabled = ((val >> 7) & 1) == 1;
	p->sweep_period = ((val >> 4) & 7) + 1;
	p->is_sweep_negate = ((val >> 3) & 1) == 1;
	p->sweep_shift = val & 7;
	p->is_sweep_reload = 1;
}
static void Pulse_WriteTimerHigh(struct Pulse* p, uint8_t val)
{
	p->length_val = length_table[val >> 3];
	p->timer_period = (p->timer_period & 0x00FF) | ((uint16_t)(val & 7) << 8);
	p->is_envelope_start = 1;
	p->duty_val = 0;
}

void NES_APU_CPUWrite(struct APU* apu, uint16_t addr, uint8_t val)
{
	switch (addr) {
	case 0x4000:
		Pulse_WriteControl(&apu->pulse1, val);
		break;
	case 0x4001:
		Pulse_WriteSweep(&apu->pulse1, val);
		break;
	case 0x4002:
		apu->pulse1.timer_period = (apu->pulse1.timer_period & 0xFF00) | (uint16_t)(val);
		break;
	case 0x4003:
		Pulse_WriteTimerHigh(&apu->pulse1, val);
		break;
	case 0x4004:
		Pulse_WriteControl(&apu->pulse2, val);
		break;
	case 0x4005:
		Pulse_WriteSweep(&apu->pulse2, val);
		break;
	case 0x4006:
		apu->pulse2.timer_period = (apu->pulse2.timer_period & 0xFF00) | (uint16_t)(val);
		break;
	case 0x4007:
		Pulse_WriteTimerHigh(&apu->pulse2, val);
		break;
	case 0x4008:
		apu->triangle.is_length_enabled = ((val >> 7) & 1) == 0;
		apu->triangle.counter_period = val & 0x7F;
		break;
	case 0x4009:
	case 0x4010:
		apu->dmc.is_irq = (val & 0x80) == 0x80;
		apu->dmc.is_loop = (val & 0x40) == 0x40;
		apu->dmc.tick_period = dmc_table[val & 0x0F];
		break;
	case 0x4011:
		apu->dmc.value = val & 0x7F;
		break;
	case 0x4012:
		apu->dmc.samp_addr = 0xC000 | ((uint16_t)(val) << 6);
		break;
	case 0x4013:
		apu->dmc.samp_len = ((uint16_t)(val) << 4) | 1;
		break;
	case 0x400A:
		apu->triangle.timer_period = (apu->triangle.timer_period & 0xFF00) | (uint16_t)(val);
		break;
	case 0x400B:
		apu->triangle.length_val = length_table[val >> 3];
		apu->triangle.timer_period = (apu->triangle.timer_period & 0x00FF) | ((uint16_t)(val & 7) << 8);
		apu->triangle.timer_val = apu->triangle.timer_period;
		apu->triangle.is_counter_reload = 1;
		break;
	case 0x400C:
		apu->noise.is_length_enabled = ((val >> 5) & 1) == 0;
		apu->noise.is_envelope_loop = ((val >> 5) & 1) == 1;
		apu->noise.is_envelope_enabled = ((val >> 4) & 1) == 0;
		apu->noise.envelope_period = val & 15;
		apu->noise.volume = val & 15;
		apu->noise.is_envelope_start = 1;
		break;
	case 0x400D:
	case 0x400E:
		apu->noise.is_mode = (val & 0x80) == 0x80;
		apu->noise.timer_period = noise_table[val & 0x0F];
		break;
	case 0x400F:
		apu->noise.length_val = length_table[val >> 3];
		apu->noise.is_envelope_start = 1;
		break;
	case 0x4015:
		apu->pulse1.is_enabled = (val & 1) == 1;
		apu->pulse2.is_enabled = (val & 2) == 2;
		apu->triangle.is_enabled = (val & 4) == 4;
		apu->noise.is_enabled = (val & 8) == 8;
		apu->dmc.is_enabled = (val & 16) == 16;
		if (!apu->pulse1.is_enabled) {
			apu->pulse1.length_val = 0;
		}
		if (!apu->pulse2.is_enabled) {
			apu->pulse2.length_val = 0;
		}
		if (!apu->triangle.is_enabled) {
			apu->triangle.length_val = 0;
		}
		if (!apu->noise.is_enabled) {
			apu->noise.length_val = 0;
		}
		if (!apu->dmc.is_enabled) {
			apu->dmc.cur_len = 0;
		}
		else {
			if (apu->dmc.cur_len == 0) {
				apu->dmc.cur_addr = apu->dmc.samp_addr;
				apu->dmc.cur_len = apu->dmc.samp_len;
			}
		}
		break;
	case 0x4017:
		apu->frame_period = 4 + ((val >> 7) & 1);
		apu->is_frame_irq = ((val >> 6) & 1) == 0;
		if (apu->frame_period == 5) {
			Env_Clock(apu);
			Sweep_Clock(apu);
			Length_Clock(apu);
		}
		break;
	}
}