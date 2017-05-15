#include "npc_class.h"
#include "skill.h"
#include "debug.h"
#include "rng.h"
#include "generic_factory.h"
#include "item_group.h"
#include "mutation.h"

#include <list>

static const std::array<npc_class_id, 17> legacy_ids = {{
        npc_class_id( "NC_NONE" ),
        npc_class_id( "NC_EVAC_SHOPKEEP" ),  // Found in the Evacuation Center, unique, has more goods than he should be able to carry
        npc_class_id( "NC_SHOPKEEP" ),       // Found in towns.  Stays in his shop mostly.
        npc_class_id( "NC_HACKER" ),         // Weak in combat but has hacking skills and equipment
        npc_class_id( "NC_DOCTOR" ),         // Found in towns, or roaming.  Stays in the clinic.
        npc_class_id( "NC_TRADER" ),         // Roaming trader, journeying between towns.
        npc_class_id( "NC_NINJA" ),          // Specializes in unarmed combat, carries few items
        npc_class_id( "NC_COWBOY" ),         // Gunslinger and survivalist
        npc_class_id( "NC_SCIENTIST" ),      // Uses intelligence-based skills and high-tech items
        npc_class_id( "NC_BOUNTY_HUNTER" ),  // Resourceful and well-armored
        npc_class_id( "NC_THUG" ),           // Moderate melee skills and poor equipment
        npc_class_id( "NC_SCAVENGER" ),      // Good with pistols light weapons
        npc_class_id( "NC_ARSONIST" ),       // Evacuation Center, restocks moltovs and anarcist type stuff
        npc_class_id( "NC_HUNTER" ),         // Survivor type good with bow or rifle
        npc_class_id( "NC_SOLDIER" ),        // Well equiped and trained combatant, good with rifles and melee
        npc_class_id( "NC_BARTENDER" ),      // Stocks alcohol
        npc_class_id( "NC_JUNK_SHOPKEEP" )   // Stocks wide range of items...
    }
};

npc_class_id NC_NONE( "NC_NONE" );
npc_class_id NC_EVAC_SHOPKEEP( "NC_EVAC_SHOPKEEP" );
npc_class_id NC_SHOPKEEP( "NC_SHOPKEEP" );
npc_class_id NC_HACKER( "NC_HACKER" );
npc_class_id NC_DOCTOR( "NC_DOCTOR" );
npc_class_id NC_TRADER( "NC_TRADER" );
npc_class_id NC_NINJA( "NC_NINJA" );
npc_class_id NC_COWBOY( "NC_COWBOY" );
npc_class_id NC_SCIENTIST( "NC_SCIENTIST" );
npc_class_id NC_BOUNTY_HUNTER( "NC_BOUNTY_HUNTER" );
npc_class_id NC_THUG( "NC_THUG" );
npc_class_id NC_SCAVENGER( "NC_SCAVENGER" );
npc_class_id NC_ARSONIST( "NC_ARSONIST" );
npc_class_id NC_HUNTER( "NC_HUNTER" );
npc_class_id NC_SOLDIER( "NC_SOLDIER" );
npc_class_id NC_BARTENDER( "NC_BARTENDER" );
npc_class_id NC_JUNK_SHOPKEEP( "NC_JUNK_SHOPKEEP" );

generic_factory<npc_class> npc_class_factory( "npc_class" );

/** @relates string_id */
template<>
const npc_class_id string_id<npc_class>::NULL_ID( "NC_NONE" );

/** @relates string_id */
template<>
const npc_class &string_id<npc_class>::obj() const
{
    return npc_class_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<npc_class>::is_valid() const
{
    return npc_class_factory.is_valid( *this );
}

npc_class::npc_class() : id( NC_NONE )
{
}

void npc_class::load_npc_class( JsonObject &jo, const std::string &src )
{
    npc_class_factory.load( jo, src );
}

void npc_class::reset_npc_classes()
{
    npc_class_factory.reset();
}

// Copies the value under the key "ALL" to all unassigned skills
template <typename T>
void apply_all_to_unassigned( T &skills )
{
    auto iter = std::find_if( skills.begin(), skills.end(),
    []( decltype( *begin( skills ) ) &pr ) {
        return pr.first == "ALL";
    } );

    if( iter != skills.end() ) {
        distribution dis = iter->second;
        skills.erase( iter );
        for( const auto &sk : Skill::skills ) {
            if( skills.count( sk.ident() ) == 0 ) {
                skills[ sk.ident() ] = dis;
            }
        }
    }
}

void npc_class::finalize_all()
{
    for( auto &cl_const : npc_class_factory.get_all() ) {
        auto &cl = const_cast<npc_class &>( cl_const );
        apply_all_to_unassigned( cl.skills );
        apply_all_to_unassigned( cl.bonus_skills );

        for( const auto &pr : cl.bonus_skills ) {
            if( cl.skills.count( pr.first ) == 0 ) {
                cl.skills[ pr.first ] = pr.second;
            } else {
                cl.skills[ pr.first ] = cl.skills[ pr.first ] + pr.second;
            }
        }
    }
}

void npc_class::check_consistency()
{
    for( const auto &legacy : legacy_ids ) {
        if( !npc_class_factory.is_valid( legacy ) ) {
            debugmsg( "Missing legacy npc class %s", legacy.c_str() );
        }
    }

    for( auto &cl : npc_class_factory.get_all() ) {
        if( !item_group::group_is_defined( cl.shopkeeper_item_group ) ) {
            debugmsg( "Missing shopkeeper item group %s", cl.shopkeeper_item_group.c_str() );
        }

        if( !cl.worn_override.empty() && !item_group::group_is_defined( cl.worn_override ) ) {
            debugmsg( "Missing worn override item group %s", cl.worn_override.c_str() );
        }

        if( !cl.carry_override.empty() && !item_group::group_is_defined( cl.carry_override ) ) {
            debugmsg( "Missing carry override item group %s", cl.carry_override.c_str() );
        }

        if( !cl.weapon_override.empty() && !item_group::group_is_defined( cl.weapon_override ) ) {
            debugmsg( "Missing weapon override item group %s", cl.weapon_override.c_str() );
        }

        for( const auto &pr : cl.skills ) {
            if( !pr.first.is_valid() ) {
                debugmsg( "Invalid skill %s", pr.first.c_str() );
            }
        }

        for( const auto &pr : cl.traits ) {
            if( !pr.first.is_valid() ) {
                debugmsg( "Invalid trait %s", pr.first.c_str() );
            }
        }
    }
}

distribution load_distribution( JsonObject &jo )
{
    if( jo.has_float( "constant" ) ) {
        return distribution::constant( jo.get_float( "constant" ) );
    }

    if( jo.has_float( "one_in" ) ) {
        return distribution::one_in( jo.get_float( "one_in" ) );
    }

    if( jo.has_array( "dice" ) ) {
        JsonArray jarr = jo.get_array( "dice" );
        return distribution::dice_roll( jarr.get_int( 0 ), jarr.get_int( 1 ) );
    }

    if( jo.has_array( "rng" ) ) {
        JsonArray jarr = jo.get_array( "rng" );
        return distribution::rng_roll( jarr.get_int( 0 ), jarr.get_int( 1 ) );
    }

    if( jo.has_array( "sum" ) ) {
        JsonArray jarr = jo.get_array( "sum" );
        JsonObject obj = jarr.next_object();
        distribution ret = load_distribution( obj );
        while( jarr.has_more() ) {
            obj = jarr.next_object();
            ret = ret + load_distribution( obj );
        }

        return ret;
    }

    if( jo.has_array( "mul" ) ) {
        JsonArray jarr = jo.get_array( "mul" );
        JsonObject obj = jarr.next_object();
        distribution ret = load_distribution( obj );
        while( jarr.has_more() ) {
            obj = jarr.next_object();
            ret = ret * load_distribution( obj );
        }

        return ret;
    }

    jo.throw_error( "Invalid distribution" );
    return distribution();
}

distribution load_distribution( JsonObject &jo, const std::string &name )
{
    if( !jo.has_member( name ) ) {
        return distribution();
    }

    if( jo.has_float( name ) ) {
        return distribution::constant( jo.get_float( name ) );
    }

    if( jo.has_object( name ) ) {
        JsonObject obj = jo.get_object( name );
        return load_distribution( obj );
    }

    jo.throw_error( "Invalid distribution type", name );
    return distribution();
}

void npc_class::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name, translated_string_reader );
    mandatory( jo, was_loaded, "job_description", job_description, translated_string_reader );

    optional( jo, was_loaded, "common", common, true );
    bonus_str = load_distribution( jo, "bonus_str" );
    bonus_dex = load_distribution( jo, "bonus_dex" );
    bonus_int = load_distribution( jo, "bonus_int" );
    bonus_per = load_distribution( jo, "bonus_per" );

    optional( jo, was_loaded, "shopkeeper_item_group", shopkeeper_item_group, "EMPTY_GROUP" );
    optional( jo, was_loaded, "worn_override", worn_override );
    optional( jo, was_loaded, "carry_override", carry_override );
    optional( jo, was_loaded, "weapon_override", weapon_override );

    if( jo.has_array( "traits" ) ) {
        JsonArray jarr = jo.get_array( "traits" );
        while( jarr.has_more() ) {
            JsonArray jarr_in = jarr.next_array();
            traits[ trait_id( jarr_in.get_string( 0 ) ) ] = jarr_in.get_int( 1 );
        }
    }

    if( jo.has_array( "skills" ) ) {
        JsonArray jarr = jo.get_array( "skills" );
        while( jarr.has_more() ) {
            JsonObject skill_obj = jarr.next_object();
            auto skill_ids = skill_obj.get_tags( "skill" );
            if( skill_obj.has_object( "level" ) ) {
                distribution dis = load_distribution( skill_obj, "level" );
                for( const auto &sid : skill_ids ) {
                    skills[ skill_id( sid ) ] = dis;
                }
            } else {
                distribution dis = load_distribution( skill_obj, "bonus" );
                for( const auto &sid : skill_ids ) {
                    bonus_skills[ skill_id( sid ) ] = dis;
                }
            }
        }
    }
}

const npc_class_id &npc_class::from_legacy_int( int i )
{
    if( i < 0 || ( size_t )i >= legacy_ids.size() ) {
        debugmsg( "Invalid legacy class id: %d", i );
        return NULL_ID;
    }

    return legacy_ids[ i ];
}

const std::vector<npc_class> &npc_class::get_all()
{
    return npc_class_factory.get_all();
}

const npc_class_id &npc_class::random_common()
{
    std::list<const npc_class_id *> common_classes;
    for( const auto &pr : npc_class_factory.get_all() ) {
        if( pr.common ) {
            common_classes.push_back( &pr.id );
        }
    }

    if( common_classes.empty() ) {
        return NC_NONE;
    }

    return *random_entry( common_classes );
}

const std::string &npc_class::get_name() const
{
    return name;
}

const std::string &npc_class::get_job_description() const
{
    return job_description;
}

const Group_tag &npc_class::get_shopkeeper_items() const
{
    return shopkeeper_item_group;
}

int npc_class::roll_strength() const
{
    return dice( 4, 3 ) + bonus_str.roll();
}

int npc_class::roll_dexterity() const
{
    return dice( 4, 3 ) + bonus_dex.roll();
}

int npc_class::roll_intelligence() const
{
    return dice( 4, 3 ) + bonus_int.roll();
}

int npc_class::roll_perception() const
{
    return dice( 4, 3 ) + bonus_per.roll();
}

int npc_class::roll_skill( const skill_id &sid ) const
{
    const auto &iter = skills.find( sid );
    if( iter == skills.end() ) {
        return 0;
    }

    return std::max<int>( 0, iter->second.roll() );
}

distribution::distribution()
{
    generator_function = []() {
        return 0.0f;
    };
}

distribution::distribution( std::function<float()> gen )
{
    generator_function = gen;
}

float distribution::roll() const
{
    return generator_function();
}

distribution distribution::constant( float val )
{
    return distribution( [val]() {
        return val;
    } );
}

distribution distribution::one_in( float in )
{
    if( in <= 1.0f ) {
        debugmsg( "Invalid one_in: %.2f", in );
        return distribution();
    }

    return distribution( [in]() {
        return one_in_improved( in );
    } );
}

distribution distribution::rng_roll( int from, int to )
{
    return distribution( [from, to]() -> float {
        return rng( from, to );
    } );
}

distribution distribution::dice_roll( int sides, int size )
{
    if( sides < 1 || size < 1 ) {
        debugmsg( "Invalid dice: %d sides, %d sizes", sides, size );
        return distribution();
    }

    return distribution( [sides, size]() -> float {
        return dice( sides, size );
    } );
}

distribution distribution::operator+( const distribution &other ) const
{
    auto my_fun = generator_function;
    auto other_fun = other.generator_function;
    return distribution( [my_fun, other_fun]() {
        return my_fun() + other_fun();
    } );
}

distribution distribution::operator*( const distribution &other ) const
{
    auto my_fun = generator_function;
    auto other_fun = other.generator_function;
    return distribution( [my_fun, other_fun]() {
        return my_fun() * other_fun();
    } );
}

distribution &distribution::operator=( const distribution &other )
{
    generator_function = other.generator_function;
    return *this;
}
