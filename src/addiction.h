#pragma once
#ifndef ADDICTION_H
#define ADDICTION_H

#include <string>

#include "type_id.h"

class addiction;
class player;

enum add_type : int;

constexpr int MIN_ADDICTION_LEVEL = 3; // Minimum intensity before effects are seen
constexpr int MAX_ADDICTION_LEVEL = 20;

// cancel_activity is called when the addiction effect wants to interrupt the player
// with an optional pre-translated message.
void addict_effect( player &u, addiction &add );

std::string addiction_type_name( add_type cur );

std::string addiction_name( const addiction &cur );

morale_type addiction_craving( add_type cur );

add_type addiction_type( const std::string &name );

std::string addiction_text( const addiction &cur );

#endif
