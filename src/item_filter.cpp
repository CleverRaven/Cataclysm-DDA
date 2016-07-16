#include "debug.h"
#include "item_filter.h"
#include "item_location.h"

item_filter_response::item_filter_response( response_type type, int rank ) :
    type( type ), rank( rank ) {}

const item_filter_response &item_filter_response::make_unsuitable()
{
    static const item_filter_response refusal( unsuitable );
    return refusal;
}

const item_filter_response &item_filter_response::make_fine()
{
    static const item_filter_response acceptance( fine );
    return acceptance;
}

const item_filter_response &item_filter_response::make( bool result )
{
    return result ? make_fine() : make_unsuitable();
}

item_filter_response item_filter_response::make_unready( int rank )
{
    return item_filter_response( unready, rank );
}

item_filter_response item_filter_response::make_fine( int rank )
{
    return item_filter_response( fine, rank );
}

item_filter_advanced convert_filter( const item_filter_simple &filter )
{
    return [ &filter ]( const item_location & loc ) {
        return item_filter_response::make( filter( *loc ) );
    };
}
