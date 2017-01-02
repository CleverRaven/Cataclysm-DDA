#include <algorithm>
#include <string>
#include "assign.h"
#include "debug.h"
#include "harvest.h"
#include "item.h"
#include "output.h"

template <>
const harvest_id string_id<harvest_list>::NULL_ID( "null" );

static std::map<harvest_id, harvest_list> harvest_all;

template<>
const harvest_list &string_id<harvest_list>::obj() const
{
    const auto found = harvest_all.find( *this );
    if( found == harvest_all.end() ) {
        debugmsg( "Tried to get invalid harvest list: %s", c_str() );
        static const harvest_list null_list{};
        return null_list;
    }
    return found->second;
}

harvest_entry harvest_entry::load( JsonObject &jo, const std::string &src )
{
    const bool strict = src == "core";

    harvest_entry ret;
    assign( jo, "drop", ret.drop, strict );
    assign( jo, "base_num", ret.base_num, strict, -1000.0f );
    assign( jo, "scale_num", ret.scale_num, strict, -1000.0f );
    assign( jo, "max", ret.max, strict, 1 );

    return ret;
}

const harvest_id &harvest_list::load( JsonObject &jo, const std::string &src,
                                      const std::string &id )
{
    harvest_list ret;
    if( jo.has_string( "id" ) ) {
        ret.id_ = harvest_id( jo.get_string( "id" ) );
    } else if( !id.empty() ) {
        ret.id_ = harvest_id( id );
    } else {
        jo.throw_error( "id was not specified for harvest" );
    }

    JsonArray jo_entries = jo.get_array( "entries" );
    while( jo_entries.has_more() ) {
        JsonObject current_entry = jo_entries.next_object();
        ret.entries_.push_back( harvest_entry::load( current_entry, src ) );
    }

    auto &new_entry = harvest_all[ ret.id_ ];
    new_entry = ret;
    return new_entry.id();
}

void harvest_list::finalize()
{
    std::transform( entries_.begin(), entries_.end(), std::inserter( names_, names_.begin() ),
    []( const harvest_entry &entry ) {
        return item::type_is_defined( entry.drop ) ? item::nname( entry.drop ) : "";
    } );
}

void harvest_list::finalize_all()
{
    for( auto &pr : harvest_all ) {
        pr.second.finalize();
    }
}

const std::map<harvest_id, harvest_list> &harvest_list::all()
{
    return harvest_all;
}

void harvest_list::check_consistency()
{
    for( const auto &pr : harvest_all ) {
        const auto &hl = pr.second;
        const std::string errors = enumerate_as_string( hl.entries_.begin(), hl.entries_.end(),
        []( const harvest_entry &entry ) {
            return item::type_is_defined( entry.drop ) ? "" : entry.drop;
        } );
        if( !errors.empty() ) {
            debugmsg( "Harvest list %s has invalid drop(s): %s", hl.id_.c_str(), errors.c_str() );
        }
    }
}

std::list<harvest_entry>::const_iterator harvest_list::begin() const
{
    return entries().begin();
}

std::list<harvest_entry>::const_iterator harvest_list::end() const
{
    return entries().end();
}

std::list<harvest_entry>::const_reverse_iterator harvest_list::rbegin() const
{
    return entries().rbegin();
}

std::list<harvest_entry>::const_reverse_iterator harvest_list::rend() const
{
    return entries().rend();
}
