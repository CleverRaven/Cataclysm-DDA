#include <algorithm>
#include <string>
#include "assign.h"
#include "debug.h"
#include "harvest.h"
#include "item.h"
#include "output.h"
#include "requirements.h"
#include "inventory.h"
#include "messages.h"

#include <numeric>

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

const harvest_id &harvest_list::load( JsonObject &jo, const std::string &src, const std::string &id )
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

    if( jo.has_string( "requirements" ) ) {
        ret.reqs.clear();
        ret.reqs.emplace_back( requirement_id( jo.get_string( "requirements" ) ), 1 );

    } else if( jo.has_array( "requirements" ) ) {
        ret.reqs.clear();
        auto arr = jo.get_array( "requirements" );
        while( arr.has_more() ) {
            auto cur = arr.next_array();
            ret.reqs.emplace_back( requirement_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }

    } else if( jo.has_object( "requirements" ) ) {
        auto req_id = string_format( "inline_harvest_%s", ret.id_.c_str() );
        JsonObject data = jo.get_object( "requirements" );
        requirement_data::load_requirement( data, req_id );
        ret.reqs = { { requirement_id( req_id ), 1 } };
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

        for( const auto &e : hl.reqs ) {
            if( !( e.first.is_null() || e.first.is_valid() ) || e.second < 0 ) {
                debugmsg( "harvest list %s has unknown or incorrectly specified requirements %s",
                          hl.id_.c_str(), e.first.c_str() );
            }
        }
    }
}

requirement_data harvest_list::requirements() const
{
    return std::accumulate( reqs.begin(), reqs.end(), requirement_data(),
        []( const requirement_data &lhs, const std::pair<requirement_id, int> &rhs ) {
        return lhs + ( *rhs.first * rhs.second );
    } );
}

bool harvest_list::can_harvest( const inventory &inv, bool alert ) const {
    const requirement_data r = requirements();

    for( const auto &opts : r.get_qualities() ) {
        for( const auto &qual : opts ) {
            if( !qual.has( inv ) ) {
                if( alert ) {
                    add_msg( m_info, _( "To perform this action you need %s" ), qual.to_string().c_str() );
                }
                return false;
            }
        }
    }

    for( const auto &opts : r.get_tools() ) {
        if( !std::any_of( opts.begin(), opts.end(), [&]( const tool_comp & tool ) {
        return ( tool.count <= 0 && inv.has_tools( tool.type, 1 ) ) ||
               ( tool.count  > 0 && inv.has_charges( tool.type, tool.count ) );
        } ) ) {
            if( alert ) {
                if( opts[0].count <= 0 ) {
                    add_msg( m_info, _( "You need a %s to perform this action." ),
                             item::nname( opts[0].type ).c_str() );
                } else {
                    add_msg( m_info, ngettext( "You need a %s with %d charge to perform this action.",
                                               "You need a %s with %d charges to perform this action.",
                                               opts[0].count ),
                            item::nname( opts[0].type ).c_str(), opts[0].count );
                }
            }
            return false;
        }
    }

    return true;
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
