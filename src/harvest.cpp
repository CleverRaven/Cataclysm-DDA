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
#include "string_id.h"
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

void harvest_entry::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "drop", drop );

    optional( jo, was_loaded, "type", type );
    optional( jo, was_loaded, "base_num", base_num );
    optional( jo, was_loaded, "scale_num", scale_num );
    optional( jo, was_loaded, "max", max );
    optional( jo, was_loaded, "mass_ratio", mass_ratio );
    optional( jo, was_loaded, "flags", flags );
    optional( jo, was_loaded, "faults", faults );
    optional( jo, was_loaded, "butchery_requirements", butchery_requirements_,
              butchery_requirements_id( "default" ) );
}

void harvest_entry::deserialize( JsonIn &jsin )
{
    const JsonObject &jo = jsin.get_object();
    load( jo );
}

const harvest_id &harvest_list::load( const JsonObject &jo, const std::string &src,
                                      const std::string &force_id )
{
    harvest_list ret;
    if( jo.has_string( "id" ) ) {
        ret.id = harvest_id( jo.get_string( "id" ) );
    } else if( !force_id.empty() ) {
        ret.id = harvest_id( force_id );
    } else {
        jo.throw_error( "id was not specified for harvest" );
    }

    jo.read( "message", ret.message_ );
    jo.read( "entries", ret.entries_ );

    assign( jo, "leftovers", ret.leftovers );

    for( const JsonObject current_entry : jo.get_array( "entries" ) ) {
        ret.entries_.push_back( harvest_entry::load( current_entry, src ) );
    }

    if( !jo.read( "butchery_requirements", ret.butchery_requirements_ ) ) {
        ret.butchery_requirements_ = butchery_requirements_id( "default" );
    }

    auto &new_entry = harvest_all[ ret.id_ ];
    new_entry = ret;

    return ret.id;
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

void harvest_list::load( const JsonObject &obj )
{
    mandatory( obj, was_loaded, "id", id );
    mandatory( obj, was_loaded, "message", message_ );
    mandatory( obj, was_loaded, "entries", entries_ );

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
        if( !pr.first->leftovers.is_valid() ) {
            debugmsg( "Harvest id %s has invalid leftovers: %s", pr.first.c_str(),
                      pr.first->leftovers.c_str() );
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
