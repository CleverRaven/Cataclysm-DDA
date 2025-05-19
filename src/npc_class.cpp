#include "npc_class.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <utility>

#include "avatar.h"
#include "condition.h"
#include "creature.h"
#include "debug.h"
#include "dialogue.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item_group.h"
#include "mutation.h"
#include "npc.h"
#include "rng.h"
#include "shop_cons_rate.h"
#include "skill.h"
#include "string_formatter.h"
#include "trait_group.h"
#include "translations.h"
#include "weighted_list.h"

class item;

static generic_factory<npc_class> npc_class_factory( "npc_class" );

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

npc_class::npc_class() : id( npc_class_id::NULL_ID() )
{
}

void npc_class::load_npc_class( const JsonObject &jo, const std::string &src )
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
        return pr.first.str() == "ALL";
    } );

    if( iter != skills.end() ) {
        distribution dis = iter->second;
        skills.erase( iter );
        for( const Skill &sk : Skill::skills ) {
            if( skills.count( sk.ident() ) == 0 ) {
                skills[ sk.ident() ] = dis;
            }
        }
    }
}

void npc_class::finalize_all()
{
    for( const npc_class &cl_const : npc_class_factory.get_all() ) {
        npc_class &cl = const_cast<npc_class &>( cl_const );
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
    for( const npc_class &cl : npc_class_factory.get_all() ) {
        for( const shopkeeper_item_group &ig : cl.shop_item_groups ) {
            if( !item_group::group_is_defined( ig.id ) ) {
                debugmsg( "Missing shopkeeper item group %s", ig.id.c_str() );
            }
        }

        if( !cl.worn_override.is_empty() && !item_group::group_is_defined( cl.worn_override ) ) {
            debugmsg( "Missing worn override item group %s", cl.worn_override.c_str() );
        }

        if( !cl.carry_override.is_empty() && !item_group::group_is_defined( cl.carry_override ) ) {
            debugmsg( "Missing carry override item group %s", cl.carry_override.c_str() );
        }

        if( !cl.weapon_override.is_empty() && !item_group::group_is_defined( cl.weapon_override ) ) {
            debugmsg( "Missing weapon override item group %s", cl.weapon_override.c_str() );
        }

        for( const auto &pr : cl.skills ) {
            if( !pr.first.is_valid() ) {
                debugmsg( "Invalid skill %s", pr.first.c_str() );
            }
        }

        if( !cl.traits.is_valid() ) {
            debugmsg( "Trait group %s is undefined", cl.traits.c_str() );
        }
    }
}

bool npc_class::is_common() const
{
    return common;
}

static distribution load_distribution( const JsonObject &jo )
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
        distribution ret = load_distribution( jarr.next_object() );
        while( jarr.has_more() ) {
            ret = ret + load_distribution( jarr.next_object() );
        }

        return ret;
    }

    if( jo.has_array( "mul" ) ) {
        JsonArray jarr = jo.get_array( "mul" );
        distribution ret = load_distribution( jarr.next_object() );
        while( jarr.has_more() ) {
            ret = ret * load_distribution( jarr.next_object() );
        }

        return ret;
    }

    jo.throw_error( "Invalid distribution" );
}

static distribution load_distribution( const JsonObject &jo, std::string_view name )
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

    jo.throw_error_at( name, "Invalid distribution type" );
}

bool shopkeeper_item_group::can_sell( npc const &guy ) const
{
    const_dialogue temp( get_const_talker_for( get_avatar() ), get_const_talker_for( guy ) );
    faction *const fac = guy.get_faction();

    return ( fac == nullptr || trust <= guy.get_faction()->trusts_u ) &&
           ( !condition || condition( temp ) );
}

bool shopkeeper_item_group::can_restock( npc const &guy ) const
{
    return !strict || can_sell( guy );
}

std::string shopkeeper_item_group::get_refusal() const
{
    if( refusal.empty() ) {
        return _( "<npcname> does not trust you enough" );
    }

    return refusal.translated();
}

void shopkeeper_item_group::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "group", id );
    optional( jo, false, "trust", trust, 0 );
    optional( jo, false, "strict", strict, false );
    optional( jo, false, "rigid", rigid, false );
    optional( jo, false, "refusal", refusal );
    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", condition, false );
    }
}

void npc_class::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "job_description", job_description );

    optional( jo, was_loaded, "common", common, true );
    if( common ) {
        optional( jo, was_loaded, "common_spawn_weight", common_spawn_weight, 1.0 );
    } else if( jo.has_float( "common_spawn_weight" ) ) {
        jo.throw_error_at( "common_spawn_weight",
                           string_format( "npc class %s defines a spawn weighting, but cannot spawn randomly", name ) );
    }
    bonus_str = load_distribution( jo, "bonus_str" );
    bonus_dex = load_distribution( jo, "bonus_dex" );
    bonus_int = load_distribution( jo, "bonus_int" );
    bonus_per = load_distribution( jo, "bonus_per" );

    bonus_aggression = load_distribution( jo, "bonus_aggression" );
    bonus_bravery = load_distribution( jo, "bonus_bravery" );
    bonus_collector = load_distribution( jo, "bonus_collector" );
    bonus_altruism = load_distribution( jo, "bonus_altruism" );

    if( jo.has_member( "shopkeeper_item_group" ) ) {
        if( jo.has_array( "shopkeeper_item_group" ) &&
            jo.get_array( "shopkeeper_item_group" ).test_object() ) {
            mandatory( jo, was_loaded, "shopkeeper_item_group", shop_item_groups );
        } else if( jo.has_string( "shopkeeper_item_group" ) ) {
            const std::string &ig_str = jo.get_string( "shopkeeper_item_group" );
            shop_item_groups.emplace_back( ig_str, 0, false );
        } else {
            jo.throw_error( string_format( "invalid format for shopkeeper_item_group in npc class %s", name ) );
        }
    }
    optional( jo, was_loaded, "shopkeeper_price_rules", shop_price_rules, faction_price_rules_reader {} );
    optional( jo, was_loaded, SHOPKEEPER_CONSUMPTION_RATES, shop_cons_rates_id,
              shopkeeper_cons_rates_id::NULL_ID() );
    optional( jo, was_loaded, SHOPKEEPER_BLACKLIST, shop_blacklist_id,
              shopkeeper_blacklist_id::NULL_ID() );
    optional( jo, was_loaded, "restock_interval", restock_interval, 6_days );
    optional( jo, was_loaded, "worn_override", worn_override );
    optional( jo, was_loaded, "carry_override", carry_override );
    optional( jo, was_loaded, "weapon_override", weapon_override );
    optional( jo, was_loaded, "bye_message_override", bye_message_override );

    if( jo.has_member( "traits" ) ) {
        traits = trait_group::load_trait_group( jo.get_member( "traits" ), "collection" );
    }

    if( jo.has_array( "spells" ) ) {
        for( JsonObject subobj : jo.get_array( "spells" ) ) {
            const int level = subobj.get_int( "level" );
            const spell_id sp = spell_id( subobj.get_string( "id" ) );
            _starting_spells.emplace( sp, level );
        }
    }

    optional( jo, was_loaded, "proficiencies", _starting_proficiencies );
    optional( jo, was_loaded, "sells_belongings", sells_belongings, true );
    /* Mutation rounds can be specified as follows:
     *   "mutation_rounds": {
     *     "ANY" : { "constant": 1 },
     *     "INSECT" : { "rng": [1, 3] }
     *   }
     */
    if( jo.has_object( "mutation_rounds" ) ) {
        const std::map<mutation_category_id, mutation_category_trait> &mutation_categories =
            mutation_category_trait::get_all();
        for( const JsonMember member : jo.get_object( "mutation_rounds" ) ) {
            const mutation_category_id mutation( member.name() );
            const auto category_match = [&mutation]( const
                                        std::pair<const mutation_category_id, mutation_category_trait>
            &p ) {
                return p.second.id == mutation;
            };
            if( std::find_if( mutation_categories.begin(), mutation_categories.end(),
                              category_match ) == mutation_categories.end() ) {
                debugmsg( "Unrecognized mutation category %s", mutation.str() );
                continue;
            }
            JsonObject distrib = member.get_object();
            mutation_rounds[mutation] = load_distribution( distrib );
        }
    }

    if( jo.has_array( "skills" ) ) {
        for( JsonObject skill_obj : jo.get_array( "skills" ) ) {
            auto skill_ids = skill_obj.get_tags( "skill" );
            if( skill_obj.has_object( "level" ) ) {
                const distribution dis = load_distribution( skill_obj, "level" );
                for( const auto &sid : skill_ids ) {
                    skills[ skill_id( sid ) ] = dis;
                }
            } else {
                const distribution dis = load_distribution( skill_obj, "bonus" );
                for( const auto &sid : skill_ids ) {
                    bonus_skills[ skill_id( sid ) ] = dis;
                }
            }
        }
    }

    if( jo.has_array( "bionics" ) ) {
        for( JsonObject bionic_obj : jo.get_array( "bionics" ) ) {
            auto bionic_ids = bionic_obj.get_tags( "id" );
            int chance = bionic_obj.get_int( "chance" );
            for( const auto &bid : bionic_ids ) {
                bionic_list[ bionic_id( bid )] = chance;
            }
        }
    }
}

const std::vector<npc_class> &npc_class::get_all()
{
    return npc_class_factory.get_all();
}

const npc_class_id &npc_class::random_common()
{
    weighted_float_list<const npc_class_id *> weighted_classes;
    for( const npc_class &pr : npc_class_factory.get_all() ) {
        if( pr.common ) {
            weighted_classes.add( &pr.id, pr.common_spawn_weight );
        }
    }

    const npc_class_id *chosen_class = *weighted_classes.pick();

    if( !chosen_class ) {
        return npc_class_id::NULL_ID();
    }

    return *chosen_class;
}

std::string npc_class::get_name() const
{
    return name.translated();
}

std::string npc_class::get_job_description() const
{
    return job_description.translated();
}

const std::vector<shopkeeper_item_group> &npc_class::get_shopkeeper_items() const
{
    return shop_item_groups;
}

const shopkeeper_cons_rates &npc_class::get_shopkeeper_cons_rates() const
{
    if( shop_cons_rates_id.is_null() ) {
        shopkeeper_cons_rates static const null_rates;
        return null_rates;
    }
    return shop_cons_rates_id.obj();
}

const shopkeeper_blacklist &npc_class::get_shopkeeper_blacklist() const
{
    if( shop_blacklist_id.is_null() ) {
        shopkeeper_blacklist static const null_blacklist;
        return null_blacklist;
    }
    return shop_blacklist_id.obj();
}

faction_price_rule const *npc_class::get_price_rules( item const &it, npc const &guy ) const
{
    auto const el = std::find_if(
    shop_price_rules.crbegin(), shop_price_rules.crend(), [&it, &guy]( faction_price_rule const & fc ) {
        return fc.matches( it, guy );
    } );
    if( el != shop_price_rules.crend() ) {
        return &*el;
    }
    return nullptr;
}

const time_duration &npc_class::get_shop_restock_interval() const
{
    return restock_interval;
}

int npc_class::roll_strength() const
{
    return bonus_str.roll();
}

int npc_class::roll_dexterity() const
{
    return bonus_dex.roll();
}

int npc_class::roll_intelligence() const
{
    return bonus_int.roll();
}

int npc_class::roll_perception() const
{
    return bonus_per.roll();
}

int npc_class::roll_aggression() const
{
    return bonus_aggression.roll();
}

int npc_class::roll_bravery() const
{
    return bonus_bravery.roll();
}

int npc_class::roll_collector() const
{
    return bonus_collector.roll();
}

int npc_class::roll_altruism() const
{
    return bonus_altruism.roll();
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

distribution::distribution( const distribution &d )
{
    generator_function = d.generator_function;
}

distribution::distribution( const std::function<float()> &gen )
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
        return x_in_y( 1, in );
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

distribution &distribution::operator=( const distribution &other ) = default;
