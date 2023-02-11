#include "ps1_collection.h"
#include "cpu.h"
#include "bus.h"
#include "bios.h"

struct PS1* PS1_Alloc(struct AppData* app)
{
	struct PS1* out = (struct PS1*)malloc(sizeof(struct PS1));
	if (!out) return 0;
	memset(out, 0, sizeof(struct PS1));
	out->bios = PS1_BIOS_Alloc();
	out->bus = PS1_BUS_Alloc(out->bios);
	out->cpu = PS1_CPU_Alloc(out->bus);
	return out;
}
void PS1_Free(struct PS1** ps1)
{
	if (ps1 && *ps1)
	{
		struct PS1* p = *ps1;
		free(p);
		*ps1 = 0;
	}
}

uint8_t PS1_Tick(struct AppData* app, struct PS1* ps1, float dt)
{
	PS1_CPU_Clock(ps1->cpu);
	return 1;
}