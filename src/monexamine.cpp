#include "monexamine.h"
#include "monster.h"
#include "game.h"
#include "map.h"
#include "messages.h"

const efftype_id effect_milked( "milked" );

void monexamine::milk_source( monster &source_mon )
{

    monster *mon = &source_mon;
    item milk( "milk", 0, 1 );

    if( source_mon.get_effect_dur( effect_milked ) < HOURS( 24 ) ) {
        g->handle_liquid( milk, nullptr, 0, nullptr, nullptr, mon );
        add_msg( _( "You milk the %s." ), source_mon.get_name().c_str() );
    } else {
        add_msg( _( "The %s's udders run dry." ), source_mon.get_name().c_str() );
    }

}