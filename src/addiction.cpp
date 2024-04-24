#include "addiction.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "calendar.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "init.h"
#include "json_error.h"
#include "morale_types.h"
#include "rng.h"
#include "talker.h"
#include "text_snippets.h"

static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_shakes( "shakes" );

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

static bool alcohol_diazepam_add( Character &u, int in, bool is_alcohol )
{
    static time_point last_alc_dream = calendar::turn_zero;
    static time_point last_dia_dream = calendar::turn_zero;
    const bool recent_dream = ( is_alcohol && ( calendar::turn - last_alc_dream < 2_hours ) ) ||
                              ( !is_alcohol && ( calendar::turn - last_dia_dream < 2_hours ) );
    const morale_type morale_type = is_alcohol ? MORALE_CRAVING_ALCOHOL :
                                    MORALE_CRAVING_DIAZEPAM;
    bool ret = false;
    u.mod_per_bonus( -1 );
    u.mod_int_bonus( -1 );
    if( x_in_y( in, to_turns<int>( 2_hours ) ) ) {
        u.mod_daily_health( -1, -in * 10 );
        ret = true;
    }
    if( one_in( 20 ) && rng( 0, 20 ) < in && ( !u.in_sleep_state() || !recent_dream ) ) {
        const std::string msg_1 =
            is_alcohol ?
            ( u.in_sleep_state() ? "addict_alcohol_mild_asleep" : "addict_alcohol_mild_awake" ) :
            ( u.in_sleep_state() ? "addict_diazepam_mild_asleep" : "addict_diazepam_mild_awake" );
        u.add_msg_if_player( m_warning,
                             SNIPPET.random_from_category( msg_1 ).value_or( translation() ).translated() );
        u.add_morale( morale_type, -35, -10 * in );
        ret = true;
        if( u.in_sleep_state() ) {
            if( is_alcohol ) {
                last_alc_dream = calendar::turn;
            } else {
                last_dia_dream = calendar::turn;
            }
        }
    } else if( rng( 8, 300 ) < in && ( !u.in_sleep_state() || !recent_dream ) ) {
        const std::string msg_2 =
            is_alcohol ?
            ( u.in_sleep_state() ? "addict_alcohol_strong_asleep" : "addict_alcohol_strong_awake" ) :
            ( u.in_sleep_state() ? "addict_diazepam_strong_asleep" : "addict_diazepam_strong_awake" );
        u.add_msg_if_player( m_bad,
                             SNIPPET.random_from_category( msg_2 ).value_or( translation() ).translated() );
        u.add_morale( morale_type, -35, -10 * in );
        u.add_effect( effect_shakes, 5_minutes );
        ret = true;
        if( u.in_sleep_state() ) {
            if( is_alcohol ) {
                last_alc_dream = calendar::turn;
            } else {
                last_dia_dream = calendar::turn;
            }
        }
    } else if( !u.has_effect( effect_hallu ) && rng( 10, 1600 ) < in ) {
        u.add_effect( effect_hallu, 6_hours );
        ret = true;
    }
    return ret;
}

static bool crack_coke_add( Character &u, int in, int stim, bool is_crack )
{
    static time_point last_coke_dream = calendar::turn_zero;
    static time_point last_crack_dream = calendar::turn_zero;
    const bool recent_dream = ( is_crack && ( calendar::turn - last_crack_dream < 2_hours ) ) ||
                              ( !is_crack && ( calendar::turn - last_coke_dream < 2_hours ) );
    const std::string cur_msg =
        !is_crack ?
        ( u.in_sleep_state() ? "addict_coke_asleep" : "addict_coke_awake" ) :
        ( u.in_sleep_state() ? "addict_crack_asleep" : "addict_crack_awake" );
    const morale_type morale_type = !is_crack ? MORALE_CRAVING_COCAINE :
                                    MORALE_CRAVING_CRACK;
    bool ret = false;
    auto run_addict_eff = [&ret, &morale_type]( Character & u, int in, bool is_crack,
    const std::string & snippet ) {
        u.add_msg_if_player( m_warning,
                             SNIPPET.random_from_category( snippet ).value_or( translation() ).translated() );
        u.add_morale( morale_type, -20, -15 * in );
        ret = true;
        if( u.in_sleep_state() ) {
            if( is_crack ) {
                last_crack_dream = calendar::turn;
            } else {
                last_coke_dream = calendar::turn;
            }
        }
    };

    u.mod_int_bonus( -1 );
    u.mod_per_bonus( -1 );
    if( one_in( 900 - 30 * in ) && ( !u.in_sleep_state() || !recent_dream ) ) {
        run_addict_eff( u, in, is_crack, cur_msg );
    }
    if( dice( 2, 80 ) <= in && ( !u.in_sleep_state() || !recent_dream ) ) {
        run_addict_eff( u, in, is_crack, cur_msg );
        if( stim > -150 ) {
            u.mod_stim( -3 );
        }
    }
    return ret;
}

/************** Builtin effects **************/

static bool nicotine_effect( Character &u, addiction &add )
{
    static time_point last_dream = calendar::turn_zero;
    const int in = std::min( 20, add.intensity );
    const int current_stim = u.get_stim();
    if( one_in( 2000 - 20 * in ) && ( !u.in_sleep_state() || calendar::turn - last_dream > 2_hours ) ) {
        if( u.in_sleep_state() ) {
            last_dream = calendar::turn;
        }
        const bool strong = rng( 0, 6 ) < in;
        const std::string msg =
            !strong ?
            ( u.in_sleep_state() ? "addict_nicotine_mild_asleep" : "addict_nicotine_mild_awake" ) :
            ( u.in_sleep_state() ? "addict_nicotine_strong_asleep" : "addict_nicotine_strong_awake" );
        u.add_msg_if_player( m_warning,
                             SNIPPET.random_from_category( msg ).value_or( translation() ).translated() );
        u.add_morale( MORALE_CRAVING_NICOTINE, -15, -3 * in );
        if( one_in( 800 - 50 * in ) ) {
            u.mod_sleepiness( 1 );
        }
        if( current_stim > -5 * in && one_in( 400 - 20 * in ) ) {
            u.mod_stim( -1 );
        }
        return true;
    }
    return false;
}

static bool alcohol_effect( Character &u, addiction &add )
{
    const int in = std::min( 20, add.intensity );
    return alcohol_diazepam_add( u, in, true );
}

static bool diazepam_effect( Character &u, addiction &add )
{
    const int in = std::min( 20, add.intensity );
    return alcohol_diazepam_add( u, in, false );
}

static bool opiate_effect( Character &u, addiction &add )
{
    static time_point last_dream = calendar::turn_zero;
    const int in = std::min( 20, add.intensity );
    if( calendar::once_every( time_duration::from_turns( 100 - in * 4 ) ) &&
        u.get_painkiller() > 20 - in ) {
        // Tolerance increases!
        u.mod_painkiller( -1 );
    }
    // No further effects if we're doped up.
    if( u.get_painkiller() >= 35 ) {
        add.sated = 0_turns;
        return false;
    }
    u.mod_str_bonus( -1 );
    u.mod_per_bonus( -1 );
    u.mod_dex_bonus( -1 );
    if( u.get_pain() < in * 2 ) {
        u.mod_pain( 1 );
    }
    if( one_in( 1200 - 30 * in ) ) {
        u.mod_daily_health( -1, -in * 30 );
    }
    if( one_in( 20 ) && dice( 2, 20 ) < in &&
        ( !u.in_sleep_state() || calendar::turn - last_dream > 2_hours ) ) {
        if( u.in_sleep_state() ) {
            last_dream = calendar::turn;
        }
        const std::string msg =
            u.in_sleep_state() ? "addict_opiate_mild_asleep" : "addict_opiate_mild_awake";
        u.add_msg_if_player( m_bad,
                             SNIPPET.random_from_category( msg ).value_or( translation() ).translated() );
        u.add_morale( MORALE_CRAVING_OPIATE, -40, -10 * in );
        u.add_effect( effect_shakes, 2_minutes + in * 30_seconds );
    } else if( one_in( 20 ) && dice( 2, 30 ) < in &&
               ( !u.in_sleep_state() || calendar::turn - last_dream > 2_hours ) ) {
        if( u.in_sleep_state() ) {
            last_dream = calendar::turn;
        }
        const std::string msg =
            u.in_sleep_state() ? "addict_opiate_strong_asleep" : "addict_opiate_strong_awake";
        u.add_msg_if_player( m_bad,
                             SNIPPET.random_from_category( msg ).value_or( translation() ).translated() );
        u.add_morale( MORALE_CRAVING_OPIATE, -30, -10 * in );
    } else if( one_in( 50 ) && dice( 3, 50 ) < in ) {
        u.vomit();
    }
    return true;
}

static bool amphetamine_effect( Character &u, addiction &add )
{
    static time_point last_dream = calendar::turn_zero;
    const int in = std::min( 20, add.intensity );
    const int current_stim = u.get_stim();
    bool ret = false;
    u.mod_int_bonus( -1 );
    u.mod_str_bonus( -1 );
    if( current_stim > -100 && x_in_y( in, 20 ) ) {
        u.mod_stim( -1 );
        ret = true;
    }
    if( rng( 0, 150 ) <= in ) {
        u.mod_daily_health( -1, -in );
        ret = true;
    }
    if( dice( 2, 100 ) < in && ( !u.in_sleep_state() || calendar::turn - last_dream > 2_hours ) ) {
        if( u.in_sleep_state() ) {
            last_dream = calendar::turn;
        }
        const std::string msg =
            u.in_sleep_state() ? "addict_amphetamine_mild_asleep" : "addict_amphetamine_mild_awake";
        u.add_msg_if_player( m_warning,
                             SNIPPET.random_from_category( msg ).value_or( translation() ).translated() );
        u.add_morale( MORALE_CRAVING_SPEED, -25, -20 * in );
        ret = true;
    } else if( one_in( 10 ) && dice( 2, 80 ) < in &&
               ( !u.in_sleep_state() || calendar::turn - last_dream > 2_hours ) ) {
        if( u.in_sleep_state() ) {
            last_dream = calendar::turn;
        }
        const std::string msg =
            u.in_sleep_state() ? "addict_amphetamine_strong_asleep" : "addict_amphetamine_strong_awake";
        u.add_msg_if_player( m_warning,
                             SNIPPET.random_from_category( msg ).value_or( translation() ).translated() );
        u.add_morale( MORALE_CRAVING_SPEED, -25, -20 * in );
        u.add_effect( effect_shakes, in * 2_minutes );
        ret = true;
    } else if( one_in( 50 ) && dice( 2, 100 ) < in &&
               ( !u.in_sleep_state() || calendar::turn - last_dream > 2_hours ) ) {
        if( u.in_sleep_state() ) {
            last_dream = calendar::turn;
        }
        const std::string msg =
            u.in_sleep_state() ? "addict_amphetamine_paralysis_asleep" : "addict_amphetamine_paralysis_awake";
        u.add_msg_if_player( m_warning,
                             SNIPPET.random_from_category( msg ).value_or( translation() ).translated() );
        u.mod_moves( -( u.in_sleep_state() ? 6000 : 300 ) );
        u.wake_up();
        ret = true;
    } else if( !u.has_effect( effect_hallu ) && one_in( 20 ) && 8 + dice( 2, 80 ) < in ) {
        u.add_effect( effect_hallu, 6_hours );
        ret = true;
    }
    return ret;
}

static bool cocaine_effect( Character &u, addiction &add )
{
    const int in = std::min( 20, add.intensity );
    const int current_stim = u.get_stim();
    return crack_coke_add( u, in, current_stim, false );
}

static bool crack_effect( Character &u, addiction &add )
{
    const int in = std::min( 20, add.intensity );
    const int current_stim = u.get_stim();
    return crack_coke_add( u, in, current_stim, true );
}

/*********************************************/

static const std::map<std::string, std::function<bool( Character &, addiction & )>> builtin_map {
    {"nicotine_effect",    ::nicotine_effect},
    {"alcohol_effect",     ::alcohol_effect},
    {"diazepam_effect",    ::diazepam_effect},
    {"opiate_effect",      ::opiate_effect},
    {"amphetamine_effect", ::amphetamine_effect},
    {"cocaine_effect",     ::cocaine_effect},
    {"crack_effect",       ::crack_effect}
};

bool addiction::run_effect( Character &u )
{
    bool ret = false;
    if( !type->get_effect().is_null() ) {
        dialogue d( get_talker_for( u ), nullptr );
        ret = type->get_effect()->activate( d );
    } else {
        auto iter = builtin_map.find( type->get_builtin() );
        if( iter != builtin_map.end() ) {
            ret = iter->second.operator()( u, *this );
        } else {
            debugmsg( "invalid builtin \"%s\" for addiction_type \"%s\"", type->get_builtin(), type.c_str() );
        }
    }
    return ret;
}

void add_type::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "name", _name );
    mandatory( jo, was_loaded, "type_name", _type_name );
    mandatory( jo, was_loaded, "description", _desc );
    optional( jo, was_loaded, "craving_morale", _craving_morale, MORALE_NULL );
    optional( jo, was_loaded, "effect_on_condition", _effect );
    optional( jo, was_loaded, "builtin", _builtin );
}

void add_type::check_add_types()
{
    for( const add_type &add : add_type::get_all() ) {
        if( add._effect.is_null() == add._builtin.empty() ) {
            debugmsg( "addiction_type \"%s\" defines %s effect_on_condition %s builtin.  Addictions must define either field, but not both.",
                      add.id.c_str(), add._builtin.empty() ? "neither" : "both", add._builtin.empty() ? "or" : "and" );
        }
        if( !add._builtin.empty() && builtin_map.find( add._builtin ) == builtin_map.end() ) {
            debugmsg( "invalid builtin \"%s\" for addiction_type \"%s\"", add._builtin, add.id.c_str() );
        }
    }
}

std::string add_type_legacy_conv( std::string const &v )
{
    if( v == "CAFFEINE" ) {
        return "caffeine";
    } else if( v == "ALCOHOL" ) {
        return "alcohol";
    } else if( v == "SLEEP" ) {
        return "sleeping pill";
    } else if( v == "PKILLER" ) {
        return "opiate";
    } else if( v == "SPEED" ) {
        return "amphetamine";
    } else if( v == "CIG" ) {
        return "nicotine";
    } else if( v == "COKE" ) {
        return "cocaine";
    } else if( v == "CRACK" ) {
        return "crack";
    } else if( v == "MUTAGEN" ) {
        return "mutagen";
    } else if( v == "DIAZEPAM" ) {
        return "diazepam";
    } else if( v == "MARLOSS_R" ) {
        return "marloss_r";
    } else if( v == "MARLOSS_B" ) {
        return "marloss_b";
    } else if( v == "MARLOSS_Y" ) {
        return "marloss_y";
    }
    return {};
}
