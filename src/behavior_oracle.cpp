#include "behavior_oracle.h"

#include <functional>
#include <array>
#include <list>

#include "behavior.h"
#include "bodypart.h"
#include "itype.h"
#include "player.h"
#include "weather.h"
#include "character.h"
#include "inventory.h"
#include "item.h"
#include "optional.h"
#include "ret_val.h"
#include "cata_string_consts.h"

namespace behavior
{

status_t return_running( const oracle_t * )
{
    return running;
}

// To avoid a local minima when the character has access to warmth in a shelter but gets cold
// when they go outside, this method needs to only alert when travel time to known shelter
// approaches time to freeze.
status_t character_oracle_t::needs_warmth_badly() const
{
    const player *p = dynamic_cast<const player *>( subject );
    // Use player::temp_conv to predict whether the Character is "in trouble".
    for( const body_part bp : all_body_parts ) {
        if( p->temp_conv[ bp ] <= BODYTEMP_VERY_COLD ) {
            return running;
        }
    }
    return success;
}

status_t character_oracle_t::needs_water_badly() const
{
    // Check thirst threshold.
    if( subject->get_thirst() > 520 ) {
        return running;
    }
    return success;
}

status_t character_oracle_t::needs_food_badly() const
{
    // Check hunger threshold.
    if( subject->get_hunger() >= 300 && subject->get_starvation() > 2500 ) {
        return running;
    }
    return success;
}

status_t character_oracle_t::can_wear_warmer_clothes() const
{
    const player *p = dynamic_cast<const player *>( subject );
    // Check inventory for wearable warmer clothes, greedily.
    // Don't consider swapping clothes yet, just evaluate adding clothes.
    for( const auto &i : subject->inv.const_slice() ) {
        const item &candidate = i->front();
        if( candidate.get_warmth() > 0 || p->can_wear( candidate ).success() ) {
            return running;
        }
    }
    return failure;
}

status_t character_oracle_t::can_make_fire() const
{
    // Check inventory for firemaking tools and fuel
    bool tool = false;
    bool fuel = false;
    for( const auto &i : subject->inv.const_slice() ) {
        const item &candidate = i->front();
        if( candidate.has_flag( flag_FIRESTARTER ) ) {
            tool = true;
            if( fuel ) {
                return running;
            }
        } else if( candidate.flammable() ) {
            fuel = true;
            if( tool ) {
                return running;
            }
        }
    }
    return success;
}

status_t character_oracle_t::can_take_shelter() const
{
    // See if we know about some shelter
    // Don't know how yet.
    return failure;
}

status_t character_oracle_t::has_water() const
{
    // Check if we know about water somewhere
    bool found_water = subject->inv.has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->quench > 0;
    } );
    return found_water ? running : failure;
}

status_t character_oracle_t::has_food() const
{
    // Check if we know about food somewhere
    bool found_food = subject->inv.has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->has_calories();
    } );
    return found_food ? running : failure;
}

// predicate_map doesn't have to live here, but for the time being it's pretty pointless
// to break it out into it's own module.
// In principle this can be populated with any function that has a matching signature.
// In practice each element is a pointer-to-function to one of the above methods so that
// They can have provlidged access to the subject's internals.

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
        { "npc_has_food", make_function( &character_oracle_t::has_food ) }
    }
};
} // namespace behavior
