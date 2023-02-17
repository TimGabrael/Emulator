#include "AppData.h"
#include <memory.h>
#include "Helper.h"
#include "nes/nes_collection.h"
#include "nes/DataBus.h"
#include "nes/apu.h"
#include "ps1/ps1_collection.h"

static void _AD_AudioCallback(void* udata, Uint8* stream, int len)
{
    struct AppData* app = (struct AppData*)udata;
    uint16_t* ustream = (uint16_t*)stream;
    for (int i = 0; i < len / 2; i++)
    {
        if (app->nes)
        {
            struct APU* apu = app->nes->bus->apu;
            if (apu->buf.cur_read < apu->buf.cur_write)
            {
                ustream[i] = apu->buf.data[apu->buf.cur_read % apu->buf.size] * INT16_MAX;
                apu->buf.cur_read = apu->buf.cur_read + 1;
            }
            else
            {
                ustream[i] = apu->buf.data[(apu->buf.cur_read - 1) % apu->buf.size] * INT16_MAX;
            }
        }
    }
}


struct AppData* AD_Alloc(int w, int h)
{
    struct AppData* result = malloc(sizeof(struct AppData));
    if (!result) return NULL;

	memset(result, 0, sizeof(struct AppData));
    if (SDL_CreateWindowAndRenderer(w, h, 0, &result->window, &result->renderer) < 0)
    {
        LOG("FAILED TO CREATE WINDOW AND RENDERER\n");
        free(result);
        return NULL;
    }

    SDL_AudioSpec wanted;
    memset(&wanted, 0, sizeof(SDL_AudioSpec));
    wanted.freq = 44100;
    wanted.format = AUDIO_S16;
    wanted.channels = 1;
    wanted.samples = 1024;
    wanted.callback = _AD_AudioCallback;
    wanted.userdata = result;

    SDL_AudioSpec obtained;
    result->audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
    if (result->audio_device <= 0)
    {
        LOG("FAILED TO OPEN AUDIO DEVICE\n");
        free(result);
        return NULL;
    }

    result->nes = NES_Alloc(result);
    result->ps1 = PS1_Alloc(result);
    SDL_PauseAudioDevice(result->audio_device, SDL_FALSE);

	return result;
}

void AD_Free(struct AppData** app)
{
    if (app && *app)
    {
        struct AppData* a = *app;
        SDL_PauseAudioDevice(a->audio_device, SDL_TRUE);
        SDL_CloseAudioDevice(a->audio_device);

        SDL_DestroyRenderer(a->renderer);
        SDL_DestroyWindow(a->window);

        free(a);
        *app = NULL;
    }
}