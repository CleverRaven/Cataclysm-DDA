#include <array>
#include <list>
#include <memory>

#include "behavior.h"
#include "character_oracle.h"
#include "bodypart.h"
#include "character.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "player.h"
#include "ret_val.h"
#include "value_ptr.h"
#include "weather.h"

static const std::string flag_FIRESTARTER( "FIRESTARTER" );

namespace behavior
{

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

} // namespace behavior
