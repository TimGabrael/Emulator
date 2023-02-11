#include <SDL2/SDL.h>
#include <stdio.h>
#include "Helper.h"
#include "AppData.h"
#include "nes/nes_collection.h"
#include "ps1/ps1_collection.h"

struct NES* nes = 0;
struct PS1* ps1 = 0;
int EmulatorMain(int w, int h)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        LOG("FAILED TO INITIALIZE SDL\n");
        return -1;
    }

    struct AppData* app = AD_Alloc(w, h);

    //nes = NES_Alloc(app);
    ps1 = PS1_Alloc(app);
    
    uint64_t prev = SDL_GetTicks64();
    uint8_t should_close = 0;
    while (!should_close) {


        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                should_close = 1;
                break;
            }
            else if (event.type == SDL_DROPFILE)
            {
                const char* dropped_filedir = event.drop.file;
                NES_LoadFile(nes, dropped_filedir);
            }
            else if (event.type == SDL_KEYDOWN) app->keys[event.key.keysym.scancode] = 1;
            else if (event.type == SDL_KEYUP) app->keys[event.key.keysym.scancode] = 0;
        }

        //SDL_RenderClear(app->renderer);

        uint64_t now = SDL_GetTicks64();

        float dt = (now - prev) * 0.01f;

        prev = now;
        //NES_Tick(app, nes, dt);
        PS1_Tick(app, ps1, dt);

        // leaving this away is just alot faster for now
        //SDL_RenderPresent(app->renderer);
#ifdef EMSCRIPTEN
        emscripten_sleep(0);
#endif

    }

    AD_Free(&app);

    SDL_Quit();


    return 0;
}



#ifdef _WIN32
#include <Windows.h>
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return EmulatorMain(800, 600);
}
#elif defined(EMSCRIPTEN)
EM_JS(int, canvas_get_width, (), {
  return canvas.width;
});

EM_JS(int, canvas_get_height, (), {
  return canvas.height;
});

int main(int argv, char** argc)
{
    return EmulatorMain(800, 600);
}

EMSCRIPTEN_KEEPALIVE void AddFile()
{
    NES_LoadFile(nes, "nes_file.nes");
}

#else

int main(int argv, char** argc)
{
    return EmulatorMain(300, 300);
}

#endif