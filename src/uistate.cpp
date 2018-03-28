#include "uistate.h"
#include "item.h"
#include "json.h"
#include "crafting.h"
#include "advanced_inv.h"
#include "construction.h"
#include "editmap.h"
#include "game.h"
#include "iexamine.h"
#include "overmap.h"
#include "player.h"
#include "wish.h"

uistatedata uistate;

uistatedata::uistatedata()
{
    modules.emplace_back( new JsonSerDesAdapter<advanced_inv_uistatedata>( *advanced_inv ) );
    modules.emplace_back( new JsonSerDesAdapter<construction_uistatedata>( *construction ) );
    modules.emplace_back( new JsonSerDesAdapter<crafting_uistatedata>( *crafting ) );
    modules.emplace_back( new JsonSerDesAdapter<editmap_uistatedata>( *editmap ) );
    modules.emplace_back( new JsonSerDesAdapter<game_uistatedata>( *game ) );
    modules.emplace_back( new JsonSerDesAdapter<iexamine_uistatedata>( *iexamine ) );
    modules.emplace_back( new JsonSerDesAdapter<overmap_uistatedata>( *overmap ) );
    modules.emplace_back( new JsonSerDesAdapter<player_uistatedata>( *player ) );
    modules.emplace_back( new JsonSerDesAdapter<wish_uistatedata>( *wish ) );
}

void uistatedata::serialize( JsonOut &json ) const
{
    json.start_object();

    for( auto &module : modules ) {
        module->serialize( json );
    }

    json.end_object();
}

void uistatedata::deserialize( JsonIn &jsin )
{
    int start = jsin.tell();
    auto jo = jsin.get_object();
    int end = jsin.tell();

    for( auto &module : modules ) {
        jsin.seek( start );
        module->deserialize( jsin );
    }

    jsin.seek( end );
}
