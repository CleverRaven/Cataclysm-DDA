#include "monexamine.h"
#include "monster.h"
#include "item.h"
#include "game.h"
#include "translations.h"
#include "messages.h"
#include "mtype.h"
#include "calendar.h"
#include <utility>

const efftype_id effect_milked( "milked" );

void monexamine::milk_source( monster &source_mon )
{
    const auto milked_item = source_mon.type->starting_ammo;
    item milk( milked_item.begin()->first, calendar::turn, 1 );

    // Milked items must be liquids.
    if( !milk.made_of( LIQUID ) ) {
        debugmsg( "milked item must be a liquid" );
    } else {

        const time_duration milk_per_day = 1_days / milked_item.begin()->second;

        if( !source_mon.has_effect( effect_milked ) ) {
            g->handle_liquid( milk, nullptr, 0, nullptr, nullptr, &source_mon );
            source_mon.add_effect( effect_milked, milk_per_day );
            add_msg( _( "You milk the %s." ), source_mon.get_name().c_str() );

        } else if( 1_days - source_mon.get_effect_dur( effect_milked ) >= milk_per_day ) {
            source_mon.add_effect( effect_milked, milk_per_day );
            g->handle_liquid( milk, nullptr, 0, nullptr, nullptr, &source_mon );
            add_msg( _( "You milk the %s." ), source_mon.get_name().c_str() );
        } else {
            add_msg( _( "The %s's udders run dry." ), source_mon.get_name().c_str() );
        }
    }
}
