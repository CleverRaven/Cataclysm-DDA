#ifndef ITEM_FILTER_H
#define ITEM_FILTER_H

#include <string>
#include <functional>

class item;
class item_location;

struct item_filter_response {
    enum response_type : int {
        unsuitable = 0, // the item can't be used whatever happens
        unready,        // normally, the item can be used, but something's wrong with this one
        fine,           // the item is free to use
    };

    response_type type;
    int rank;                   // For sorting purposes

    item_filter_response( response_type type = unsuitable, int rank = 0 );
    // Common cases
    static const item_filter_response &make_unsuitable();
    static const item_filter_response &make_fine();
    static const item_filter_response &make( bool result = false );
    // Advanced cases require unique descriptions
    static item_filter_response make_unready( int rank = 0 );
    static item_filter_response make_fine( int rank );
};

typedef std::function<bool( const item & )> item_filter_simple;
typedef std::function<item_filter_response( const item_location & )> item_filter_advanced;

item_filter_advanced convert_filter( const item_filter_simple &filter );

#endif // ITEM_FILTER_H
