#include "monexamine.h"
#include "monster.h"
#include "game.h"
#include "map.h"
#include "messages.h"

const efftype_id effect_milked( "milked" );

void monexamine::milk_source( monster *source_mon )
{
    if ( source_mon == nullptr ) {

        debugmsg("could not find creature to milk");
    }
    else {
        item milk("milk", 0, 1);
        if (source_mon->get_effect_dur(effect_milked) < HOURS(24)) {
            g->handle_liquid(milk, nullptr, 0, nullptr, nullptr, source_mon);
            add_msg(_("You milk the %s."), source_mon->get_name().c_str());
        }
        else {
            add_msg(_("The %s's udders run dry."), source_mon->get_name().c_str());
        }
    }
}


