#include "player.h" // IWYU pragma: associated

#include <algorithm>
#include <string>

#include "addiction.h"
#include "cata_utility.h"
#include "debug.h"
#include "game.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "material.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mutation.h"
#include "options.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "vitamin.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_reference.h"

namespace
{
const skill_id skill_survival( "survival" );
const skill_id skill_cooking( "cooking" );

const efftype_id effect_foodpoison( "foodpoison" );
const efftype_id effect_poison( "poison" );
const efftype_id effect_tapeworm( "tapeworm" );
const efftype_id effect_bloodworms( "bloodworms" );
const efftype_id effect_brainworms( "brainworms" );
const efftype_id effect_paincysts( "paincysts" );
const efftype_id effect_nausea( "nausea" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_common_cold( "common_cold" );
const efftype_id effect_flu( "flu" );

const mtype_id mon_player_blob( "mon_player_blob" );

const bionic_id bio_advreactor( "bio_advreactor" );
const bionic_id bio_batteries( "bio_batteries" );
const bionic_id bio_digestion( "bio_digestion" );
const bionic_id bio_ethanol( "bio_ethanol" );
const bionic_id bio_furnace( "bio_furnace" );
const bionic_id bio_reactor( "bio_reactor" );
const bionic_id bio_taste_blocker( "bio_taste_blocker" );

const std::vector<std::string> carnivore_blacklist {{
        "ALLERGEN_VEGGY", "ALLERGEN_FRUIT", "ALLERGEN_WHEAT",
    }
};
// This ugly temp array is here because otherwise it goes
// std::vector(char*, char*)->vector(InputIterator,InputIterator) or some such
const std::array<std::string, 2> temparray {{"ALLERGEN_MEAT", "ALLERGEN_EGG"}};
const std::vector<std::string> herbivore_blacklist( temparray.begin(), temparray.end() );

// Defines the maximum volume that a internal furnace can consume
const units::volume furnace_max_volume( 3000_ml ) ;

// TODO: JSONize.
const std::map<itype_id, int> plut_charges = {
    { "plut_cell",         PLUTONIUM_CHARGES * 10 },
    { "plut_slurry_dense", PLUTONIUM_CHARGES },
    { "plut_slurry",       PLUTONIUM_CHARGES / 2 }
};

}

int player::stomach_capacity() const
{
    if( has_trait( trait_id( "GIZZARD" ) ) ) {
        return 0;
    }

    if( has_active_mutation( trait_id( "HIBERNATE" ) ) ) {
        return -620;
    }

    if( has_trait( trait_id( "GOURMAND" ) ) || has_trait( trait_id( "HIBERNATE" ) ) ) {
        return -60;
    }

    return -20;
}

// TODO: Move pizza scraping here.
// Same for other kinds of nutrition alterations
// This is used by item display, making actual nutrition available to player.
int player::kcal_for( const item &comest ) const
{
    static const trait_id trait_CARNIVORE( "CARNIVORE" );
    static const trait_id trait_GIZZARD( "GIZZARD" );
    static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
    static const std::string flag_CARNIVORE_OK( "CARNIVORE_OK" );
    if( !comest.is_comestible() ) {
        return 0;
    }

    // As float to avoid rounding too many times
    float kcal = 0;

    // if item has components, will derive calories from that instead.
    if( !comest.components.empty() && !comest.has_flag( "NUTRIENT_OVERRIDE" ) ) {
        int byproduct_multiplier;
        for( const item &component : comest.components ) {
            component.has_flag( "BYPRODUCT" ) ? byproduct_multiplier = -1 : byproduct_multiplier = 1;
            kcal += this->kcal_for( component ) * component.charges * byproduct_multiplier;
        }
        kcal /= comest.recipe_charges;
    } else {
        kcal = comest.type->comestible->get_calories();
    }

    if( has_trait( trait_GIZZARD ) ) {
        kcal *= 0.6f;
    }

    if( has_trait( trait_CARNIVORE ) && comest.has_flag( flag_CARNIVORE_OK ) &&
        comest.has_any_flag( carnivore_blacklist ) ) {
        // TODO: Comment pizza scrapping
        kcal *= 0.5f;
    }

    const float relative_rot = comest.get_relative_rot();
    // Saprophages get full nutrition from rotting food
    if( relative_rot > 1.0f && !has_trait( trait_SAPROPHAGE ) ) {
        // everyone else only gets a portion of the nutrition
        // Scaling linearly from 100% at just-rotten to 0 at halfway-rotten-away
        const float rottedness = clamp( 2 * relative_rot - 2.0f, 0.1f, 1.0f );
        kcal *= ( 1.0f - rottedness );
    }

    // Bionic digestion gives extra nutrition
    if( has_bionic( bio_digestion ) ) {
        kcal *= 1.5f;
    }

    return static_cast<int>( kcal );
}

int player::nutrition_for( const item &comest ) const
{
    return kcal_for( comest ) / islot_comestible::kcal_per_nutr;
}

std::pair<int, int> player::fun_for( const item &comest ) const
{
    static const trait_id trait_GOURMAND( "GOURMAND" );
    static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
    static const trait_id trait_SAPROVORE( "SAPROVORE" );
    static const std::string flag_EATEN_COLD( "EATEN_COLD" );
    static const std::string flag_COLD( "COLD" );
    static const std::string flag_FROZEN( "FROZEN" );
    static const std::string flag_MUSHY( "MUSHY" );
    static const std::string flag_MELTS( "MELTS" );
    if( !comest.is_comestible() ) {
        return std::pair<int, int>( 0, 0 );
    }

    // As float to avoid rounding too many times
    float fun = comest.type->comestible->fun;
    if( comest.has_flag( flag_MUSHY ) && fun > -5.0f ) {
        fun = -5.0f; // defrosted MUSHY food is practicaly tastless or tastes off
    }
    if( ( has_effect( effect_common_cold ) || has_effect( effect_flu ) ) && fun > 0 ) {
        fun /= 3; // food doesn't taste as good when you're sick
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

    float fun_max = fun < 0 ? fun * 6 : fun * 3;
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
            fun *= 1.25; // melted freezable food tastes 25% worse than frozen freezable food
            // frozen freezable food... say that 5 times fast
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
        power_level > abs( comest.type->comestible->fun ) &&
        comest.type->comestible->fun < 0 ) {
        fun = 0;
    }

    return { static_cast< int >( fun ), static_cast< int >( fun_max ) };
}

std::map<vitamin_id, int> player::vitamins_from( const itype_id &id ) const
{
    return vitamins_from( item( id ) );
}

// list of traits the player has that modifies vitamin absorption
std::list<trait_id> mut_vitamin_absorb_modify( const player &p )
{
    std::list<trait_id> traits;
    for( auto &m : p.get_mutations() ) {
        const auto &mut = m.obj();
        if( !mut.vitamin_absorb_multi.empty() ) {
            traits.push_back( m );
        }
    }
    return traits;
}

// is the material associated with this item?
bool material_exists( const material_id &material, const item &item )
{
    for( const material_id &mat : item.type->materials ) {
        if( mat == material ) {
            return true;
        }
    }
    return false;
}

std::map<vitamin_id, int> player::vitamins_from( const item &it ) const
{
    std::map<vitamin_id, int> res;

    if( !it.type->comestible ) {
        return res;
    }

    if( !it.components.empty() && !it.has_flag( "NUTRIENT_OVERRIDE" ) ) {
        // if an item is a byproduct, it should subtract the calories and vitamins instead of add
        int byproduct_multiplier = 1;
        for( const auto &comp : it.components ) {
            comp.has_flag( "BYPRODUCT" ) ? byproduct_multiplier = -1 : byproduct_multiplier = 1;
            std::map<vitamin_id, int> component_map = this->vitamins_from( comp );
            for( const auto &vit : component_map ) {
                res[ vit.first ] += byproduct_multiplier * ceil( static_cast<float>
                                    ( vit.second ) / static_cast<float>
                                    ( it.type->charges_default() ) );
            }
        }
    } else {
        // if we're here, whatever is returned is going to be based on the item's defined stats
        res = it.type->comestible->vitamins;
        std::list<trait_id> traits = mut_vitamin_absorb_modify( *this );
        // traits modify the absorption of vitamins here
        if( !traits.empty() ) {
            // make sure to iterate over every trait that has an effect on vitamin absorption
            for( const trait_id &trait : traits ) {
                const auto &mut = trait.obj();
                // make sure to iterate over every material defined for vitamin absorption
                // TODO: put this loop into a function and utilize it again for bionics
                for( const auto &mat : mut.vitamin_absorb_multi ) {
                    // this is where we are able to check if the food actually is changed by the trait
                    if( mat.first == material_id( "all" ) || material_exists( mat.first, it ) ) {
                        std::map<vitamin_id, double> mat_vit_map = mat.second;
                        // finally iterate over every vitamin in each material
                        for( const auto &vit : res ) {
                            // to avoid errors with undefined keys, and to initialize numbers to 1 if undefined
                            mat_vit_map.emplace( vit.first, 1 );
                            // finally edit the vitamin value that will be returned
                            res[ vit.first ] *= mat_vit_map[ vit.first ];
                        }
                    }
                }
            }
        }
    }

    return res;
}

time_duration player::vitamin_rate( const vitamin_id &vit ) const
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

int player::vitamin_mod( const vitamin_id &vit, int qty, bool capped )
{
    auto it = vitamin_levels.find( vit );
    if( it == vitamin_levels.end() ) {
        return 0;
    }
    const auto &v = it->first.obj();

    if( qty > 0 ) {
        // accumulations can never occur from food sources
        it->second = std::min( it->second + qty, capped ? 0 : v.max() );
        update_vitamins( vit );

    } else if( qty < 0 ) {
        it->second = std::max( it->second + qty, v.min() );
        update_vitamins( vit );
    }

    return it->second;
}

int player::vitamin_get( const vitamin_id &vit ) const
{
    if( get_option<bool>( "NO_VITAMINS" ) ) {
        return 0;
    }

    const auto &v = vitamin_levels.find( vit );
    return v != vitamin_levels.end() ? v->second : 0;
}

bool player::vitamin_set( const vitamin_id &vit, int qty )
{
    auto v = vitamin_levels.find( vit );
    if( v == vitamin_levels.end() ) {
        return false;
    }
    vitamin_mod( vit, qty - v->second, false );

    return true;
}

float player::metabolic_rate_base() const
{
    float hunger_rate = get_option< float >( "PLAYER_HUNGER_RATE" );
    return hunger_rate * ( 1.0f + mutation_value( "metabolism_modifier" ) );
}

// TODO: Make this less chaotic to let NPC retroactive catch up work here
// TODO: Involve body heat (cold -> higher metabolism, unless cold-blooded)
// TODO: Involve stamina (maybe not here?)
float player::metabolic_rate() const
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

morale_type player::allergy_type( const item &food ) const
{
    using allergy_tuple = std::tuple<trait_id, std::string, morale_type>;
    static const std::array<allergy_tuple, 8> allergy_tuples = {{
            std::make_tuple( trait_id( "VEGETARIAN" ), "ALLERGEN_MEAT", MORALE_VEGETARIAN ),
            std::make_tuple( trait_id( "MEATARIAN" ), "ALLERGEN_VEGGY", MORALE_MEATARIAN ),
            std::make_tuple( trait_id( "LACTOSE" ), "ALLERGEN_MILK", MORALE_LACTOSE ),
            std::make_tuple( trait_id( "ANTIFRUIT" ), "ALLERGEN_FRUIT", MORALE_ANTIFRUIT ),
            std::make_tuple( trait_id( "ANTIJUNK" ), "ALLERGEN_JUNK", MORALE_ANTIJUNK ),
            std::make_tuple( trait_id( "ANTIWHEAT" ), "ALLERGEN_WHEAT", MORALE_ANTIWHEAT )
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

ret_val<edible_rating> player::can_eat( const item &food ) const
{

    const auto &comest = food.type->comestible;
    if( !comest ) {
        return ret_val<edible_rating>::make_failure( _( "That doesn't look edible." ) );
    }

    if( food.item_tags.count( "DIRTY" ) ) {
        return ret_val<edible_rating>::make_failure(
                   _( "This is full of dirt after being on the ground." ) );
    }

    const bool eat_verb  = food.has_flag( "USE_EAT_VERB" );
    const bool edible    = eat_verb ||  comest->comesttype == "FOOD";
    const bool drinkable = !eat_verb && comest->comesttype == "DRINK";

    // TODO: This condition occurs way too often. Unify it.
    // update Sep. 26 2018: this apparently still occurs way too often. yay!
    if( is_underwater() && !has_trait( trait_id( "WATERSLEEP" ) ) ) {
        return ret_val<edible_rating>::make_failure( _( "You can't do that while underwater." ) );
    }

    if( edible || drinkable ) {
        for( const auto &elem : food.type->materials ) {
            if( !elem->edible() ) {
                return ret_val<edible_rating>::make_failure( _( "That doesn't look edible in its current form." ) );
            }
        }
    }
    if( food.item_tags.count( "FROZEN" ) && !food.has_flag( "EDIBLE_FROZEN" ) &&
        !food.has_flag( "MELTS" ) ) {
        if( edible ) {
            return ret_val<edible_rating>::make_failure(
                       _( "It's frozen solid.  You must defrost it before you can eat it." ) );
        }
        if( drinkable ) {
            return ret_val<edible_rating>::make_failure( _( "You can't drink it while it's frozen." ) );
        }
    }

    if( comest->tool != "null" ) {
        const bool has = item::count_by_charges( comest->tool )
                         ? has_charges( comest->tool, 1 )
                         : has_amount( comest->tool, 1 );
        if( !has ) {
            return ret_val<edible_rating>::make_failure( NO_TOOL,
                    string_format( _( "You need a %s to consume that!" ),
                                   item::nname( comest->tool ).c_str() ) );
        }
    }

    // For all those folks who loved eating marloss berries.  D:< mwuhahaha
    if( has_trait( trait_id( "M_DEPENDENT" ) ) && !food.has_flag( "MYCUS_OK" ) ) {
        return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION,
                _( "We can't eat that.  It's not right for us." ) );
    }
    // Here's why PROBOSCIS is such a negative trait.
    if( has_trait( trait_id( "PROBOSCIS" ) ) && !( drinkable || food.is_medication() ) ) {
        return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION, _( "Ugh, you can't drink that!" ) );
    }

    if( has_trait( trait_id( "CARNIVORE" ) ) && nutrition_for( food ) > 0 &&
        food.has_any_flag( carnivore_blacklist ) && !food.has_flag( "CARNIVORE_OK" ) ) {
        return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION,
                _( "Eww.  Inedible plant stuff!" ) );
    }

    if( ( has_trait( trait_id( "HERBIVORE" ) ) || has_trait( trait_id( "RUMINANT" ) ) ) &&
        food.has_any_flag( herbivore_blacklist ) ) {
        // Like non-cannibal, but more strict!
        return ret_val<edible_rating>::make_failure( INEDIBLE_MUTATION,
                _( "The thought of eating that makes you feel sick." ) );
    }

    return ret_val<edible_rating>::make_success();
}

ret_val<edible_rating> player::will_eat( const item &food, bool interactive ) const
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

    const bool saprophage = has_trait( trait_id( "SAPROPHAGE" ) );
    const auto &comest = food.type->comestible;

    if( food.rotten() ) {
        const bool saprovore = has_trait( trait_id( "SAPROVORE" ) );
        if( !saprophage && !saprovore ) {
            add_consequence( _( "This is rotten and smells awful!" ), ROTTEN );
        }
    }

    const bool carnivore = has_trait( trait_id( "CARNIVORE" ) );
    if( food.has_flag( "CANNIBALISM" ) && !has_trait_flag( "CANNIBAL" ) ) {
        add_consequence( _( "The thought of eating human flesh makes you feel sick." ), CANNIBALISM );
    }

    const bool edible = comest->comesttype == "FOOD" || food.has_flag( "USE_EAT_VERB" );

    if( edible && has_effect( effect_nausea ) ) {
        add_consequence( _( "You still feel nauseous and will probably puke it all up again." ), NAUSEA );
    }

    if( ( allergy_type( food ) != MORALE_NULL ) || ( carnivore && food.has_flag( "ALLERGEN_JUNK" ) &&
            !food.has_flag( "CARNIVORE_OK" ) ) ) {
        add_consequence( _( "Your stomach won't be happy (allergy)." ), ALLERGY );
    }

    if( saprophage && edible && food.rotten() && !food.has_flag( "FERTILIZER" ) ) {
        // Note: We're allowing all non-solid "food". This includes drugs
        // Hard-coding fertilizer for now - should be a separate flag later
        //~ No, we don't eat "rotten" food. We eat properly aged food, like a normal person.
        //~ Semantic difference, but greatly facilitates people being proud of their character.
        add_consequence( _( "Your stomach won't be happy (not rotten enough)." ), ALLERGY_WEAK );
    }

    const int nutr = nutrition_for( food );
    const int quench = comest->quench;
    const int temp_hunger = get_hunger() - nutr;
    const int temp_thirst = get_thirst() - quench;

    if( !has_active_mutation( trait_id( "EATHEALTH" ) ) &&
        !has_active_mutation( trait_id( "HIBERNATE" ) ) &&
        !has_trait( trait_id( "SLIMESPAWNER" ) ) ) {

        if( get_hunger() < 0 && nutr >= 5 && !has_active_mutation( trait_id( "GOURMAND" ) ) ) {
            add_consequence( _( "You're full already and will be forcing yourself to eat." ), TOO_FULL );
        } else if( ( ( nutr > 0           && temp_hunger < stomach_capacity() ) ||
                     ( comest->quench > 0 && temp_thirst < stomach_capacity() ) ) &&
                   !food.has_infinite_charges() && !has_trait( trait_id( "NO_THIRST" ) ) ) {
            add_consequence( _( "You will not be able to finish it all." ), TOO_FULL );
        }
    }

    if( !consequences.empty() ) {
        if( !interactive ) {
            return consequences.front();
        }
        std::ostringstream req;
        for( const auto &elem : consequences ) {
            req << elem.str() << std::endl;
        }

        const bool eat_verb  = food.has_flag( "USE_EAT_VERB" );
        std::string food_tame = food.tname();
        const nc_color food_color = food.color_in_inventory();
        if( eat_verb || comest->comesttype == "FOOD" ) {
            req << string_format( _( "Eat your %s anyway?" ), colorize( food_tame, food_color ) );
        } else if( !eat_verb && comest->comesttype == "DRINK" ) {
            req << string_format( _( "Drink your %s anyway?" ), colorize( food_tame, food_color ) );
        } else {
            req << string_format( _( "Consume your %s anyway?" ), colorize( food_tame, food_color ) );
        }

        if( !query_yn( req.str() ) ) {
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

    if( food.type->has_use() ) {
        if( food.type->invoke( *this, food, pos() ) <= 0 ) {
            return false;
        }
    }

    // Note: the block below assumes we decided to eat it
    // No coming back from here

    const bool hibernate = has_active_mutation( trait_id( "HIBERNATE" ) );
    const int nutr = nutrition_for( food );
    const int quench = food.type->comestible->quench;
    const bool spoiled = food.rotten();

    // The item is solid food
    const bool chew = food.type->comestible->comesttype == "FOOD" || food.has_flag( "USE_EAT_VERB" );
    // This item is a drink and not a solid food (and not a thick soup)
    const bool drinkable = !chew && food.type->comestible->comesttype == "DRINK";
    // If neither of the above is true then it's a drug and shouldn't get mealtime penalty/bonus

    if( hibernate &&
        ( get_hunger() > -60 && get_thirst() > -60 ) &&
        ( get_hunger() - nutr < -60 || get_thirst() - quench < -60 ) ) {
        add_memorial_log( pgettext( "memorial_male", "Began preparing for hibernation." ),
                          pgettext( "memorial_female", "Began preparing for hibernation." ) );
        add_msg_if_player(
            _( "You've begun stockpiling calories and liquid for hibernation.  You get the feeling that you should prepare for bed, just in case, but...you're hungry again, and you could eat a whole week's worth of food RIGHT NOW." ) );
    }

    const bool will_vomit = get_hunger() < 0 && nutr >= 5 && !has_trait( trait_id( "GOURMAND" ) ) &&
                            !hibernate &&
                            !has_trait( trait_id( "SLIMESPAWNER" ) ) && !has_trait( trait_id( "EATHEALTH" ) ) &&
                            rng( -200, 0 ) > get_hunger() - nutr;
    const bool saprophage = has_trait( trait_id( "SAPROPHAGE" ) );
    if( spoiled && !saprophage ) {
        add_msg_if_player( m_bad, _( "Ick, this %s doesn't taste so good..." ), food.tname().c_str() );
        if( !has_trait( trait_id( "SAPROVORE" ) ) && !has_trait( trait_id( "EATDEAD" ) ) &&
            ( !has_bionic( bio_digestion ) || one_in( 3 ) ) ) {
            add_effect( effect_foodpoison, rng( 6_minutes, ( nutr + 1 ) * 6_minutes ) );
        }
        consume_effects( food );
    } else if( spoiled && saprophage ) {
        add_msg_if_player( m_good, _( "Mmm, this %s tastes delicious..." ), food.tname().c_str() );
        consume_effects( food );
    } else {
        consume_effects( food );
    }

    const bool amorphous = has_trait( trait_id( "AMORPHOUS" ) );
    int mealtime = 250;
    if( drinkable || chew ) {
        // Those bonuses/penalties only apply to food
        // Not to smoking weed or applying bandages!
        if( has_trait( trait_id( "MOUTH_TENTACLES" ) )  || has_trait( trait_id( "MANDIBLES" ) ) ||
            has_trait( trait_id( "FANGS_SPIDER" ) ) ) {
            mealtime /= 2;
        } else if( has_trait( trait_id( "SHARKTEETH" ) ) ) {
            //SHARKBAIT! HOO HA HA!
            mealtime /= 3;
        } else if( has_trait( trait_id( "GOURMAND" ) ) ) {
            // Don't stack those two - that would be 25 moves per item
            mealtime -= 100;
        }

        if( has_trait( trait_id( "BEAK_HUM" ) ) && !drinkable ) {
            mealtime += 200; // Much better than PROBOSCIS but still optimized for fluids
        } else if( has_trait( trait_id( "SABER_TEETH" ) ) ) {
            mealtime += 250; // They get In The Way
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
    if( food.poison > 0 && !has_trait( trait_id( "EATPOISON" ) ) &&
        !has_trait( trait_id( "EATDEAD" ) ) ) {
        if( food.poison >= rng( 2, 4 ) ) {
            add_effect( effect_poison, food.poison * 10_minutes );
        }

        add_effect( effect_foodpoison, food.poison * 30_minutes );
    }

    const bool spiritual = has_trait( trait_id( "SPIRITUAL" ) );
    if( food.has_flag( "HIDDEN_HALLU" ) ) {
        if( spiritual ) {
            add_morale( MORALE_FOOD_GOOD, 36, 72, 2_hours, 1_hours, false );
        } else {
            add_morale( MORALE_FOOD_GOOD, 18, 36, 1_hours, 30_minutes, false );
        }
        if( !has_effect( effect_hallu ) ) {
            add_effect( effect_hallu, 6_hours );
        }
    }

    if( amorphous ) {
        add_msg_player_or_npc( _( "You assimilate your %s." ), _( "<npcname> assimilates a %s." ),
                               food.tname().c_str() );
    } else if( drinkable ) {
        if( ( has_trait( trait_id( "SCHIZOPHRENIC" ) ) || has_artifact_with( AEP_SCHIZO ) ) &&
            one_in( 50 ) && !spoiled && food.goes_bad() && is_player() ) {

            add_msg( m_bad, _( "Ick, this %s (rotten) doesn't taste so good..." ), food.tname().c_str() );
            add_msg( _( "You drink your %s (rotten)." ), food.tname().c_str() );
        } else {
            add_msg_player_or_npc( _( "You drink your %s." ), _( "<npcname> drinks a %s." ),
                                   food.tname().c_str() );
        }
    } else if( chew ) {
        if( ( has_trait( trait_id( "SCHIZOPHRENIC" ) ) || has_artifact_with( AEP_SCHIZO ) ) &&
            one_in( 50 ) && !spoiled && food.goes_bad() && is_player() ) {

            add_msg( m_bad, _( "Ick, this %s (rotten) doesn't taste so good..." ), food.tname().c_str() );
            add_msg( _( "You eat your %s (rotten)." ), food.tname().c_str() );
        } else {
            add_msg_player_or_npc( _( "You eat your %s." ), _( "<npcname> eats a %s." ),
                                   food.tname().c_str() );
            if( !spoiled && !food.has_flag( "ALLERGEN_JUNK" ) ) {
                bool has_table_nearby = false;
                for( const tripoint &pt : g->m.points_in_radius( pos(), 1 ) ) {
                    if( g->m.has_flag_furn( "FLAT_SURF", pt ) || g->m.has_flag( "FLAT_SURF", pt ) ||
                        ( g->m.veh_at( pt ) && g->m.veh_at( pt )->vehicle().has_part( "KITCHEN" ) ) ) {
                        has_table_nearby = true;
                    }
                }
                if( g->m.has_flag_furn( "CAN_SIT", pos() ) && has_table_nearby ) {
                    if( has_trait( trait_id( "TABLEMANNERS" ) ) ) {
                        rem_morale( MORALE_ATE_WITHOUT_TABLE );
                        add_morale( MORALE_ATE_WITH_TABLE, 3, 3, 3_hours, 2_hours, true );
                    } else {
                        add_morale( MORALE_ATE_WITH_TABLE, 1, 1, 3_hours, 2_hours, true );
                    }
                } else {
                    if( has_trait( trait_id( "TABLEMANNERS" ) ) ) {
                        rem_morale( MORALE_ATE_WITH_TABLE );
                        add_morale( MORALE_ATE_WITHOUT_TABLE, -2, -4, 3_hours, 2_hours, true );
                    }
                }
            }
        }
    }

    if( item::find_type( food.type->comestible->tool )->tool ) {
        // Tools like lighters get used
        use_charges( food.type->comestible->tool, 1 );
    }

    if( has_bionic( bio_ethanol ) && food.type->can_use( "ALCOHOL" ) ) {
        charge_power( rng( 50, 200 ) );
    }
    if( has_bionic( bio_ethanol ) && food.type->can_use( "ALCOHOL_WEAK" ) ) {
        charge_power( rng( 25, 100 ) );
    }
    if( has_bionic( bio_ethanol ) && food.type->can_use( "ALCOHOL_STRONG" ) ) {
        charge_power( rng( 75, 300 ) );
    }

    if( has_active_bionic( bio_taste_blocker ) ) {
        charge_power( -abs( food.type->comestible->fun ) );
    }

    if( food.has_flag( "CANNIBALISM" ) ) {
        // Sapiovores don't recognize humans as the same species.
        // But let them possibly feel cool about eating sapient stuff - treat like psycho
        // However, spiritual sapiovores should still recognize humans as having a soul or special for religious reasons
        const bool cannibal = has_trait( trait_id( "CANNIBAL" ) );
        const bool psycho = has_trait( trait_id( "PSYCHOPATH" ) );
        const bool sapiovore = has_trait( trait_id( "SAPIOVORE" ) );
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
            add_msg_if_player( _( "Meh. You've eaten worse." ) );
        } else if( spiritual ) {
            add_msg_if_player( m_bad,
                               _( "This is probably going to count against you if there's still an afterlife." ) );
            add_morale( MORALE_CANNIBAL, -60, -400, 60_minutes, 30_minutes );
        } else {
            add_msg_if_player( m_bad, _( "You feel horrible for eating a person." ) );
            add_morale( MORALE_CANNIBAL, -60, -400, 60_minutes, 30_minutes );
        }
    }

    // The fun changes for these effects are applied in fun_for().
    if( food.has_flag( "MUSHY" ) ) {
        add_msg_if_player( m_bad,
                           _( "You try to ignore its mushy texture, but it leaves you with an awful aftertaste." ) );
    }
    if( food.type->comestible->fun > 0 ) {
        if( has_effect( effect_common_cold ) ) {
            add_msg_if_player( m_bad, _( "You can't taste much of anything with this cold." ) );
        }
        if( has_effect( effect_flu ) ) {
            add_msg_if_player( m_bad, _( "You can't taste much of anything with this flu." ) );
        }
    }

    // Allergy check
    const auto allergy = allergy_type( food );
    if( allergy != MORALE_NULL ) {
        add_msg_if_player( m_bad, _( "Yuck! How can anybody eat this stuff?" ) );
        add_morale( allergy, -75, -400, 30_minutes, 24_minutes );
    }
    if( food.has_flag( "ALLERGEN_JUNK" ) ) {
        if( has_trait( trait_id( "PROJUNK" ) ) ) {
            add_msg_if_player( m_good, _( "Mmm, junk food." ) );
            add_morale( MORALE_SWEETTOOTH, 5, 30, 30_minutes, 24_minutes );
        }
        if( has_trait( trait_id( "PROJUNK2" ) ) ) {
            if( !one_in( 100 ) ) {
                add_msg_if_player( m_good, _( "When life's got you down, there's always sugar." ) );
            } else {
                add_msg_if_player( m_good, _( "They may do what they must... you've already won." ) );
            }
            add_morale( MORALE_SWEETTOOTH, 10, 50, 1_hours, 50_minutes );
        }
    }
    // Carnivores CAN eat junk food, but they won't like it much.
    // Pizza-scraping happens in consume_effects.
    if( has_trait( trait_id( "CARNIVORE" ) ) && food.has_flag( "ALLERGEN_JUNK" ) &&
        !food.has_flag( "CARNIVORE_OK" ) ) {
        add_msg_if_player( m_bad, _( "Your stomach begins gurgling and you feel bloated and ill." ) );
        add_morale( MORALE_NO_DIGEST, -25, -125, 30_minutes, 24_minutes );
    }
    if( !spoiled && chew && has_trait( trait_id( "SAPROPHAGE" ) ) ) {
        // It's OK to *drink* things that haven't rotted.  Alternative is to ban water.  D:
        add_msg_if_player( m_bad, _( "Your stomach begins gurgling and you feel bloated and ill." ) );
        add_morale( MORALE_NO_DIGEST, -75, -400, 30_minutes, 24_minutes );
    }
    if( food.has_flag( "URSINE_HONEY" ) && ( !crossed_threshold() ||
            has_trait( trait_id( "THRESH_URSINE" ) ) ) &&
        mutation_category_level["URSINE"] > 40 ) {
        //Need at least 5 bear mutations for effect to show, to filter out mutations in common with other categories
        int honey_fun = has_trait( trait_id( "THRESH_URSINE" ) ) ?
                        std::min( mutation_category_level["URSINE"] / 8, 20 ) :
                        mutation_category_level["URSINE"] / 12;
        if( honey_fun < 10 ) {
            add_msg_if_player( m_good, _( "You find the sweet taste of honey surprisingly palatable." ) );
        } else {
            add_msg_if_player( m_good, _( "You feast upon the sweet honey." ) );
        }
        add_morale( MORALE_HONEY, honey_fun, 100 );
    }

    if( will_vomit ) {
        vomit();
    }

    // chance to become parasitised
    if( !( has_bionic( bio_digestion ) || has_trait( trait_id( "PARAIMMUNE" ) ) ) ) {
        if( food.type->comestible->parasites > 0 && !food.has_flag( "NO_PARASITES" ) &&
            one_in( food.type->comestible->parasites ) ) {
            switch( rng( 0, 3 ) ) {
                case 0:
                    if( !has_trait( trait_id( "EATHEALTH" ) ) ) {
                        add_effect( effect_tapeworm, 1_turns, num_bp, true );
                    }
                    break;
                case 1:
                    if( !has_trait( trait_id( "ACIDBLOOD" ) ) ) {
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

    for( const auto &v : this->vitamins_from( food ) ) {
        auto qty = has_effect( effect_tapeworm ) ? v.second / 2 : v.second;

        // can never develop hypervitaminosis from consuming food
        vitamin_mod( v.first, qty );
    }

    food.mod_charges( -1 );
    return true;
}

// Caps both actual nutrition/thirst and stomach capacity
void cap_nutrition_thirst( player &p, int capacity, bool food, bool water )
{
    if( ( food && p.get_hunger() < capacity ) ||
        ( water && p.get_thirst() < capacity ) ) {
        p.add_msg_if_player( _( "You can't finish it all!" ) );
    }

    if( p.get_hunger() < capacity ) {
        p.mod_stomach_food( p.get_hunger() - capacity );
        p.set_hunger( capacity );
    }

    if( p.get_thirst() < capacity ) {
        p.mod_stomach_water( p.get_thirst() - capacity );
        p.set_thirst( capacity );
    }

    if( p.has_trait( trait_id( "NO_THIRST" ) ) ) {
        p.set_thirst( p.get_hunger() );
    }

    add_msg( m_debug, "%s nutrition cap: hunger %d, thirst %d, stomach food %d, stomach water %d",
             p.disp_name().c_str(), p.get_hunger(), p.get_thirst(), p.get_stomach_food(),
             p.get_stomach_water() );
}

void player::consume_effects( const item &food )
{
    if( !food.is_comestible() ) {
        debugmsg( "called player::consume_effects with non-comestible" );
        return;
    }
    const auto &comest = *food.type->comestible;

    const int capacity = stomach_capacity();
    if( has_trait( trait_id( "THRESH_PLANT" ) ) && food.type->can_use( "PLANTBLECH" ) ) {
        // Just keep nutrition capped, to prevent vomiting
        cap_nutrition_thirst( *this, capacity, true, true );
        return;
    }
    if( ( has_trait( trait_id( "HERBIVORE" ) ) || has_trait( trait_id( "RUMINANT" ) ) ) &&
        food.has_any_flag( herbivore_blacklist ) ) {
        // No good can come of this.
        return;
    }

    // Rotten food causes health loss
    const float relative_rot = food.get_relative_rot();
    if( relative_rot > 1.0f && !has_trait( trait_id( "SAPROPHAGE" ) ) &&
        !has_trait( trait_id( "SAPROVORE" ) ) && !has_bionic( bio_digestion ) ) {
        const float rottedness = clamp( 2 * relative_rot - 2.0f, 0.1f, 1.0f );
        // ~-1 health per 1 nutrition at halfway-rotten-away, ~0 at "just got rotten"
        // But always round down
        int h_loss = -rottedness * comest.get_nutr();
        mod_healthy_mod( h_loss, -200 );
        add_msg( m_debug, "%d health from %0.2f%% rotten food", h_loss, rottedness );
    }

    const auto nutr = nutrition_for( food );
    mod_hunger( -nutr );
    mod_thirst( -comest.quench );
    mod_stomach_food( nutr );
    mod_stomach_water( comest.quench );
    int effective_health = comest.healthy;
    if( effective_health != 0 ) {
        // Effectively no cap on health modifiers from food
        if( has_trait( trait_id( "PROJUNK2" ) ) && effective_health < 0 ) {
            effective_health = 0; // we can handle junk just fine!
        }
        mod_healthy_mod( effective_health, ( effective_health >= 0 ) ? 200 : -200 );
    }

    if( comest.stim != 0 &&
        ( abs( stim ) < ( abs( comest.stim ) * 3 ) ||
          sgn( stim ) != sgn( comest.stim ) ) ) {
        if( comest.stim < 0 ) {
            stim = std::max( comest.stim * 3, stim + comest.stim );
        } else {
            stim = std::min( comest.stim * 3, stim + comest.stim );
        }
    }
    if( has_trait( trait_id( "STIMBOOST" ) ) && ( stim > 30 ) && ( ( comest.add == ADD_CAFFEINE )
            || ( comest.add == ADD_SPEED ) || ( comest.add == ADD_COKE ) || ( comest.add == ADD_CRACK ) ) ) {
        int hallu_duration = ( stim - comest.stim < 30 ) ? stim - 30 : comest.stim;
        add_effect( effect_visuals, hallu_duration * 30_minutes );
        std::vector<std::string> stimboost_msg{ _( "The shadows are getting ever closer." ),
                                                _( "You have a bad feeling about this." ),
                                                _( "A powerful sense of dread comes over you." ),
                                                _( "Your skin starts crawling." ),
                                                _( "They're coming to get you." ),
                                                _( "This might've been a bad idea..." ),
                                                _( "You've really done it this time, haven't you?" ),
                                                _( "You have to stay vigilant. They're always watching..." ),
                                                _( "mistake mistake mistake mistake mistake" ),
                                                _( "Just gotta stay calm, and you'll make it through this." ),
                                                _( "You're starting to feel very jumpy." ),
                                                _( "Something is twitching at the edge of your vision." ),
                                                _( "They know what you've done..." ),
                                                _( "You're feeling even more paranoid than usual." ) };
        add_msg_if_player( m_bad, random_entry_ref( stimboost_msg ) );
    }
    add_addiction( comest.add, comest.addict );
    if( addiction_craving( comest.add ) != MORALE_NULL ) {
        rem_morale( addiction_craving( comest.add ) );
    }

    time_duration morale_time = 2_hours;
    if( food.has_flag( "HOT" ) && food.has_flag( "EATEN_HOT" ) ) {
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

    const bool hibernate = has_active_mutation( trait_id( "HIBERNATE" ) );
    if( hibernate ) {
        if( ( nutr > 0 && get_hunger() < -60 ) || ( comest.quench > 0 && get_thirst() < -60 ) ) {
            //Tell the player what's going on
            add_msg_if_player( _( "You gorge yourself, preparing to hibernate." ) );
            if( one_in( 2 ) ) {
                //50% chance of the food tiring you
                mod_fatigue( nutr );
            }
        }
        if( ( nutr > 0 && get_hunger() < -200 ) || ( comest.quench > 0 && get_thirst() < -200 ) ) {
            //Hibernation should cut burn to 60/day
            add_msg_if_player( _( "You feel stocked for a day or two. Got your bed all ready and secured?" ) );
            if( one_in( 2 ) ) {
                //And another 50%, intended cumulative
                mod_fatigue( nutr );
            }
        }

        if( ( nutr > 0 && get_hunger() < -400 ) || ( comest.quench > 0 && get_thirst() < -400 ) ) {
            add_msg_if_player(
                _( "Mmm.  You can still fit some more in...but maybe you should get comfortable and sleep." ) );
            if( !one_in( 3 ) ) {
                //Third check, this one at 66%
                mod_fatigue( nutr );
            }
        }
        if( ( nutr > 0 && get_hunger() < -600 ) || ( comest.quench > 0 && get_thirst() < -600 ) ) {
            add_msg_if_player( _( "That filled a hole!  Time for bed..." ) );
            // At this point, you're done.  Schlaf gut.
            mod_fatigue( nutr );
        }
    }

    // Moved here and changed a bit - it was too complex
    // Incredibly minor stuff like this shouldn't require complexity
    if( !is_npc() && has_trait( trait_id( "SLIMESPAWNER" ) ) &&
        ( get_hunger() < capacity + 40 || get_thirst() < capacity + 40 ) ) {
        add_msg_if_player( m_mixed,
                           _( "You feel as though you're going to split open!  In a good way?" ) );
        mod_pain( 5 );
        std::vector<tripoint> valid;
        for( const tripoint &dest : g->m.points_in_radius( pos(), 1 ) ) {
            if( g->is_empty( dest ) ) {
                valid.push_back( dest );
            }
        }
        int numslime = 1;
        for( int i = 0; i < numslime && !valid.empty(); i++ ) {
            const tripoint target = random_entry_removed( valid );
            if( monster *const slime = g->summon_mon( mon_player_blob, target ) ) {
                slime->friendly = -1;
            }
        }
        mod_hunger( 40 );
        mod_thirst( 40 );
        //~slimespawns have *small voices* which may be the Nice equivalent
        //~of the Rat King's ALL CAPS invective.  Probably shared-brain telepathy.
        add_msg_if_player( m_good, _( "hey, you look like me! let's work together!" ) );
    }

    // Last thing that happens before capping hunger
    if( get_hunger() < capacity && has_trait( trait_id( "EATHEALTH" ) ) ) {
        int excess_food = capacity - get_hunger();
        add_msg_player_or_npc( _( "You feel the %s filling you out." ),
                               _( "<npcname> looks better after eating the %s." ),
                               food.tname().c_str() );
        // Guaranteed 1 HP healing, no matter what.  You're welcome.  ;-)
        if( excess_food <= 5 ) {
            healall( 1 );
        } else {
            // Straight conversion, except it's divided amongst all your body parts.
            healall( excess_food /= 5 );
        }

        // Note: We want this here to prevent "you can't finish this" messages
        set_hunger( capacity );
    }

    cap_nutrition_thirst( *this, capacity, nutr > 0, comest.quench > 0 );
}

hint_rating player::rate_action_eat( const item &it ) const
{
    if( !can_consume( it ) ) {
        return HINT_CANT;
    }

    const auto rating = will_eat( it );
    if( rating.success() ) {
        return HINT_GOOD;
    } else if( rating.value() == INEDIBLE || rating.value() == INEDIBLE_MUTATION ) {
        return HINT_CANT;
    }

    return HINT_IFFY;
}

bool player::can_feed_battery_with( const item &it ) const
{
    if( !it.is_ammo() || can_eat( it ).success() || !has_active_bionic( bio_batteries ) ) {
        return false;
    }

    return it.type->ammo->type.count( ammotype( "battery" ) );
}

bool player::feed_battery_with( item &it )
{
    if( !can_feed_battery_with( it ) ) {
        return false;
    }

    const int energy = get_acquirable_energy( it, rechargeable_cbm::battery );
    const int profitable_energy = std::min( energy, max_power_level - power_level );

    if( profitable_energy <= 0 ) {
        add_msg_player_or_npc( m_info,
                               _( "Your internal power storage is fully powered." ),
                               _( "<npcname>'s internal power storage is fully powered." ) );
        return false;
    }

    charge_power( it.charges );
    it.charges -= profitable_energy;

    add_msg_player_or_npc( m_info,
                           _( "You recharge your battery system with the %s." ),
                           _( "<npcname> recharges their battery system with the %s." ),
                           it.tname().c_str() );
    mod_moves( -250 );
    return true;
}

bool player::can_feed_reactor_with( const item &it ) const
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
        return it.type->ammo->type.count( elem );
    } );
}

bool player::feed_reactor_with( item &it )
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
                           it.tname().c_str() );

    tank_plut += amount; // TODO: Encapsulate
    it.charges -= 1;
    mod_moves( -250 );
    return true;
}

bool player::can_feed_furnace_with( const item &it ) const
{
    if( !it.flammable() || it.has_flag( "RADIOACTIVE" ) || can_eat( it ).success() ) {
        return false;
    }

    if( !has_active_bionic( bio_furnace ) ) {
        return false;
    }

    if( it.charges_per_volume( furnace_max_volume ) < 1 ) { // not even one charge fits
        return false;
    }

    return it.typeId() != "corpse"; // TODO: Eliminate the hard-coded special case.
}

bool player::feed_furnace_with( item &it )
{
    if( !can_feed_furnace_with( it ) ) {
        return false;
    }

    const long consumed_charges = std::min( it.charges, it.charges_per_volume( furnace_max_volume ) );
    const int energy = get_acquirable_energy( it, rechargeable_cbm::furnace );

    if( energy == 0 ) {
        add_msg_player_or_npc( m_info,
                               _( "You digest your %s, but fail to acquire energy from it." ),
                               _( "<npcname> digests their %s for energy, but fails to acquire energy from it." ),
                               it.tname().c_str() );
    } else if( power_level >= max_power_level ) {
        add_msg_player_or_npc(
            _( "You digest your %s, but you're fully powered already, so the energy is wasted." ),
            _( "<npcname> digests a %s for energy, they're fully powered already, so the energy is wasted." ),
            it.tname().c_str() );
    } else {
        const int profitable_energy = std::min( energy, max_power_level - power_level );
        if( it.count_by_charges() ) {
            add_msg_player_or_npc( m_info,
                                   ngettext( "You digest %d %s and recharge %d point of energy.",
                                             "You digest %d %s and recharge %d points of energy.",
                                             profitable_energy
                                           ),
                                   ngettext( "<npcname> digests %d %s and recharges %d point of energy.",
                                             "<npcname> digests %d %s and recharges %d points of energy.",
                                             profitable_energy
                                           ), consumed_charges, it.tname().c_str(), profitable_energy
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
                                           ), it.tname().c_str(), profitable_energy
                                 );
        }
        charge_power( profitable_energy );
    }

    it.charges -= consumed_charges;
    mod_moves( -250 );

    return true;
}

rechargeable_cbm player::get_cbm_rechargeable_with( const item &it ) const
{
    if( can_feed_reactor_with( it ) ) {
        return rechargeable_cbm::reactor;
    }

    const int battery_energy = get_acquirable_energy( it, rechargeable_cbm::battery );
    const int furnace_energy = get_acquirable_energy( it, rechargeable_cbm::furnace );

    if( can_feed_battery_with( it ) && battery_energy >= furnace_energy ) {
        return rechargeable_cbm::battery;
    } else if( can_feed_furnace_with( it ) ) {
        return rechargeable_cbm::furnace;
    }

    return rechargeable_cbm::none;
}

int player::get_acquirable_energy( const item &it, rechargeable_cbm cbm ) const
{
    switch( cbm ) {
        case rechargeable_cbm::none:
            break;

        case rechargeable_cbm::battery:
            return std::min<long>( it.charges, std::numeric_limits<int>::max() );

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
                consumed_mass = it.type->weight * 10 * n_stacks; // it.type->weight is in 10g units?
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
    }

    return 0;
}

int player::get_acquirable_energy( const item &it ) const
{
    return get_acquirable_energy( it, get_cbm_rechargeable_with( it ) );
}

bool player::can_estimate_rot() const
{
    return get_skill_level( skill_cooking ) >= 3 || get_skill_level( skill_survival ) >= 4;
}

bool player::can_consume_as_is( const item &it ) const
{
    return it.is_comestible() || get_cbm_rechargeable_with( it ) != rechargeable_cbm::none;
}

bool player::can_consume( const item &it ) const
{
    if( can_consume_as_is( it ) ) {
        return true;
    }
    // checking NO_UNLOAD to prevent consumption of `battery` when contained in `battery_car` (#20012)
    return !it.is_container_empty() && !it.has_flag( "NO_UNLOAD" ) &&
           can_consume_as_is( it.contents.front() );
}

item &player::get_comestible_from( item &it ) const
{
    if( !it.is_container_empty() && can_consume_as_is( it.contents.front() ) ) {
        return it.contents.front();
    } else if( can_consume_as_is( it ) ) {
        return it;
    }

    static item null_comestible;
    null_comestible = item();   // Since it's not const.
    return null_comestible;
}
