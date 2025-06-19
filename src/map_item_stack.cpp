#include "map_item_stack.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>

#include "item.h"
#include "item_category.h"
#include "item_search.h"
#include "item_tname.h"
#include "localized_comparator.h"

map_item_stack::item_group::item_group() : count( 0 ), it( nullptr )
{
}

map_item_stack::item_group::item_group( const tripoint_rel_ms &p, const int arg_count,
                                        const item *itm ) : pos( p ),
    count( arg_count ), it( itm )
{
}

map_item_stack::map_item_stack() : example( nullptr ), totalcount( 0 )
{
    vIG.emplace_back( );
}

map_item_stack::map_item_stack( const item *const it, const tripoint_rel_ms &pos ) : example( it ),
    totalcount( it->count() )
{
    vIG.emplace_back( pos, totalcount, it );
}

void map_item_stack::add_at_pos( const item *const it, const tripoint_rel_ms &pos )
{
    const int amount = it->count();

    if( vIG.empty() || vIG.back().pos != pos ) {
        vIG.emplace_back( pos, amount, it );
    } else {
        vIG.back().count += amount;
    }

    totalcount += amount;
}

bool map_item_stack::compare_item_names( const map_item_stack &lhs, const map_item_stack &rhs )
{
    std::string left = lhs.example->tname( 1, tname::unprefixed_tname );
    std::string right = rhs.example->tname( 1, tname::unprefixed_tname );
    transform( left.begin(), left.end(), left.begin(), ::tolower );
    transform( right.begin(), right.end(), right.begin(), ::tolower );

    return localized_compare( left, right );
}

bool map_item_stack::map_item_stack_sort_category_distance( const map_item_stack &lhs,
        const map_item_stack &rhs )
{
    const item_category &lhs_cat = lhs.example->get_category_of_contents();
    const item_category &rhs_cat = rhs.example->get_category_of_contents();

    if( lhs_cat == rhs_cat ) {
        return square_dist( tripoint_rel_ms::zero, lhs.vIG[0].pos ) <
               square_dist( tripoint_rel_ms::zero, rhs.vIG[0].pos );
    }

    return lhs_cat < rhs_cat;
}

bool map_item_stack::map_item_stack_sort_category_name( const map_item_stack &lhs,
        const map_item_stack &rhs )
{
    const item_category &lhs_cat = lhs.example->get_category_of_contents();
    const item_category &rhs_cat = rhs.example->get_category_of_contents();

    if( lhs_cat == rhs_cat ) {
        return compare_item_names( lhs, rhs );
    }

    return lhs_cat < rhs_cat;
}

bool map_item_stack::map_item_stack_sort_name( const map_item_stack &lhs,
        const map_item_stack &rhs )
{
    return compare_item_names( lhs, rhs );
}

std::vector<map_item_stack> filter_item_stacks( const std::vector<map_item_stack> &stack,
        const std::string &filter )
{
    std::vector<map_item_stack> ret;

    auto z = item_filter_from_string( filter );
    std::copy_if( stack.begin(), stack.end(),
    std::back_inserter( ret ), [z]( const map_item_stack & a ) {
        if( a.example != nullptr ) {
            return z( *a.example );
        }
        return false;
    } );

    return ret;
}

//returns the first non priority items.
int list_filter_high_priority( std::vector<map_item_stack> &stack, const std::string &priorities )
{
    // TODO:optimize if necessary
    std::vector<map_item_stack> tempstack;
    const auto filter_fn = item_filter_from_string( priorities );
    for( auto it = stack.begin(); it != stack.end(); ) {
        if( priorities.empty() || ( it->example != nullptr && !filter_fn( *it->example ) ) ) {
            tempstack.push_back( *it );
            it = stack.erase( it );
        } else {
            ++it;
        }
    }

    int id = stack.size();
    for( map_item_stack &elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}

int list_filter_low_priority( std::vector<map_item_stack> &stack, const int start,
                              const std::string &priorities )
{
    // TODO:optimize if necessary
    std::vector<map_item_stack> tempstack;
    const auto filter_fn = item_filter_from_string( priorities );
    for( auto it = stack.begin() + start; it != stack.end(); ) {
        if( !priorities.empty() && it->example != nullptr && filter_fn( *it->example ) ) {
            tempstack.push_back( *it );
            it = stack.erase( it );
        } else {
            ++it;
        }
    }

    int id = stack.size();
    for( map_item_stack &elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}
