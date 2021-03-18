#include "harvest.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>

#include "assign.h"
#include "debug.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "json.h"
#include "output.h"
#include "string_formatter.h"
#include "text_snippets.h"

namespace
{
generic_factory<harvest_list> harvest_list_factory( "harvest_list" );
} //namespace

/** @relates string_id */
template<>
const harvest_list &string_id<harvest_list>::obj() const
{
    return harvest_list_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<harvest_list>::is_valid() const
{
    return harvest_list_factory.is_valid( *this );
}

harvest_list::harvest_list() : id( harvest_id::NULL_ID() ) {}

std::string harvest_list::message() const
{
    return SNIPPET.expand( message_.translated() );
}

bool harvest_list::is_null() const
{
    return id == harvest_id::NULL_ID();
}

bool harvest_list::has_entry_type( const std::string &type ) const
{
    for( const harvest_entry &entry : entries() ) {
        if( entry.type == type ) {
            return true;
        }
    }
    return false;
}

void harvest_entry::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "drop", drop );

    optional( jo, was_loaded, "type", type );
    optional( jo, was_loaded, "base_num", base_num, { 1.0f, 1.0f } );
    optional( jo, was_loaded, "scale_num", scale_num, { 0.0f, 0.0f } );
    optional( jo, was_loaded, "max", max, 1000 );
    optional( jo, was_loaded, "mass_ratio", mass_ratio, 0.00f );
    optional( jo, was_loaded, "flags", flags );
    optional( jo, was_loaded, "faults", faults );
}

void harvest_entry::deserialize( JsonIn &jsin )
{
    const JsonObject &jo = jsin.get_object();
    load( jo );
}

void harvest_list::finalize()
{
    std::transform( entries_.begin(), entries_.end(), std::inserter( names_, names_.begin() ),
    []( const harvest_entry & entry ) {
        return item::type_is_defined( itype_id( entry.drop ) ) ?
               item::nname( itype_id( entry.drop ) ) : "";
    } );
}

void harvest_list::finalize_all()
{
    for( const harvest_list &pr : get_all() ) {
        const_cast<harvest_list &>( pr ).finalize();
    }
}

void harvest_list::load( const JsonObject &obj, const std::string & )
{
    mandatory( obj, was_loaded, "id", id );
    mandatory( obj, was_loaded, "entries", entries_ );

    optional( obj, was_loaded, "butchery_requirements", butchery_requirements_,
              butchery_requirements_id( "default" ) );
    optional( obj, was_loaded, "message", message_ );
    optional( obj, was_loaded, "leftovers", leftovers, itype_id( "ruined_chunks" ) );
}

void harvest_list::load_harvest_list( const JsonObject &jo, const std::string &src )
{
    harvest_list_factory.load( jo, src );
}

const std::vector<harvest_list> &harvest_list::get_all()
{
    return harvest_list_factory.get_all();
}

void harvest_list::check_consistency()
{
    for( const harvest_list &hl : get_all() ) {
        const std::string hl_id = hl.id.c_str();
        auto error_func = [&]( const harvest_entry & entry ) {
            std::string errorlist;
            bool item_valid = true;
            if( !( item::type_is_defined( itype_id( entry.drop ) ) ||
                   ( entry.type == "bionic_group" &&
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
                              entry.type == "blood" ||
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
        if( !hl.leftovers.is_valid() ) {
            debugmsg( "Harvest id %s has invalid leftovers: %s", hl.id.c_str(), hl.leftovers.c_str() );
        }
    }
}

void harvest_list::reset()
{
    harvest_list_factory.reset();
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
        ss += "<bold>" + item::nname( itype_id( en.drop ), max_drops ) + "</bold>";
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
