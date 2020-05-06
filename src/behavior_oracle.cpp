#include "behavior_oracle.h"

#include <functional>
#include <string>
#include <unordered_map>

#include "behavior.h"
#include "character_oracle.h"
#include "monster_oracle.h"

namespace behavior
{

status_t return_running( const oracle_t * )
{
    return running;
}

// Just a little helper to make populating predicate_map slightly less gross.
static std::function < status_t( const oracle_t * ) >
make_function( status_t ( character_oracle_t::* fun )() const )
{
    return static_cast<status_t ( oracle_t::* )() const>( fun );
}

std::unordered_map<std::string, std::function<status_t( const oracle_t * )>> predicate_map = {{
        { "npc_needs_warmth_badly", make_function( &character_oracle_t::needs_warmth_badly ) },
        { "npc_needs_water_badly", make_function( &character_oracle_t::needs_water_badly ) },
        { "npc_needs_food_badly", make_function( &character_oracle_t::needs_food_badly ) },
        { "npc_can_wear_warmer_clothes", make_function( &character_oracle_t::can_wear_warmer_clothes ) },
        { "npc_can_make_fire", make_function( &character_oracle_t::can_make_fire ) },
        { "npc_can_take_shelter", make_function( &character_oracle_t::can_take_shelter ) },
        { "npc_has_water", make_function( &character_oracle_t::has_water ) },
        { "npc_has_food", make_function( &character_oracle_t::has_food ) },
        { "monster_has_special", static_cast<status_t ( oracle_t::* )() const>( &monster_oracle_t::has_special ) },
        { "monster_not_hallucination", static_cast<status_t ( oracle_t::* )() const>( &monster_oracle_t::not_hallucination ) },
        { "monster_items_available", static_cast<status_t ( oracle_t::* )() const>( &monster_oracle_t::items_available ) }
    }
};

} // namespace behavior
