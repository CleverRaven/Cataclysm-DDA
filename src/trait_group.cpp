#include "trait_group.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <map>
#include <utility>

#include "compatibility.h"
#include "debug.h"
#include "json.h"
#include "mutation.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"

using namespace trait_group;

Trait_list trait_group::traits_from( const Trait_group_tag &gid )
{
    return mutation_branch::get_group( gid )->create();
}

bool trait_group::group_contains_trait( const Trait_group_tag &gid, const trait_id &tid )
{
    return mutation_branch::get_group( gid )->has_trait( tid );
}

void trait_group::load_trait_group( const JsonObject &jsobj, const Trait_group_tag &gid,
                                    const std::string &subtype )
{
    mutation_branch::load_trait_group( jsobj, gid, subtype );
}

// NOTE: This function is largely based on item_group::get_unique_group_id()
static Trait_group_tag get_unique_trait_group_id()
{
    // This is just a hint what id to use next. Overflow of it is defined and if the group
    // name is already used, we simply go the next id.
    static unsigned int next_id = 0;
    // Prefixing with a character outside the ASCII range, so it is hopefully unique and
    // (if actually printed somewhere) stands out. Theoretically those auto-generated group
    // names should not be seen anywhere.
    static const std::string unique_prefix = "\u01F7 ";
    while( true ) {
        const Trait_group_tag new_group( unique_prefix + to_string( next_id++ ) );
        if( !new_group.is_valid() ) {
            return new_group;
        }
    }
}

Trait_group_tag trait_group::load_trait_group( const JsonValue &value,
        const std::string &default_subtype )
{
    if( value.test_string() ) {
        return Trait_group_tag( value.get_string() );
    } else if( value.test_object() ) {
        const Trait_group_tag group = get_unique_trait_group_id();

        JsonObject jo = value.get_object();
        const std::string subtype = jo.get_string( "subtype", default_subtype );

        mutation_branch::load_trait_group( jo, group, subtype );

        return group;
    } else if( value.test_array() ) {
        const Trait_group_tag group = get_unique_trait_group_id();

        if( default_subtype != "collection" && default_subtype != "distribution" ) {
            value.throw_error( "invalid subtype for trait group" );
        }

        mutation_branch::load_trait_group( value.get_array(), group, default_subtype == "collection" );
        return group;
    } else {
        value.throw_error( "invalid trait group, must be string (group id) or object/array (the group data)" );
        return Trait_group_tag{};
    }
}

// NOTE: This is largely based on item_group::debug_spawn()
void trait_group::debug_spawn()
{
    std::vector<Trait_group_tag> groups = mutation_branch::get_all_group_names();
    uilist menu;
    menu.text = _( "Test which group?" );
    for( size_t i = 0; i < groups.size(); i++ ) {
        menu.entries.emplace_back( static_cast<int>( i ), true, -2, groups[i].str() );
    }
    while( true ) {
        menu.query();
        const int index = menu.ret;
        if( index >= static_cast<int>( groups.size() ) || index < 0 ) {
            break;
        }
        // Spawn traits from the group 100 times
        std::map<std::string, int> traitnames;
        for( size_t a = 0; a < 100; a++ ) {
            const auto traits = traits_from( groups[index] );
            for( auto &tr : traits ) {
                traitnames[mutation_branch::get_name( tr )]++;
            }
        }
        // Invert the map to get sorting!
        std::multimap<int, std::string> traitnames2;
        for( const auto &e : traitnames ) {
            traitnames2.insert( std::pair<int, std::string>( e.second, e.first ) );
        }
        uilist menu2;
        menu2.text = _( "Result of 100 spawns:" );
        for( const auto &e : traitnames2 ) {
            menu2.entries.emplace_back( static_cast<int>( menu2.entries.size() ), true, -2,
                                        string_format( _( "%d x %s" ), e.first, e.second ) );
        }
        menu2.query();
    }
}

Trait_list Trait_creation_data::create() const
{
    RecursionList rec;
    auto result = create( rec );
    return result;
}

Trait_group::Trait_group( int probability )
    : Trait_creation_data( probability ), sum_prob( 0 )
{
}

Single_trait_creator::Single_trait_creator( const trait_id &id, int probability )
    : Trait_creation_data( probability ), id( id )
{
}

Trait_list Single_trait_creator::create( RecursionList & /* rec */ ) const
{
    return Trait_list { id };
}

void Single_trait_creator::check_consistency() const
{
    if( !id.is_valid() ) {
        debugmsg( "trait id %s is unknown", id.c_str() );
    }
}

bool Single_trait_creator::remove_trait( const trait_id &tid )
{
    // Return true if I would be empty after "removing" the given trait, thus
    // signaling to my parent that I should be removed.
    return tid == id;
}

bool Single_trait_creator::has_trait( const trait_id &tid ) const
{
    return tid == id;
}

Trait_group_creator::Trait_group_creator( const Trait_group_tag &id, int probability )
    : Trait_creation_data( probability ), id( id )
{
}

Trait_list Trait_group_creator::create( RecursionList &rec ) const
{

    Trait_list result;
    if( std::find( rec.begin(), rec.end(), id ) != rec.end() ) {
        debugmsg( "recursion in trait creation list %s", id.c_str() );
        return result;
    }
    rec.push_back( id );

    if( !id.is_valid() ) {
        debugmsg( "unknown trait creation list %s", id.c_str() );
        return result;
    }
    const auto tcd = mutation_branch::get_group( id );

    Trait_list tmplist = tcd->create( rec );
    rec.pop_back();
    result.insert( result.end(), tmplist.begin(), tmplist.end() );

    return result;
}

void Trait_group_creator::check_consistency() const
{
    if( id.is_valid() ) {
        debugmsg( "trait group id %s is unknown", id.c_str() );
    }
}

bool Trait_group_creator::remove_trait( const trait_id &tid )
{
    return mutation_branch::get_group( id )->remove_trait( tid );
}

bool Trait_group_creator::has_trait( const trait_id &tid ) const
{
    return mutation_branch::get_group( id )->has_trait( tid );
}

void Trait_group::add_trait_entry( const trait_id &tid, int probability )
{
    add_entry( std::make_unique<Single_trait_creator>( tid, probability ) );
}

void Trait_group::add_group_entry( const Trait_group_tag &gid, int probability )
{
    add_entry( std::make_unique<Trait_group_creator>( gid, probability ) );
}

void Trait_group::check_consistency() const
{
    for( const auto &creator : creators ) {
        creator->check_consistency();
    }
}

bool Trait_group::remove_trait( const trait_id &tid )
{
    for( auto it = creators.begin(); it != creators.end(); ) {
        if( ( *it )->remove_trait( tid ) ) {
            sum_prob -= ( *it )->probability;
            it = creators.erase( it );
        } else {
            ++it;
        }
    }
    return creators.empty();
}

bool Trait_group::has_trait( const trait_id &tid ) const
{
    for( const auto &creator : creators ) {
        if( creator->has_trait( tid ) ) {
            return true;
        }
    }
    return false;
}

Trait_group_collection::Trait_group_collection( int probability )
    : Trait_group( probability )
{
    if( probability <= 0 || probability > 100 ) {
        debugmsg( "Probability %d out of range", probability );
    }
}

Trait_list Trait_group_collection::create( RecursionList &rec ) const
{
    Trait_list result;
    for( const auto &creator : creators ) {
        if( rng( 0, 99 ) >= ( creator )->probability ) {
            continue;
        }
        Trait_list tmp = ( creator )->create( rec );
        result.insert( result.end(), tmp.begin(), tmp.end() );
    }
    return result;
}

void Trait_group_collection::add_entry( std::unique_ptr<Trait_creation_data> ptr )
{
    assert( ptr.get() != nullptr );
    if( ptr->probability <= 0 ) {
        return;
    }

    ptr->probability = std::min( 100, ptr->probability );

    creators.push_back( std::move( ptr ) );
    ptr.release();
}

void Trait_group_distribution::add_entry( std::unique_ptr<Trait_creation_data> ptr )
{
    assert( ptr.get() != nullptr );
    if( ptr->probability <= 0 ) {
        return;
    }

    sum_prob += ptr->probability;

    creators.push_back( std::move( ptr ) );
    ptr.release();
}

Trait_list Trait_group_distribution::create( RecursionList &rec ) const
{
    Trait_list result;
    int p = rng( 0, sum_prob - 1 );
    for( const auto &creator : creators ) {
        p -= ( creator )->probability;
        if( p >= 0 ) {
            continue;
        }
        Trait_list tmp = ( creator )->create( rec );
        result.insert( result.end(), tmp.begin(), tmp.end() );
        break;
    }
    return result;
}
