#include "item_components.h"

#include <iterator>
#include <list>
#include <memory>

#include "debug.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "ret_val.h"
#include "rng.h"
#include "type_id.h"

std::vector<item> item_components::operator[]( const itype_id it_id )
{
    return comps[it_id];
}

item_components::comp_iterator item_components::begin()
{
    return comps.begin();
}
item_components::comp_iterator item_components::end()
{
    return comps.end();
}
item_components::const_comp_iterator item_components::begin() const
{
    return comps.begin();
}
item_components::const_comp_iterator item_components::end() const
{
    return comps.end();
}

bool item_components::empty()
{
    return comps.empty();
}

bool item_components::empty() const
{
    return comps.empty();
}

void item_components::clear()
{
    comps.clear();
}

item item_components::only_item()
{
    if( comps.size() != 1 || comps.begin()->second.size() != 1 ) {
        debugmsg( "item_components::only_item called but components don't contain exactly one item" );
        return item();
    }
    return *comps.begin()->second.begin();
}

item item_components::only_item() const
{
    if( comps.size() != 1 || comps.begin()->second.size() != 1 ) {
        debugmsg( "item_components::only_item called but components don't contain exactly one item" );
        return item();
    }
    return *comps.begin()->second.begin();
}

size_t item_components::size() const
{
    size_t ret = 0;
    for( const type_vector_pair &tvp : comps ) {
        ret += tvp.second.size();
    }
    return ret;
}

void item_components::add( item &new_it )
{
    comp_iterator it = comps.find( new_it.typeId() );
    if( it != comps.end() ) {
        if( it->first->count_by_charges() ) {
            it->second.front().charges += new_it.charges;
        } else {
            it->second.push_back( new_it );
        }
    } else {
        comps[new_it.typeId()] = { new_it };
    }
}

ret_val<item> item_components::remove( itype_id it_id )
{
    comp_iterator it = comps.find( it_id );
    if( it == comps.end() ) {
        return ret_val<item>::make_failure( item() );
    }
    item itm = *it->second.begin();
    it->second.erase( it->second.begin() );
    if( it->second.empty() ) {
        comps.erase( it );
    }
    return ret_val<item>::make_success( itm );
}

item item_components::get_and_remove_random_entry()
{
    comp_iterator iter = comps.begin();
    std::advance( iter, rng( 0, comps.size() - 1 ) );
    item ret = random_entry_removed( iter->second );
    if( iter->second.empty() ) {
        comps.erase( iter );
    }
    return ret;
}

static void adjust_new_comp( item &new_comp, bool is_cooked )
{
    if( new_comp.is_comestible() && !new_comp.get_comestible()->cooks_like.is_empty() ) {
        const double relative_rot = new_comp.get_relative_rot();
        new_comp = item( new_comp.get_comestible()->cooks_like, new_comp.birthday(), new_comp.charges );
        new_comp.set_relative_rot( relative_rot );
    }

    if( is_cooked ) {
        new_comp.set_flag_recursive( flag_COOKED );
    }
}

item_components item_components::split( const int batch_size, const size_t offset,
                                        const bool is_cooked )
{
    item_components ret;

    for( item_components::type_vector_pair &tvp : comps ) {
        if( tvp.first->count_by_charges() ) {
            if( tvp.second.size() != 1 ) {
                debugmsg( "count by charges component %s wasn't merged properly, can't distribute components to resulting items",
                          tvp.first.str() );
                return item_components();
            }

            item new_comp( tvp.second.front() );

            if( new_comp.charges % batch_size != 0 ) {
                debugmsg( "component %s can't be evenly distributed to resulting items", tvp.first.str() );
                return item_components();
            }

            adjust_new_comp( new_comp, is_cooked );

            new_comp.charges /= batch_size;
            ret.add( new_comp );
        } else {
            if( tvp.second.size() % batch_size != 0 ) {
                debugmsg( "component %s can't be evenly distributed to resulting items", tvp.first.str() );
                return item_components();
            }

            for( size_t i = offset; i < tvp.second.size(); i += batch_size ) {
                item new_comp( tvp.second[i] );
                adjust_new_comp( new_comp, is_cooked );
                ret.add( new_comp );
            }
        }
    }

    return ret;
}

void item_components::serialize( JsonOut &jsout ) const
{
    jsout.write( comps );
}

void item_components::deserialize( const JsonValue &jv )
{
    comps.clear();
    // read legacy arrays
    if( jv.test_array() ) {
        std::list<item> temp;
        jv.read( temp );
        for( item &it : temp ) {
            add( it );
        }
    } else {
        jv.read( comps );
    }
}
