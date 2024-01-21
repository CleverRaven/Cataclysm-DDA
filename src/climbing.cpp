#include <cstdlib>
#include <unordered_set>
#include <utility>

#include "climbing.h"

#include "cata_assert.h"
#include "character.h"
#include "creature_tracker.h"
#include "enum_conversions.h"
#include "enums.h"
#include "game.h"
#include "generic_factory.h"
#include "int_id.h"
#include "json.h"
#include "map.h"
#include "string_formatter.h"
#include "vehicle.h"
#include "vpart_position.h"


static const climbing_aid_id climbing_aid_default( "default" );


namespace
{
generic_factory<climbing_aid> climbing_aid_factory( "climbing_aid" );
} // namespace

static climbing_aid::lookup climbing_lookup;

static const climbing_aid *climbing_aid_default_ptr = nullptr;



template<>
const climbing_aid &string_id<climbing_aid>::obj() const
{
    return climbing_aid_factory.obj( *this );
}
template<>
const climbing_aid &int_id<climbing_aid>::obj() const
{
    return climbing_aid_factory.obj( *this );
}

template<>
int_id<climbing_aid> string_id<climbing_aid>::id() const
{
    return climbing_aid_factory.convert( *this, int_id<climbing_aid>( -1 ) );
}
template<>
const string_id<climbing_aid> &int_id<climbing_aid>::id() const
{
    return climbing_aid_factory.convert( *this );
}

template<>
bool int_id<climbing_aid>::is_valid() const
{
    return climbing_aid_factory.is_valid( *this );
}
template<>
bool string_id<climbing_aid>::is_valid() const
{
    return climbing_aid_factory.is_valid( *this );
}

template<>
int_id<climbing_aid>::int_id( const string_id<climbing_aid> &id ) : _id( id.id() )
{
}

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
    climbing_aid_default_ptr = nullptr;
    climbing_lookup.clear();
    climbing_lookup.resize( int( category::last ) );

    for( const climbing_aid &aid : get_all() ) {
        int category_index = int( aid.base_condition.cat );
        if( category_index >= int( climbing_lookup.size() ) ) {
            debugmsg( "Climbing aid %s has invalid condition type.", aid.id.str() );
            continue;
        }
        climbing_lookup[ category_index ].emplace( aid.base_condition.flag, &aid );

        if( aid.id.str() == "default" ) {
            climbing_aid_default_ptr = &aid;
        }
    }

    if( !climbing_aid_default_ptr ) {
        // Force-generate the default climbing aid.
        static climbing_aid def;
        def.id = climbing_aid_default;
        def.slip_chance_mod = 0;
        def.base_condition.cat = category::special;
        def.down.menu_text = to_translation( "Climb down by lowering yourself from the ledge." );
        def.down.menu_hotkey = 'c';
        def.down.confirm_text = to_translation( "Climb down the ledge?" );
        def.down.msg_after = to_translation( "You lower yourself from the ledge." );
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
    climbing_aid_default_ptr = nullptr;
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
    case climbing_aid::category::trait: return "trait";
        // *INDENT-ON*
        case climbing_aid::category::last:
            break;
    }
    cata_fatal( "Invalid climbing aid condition category" );
}
} // namespace io

std::string climbing_aid::condition::category_string() const noexcept
{
    return io::enum_to_string( cat );
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
    optional( jo, true, "easy_climb_back_up", easy_climb_back_up );
    optional( jo, true, "allow_remaining_height", allow_remaining_height );

    if( enabled() ) {
        // Mechanics.  Deploying changes presentation of menus.
        optional( jo, true, "cost", cost );
        optional( jo, true, "deploy_furn", deploy_furn );

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
            menu_hotkey = std::uint8_t( menu_hotkey_str[ 0 ] );
        }

        // Messages show when actually climbing.
        optional( jo, true, "msg_before", msg_before );
        optional( jo, true, "msg_after", msg_after );
    }
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
    for( const climbing_aid::condition &cond : conditions ) {
        if( int( cond.cat ) < int( climbing_lookup.size() ) ) {
            auto &cat = climbing_lookup[ int( cond.cat ) ];
            auto range = cat.equal_range( cond.flag );
            for( auto i = range.first; i != range.second; ++i ) {
                func( cond, *i->second );
            }
        }
    }
}

climbing_aid::aid_list climbing_aid::list(
    const condition_list &conditions )
{
    std::vector<const climbing_aid *> list;

    const auto add_deployables = [&list]( const condition &, const climbing_aid & aid ) {
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

    const auto add_climbing_aids = [&list]( const condition &, const climbing_aid & aid ) {
        list.push_back( &aid );
    };
    for_each_aid_condition( conditions, add_climbing_aids );

    list.push_back( climbing_aid_default_ptr );

    return list;
}

const climbing_aid &climbing_aid::get_safest(
    const condition_list &conditions, bool no_deploy )
{
    const climbing_aid *choice = climbing_aid_default_ptr;

    const auto choose_safest = [&choice, no_deploy]( const condition &, const climbing_aid & aid ) {
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
    return *climbing_aid_default_ptr;
}

template< typename Test >
static void detect_conditions_sub( climbing_aid::condition_list &list,
                                   climbing_aid::category category, const Test &test )
{
    std::string empty;
    const std::string *flag_already_tested = &empty;
    for( auto &[ flag, aid ] : climbing_lookup[ int( category ) ] ) {
        // climbing_lookup is a multimap, don't test flags twice.
        if( flag == *flag_already_tested ) {
            continue;
        }

        climbing_aid::condition cond = { category, flag };
        if( test( cond ) ) {
            list.emplace_back( cond );
        }

        flag_already_tested = &flag;
    }
}

climbing_aid::condition_list climbing_aid::detect_conditions( Character &you,
        const tripoint &examp )
{
    condition_list list;

    map &here = get_map();

    fall_scan fall( examp );

    auto detect_you_character_flag = [&you]( condition & cond ) {
        return you.has_flag( json_character_flag( cond.flag ) );
    };
    auto detect_you_trait = [&you]( condition & cond ) {
        return you.has_trait( trait_id( cond.flag ) );
    };
    auto detect_item = [&you]( condition & cond ) {
        cond.uses_item = you.amount_of( itype_id( cond.flag ) );
        return cond.uses_item > 0;
    };
    auto detect_ter_furn_flag = [&here, &fall]( condition & cond ) {
        tripoint pos = fall.pos_furniture_or_floor();
        cond.range = fall.pos_top().z - pos.z;
        return here.has_flag( cond.flag, pos );
    };
    auto detect_vehicle = [&fall]( condition & cond ) {
        // TODO implement flags and range?
        cond.range = 1;
        return fall.veh_just_below();
    };

    detect_conditions_sub( list, category::character, detect_you_character_flag );
    detect_conditions_sub( list, category::trait, detect_you_trait );
    detect_conditions_sub( list, category::item, detect_item );
    detect_conditions_sub( list, category::ter_furn, detect_ter_furn_flag );
    detect_conditions_sub( list, category::veh, detect_vehicle );

    return list;
}

climbing_aid::fall_scan::fall_scan( const tripoint &examp )
{
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();

    this->examp = examp;
    height = 0;
    height_until_furniture = 0;
    height_until_creature = 0;
    height_until_vehicle = 0;

    // Get coordinates just below and at ground level.
    // Also detect if furniture would block our tools/abilities.
    tripoint bottom = examp;
    tripoint just_below = examp;
    just_below.z--;

    int hit_furn = false;
    int hit_crea = false;
    int hit_veh = false;

    for( tripoint lower = just_below; here.valid_move( bottom, lower, false, true ); ) {
        if( !hit_furn ) {
            if( here.has_furn( lower ) ) {
                hit_furn = true;
            } else {
                ++height_until_furniture;
            }
        }
        if( !hit_veh ) {
            if( here.veh_at( lower ) ) {
                hit_veh = true;
            } else {
                ++height_until_vehicle;
            }
        }
        if( !hit_crea ) {
            ++height_until_creature;
            if( creatures.creature_at( lower, false ) ) {
                hit_crea = true;
            } else {
                ++height_until_creature;
            }
        }
        ++height;
        bottom.z--;
        lower.z--;
    }
}
