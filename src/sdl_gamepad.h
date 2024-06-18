#pragma once
#ifndef CATA_SRC_SDL_GAMEPAD_H
#define CATA_SRC_SDL_GAMEPAD_H
#if defined(TILES)

#include "input.h"
#include "sdl_wrappers.h"

#define SDL_GAMEPAD_SCHEDULER (SDL_USEREVENT+1)

extern input_event last_input;

namespace gamepad
{

void init();
void quit();
void handle_axis_event( SDL_Event &event );
void handle_button_event( SDL_Event &event );
void handle_scheduler_event( SDL_Event &event );
SDL_GameController *get_controller();

} // namespace gamepad

#endif // TILES
#endif // CATA_SRC_SDL_GAMEPAD_H

