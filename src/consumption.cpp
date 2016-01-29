#include "player.h"
#include "monster.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "itype.h"
#include "messages.h"
#include "addiction.h"

const efftype_id effect_foodpoison( "foodpoison" );
const efftype_id effect_poison( "poison" );

const mtype_id mon_player_blob( "mon_player_blob" );

static const std::vector<std::string> carnivore_blacklist = {{
        "veggy", "fruit", "wheat"
    }
};
static const std::vector<std::string> carnivore_whitelist = {{
        "flesh", "hflesh", "iflesh", "milk", "egg"
    }
};
static const std::vector<std::string> herbivore_blacklist = {{
        "flesh", "hflesh", "iflesh", "egg"
    }
};
static const std::vector<std::string> cannibal_traits = {{
        "CANNIBAL", "PSYCHOPATH", "SAPIOVORE"
    }
};

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

// TODO: Move pizza scraping here
// This would require carnivore check to be doable on it_comest alone
// For example, by using tags rather than materials
int player::nutrition_for( const it_comest *comest ) const
{
    // First value is hunger, second is nutrition multiplier
    using threshold_pair = std::pair<int, float>;
    static const std::array<threshold_pair, 7> thresholds = {{
            { INT_MIN, 1.0f },
            { 100, 1.0f },
            { 300, 2.0f },
            { 1400, 4.0f },
            { 2800, 6.0f },
            { 6000, 10.0f },
            { INT_MAX, 10.0f }
        }
    };

    const int hng = get_hunger();
    // Find the first threshold > hunger
    int i = 1;
    while( thresholds[i].first <= hng ) {
        i++;
    }

    // How far are we along the way from last threshold to current one
    const float t = ( float )( hng - thresholds[i - 1].first ) /
                    ( thresholds[i].first - thresholds[i - 1].first );

    // Linear interpolation of values at relevant thresholds
    const float modifier = ( t * thresholds[i].second ) +
                           ( ( 1 - t ) * thresholds[i - 1].second );

    return lround( comest->get_nutrition() * modifier );
}

morale_type player::allergy_type( const item &food ) const
{
    using allergy_tuple = std::tuple<std::string, std::string, morale_type>;
    static const std::array<allergy_tuple, 8> allergy_tuples = {{
            std::make_tuple( "VEGETARIAN", "flesh", MORALE_VEGETARIAN ),
            std::make_tuple( "VEGETARIAN", "hflesh", MORALE_VEGETARIAN ),
            std::make_tuple( "VEGETARIAN", "iflesh", MORALE_VEGETARIAN ),
            std::make_tuple( "MEATARIAN", "veggy", MORALE_MEATARIAN ),
            std::make_tuple( "LACTOSE", "milk", MORALE_LACTOSE ),
            std::make_tuple( "ANTIFRUIT", "fruit", MORALE_ANTIFRUIT ),
            std::make_tuple( "ANTIJUNK", "junk", MORALE_ANTIJUNK ),
            std::make_tuple( "ANTIWHEAT", "wheat", MORALE_ANTIWHEAT )
        }
    };

    for( const auto &tp : allergy_tuples ) {
        if( has_trait( std::get<0>( tp ) ) &&
            food.made_of( std::get<1>( tp ) ) ) {
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

    const auto comest = dynamic_cast<const it_comest *>( food.type );
    if( comest == nullptr ) {
        maybe_print( m_info, _( "That doesn't look edible." ) );
        return INEDIBLE;
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
    if( has_trait( "M_DEPENDENT" ) && food.type->id != "mycus_fruit" ) {
        maybe_print( m_info, _( "We can't eat that.  It's not right for us." ) );
        return INEDIBLE_MUTATION;
    }

    const bool drinkable = comest->comesttype == "DRINK" && !food.has_flag( "USE_EAT_VERB" );
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
    const int nutr = nutrition_for( comest );
    const int quench = comest->quench;
    bool spoiled = food.rotten();

    const int temp_hunger = get_hunger() - nutr;
    const int temp_thirst = thirst - quench;

    const bool overeating = get_hunger() < 0 && nutr >= 5 && !gourmand && !eathealth && !slimespawner &&
                            !hibernate;

    if( interactive && hibernate &&
        ( get_hunger() >= -60 && thirst >= -60 ) &&
        ( temp_hunger < -60 || temp_thirst < -60 ) ) {
        if( !maybe_query( _( "You're adequately fueled. Prepare for hibernation?" ) ) ) {
            return TOO_FULL;
        }
    }

    const bool carnivore = has_trait( "CARNIVORE" );
    if( carnivore && nutr > 0 &&
        food.made_of_any( carnivore_blacklist ) && !food.made_of_any( carnivore_whitelist ) ) {
        maybe_print( m_info, _( "Eww.  Inedible plant stuff!" ) );
        return INEDIBLE_MUTATION;
    }

    if( ( has_trait( "HERBIVORE" ) || has_trait( "RUMINANT" ) ) &&
        food.made_of_any( herbivore_blacklist ) ) {
        // Like non-cannibal, but more strict!
        maybe_print( m_info, _( "The thought of eating that makes you feel sick.  You decide not to." ) );
        return INEDIBLE_MUTATION;
    }

    if( food.made_of( "hflesh" ) ) {
        const bool is_cannibal = std::any_of( cannibal_traits.begin(), cannibal_traits.end(),
        [this]( const std::string & trait ) {
            return has_trait( trait );
        } );
        if( !is_cannibal &&
            !maybe_query( _( "The thought of eating that makes you feel sick.  Really do it?" ) ) ) {
            return CANNIBALISM;
        }
    }

    if( is_allergic( food ) &&
        !maybe_query( _( "Really eat that %s?  Your stomach won't be happy." ) ) ) {
        return ALLERGY;
    }

    if( carnivore && food.made_of( "junk" ) && !food.made_of_any( carnivore_whitelist ) &&
        !maybe_query( _( "Really eat that %s?  Your stomach won't be happy." ) ) ) {
        return ALLERGY;
    }

    const bool saprophage = has_trait( "SAPROPHAGE" );
    // The item is solid food
    const bool chew = comest->comesttype == "FOOD" || food.has_flag( "USE_EAT_VERB" );
    if( spoiled ) {
        if( !saprophage && !has_trait( "SAPROVORE" ) &&
            !maybe_query( _( "This %s smells awful!  Eat it?" ) ) ) {
            return ROTTEN;
        }
    } else if( saprophage && chew && !food.has_flag( "FERTILIZER" ) &&
               !maybe_query( _( "Really eat that %s?  Your stomach won't be happy." ) ) ) {
        // Note: We're allowing all non-solid "food". This includes drugs
        // Hardcoding fertilizer for now - should be a separate flag later
        //~ No, we don't eat "rotten" food. We eat properly aged food, like a normal person.
        //~ Semantic difference, but greatly facilitates people being proud of their character.
        maybe_print( m_info, _( "It's too fresh, let it age a little first." ) );
        return ROTTEN;
    }

    // Print at most one of those
    bool overfull = false;
    if( overeating ) {
        overfull = !maybe_query( _( "You're full.  Force yourself to eat?" ) );
    } else if( ( ( nutr > 0 && temp_hunger < capacity ) ||
                 ( comest->quench > 0 && temp_thirst < capacity ) ) &&
               !eathealth && !slimespawner ) {
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
    // Check if it's rotten before eating!
    food.calc_rot( global_square_location() );
    const auto edible = can_eat( food, is_player() && !force, force );
    if( edible != EDIBLE ) {
        return false;
    }

    const auto comest = dynamic_cast<const it_comest *>( food.type );
    if( comest->has_use() ) {
        const auto charges_consumed = comest->invoke( this, &food, pos() );
        if( charges_consumed <= 0 ) {
            return false;
        }
    }

    // Note: the block below assumes we decided to eat it
    // No coming back from here

    const bool hibernate = has_active_mutation( "HIBERNATE" );
    const int nutr = nutrition_for( comest );
    const int quench = comest->quench;
    const bool spoiled = food.rotten();

    // The item is solid food
    const bool chew = comest->comesttype == "FOOD" || food.has_flag( "USE_EAT_VERB" );
    // This item is a drink and not a solid food (and not a thick soup)
    const bool drinkable = !chew && comest->comesttype == "DRINK";
    // If neither of the above is true then it's a drug and shouldn't get mealtime penalty/bonus

    if( hibernate &&
        ( get_hunger() > -60 && thirst > -60 ) &&
        ( get_hunger() - nutr < -60 || thirst - quench < -60 ) ) {
        add_memorial_log( pgettext( "memorial_male", "Began preparing for hibernation." ),
                          pgettext( "memorial_female", "Began preparing for hibernation." ) );
        add_msg_if_player(
            _( "You've begun stockpiling calories and liquid for hibernation.  You get the feeling that you should prepare for bed, just in case, but...you're hungry again, and you could eat a whole week's worth of food RIGHT NOW." ) );
    }

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

    if( get_hunger() < 0 && nutr >= 5 && !has_trait( "GOURMAND" ) && !hibernate &&
        !has_trait( "SLIMESPAWNER" ) && !has_trait( "EATHEALTH" ) &&
        rng( -200, 0 ) > get_hunger() ) {
        vomit();
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

    if( item::find_type( comest->tool )->is_tool() ) {
        // Tools like lighters get used
        use_charges( comest->tool, 1 );
    }

    if( has_bionic( "bio_ethanol" ) && comest->can_use( "ALCOHOL" ) ) {
        charge_power( rng( 50, 200 ) );
    }
    if( has_bionic( "bio_ethanol" ) && comest->can_use( "ALCOHOL_WEAK" ) ) {
        charge_power( rng( 25, 100 ) );
    }
    if( has_bionic( "bio_ethanol" ) && comest->can_use( "ALCOHOL_STRONG" ) ) {
        charge_power( rng( 75, 300 ) );
    }
    // Eating plant fertilizer stops here
    if( comest->can_use( "PLANTBLECH" ) && has_trait( "THRESH_PLANT" ) ) {
        return true;
    }
    if( food.made_of( "hflesh" ) ) {
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
    if( has_trait( "CARNIVORE" ) && food.made_of( "junk" ) &&
        !food.made_of_any( carnivore_whitelist ) ) {
        add_msg_if_player( m_bad, _( "Your stomach begins gurgling and you feel bloated and ill." ) );
        add_morale( MORALE_NO_DIGEST, -25, -125, 300, 240 );
    }
    if( !spoiled && chew && has_trait( "SAPROPHAGE" ) ) {
        // It's OK to *drink* things that haven't rotted.  Alternative is to ban water.  D:
        add_msg_if_player( m_bad, _( "Your stomach begins gurgling and you feel bloated and ill." ) );
        add_morale( MORALE_NO_DIGEST, -75, -400, 300, 240 );
    }
    if( food.made_of( "honey" ) && ( !crossed_threshold() || has_trait( "THRESH_URSINE" ) ) &&
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

    return true;
}

// Caps both actual nutrition/thirst and stomach capacity
void cap_nutrition_thirst( player &p, int capacity, bool food, bool water )
{
    if( ( food && p.get_hunger() < capacity ) ||
        ( water && p.thirst < capacity ) ) {
        p.add_msg_if_player( _( "You can't finish it all!" ) );
    }

    if( p.get_hunger() < capacity ) {
        p.mod_stomach_food( p.get_hunger() - capacity );
        p.set_hunger( capacity );
    }

    if( p.thirst < capacity ) {
        p.mod_stomach_water( p.thirst - capacity );
        p.thirst = capacity;
    }

    add_msg( m_debug, "%s nutrition cap: hunger %d, thirst %d, stomach food %d, stomach water %d",
             p.disp_name().c_str(), p.get_hunger(), p.thirst, p.get_stomach_food(), p.get_stomach_water() );
}

void player::consume_effects( item &food, bool rotten )
{
    const int capacity = stomach_capacity();
    const auto comest = dynamic_cast<const it_comest *>( food.type );
    if( has_trait( "THRESH_PLANT" ) && comest->can_use( "PLANTBLECH" ) ) {
        // Just keep nutrition capped, to prevent vomiting
        cap_nutrition_thirst( *this, capacity, true, true );
        return;
    }
    if( food.made_of_any( herbivore_blacklist ) &&
        ( has_trait( "HERBIVORE" ) || has_trait( "RUMINANT" ) ) ) {
        // No good can come of this.
        return;
    }
    float factor = 1.0;
    float hunger_factor = 1.0;
    bool unhealthy_allowed = true;

    if( has_trait( "GIZZARD" ) ) {
        factor *= .6;
    }

    if( has_trait( "CARNIVORE" ) && food.made_of_any( carnivore_whitelist ) ) {
        // At least partially edible
        if( food.made_of_any( carnivore_blacklist ) ) {
            // Other things are in it, we only get partial benefits
            add_msg_if_player( _( "You pick out the edible parts and throw away the rest." ) );
            factor *= .5;
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

    const auto nutr = nutrition_for( comest );
    mod_hunger( -nutr * factor * hunger_factor );
    thirst -= comest->quench * factor;
    mod_stomach_food( nutr * factor * hunger_factor );
    mod_stomach_water( comest->quench * factor );
    if( unhealthy_allowed || comest->healthy > 0 ) {
        // Effectively no upper cap on healthy food, moderate cap on unhealthy food.
        mod_healthy_mod( comest->healthy, ( comest->healthy >= 0 ) ? 200 : -50 );
    }

    if( comest->stim != 0 ) {
        if( abs( stim ) < ( abs( comest->stim ) * 3 ) ) {
            if( comest->stim < 0 ) {
                stim = std::max( comest->stim * 3, stim + comest->stim );
            } else {
                stim = std::min( comest->stim * 3, stim + comest->stim );
            }
        }
    }
    add_addiction( comest->add, comest->addict );
    if( addiction_craving( comest->add ) != MORALE_NULL ) {
        rem_morale( addiction_craving( comest->add ) );
    }
    if( food.has_flag( "HOT" ) && food.has_flag( "EATEN_HOT" ) ) {
        add_morale( MORALE_FOOD_HOT, 5, 10 );
    }
    auto fun = comest->fun;
    if( food.has_flag( "COLD" ) && food.has_flag( "EATEN_COLD" ) && fun > 0 ) {
        if( fun > 0 ) {
            add_morale( MORALE_FOOD_GOOD, fun * 3, fun * 3, 60, 30, false, comest );
        } else {
            fun = 1;
        }
    }

    const bool gourmand = has_trait( "GOURMAND" );
    const bool hibernate = has_active_mutation( "HIBERNATE" );
    if( gourmand ) {
        if( fun < -2 ) {
            add_morale( MORALE_FOOD_BAD, fun * 0.5, fun, 60, 30, false, comest );
        } else if( fun > 0 ) {
            add_morale( MORALE_FOOD_GOOD, fun * 3, fun * 6, 60, 30, false, comest );
        }
    } else if( fun < 0 ) {
        add_morale( MORALE_FOOD_BAD, fun, fun * 6, 60, 30, false, comest );
    } else if( fun > 0 ) {
        add_morale( MORALE_FOOD_GOOD, fun, fun * 4, 60, 30, false, comest );
    }

    if( hibernate ) {
        if( ( nutr > 0 && get_hunger() < -60 ) || ( comest->quench > 0 && thirst < -60 ) ) {
            //Tell the player what's going on
            add_msg_if_player( _( "You gorge yourself, preparing to hibernate." ) );
            if( one_in( 2 ) ) {
                //50% chance of the food tiring you
                fatigue += nutr;
            }
        }
        if( ( nutr > 0 && get_hunger() < -200 ) || ( comest->quench > 0 && thirst < -200 ) ) {
            //Hibernation should cut burn to 60/day
            add_msg_if_player( _( "You feel stocked for a day or two. Got your bed all ready and secured?" ) );
            if( one_in( 2 ) ) {
                //And another 50%, intended cumulative
                fatigue += nutr;
            }
        }

        if( ( nutr > 0 && get_hunger() < -400 ) || ( comest->quench > 0 && thirst < -400 ) ) {
            add_msg_if_player(
                _( "Mmm.  You can still fit some more in...but maybe you should get comfortable and sleep." ) );
            if( !one_in( 3 ) ) {
                //Third check, this one at 66%
                fatigue += nutr;
            }
        }
        if( ( nutrition_for( comest ) > 0 && get_hunger() < -600 ) || ( comest->quench > 0 &&
                thirst < -600 ) ) {
            add_msg_if_player( _( "That filled a hole!  Time for bed..." ) );
            // At this point, you're done.  Schlaf gut.
            fatigue += nutr;
        }
    }

    // Moved here and changed a bit - it was too complex
    // Incredibly minor stuff like this shouldn't require complexity
    if( !is_npc() && has_trait( "SLIMESPAWNER" ) &&
        ( get_hunger() < capacity + 40 || thirst < capacity + 40 ) ) {
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
        thirst += 40;
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
