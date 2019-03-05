#include "monexamine.h"

#include <utility>

#include "calendar.h"
#include "game.h"
#include "item.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "player.h"
#include "translations.h"

const efftype_id effect_milked( "milked" );

void monexamine::milk_source( monster &source_mon )
{
    const auto milked_item = source_mon.type->starting_ammo;
    const long milk_per_day = milked_item.begin()->second;
    const time_duration milking_freq = 1_days / milk_per_day;

    long remaining_milk = milk_per_day;
    if( source_mon.has_effect( effect_milked ) ) {
        remaining_milk -= source_mon.get_effect_dur( effect_milked ) / milking_freq;
    }

    if( remaining_milk > 0 ) {
        item milk( milked_item.begin()->first, calendar::turn, remaining_milk );
        if( g->handle_liquid( milk, nullptr, 1, nullptr, nullptr, -1, &source_mon ) ) {
            add_msg( _( "You milk the %s." ), source_mon.get_name().c_str() );
            long transferred_milk = remaining_milk - milk.charges;
            source_mon.add_effect( effect_milked, milking_freq * transferred_milk );
            g->u.mod_moves( -to_moves<int>( transferred_milk * 1_minutes / 5 ) );
        }
    } else {
        add_msg( _( "The %s's udders run dry." ), source_mon.get_name().c_str() );
    }
}
