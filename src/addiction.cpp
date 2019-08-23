#include "addiction.h"

#include <algorithm>
#include <map>
#include <utility>

#include "morale_types.h"
#include "player.h"
#include "pldata.h"
#include "rng.h"
#include "translations.h"
#include "calendar.h"
#include "enums.h"

const efftype_id effect_hallu( "hallu" );
const efftype_id effect_shakes( "shakes" );

namespace io
{

template<>
std::string enum_to_string<add_type>( add_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case add_type::ADD_NULL: return "NULL";
        case add_type::ADD_CAFFEINE: return "CAFFEINE";
        case add_type::ADD_ALCOHOL: return "ALCOHOL";
        case add_type::ADD_SLEEP: return "SLEEP";
        case add_type::ADD_PKILLER: return "PKILLER";
        case add_type::ADD_SPEED: return "SPEED";
        case add_type::ADD_CIG: return "CIG";
        case add_type::ADD_COKE: return "COKE";
        case add_type::ADD_CRACK: return "CRACK";
        case add_type::ADD_MUTAGEN: return "MUTAGEN";
        case add_type::ADD_DIAZEPAM: return "DIAZEPAM";
        case add_type::ADD_MARLOSS_R: return "MARLOSS_R";
        case add_type::ADD_MARLOSS_B: return "MARLOSS_B";
        case add_type::ADD_MARLOSS_Y: return "MARLOSS_Y";
        // *INDENT-ON*
        case add_type::NUM_ADD_TYPES:
            break;
    }
    debugmsg( "Invalid add_type" );
    abort();
}

}

void marloss_add( player &u, int in, const char *msg );

void addict_effect( player &u, addiction &add )
{
    const int in = std::min( 20, add.intensity );

    switch( add.type ) {
        case ADD_CIG:
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
            if( u.stim > -5 * in && one_in( 400 - 20 * in ) ) {
                u.stim--;
            }
            break;

        case ADD_CAFFEINE:
            if( !one_in( 2000 - 20 * in ) ) {
                break;
            }

            u.add_msg_if_player( m_warning, _( "You want some caffeine." ) );
            u.add_morale( MORALE_CRAVING_CAFFEINE, -5, -30 );
            if( u.stim > -10 * in && rng( 0, 10 ) < in ) {
                u.stim--;
            }
            if( rng( 8, 400 ) < in ) {
                u.add_msg_if_player( m_bad, _( "Your hands start shaking... you need it bad!" ) );
                u.add_effect( effect_shakes, 2_minutes );
            }
            break;

        case ADD_ALCOHOL:
        case ADD_DIAZEPAM: {
            static const std::string alc_1 = _( "You could use a drink. " );
            static const std::string alc_2 = _( "Your hands start shaking... you need a drink bad!" ) ;
            static const std::string dia_1 = _( "You could use some diazepam." );
            static const std::string dia_2 = _( "You're shaking... you need some diazepam!" );

            const std::string &msg_1 = add.type == ADD_ALCOHOL ? alc_1 : dia_1;
            const std::string &msg_2 = add.type == ADD_ALCOHOL ? alc_2 : dia_2;

            const auto morale_type = add.type == ADD_ALCOHOL ? MORALE_CRAVING_ALCOHOL : MORALE_CRAVING_DIAZEPAM;

            u.mod_per_bonus( -1 );
            u.mod_int_bonus( -1 );
            if( x_in_y( in, to_turns<int>( 2_hours ) ) ) {
                u.mod_healthy_mod( -1, -in * 10 );
            }
            if( one_in( 20 ) && rng( 0, 20 ) < in ) {
                u.add_msg_if_player( m_warning, msg_1 );
                u.add_morale( morale_type, -35, -10 * in );
            } else if( rng( 8, 300 ) < in ) {
                u.add_msg_if_player( m_bad, msg_2 );
                u.add_morale( morale_type, -35, -10 * in );
                u.add_effect( effect_shakes, 5_minutes );
            } else if( !u.has_effect( effect_hallu ) && rng( 10, 1600 ) < in ) {
                u.add_effect( effect_hallu, 6_hours );
            }
            break;
        }

        case ADD_SLEEP:
            // No effects here--just in player::can_sleep()
            // EXCEPT!  Prolong this addiction longer than usual.
            if( one_in( 2 ) && add.sated < 0_turns ) {
                add.sated += 1_turns;
            }
            break;

        case ADD_PKILLER:
            if( calendar::once_every( time_duration::from_turns( 100 - in * 4 ) ) &&
                u.get_painkiller() > 20 - in ) {
                u.mod_painkiller( -1 );    // Tolerance increases!
            }
            if( u.get_painkiller() >= 35 ) { // No further effects if we're doped up.
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
                u.add_msg_if_player( m_bad, _( "Your hands start shaking... you need some painkillers." ) );
                u.add_morale( MORALE_CRAVING_OPIATE, -40, -10 * in );
                u.add_effect( effect_shakes, 2_minutes + in * 30_seconds );
            } else if( one_in( 20 ) && dice( 2, 30 ) < in ) {
                u.add_msg_if_player( m_bad, _( "You feel anxious.  You need your painkillers!" ) );
                u.add_morale( MORALE_CRAVING_OPIATE, -30, -10 * in );
            } else if( one_in( 50 ) && dice( 3, 50 ) < in ) {
                u.vomit();
            }
            break;

        case ADD_SPEED: {
            u.mod_int_bonus( -1 );
            u.mod_str_bonus( -1 );
            if( u.stim > -100 && x_in_y( in, 20 ) ) {
                u.stim--;
            }
            if( rng( 0, 150 ) <= in ) {
                u.mod_healthy_mod( -1, -in );
            }
            if( dice( 2, 100 ) < in ) {
                u.add_msg_if_player( m_warning, _( "You feel depressed.  Speed would help." ) );
                u.add_morale( MORALE_CRAVING_SPEED, -25, -20 * in );
            } else if( one_in( 10 ) && dice( 2, 80 ) < in ) {
                u.add_msg_if_player( m_bad, _( "Your hands start shaking... you need a pick-me-up." ) );
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

        case ADD_COKE:
        case ADD_CRACK: {
            static const std::string coke_msg = _( "You feel like you need a bump." );
            static const std::string crack_msg = _( "You're shivering, you need some crack." );
            const std::string &cur_msg = add.type == ADD_COKE ? coke_msg : crack_msg;
            const auto morale_type = add.type == ADD_COKE ? MORALE_CRAVING_COCAINE : MORALE_CRAVING_CRACK;
            u.mod_int_bonus( -1 );
            u.mod_per_bonus( -1 );
            if( one_in( 900 - 30 * in ) ) {
                u.add_msg_if_player( m_warning, cur_msg );
                u.add_morale( morale_type, -20, -15 * in );
            }
            if( dice( 2, 80 ) <= in ) {
                u.add_msg_if_player( m_warning, cur_msg );
                u.add_morale( morale_type, -20, -15 * in );
                if( u.stim > -150 ) {
                    u.stim -= 3;
                }
            }
            break;
        }

        case ADD_MUTAGEN:
            if( u.has_trait( trait_id( "MUT_JUNKIE" ) ) ) {
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
                                     _( "You could use some new parts..." ) );
                u.add_morale( MORALE_CRAVING_MUTAGEN, -5, -50 );
            }
            break;
        case ADD_MARLOSS_R:
            marloss_add( u, in, _( "You daydream about luscious pink berries as big as your fist." ) );
            break;
        case ADD_MARLOSS_B:
            marloss_add( u, in, _( "You daydream about nutty cyan seeds as big as your hand." ) );
            break;
        case ADD_MARLOSS_Y:
            marloss_add( u, in, _( "You daydream about succulent, pale golden gel, sweet but light." ) );
            break;
        case ADD_NULL:
        case NUM_ADD_TYPES:
            break;
    }
}

/**
 * Returns the name of an addiction. It should be able to finish the sentence
 * "Became addicted to ______".
 */
const std::string &addiction_type_name( add_type const cur )
{
    static const std::map<add_type, std::string> type_map = {{
            { ADD_CIG, _( "nicotine" ) },
            { ADD_CAFFEINE, _( "caffeine" ) },
            { ADD_ALCOHOL, _( "alcohol" ) },
            { ADD_SLEEP, _( "sleeping pills" ) },
            { ADD_PKILLER, _( "opiates" ) },
            { ADD_SPEED, _( "amphetamine" ) },
            { ADD_COKE, _( "cocaine" ) },
            { ADD_CRACK, _( "crack cocaine" ) },
            { ADD_MUTAGEN, _( "mutation" ) },
            { ADD_DIAZEPAM, _( "diazepam" ) },
            { ADD_MARLOSS_R, _( "Marloss berries" ) },
            { ADD_MARLOSS_B, _( "Marloss seeds" ) },
            { ADD_MARLOSS_Y, _( "Marloss gel" ) },
        }
    };

    const auto iter = type_map.find( cur );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    static const std::string error_string( "bugs in addiction.cpp" );
    return error_string;
}

const std::string &addiction_name( const addiction &cur )
{
    static const std::map<add_type, std::string> type_map = {{
            { ADD_CIG, _( "Nicotine Withdrawal" ) },
            { ADD_CAFFEINE, _( "Caffeine Withdrawal" ) },
            { ADD_ALCOHOL, _( "Alcohol Withdrawal" ) },
            { ADD_SLEEP, _( "Sleeping Pill Dependence" ) },
            { ADD_PKILLER, _( "Opiate Withdrawal" ) },
            { ADD_SPEED, _( "Amphetamine Withdrawal" ) },
            { ADD_COKE, _( "Cocaine Withdrawal" ) },
            { ADD_CRACK, _( "Crack Cocaine Withdrawal" ) },
            { ADD_MUTAGEN, _( "Mutation Withdrawal" ) },
            { ADD_DIAZEPAM, _( "Diazepam Withdrawal" ) },
            { ADD_MARLOSS_R, _( "Marloss Longing" ) },
            { ADD_MARLOSS_B, _( "Marloss Desire" ) },
            { ADD_MARLOSS_Y, _( "Marloss Cravings" ) },
        }
    };

    const auto iter = type_map.find( cur.type );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    static const std::string error_string( "Erroneous addiction" );
    return error_string;
}

morale_type addiction_craving( add_type const cur )
{
    static const std::map<add_type, morale_type> type_map = {{
            { ADD_CIG, MORALE_CRAVING_NICOTINE },
            { ADD_CAFFEINE, MORALE_CRAVING_CAFFEINE },
            { ADD_ALCOHOL, MORALE_CRAVING_ALCOHOL },
            { ADD_PKILLER, MORALE_CRAVING_OPIATE },
            { ADD_SPEED, MORALE_CRAVING_SPEED },
            { ADD_COKE, MORALE_CRAVING_COCAINE },
            { ADD_CRACK, MORALE_CRAVING_CRACK },
            { ADD_MUTAGEN, MORALE_CRAVING_MUTAGEN },
            { ADD_DIAZEPAM, MORALE_CRAVING_DIAZEPAM },
            { ADD_MARLOSS_R, MORALE_CRAVING_MARLOSS },
            { ADD_MARLOSS_B, MORALE_CRAVING_MARLOSS },
            { ADD_MARLOSS_Y, MORALE_CRAVING_MARLOSS },
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
            { "nicotine", ADD_CIG },
            { "caffeine", ADD_CAFFEINE },
            { "alcohol", ADD_ALCOHOL },
            { "sleeping pill", ADD_SLEEP },
            { "opiate", ADD_PKILLER },
            { "amphetamine", ADD_SPEED },
            { "cocaine", ADD_COKE },
            { "crack", ADD_CRACK },
            { "mutagen", ADD_MUTAGEN },
            { "diazepam", ADD_DIAZEPAM },
            { "marloss_r", ADD_MARLOSS_R },
            { "marloss_b", ADD_MARLOSS_B },
            { "marloss_y", ADD_MARLOSS_Y },
            { "none", ADD_NULL }
        }
    };

    const auto iter = type_map.find( name );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    return ADD_NULL;
}

const std::string &addiction_text( const addiction &cur )
{
    static const std::map<add_type, std::string> addiction_msg = {{
            { ADD_CIG, _( "Intelligence - 1;   Occasional cravings" ) },
            { ADD_CAFFEINE, _( "Strength - 1;   Slight sluggishness;   Occasional cravings" )  },
            {
                ADD_ALCOHOL, _( "Perception - 1;   Intelligence - 1;   Occasional Cravings;\nRisk of delirium tremens" )
            },
            { ADD_SLEEP, _( "You may find it difficult to sleep without medication." ) },
            {
                ADD_PKILLER, _( "Strength - 1;   Perception - 1;   Dexterity - 1;\nDepression and physical pain to some degree.  Frequent cravings.  Vomiting." )
            },
            { ADD_SPEED, _( "Strength - 1;   Intelligence - 1;\nMovement rate reduction.  Depression.  Weak immune system.  Frequent cravings." ) },
            { ADD_COKE, _( "Perception - 1;   Intelligence - 1;  Frequent cravings." ) },
            { ADD_CRACK, _( "Perception - 2;   Intelligence - 2;  Frequent cravings." ) },
            { ADD_MUTAGEN, _( "You've gotten a taste for mutating and the chemicals that cause it.  But you can stop, yeah, any time you want." ) },
            {
                ADD_DIAZEPAM, _( "Perception - 1;   Intelligence - 1;\nAnxiety, nausea, hallucinations, and general malaise." )
            },
            { ADD_MARLOSS_R, _( "You should try some of those pink berries." ) },
            { ADD_MARLOSS_B, _( "You should try some of those cyan seeds." ) },
            { ADD_MARLOSS_Y, _( "You should try some of that golden gel." ) },
        }
    };

    const auto iter = addiction_msg.find( cur.type );
    if( iter != addiction_msg.end() ) {
        return iter->second;
    }

    static const std::string error_string;
    return error_string;
}

void marloss_add( player &u, int in, const char *msg )
{
    if( one_in( 800 - 20 * in ) ) {
        u.add_morale( MORALE_CRAVING_MARLOSS, -5, -25 );
        u.add_msg_if_player( m_info, msg );
        if( u.focus_pool > 40 ) {
            u.focus_pool -= in;
        }
    }
}
