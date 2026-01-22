#pragma once
#ifndef CATA_SRC_SDL_GAMEPAD_H
#define CATA_SRC_SDL_GAMEPAD_H
#if defined(TILES)
#include "input_enums.h"
#include "sdl_wrappers.h"

#define SDL_GAMEPAD_SCHEDULER (SDL_USEREVENT+1)

struct tripoint;

// IWYU pragma: no_forward_declare input_event  // Is valid, but looks silly
extern input_event last_input;

namespace gamepad
{

// Direction states for left stick
enum class direction : int {
    NONE = 0,
    N,    // North (up)
    NE,   // Northeast
    E,    // East (right)
    SE,   // Southeast
    S,    // South (down)
    SW,   // Southwest
    W,    // West (left)
    NW    // Northwest
};

void init();
void quit();
// Returns true if the selected direction changed (UI needs refresh)
bool handle_axis_event( SDL_Event &event );
void handle_button_event( SDL_Event &event );
void handle_device_event( SDL_Event &event );
void handle_scheduler_event( SDL_Event &event );
SDL_GameController *get_controller();

direction get_left_stick_direction();
direction get_right_stick_direction();
direction get_radial_left_direction();
direction get_radial_right_direction();
bool is_radial_left_open();
bool is_radial_right_open();
bool is_alt_held();
bool is_in_menu();
bool is_active();

// Convert direction enum to movement offset
tripoint direction_to_offset( direction dir );

// Convert direction to radial joy event
int direction_to_radial_joy( direction dir, int stick_idx );

} // namespace gamepad

#endif // TILES
#endif // CATA_SRC_SDL_GAMEPAD_H

