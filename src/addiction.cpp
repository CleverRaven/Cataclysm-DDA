#include "addiction.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>

#include "calendar.h"
#include "character.h"
#include "debug.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "generic_factory.h"
#include "make_static.h"
#include "morale_types.h"
#include "rng.h"
#include "translations.h"

static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_shakes( "shakes" );

static const trait_id trait_MUT_JUNKIE( "MUT_JUNKIE" );

namespace
{

generic_factory<add_type> add_type_factory( "addiction" );

} // namespace

/** @relates string_id */
template<>
const add_type &string_id<add_type>::obj() const
{
    return add_type_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<add_type>::is_valid() const
{
    return add_type_factory.is_valid( *this );
}

void add_type::load_add_types( const JsonObject &jo, const std::string &src )
{
    add_type_factory.load( jo, src );
}

void add_type::reset()
{
    add_type_factory.reset();
}

const std::vector<add_type> &add_type::get_all()
{
    return add_type_factory.get_all();
}

void add_type::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", _name );
    mandatory( jo, was_loaded, "type_name", _type_name );
    mandatory( jo, was_loaded, "description", _desc );
    optional( jo, was_loaded, "craving_morale", _craving_morale, MORALE_NULL );
    optional( jo, was_loaded, "effect_on_condition", _effect );
}

static void marloss_add( Character &u, int in, const char *msg )
{
    if( one_in( 800 - 20 * in ) ) {
        u.add_morale( MORALE_CRAVING_MARLOSS, -5, -25 );
        u.add_msg_if_player( m_info, msg );
        if( u.get_focus() > 40 ) {
            u.mod_focus( -in );
        }
    }
}

static void alcohol_diazepam_add( Character &u, int in, bool is_alcohol )
{
    const auto morale_type = is_alcohol ? MORALE_CRAVING_ALCOHOL :
                             MORALE_CRAVING_DIAZEPAM;
    u.mod_per_bonus( -1 );
    u.mod_int_bonus( -1 );
    if( x_in_y( in, to_turns<int>( 2_hours ) ) ) {
        u.mod_healthy_mod( -1, -in * 10 );
    }
    if( one_in( 20 ) && rng( 0, 20 ) < in ) {
        const std::string msg_1 = is_alcohol ?
                                  _( "You could use a drink." ) :
                                  _( "You could use some diazepam." );
        u.add_msg_if_player( m_warning, msg_1 );
        u.add_morale( morale_type, -35, -10 * in );
    } else if( rng( 8, 300 ) < in ) {
        const std::string msg_2 = is_alcohol ?
                                  _( "Your hands start shaking… you need a drink bad!" ) :
                                  _( "You're shaking… you need some diazepam!" );
        u.add_msg_if_player( m_bad, msg_2 );
        u.add_morale( morale_type, -35, -10 * in );
        u.add_effect( effect_shakes, 5_minutes );
    } else if( !u.has_effect( effect_hallu ) && rng( 10, 1600 ) < in ) {
        u.add_effect( effect_hallu, 6_hours );
    }
}

static void crack_coke_add( Character &u, int in, int stim, bool is_crack )
{
    const std::string &cur_msg = !is_crack ?
                                 _( "You feel like you need a bump." ) :
                                 _( "You're shivering, you need some crack." );
    const auto morale_type = !is_crack ? MORALE_CRAVING_COCAINE :
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
        if( stim > -150 ) {
            u.mod_stim( -3 );
        }
    }
}

void addict_effect( Character &u, addiction &add )
{
    if( add.type->get_effect().is_valid() ) {
        dialogue d( get_talker_for( u ), nullptr );
        add.type->get_effect()->activate( d );
        return;
    }

    const int in = std::min( 20, add.intensity );
    const int current_stim = u.get_stim();

    if( add.type == STATIC( addiction_id( "nicotine" ) ) ) {
        if( one_in( 2000 - 20 * in ) ) {
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
        }
    } else if( add.type == STATIC( addiction_id( "caffeine" ) ) ) {
        if( one_in( 2000 - 20 * in ) ) {
            u.add_msg_if_player( m_warning, _( "You want some caffeine." ) );
            u.add_morale( MORALE_CRAVING_CAFFEINE, -5, -30 );
            if( current_stim > -10 * in && rng( 0, 10 ) < in ) {
                u.mod_stim( -1 );
            }
            if( rng( 8, 400 ) < in ) {
                u.add_msg_if_player( m_bad, _( "Your hands start shaking… you need it bad!" ) );
                u.add_effect( effect_shakes, 2_minutes );
            }
        }
    } else if( add.type == STATIC( addiction_id( "alcohol" ) ) ) {
        alcohol_diazepam_add( u, in, true );
    } else if( add.type == STATIC( addiction_id( "diazepam" ) ) ) {
        alcohol_diazepam_add( u, in, false );
    } else if( add.type == STATIC( addiction_id( "sleeping pill" ) ) ) {
        // No effects here--just in player::can_sleep()
        // EXCEPT!  Prolong this addiction longer than usual.
        if( one_in( 2 ) && add.sated < 0_turns ) {
            add.sated += 1_turns;
        }
    } else if( add.type == STATIC( addiction_id( "opiate" ) ) ) {
        if( calendar::once_every( time_duration::from_turns( 100 - in * 4 ) ) &&
            u.get_painkiller() > 20 - in ) {
            // Tolerance increases!
            u.mod_painkiller( -1 );
        }
        // No further effects if we're doped up.
        if( u.get_painkiller() >= 35 ) {
            add.sated = 0_turns;
        } else {
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
        }
    } else if( add.type == STATIC( addiction_id( "amphetamine" ) ) ) {
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
    } else if( add.type == STATIC( addiction_id( "cocaine" ) ) ) {
        crack_coke_add( u, in, current_stim, false );
    } else if( add.type == STATIC( addiction_id( "crack" ) ) ) {
        crack_coke_add( u, in, current_stim, true );
    } else if( add.type == STATIC( addiction_id( "mutagen" ) ) ) {
        if( u.has_trait( trait_MUT_JUNKIE ) ) {
            if( one_in( 600 - 50 * in ) ) {
                u.add_msg_if_player( m_warning, rng( 0, 6 ) < in ?
                                     _( "You so miss the exquisite rainbow of post-humanity." ) :
                                     _( "Your body is SOO booorrrring.  Just a little sip to liven things up?" ) );
                u.add_morale( MORALE_CRAVING_MUTAGEN, -20, -200 );
            }
            if( u.get_focus() > 40 && one_in( 800 - 20 * in ) ) {
                u.mod_focus( -in );
                u.add_msg_if_player( m_warning,
                                     _( "You daydream what it'd be like if you were *different*.  Different is good." ) );
            }
        } else if( one_in( 500 - 15 * in ) ) {
            u.add_msg_if_player( m_warning, rng( 0, 6 ) < in ? _( "You haven't had any mutagen lately." ) :
                                 _( "You could use some new parts…" ) );
            u.add_morale( MORALE_CRAVING_MUTAGEN, -5, -50 );
        }
    } else if( add.type == STATIC( addiction_id( "marloss_r" ) ) ) {
        marloss_add( u, in, _( "You daydream about luscious pink berries as big as your fist." ) );
    } else if( add.type == STATIC( addiction_id( "marloss_b" ) ) ) {
        marloss_add( u, in, _( "You daydream about nutty cyan seeds as big as your hand." ) );
    } else if( add.type == STATIC( addiction_id( "marloss_y" ) ) ) {
        marloss_add( u, in, _( "You daydream about succulent, pale golden gel, sweet but light." ) );
    }
}
