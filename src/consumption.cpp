#include "player.h"

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
#include "translations.h"

#include <string>
#include <algorithm>

const efftype_id effect_foodpoison( "foodpoison" );
const efftype_id effect_poison( "poison" );
const efftype_id effect_tapeworm( "tapeworm" );
const efftype_id effect_bloodworms( "bloodworms" );
const efftype_id effect_brainworms( "brainworms" );
const efftype_id effect_paincysts( "paincysts" );
const efftype_id effect_nausea( "nausea" );

const mtype_id mon_player_blob( "mon_player_blob" );

static const std::vector<std::string> carnivore_blacklist {{
        "ALLERGEN_VEGGY", "ALLERGEN_FRUIT", "ALLERGEN_WHEAT",
    }
};
// This ugly temp array is here because otherwise it goes
// std::vector(char*, char*)->vector(InputIterator,InputIterator) or some such
const std::array<std::string, 2> temparray {{"ALLERGEN_MEAT", "ALLERGEN_EGG"}};
static const std::vector<std::string> herbivore_blacklist( temparray.begin(), temparray.end() );

int player::stomach_capacity() const
{
    if( has_trait( "GIZZARD" ) ) {
        return 0;
    }

    if( has_active_mutation( "HIBERNATE" ) ) {
        return -620;
    }

    if( has_trait( "GOURMAND" ) || has_trait( "HIBERNATE" ) ) {
        return -60;
    }

    return -20;
}

// TODO: Move pizza scraping here.
// Same for other kinds of nutrition alterations
// This is used by item display, making actual nutrition available to player.
int player::nutrition_for( const itype *comest ) const
{
    return ( comest && comest->comestible ) ? comest->comestible->nutr : 0;
}

std::map<vitamin_id, int> player::vitamins_from( const itype_id &id ) const
{
    return vitamins_from( item( id ) );
}

std::map<vitamin_id, int> player::vitamins_from( const item &it ) const
{
    std::map<vitamin_id, int> res;

    if( !it.type->comestible ) {
        return res;
    }

    // food to which the player is allergic to never contains any vitamins
    if( allergy_type( it ) != MORALE_NULL ) {
        return res;
    }

    // @todo bionics and mutations can affect vitamin absorption
    for( const auto &e : it.type->comestible->vitamins ) {
        res.emplace( e.first, e.second );
    }

    return res;
}

int player::vitamin_rate( const vitamin_id &vit ) const
{
    int res = vit.obj().rate();

    for( const auto &m : get_mutations() ) {
        const auto &mut = mutation_branch::get( m );
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
    if( get_world_option<bool>( "NO_VITAMINS" ) ) {
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
    float ret = 1.0f;
    if( has_trait( "LIGHTEATER" ) ) {
        ret -= ( 1.0f / 3.0f );
    }

    if( has_trait( "HUNGER" ) ) {
        ret += 0.5f;
    } else if( has_trait( "HUNGER2" ) ) {
        ret += 1.0f;
    } else if( has_trait( "HUNGER3" ) ) {
        ret += 2.0f;
    }

    if( has_trait( "MET_RAT" ) ) {
        ret += ( 1.0f / 3.0f );
    }

    return ret;
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
            { 300, 1.0f },
            { 2000, 0.8f },
            { 5000, 0.6f },
            { 8000, 0.5f }
        }
    };

    // Penalize fast survivors
    // TODO: Have cold temperature increase, not decrease, metabolism
    const float effective_hunger = get_hunger() * 100.0f / std::max( 50, get_speed() );
    const float modifier = multi_lerp( thresholds, effective_hunger );

    return modifier * metabolic_rate_base();
}

morale_type player::allergy_type( const item &food ) const
{
    using allergy_tuple = std::tuple<std::string, std::string, morale_type>;
    static const std::array<allergy_tuple, 8> allergy_tuples = {{
            std::make_tuple( "VEGETARIAN", "ALLERGEN_MEAT", MORALE_VEGETARIAN ),
            std::make_tuple( "MEATARIAN", "ALLERGEN_VEGGY", MORALE_MEATARIAN ),
            std::make_tuple( "LACTOSE", "ALLERGEN_MILK", MORALE_LACTOSE ),
            std::make_tuple( "ANTIFRUIT", "ALLERGEN_FRUIT", MORALE_ANTIFRUIT ),
            std::make_tuple( "ANTIJUNK", "ALLERGEN_JUNK", MORALE_ANTIJUNK ),
            std::make_tuple( "ANTIWHEAT", "ALLERGEN_WHEAT", MORALE_ANTIWHEAT )
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

bool player::is_allergic( const item &food ) const
{
    return allergy_type( food ) != MORALE_NULL;
}

edible_rating player::can_eat( const item &food, bool interactive, bool force ) const
{
    if( is_npc() || force ) {
        // Just to be sure
        interactive = false;
    }

    const std::string &itname = food.tname();
    // Helper to avoid ton of `if( interactive )`
    // Prints if interactive is true, does nothing otherwise
    const auto maybe_print = [interactive, &itname]
    ( game_message_type type, const char *str ) {
        if( interactive ) {
            add_msg( type, str, itname.c_str() );
        }
    };
    // As above, but for queries
    // Asks if interactive and not force
    // Always true if force
    // Never true otherwise
    const auto maybe_query = [force, interactive, &itname, this]( const char *str ) {
        if( force ) {
            return true;
        } else if( !interactive ) {
            return false;
        }

        return query_yn( str, itname.c_str() );
    };

    const auto comest = food.type->comestible.get();
    if( comest == nullptr ) {
        maybe_print( m_info, _( "That doesn't look edible." ) );
        return INEDIBLE;
    }

    const bool edible    = comest->comesttype == "FOOD" || food.has_flag( "USE_EAT_VERB" );
    const bool drinkable = comest->comesttype == "DRINK" && !food.has_flag( "USE_EAT_VERB" );

    if( edible || drinkable ) {
        for( const auto &m : food.type->materials ) {
            if( !m.obj().edible() ) {
                maybe_print( m_info, _( "That doesn't look edible in its current form." ) );
                return INEDIBLE;
            }
        }
    }

    if( comest->tool != "null" ) {
        bool has = has_amount( comest->tool, 1 );
        if( item::count_by_charges( comest->tool ) ) {
            has = has_charges( comest->tool, 1 );
        }
        if( !has ) {
            if( interactive ) {
                add_msg_if_player( m_info, _( "You need a %s to consume that!" ),
                                   item::nname( comest->tool ).c_str() );
            }
            return NO_TOOL;
        }
    }

    if( is_underwater() ) {
        maybe_print( m_info, _( "You can't do that while underwater." ) );
        return INEDIBLE;
    }
    // For all those folks who loved eating marloss berries.  D:< mwuhahaha
    if( has_trait( "M_DEPENDENT" ) && food.typeId() != "mycus_fruit" ) {
        maybe_print( m_info, _( "We can't eat that.  It's not right for us." ) );
        return INEDIBLE_MUTATION;
    }

    // Here's why PROBOSCIS is such a negative trait.
    if( has_trait( "PROBOSCIS" ) && !drinkable ) {
        maybe_print( m_info, _( "Ugh, you can't drink that!" ) );
        return INEDIBLE_MUTATION;
    }

    int capacity = stomach_capacity();

    // TODO: Move this cache to a structure and pass it around
    // to speed up checking entire inventory for edibles
    const bool gourmand = has_trait( "GOURMAND" );
    const bool hibernate = has_active_mutation( "HIBERNATE" );
    const bool eathealth = has_trait( "EATHEALTH" );
    const bool slimespawner = has_trait( "SLIMESPAWNER" );
    const int nutr = nutrition_for( food.type );
    const int quench = comest->quench;
    bool spoiled = food.rotten();

    const int temp_hunger = get_hunger() - nutr;
    const int temp_thirst = get_thirst() - quench;

    const bool overeating = get_hunger() < 0 && nutr >= 5 && !gourmand && !eathealth && !slimespawner &&
                            !hibernate;

    if( interactive && hibernate &&
        ( get_hunger() >= -60 && get_thirst() >= -60 ) &&
        ( temp_hunger < -60 || temp_thirst < -60 ) ) {
        if( !maybe_query( _( "You're adequately fueled. Prepare for hibernation?" ) ) ) {
            return TOO_FULL;
        }
    }

    const bool carnivore = has_trait( "CARNIVORE" );
    if( carnivore && nutr > 0 &&
        food.has_any_flag( carnivore_blacklist ) && !food.has_flag( "CARNIVORE_OK" ) ) {
        maybe_print( m_info, _( "Eww.  Inedible plant stuff!" ) );
        return INEDIBLE_MUTATION;
    }

    if( ( has_trait( "HERBIVORE" ) || has_trait( "RUMINANT" ) ) &&
        food.has_any_flag( herbivore_blacklist ) ) {
        // Like non-cannibal, but more strict!
        maybe_print( m_info, _( "The thought of eating that makes you feel sick.  You decide not to." ) );
        return INEDIBLE_MUTATION;
    }

    if( food.has_flag( "CANNIBALISM" ) ) {
        if( !has_trait_flag( "CANNIBAL" ) &&
            !maybe_query( _( "The thought of eating that makes you feel sick.  Really do it?" ) ) ) {
            return CANNIBALISM;
        }
    }

    if( is_allergic( food ) &&
        !maybe_query( _( "Really eat that %s?  Your stomach won't be happy." ) ) ) {
        return ALLERGY;
    }

    if( carnivore && food.has_flag( "ALLERGEN_JUNK" ) && !food.has_flag( "CARNIVORE_OK" ) &&
        !maybe_query( _( "Really eat that %s?  Your stomach won't be happy." ) ) ) {
        return ALLERGY;
    }

    const bool saprophage = has_trait( "SAPROPHAGE" );
    if( spoiled ) {
        if( !saprophage && !has_trait( "SAPROVORE" ) &&
            !maybe_query( _( "This %s smells awful!  Eat it?" ) ) ) {
            return ROTTEN;
        }
    } else if( saprophage && edible && !food.has_flag( "FERTILIZER" ) &&
               !maybe_query( _( "Really eat that %s?  Your stomach won't be happy." ) ) ) {
        // Note: We're allowing all non-solid "food". This includes drugs
        // Hardcoding fertilizer for now - should be a separate flag later
        //~ No, we don't eat "rotten" food. We eat properly aged food, like a normal person.
        //~ Semantic difference, but greatly facilitates people being proud of their character.
        maybe_print( m_info, _( "It's too fresh, let it age a little first." ) );
        return ROTTEN;
    }

    if( edible && has_effect( effect_nausea ) ) {
        if( !maybe_query(
                _( "You still feel nauseous and will probably puke it all up again.  Eat anyway?" ) ) ) {
            return ALLERGY;
        }
    }

    // Print at most one of those
    bool overfull = false;
    if( overeating ) {
        overfull = !maybe_query( _( "You're full.  Force yourself to eat?" ) );
    } else if( ( ( nutr > 0 && temp_hunger < capacity ) ||
                 ( comest->quench > 0 && temp_thirst < capacity ) ) &&
               !food.has_infinite_charges() && !eathealth && !slimespawner ) {
        overfull = !maybe_query( _( "You will not be able to finish it all.  Consume it?" ) );
    }

    if( overfull ) {
        return TOO_FULL;
    }

    // All checks ended, it's edible (or we're pretending it is)
    return EDIBLE;
}

bool player::eat( item &food, bool force )
{
    if( !food.is_food() ) {
        return false;
    }

    // Check if it's rotten before eating!
    food.calc_rot( global_square_location() );
    const auto edible = can_eat( food, is_player() && !force, force );
    g->refresh_all();
    if( edible != EDIBLE ) {
        return false;
    }

    if( food.type->has_use() ) {
        if( food.type->invoke( this, &food, pos() ) <= 0 ) {
            return false;
        }
    }

    // Note: the block below assumes we decided to eat it
    // No coming back from here

    const bool hibernate = has_active_mutation( "HIBERNATE" );
    const int nutr = nutrition_for( food.type );
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

    const bool will_vomit = get_hunger() < 0 && nutr >= 5 && !has_trait( "GOURMAND" ) && !hibernate &&
                            !has_trait( "SLIMESPAWNER" ) && !has_trait( "EATHEALTH" ) &&
                            rng( -200, 0 ) > get_hunger() - nutr;
    const bool saprophage = has_trait( "SAPROPHAGE" );
    if( spoiled && !saprophage ) {
        add_msg_if_player( m_bad, _( "Ick, this %s doesn't taste so good..." ), food.tname().c_str() );
        if( !has_trait( "SAPROVORE" ) && !has_trait( "EATDEAD" ) &&
            ( !has_bionic( "bio_digestion" ) || one_in( 3 ) ) ) {
            add_effect( effect_foodpoison, rng( 60, ( nutr + 1 ) * 60 ) );
        }
        consume_effects( food, spoiled );
    } else if( spoiled && saprophage ) {
        add_msg_if_player( m_good, _( "Mmm, this %s tastes delicious..." ), food.tname().c_str() );
        consume_effects( food, spoiled );
    } else {
        consume_effects( food, spoiled );
    }

    const bool amorphous = has_trait( "AMORPHOUS" );
    int mealtime = 250;
    if( drinkable || chew ) {
        // Those bonuses/penalties only apply to food
        // Not to smoking weed or applying bandages!
        if( has_trait( "MOUTH_TENTACLES" )  || has_trait( "MANDIBLES" ) ) {
            mealtime /= 2;
        } else if( has_trait( "GOURMAND" ) ) {
            // Don't stack those two - that would be 25 moves per item
            mealtime -= 100;
        }

        if( has_trait( "BEAK_HUM" ) && !drinkable ) {
            mealtime += 200; // Much better than PROBOSCIS but still optimized for fluids
        } else if( has_trait( "SABER_TEETH" ) ) {
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
    if( food.poison > 0 && !has_trait( "EATPOISON" ) && !has_trait( "EATDEAD" ) ) {
        if( food.poison >= rng( 2, 4 ) ) {
            add_effect( effect_poison, food.poison * 100 );
        }

        add_effect( effect_foodpoison, food.poison * 300 );
    }

    if( amorphous ) {
        add_msg_player_or_npc( _( "You assimilate your %s." ), _( "<npcname> assimilates a %s." ),
                               food.tname().c_str() );
    } else if( drinkable ) {
        add_msg_player_or_npc( _( "You drink your %s." ), _( "<npcname> drinks a %s." ),
                               food.tname().c_str() );
    } else if( chew ) {
        add_msg_player_or_npc( _( "You eat your %s." ), _( "<npcname> eats a %s." ),
                               food.tname().c_str() );
    }

    if( item::find_type( food.type->comestible->tool )->tool ) {
        // Tools like lighters get used
        use_charges( food.type->comestible->tool, 1 );
    }

    if( has_bionic( "bio_ethanol" ) && food.type->can_use( "ALCOHOL" ) ) {
        charge_power( rng( 50, 200 ) );
    }
    if( has_bionic( "bio_ethanol" ) && food.type->can_use( "ALCOHOL_WEAK" ) ) {
        charge_power( rng( 25, 100 ) );
    }
    if( has_bionic( "bio_ethanol" ) && food.type->can_use( "ALCOHOL_STRONG" ) ) {
        charge_power( rng( 75, 300 ) );
    }

    if( food.has_flag( "CANNIBALISM" ) ) {
        // Sapiovores don't recognize humans as the same species.
        // But let them possibly feel cool about eating sapient stuff - treat like psycho
        const bool cannibal = has_trait( "CANNIBAL" );
        const bool psycho = has_trait( "PSYCHOPATH" ) || has_trait( "SAPIOVORE" );
        const bool spiritual = has_trait( "SPIRITUAL" );
        if( cannibal && psycho && spiritual ) {
            add_msg_if_player( m_good,
                               _( "You feast upon the human flesh, and in doing so, devour their spirit." ) );
            // You're not really consuming anything special; you just think you are.
            add_morale( MORALE_CANNIBAL, 25, 300 );
        } else if( cannibal && psycho ) {
            add_msg_if_player( m_good, _( "You feast upon the human flesh." ) );
            add_morale( MORALE_CANNIBAL, 15, 200 );
        } else if( cannibal && spiritual ) {
            add_msg_if_player( m_good, _( "You consume the sacred human flesh." ) );
            // Boosted because you understand the philosophical implications of your actions, and YOU LIKE THEM.
            add_morale( MORALE_CANNIBAL, 15, 200 );
        } else if( cannibal ) {
            add_msg_if_player( m_good, _( "You indulge your shameful hunger." ) );
            add_morale( MORALE_CANNIBAL, 10, 50 );
        } else if( psycho && spiritual ) {
            add_msg_if_player( _( "You greedily devour the taboo meat." ) );
            // Small bonus for violating a taboo.
            add_morale( MORALE_CANNIBAL, 5, 50 );
        } else if( psycho ) {
            add_msg_if_player( _( "Meh. You've eaten worse." ) );
        } else if( spiritual ) {
            add_msg_if_player( m_bad,
                               _( "This is probably going to count against you if there's still an afterlife." ) );
            add_morale( MORALE_CANNIBAL, -60, -400, 600, 300 );
        } else {
            add_msg_if_player( m_bad, _( "You feel horrible for eating a person." ) );
            add_morale( MORALE_CANNIBAL, -60, -400, 600, 300 );
        }
    }

    // Allergy check
    const auto allergy = allergy_type( food );
    if( allergy != MORALE_NULL ) {
        add_msg_if_player( m_bad, _( "Yuck! How can anybody eat this stuff?" ) );
        add_morale( allergy, -75, -400, 300, 240 );
    }
    // Carnivores CAN eat junk food, but they won't like it much.
    // Pizza-scraping happens in consume_effects.
    if( has_trait( "CARNIVORE" ) && food.has_flag( "ALLERGEN_JUNK" ) &&
        !food.has_flag( "CARNIVORE_OK" ) ) {
        add_msg_if_player( m_bad, _( "Your stomach begins gurgling and you feel bloated and ill." ) );
        add_morale( MORALE_NO_DIGEST, -25, -125, 300, 240 );
    }
    if( !spoiled && chew && has_trait( "SAPROPHAGE" ) ) {
        // It's OK to *drink* things that haven't rotted.  Alternative is to ban water.  D:
        add_msg_if_player( m_bad, _( "Your stomach begins gurgling and you feel bloated and ill." ) );
        add_morale( MORALE_NO_DIGEST, -75, -400, 300, 240 );
    }
    if( food.has_flag( "URSINE_HONEY" ) && ( !crossed_threshold() || has_trait( "THRESH_URSINE" ) ) &&
        mutation_category_level["MUTCAT_URSINE"] > 40 ) {
        //Need at least 5 bear muts for effect to show, to filter out mutations in common with other mutcats
        int honey_fun = has_trait( "THRESH_URSINE" ) ?
                        std::min( mutation_category_level["MUTCAT_URSINE"] / 8, 20 ) :
                        mutation_category_level["MUTCAT_URSINE"] / 12;
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
    if( !( has_bionic( "bio_digestion" ) || has_trait( "PARAIMMUNE" ) ) ) {
        if( food.type->comestible->parasites > 0 && one_in( food.type->comestible->parasites ) ) {
            switch( rng( 0, 3 ) ) {
                case 0:
                    if( !has_trait( "EATHEALTH" ) ) {
                        add_effect( effect_tapeworm, 1, num_bp, true );
                    }
                case 1:
                    if( !has_trait( "ACIDBLOOD" ) ) {
                        add_effect( effect_bloodworms, 1, num_bp, true );
                    }
                case 2:
                    add_effect( effect_brainworms, 1, num_bp, true );
                case 3:
                    add_effect( effect_paincysts, 1, num_bp, true );
            }
        }
    }

    for( const auto &v : this->vitamins_from( food ) ) {
        auto qty = has_effect( effect_tapeworm ) ? v.second / 2 : v.second;

        // can never develop hypervitaminosis from consuming food
        vitamin_mod( v.first, qty );
    }

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

    add_msg( m_debug, "%s nutrition cap: hunger %d, thirst %d, stomach food %d, stomach water %d",
             p.disp_name().c_str(), p.get_hunger(), p.get_thirst(), p.get_stomach_food(),
             p.get_stomach_water() );
}

void player::consume_effects( item &food, bool rotten )
{
    if( !food.is_food() ) {
        debugmsg( "called player::consume_effects with non-comestible" );
        return;
    }
    const auto comest = food.type->comestible.get();

    const int capacity = stomach_capacity();
    if( has_trait( "THRESH_PLANT" ) && food.type->can_use( "PLANTBLECH" ) ) {
        // Just keep nutrition capped, to prevent vomiting
        cap_nutrition_thirst( *this, capacity, true, true );
        return;
    }
    if( ( has_trait( "HERBIVORE" ) || has_trait( "RUMINANT" ) ) &&
        food.has_any_flag( herbivore_blacklist ) ) {
        // No good can come of this.
        return;
    }
    float factor = 1.0f;
    float hunger_factor = 1.0f;
    bool unhealthy_allowed = true;

    if( has_trait( "GIZZARD" ) ) {
        factor *= 0.6f;
    }

    if( has_trait( "CARNIVORE" ) && food.has_flag( "CARNIVORE_OK" ) ) {
        // At least partially edible
        if( food.has_any_flag( carnivore_blacklist ) ) {
            // Other things are in it, we only get partial benefits
            add_msg_if_player( _( "You pick out the edible parts and throw away the rest." ) );
            factor *= 0.5f;
        } else {
            // Carnivores don't get unhealthy off pure meat diets
            unhealthy_allowed = false;
        }
    }
    // Saprophages get full nutrition from rotting food
    if( rotten && !has_trait( "SAPROPHAGE" ) ) {
        // everyone else only gets a portion of the nutrition
        hunger_factor *= rng_float( 0, 1 );
        // and takes a health penalty if they aren't adapted
        if( !has_trait( "SAPROVORE" ) && !has_bionic( "bio_digestion" ) ) {
            mod_healthy_mod( -30, -200 );
        }
    }

    // Bio-digestion gives extra nutrition
    if( has_bionic( "bio_digestion" ) ) {
        hunger_factor += rng_float( 0, 1 );
    }

    const auto nutr = nutrition_for( food.type );
    mod_hunger( -nutr * factor * hunger_factor );
    mod_thirst( -comest->quench * factor );
    mod_stomach_food( nutr * factor * hunger_factor );
    mod_stomach_water( comest->quench * factor );
    if( unhealthy_allowed || comest->healthy > 0 ) {
        // Effectively no cap on health modifiers from food
        mod_healthy_mod( comest->healthy, ( comest->healthy >= 0 ) ? 200 : -200 );
    }

    if( comest->stim != 0 &&
        ( abs( stim ) < ( abs( comest->stim ) * 3 ) ||
          sgn( stim ) != sgn( comest->stim ) ) ) {
        if( comest->stim < 0 ) {
            stim = std::max( comest->stim * 3, stim + comest->stim );
        } else {
            stim = std::min( comest->stim * 3, stim + comest->stim );
        }
    }
    add_addiction( comest->add, comest->addict );
    if( addiction_craving( comest->add ) != MORALE_NULL ) {
        rem_morale( addiction_craving( comest->add ) );
    }

    // Morale is in minutes
    int morale_time = HOURS( 2 ) / MINUTES( 1 );
    if( food.has_flag( "HOT" ) && food.has_flag( "EATEN_HOT" ) ) {
        morale_time = HOURS( 3 ) / MINUTES( 1 );
        int clamped_nutr = std::max( 5, std::min( 20, nutr / 10 ) );
        add_morale( MORALE_FOOD_HOT, clamped_nutr, 20, morale_time, morale_time / 2 );
    }

    auto fun = comest->fun;
    auto fun_max = fun < 0 ? fun * 6 : fun * 3;
    if( food.has_flag( "EATEN_COLD" ) && food.has_flag( "COLD" ) ) {
        if( fun > 0 ) {
            fun *= 3;
        } else {
            fun = 1;
            fun_max = 5;
        }
    }

    const bool gourmand = has_trait( "GOURMAND" );
    if( gourmand ) {
        if( fun < -1 ) {
            fun_max = fun;
            fun /= 2;
        } else if( fun > 0 ) {
            fun_max = fun_max * 3 / 2;
            fun *= 3;
        }
    }

    if( fun < 0 ) {
        add_morale( MORALE_FOOD_BAD, fun, fun_max, morale_time, morale_time / 2, false, food.type );
    } else if( fun > 0 ) {
        add_morale( MORALE_FOOD_GOOD, fun, fun_max, morale_time, morale_time / 2, false, food.type );
    }

    const bool hibernate = has_active_mutation( "HIBERNATE" );
    if( hibernate ) {
        if( ( nutr > 0 && get_hunger() < -60 ) || ( comest->quench > 0 && get_thirst() < -60 ) ) {
            //Tell the player what's going on
            add_msg_if_player( _( "You gorge yourself, preparing to hibernate." ) );
            if( one_in( 2 ) ) {
                //50% chance of the food tiring you
                mod_fatigue( nutr );
            }
        }
        if( ( nutr > 0 && get_hunger() < -200 ) || ( comest->quench > 0 && get_thirst() < -200 ) ) {
            //Hibernation should cut burn to 60/day
            add_msg_if_player( _( "You feel stocked for a day or two. Got your bed all ready and secured?" ) );
            if( one_in( 2 ) ) {
                //And another 50%, intended cumulative
                mod_fatigue( nutr );
            }
        }

        if( ( nutr > 0 && get_hunger() < -400 ) || ( comest->quench > 0 && get_thirst() < -400 ) ) {
            add_msg_if_player(
                _( "Mmm.  You can still fit some more in...but maybe you should get comfortable and sleep." ) );
            if( !one_in( 3 ) ) {
                //Third check, this one at 66%
                mod_fatigue( nutr );
            }
        }
        if( ( nutr > 0 && get_hunger() < -600 ) || ( comest->quench > 0 && get_thirst() < -600 ) ) {
            add_msg_if_player( _( "That filled a hole!  Time for bed..." ) );
            // At this point, you're done.  Schlaf gut.
            mod_fatigue( nutr );
        }
    }

    // Moved here and changed a bit - it was too complex
    // Incredibly minor stuff like this shouldn't require complexity
    if( !is_npc() && has_trait( "SLIMESPAWNER" ) &&
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
            if( g->summon_mon( mon_player_blob, target ) ) {
                monster *slime = g->monster_at( target );
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
    if( get_hunger() < capacity && has_trait( "EATHEALTH" ) ) {
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

    cap_nutrition_thirst( *this, capacity, nutr > 0, comest->quench > 0 );
}

hint_rating player::rate_action_eat( const item &it ) const
{
    if( !it.is_food_container( this ) && !it.is_food( this ) ) {
        return HINT_CANT;
    }

    const auto rating = can_eat( it );
    if( rating == EDIBLE ) {
        return HINT_GOOD;
    } else if( rating == INEDIBLE || rating == INEDIBLE_MUTATION ) {
        return HINT_CANT;
    }

    return HINT_IFFY;
}
