#include "character.h"
#include "contents_change_handler.h"

#include <algorithm>

#include "debug.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "item_pocket.h"
#include "json.h"
#include "json_error.h"

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

void contents_change_handler::handle_by( Character &guy )
{
    // some containers could have been destroyed by e.g. monster attack
    auto it = std::remove_if( unsealed.begin(), unsealed.end(),
    []( const item_location & loc ) -> bool {
        return !loc;
    } );
    unsealed.erase( it, unsealed.end() );
    guy.handle_contents_changed( unsealed );
    unsealed.clear();
}

void contents_change_handler::serialize( JsonOut &jsout ) const
{
    jsout.write( unsealed );
}

void contents_change_handler::deserialize( const JsonValue &jsin )
{
    jsin.read( unsealed );
}

