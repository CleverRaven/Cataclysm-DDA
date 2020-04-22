#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <utility>

#include "activity_handlers.h"
#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "harvest.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "options_helpers.h"
#include "options.h"
#include "player.h"
#include "player_helpers.h"
#include "test_statistics.h"
#include "game_constants.h"
#include "item.h"
#include "line.h"
#include "point.h"

TEST_CASE( "quarter" )
{
    static const activity_id ACT_FIELD_DRESS( "ACT_FIELD_DRESS" );
    static const activity_id ACT_QUARTER( "ACT_QUARTER" );
    player &dummy = g->u;
    
    // Check field dress and quartering does not work on giant web spider
    static const mtype_id mon_spider_web( "mon_spider_web" );
    item spider = item::make_corpse( mon_spider_web );
    for ( const harvest_entry &entry : spider.get_mtype()->harvest.obj()) {
        activity_handlers::butcher_finish( &player_activity( activity_id ( ACT_FIELD_DRESS ) ), &dummy );
        activity_handlers::butcher_finish( &player_activity( activity_id( ACT_QUARTER ) ), &dummy );
        CHECK( !(spider.has_flag( "F_DRESS" ) || 
        spider.has_flag( "F_DRESS_FAILED" ) ||
        spider.has_flag( "QUARTERED" )));
    }

    // Check field dress and quartering does work on wolf
    static const mtype_id mon_wolf( "mon_wolf" );
    item wolf = item::make_corpse( mon_wolf );
    for ( const harvest_entry &entry : wolf.get_mtype()->harvest.obj()) {
        activity_handlers::butcher_finish( &player_activity( activity_id ( ACT_FIELD_DRESS ) ), &dummy );
        activity_handlers::butcher_finish( &player_activity( activity_id( ACT_QUARTER ) ), &dummy );
        CHECK( (wolf.has_flag( "F_DRESS" ) || 
        wolf.has_flag( "F_DRESS_FAILED" )) &&
        wolf.has_flag( "QUARTERED" ));
    }
}