#pragma once
#ifndef CATA_SRC_ADDICTION_H
#define CATA_SRC_ADDICTION_H

#include <iosfwd>

#include "pldata.h"
#include "type_id.h"

class Character;

// Minimum intensity before effects are seen
constexpr int MIN_ADDICTION_LEVEL = 3;
constexpr int MAX_ADDICTION_LEVEL = 20;

// cancel_activity is called when the addiction effect wants to interrupt the player
// with an optional pre-translated message.
void addict_effect( Character &u, addiction &add );

std::string addiction_type_name( add_type cur );

std::string addiction_name( const addiction &cur );

morale_type addiction_craving( add_type cur );

add_type addiction_type( const std::string &name );

std::string addiction_text( const addiction &cur );

#endif // CATA_SRC_ADDICTION_H
