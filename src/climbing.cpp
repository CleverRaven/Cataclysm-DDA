#include <cstdlib>
#include <utility>

#include "climbing.h"

#include "cata_assert.h"
#include "enum_conversions.h"
#include "enums.h"
#include "generic_factory.h"
#include "json.h"
#include "string_formatter.h"


namespace
{
generic_factory<climbing_aid> climbing_aid_factory( "climbing_aid" );
} // namespace


static climbing_aid::lookup climbing_lookup;

static const climbing_aid *climbing_aid_default = nullptr;


const std::vector<climbing_aid> &climbing_aid::get_all()
{
    return climbing_aid_factory.get_all();
}

void climbing_aid::load_climbing_aid( const JsonObject &jo, const std::string &src )
{
    climbing_aid_factory.load( jo, src );
}

void climbing_aid::finalize()
{
    // Build the climbing aids by condition lookup table
    climbing_aid_default = nullptr;
    climbing_lookup.clear();
    climbing_lookup.resize( int( category::last ) );
    for( const climbing_aid &aid : get_all() ) {
        int index = int( aid.base_condition.cat );
        if( climbing_lookup.size() < index ) {
            return;
        }
        climbing_lookup[ index ].emplace( aid.base_condition.flag, &aid );

        if( aid.id.str() == "default" ) {
            climbing_aid_default = &aid;
        }
    }

    if( !climbing_aid_default ) {
        // Force-generate the default climbing aid.
        static climbing_aid def;
        def.id = climbing_aid_id( "default" );
        def.slip_chance_mod = 0;
        def.base_condition.cat = category::special;
        def.down.menu_text = "Climb down by lowering yourself from the ledge.";
        def.down.menu_hotkey = 'c';
        def.down.confirm_text = "Climb down the ledge?";
        def.down.msg_after = "You lower yourself from the ledge.";
        def.down.max_height = 1;
        def.was_loaded = false;
        def.down.was_loaded = true;
    }
}

void climbing_aid::check_consistency()
{
    if( !climbing_aid_default || !climbing_aid_default->was_loaded ) {
        debugmsg( "Default climbing aid was not defined." );
    }
}

void climbing_aid::reset()
{
    climbing_aid_factory.reset();
    climbing_lookup.clear();
    climbing_aid_default = nullptr;
}

void climbing_aid::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "slip_chance_mod", slip_chance_mod );
    mandatory( jo, was_loaded, "down", down );
    mandatory( jo, was_loaded, "condition", base_condition );

    was_loaded = true;
}

template<>
struct enum_traits<climbing_aid::category> {
    static constexpr climbing_aid::category last = climbing_aid::category::last;
};
namespace io
{
template<>
std::string enum_to_string<climbing_aid::category>( climbing_aid::category data )
{
    switch( data ) {
        // *INDENT-OFF*
    case climbing_aid::category::special: return "special";
    case climbing_aid::category::ter_furn: return "ter_furn";
    case climbing_aid::category::veh: return "veh";
    case climbing_aid::category::item: return "item";
    case climbing_aid::category::character: return "character";
    case climbing_aid::category::mutation: return "mutation";
        // *INDENT-ON*
        case climbing_aid::category::last:
            break;
    }
    cata_fatal( "Invalid achievement_completion" );
}
}

void climbing_aid::condition::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "type", cat );
    mandatory( jo, false, "flag", flag );
    switch( cat ) {
        case category::item: {
            mandatory( jo, false, "uses", uses_item );
            break;
        }
        case category::ter_furn: {
            optional( jo, false, "range", range );
            break;
        }
        default: {
            break;
        }
    }

}

void climbing_aid::down_t::deserialize( const JsonObject &jo )
{
    // max_height is optional; setting it to zero disables "down" motion
    optional( jo, true, "max_height", max_height );

    if( enabled() ) {
        // Mandatory for all aids that
        mandatory( jo, was_loaded, "menu_text", menu_text );
        mandatory( jo, was_loaded, "confirm_text", confirm_text );
        std::string menu_hotkey_str;
        if( deploys_furniture() ) {
            mandatory( jo, was_loaded, "menu_cant", menu_cant );
            mandatory( jo, was_loaded, "menu_hotkey", menu_hotkey_str );
            if( menu_hotkey_str.length() != 1 ) {
                jo.throw_error( str_cat( "failed to read mandatory member \"menu_hotkey\"" ) );
            }
        } else {
            optional( jo, was_loaded, "menu_cant", menu_cant );
            optional( jo, was_loaded, "menu_hotkey", menu_hotkey_str );
            if( menu_hotkey_str.length() > 1 ) {
                jo.throw_error( str_cat( "failed to read optional member \"menu_hotkey\"" ) );
            }
        }
        if( menu_hotkey_str.length() ) {
            menu_hotkey = menu_hotkey_str[ 0 ];
        }

        optional( jo, true, "msg_before", msg_before );
        optional( jo, true, "msg_after", msg_after );
        optional( jo, true, "cost", cost );
        optional( jo, true, "deploy_furn", deploy_furn );
    }

    was_loaded = true;
}

void climbing_aid::climb_cost::deserialize( const JsonObject &jo )
{
    optional( jo, true, "kcal", kcal );
    optional( jo, true, "thirst", thirst );
    optional( jo, true, "damage", damage );
    optional( jo, true, "pain", pain );
}

template< typename Lambda >
void for_each_aid_condition( const climbing_aid::condition_list &conditions, const Lambda &func )
{
    for( auto &cond : conditions ) {
        if( int( cond.cat ) < climbing_lookup.size() ) {
            auto &cat = climbing_lookup[ int( cond.cat ) ];
            for( auto i = cat.lower_bound( cond.flag ), e = cat.upper_bound( cond.flag ); i != e; ++i ) {
                func( cond, *i->second );
            }
        }
    }
}

climbing_aid::aid_list climbing_aid::list(
    const condition_list &conditions )
{
    std::vector<const climbing_aid *> list;

    const auto add_deployables = [&]( const condition & cond, const climbing_aid & aid ) {
        if( aid.down.deploys_furniture() ) {
            list.push_back( &aid );
        }
    };
    for_each_aid_condition( conditions, add_deployables );

    list.push_back( &get_safest( conditions, true ) );

    return list;
}

climbing_aid::aid_list climbing_aid::list_all(
    const condition_list &conditions )
{
    std::vector<const climbing_aid *> list;

    const auto add_climbing_aids = [&]( const condition & cond, const climbing_aid & aid ) {
        list.push_back( &aid );
    };
    for_each_aid_condition( conditions, add_climbing_aids );

    list.push_back( climbing_aid_default );

    return list;
}

const climbing_aid &climbing_aid::get_safest(
    const condition_list &conditions, bool no_deploy )
{
    const climbing_aid *choice = climbing_aid_default;

    const auto choose_safest = [&]( const condition & cond, const climbing_aid & aid ) {
        if( !no_deploy || aid.down.deploy_furn.is_empty() ) {
            if( aid.slip_chance_mod < choice->slip_chance_mod ) {
                choice = &aid;
            }
        }
    };
    for_each_aid_condition( conditions, choose_safest );

    return *choice;
}

const climbing_aid &climbing_aid::get_default()
{
    return *climbing_aid_default;
}
