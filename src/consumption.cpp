#include "player.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <string>
#include <tuple>

#include "addiction.h"
#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_utility.h"
#include "debug.h"
#include "enums.h"
#include "flat_set.h"
#include "game.h"
#include "item_contents.h"
#include "itype.h"
#include "map.h"
#include "material.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "options.h"
#include "pldata.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "rng.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "units.h"
#include "value_ptr.h"
#include "vitamin.h"

static const std::string comesttype_DRINK( "DRINK" );
static const std::string comesttype_FOOD( "FOOD" );

static const skill_id skill_cooking( "cooking" );
static const skill_id skill_survival( "survival" );

static const mtype_id mon_player_blob( "mon_player_blob" );

static const bionic_id bio_advreactor( "bio_advreactor" );
static const bionic_id bio_digestion( "bio_digestion" );
static const bionic_id bio_furnace( "bio_furnace" );
static const bionic_id bio_reactor( "bio_reactor" );
static const bionic_id bio_taste_blocker( "bio_taste_blocker" );

static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_bloated( "bloated" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_foodpoison( "foodpoison" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_visuals( "visuals" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_AMORPHOUS( "AMORPHOUS" );
static const trait_id trait_ANTIFRUIT( "ANTIFRUIT" );
static const trait_id trait_ANTIJUNK( "ANTIJUNK" );
static const trait_id trait_ANTIWHEAT( "ANTIWHEAT" );
static const trait_id trait_BEAK_HUM( "BEAK_HUM" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CARNIVORE( "CARNIVORE" );
static const trait_id trait_EATDEAD( "EATDEAD" );
static const trait_id trait_EATPOISON( "EATPOISON" );
static const trait_id trait_FANGS_SPIDER( "FANGS_SPIDER" );
static const trait_id trait_GIZZARD( "GIZZARD" );
static const trait_id trait_GOURMAND( "GOURMAND" );
static const trait_id trait_HERBIVORE( "HERBIVORE" );
static const trait_id trait_LACTOSE( "LACTOSE" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_MANDIBLES( "MANDIBLES" );
static const trait_id trait_MEATARIAN( "MEATARIAN" );
static const trait_id trait_MOUTH_TENTACLES( "MOUTH_TENTACLES" );
static const trait_id trait_PARAIMMUNE( "PARAIMMUNE" );
static const trait_id trait_PROBOSCIS( "PROBOSCIS" );
static const trait_id trait_PROJUNK( "PROJUNK" );
static const trait_id trait_PROJUNK2( "PROJUNK2" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_RUMINANT( "RUMINANT" );
static const trait_id trait_SABER_TEETH( "SABER_TEETH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );
static const trait_id trait_SCHIZOPHRENIC( "SCHIZOPHRENIC" );
static const trait_id trait_SHARKTEETH( "SHARKTEETH" );
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

static const std::string flag_EATEN_COLD( "EATEN_COLD" );
static const std::string flag_HIDDEN_HALLU( "HIDDEN_HALLU" );
static const std::string flag_ALLERGEN_EGG( "ALLERGEN_EGG" );
static const std::string flag_ALLERGEN_FRUIT( "ALLERGEN_FRUIT" );
static const std::string flag_ALLERGEN_JUNK( "ALLERGEN_JUNK" );
static const std::string flag_ALLERGEN_MEAT( "ALLERGEN_MEAT" );
static const std::string flag_ALLERGEN_MILK( "ALLERGEN_MILK" );
static const std::string flag_ALLERGEN_VEGGY( "ALLERGEN_VEGGY" );
static const std::string flag_ALLERGEN_WHEAT( "ALLERGEN_WHEAT" );
static const std::string flag_ALLERGEN_NUT( "ALLERGEN_NUT" );
static const std::string flag_BIRD( "BIRD" );
static const std::string flag_BYPRODUCT( "BYPRODUCT" );
static const std::string flag_CANNIBALISM( "CANNIBALISM" );
static const std::string flag_CARNIVORE_OK( "CARNIVORE_OK" );
static const std::string flag_CATTLE( "CATTLE" );
static const std::string flag_COOKED( "COOKED" );
static const std::string flag_CORPSE( "CORPSE" );
static const std::string flag_EATEN_HOT( "EATEN_HOT" );
static const std::string flag_FELINE( "FELINE" );
static const std::string flag_FERTILIZER( "FERTILIZER" );
static const std::string flag_FUNGAL_VECTOR( "FUNGAL_VECTOR" );
static const std::string flag_INEDIBLE( "INEDIBLE" );
static const std::string flag_LUPINE( "LUPINE" );
static const std::string flag_MYCUS_OK( "MYCUS_OK" );
static const std::string flag_NEGATIVE_MONOTONY_OK( "NEGATIVE_MONOTONY_OK" );
static const std::string flag_NO_PARASITES( "NO_PARASITES" );
static const std::string flag_NO_RELOAD( "NO_RELOAD" );
static const std::string flag_NUTRIENT_OVERRIDE( "NUTRIENT_OVERRIDE" );
static const std::string flag_RADIOACTIVE( "RADIOACTIVE" );
static const std::string flag_RAW( "RAW" );
static const std::string flag_URSINE_HONEY( "URSINE_HONEY" );
static const std::string flag_USE_EAT_VERB( "USE_EAT_VERB" );

const std::vector<std::string> carnivore_blacklist {{
        flag_ALLERGEN_VEGGY, flag_ALLERGEN_FRUIT, flag_ALLERGEN_WHEAT, flag_ALLERGEN_NUT,
    }
};
// This ugly temp array is here because otherwise it goes
// std::vector(char*, char*)->vector(InputIterator,InputIterator) or some such
const std::array<std::string, 2> temparray {{flag_ALLERGEN_MEAT, flag_ALLERGEN_EGG}};
const std::vector<std::string> herbivore_blacklist( temparray.begin(), temparray.end() );

// Defines the maximum volume that a internal furnace can consume
const units::volume furnace_max_volume( 3_liter );

// TODO: JSONize.
const std::map<itype_id, int> plut_charges = {
    { "plut_cell",         PLUTONIUM_CHARGES * 10 },
    { "plut_slurry_dense", PLUTONIUM_CHARGES },
    { "plut_slurry",       PLUTONIUM_CHARGES / 2 }
};

static int compute_default_effective_kcal( const item &comest, const Character &you,
        const cata::flat_set<std::string> &extra_flags = {} )
{
    if( !comest.get_comestible() ) {
        return 0;
    }

    // As float to avoid rounding too many times
    float kcal = comest.get_comestible()->default_nutrition.kcal;

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
        // Everyone else only gets a half of the nutrition
        kcal *= 0.5f;
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

    return res;
}

// Calculate the effective nutrients for a given item, taking
// into account character traits but not item components.
static nutrients compute_default_effective_nutrients( const item &comest,
        const Character &you, const cata::flat_set<std::string> &extra_flags = {} )
{
    return { compute_default_effective_kcal( comest, you, extra_flags ),
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
    const cata::flat_set<std::string> &extra_flags ) const
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

    cata::flat_set<std::string> our_extra_flags = extra_flags;

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
    const itype_id &comest_id, const cata::flat_set<std::string> &extra_flags ) const
{
    const itype *comest = item::find_type( comest_id );
    if( !comest->comestible ) {
        return {};
    }

    item comest_it( comest, calendar::turn, 1 );
    // The default nutrients are always a possibility
    nutrients min_nutr = compute_default_effective_nutrients( comest_it, *this, extra_flags );

    if( comest->item_tags.count( flag_NUTRIENT_OVERRIDE ) ||
        recipe_dict.is_item_on_loop( comest->get_id() ) ) {
        return { min_nutr, min_nutr };
    }

    nutrients max_nutr = min_nutr;

    for( const recipe_id &rec : comest->recipes ) {
        nutrients this_min;
        nutrients this_max;

        item result_it = rec->create_result();
        if( result_it.contents.num_item_stacks() == 1 ) {
            const item alt_result = result_it.contents.front();
            if( alt_result.typeId() == comest_it.typeId() ) {
                result_it = alt_result;
            }
        }
        if( result_it.typeId() != comest_it.typeId() ) {
            debugmsg( "When creating recipe result expected %s, got %s\n",
                      comest_it.typeId(), result_it.typeId() );
        }
        std::tie( this_min, this_max ) = compute_nutrient_range( result_it, rec, extra_flags );
        min_nutr.min_in_place( this_min );
        max_nutr.max_in_place( this_max );
    }

    return { min_nutr, max_nutr };
}

int Character::nutrition_for( const item &comest ) const
{
    return compute_effective_nutrients( comest ).kcal / islot_comestible::kcal_per_nutr;
}

std::pair<int, int> Character::fun_for( const item &comest ) const
{
    if( !comest.is_comestible() ) {
        return std::pair<int, int>( 0, 0 );
    }

    // As float to avoid rounding too many times
    float fun = comest.get_comestible_fun();
    // Rotten food should be pretty disgusting
    const float relative_rot = comest.get_relative_rot();
    if( relative_rot > 1.0f && !has_trait( trait_SAPROPHAGE ) && !has_trait( trait_SAPROVORE ) ) {
        fun -= 10;
        if( fun > 0 ) {
            fun *= 0.5f;
        } else {
            fun *= 2.0f;
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

    float fun_max = fun < 0 ? fun * 6 : fun * 3;

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

    if( has_active_bionic( bio_taste_blocker ) && comest.get_comestible_fun() < 0 &&
        get_power_level() > units::from_kilojoule( -comest.get_comestible_fun() ) &&
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
    auto it = vitamin_levels.find( vit );
    if( it == vitamin_levels.end() ) {
        return 0;
    }
    const auto &v = it->first.obj();

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
    for( auto vit : vitamins ) {
        vitamin_mod( vit.first, vit.second, capped );
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
    const float effective_hunger = get_hunger() * 100.0f / std::max( 50, get_speed() );
    const float modifier = multi_lerp( thresholds, effective_hunger );

    return modifier * metabolic_rate_base();
}

morale_type Character::allergy_type( const item &food ) const
{
    using allergy_tuple = std::tuple<trait_id, std::string, morale_type>;
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

    const auto &comest = food.get_comestible();
    if( !comest ) {
        return ret_val<edible_rating>::make_failure( _( "That doesn't look edible." ) );
    }

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

    if( food.item_tags.count( "DIRTY" ) ) {
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
    }

    if( comest->tool != "null" ) {
        const bool has = item::count_by_charges( comest->tool )
                         ? has_charges( comest->tool, 1 )
                         : has_amount( comest->tool, 1 );
        if( !has ) {
            return ret_val<edible_rating>::make_failure( edible_rating::no_tool,
                    string_format( _( "You need a %s to consume that!" ),
                                   item::nname( comest->tool ) ) );
        }
    }

    // For all those folks who loved eating marloss berries.  D:< mwuhahaha
    if( has_trait( trait_M_DEPENDENT ) && !food.has_flag( flag_MYCUS_OK ) ) {
        return ret_val<edible_rating>::make_failure( edible_rating::inedible_mutation,
                _( "We can't eat that.  It's not right for us." ) );
    }
    // Here's why PROBOSCIS is such a negative trait.
    if( has_trait( trait_PROBOSCIS ) && !( drinkable || food.is_medication() ) ) {
        return ret_val<edible_rating>::make_failure( edible_rating::inedible_mutation,
                _( "Ugh, you can't drink that!" ) );
    }

    if( has_trait( trait_CARNIVORE ) && nutrition_for( food ) > 0 &&
        food.has_any_flag( carnivore_blacklist ) && !food.has_flag( flag_CARNIVORE_OK ) ) {
        return ret_val<edible_rating>::make_failure( edible_rating::inedible_mutation,
                _( "Eww.  Inedible plant stuff!" ) );
    }

    if( ( has_trait( trait_HERBIVORE ) || has_trait( trait_RUMINANT ) ) &&
        food.has_any_flag( herbivore_blacklist ) ) {
        // Like non-cannibal, but more strict!
        return ret_val<edible_rating>::make_failure( edible_rating::inedible_mutation,
                _( "The thought of eating that makes you feel sick." ) );
    }

    for( const trait_id &mut : get_mutations() ) {
        if( !food.made_of_any( mut.obj().can_only_eat ) && !mut.obj().can_only_eat.empty() ) {
            return ret_val<edible_rating>::make_failure( edible_rating::inedible_mutation,
                    _( "You can't eat this." ) );
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

    std::vector<ret_val<edible_rating>> consequences;
    const auto add_consequence = [&consequences]( const std::string & msg, edible_rating code ) {
        consequences.emplace_back( ret_val<edible_rating>::make_failure( code, msg ) );
    };

    const bool saprophage = has_trait( trait_SAPROPHAGE );
    const auto &comest = food.get_comestible();

    if( food.rotten() ) {
        const bool saprovore = has_trait( trait_SAPROVORE );
        if( !saprophage && !saprovore ) {
            add_consequence( _( "This is rotten and smells awful!" ), edible_rating::rotten );
        }
    }

    const bool carnivore = has_trait( trait_CARNIVORE );
    if( food.has_flag( flag_CANNIBALISM ) && !has_trait_flag( "CANNIBAL" ) ) {
        add_consequence( _( "The thought of eating human flesh makes you feel sick." ),
                         edible_rating::cannibalism );
    }

    const bool edible = comest->comesttype == comesttype_FOOD || food.has_flag( flag_USE_EAT_VERB );

    int food_kcal = compute_effective_nutrients( food ).kcal;
    if( food_kcal > 0 && has_effect( effect_nausea ) ) {
        add_consequence( _( "You still feel nauseous and will probably puke it all up again." ),
                         edible_rating::nausea );
    }

    if( ( food_kcal > 0 || comest->quench > 0 ) && has_effect( effect_bloated ) ) {
        add_consequence( _( "You're full and will vomit if you try to consume anything." ),
                         edible_rating::bloated );
    }

    if( ( allergy_type( food ) != MORALE_NULL ) || ( carnivore && food.has_flag( flag_ALLERGEN_JUNK ) &&
            !food.has_flag( flag_CARNIVORE_OK ) ) ) {
        add_consequence( _( "Your stomach won't be happy (allergy)." ), edible_rating::allergy );
    }

    if( saprophage && edible && food.rotten() && !food.has_flag( flag_FERTILIZER ) ) {
        // Note: We're allowing all non-solid "food". This includes drugs
        // Hard-coding fertilizer for now - should be a separate flag later
        //~ No, we don't eat "rotten" food. We eat properly aged food, like a normal person.
        //~ Semantic difference, but greatly facilitates people being proud of their character.
        add_consequence( _( "Your stomach won't be happy (not rotten enough)." ),
                         edible_rating::allergy_weak );
    }

    if( !food.has_infinite_charges() &&
        ( ( food_kcal > 0 &&
            get_stored_kcal() + stomach.get_calories() + food_kcal
            > max_stored_calories() ) ||
          ( comest->quench > 0 && get_thirst() < comest->quench ) ) ) {
        add_consequence( _( "You're full already and the excess food will be wasted." ),
                         edible_rating::too_full );
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
        std::string food_tname = food.tname();
        const nc_color food_color = food.color_in_inventory();
        if( eat_verb || comest->comesttype == comesttype_FOOD ) {
            req += string_format( _( "Eat your %s anyway?" ), colorize( food_tname, food_color ) );
        } else if( !eat_verb && comest->comesttype == comesttype_DRINK ) {
            req += string_format( _( "Drink your %s anyway?" ), colorize( food_tname, food_color ) );
        } else {
            req += string_format( _( "Consume your %s anyway?" ), colorize( food_tname, food_color ) );
        }

        if( !query_yn( req ) ) {
            return consequences.front();
        }
    }
    // All checks ended, it's edible (or we're pretending it is)
    return ret_val<edible_rating>::make_success();
}

bool player::eat( item &food, bool force )
{
    if( !food.is_food() ) {
        return false;
    }

    const auto ret = force ? can_eat( food ) : will_eat( food, is_player() );
    if( !ret.success() ) {
        return false;
    }

    if( has_effect( effect_bloated ) &&
        ( compute_effective_nutrients( food ).kcal > 0 || food.get_comestible()->quench > 0 ) ) {
        add_msg_if_player( _( "You force yourself to vomit to make space for %s." ), food.tname() );
        vomit();
    }

    int charges_used = 0;
    if( food.type->has_use() ) {
        if( !food.type->can_use( "DOGFOOD" ) &&
            !food.type->can_use( "CATFOOD" ) &&
            !food.type->can_use( "BIRDFOOD" ) &&
            !food.type->can_use( "CATTLEFODDER" ) ) {
            charges_used = food.type->invoke( *this, food, pos() );
            if( charges_used <= 0 ) {
                return false;
            }
        }
    }

    // Note: the block below assumes we decided to eat it
    // No coming back from here

    const int nutr = nutrition_for( food );
    const bool spoiled = food.rotten();

    // The item is solid food
    const bool chew = food.get_comestible()->comesttype == comesttype_FOOD ||
                      food.has_flag( flag_USE_EAT_VERB );
    // This item is a drink and not a solid food (and not a thick soup)
    const bool drinkable = !chew && food.get_comestible()->comesttype == comesttype_DRINK;
    // If neither of the above is true then it's a drug and shouldn't get mealtime penalty/bonus

    const bool saprophage = has_trait( trait_SAPROPHAGE );
    if( spoiled && !saprophage ) {
        add_msg_if_player( m_bad, _( "Ick, this %s doesn't taste so good…" ), food.tname() );
        if( !has_trait( trait_SAPROVORE ) && !has_trait( trait_EATDEAD ) &&
            ( !has_bionic( bio_digestion ) || one_in( 3 ) ) ) {
            add_effect( effect_foodpoison, rng( 6_minutes, ( nutr + 1 ) * 6_minutes ) );
        }
    } else if( spoiled && saprophage ) {
        add_msg_if_player( m_good, _( "Mmm, this %s tastes delicious…" ), food.tname() );
    }
    if( !consume_effects( food ) ) {
        // Already consumed by using `food.type->invoke`?
        if( charges_used > 0 ) {
            food.mod_charges( -charges_used );
        }
        return false;
    }
    food.mod_charges( -1 );

    const bool amorphous = has_trait( trait_AMORPHOUS );
    int mealtime = 250;
    if( drinkable || chew ) {
        // Those bonuses/penalties only apply to food
        // Not to smoking weed or applying bandages!
        if( has_trait( trait_MOUTH_TENTACLES )  || has_trait( trait_MANDIBLES ) ||
            has_trait( trait_FANGS_SPIDER ) ) {
            mealtime /= 2;
        } else if( has_trait( trait_SHARKTEETH ) ) {
            // SHARKBAIT! HOO HA HA!
            mealtime /= 3;
        } else if( has_trait( trait_GOURMAND ) ) {
            // Don't stack those two - that would be 25 moves per item
            mealtime -= 100;
        }

        if( has_trait( trait_BEAK_HUM ) && !drinkable ) {
            // Much better than PROBOSCIS but still optimized for fluids
            mealtime += 200;
        } else if( has_trait( trait_SABER_TEETH ) ) {
            // They get In The Way
            mealtime += 250;
        }

        if( amorphous ) {
            mealtime *= 1.1;
            // Minor speed penalty for having to flow around it
            // rather than just grab & munch
        }
    }

    moves -= mealtime;

    // If it's poisonous... poison us.
    // TODO: Move this to a flag
    if( food.poison > 0 && !has_trait( trait_EATPOISON ) &&
        !has_trait( trait_EATDEAD ) ) {
        if( food.poison >= rng( 2, 4 ) ) {
            add_effect( effect_poison, food.poison * 10_minutes );
        }

        add_effect( effect_foodpoison, food.poison * 30_minutes );
    }

    if( food.has_flag( flag_HIDDEN_HALLU ) ) {
        if( !has_effect( effect_hallu ) ) {
            add_effect( effect_hallu, 6_hours );
        }
    }

    if( amorphous ) {
        add_msg_player_or_npc( _( "You assimilate your %s." ), _( "<npcname> assimilates a %s." ),
                               food.tname() );
    } else if( drinkable ) {
        add_msg_player_or_npc( _( "You drink your %s." ), _( "<npcname> drinks a %s." ),
                               food.tname() );
    } else if( chew ) {
        add_msg_player_or_npc( _( "You eat your %s." ), _( "<npcname> eats a %s." ),
                               food.tname() );
    }

    if( item::find_type( food.get_comestible()->tool )->tool ) {
        // Tools like lighters get used
        use_charges( food.get_comestible()->tool, 1 );
    }

    if( has_active_bionic( bio_taste_blocker ) ) {
        mod_power_level( units::from_kilojoule( -std::min( 0, food.get_comestible_fun() ) ) );
    }

    if( food.has_flag( flag_FUNGAL_VECTOR ) && !has_trait( trait_M_IMMUNE ) ) {
        add_effect( effect_fungus, 1_turns, num_bp, true );
    }

    // Chance to become parasitised
    if( !( has_bionic( bio_digestion ) || has_trait( trait_PARAIMMUNE ) ) ) {
        if( food.get_comestible()->parasites > 0 && !food.has_flag( flag_NO_PARASITES ) &&
            one_in( food.get_comestible()->parasites ) ) {
            switch( rng( 0, 3 ) ) {
                case 0:
                    add_effect( effect_tapeworm, 1_turns, num_bp, true );
                    break;
                case 1:
                    if( !has_trait( trait_ACIDBLOOD ) ) {
                        add_effect( effect_bloodworms, 1_turns, num_bp, true );
                    }
                    break;
                case 2:
                    add_effect( effect_brainworms, 1_turns, num_bp, true );
                    break;
                case 3:
                    add_effect( effect_paincysts, 1_turns, num_bp, true );
            }
        }
    }

    for( const std::pair<const diseasetype_id, int> &elem : food.get_comestible()->contamination ) {
        if( rng( 1, 100 ) <= elem.second ) {
            expose_to_disease( elem.first );
        }
    }

    consumption_history.emplace_back( food );
    // Clean out consumption_history so it doesn't get bigger than needed.
    while( consumption_history.front().time < calendar::turn - 2_days ) {
        consumption_history.pop_front();
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
    const int current_stim = get_stim();
    if( comest.stim != 0 &&
        ( std::abs( current_stim ) < ( std::abs( comest.stim ) * 3 ) ||
          sgn( current_stim ) != sgn( comest.stim ) ) ) {
        if( comest.stim < 0 ) {
            set_stim( std::max( comest.stim * 3, current_stim + comest.stim ) );
        } else {
            set_stim( std::min( comest.stim * 3, current_stim + comest.stim ) );
        }
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

void Character::modify_morale( item &food, const int )
{
    time_duration morale_time = 2_hours;

    std::pair<int, int> fun = fun_for( food );
    if( fun.first < 0 ) {
        add_morale( MORALE_FOOD_BAD, fun.first, fun.second, morale_time, morale_time / 2, false,
                    food.type );
    } else if( fun.first > 0 ) {
        add_morale( MORALE_FOOD_GOOD, fun.first, fun.second, morale_time, morale_time / 2, false,
                    food.type );
    }

    if( food.has_flag( flag_HIDDEN_HALLU ) ) {
        if( has_trait( trait_SPIRITUAL ) ) {
            add_morale( MORALE_FOOD_GOOD, 36, 72, 2_hours, 1_hours, false );
        } else {
            add_morale( MORALE_FOOD_GOOD, 18, 36, 1_hours, 30_minutes, false );
        }
    }

    if( food.has_flag( flag_CANNIBALISM ) ) {
        const bool cannibal = has_trait( trait_CANNIBAL );
        const bool psycho = has_trait( trait_PSYCHOPATH );
        const bool sapiovore = has_trait( trait_SAPIOVORE );
        if( cannibal ) {
            add_msg_if_player( m_good, _( "You indulge your shameful hunger." ) );
            add_morale( MORALE_CANNIBAL, 20, 200 );
        } else if( psycho || sapiovore ) {
            // Nothing - doesn't care enough to print a message
        } else {
            add_msg_if_player( m_bad, _( "You feel horrible for eating a person." ) );
            add_morale( MORALE_CANNIBAL, -60, -400, 60_minutes, 30_minutes );
        }
    }

    // Allergy check for food that is ingested (not gum)
    if( !food.has_flag( "NO_INGEST" ) ) {
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
        mutation_category_level["URSINE"] > 40 ) {
        // Need at least 5 bear mutations for effect to show, to filter out mutations in common with other categories
        int honey_fun = has_trait( trait_THRESH_URSINE ) ?
                        std::min( mutation_category_level["URSINE"] / 8, 20 ) :
                        mutation_category_level["URSINE"] / 12;
        if( honey_fun < 10 ) {
            add_msg_if_player( m_good, _( "You find the sweet taste of honey surprisingly palatable." ) );
        } else {
            add_msg_if_player( m_good, _( "You feast upon the sweet honey." ) );
        }
        add_morale( MORALE_HONEY, honey_fun, 100 );
    }
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
    if( relative_rot > 1.0f && !has_trait( trait_SAPROPHAGE ) &&
        !has_trait( trait_SAPROVORE ) && !has_bionic( bio_digestion ) ) {
        const float rottedness = clamp( 2 * relative_rot - 2.0f, 0.1f, 1.0f );
        // ~-1 health per 1 nutrition at halfway-rotten-away, ~0 at "just got rotten"
        // But always round down
        int h_loss = -rottedness * comest.get_default_nutr();
        mod_healthy_mod( h_loss, -200 );
        add_msg( m_debug, "%d health from %0.2f%% rotten food", h_loss, rottedness );
    }

    // Used in hibernation messages.
    const auto nutr = nutrition_for( food );
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

    // Moved here and changed a bit - it was too complex
    // Incredibly minor stuff like this shouldn't require complexity
    if( !is_npc() && has_trait( trait_SLIMESPAWNER ) &&
        max_stored_calories() < get_stored_kcal() + 4000 && get_thirst() < thirst_levels::slaked ) {
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

    // Set up food for ingestion
    const item &contained_food = food.is_container() ? food.get_contained() : food;
    food_summary ingested{
        compute_effective_nutrients( contained_food )
    };
    // Maybe move tapeworm to digestion
    if( has_effect( effect_tapeworm ) ) {
        ingested.nutr /= 2;
    }

    int excess_kcal = get_stored_kcal() + stomach.get_calories() + ingested.nutr.kcal -
                      max_stored_calories();
    int excess_quench = -( get_thirst() - comest.quench );
    stomach.ingest( ingested );
    mod_thirst( -contained_food.type->comestible->quench );

    if( excess_kcal > 0 || excess_quench > 0 ) {
        add_effect( effect_bloated, 5_minutes );
    }

    return true;
}

hint_rating Character::rate_action_eat( const item &it ) const
{
    if( !can_consume( it ) ) {
        return hint_rating::cant;
    }

    const auto rating = will_eat( it );
    if( rating.success() ) {
        return hint_rating::good;
    } else if( rating.value() == edible_rating::inedible ||
               rating.value() == edible_rating::inedible_mutation ) {

        return hint_rating::cant;
    }

    return hint_rating::iffy;
}

bool Character::can_feed_reactor_with( const item &it ) const
{
    static const std::set<ammotype> acceptable = {{
            ammotype( "reactor_slurry" ),
            ammotype( "plutonium" )
        }
    };

    if( !it.is_ammo() || can_eat( it ).success() ) {
        return false;
    }

    if( !has_active_bionic( bio_reactor ) && !has_active_bionic( bio_advreactor ) ) {
        return false;
    }

    return std::any_of( acceptable.begin(), acceptable.end(), [ &it ]( const ammotype & elem ) {
        return it.ammo_type() == elem;
    } );
}

bool Character::feed_reactor_with( item &it )
{
    if( !can_feed_reactor_with( it ) ) {
        return false;
    }

    const auto iter = plut_charges.find( it.typeId() );
    const int max_amount = iter != plut_charges.end() ? iter->second : 0;
    const int amount = std::min( get_acquirable_energy( it, rechargeable_cbm::reactor ), max_amount );

    if( amount >= PLUTONIUM_CHARGES * 10 &&
        !query_yn( _( "That is a LOT of plutonium.  Are you sure you want that much?" ) ) ) {
        return false;
    }

    add_msg_player_or_npc( _( "You add your %s to your reactor's tank." ),
                           _( "<npcname> pours %s into their reactor's tank." ),
                           it.tname() );

    // TODO: Encapsulate
    tank_plut += amount;
    it.charges -= 1;
    mod_moves( -250 );
    return true;
}

bool Character::can_feed_furnace_with( const item &it ) const
{
    if( !it.flammable() || it.has_flag( flag_RADIOACTIVE ) || can_eat( it ).success() ) {
        return false;
    }

    if( !has_active_bionic( bio_furnace ) ) {
        return false;
    }

    // Not even one charge fits
    if( it.charges_per_volume( furnace_max_volume ) < 1 ) {
        return false;
    }

    return !it.has_flag( flag_CORPSE );
}

bool Character::feed_furnace_with( item &it )
{
    if( !can_feed_furnace_with( it ) ) {
        return false;
    }
    if( it.is_favorite &&
        !g->u.query_yn( _( "Are you sure you want to eat your favorited %s?" ), it.tname() ) ) {
        return false;
    }

    const int consumed_charges =  std::min( it.charges, it.charges_per_volume( furnace_max_volume ) );
    const int energy =  get_acquirable_energy( it, rechargeable_cbm::furnace );

    if( energy == 0 ) {
        add_msg_player_or_npc( m_info,
                               _( "You digest your %s, but fail to acquire energy from it." ),
                               _( "<npcname> digests their %s for energy, but fails to acquire energy from it." ),
                               it.tname() );
    } else if( is_max_power() ) {
        add_msg_player_or_npc(
            _( "You digest your %s, but you're fully powered already, so the energy is wasted." ),
            _( "<npcname> digests a %s for energy, they're fully powered already, so the energy is wasted." ),
            it.tname() );
    } else {
        const int profitable_energy = std::min( energy,
                                                units::to_kilojoule( get_max_power_level() - get_power_level() ) );
        if( it.count_by_charges() ) {
            add_msg_player_or_npc( m_info,
                                   ngettext( "You digest %d %s and recharge %d point of energy.",
                                             "You digest %d %s and recharge %d points of energy.",
                                             profitable_energy
                                           ),
                                   ngettext( "<npcname> digests %d %s and recharges %d point of energy.",
                                             "<npcname> digests %d %s and recharges %d points of energy.",
                                             profitable_energy
                                           ), consumed_charges, it.tname(), profitable_energy
                                 );
        } else {
            add_msg_player_or_npc( m_info,
                                   ngettext( "You digest your %s and recharge %d point of energy.",
                                             "You digest your %s and recharge %d points of energy.",
                                             profitable_energy
                                           ),
                                   ngettext( "<npcname> digests a %s and recharges %d point of energy.",
                                             "<npcname> digests a %s and recharges %d points of energy.",
                                             profitable_energy
                                           ), it.tname(), profitable_energy
                                 );
        }
        mod_power_level( units::from_kilojoule( profitable_energy ) );
    }

    it.charges -= consumed_charges;
    mod_moves( -250 );

    return true;
}

bool Character::fuel_bionic_with( item &it )
{
    if( !can_fuel_bionic_with( it ) ) {
        return false;
    }

    const bionic_id bio = get_most_efficient_bionic( get_bionic_fueled_with( it ) );

    const int loadable = std::min( it.charges, get_fuel_capacity( it.typeId() ) );
    const std::string str_loaded  = get_value( it.typeId() );
    int loaded = 0;
    if( !str_loaded.empty() ) {
        loaded = std::stoi( str_loaded );
    }

    const std::string new_charge = std::to_string( loadable + loaded );

    it.charges -= loadable;
    // Type and amount of fuel
    set_value( it.typeId(), new_charge );
    update_fuel_storage( it.typeId() );
    add_msg_player_or_npc( m_info,
                           //~ %1$i: charge number, %2$s: item name, %3$s: bionics name
                           ngettext( "You load %1$i charge of %2$s in your %3$s.",
                                     "You load %1$i charges of %2$s in your %3$s.", loadable ),
                           //~ %1$i: charge number, %2$s: item name, %3$s: bionics name
                           ngettext( "<npcname> load %1$i charge of %2$s in their %3$s.",
                                     "<npcname> load %1$i charges of %2$s in their %3$s.", loadable ), loadable, it.tname(), bio->name );
    mod_moves( -250 );
    return true;
}

rechargeable_cbm Character::get_cbm_rechargeable_with( const item &it ) const
{
    if( can_feed_reactor_with( it ) ) {
        return rechargeable_cbm::reactor;
    }

    if( can_feed_furnace_with( it ) ) {
        return rechargeable_cbm::furnace;
    }

    if( can_fuel_bionic_with( it ) ) {
        return rechargeable_cbm::other;
    }

    return rechargeable_cbm::none;
}

int Character::get_acquirable_energy( const item &it, rechargeable_cbm cbm ) const
{
    switch( cbm ) {
        case rechargeable_cbm::none:
            break;

        case rechargeable_cbm::reactor:
            if( it.charges > 0 ) {
                const auto iter = plut_charges.find( it.typeId() );
                return iter != plut_charges.end() ? it.charges * iter->second : 0;
            }

            break;

        case rechargeable_cbm::furnace: {
            units::volume consumed_vol = it.volume();
            units::mass consumed_mass = it.weight();
            if( it.count_by_charges() && it.charges > it.charges_per_volume( furnace_max_volume ) ) {
                const double n_stacks = static_cast<double>( it.charges_per_volume( furnace_max_volume ) ) /
                                        it.type->stack_size;
                consumed_vol = it.type->volume * n_stacks;
                // it.type->weight is in 10g units?
                consumed_mass = it.type->weight * 10 * n_stacks;
            }
            int amount = ( consumed_vol / 250_ml + consumed_mass / 1_gram ) / 9;

            // TODO: JSONize.
            if( it.made_of( material_id( "leather" ) ) ) {
                amount /= 4;
            }
            if( it.made_of( material_id( "wood" ) ) ) {
                amount /= 2;
            }

            return amount;
        }
        case rechargeable_cbm::other:
            const bionic_id &bid = get_most_efficient_bionic( get_bionic_fueled_with( it ) );
            const int to_consume = std::min( it.charges, bid->fuel_capacity );
            const int to_charge = static_cast<int>( it.fuel_energy() * to_consume * bid->fuel_efficiency );
            return to_charge;
    }

    return 0;
}

int Character::get_acquirable_energy( const item &it ) const
{
    return get_acquirable_energy( it, get_cbm_rechargeable_with( it ) );
}

bool Character::can_estimate_rot() const
{
    return get_skill_level( skill_cooking ) >= 3 || get_skill_level( skill_survival ) >= 4;
}

bool Character::can_consume_as_is( const item &it ) const
{
    return it.is_comestible() || can_consume_for_bionic( it );
}

bool Character::can_consume_for_bionic( const item &it ) const
{
    return get_cbm_rechargeable_with( it ) != rechargeable_cbm::none;
}

bool Character::can_consume( const item &it ) const
{
    if( can_consume_as_is( it ) ) {
        return true;
    }
    // Checking NO_RELOAD to prevent consumption of `battery` when contained in `battery_car` (#20012)
    return !it.is_container_empty() && !it.has_flag( flag_NO_RELOAD ) &&
           can_consume_as_is( it.contents.front() );
}

item &Character::get_consumable_from( item &it ) const
{
    if( !it.is_container_empty() && can_consume_as_is( it.contents.front() ) ) {
        return it.contents.front();
    } else if( can_consume_as_is( it ) ) {
        return it;
    }

    static item null_comestible;
    // Since it's not const.
    null_comestible = item();
    return null_comestible;
}
