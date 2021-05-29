#include "player.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include "addiction.h"
#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "flag.h"
#include "flat_set.h"
#include "game.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "material.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "pickup.h"
#include "pldata.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "rng.h"
#include "stomach.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"
#include "vitamin.h"

static const std::string comesttype_DRINK( "DRINK" );
static const std::string comesttype_FOOD( "FOOD" );

static const skill_id skill_cooking( "cooking" );
static const skill_id skill_survival( "survival" );

static const mtype_id mon_player_blob( "mon_player_blob" );

static const bionic_id bio_digestion( "bio_digestion" );
static const bionic_id bio_taste_blocker( "bio_taste_blocker" );

static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_common_cold( "common_cold" );
static const efftype_id effect_flu( "flu" );
static const efftype_id effect_foodpoison( "foodpoison" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_hunger_engorged( "hunger_engorged" );
static const efftype_id effect_hunger_full( "hunger_full" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_visuals( "visuals" );

static const json_character_flag json_flag_IMMUNE_SPOIL( "IMMUNE_SPOIL" );
static const json_character_flag json_flag_PARAIMMUNE( "PARAIMMUNE" );
static const json_character_flag json_flag_PRED1( "PRED1" );
static const json_character_flag json_flag_PRED2( "PRED2" );
static const json_character_flag json_flag_PRED3( "PRED3" );
static const json_character_flag json_flag_PRED4( "PRED4" );
static const json_character_flag json_flag_STRICT_HUMANITARIAN( "STRICT_HUMANITARIAN" );

static const item_category_id item_category_chems( "chems" );

static const itype_id itype_apparatus( "apparatus" );
static const itype_id itype_dab_pen_on( "dab_pen_on" );
static const itype_id itype_syringe( "syringe" );

static const mutation_category_id mutation_category_URSINE( "URSINE" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_AMORPHOUS( "AMORPHOUS" );
static const trait_id trait_ANTIFRUIT( "ANTIFRUIT" );
static const trait_id trait_ANTIJUNK( "ANTIJUNK" );
static const trait_id trait_ANTIWHEAT( "ANTIWHEAT" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CARNIVORE( "CARNIVORE" );
static const trait_id trait_EATDEAD( "EATDEAD" );
static const trait_id trait_EATHEALTH( "EATHEALTH" );
static const trait_id trait_EATPOISON( "EATPOISON" );
static const trait_id trait_GIZZARD( "GIZZARD" );
static const trait_id trait_GOURMAND( "GOURMAND" );
static const trait_id trait_HERBIVORE( "HERBIVORE" );
static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_LACTOSE( "LACTOSE" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_MEATARIAN( "MEATARIAN" );
static const trait_id trait_PROBOSCIS( "PROBOSCIS" );
static const trait_id trait_PROJUNK( "PROJUNK" );
static const trait_id trait_PROJUNK2( "PROJUNK2" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_RUMINANT( "RUMINANT" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );
static const trait_id trait_SCHIZOPHRENIC( "SCHIZOPHRENIC" );
static const trait_id trait_SLIMESPAWNER( "SLIMESPAWNER" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_STIMBOOST( "STIMBOOST" );
static const trait_id trait_TABLEMANNERS( "TABLEMANNERS" );
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
static const trait_id trait_THRESH_CATTLE( "THRESH_CATTLE" );
static const trait_id trait_THRESH_FELINE( "THRESH_FELINE" );
static const trait_id trait_THRESH_LUPINE( "THRESH_LUPINE" );
static const trait_id trait_THRESH_PLANT( "THRESH_PLANT" );
static const trait_id trait_THRESH_URSINE( "THRESH_URSINE" );
static const trait_id trait_VEGETARIAN( "VEGETARIAN" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );

// note: cannot use constants from flag.h (e.g. flag_ALLERGEN_VEGGY) here, as they
// might be uninitialized at the time these const arrays are created
static const std::array<flag_id, 4> carnivore_blacklist {{
        flag_id( "ALLERGEN_VEGGY" ), flag_id( "ALLERGEN_FRUIT" ),
        flag_id( "ALLERGEN_WHEAT" ), flag_id( "ALLERGEN_NUT" )
    }};

static const std::array<flag_id, 2> herbivore_blacklist {{
        flag_id( "ALLERGEN_MEAT" ), flag_id( "ALLERGEN_EGG" )
    }};

// TODO: Move pizza scraping here.
static int compute_default_effective_kcal( const item &comest, const Character &you,
        const cata::flat_set<flag_id> &extra_flags = {} )
{
    if( !comest.get_comestible() ) {
        return 0;
    }

    // As float to avoid rounding too many times
    float kcal = comest.get_comestible()->default_nutrition.kcal();

    // Many raw foods give less calories, as your body has expends more energy digesting them.
    bool cooked = comest.has_flag( flag_COOKED ) || extra_flags.count( flag_COOKED );
    if( comest.has_flag( flag_RAW ) && !cooked ) {
        kcal *= 0.75f;
    }

    if( you.has_trait( trait_GIZZARD ) ) {
        kcal *= 0.6f;
    }

    if( you.has_trait( trait_CARNIVORE ) && comest.has_flag( flag_CARNIVORE_OK ) &&
        comest.has_any_flag( carnivore_blacklist ) ) {
        // TODO: Comment pizza scrapping
        kcal *= 0.5f;
    }

    const float relative_rot = comest.get_relative_rot();
    // Saprophages get full nutrition from rotting food
    if( relative_rot > 1.0f && !you.has_trait( trait_SAPROPHAGE ) ) {
        // everyone else only gets a portion of the nutrition
        // Scaling linearly from 100% at just-rotten to 0 at halfway-rotten-away
        const float rottedness = clamp( 2 * relative_rot - 2.0f, 0.1f, 1.0f );
        kcal *= ( 1.0f - rottedness );
    }

    // Bionic digestion gives extra nutrition
    if( you.has_bionic( bio_digestion ) ) {
        kcal *= 1.5f;
    }

    return static_cast<int>( kcal );
}

// Compute default effective vitamins for an item, taking into account player
// traits, but not components of the item.
static std::map<vitamin_id, int> compute_default_effective_vitamins(
    const item &it, const Character &you )
{
    if( !it.get_comestible() ) {
        return {};
    }

    std::map<vitamin_id, int> res = it.get_comestible()->default_nutrition.vitamins;

    for( const trait_id &trait : you.get_mutations() ) {
        const auto &mut = trait.obj();
        // make sure to iterate over every material defined for vitamin absorption
        // TODO: put this loop into a function and utilize it again for bionics
        for( const auto &mat : mut.vitamin_absorb_multi ) {
            // this is where we are able to check if the food actually is changed by the trait
            if( mat.first == material_id( "all" ) || it.made_of( mat.first ) ) {
                const std::map<vitamin_id, double> &mat_vit_map = mat.second;
                for( auto &vit : res ) {
                    auto vit_factor = mat_vit_map.find( vit.first );
                    if( vit_factor != mat_vit_map.end() ) {
                        vit.second *= vit_factor->second;
                    }
                }
            }
        }
    }
    for( const bionic_id &bid : you.get_bionics() ) {
        if( bid->vitamin_absorb_mod == 1.0f ) {
            continue;
        }
        for( std::pair<const vitamin_id, int> &vit : res ) {
            vit.second *= bid->vitamin_absorb_mod;
        }
    }
    return res;
}

// Calculate the effective nutrients for a given item, taking
// into account character traits but not item components.
static nutrients compute_default_effective_nutrients( const item &comest,
        const Character &you, const cata::flat_set<flag_id> &extra_flags = {} )
{
    // Multiply by 1000 to get it in calories
    return { compute_default_effective_kcal( comest, you, extra_flags ) * 1000,
             compute_default_effective_vitamins( comest, you ) };
}

// Calculate the nutrients that the given character would receive from consuming
// the given item, taking into account the item components and the character's
// traits.
// This is used by item display, making actual nutrition available to character.
nutrients Character::compute_effective_nutrients( const item &comest ) const
{
    if( !comest.is_comestible() ) {
        return {};
    }

    // if item has components, will derive calories from that instead.
    if( !comest.components.empty() && !comest.has_flag( flag_NUTRIENT_OVERRIDE ) ) {
        nutrients tally{};
        for( const item &component : comest.components ) {
            nutrients component_value =
                compute_effective_nutrients( component ) * component.charges;
            if( component.has_flag( flag_BYPRODUCT ) ) {
                tally -= component_value;
            } else {
                tally += component_value;
            }
        }
        return tally / comest.recipe_charges;
    } else {
        return compute_default_effective_nutrients( comest, *this );
    }
}

// Calculate range of nutrients obtainable for a given item when crafted via
// the given recipe
std::pair<nutrients, nutrients> Character::compute_nutrient_range(
    const item &comest, const recipe_id &recipe_i,
    const cata::flat_set<flag_id> &extra_flags ) const
{
    if( !comest.is_comestible() ) {
        return {};
    }

    // if item has components, will derive calories from that instead.
    if( comest.has_flag( flag_NUTRIENT_OVERRIDE ) ) {
        nutrients result = compute_default_effective_nutrients( comest, *this );
        return { result, result };
    }

    nutrients tally_min;
    nutrients tally_max;

    const recipe &rec = *recipe_i;

    cata::flat_set<flag_id> our_extra_flags = extra_flags;

    if( rec.hot_result() ) {
        our_extra_flags.insert( flag_COOKED );
    }

    const requirement_data requirements = rec.simple_requirements();
    const requirement_data::alter_item_comp_vector &component_requirements =
        requirements.get_components();

    for( const std::vector<item_comp> &component_options : component_requirements ) {
        nutrients this_min;
        nutrients this_max;
        bool first = true;
        for( const item_comp &component_option : component_options ) {
            std::pair<nutrients, nutrients> component_option_range =
                compute_nutrient_range( component_option.type, our_extra_flags );
            component_option_range.first *= component_option.count;
            component_option_range.second *= component_option.count;

            if( first ) {
                std::tie( this_min, this_max ) = component_option_range;
                first = false;
            } else {
                this_min.min_in_place( component_option_range.first );
                this_max.max_in_place( component_option_range.second );
            }
        }
        tally_min += this_min;
        tally_max += this_max;
    }

    for( const std::pair<const itype_id, int> &byproduct : rec.byproducts ) {
        item byproduct_it( byproduct.first, calendar::turn, byproduct.second );
        nutrients byproduct_nutr = compute_default_effective_nutrients( byproduct_it, *this );
        tally_min -= byproduct_nutr;
        tally_max -= byproduct_nutr;
    }

    int charges = comest.count();
    return { tally_min / charges, tally_max / charges };
}

// Calculate the range of nturients possible for a given item across all
// possible recipes
std::pair<nutrients, nutrients> Character::compute_nutrient_range(
    const itype_id &comest_id, const cata::flat_set<flag_id> &extra_flags ) const
{
    const itype *comest = item::find_type( comest_id );
    if( !comest->comestible ) {
        return {};
    }

    item comest_it( comest, calendar::turn, 1 );
    // The default nutrients are always a possibility
    nutrients min_nutr = compute_default_effective_nutrients( comest_it, *this, extra_flags );

    if( comest->has_flag( flag_NUTRIENT_OVERRIDE ) ||
        recipe_dict.is_item_on_loop( comest->get_id() ) ) {
        return { min_nutr, min_nutr };
    }

    nutrients max_nutr = min_nutr;

    for( const recipe_id &rec : comest->recipes ) {
        nutrients this_min;
        nutrients this_max;

        item result_it = rec->create_result();
        if( result_it.contents.num_item_stacks() == 1 ) {
            const item alt_result = result_it.contents.legacy_front();
            if( alt_result.typeId() == comest_it.typeId() ) {
                result_it = alt_result;
            }
        }
        if( result_it.typeId() != comest_it.typeId() ) {
            debugmsg( "When creating recipe result expected %s, got %s\n",
                      comest_it.typeId().str(), result_it.typeId().str() );
        }
        std::tie( this_min, this_max ) = compute_nutrient_range( result_it, rec, extra_flags );
        min_nutr.min_in_place( this_min );
        max_nutr.max_in_place( this_max );
    }

    return { min_nutr, max_nutr };
}

int Character::nutrition_for( const item &comest ) const
{
    return compute_effective_nutrients( comest ).kcal() / islot_comestible::kcal_per_nutr;
}

std::pair<int, int> Character::fun_for( const item &comest ) const
{
    if( !comest.is_comestible() ) {
        return std::pair<int, int>( 0, 0 );
    }

    // As float to avoid rounding too many times
    float fun = comest.get_comestible_fun();
    // Food doesn't taste as good when you're sick
    if( ( has_effect( effect_common_cold ) || has_effect( effect_flu ) ) && fun > 0 ) {
        fun /= 3;
    }
    // Rotten food should be pretty disgusting
    const float relative_rot = comest.get_relative_rot();
    if( relative_rot > 1.0f && !has_trait( trait_SAPROPHAGE ) && !has_trait( trait_SAPROVORE ) ) {
        const float rottedness = clamp( 2 * relative_rot - 2.0f, 0.1f, 1.0f );
        // Three effects:
        // penalty for rot goes from -2 to -20
        // bonus for tasty food drops from 90% to 0%
        // disgusting food unfun increases from 110% to 200%
        fun -= rottedness * 10;
        if( fun > 0 ) {
            fun *= ( 1.0f - rottedness );
        } else {
            fun *= ( 1.0f + rottedness );
        }
    }

    // Food is less enjoyable when eaten too often.
    if( fun > 0 || comest.has_flag( flag_NEGATIVE_MONOTONY_OK ) ) {
        for( const consumption_event &event : consumption_history ) {
            if( event.time > calendar::turn - 2_days && event.type_id == comest.typeId() &&
                event.component_hash == comest.make_component_hash() ) {
                fun -= comest.get_comestible()->monotony_penalty;
                // This effect can't drop fun below 0, unless the food has the right flag.
                // 0 is the lowest we'll go, no need to keep looping.
                if( fun <= 0 && !comest.has_flag( flag_NEGATIVE_MONOTONY_OK ) ) {
                    fun = 0;
                    break;
                }
            }
        }
    }

    float fun_max = fun < 0 ? fun * 6.0f : fun * 3.0f;
    if( comest.has_flag( flag_EATEN_COLD ) && comest.has_flag( flag_COLD ) ) {
        if( fun > 0 ) {
            fun *= 2;
        } else {
            fun = 1;
            fun_max = 5;
        }
    }

    if( comest.has_flag( flag_MELTS ) && !comest.has_flag( flag_FROZEN ) ) {
        if( fun > 0 ) {
            fun *= 0.5;
        } else {
            // Melted freezable food tastes 25% worse than frozen freezable food.
            // Frozen freezable food... say that 5 times fast
            fun *= 1.25;
        }
    }

    if( ( comest.has_flag( flag_LUPINE ) && has_trait( trait_THRESH_LUPINE ) ) ||
        ( comest.has_flag( flag_FELINE ) && has_trait( trait_THRESH_FELINE ) ) ) {
        if( fun < 0 ) {
            fun = -fun;
            fun /= 2;
        }
    }

    if( has_trait( trait_GOURMAND ) ) {
        if( fun < -1 ) {
            fun_max = fun;
            fun /= 2;
        } else if( fun > 0 ) {
            fun_max *= 3;
            fun = fun * 3 / 2;
        }
    }

    if( has_active_bionic( bio_taste_blocker ) &&
        get_power_level() > units::from_kilojoule( std::abs( comest.get_comestible_fun() ) ) &&
        fun < 0 ) {
        fun = 0;
    }

    return { static_cast< int >( fun ), static_cast< int >( fun_max ) };
}

time_duration Character::vitamin_rate( const vitamin_id &vit ) const
{
    time_duration res = vit.obj().rate();

    for( const auto &m : get_mutations() ) {
        const auto &mut = m.obj();
        auto iter = mut.vitamin_rates.find( vit );
        if( iter != mut.vitamin_rates.end() ) {
            res += iter->second;
        }
    }

    return res;
}

int Character::vitamin_mod( const vitamin_id &vit, int qty, bool capped )
{
    if( !vit.is_valid() ) {
        debugmsg( "Vitamin with id %s does not exist, and cannot be modified", vit.str() );
        return 0;
    }
    // What's going on here? Emplace returns either an iterator to the inserted
    // item or, if it already exists, an iterator to the (unchanged) extant item
    // (Okay, technically it returns a pair<iterator, bool>, the iterator is what we want)
    auto it = vitamin_levels.emplace( vit, 0 ).first;
    const vitamin &v = *it->first;

    if( qty > 0 ) {
        // Accumulations can never occur from food sources
        it->second = std::min( it->second + qty, capped ? 0 : v.max() );
        update_vitamins( vit );

    } else if( qty < 0 ) {
        it->second = std::max( it->second + qty, v.min() );
        update_vitamins( vit );
    }

    return it->second;
}

void Character::vitamins_mod( const std::map<vitamin_id, int> &vitamins, bool capped )
{
    const bool npc_no_food = is_npc() && get_option<bool>( "NO_NPC_FOOD" );
    if( !npc_no_food ) {
        for( const std::pair<const vitamin_id, int> &vit : vitamins ) {
            vitamin_mod( vit.first, vit.second, capped );
        }
    }
}

int Character::vitamin_get( const vitamin_id &vit ) const
{
    if( get_option<bool>( "NO_VITAMINS" ) && vit->type() == vitamin_type::VITAMIN ) {
        return 0;
    }

    const auto &v = vitamin_levels.find( vit );
    return v != vitamin_levels.end() ? v->second : 0;
}

bool Character::vitamin_set( const vitamin_id &vit, int qty )
{
    auto v = vitamin_levels.find( vit );
    if( v == vitamin_levels.end() ) {
        return false;
    }
    vitamin_mod( vit, qty - v->second, false );

    return true;
}

float Character::metabolic_rate_base() const
{
    static const std::string hunger_rate_string( "PLAYER_HUNGER_RATE" );
    float hunger_rate = get_option< float >( hunger_rate_string );
    static const std::string metabolism_modifier( "metabolism_modifier" );
    return hunger_rate * ( 1.0f + mutation_value( metabolism_modifier ) );
}

// TODO: Make this less chaotic to let NPC retroactive catch up work here
// TODO: Involve body heat (cold -> higher metabolism, unless cold-blooded)
// TODO: Involve stamina (maybe not here?)
float Character::metabolic_rate() const
{
    // First value is effective hunger, second is nutrition multiplier
    // Note: Values do not match hungry/v.hungry/famished/starving,
    // because effective hunger is affected by speed (which drops when hungry)
    static const std::vector<std::pair<float, float>> thresholds = {{
            { 300.0f, 1.0f },
            { 2000.0f, 0.8f },
            { 5000.0f, 0.6f },
            { 8000.0f, 0.5f }
        }
    };

    // Penalize fast survivors
    // TODO: Have cold temperature increase, not decrease, metabolism
    const float effective_hunger = ( get_hunger() + get_starvation() ) * 100.0f / std::max( 50,
                                   get_speed() );
    const float modifier = multi_lerp( thresholds, effective_hunger );

    return modifier * metabolic_rate_base();
}

morale_type Character::allergy_type( const item &food ) const
{
    using allergy_tuple = std::tuple<trait_id, flag_id, morale_type>;
    static const std::array<allergy_tuple, 8> allergy_tuples = {{
            std::make_tuple( trait_VEGETARIAN, flag_ALLERGEN_MEAT, MORALE_VEGETARIAN ),
            std::make_tuple( trait_MEATARIAN, flag_ALLERGEN_VEGGY, MORALE_MEATARIAN ),
            std::make_tuple( trait_LACTOSE, flag_ALLERGEN_MILK, MORALE_LACTOSE ),
            std::make_tuple( trait_ANTIFRUIT, flag_ALLERGEN_FRUIT, MORALE_ANTIFRUIT ),
            std::make_tuple( trait_ANTIJUNK, flag_ALLERGEN_JUNK, MORALE_ANTIJUNK ),
            std::make_tuple( trait_ANTIWHEAT, flag_ALLERGEN_WHEAT, MORALE_ANTIWHEAT )
        }
    };

    for( const auto &tp : allergy_tuples ) {
        if( has_trait( std::get<0>( tp ) ) &&
            food.has_flag( std::get<1>( tp ) ) ) {
            return std::get<2>( tp );
        }
    }

    return MORALE_NULL;
}

ret_val<edible_rating> Character::can_eat( const item &food ) const
{
    bool can_fuel_cbm = can_fuel_bionic_with( food );
    if( !food.is_comestible() && !can_fuel_cbm ) {
        return ret_val<edible_rating>::make_failure( _( "That doesn't look edible." ) );
    } else if( can_fuel_cbm ) {
        std::string item_name = food.tname();
        material_id mat_type = food.get_base_material().id;
        if( food.type->magazine ) {
            const item ammo = item( food.ammo_current() );
            item_name = ammo.tname();
            mat_type = ammo.get_base_material().id;
        }
        if( get_fuel_capacity( mat_type ) <= 0 ) {
            return ret_val<edible_rating>::make_failure( _( "No space to store more %s" ), item_name );
        } else {
            return ret_val<edible_rating>::make_success();
        }
    }

    const auto &comest = food.get_comestible();

    if( food.has_flag( flag_INEDIBLE ) ) {
        if( ( food.has_flag( flag_CATTLE ) && !has_trait( trait_THRESH_CATTLE ) ) ||
            ( food.has_flag( flag_FELINE ) && !has_trait( trait_THRESH_FELINE ) ) ||
            ( food.has_flag( flag_LUPINE ) && !has_trait( trait_THRESH_LUPINE ) ) ||
            ( food.has_flag( flag_BIRD ) && !has_trait( trait_THRESH_BIRD ) ) ) {
            return ret_val<edible_rating>::make_failure( _( "That doesn't look edible to you." ) );
        }
    }

    if( food.is_craft() ) {
        return ret_val<edible_rating>::make_failure( _( "That doesn't look edible in its current form." ) );
    }

    if( food.has_own_flag( flag_DIRTY ) ) {
        return ret_val<edible_rating>::make_failure(
                   _( "This is full of dirt after being on the ground." ) );
    }

    const bool eat_verb  = food.has_flag( flag_USE_EAT_VERB );
    const bool edible    = eat_verb ||  comest->comesttype == comesttype_FOOD;
    const bool drinkable = !eat_verb && comest->comesttype == comesttype_DRINK;

    // TODO: This condition occurs way too often. Unify it.
    // update Sep. 26 2018: this apparently still occurs way too often. yay!
    if( is_underwater() && !has_trait( trait_WATERSLEEP ) ) {
        return ret_val<edible_rating>::make_failure( _( "You can't do that while underwater." ) );
    }

    if( edible || drinkable ) {
        for( const auto &elem : food.type->materials ) {
            if( !elem->edible() ) {
                return ret_val<edible_rating>::make_failure( _( "That doesn't look edible in its current form." ) );
            }
        }
        // For all those folks who loved eating marloss berries.  D:< mwuhahaha
        if( has_trait( trait_M_DEPENDENT ) && !food.has_flag( flag_MYCUS_OK ) ) {
            return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION,
                    _( "We can't eat that.  It's not right for us." ) );
        }
    }
    if( food.has_own_flag( flag_FROZEN ) && !food.has_flag( flag_EDIBLE_FROZEN ) &&
        !food.has_flag( flag_MELTS ) ) {
        if( edible ) {
            return ret_val<edible_rating>::make_failure(
                       _( "It's frozen solid.  You must defrost it before you can eat it." ) );
        }
        if( drinkable ) {
            return ret_val<edible_rating>::make_failure( _( "You can't drink it while it's frozen." ) );
        }
    }

    const use_function *consume_drug = food.type->get_use( "consume_drug" );
    if( consume_drug != nullptr ) { //its a drug)
        const consume_drug_iuse *consume_drug_use = dynamic_cast<const consume_drug_iuse *>
                ( consume_drug->get_actor_ptr() );
        for( auto &tool : consume_drug_use->tools_needed ) {
            const bool has = item::count_by_charges( tool.first )
                             ? has_charges( tool.first, ( tool.second == -1 ) ? 1 : tool.second )
                             : has_amount( tool.first, 1 );
            if( !has ) {
                return ret_val<edible_rating>::make_failure( NO_TOOL,
                        string_format( _( "You need a %s to consume that!" ),
                                       item::nname( tool.first ) ) );
            }
        }
    }

    const use_function *smoking = food.type->get_use( "SMOKING" );
    if( smoking != nullptr ) {
        cata::optional<std::string> litcig = iuse::can_smoke( *this->as_player() );
        if( litcig.has_value() ) {
            return ret_val<edible_rating>::make_failure( NO_TOOL, _( litcig.value_or( "" ) ) );
        }
    }

    if( !comest->tool.is_null() ) {
        const bool has = item::count_by_charges( comest->tool )
                         ? has_charges( comest->tool, 1 )
                         : has_amount( comest->tool, 1 );
        if( !has ) {
            return ret_val<edible_rating>::make_failure( NO_TOOL,
                    string_format( _( "You need a %s to consume that!" ),
                                   item::nname( comest->tool ) ) );
        }
    }

    // Here's why PROBOSCIS is such a negative trait.
    if( has_trait( trait_PROBOSCIS ) && !( drinkable || food.is_medication() ) ) {
        return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION, _( "Ugh, you can't drink that!" ) );
    }

    if( has_trait( trait_CARNIVORE ) && nutrition_for( food ) > 0 &&
        food.has_any_flag( carnivore_blacklist ) && !food.has_flag( flag_CARNIVORE_OK ) ) {
        return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION,
                _( "Eww.  Inedible plant stuff!" ) );
    }

    if( ( has_trait( trait_HERBIVORE ) || has_trait( trait_RUMINANT ) ) &&
        food.has_any_flag( herbivore_blacklist ) ) {
        // Like non-cannibal, but more strict!
        return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION,
                _( "The thought of eating that makes you feel sick." ) );
    }

    for( const trait_id &mut : get_mutations() ) {
        if( !food.made_of_any( mut.obj().can_only_eat ) && !mut.obj().can_only_eat.empty() ) {
            return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION, _( "You can't eat this." ) );
        }
    }

    return ret_val<edible_rating>::make_success();
}

ret_val<edible_rating> Character::will_eat( const item &food, bool interactive ) const
{
    const auto ret = can_eat( food );
    if( !ret.success() ) {
        if( interactive ) {
            add_msg_if_player( m_info, "%s", ret.c_str() );
        }
        return ret;
    }

    // exit early for cbm fuel as we've already tested everything in can_eat
    if( !food.is_comestible() ) {
        return ret_val<edible_rating>::make_success();
    }

    std::vector<ret_val<edible_rating>> consequences;
    const auto add_consequence = [&consequences]( const std::string & msg, edible_rating code ) {
        consequences.emplace_back( ret_val<edible_rating>::make_failure( code, msg ) );
    };

    const bool saprophage = has_trait( trait_SAPROPHAGE );
    const auto &comest = food.get_comestible();

    if( food.rotten() ) {
        const bool saprovore = has_trait( trait_SAPROVORE );
        if( !saprophage && !saprovore ) {
            add_consequence( _( "This is rotten and smells awful!" ), ROTTEN );
        }
    }

    if( food.has_flag( flag_STRICT_HUMANITARIANISM ) &&
        !has_trait_flag( json_flag_STRICT_HUMANITARIAN ) ) {
        add_consequence( _( "The thought of eating demihuman flesh makes you feel sick." ), CANNIBALISM );
    }

    const bool carnivore = has_trait( trait_CARNIVORE );
    if( food.has_flag( flag_CANNIBALISM ) &&
        !has_trait_flag( STATIC( json_character_flag( "CANNIBAL" ) ) ) ) {
        add_consequence( _( "The thought of eating human flesh makes you feel sick." ), CANNIBALISM );
    }

    if( food.get_comestible()->parasites > 0 && !food.has_flag( flag_NO_PARASITES ) &&
        !has_flag( json_flag_PARAIMMUNE ) ) {
        add_consequence( _( "Eating this raw meat probably isn't very healthy." ), PARASITES );
    }

    const bool edible = comest->comesttype == comesttype_FOOD || food.has_flag( flag_USE_EAT_VERB );

    if( edible && has_effect( effect_nausea ) ) {
        add_consequence( _( "You still feel nauseous and will probably puke it all up again." ), NAUSEA );
    }

    if( ( allergy_type( food ) != MORALE_NULL ) || ( carnivore && food.has_flag( flag_ALLERGEN_JUNK ) &&
            !food.has_flag( flag_CARNIVORE_OK ) ) ) {
        add_consequence( _( "Your stomach won't be happy (allergy)." ), ALLERGY );
    }

    if( saprophage && edible && food.rotten() && !food.has_flag( flag_FERTILIZER ) ) {
        // Note: We're allowing all non-solid "food". This includes drugs
        // Hard-coding fertilizer for now - should be a separate flag later
        //~ No, we don't eat "rotten" food. We eat properly aged food, like a normal person.
        //~ Semantic difference, but greatly facilitates people being proud of their character.
        add_consequence( _( "Your stomach won't be happy (not rotten enough)." ), ALLERGY_WEAK );
    }

    if( food.charges > 0 && food.is_food() &&
        ( food.charges_per_volume( stomach.stomach_remaining( *this ) ) < 1 ||
          has_effect( effect_hunger_full ) || has_effect( effect_hunger_engorged ) ) ) {
        if( edible ) {
            add_consequence( _( "You're full already and will be forcing yourself to eat." ), TOO_FULL );
        } else {
            add_consequence( _( "You're full already and will be forcing yourself to drink." ), TOO_FULL );
        }
    }

    if( !consequences.empty() ) {
        if( !interactive ) {
            return consequences.front();
        }
        std::string req;
        for( const auto &elem : consequences ) {
            req += elem.str() + "\n";
        }

        const bool eat_verb  = food.has_flag( flag_USE_EAT_VERB );
        std::string food_tame = food.tname();
        const nc_color food_color = food.color_in_inventory();
        if( eat_verb || comest->comesttype == comesttype_FOOD ) {
            req += string_format( _( "Eat your %s anyway?" ), colorize( food_tame, food_color ) );
        } else if( !eat_verb && comest->comesttype == comesttype_DRINK ) {
            req += string_format( _( "Drink your %s anyway?" ), colorize( food_tame, food_color ) );
        } else {
            req += string_format( _( "Consume your %s anyway?" ), colorize( food_tame, food_color ) );
        }

        if( !query_yn( req ) ) {
            return consequences.front();
        }
    }
    // All checks ended, it's edible (or we're pretending it is)
    return ret_val<edible_rating>::make_success();
}

/** Eat a comestible.
*   @return true if item consumed.
*/
static bool eat( item &food, player &you, bool force )
{
    if( !food.is_food() ) {
        return false;
    }

    const auto ret = force ? you.can_eat( food ) : you.will_eat( food, you.is_player() );
    if( !ret.success() ) {
        return false;
    }

    int charges_used = 0;
    if( food.type->has_use() ) {
        if( !food.type->can_use( "DOGFOOD" ) &&
            !food.type->can_use( "CATFOOD" ) &&
            !food.type->can_use( "BIRDFOOD" ) &&
            !food.type->can_use( "CATTLEFODDER" ) ) {
            charges_used = food.type->invoke( you, food, you.pos() ).value_or( 0 );
            if( charges_used <= 0 ) {
                return false;
            }
        }
    }

    // Note: the block below assumes we decided to eat it
    // No coming back from here

    const bool hibernate = you.has_active_mutation( trait_HIBERNATE );
    const int nutr = you.nutrition_for( food );
    const int quench = food.get_comestible()->quench;
    const bool spoiled = food.rotten();

    // The item is solid food
    const bool chew = food.get_comestible()->comesttype == comesttype_FOOD ||
                      food.has_flag( flag_USE_EAT_VERB );
    // This item is a drink and not a solid food (and not a thick soup)
    const bool drinkable = !chew && food.get_comestible()->comesttype == comesttype_DRINK;
    // If neither of the above is true then it's a drug and shouldn't get mealtime penalty/bonus

    if( hibernate &&
        ( you.get_hunger() > -60 && you.get_thirst() > -60 ) &&
        ( you.get_hunger() - nutr < -60 || you.get_thirst() - quench < -60 ) ) {
        you.add_msg_if_player(
            _( "You've begun stockpiling calories and liquid for hibernation.  You get the feeling that you should prepare for bed, just in case, but… you're hungry again, and you could eat a whole week's worth of food RIGHT NOW." ) );
    }

    const bool will_vomit = you.stomach.stomach_remaining( you ) < food.volume() &&
                            rng( units::to_milliliter( you.stomach.capacity( you ) ) / 2,
                                 units::to_milliliter( you.stomach.contains() ) ) > units::to_milliliter(
                                you.stomach.capacity( you ) );
    const bool saprophage = you.has_trait( trait_SAPROPHAGE );
    if( spoiled && !saprophage ) {
        you.add_msg_if_player( m_bad, _( "Ick, this %s doesn't taste so good…" ), food.tname() );
        if( !you.has_flag( json_flag_IMMUNE_SPOIL ) ) {
            you.add_effect( effect_foodpoison, rng( 6_minutes, ( nutr + 1 ) * 6_minutes ) );
        }
    } else if( spoiled && saprophage ) {
        you.add_msg_if_player( m_good, _( "Mmm, this %s tastes delicious…" ), food.tname() );
    }
    if( !you.consume_effects( food ) ) {
        // Already consumed by using `food.type->invoke`?
        if( charges_used > 0 ) {
            food.mod_charges( -charges_used );
        }
        return false;
    }
    food.mod_charges( -1 );

    const bool amorphous = you.has_trait( trait_AMORPHOUS );

    // If it's poisonous... poison us.
    // TODO: Move this to a flag
    if( food.poison > 0 && !you.has_trait( trait_EATPOISON ) &&
        !you.has_trait( trait_EATDEAD ) ) {
        if( food.poison >= rng( 2, 4 ) ) {
            you.add_effect( effect_poison, food.poison * 10_minutes );
        }

        you.add_effect( effect_foodpoison, food.poison * 30_minutes );
    }

    if( food.has_flag( flag_HIDDEN_HALLU ) ) {
        if( !you.has_effect( effect_hallu ) ) {
            you.add_effect( effect_hallu, 6_hours );
        }
    }

    if( amorphous ) {
        you.add_msg_player_or_npc( _( "You assimilate your %s." ), _( "<npcname> assimilates a %s." ),
                                   food.tname() );
    } else if( drinkable ) {
        if( you.has_trait( trait_SCHIZOPHRENIC ) &&
            one_in( 50 ) && !spoiled && food.goes_bad() && you.is_player() ) {

            add_msg( m_bad, _( "Ick, this %s (rotten) doesn't taste so good…" ), food.tname() );
            add_msg( _( "You drink your %s (rotten)." ), food.tname() );
        } else {
            you.add_msg_player_or_npc( _( "You drink your %s." ), _( "<npcname> drinks a %s." ),
                                       food.tname() );
        }
    } else if( chew ) {
        if( you.has_trait( trait_SCHIZOPHRENIC ) &&
            one_in( 50 ) && !spoiled && food.goes_bad() && you.is_player() ) {

            add_msg( m_bad, _( "Ick, this %s (rotten) doesn't taste so good…" ), food.tname() );
            add_msg( _( "You eat your %s (rotten)." ), food.tname() );
        } else {
            you.add_msg_player_or_npc( _( "You eat your %s." ), _( "<npcname> eats a %s." ),
                                       food.tname() );
        }
    }

    if( item::find_type( food.get_comestible()->tool )->tool ) {
        // Tools like lighters get used
        you.use_charges( food.get_comestible()->tool, 1 );
    }

    if( you.has_active_bionic( bio_taste_blocker ) ) {
        you.mod_power_level( units::from_kilojoule( -std::abs( food.get_comestible_fun() ) ) );
    }

    if( food.has_flag( flag_FUNGAL_VECTOR ) && !you.has_trait( trait_M_IMMUNE ) ) {
        you.add_effect( effect_fungus, 1_turns, true );
    }

    // The fun changes for these effects are applied in fun_for().
    if( food.has_flag( flag_MUSHY ) ) {
        you.add_msg_if_player( m_bad,
                               _( "You try to ignore its mushy texture, but it leaves you with an awful aftertaste." ) );
    }
    if( food.get_comestible_fun() > 0 ) {
        if( you.has_effect( effect_common_cold ) ) {
            you.add_msg_if_player( m_bad, _( "You can't taste much of anything with this cold." ) );
        }
        if( you.has_effect( effect_flu ) ) {
            you.add_msg_if_player( m_bad, _( "You can't taste much of anything with this flu." ) );
        }
    }

    // Chance to become parasitised
    if( !will_vomit && !you.has_flag( json_flag_PARAIMMUNE ) ) {
        if( food.get_comestible()->parasites > 0 && !food.has_flag( flag_NO_PARASITES ) &&
            one_in( food.get_comestible()->parasites ) ) {
            switch( rng( 0, 3 ) ) {
                case 0:
                    if( !you.has_trait( trait_EATHEALTH ) ) {
                        you.add_effect( effect_tapeworm, 1_turns, true );
                    }
                    break;
                case 1:
                    if( !you.has_trait( trait_ACIDBLOOD ) ) {
                        you.add_effect( effect_bloodworms, 1_turns, true );
                    }
                    break;
                case 2:
                    you.add_effect( effect_brainworms, 1_turns, true );
                    break;
                case 3:
                    you.add_effect( effect_paincysts, 1_turns, true );
            }
        }
    }

    for( const std::pair<const diseasetype_id, int> &elem : food.get_comestible()->contamination ) {
        if( rng( 1, 100 ) <= elem.second ) {
            you.expose_to_disease( elem.first );
        }
    }

    get_event_bus().send<event_type::character_eats_item>( you.getID(), food.typeId() );

    if( will_vomit ) {
        you.vomit();
    }

    you.consumption_history.emplace_back( food );
    // Clean out consumption_history so it doesn't get bigger than needed.
    while( you.consumption_history.front().time < calendar::turn - 2_days ) {
        you.consumption_history.pop_front();
    }

    return true;
}

void Character::modify_health( const islot_comestible &comest )
{
    const int effective_health = comest.healthy;
    // Effectively no cap on health modifiers from food and meds
    const int health_cap = 200;
    mod_healthy_mod( effective_health, effective_health >= 0 ? health_cap : -health_cap );
}

void Character::modify_stimulation( const islot_comestible &comest )
{
    if( comest.stim == 0 ) {
        return;
    }
    const int current_stim = get_stim();
    if( ( std::abs( comest.stim ) * 3 ) > std::abs( current_stim ) ) {
        mod_stim( comest.stim );
    } else {
        comest.stim > 0 ? mod_stim( std::max( comest.stim / 2, 1 ) ) : mod_stim( std::min( comest.stim / 2,
                -1 ) );
    }
    if( has_trait( trait_STIMBOOST ) && ( current_stim > 30 ) &&
        ( ( comest.add == add_type::CAFFEINE ) || ( comest.add == add_type::SPEED ) ||
          ( comest.add == add_type::COKE ) || ( comest.add == add_type::CRACK ) ) ) {
        int hallu_duration = ( current_stim - comest.stim < 30 ) ? current_stim - 30 : comest.stim;
        add_effect( effect_visuals, hallu_duration * 30_minutes );
        std::vector<std::string> stimboost_msg{ _( "The shadows are getting ever closer." ),
                                                _( "You have a bad feeling about this." ),
                                                _( "A powerful sense of dread comes over you." ),
                                                _( "Your skin starts crawling." ),
                                                _( "They're coming to get you." ),
                                                _( "This might've been a bad idea…" ),
                                                _( "You've really done it this time, haven't you?" ),
                                                _( "You have to stay vigilant.  They're always watching…" ),
                                                _( "mistake mistake mistake mistake mistake" ),
                                                _( "Just gotta stay calm, and you'll make it through this." ),
                                                _( "You're starting to feel very jumpy." ),
                                                _( "Something is twitching at the edge of your vision." ),
                                                _( "They know what you've done…" ),
                                                _( "You're feeling even more paranoid than usual." ) };
        add_msg_if_player( m_bad, random_entry_ref( stimboost_msg ) );
    }
}

void Character::modify_fatigue( const islot_comestible &comest )
{
    mod_fatigue( -comest.fatigue_mod );
}

void Character::modify_radiation( const islot_comestible &comest )
{
    irradiate( comest.radiation );
}

void Character::modify_addiction( const islot_comestible &comest )
{
    add_addiction( comest.add, comest.addict );
    if( addiction_craving( comest.add ) != MORALE_NULL ) {
        rem_morale( addiction_craving( comest.add ) );
    }
}

void Character::modify_morale( item &food, const int nutr )
{
    time_duration morale_time = 2_hours;
    if( food.has_flag( flag_HOT ) && food.has_flag( flag_EATEN_HOT ) ) {
        morale_time = 3_hours;
        int clamped_nutr = std::max( 5, std::min( 20, nutr / 10 ) );
        add_morale( MORALE_FOOD_HOT, clamped_nutr, 20, morale_time, morale_time / 2 );
    }

    std::pair<int, int> fun = fun_for( food );
    if( fun.first < 0 ) {
        add_morale( MORALE_FOOD_BAD, fun.first, fun.second, morale_time, morale_time / 2, false,
                    food.type );
    } else if( fun.first > 0 ) {
        add_morale( MORALE_FOOD_GOOD, fun.first, fun.second, morale_time, morale_time / 2, false,
                    food.type );
    }

    // Morale bonus for eating unspoiled food with chair/table nearby
    // Does not apply to non-ingested consumables like bandages or drugs,
    // nor to drinks.
    if( !food.has_flag( flag_NO_INGEST ) &&
        food.get_comestible()->comesttype != "MED" &&
        food.get_comestible()->comesttype != comesttype_DRINK ) {
        map &here = get_map();
        if( here.has_nearby_chair( pos(), 1 ) && here.has_nearby_table( pos(), 1 ) ) {
            if( has_trait( trait_TABLEMANNERS ) ) {
                rem_morale( MORALE_ATE_WITHOUT_TABLE );
                if( !food.rotten() ) {
                    add_morale( MORALE_ATE_WITH_TABLE, 3, 3, 3_hours, 2_hours, true );
                }
            } else if( !food.rotten() ) {
                add_morale( MORALE_ATE_WITH_TABLE, 1, 1, 3_hours, 2_hours, true );
            }
        } else {
            if( has_trait( trait_TABLEMANNERS ) ) {
                rem_morale( MORALE_ATE_WITH_TABLE );
                add_morale( MORALE_ATE_WITHOUT_TABLE, -2, -4, 3_hours, 2_hours, true );
            }
        }
    }

    if( food.has_flag( flag_HIDDEN_HALLU ) ) {
        if( has_trait( trait_SPIRITUAL ) ) {
            add_morale( MORALE_FOOD_GOOD, 36, 72, 2_hours, 1_hours, false );
        } else {
            add_morale( MORALE_FOOD_GOOD, 18, 36, 1_hours, 30_minutes, false );
        }
    }

    if( food.has_flag( flag_CANNIBALISM ) ) {
        // Sapiovores don't recognize humans as the same species.
        // But let them possibly feel cool about eating sapient stuff - treat like psycho
        // However, spiritual sapiovores should still recognize humans as having a soul or special for religious reasons
        const bool cannibal = has_trait( trait_CANNIBAL );
        const bool psycho = has_trait( trait_PSYCHOPATH );
        const bool sapiovore = has_trait( trait_SAPIOVORE );
        const bool spiritual = has_trait( trait_SPIRITUAL );
        if( ( cannibal || sapiovore ) && psycho && spiritual ) {
            add_msg_if_player( m_good,
                               _( "You feast upon the human flesh, and in doing so, devour their spirit." ) );
            // You're not really consuming anything special; you just think you are.
            add_morale( MORALE_CANNIBAL, 25, 300 );
        } else if( cannibal && psycho ) {
            add_msg_if_player( m_good, _( "You feast upon the human flesh." ) );
            add_morale( MORALE_CANNIBAL, 15, 200 );
        } else if( ( cannibal || sapiovore ) && spiritual ) {
            add_msg_if_player( m_good, _( "You consume the sacred human flesh." ) );
            // Boosted because you understand the philosophical implications of your actions, and YOU LIKE THEM.
            add_morale( MORALE_CANNIBAL, 15, 200 );
        } else if( cannibal ) {
            add_msg_if_player( m_good, _( "You indulge your shameful hunger." ) );
            add_morale( MORALE_CANNIBAL, 10, 50 );
        } else if( ( psycho || sapiovore ) && spiritual ) {
            add_msg_if_player( _( "You greedily devour the taboo meat." ) );
            // Small bonus for violating a taboo.
            add_morale( MORALE_CANNIBAL, 5, 50 );
        } else if( psycho || sapiovore ) {
            add_msg_if_player( _( "Meh.  You've eaten worse." ) );
        } else if( spiritual ) {
            add_msg_if_player( m_bad,
                               _( "This is probably going to count against you if there's still an afterlife." ) );
            add_morale( MORALE_CANNIBAL, -60, -400, 60_minutes, 30_minutes );
        } else {
            add_msg_if_player( m_bad, _( "You feel horrible for eating a person." ) );
            add_morale( MORALE_CANNIBAL, -60, -400, 60_minutes, 30_minutes );
        }
    }

    // While raw flesh usually means negative morale, carnivores and cullers get a small bonus.
    // Hunters, predators, and apex predators don't mind raw flesh at all, maybe even like it.
    // Cooked flesh is unaffected, because people with these traits *prefer* it raw. Fat is unaffected.
    // Organs are still usually negative due to fun values as low as -35.
    // The PREDATOR_FUN flag shouldn't be on human flesh, to not interfere with sapiovores/cannibalism.
    if( food.has_flag( flag_PREDATOR_FUN ) ) {
        const bool carnivore = has_trait( trait_CARNIVORE );
        const bool culler = has_trait_flag( json_flag_PRED1 );
        const bool hunter = has_trait_flag( json_flag_PRED2 );
        const bool predator = has_trait_flag( json_flag_PRED3 );
        const bool apex_predator = has_trait_flag( json_flag_PRED4 );
        if( apex_predator ) {
            // Largest bonus, balances out to around +5 or +10. Some organs may still be negative.
            add_morale( MORALE_MEATARIAN, 20, 10 );
            add_msg_if_player( m_good,
                               _( "As you tear into the raw flesh, you feel satisfied with your meal." ) );
        } else if( predator || hunter ) {
            // Should approximately balance the fun to 0 for normal meat.
            add_morale( MORALE_MEATARIAN, 15, 5 );
            add_msg_if_player( m_good,
                               _( "Raw flesh doesn't taste all that bad, actually." ) );
        } else if( carnivore || culler ) {
            // Only a small bonus (+5), still negative fun.
            add_morale( MORALE_MEATARIAN, 5, 0 );
            add_msg_if_player( m_bad,
                               _( "This doesn't taste very good, but meat is meat." ) );
        }
    }

    // Allergy check for food that is ingested (not gum)
    if( !food.has_flag( flag_NO_INGEST ) ) {
        const auto allergy = allergy_type( food );
        if( allergy != MORALE_NULL ) {
            add_msg_if_player( m_bad, _( "Yuck!  How can anybody eat this stuff?" ) );
            add_morale( allergy, -75, -400, 30_minutes, 24_minutes );
        }
        if( food.has_flag( flag_ALLERGEN_JUNK ) ) {
            if( has_trait( trait_PROJUNK ) ) {
                add_msg_if_player( m_good, _( "Mmm, junk food." ) );
                add_morale( MORALE_SWEETTOOTH, 5, 30, 30_minutes, 24_minutes );
            }
            if( has_trait( trait_PROJUNK2 ) ) {
                if( !one_in( 100 ) ) {
                    add_msg_if_player( m_good, _( "When life's got you down, there's always sugar." ) );
                } else {
                    add_msg_if_player( m_good, _( "They may do what they must… you've already won." ) );
                }
                add_morale( MORALE_SWEETTOOTH, 10, 50, 1_hours, 50_minutes );
            }
            // Carnivores CAN eat junk food, but they won't like it much.
            // Pizza-scraping happens in consume_effects.
            if( has_trait( trait_CARNIVORE ) && !food.has_flag( flag_CARNIVORE_OK ) ) {
                add_msg_if_player( m_bad, _( "Your stomach begins gurgling and you feel bloated and ill." ) );
                add_morale( MORALE_NO_DIGEST, -25, -125, 30_minutes, 24_minutes );
            }
        }
    }
    const bool chew = food.get_comestible()->comesttype == comesttype_FOOD ||
                      food.has_flag( flag_USE_EAT_VERB );
    if( !food.rotten() && chew && has_trait( trait_SAPROPHAGE ) ) {
        // It's OK to *drink* things that haven't rotted.  Alternative is to ban water.  D:
        add_msg_if_player( m_bad, _( "Your stomach begins gurgling and you feel bloated and ill." ) );
        add_morale( MORALE_NO_DIGEST, -75, -400, 30_minutes, 24_minutes );
    }
    if( food.has_flag( flag_URSINE_HONEY ) && ( !crossed_threshold() ||
            has_trait( trait_THRESH_URSINE ) ) &&
        mutation_category_level[mutation_category_URSINE] > 40 ) {
        // Need at least 5 bear mutations for effect to show, to filter out mutations in common with other categories
        int honey_fun = has_trait( trait_THRESH_URSINE ) ?
                        std::min( mutation_category_level[mutation_category_URSINE] / 8, 20 ) :
                        mutation_category_level[mutation_category_URSINE] / 12;
        if( honey_fun < 10 ) {
            add_msg_if_player( m_good, _( "You find the sweet taste of honey surprisingly palatable." ) );
        } else {
            add_msg_if_player( m_good, _( "You feast upon the sweet honey." ) );
        }
        add_morale( MORALE_HONEY, honey_fun, 100 );
    }
}

// Used when determining stomach fullness from eating.
double Character::compute_effective_food_volume_ratio( const item &food ) const
{
    const nutrients food_nutrients = compute_effective_nutrients( food );
    units::mass food_weight = ( food.weight() / food.count() );
    double ratio = 1.0f;
    if( units::to_gram( food_weight ) != 0 ) {
        ratio = std::max( static_cast<double>( food_nutrients.kcal() ) / units::to_gram( food_weight ),
                          1.0 );
        if( ratio > 3.0f ) {
            ratio = std::sqrt( 3 * ratio );
        }
    }
    return ratio;
}

// Remove the water volume from the food, as that gets absorbed and used as water.
// If the remaining dry volume of the food is less dense than water, crunch it down to a density equal to water.
// These maths are made easier by the fact that 1 g = 1 mL. Thanks, metric system.
units::volume Character::masticated_volume( const item &food ) const
{
    units::volume water_vol = ( food.get_comestible()->quench > 0 ) ? food.get_comestible()->quench *
                              5_ml : 0_ml;
    units::mass water_weight = units::from_gram( units::to_milliliter( water_vol ) );
    units::mass food_dry_weight = food.weight() / food.count() - water_weight;
    units::volume food_dry_volume = food.volume() / food.count() - water_vol ;

    if( units::to_milliliter( food_dry_volume ) != 0 &&
        units::to_gram( food_dry_weight ) < units::to_milliliter( food_dry_volume ) ) {
        food_dry_volume = units::from_milliliter( units::to_gram( food_dry_weight ) );
    }

    return food_dry_volume;
}

// Used when displaying effective food satiation values.
int Character::compute_calories_per_effective_volume( const item &food,
        const nutrients *nutrient /* = nullptr */ )const
{
    /* Understanding how Calories Per Effective Volume are calculated requires a dive into the
    stomach fullness source code. Look at issue #44365*/
    int kcalories;
    if( nutrient ) {
        // if given the optional nutrient argument, we will compute kcal based on that. ( Crafting menu ).
        kcalories = nutrient->kcal();
    } else {
        kcalories = compute_effective_nutrients( food ).kcal();
    }
    double food_vol = round_up( units::to_liter( masticated_volume( food ) ), 2 );
    const double energy_density_ratio = compute_effective_food_volume_ratio( food );
    const double effective_volume = food_vol * energy_density_ratio;
    if( kcalories == 0 && effective_volume == 0.0 ) {
        return 0;
    }
    return std::round( kcalories / effective_volume );
}

bool Character::consume_effects( item &food )
{
    if( !food.is_comestible() ) {
        debugmsg( "called Character::consume_effects with non-comestible" );
        return false;
    }

    if( has_trait( trait_THRESH_PLANT ) && food.type->can_use( "PLANTBLECH" ) ) {
        // Was used to cap nutrition and thirst, but no longer does this
        return false;
    }
    if( ( has_trait( trait_HERBIVORE ) || has_trait( trait_RUMINANT ) ) &&
        food.has_any_flag( herbivore_blacklist ) ) {
        // No good can come of this.
        return false;
    }

    const auto &comest = *food.get_comestible();

    // Rotten food causes health loss
    const float relative_rot = food.get_relative_rot();
    if( relative_rot > 1.0f && !has_flag( json_flag_IMMUNE_SPOIL ) ) {
        const float rottedness = clamp( 2 * relative_rot - 2.0f, 0.1f, 1.0f );
        // ~-1 health per 1 nutrition at halfway-rotten-away, ~0 at "just got rotten"
        // But always round down
        int h_loss = -rottedness * comest.get_default_nutr();
        mod_healthy_mod( h_loss, -200 );
        add_msg_debug( "%d health from %0.2f%% rotten food", h_loss, rottedness );
    }

    // Used in hibernation messages.
    const int nutr = nutrition_for( food );
    const bool skip_health = has_trait( trait_PROJUNK2 ) && comest.healthy < 0;
    // We can handle junk just fine
    if( !skip_health ) {
        modify_health( comest );
    }
    modify_stimulation( comest );
    modify_fatigue( comest );
    modify_radiation( comest );
    modify_addiction( comest );
    modify_morale( food, nutr );

    const bool hibernate = has_active_mutation( trait_HIBERNATE );
    if( hibernate ) {
        if( ( nutr > 0 && get_hunger() < -60 ) || ( comest.quench > 0 && get_thirst() < -60 ) ) {
            // Tell the player what's going on
            add_msg_if_player( _( "You gorge yourself, preparing to hibernate." ) );
            if( one_in( 2 ) ) {
                // 50% chance of the food tiring you
                mod_fatigue( nutr );
            }
        }
        if( ( nutr > 0 && get_hunger() < -200 ) || ( comest.quench > 0 && get_thirst() < -200 ) ) {
            // Hibernation should cut burn to 60/day
            add_msg_if_player( _( "You feel stocked for a day or two.  Got your bed all ready and secured?" ) );
            if( one_in( 2 ) ) {
                // And another 50%, intended cumulative
                mod_fatigue( nutr );
            }
        }

        if( ( nutr > 0 && get_hunger() < -400 ) || ( comest.quench > 0 && get_thirst() < -400 ) ) {
            add_msg_if_player(
                _( "Mmm.  You can still fit some more in… but maybe you should get comfortable and sleep." ) );
            if( !one_in( 3 ) ) {
                // Third check, this one at 66%
                mod_fatigue( nutr );
            }
        }
        if( ( nutr > 0 && get_hunger() < -600 ) || ( comest.quench > 0 && get_thirst() < -600 ) ) {
            add_msg_if_player( _( "That filled a hole!  Time for bed…" ) );
            // At this point, you're done.  Schlaf gut.
            mod_fatigue( nutr );
        }
    }
    // Moved here and changed a bit - it was too complex
    // Incredibly minor stuff like this shouldn't require complexity
    if( !is_npc() && has_trait( trait_SLIMESPAWNER ) &&
        ( get_healthy_kcal() < get_stored_kcal() + 4000 &&
          get_thirst() - stomach.get_water() / 5_ml < -20 ) && get_thirst() < 40 ) {
        add_msg_if_player( m_mixed,
                           _( "You feel as though you're going to split open!  In a good way?" ) );
        mod_pain( 5 );
        int numslime = 1;
        for( int i = 0; i < numslime; i++ ) {
            if( monster *const slime = g->place_critter_around( mon_player_blob, pos(), 1 ) ) {
                slime->friendly = -1;
            }
        }
        mod_hunger( 40 );
        mod_thirst( 40 );
        //~ slimespawns have *small voices* which may be the Nice equivalent
        //~ of the Rat King's ALL CAPS invective.  Probably shared-brain telepathy.
        add_msg_if_player( m_good, _( "hey, you look like me!  let's work together!" ) );
    }

    nutrients food_nutrients = compute_effective_nutrients( food );
    const units::volume water_vol = ( food.get_comestible()->quench > 0 ) ?
                                    food.get_comestible()->quench *
                                    5_ml : 0_ml;
    units::volume food_vol = masticated_volume( food );
    if( food.count() == 0 ) {
        debugmsg( "Tried to eat food with count of zero." );
        return false;
    }
    units::mass food_weight = ( food.weight() / food.count() );
    const double ratio = compute_effective_food_volume_ratio( food );
    food_summary ingested{
        water_vol,
        food_vol * ratio,
        food_nutrients
    };
    add_msg_debug(
        "Effective volume: %d (solid) %d (liquid)\n multiplier: %g calories: %d, weight: %d",
        units::to_milliliter( ingested.solids ), units::to_milliliter( ingested.water ), ratio,
        food_nutrients.kcal(), units::to_gram( food_weight ) );
    // Maybe move tapeworm to digestion
    if( has_effect( effect_tapeworm ) ) {
        ingested.nutr /= 2;
    }

    // GET IN MAH BELLY!
    stomach.ingest( ingested );
    return true;
}

bool Character::fuel_bionic_with( item &it )
{
    if( !can_fuel_bionic_with( it ) ) {
        return false;
    }

    if( it.is_favorite &&
        !get_avatar().query_yn( _( "Are you sure you want to eat your favorited %s?" ), it.tname() ) ) {
        return false;
    }

    const bionic_id bio = get_most_efficient_bionic( get_bionic_fueled_with( it ) );

    const bool is_magazine = !!it.type->magazine;
    int loadable;
    material_id mat = it.get_base_material().id;

    if( is_magazine ) {
        const item ammo = item( it.ammo_current() );
        mat = ammo.get_base_material().id;
        loadable = std::min( it.ammo_remaining(), get_fuel_capacity( mat ) );
        it.ammo_set( ammo.typeId(), it.ammo_remaining() - loadable );
    } else if( it.flammable() ) {
        // This a special case for items that are not fuels and don't have charges
        loadable = std::min( units::to_milliliter( it.volume() ), get_fuel_capacity( mat ) );
        it.charges -= it.charges_per_volume( units::from_milliliter( loadable ) );
    } else {
        loadable = std::min( it.charges, get_fuel_capacity( mat ) );
        it.charges -= loadable;
    }

    const std::string str_loaded  = get_value( mat.str() );
    int loaded = 0;
    if( !str_loaded.empty() ) {
        loaded = std::stoi( str_loaded );
    }

    const std::string new_charge = std::to_string( loadable + loaded );

    // Type and amount of fuel
    set_value( mat.str(), new_charge );
    update_fuel_storage( mat );
    add_msg_player_or_npc( m_info,
                           //~ %1$i: charge number, %2$s: item name, %3$s: bionics name
                           ngettext( "You load %1$i charge of %2$s in your %3$s.",
                                     "You load %1$i charges of %2$s in your %3$s.", loadable ),
                           //~ %1$i: charge number, %2$s: item name, %3$s: bionics name
                           ngettext( "<npcname> load %1$i charge of %2$s in their %3$s.",
                                     "<npcname> load %1$i charges of %2$s in their %3$s.", loadable ), loadable, mat->name(),
                           bio->name );
    mod_moves( -250 );
    // Return false for magazines because only their ammo is consumed
    return !is_magazine;
}

int Character::get_acquirable_energy( const item &it ) const
{
    const std::vector<bionic_id> &bids = get_bionic_fueled_with( it );
    if( bids.empty() ) {
        return 0;
    }
    const bionic_id &bid = get_most_efficient_bionic( bids );
    int to_consume;
    int to_charge = 0;
    if( it.type->magazine ) {
        item ammo = item( it.ammo_current() );
        to_consume = std::min( it.ammo_remaining(), bid->fuel_capacity );
        to_charge = ammo.fuel_energy() * to_consume * bid->fuel_efficiency;
    } else if( it.flammable() ) {
        to_consume = std::min( units::to_milliliter( it.volume() ), bid->fuel_capacity );
        to_charge = it.get_base_material().id->get_fuel_data().energy * to_consume * bid->fuel_efficiency;
    } else {
        to_consume = std::min( it.charges, bid->fuel_capacity );
        to_charge = it.fuel_energy() * to_consume * bid->fuel_efficiency;
    }
    return to_charge;
}

bool Character::can_estimate_rot() const
{
    return get_skill_level( skill_cooking ) >= 3 || get_skill_level( skill_survival ) >= 4;
}

bool Character::can_consume_as_is( const item &it ) const
{
    return it.is_comestible() || can_fuel_bionic_with( it );
}

bool Character::can_consume( const item &it ) const
{
    if( can_consume_as_is( it ) ) {
        return true;
    }
    return it.has_item_with( [&]( const item & consumable ) {
        // Checking NO_RELOAD to prevent consumption of `battery` when contained in `battery_car` (#20012)
        return !consumable.has_flag( flag_NO_RELOAD ) && can_consume_as_is( consumable );
    } );
}

item &Character::get_consumable_from( item &it ) const
{
    item *ret = nullptr;
    it.visit_items( [&]( item * it, item * ) {
        if( can_consume_as_is( *it ) ) {
            ret = it;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );

    if( ret != nullptr ) {
        return *ret;
    }

    static item null_comestible;
    // Since it's not const.
    null_comestible = item();
    return null_comestible;
}

time_duration Character::get_consume_time( const item &it )
{
    const int charges = std::max( it.charges, 1 );
    int volume = units::to_milliliter( it.volume() ) / charges;
    if( 0 == volume && it.type ) {
        volume = units::to_milliliter( it.type->volume );
    }
    time_duration time = time_duration::from_seconds( std::max( ( volume /
                         5 ), 1 ) );  //Default 5 mL (1 tablespoon) per second
    float consume_time_modifier = 1.0f;//only for food and drinks
    const bool eat_verb = it.has_flag( flag_USE_EAT_VERB );
    const std::string comest_type = it.get_comestible() ? it.get_comestible()->comesttype : "";
    if( eat_verb || comest_type == "FOOD" ) {
        time = time_duration::from_seconds( volume / 5 ); //Eat 5 mL (1 teaspoon) per second
        consume_time_modifier = mutation_value( "consume_time_modifier" );
    } else if( !eat_verb && comest_type == "DRINK" ) {
        time = time_duration::from_seconds( volume / 15 ); //Drink 15 mL (1 tablespoon) per second
        consume_time_modifier = mutation_value( "consume_time_modifier" );
    } else if( it.is_medication() ) {
        const use_function *consume_drug = it.type->get_use( "consume_drug" );
        const use_function *smoking = it.type->get_use( "SMOKING" );
        const use_function *adrenaline_injector = it.type->get_use( "ADRENALINE_INJECTOR" );
        const use_function *heal = it.type->get_use( "heal" );
        if( consume_drug != nullptr ) { //its a drug
            const consume_drug_iuse *consume_drug_use = dynamic_cast<const consume_drug_iuse *>
                    ( consume_drug->get_actor_ptr() );
            if( consume_drug_use->tools_needed.find( itype_syringe ) != consume_drug_use->tools_needed.end() ) {
                time = time_duration::from_minutes( 5 );//sterile injections take 5 minutes
            } else if( consume_drug_use->tools_needed.find( itype_apparatus ) !=
                       consume_drug_use->tools_needed.end() ||
                       consume_drug_use->tools_needed.find( itype_dab_pen_on ) != consume_drug_use->tools_needed.end() ) {
                time = time_duration::from_seconds( 30 );//smoke a bowl
            } else {
                time = time_duration::from_seconds( 5 );//popping a pill is quick
            }
        } else if( smoking != nullptr ) {
            time = time_duration::from_minutes( 1 );//about five minutes for a cig or joint so 1 minute a charge
        } else if( adrenaline_injector != nullptr ) {
            time = time_duration::from_seconds( 15 );//epi-pens are fairly quick
        } else if( heal != nullptr ) {
            time = time_duration::from_seconds( 15 );//bandages and disinfectant are fairly quick
        } else {
            time = time_duration::from_seconds( 5 ); //probably pills so quick
        }
    } else if( it.get_category_shallow().get_id() == item_category_chems ) {
        time = time_duration::from_seconds( std::max( ( volume / 15 ),
                                            1 ) ); //Consume 15 mL (1 tablespoon) per second
        consume_time_modifier = mutation_value( "consume_time_modifier" );
    }

    return time * consume_time_modifier;
}

static bool query_consume_ownership( item &target, player &p )
{
    if( !target.is_owned_by( p, true ) ) {
        bool choice = true;
        if( p.get_value( "THIEF_MODE" ) == "THIEF_ASK" ) {
            choice = Pickup::query_thief();
        }
        if( p.get_value( "THIEF_MODE" ) == "THIEF_HONEST" || !choice ) {
            return false;
        }
        std::vector<npc *> witnesses;
        for( npc &elem : g->all_npcs() ) {
            if( rl_dist( elem.pos(), p.pos() ) < MAX_VIEW_DISTANCE && elem.sees( p.pos() ) ) {
                witnesses.push_back( &elem );
            }
        }
        for( npc *elem : witnesses ) {
            elem->say( "<witnessed_thievery>", 7 );
        }
        if( !witnesses.empty() && target.is_owned_by( p, true ) ) {
            if( p.add_faction_warning( target.get_owner() ) ) {
                for( npc *elem : witnesses ) {
                    elem->make_angry();
                }
            }
        }
    }
    return true;
}

/** Consume medication.
*   @return true if item consumed.
*/
static bool consume_med( item &target, player &you )
{
    if( !target.is_medication() ) {
        return false;
    }

    const itype_id tool_type = target.get_comestible()->tool;
    const itype *req_tool = item::find_type( tool_type );

    if( req_tool->tool ) {
        if( !( you.has_amount( tool_type, 1 ) &&
               you.has_charges( tool_type, req_tool->tool->charges_per_use ) ) ) {
            you.add_msg_if_player( m_info, _( "You need a %s to consume that!" ), req_tool->nname( 1 ) );
            return false;
        }
        you.use_charges( tool_type, req_tool->tool->charges_per_use );
    }

    int amount_used = 1;
    if( target.type->has_use() ) {
        amount_used = target.type->invoke( you, target, you.pos() ).value_or( 0 );
        if( amount_used <= 0 ) {
            return false;
        }
    }

    // TODO: Get the target it was used on
    // Otherwise injecting someone will give us addictions etc.
    if( target.has_flag( flag_NO_INGEST ) ) {
        const auto &comest = *target.get_comestible();
        // Assume that parenteral meds don't spoil, so don't apply rot
        you.modify_health( comest );
        you.modify_stimulation( comest );
        you.modify_fatigue( comest );
        you.modify_radiation( comest );
        you.modify_addiction( comest );
        you.modify_morale( target );
    } else {
        // Take by mouth
        you.consume_effects( target );
    }

    target.mod_charges( -amount_used );
    return true;
}

trinary player::consume( item &target, bool force )
{
    if( target.is_null() ) {
        add_msg_if_player( m_info, _( "You do not have that item." ) );
        return trinary::NONE;
    }
    if( is_underwater() && !has_trait( trait_WATERSLEEP ) ) {
        add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return trinary::NONE;
    }

    if( target.is_craft() ) {
        add_msg_if_player( m_info, _( "You can't eat your %s." ), target.tname() );
        if( is_npc() ) {
            debugmsg( "%s tried to eat a %s", name, target.tname() );
        }
        return trinary::NONE;
    }
    if( is_player() && !query_consume_ownership( target, *this ) ) {
        return trinary::NONE;
    }
    if( consume_med( target, *this ) ||
        eat( target, *this, force ) ||
        fuel_bionic_with( target ) ) {

        get_event_bus().send<event_type::character_consumes_item>( getID(), target.typeId() );

        target.on_contents_changed();
        return target.charges <= 0 ? trinary::ALL : trinary::SOME;
    }

    return trinary::NONE;
}

trinary player::consume( item_location loc, bool force )
{
    if( !loc ) {
        debugmsg( "Null loc to consume." );
        return trinary::NONE;
    }
    contents_change_handler handler;
    item &target = *loc;
    trinary result = consume( target, force );
    if( result != trinary::NONE ) {
        handler.unseal_pocket_containing( loc );
    }
    if( result == trinary::ALL ) {
        if( loc.where() == item_location::type::character ) {
            i_rem( loc.get_item() );
        } else {
            loc.remove_item();
        }
    }
    handler.handle_by( *this );
    return result;
}
