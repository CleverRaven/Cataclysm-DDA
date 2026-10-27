#pragma once
#ifndef CATA_SRC_TOUCH_JOYSTICK_H
#define CATA_SRC_TOUCH_JOYSTICK_H

#include <string>
#include <utility>
#include <vector>

// direction and speed of the Android touch joystick, shared by input and drawing
namespace touch_joystick
{

// clockwise from right, in screen space where y grows downward
enum class direction : int {
    none,
    right,
    down_right,
    down,
    down_left,
    left,
    up_left,
    up,
    up_right,
};

// direction for a touch offset from where the finger went down. straight slices
// span straight_width degrees, diagonal ones the rest of each quarter turn; 90
// leaves no diagonals. none inside the deadzone, edge included
direction classify( float dx, float dy, float deadzone, float straight_width );

// slice of a direction in degrees, clockwise from right; start can be negative
std::pair<float, float> wedge( direction dir, float straight_width );

// 0 at the deadzone edge up to 1 at range beyond it
float speed_fraction( float dist, float deadzone, float range );

// distance from center to the edge of a regular octagon at an angle, with flat
// sides facing the straight directions
float octagon_radius( float degrees, float apothem );

// angles outlining a slice of the octagon: both edges and every corner between,
// so straight segments between them follow the octagon exactly
std::vector<float> band_angles( float start, float end );

// whether an input context's actions are only the ones every context starts with,
// so it was pushed before its owner registered any; touch input keeps its mode
bool is_bare_context( const std::vector<std::string> &actions );

} // namespace touch_joystick

#endif // CATA_SRC_TOUCH_JOYSTICK_H
