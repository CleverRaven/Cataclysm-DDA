#include <algorithm>

#include "contents_change_handler.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "item.h" // IWYU pragma: keep
#include "item_pocket.h"
#include "json.h"


void contents_change_handler::sort_containers()
{
    sorted_containers.clear();
    for( const item_location &loc : unsealed ) {
        sorted_containers.emplace( loc );
    }
}

bool contents_change_handler::finished() const
{
    return sorted_containers.empty();
}

void contents_change_handler::add_unsealed( const item_location &loc )
{
    if( std::find( unsealed.begin(), unsealed.end(), loc ) == unsealed.end() ) {
        unsealed.emplace_back( loc );
    }
}

void contents_change_handler::unseal_pocket_containing( const item_location &loc )
{
    if( loc.has_parent() ) {
        item_location parent = loc.parent_item();
        item_pocket *const pocket = loc.parent_pocket();
        if( pocket ) {
            // on_contents_changed restacks the pocket and should be called later
            // in Character::handle_contents_changed
            pocket->unseal();
        } else {
            debugmsg( "parent container does not contain item" );
        }
        parent.on_contents_changed();
        add_unsealed( parent );
    }
}

void contents_change_handler::serialize( JsonOut &jsout ) const
{
    jsout.write( unsealed );
    jsout.write( sorted_containers );
}

void contents_change_handler::deserialize( const JsonValue &jsin )
{
    jsin.read( unsealed );
    jsin.read( sorted_containers );
}

void item_loc_with_depth::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "item_location", _loc );
    jsout.member( "depth", _depth );

    jsout.end_object();
}

void item_loc_with_depth::deserialize( const JsonObject &obj )
{
    obj.read( "item_location", _loc );
    obj.read( "depth", _depth );
}
