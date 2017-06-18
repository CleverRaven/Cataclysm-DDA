#pragma once
#ifndef ADDICTION_H
#define ADDICTION_H

#include <string>
#include <functional>

#include "string_id.h"

class addiction;
class player;
enum add_type : int;
    class morale_type_data;
    using morale_type = string_id<morale_type_data>;

    constexpr int MIN_ADDICTION_LEVEL = 3; // Minimum intensity before effects are seen
    constexpr int MAX_ADDICTION_LEVEL = 20;

    // cancel_activity is called when the addication effect wants to interrupt the player
    // with an optional pre-translated message.
    void addict_effect( player &u, addiction &add );

    const std::string &addiction_type_name( add_type cur );

    const std::string &addiction_name( addiction const &cur );

    morale_type addiction_craving( add_type cur );

    add_type addiction_type( std::string const &name );

    const std::string &addiction_text( addiction const &cur );

#endif
