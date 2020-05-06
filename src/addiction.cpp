#include "addiction.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <utility>

#include "calendar.h"
#include "character.h"
#include "debug.h"
#include "enums.h"
#include "morale_types.h"
#include "rng.h"
#include "translations.h"

static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_shakes( "shakes" );

static const trait_id trait_MUT_JUNKIE( "MUT_JUNKIE" );

namespace io
{

template<>
std::string enum_to_string<add_type>( add_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case add_type::NONE: return "NONE";
        case add_type::CAFFEINE: return "CAFFEINE";
        case add_type::ALCOHOL: return "ALCOHOL";
        case add_type::SLEEP: return "SLEEP";
        case add_type::PKILLER: return "PKILLER";
        case add_type::SPEED: return "SPEED";
        case add_type::CIG: return "CIG";
        case add_type::COKE: return "COKE";
        case add_type::CRACK: return "CRACK";
        case add_type::MUTAGEN: return "MUTAGEN";
        case add_type::DIAZEPAM: return "DIAZEPAM";
        case add_type::MARLOSS_R: return "MARLOSS_R";
        case add_type::MARLOSS_B: return "MARLOSS_B";
        case add_type::MARLOSS_Y: return "MARLOSS_Y";
        // *INDENT-ON*
        case add_type::NUM_ADD_TYPES:
            break;
    }
    debugmsg( "Invalid add_type" );
    abort();
}

} // namespace io

void marloss_add( Character &u, int in, const char *msg );

void addict_effect( Character &u, addiction &add )
{
    const int in = std::min( 20, add.intensity );
    const int current_stim = u.get_stim();

    switch( add.type ) {
        case add_type::CIG:
            if( !one_in( 2000 - 20 * in ) ) {
                break;
            }

            u.add_msg_if_player( rng( 0, 6 ) < in ?
                                 _( "You need some nicotine." ) :
                                 _( "You could use some nicotine." ) );
            u.add_morale( MORALE_CRAVING_NICOTINE, -15, -3 * in );
            if( one_in( 800 - 50 * in ) ) {
                u.mod_fatigue( 1 );
            }
            if( current_stim > -5 * in && one_in( 400 - 20 * in ) ) {
                u.mod_stim( -1 );
            }
            break;

        case add_type::CAFFEINE:
            if( !one_in( 2000 - 20 * in ) ) {
                break;
            }

            u.add_msg_if_player( m_warning, _( "You want some caffeine." ) );
            u.add_morale( MORALE_CRAVING_CAFFEINE, -5, -30 );
            if( current_stim > -10 * in && rng( 0, 10 ) < in ) {
                u.mod_stim( -1 );
            }
            if( rng( 8, 400 ) < in ) {
                u.add_msg_if_player( m_bad, _( "Your hands start shaking… you need it bad!" ) );
                u.add_effect( effect_shakes, 2_minutes );
            }
            break;

        case add_type::ALCOHOL:
        case add_type::DIAZEPAM: {
            const auto morale_type = add.type == add_type::ALCOHOL ? MORALE_CRAVING_ALCOHOL :
                                     MORALE_CRAVING_DIAZEPAM;

            u.mod_per_bonus( -1 );
            u.mod_int_bonus( -1 );
            if( x_in_y( in, to_turns<int>( 2_hours ) ) ) {
                u.mod_healthy_mod( -1, -in * 10 );
            }
            if( one_in( 20 ) && rng( 0, 20 ) < in ) {
                const std::string msg_1 = add.type == add_type::ALCOHOL ?
                                          _( "You could use a drink." ) :
                                          _( "You could use some diazepam." );
                u.add_msg_if_player( m_warning, msg_1 );
                u.add_morale( morale_type, -35, -10 * in );
            } else if( rng( 8, 300 ) < in ) {
                const std::string msg_2 = add.type == add_type::ALCOHOL ?
                                          _( "Your hands start shaking… you need a drink bad!" ) :
                                          _( "You're shaking… you need some diazepam!" );
                u.add_msg_if_player( m_bad, msg_2 );
                u.add_morale( morale_type, -35, -10 * in );
                u.add_effect( effect_shakes, 5_minutes );
            } else if( !u.has_effect( effect_hallu ) && rng( 10, 1600 ) < in ) {
                u.add_effect( effect_hallu, 6_hours );
            }
            break;
        }

        case add_type::SLEEP:
            // No effects here--just in player::can_sleep()
            // EXCEPT!  Prolong this addiction longer than usual.
            if( one_in( 2 ) && add.sated < 0_turns ) {
                add.sated += 1_turns;
            }
            break;

        case add_type::PKILLER:
            if( calendar::once_every( time_duration::from_turns( 100 - in * 4 ) ) &&
                u.get_painkiller() > 20 - in ) {
                // Tolerance increases!
                u.mod_painkiller( -1 );
            }
            // No further effects if we're doped up.
            if( u.get_painkiller() >= 35 ) {
                add.sated = 0_turns;
                break;
            }

            u.mod_str_bonus( -1 );
            u.mod_per_bonus( -1 );
            u.mod_dex_bonus( -1 );
            if( u.get_pain() < in * 2 ) {
                u.mod_pain( 1 );
            }
            if( one_in( 1200 - 30 * in ) ) {
                u.mod_healthy_mod( -1, -in * 30 );
            }
            if( one_in( 20 ) && dice( 2, 20 ) < in ) {
                u.add_msg_if_player( m_bad, _( "Your hands start shaking… you need some painkillers." ) );
                u.add_morale( MORALE_CRAVING_OPIATE, -40, -10 * in );
                u.add_effect( effect_shakes, 2_minutes + in * 30_seconds );
            } else if( one_in( 20 ) && dice( 2, 30 ) < in ) {
                u.add_msg_if_player( m_bad, _( "You feel anxious.  You need your painkillers!" ) );
                u.add_morale( MORALE_CRAVING_OPIATE, -30, -10 * in );
            } else if( one_in( 50 ) && dice( 3, 50 ) < in ) {
                u.vomit();
            }
            break;

        case add_type::SPEED: {
            u.mod_int_bonus( -1 );
            u.mod_str_bonus( -1 );
            if( current_stim > -100 && x_in_y( in, 20 ) ) {
                u.mod_stim( -1 );
            }
            if( rng( 0, 150 ) <= in ) {
                u.mod_healthy_mod( -1, -in );
            }
            if( dice( 2, 100 ) < in ) {
                u.add_msg_if_player( m_warning, _( "You feel depressed.  Speed would help." ) );
                u.add_morale( MORALE_CRAVING_SPEED, -25, -20 * in );
            } else if( one_in( 10 ) && dice( 2, 80 ) < in ) {
                u.add_msg_if_player( m_bad, _( "Your hands start shaking… you need a pick-me-up." ) );
                u.add_morale( MORALE_CRAVING_SPEED, -25, -20 * in );
                u.add_effect( effect_shakes, in * 2_minutes );
            } else if( one_in( 50 ) && dice( 2, 100 ) < in ) {
                u.add_msg_if_player( m_bad, _( "You stop suddenly, feeling bewildered." ) );
                u.moves -= 300;
            } else if( !u.has_effect( effect_hallu ) && one_in( 20 ) && 8 + dice( 2, 80 ) < in ) {
                u.add_effect( effect_hallu, 6_hours );
            }
        }
        break;

        case add_type::COKE:
        case add_type::CRACK: {
            const std::string &cur_msg = add.type == add_type::COKE ?
                                         _( "You feel like you need a bump." ) :
                                         _( "You're shivering, you need some crack." );
            const auto morale_type = add.type == add_type::COKE ? MORALE_CRAVING_COCAINE :
                                     MORALE_CRAVING_CRACK;
            u.mod_int_bonus( -1 );
            u.mod_per_bonus( -1 );
            if( one_in( 900 - 30 * in ) ) {
                u.add_msg_if_player( m_warning, cur_msg );
                u.add_morale( morale_type, -20, -15 * in );
            }
            if( dice( 2, 80 ) <= in ) {
                u.add_msg_if_player( m_warning, cur_msg );
                u.add_morale( morale_type, -20, -15 * in );
                if( current_stim > -150 ) {
                    u.mod_stim( -3 );
                }
            }
            break;
        }

        case add_type::MUTAGEN:
            if( u.has_trait( trait_MUT_JUNKIE ) ) {
                if( one_in( 600 - 50 * in ) ) {
                    u.add_msg_if_player( m_warning, rng( 0,
                                                         6 ) < in ? _( "You so miss the exquisite rainbow of post-humanity." ) :
                                         _( "Your body is SOO booorrrring.  Just a little sip to liven things up?" ) );
                    u.add_morale( MORALE_CRAVING_MUTAGEN, -20, -200 );
                }
                if( u.focus_pool > 40 && one_in( 800 - 20 * in ) ) {
                    u.focus_pool -= in;
                    u.add_msg_if_player( m_warning,
                                         _( "You daydream what it'd be like if you were *different*.  Different is good." ) );
                }
            } else if( in > 5 || one_in( 500 - 15 * in ) ) {
                u.add_msg_if_player( m_warning, rng( 0, 6 ) < in ? _( "You haven't had any mutagen lately." ) :
                                     _( "You could use some new parts…" ) );
                u.add_morale( MORALE_CRAVING_MUTAGEN, -5, -50 );
            }
            break;
        case add_type::MARLOSS_R:
            marloss_add( u, in, _( "You daydream about luscious pink berries as big as your fist." ) );
            break;
        case add_type::MARLOSS_B:
            marloss_add( u, in, _( "You daydream about nutty cyan seeds as big as your hand." ) );
            break;
        case add_type::MARLOSS_Y:
            marloss_add( u, in, _( "You daydream about succulent, pale golden gel, sweet but light." ) );
            break;
        case add_type::NONE:
        case add_type::NUM_ADD_TYPES:
            break;
    }
}

/**
 * Returns the name of an addiction. It should be able to finish the sentence
 * "Became addicted to ______".
 */
std::string addiction_type_name( add_type const cur )
{
    static const std::map<add_type, std::string> type_map = {{
            { add_type::CIG, translate_marker( "nicotine" ) },
            { add_type::CAFFEINE, translate_marker( "caffeine" ) },
            { add_type::ALCOHOL, translate_marker( "alcohol" ) },
            { add_type::SLEEP, translate_marker( "sleeping pills" ) },
            { add_type::PKILLER, translate_marker( "opiates" ) },
            { add_type::SPEED, translate_marker( "amphetamine" ) },
            { add_type::COKE, translate_marker( "cocaine" ) },
            { add_type::CRACK, translate_marker( "crack cocaine" ) },
            { add_type::MUTAGEN, translate_marker( "mutation" ) },
            { add_type::DIAZEPAM, translate_marker( "diazepam" ) },
            { add_type::MARLOSS_R, translate_marker( "Marloss berries" ) },
            { add_type::MARLOSS_B, translate_marker( "Marloss seeds" ) },
            { add_type::MARLOSS_Y, translate_marker( "Marloss gel" ) },
        }
    };

    const auto iter = type_map.find( cur );
    if( iter != type_map.end() ) {
        return _( iter->second );
    }

    return "bugs in addiction.cpp";
}

std::string addiction_name( const addiction &cur )
{
    static const std::map<add_type, std::string> type_map = {{
            { add_type::CIG, translate_marker( "Nicotine Withdrawal" ) },
            { add_type::CAFFEINE, translate_marker( "Caffeine Withdrawal" ) },
            { add_type::ALCOHOL, translate_marker( "Alcohol Withdrawal" ) },
            { add_type::SLEEP, translate_marker( "Sleeping Pill Dependence" ) },
            { add_type::PKILLER, translate_marker( "Opiate Withdrawal" ) },
            { add_type::SPEED, translate_marker( "Amphetamine Withdrawal" ) },
            { add_type::COKE, translate_marker( "Cocaine Withdrawal" ) },
            { add_type::CRACK, translate_marker( "Crack Cocaine Withdrawal" ) },
            { add_type::MUTAGEN, translate_marker( "Mutation Withdrawal" ) },
            { add_type::DIAZEPAM, translate_marker( "Diazepam Withdrawal" ) },
            { add_type::MARLOSS_R, translate_marker( "Marloss Longing" ) },
            { add_type::MARLOSS_B, translate_marker( "Marloss Desire" ) },
            { add_type::MARLOSS_Y, translate_marker( "Marloss Cravings" ) },
        }
    };

    const auto iter = type_map.find( cur.type );
    if( iter != type_map.end() ) {
        return _( iter->second );
    }

    return "Erroneous addiction";
}

morale_type addiction_craving( add_type const cur )
{
    static const std::map<add_type, morale_type> type_map = {{
            { add_type::CIG, MORALE_CRAVING_NICOTINE },
            { add_type::CAFFEINE, MORALE_CRAVING_CAFFEINE },
            { add_type::ALCOHOL, MORALE_CRAVING_ALCOHOL },
            { add_type::PKILLER, MORALE_CRAVING_OPIATE },
            { add_type::SPEED, MORALE_CRAVING_SPEED },
            { add_type::COKE, MORALE_CRAVING_COCAINE },
            { add_type::CRACK, MORALE_CRAVING_CRACK },
            { add_type::MUTAGEN, MORALE_CRAVING_MUTAGEN },
            { add_type::DIAZEPAM, MORALE_CRAVING_DIAZEPAM },
            { add_type::MARLOSS_R, MORALE_CRAVING_MARLOSS },
            { add_type::MARLOSS_B, MORALE_CRAVING_MARLOSS },
            { add_type::MARLOSS_Y, MORALE_CRAVING_MARLOSS },
        }
    };

    const auto iter = type_map.find( cur );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    return MORALE_NULL;
}

add_type addiction_type( const std::string &name )
{
    static const std::map<std::string, add_type> type_map = {{
            { "nicotine", add_type::CIG },
            { "caffeine", add_type::CAFFEINE },
            { "alcohol", add_type::ALCOHOL },
            { "sleeping pill", add_type::SLEEP },
            { "opiate", add_type::PKILLER },
            { "amphetamine", add_type::SPEED },
            { "cocaine", add_type::COKE },
            { "crack", add_type::CRACK },
            { "mutagen", add_type::MUTAGEN },
            { "diazepam", add_type::DIAZEPAM },
            { "marloss_r", add_type::MARLOSS_R },
            { "marloss_b", add_type::MARLOSS_B },
            { "marloss_y", add_type::MARLOSS_Y },
            { "none", add_type::NONE }
        }
    };

    const auto iter = type_map.find( name );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    return add_type::NONE;
}

std::string addiction_text( const addiction &cur )
{
    static const std::map<add_type, std::string> addiction_msg = {{
            { add_type::CIG, translate_marker( "Intelligence - 1;   Occasional cravings" ) },
            { add_type::CAFFEINE, translate_marker( "Strength - 1;   Slight sluggishness;   Occasional cravings" )  },
            {
                add_type::ALCOHOL, translate_marker( "Perception - 1;   Intelligence - 1;   Occasional Cravings;\nRisk of delirium tremens" )
            },
            { add_type::SLEEP, translate_marker( "You may find it difficult to sleep without medication." ) },
            {
                add_type::PKILLER, translate_marker( "Strength - 1;   Perception - 1;   Dexterity - 1;\nDepression and physical pain to some degree.  Frequent cravings.  Vomiting." )
            },
            { add_type::SPEED, translate_marker( "Strength - 1;   Intelligence - 1;\nMovement rate reduction.  Depression.  Weak immune system.  Frequent cravings." ) },
            { add_type::COKE, translate_marker( "Perception - 1;   Intelligence - 1;   Frequent cravings." ) },
            { add_type::CRACK, translate_marker( "Perception - 2;   Intelligence - 2;   Frequent cravings." ) },
            { add_type::MUTAGEN, translate_marker( "You've gotten a taste for mutating and the chemicals that cause it.  But you can stop, yeah, any time you want." ) },
            {
                add_type::DIAZEPAM, translate_marker( "Perception - 1;   Intelligence - 1;\nAnxiety, nausea, hallucinations, and general malaise." )
            },
            { add_type::MARLOSS_R, translate_marker( "You should try some of those pink berries." ) },
            { add_type::MARLOSS_B, translate_marker( "You should try some of those cyan seeds." ) },
            { add_type::MARLOSS_Y, translate_marker( "You should try some of that golden gel." ) },
        }
    };

    const auto iter = addiction_msg.find( cur.type );
    if( iter != addiction_msg.end() ) {
        return _( iter->second );
    }

    return "You crave to report this bug.";
}

void marloss_add( Character &u, int in, const char *msg )
{
    if( one_in( 800 - 20 * in ) ) {
        u.add_morale( MORALE_CRAVING_MARLOSS, -5, -25 );
        u.add_msg_if_player( m_info, msg );
        if( u.focus_pool > 40 ) {
            u.focus_pool -= in;
        }
    }
}
