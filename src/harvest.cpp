#include "harvest.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>

#include "assign.h"
#include "debug.h"
#include "item.h"
#include "item_group.h"
#include "json.h"
#include "output.h"
#include "string_formatter.h"
#include "string_id.h"
#include "text_snippets.h"

// TODO: Make a generic factory
static std::map<harvest_id, harvest_list> harvest_all;

/** @relates string_id */
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

/** @relates string_id */
template<>
bool string_id<harvest_list>::is_valid() const
{
    return harvest_all.count( *this ) > 0;
}

harvest_list::harvest_list() : id_( harvest_id::NULL_ID() ) {}

const harvest_id &harvest_list::id() const
{
    return id_;
}

std::string harvest_list::message() const
{
    return SNIPPET.expand( message_.translated() );
}

bool harvest_list::is_null() const
{
    return id_ == harvest_id::NULL_ID();
}

bool harvest_list::has_entry_type( std::string type ) const
{
    for( const harvest_entry &entry : entries() ) {
        if( entry.type == type ) {
            return true;
        }
    }
    return false;
}

harvest_entry harvest_entry::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    harvest_entry ret;
    assign( jo, "drop", ret.drop, strict );
    assign( jo, "base_num", ret.base_num, strict, -1000.0f );
    assign( jo, "scale_num", ret.scale_num, strict, -1000.0f );
    assign( jo, "max", ret.max, strict, 1 );
    assign( jo, "type", ret.type, strict );
    assign( jo, "mass_ratio", ret.mass_ratio, strict, 0.00f );
    assign( jo, "flags", ret.flags );
    assign( jo, "faults", ret.faults );

    return ret;
}

const harvest_id &harvest_list::load( const JsonObject &jo, const std::string &src,
                                      const std::string &force_id )
{
    harvest_list ret;
    if( jo.has_string( "id" ) ) {
        ret.id_ = harvest_id( jo.get_string( "id" ) );
    } else if( !force_id.empty() ) {
        ret.id_ = harvest_id( force_id );
    } else {
        jo.throw_error( "id was not specified for harvest" );
    }

    jo.read( "message", ret.message_ );

    for( const JsonObject current_entry : jo.get_array( "entries" ) ) {
        ret.entries_.push_back( harvest_entry::load( current_entry, src ) );
    }

    auto &new_entry = harvest_all[ ret.id_ ];
    new_entry = ret;
    return new_entry.id();
}

void harvest_list::finalize()
{
    std::transform( entries_.begin(), entries_.end(), std::inserter( names_, names_.begin() ),
    []( const harvest_entry & entry ) {
        return item::type_is_defined( entry.drop ) ? item::nname( entry.drop ) : "";
    } );
}

void harvest_list::finalize_all()
{
    for( auto &pr : harvest_all ) {
        pr.second.finalize();
    }
}

void harvest_list::reset()
{
    harvest_all.clear();
}

const std::map<harvest_id, harvest_list> &harvest_list::all()
{
    return harvest_all;
}

void harvest_list::check_consistency()
{
    for( const auto &pr : harvest_all ) {
        const auto &hl = pr.second;
        const std::string hl_id = hl.id().c_str();
        auto error_func = [&]( const harvest_entry & entry ) {
            std::string errorlist;
            bool item_valid = true;
            if( !( item::type_is_defined( entry.drop ) || ( entry.type == "bionic_group" &&
                    item_group::group_is_defined( item_group_id( entry.drop ) ) ) ) ) {
                item_valid = false;
                errorlist += entry.drop;
            }
            // non butchery harvests need to be excluded
            if( hl_id.substr( 0, 14 ) != "harvest_inline" ) {
                if( entry.type == "null" ) {
                    if( !item_valid ) {
                        errorlist += ", ";
                    }
                    errorlist += "null type";
                } else if( !( entry.type == "flesh" || entry.type == "bone" || entry.type == "skin" ||
                              entry.type == "offal" || entry.type == "bionic" || entry.type == "bionic_group" ) ) {
                    if( !item_valid ) {
                        errorlist += ", ";
                    }
                    errorlist += entry.type;
                }
            }
            return errorlist;
        };
        const std::string errors = enumerate_as_string( hl.entries_.begin(), hl.entries_.end(),
                                   error_func );
        if( !errors.empty() ) {
            debugmsg( "Harvest list %s has invalid entry: %s", hl_id, errors );
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

std::string harvest_list::describe( int at_skill ) const
{
    if( empty() ) {
        return "";
    }

    return enumerate_as_string( entries().begin(), entries().end(),
    [at_skill]( const harvest_entry & en ) {
        float min_f = en.base_num.first;
        float max_f = en.base_num.second;
        if( at_skill >= 0 ) {
            min_f += en.scale_num.first * at_skill;
            max_f += en.scale_num.second * at_skill;
        } else {
            max_f = en.max;
        }
        // TODO: Avoid repetition here by making a common harvest drop function
        int max_drops = std::min<int>( en.max, std::round( std::max( 0.0f, max_f ) ) );
        int min_drops = std::max<int>( 0.0f, std::round( std::min( min_f, max_f ) ) );
        if( max_drops <= 0 ) {
            return std::string();
        }

        std::string ss;
        ss += "<bold>" + item::nname( en.drop, max_drops ) + "</bold>";
        // If the number is unspecified, just list the type
        if( max_drops >= 1000 && min_drops <= 0 ) {
            return ss;
        }
        ss += ": ";
        if( min_drops == max_drops ) {
            ss += string_format( "<stat>%d</stat>", min_drops );
        } else if( max_drops < 1000 ) {
            ss += string_format( "<stat>%d-%d</stat>", min_drops, max_drops );
        } else {
            ss += string_format( "<stat>%d+</stat>", min_drops );
        }

        return ss;
    } );
}
