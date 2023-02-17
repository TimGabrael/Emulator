#include "../AppData.h"
#include "nes_collection.h"
#include "cartridge.h"
#include "cpu.h"
#include "DataBus.h"
#include "mapper.h"
#include "ppu.h"
#include "apu.h"

struct NES* NES_Alloc(struct AppData* app)
{
	struct NES* out = (struct NES*)malloc(sizeof(struct NES));
	if (!out) return 0;
	memset(out, 0, sizeof(struct NES));

	out->cpu = NES_CPU_Alloc();
	out->ppu = NES_PPU_Alloc();
	out->bus = (struct DataBus*)malloc(sizeof(struct DataBus));
	if (!out->bus) return 0;
	NES_DBus_Init(out->bus, out->ppu, out->cpu);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	out->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 240);

	return out;
}
void NES_Free(struct NES** nes)
{
	if (nes && *nes)
	{
		struct NES* n = *nes;
		if (n->cart) NES_Cart_Free(&n->cart);

		SDL_DestroyTexture(n->texture);
		NES_DBus_Uninit(n->bus);
		free(n->bus);
		n->bus = 0;

		NES_CPU_Free(&n->cpu);
		NES_PPU_Free(&n->ppu);


		free(*nes);
		*nes = 0;
	}
}
uint8_t NES_Tick(struct AppData* app, struct NES* nes, float dt)
{
	nes->bus->controller[0] = 0x00;
	nes->bus->controller[0] |= SDL_GameControllerGetButton(app->controller, SDL_CONTROLLER_BUTTON_A) ? 0x80 : 0x00; // A
	nes->bus->controller[0] |= SDL_GameControllerGetButton(app->controller, SDL_CONTROLLER_BUTTON_B) ? 0x40 : 0x00; // B
	nes->bus->controller[0] |= SDL_GameControllerGetButton(app->controller, SDL_CONTROLLER_BUTTON_START) ? 0x20 : 0x00; // START
	nes->bus->controller[0] |= SDL_GameControllerGetButton(app->controller, SDL_CONTROLLER_BUTTON_BACK) ? 0x10 : 0x00; // SELECT
	nes->bus->controller[0] |= SDL_GameControllerGetButton(app->controller, SDL_CONTROLLER_BUTTON_DPAD_UP) ? 0x08 : 0x00; // UP
	nes->bus->controller[0] |= SDL_GameControllerGetButton(app->controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ? 0x04 : 0x00; // DOWN
	nes->bus->controller[0] |= SDL_GameControllerGetButton(app->controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? 0x02 : 0x00; // LEFT
	nes->bus->controller[0] |= SDL_GameControllerGetButton(app->controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ? 0x01 : 0x00; // RIGHT

	
	nes->bus->controller[0] |= app->keys[SDL_SCANCODE_SPACE] ? 0x80 : 0x00; // A
	nes->bus->controller[0] |= app->keys[SDL_SCANCODE_LSHIFT] ? 0x40 : 0x00; // B
	nes->bus->controller[0] |= app->keys[SDL_SCANCODE_ESCAPE] ? 0x10 : 0x00; // SELECT
	nes->bus->controller[0] |= app->keys[SDL_SCANCODE_UP] ? 0x08 : 0x00; // UP
	nes->bus->controller[0] |= app->keys[SDL_SCANCODE_DOWN] ? 0x04 : 0x00; // DOWN
	nes->bus->controller[0] |= app->keys[SDL_SCANCODE_LEFT] ? 0x02 : 0x00; // LEFT
	nes->bus->controller[0] |= app->keys[SDL_SCANCODE_RIGHT] ? 0x01 : 0x00; // RIGHT
	
	if (nes->timer > 0.0f)
	{
		nes->timer -= dt;
	}
	else
	{
		nes->timer = nes->timer - dt + (1.0f / 60.0f);
		if (nes->cart)
		{
			do {
				NES_DBus_Clock(nes->bus);
			} while (!nes->ppu->is_frame_complete);
			nes->ppu->is_frame_complete = 0;

			SDL_UpdateTexture(nes->texture, NULL, nes->ppu->out_screen, 256 * 3);
		}
	}
	SDL_RenderCopy(app->renderer, nes->texture, NULL, NULL);
}

uint8_t NES_LoadFile(struct NES* nes, const char* filename)
{
	if (nes->cart) NES_Cart_Free(&nes->cart);
	nes->cart = NES_Cart_AllocFromFile(filename);
	NES_DBus_InsertCartridge(nes->bus, nes->cart);
	NES_DBus_Reset(nes->bus);
	nes->timer = 0.0f;
	if (nes->cart)
	{
		return 1;
	}
	return 0;
}
uint8_t NES_Load(struct NES* nes, uint8_t* data, int sz)
{
	nes->cart = NES_Cart_Alloc(data, sz);
	NES_DBus_InsertCartridge(nes->bus, nes->cart);
	NES_DBus_Reset(nes->bus);
	if (nes->cart) return 1;
	return 0;
}

float NES_GetAudioSample(struct NES* nes)
{
	float out = 0.0f;
	struct APU* apu = nes->bus->apu;
	if (apu->buf.cur_read < apu->buf.cur_write)
	{
		out = apu->buf.data[apu->buf.cur_read % apu->buf.size];
		apu->buf.cur_read = apu->buf.cur_read + 1;
	}
	else
	{
		out = apu->buf.data[(apu->buf.cur_read - 1) % apu->buf.size];
	}
	return out;
}