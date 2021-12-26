#pragma once
#ifndef GAMEPAD_H
#define GAMEPAD_H
#if defined(TILES)

#include "input.h"
#include "sdl_wrappers.h"

#define GAMEPAD_SCHEDULER SDL_USEREVENT+1

namespace gamepad
{

void init();
void quit();
void handle_axis_event( SDL_Event &event );
void handle_hat_event( SDL_Event &event );
void handle_button_event( SDL_Event &event );
void handle_scheduler_event( SDL_Event &event );
SDL_GameController *get_controller();

}

#endif // TILES
#endif // GAMEPAD_H

