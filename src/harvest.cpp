#include "harvest.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <optional>
#include <string>

#include "debug.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "output.h"
#include "string_formatter.h"
#include "text_snippets.h"

static const butchery_requirements_id butchery_requirements_default( "default" );

static const itype_id itype_ruined_chunks( "ruined_chunks" );

static const skill_id skill_survival( "survival" );

namespace
{
generic_factory<harvest_drop_type> harvest_drop_type_factory( "harvest_drop_type" );
generic_factory<harvest_list> harvest_list_factory( "harvest_list" );
} //namespace

/** @relates string_id */
template<>
const harvest_drop_type &string_id<harvest_drop_type>::obj() const
{
    return harvest_drop_type_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<harvest_drop_type>::is_valid() const
{
    return harvest_drop_type_factory.is_valid( *this );
}

translation harvest_drop_type::field_dress_msg( bool succeeded ) const
{
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    return SNIPPET.random_from_category(
               succeeded ? msg_fielddress_success : msg_fielddress_fail ).value_or( translation() );
}

translation harvest_drop_type::butcher_msg( bool succeeded ) const
{
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    return SNIPPET.random_from_category(
               succeeded ? msg_butcher_success : msg_butcher_fail ).value_or( translation() );
}

translation harvest_drop_type::dissect_msg( bool succeeded ) const
{
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    return SNIPPET.random_from_category(
               succeeded ? msg_dissect_success : msg_dissect_fail ).value_or( translation() );
}

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

bool harvest_list::has_entry_type( const harvest_drop_type_id &type ) const
{
    for( const harvest_entry &entry : entries() ) {
        if( entry.type == type ) {
            return true;
        }
    }
    return false;
}

void harvest_drop_type::load_harvest_drop_types( const JsonObject &jo, const std::string &src )
{
    harvest_drop_type_factory.load( jo, src );
}

void harvest_drop_type::reset()
{
    harvest_drop_type_factory.reset();
}

void harvest_drop_type::load( const JsonObject &jo, std::string_view )
{
    harvest_skills.clear();
    optional( jo, was_loaded, "group", is_group_, false );
    optional( jo, was_loaded, "dissect_only", dissect_only_, false );
    optional( jo, was_loaded, "msg_fielddress_fail", msg_fielddress_fail, "" );
    optional( jo, was_loaded, "msg_fielddress_success", msg_fielddress_success, "" );
    optional( jo, was_loaded, "msg_butcher_fail", msg_butcher_fail, "" );
    optional( jo, was_loaded, "msg_butcher_success", msg_butcher_success, "" );
    optional( jo, was_loaded, "msg_dissect_fail", msg_dissect_fail, "" );
    optional( jo, was_loaded, "msg_dissect_success", msg_dissect_success, "" );
    if( jo.has_string( "harvest_skills" ) ) {
        skill_id sk;
        mandatory( jo, was_loaded, "harvest_skills", sk );
        harvest_skills.emplace_back( sk );
    } else if( jo.has_member( "harvest_skills" ) ) {
        optional( jo, was_loaded, "harvest_skills", harvest_skills );
    } else {
        harvest_skills.emplace_back( skill_survival );
    }
}

const std::vector<harvest_drop_type> &harvest_drop_type::get_all()
{
    return harvest_drop_type_factory.get_all();
}

void harvest_entry::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "drop", drop );

    optional( jo, was_loaded, "type", type, harvest_drop_type_id::NULL_ID() );
    optional( jo, was_loaded, "base_num", base_num, { 1.0f, 1.0f } );
    optional( jo, was_loaded, "scale_num", scale_num, { 0.0f, 0.0f } );
    optional( jo, was_loaded, "max", max, 1000 );
    optional( jo, was_loaded, "mass_ratio", mass_ratio, 0.00f );
    optional( jo, was_loaded, "flags", flags );
    optional( jo, was_loaded, "faults", faults );
}

void harvest_entry::deserialize( const JsonObject &jo )
{
    load( jo );
}

bool harvest_entry::operator==( const harvest_entry &rhs ) const
{
    return drop == rhs.drop;
}

class harvest_entry_reader : public generic_typed_reader<harvest_entry_reader>
{
    public:
        harvest_entry get_next( const JsonValue &jv ) const {
            JsonObject jo = jv.get_object();
            harvest_entry ret;
            mandatory( jo, false, "drop", ret.drop );

            optional( jo, false, "type", ret.type, harvest_drop_type_id::NULL_ID() );
            optional( jo, false, "base_num", ret.base_num, { 1.0f, 1.0f } );
            optional( jo, false, "scale_num", ret.scale_num, { 0.0f, 0.0f } );
            optional( jo, false, "max", ret.max, 1000 );
            optional( jo, false, "mass_ratio", ret.mass_ratio, 0.00f );
            optional( jo, false, "flags", ret.flags );
            optional( jo, false, "faults", ret.faults );
            return ret;
        }
};

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
    harvest_list_factory.finalize();
    for( const harvest_list &pr : get_all() ) {
        const_cast<harvest_list &>( pr ).finalize();
    }
}

void harvest_list::load( const JsonObject &obj, std::string_view )
{
    mandatory( obj, was_loaded, "id", id );
    mandatory( obj, was_loaded, "entries", entries_, harvest_entry_reader{} );

    optional( obj, was_loaded, "butchery_requirements", butchery_requirements_,
              butchery_requirements_default );
    optional( obj, was_loaded, "message", message_ );
    optional( obj, was_loaded, "leftovers", leftovers, itype_ruined_chunks );
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
            if( entry.type.is_null() ) {
                // Type id is null
                if( item::type_is_defined( itype_id( entry.drop ) ) ) {
                    return errorlist;
                }
                errorlist += "null type";
            } else if( !entry.type.is_valid() ) {
                // Type id is invalid
                errorlist += "invalid type \"" + entry.type.str() + "\"";
            } else if( ( entry.type->is_item_group() &&
                         !item_group::group_is_defined( item_group_id( entry.drop ) ) ) ||
                       ( !entry.type->is_item_group() && !item::type_is_defined( itype_id( entry.drop ) ) ) ) {
                // Specified as item_group but no such group exists, or
                // specified as single itype but no such itype exists
                errorlist += entry.drop;
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

std::vector<harvest_entry>::const_iterator harvest_list::begin() const
{
    return entries().begin();
}

std::vector<harvest_entry>::const_iterator harvest_list::end() const
{
    return entries().end();
}

std::vector<harvest_entry>::const_reverse_iterator harvest_list::rbegin() const
{
    return entries().rbegin();
}

std::vector<harvest_entry>::const_reverse_iterator harvest_list::rend() const
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
