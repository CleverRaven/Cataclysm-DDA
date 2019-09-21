#include "player.h" // IWYU pragma: associated

#include <cmath>
#include <cstdlib>

#include "avatar.h"
#include "effect.h"
#include "event_bus.h"
#include "fungal_effects.h"
#include "field_type.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "sounds.h"
#include "weather.h"
#include "rng.h"
#include "translations.h"
#include "units.h"
#include "enums.h"
#include "mtype.h"
#include "stomach.h"
#include "teleport.h"

#if defined(TILES)
#   if defined(_MSC_VER) && defined(USE_VCPKG)
#       include <SDL2/SDL.h>
#   else
#       include <SDL.h>
#   endif
#endif // TILES

#include <functional>
#include <algorithm>

const mtype_id mon_dermatik_larva( "mon_dermatik_larva" );

const efftype_id effect_adrenaline( "adrenaline" );
const efftype_id effect_alarm_clock( "alarm_clock" );
const efftype_id effect_antibiotic( "antibiotic" );
const efftype_id effect_asthma( "asthma" );
const efftype_id effect_attention( "attention" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_bloodworms( "bloodworms" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_brainworms( "brainworms" );
const efftype_id effect_cold( "cold" );
const efftype_id effect_cough_suppress( "cough_suppress" );
const efftype_id effect_datura( "datura" );
const efftype_id effect_dermatik( "dermatik" );
const efftype_id effect_disabled( "disabled" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_evil( "evil" );
const efftype_id effect_formication( "formication" );
const efftype_id effect_frostbite( "frostbite" );
const efftype_id effect_fungus( "fungus" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_grabbing( "grabbing" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_hot( "hot" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_mending( "mending" );
const efftype_id effect_meth( "meth" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_paincysts( "paincysts" );
const efftype_id effect_panacea( "panacea" );
const efftype_id effect_rat( "rat" );
const efftype_id effect_recover( "recover" );
const efftype_id effect_shakes( "shakes" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
const efftype_id effect_spores( "spores" );
const efftype_id effect_strong_antibiotic( "strong_antibiotic" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_tapeworm( "tapeworm" );
const efftype_id effect_teleglow( "teleglow" );
const efftype_id effect_tetanus( "tetanus" );
const efftype_id effect_valium( "valium" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_weak_antibiotic( "weak_antibiotic" );

const vitamin_id vitamin_iron( "iron" );

static void eff_fun_onfire( player &u, effect &it )
{
    const int intense = it.get_intensity();
    u.deal_damage( nullptr, it.get_bp(), damage_instance( DT_HEAT, rng( intense, intense * 2 ) ) );
}
static void eff_fun_spores( player &u, effect &it )
{
    // Equivalent to X in 150000 + health * 100
    const int intense = it.get_intensity();
    if( ( !u.has_trait( trait_id( "M_IMMUNE" ) ) ) && ( one_in( 100 ) &&
            x_in_y( intense, 900 + u.get_healthy() * 0.6 ) ) ) {
        u.add_effect( effect_fungus, 1_turns, num_bp, true );
    }
}
static void eff_fun_fungus( player &u, effect &it )
{
    const time_duration dur = it.get_duration();
    const int intense = it.get_intensity();
    const int bonus = u.get_healthy() / 10 + ( u.resists_effect( it ) ? 100 : 0 );
    switch( intense ) {
        case 1: // First hour symptoms
            if( one_in( 960 + bonus * 6 ) ) {
                u.cough( true );
            }
            if( one_in( 600 + bonus * 6 ) ) {
                u.add_msg_if_player( m_warning, _( "You feel nauseous." ) );
            }
            if( one_in( 600 + bonus * 6 ) ) {
                u.add_msg_if_player( m_warning, _( "You smell and taste mushrooms." ) );
            }
            it.mod_duration( 1_turns );
            if( dur > 1_hours ) {
                it.mod_intensity( 1 );
            }
            break;
        case 2: // Five hours of worse symptoms
            if( one_in( 3600 + bonus * 18 ) ) {
                u.add_msg_if_player( m_bad,  _( "You spasm suddenly!" ) );
                u.moves -= 100;
                u.apply_damage( nullptr, bp_torso, 5 );
            }
            if( x_in_y( u.vomit_mod(), ( 4800 + bonus * 24 ) ) || one_in( 12000 + bonus * 60 ) ) {
                u.add_msg_player_or_npc( m_bad, _( "You vomit a thick, gray goop." ),
                                         _( "<npcname> vomits a thick, gray goop." ) );

                const int awfulness = rng( 0, 70 );
                u.moves = -200;
                u.mod_hunger( awfulness );
                u.mod_thirst( awfulness );
                ///\EFFECT_STR decreases damage taken by fungus effect
                u.apply_damage( nullptr, bp_torso, awfulness / std::max( u.str_cur, 1 ) ); // can't be healthy
            }
            it.mod_duration( 1_turns );
            if( dur > 6_hours ) {
                it.mod_intensity( 1 );
            }
            break;
        case 3: // Permanent symptoms
            if( one_in( 6000 + bonus * 48 ) ) {
                u.add_msg_player_or_npc( m_bad,  _( "You vomit thousands of live spores!" ),
                                         _( "<npcname> vomits thousands of live spores!" ) );

                u.moves = -500;
                fungal_effects fe( *g, g->m );
                for( const tripoint &sporep : g->m.points_in_radius( u.pos(), 1 ) ) {
                    if( sporep == u.pos() ) {
                        continue;
                    }
                    fe.fungalize( sporep, &u, 0.25 );
                }
                // We're fucked
            } else if( one_in( 36000 + bonus * 120 ) ) {
                if( u.hp_cur[hp_arm_l] <= 0 || u.hp_cur[hp_arm_r] <= 0 ) {
                    if( u.hp_cur[hp_arm_l] <= 0 && u.hp_cur[hp_arm_r] <= 0 ) {
                        u.add_msg_player_or_npc( m_bad,
                                                 _( "The flesh on your broken arms bulges. Fungus stalks burst through!" ),
                                                 _( "<npcname>'s broken arms bulge. Fungus stalks burst out of the bulges!" ) );
                    } else {
                        u.add_msg_player_or_npc( m_bad,
                                                 _( "The flesh on your broken and unbroken arms bulge. Fungus stalks burst through!" ),
                                                 _( "<npcname>'s arms bulge. Fungus stalks burst out of the bulges!" ) );
                    }
                } else {
                    u.add_msg_player_or_npc( m_bad, _( "Your hands bulge. Fungus stalks burst through the bulge!" ),
                                             _( "<npcname>'s hands bulge. Fungus stalks burst through the bulge!" ) );
                }
                u.apply_damage( nullptr, bp_arm_l, 999 );
                u.apply_damage( nullptr, bp_arm_r, 999 );
            }
            break;
    }
}
static void eff_fun_rat( player &u, effect &it )
{
    const int dur = to_turns<int>( it.get_duration() );
    it.set_intensity( dur / 10 );
    if( rng( 0, 100 ) < dur / 10 ) {
        if( !one_in( 5 ) ) {
            u.mutate_category( "RAT" );
            it.mult_duration( .2 );
        } else {
            u.mutate_category( "TROGLOBITE" );
            it.mult_duration( .33 );
        }
    } else if( rng( 0, 100 ) < dur / 8 ) {
        if( one_in( 3 ) ) {
            u.vomit();
            it.mod_duration( -1_minutes );
        } else {
            u.add_msg_if_player( m_bad, _( "You feel nauseous!" ) );
            it.mod_duration( 3_turns );
        }
    }
}
static void eff_fun_bleed( player &u, effect &it )
{
    // Presuming that during the first-aid process you're putting pressure
    // on the wound or otherwise suppressing the flow. (Kits contain either
    // QuikClot or bandages per the recipe.)
    const int intense = it.get_intensity();
    if( one_in( 36 / intense ) && u.activity.id() != activity_id( "ACT_FIRSTAID" ) ) {
        u.add_msg_player_or_npc( m_bad, _( "You lose some blood." ),
                                 _( "<npcname> loses some blood." ) );
        // Prolonged hemorrhage is a significant risk for developing anemia
        u.vitamin_mod( vitamin_iron, rng( -1, -4 ) );
        u.mod_pain( 1 );
        u.apply_damage( nullptr, it.get_bp(), 1 );
        u.bleed();
    }
}
static void eff_fun_hallu( player &u, effect &it )
{
    // TODO: Redo this to allow for variable durations
    // Time intervals are drawn from the old ones based on 3600 (6-hour) duration.
    constexpr int maxDuration = 21600;
    constexpr int comeupTime = static_cast<int>( maxDuration * 0.9 );
    constexpr int noticeTime = static_cast<int>( comeupTime + ( maxDuration - comeupTime ) / 2 );
    constexpr int peakTime = static_cast<int>( maxDuration * 0.8 );
    constexpr int comedownTime = static_cast<int>( maxDuration * 0.3 );
    const int dur = to_turns<int>( it.get_duration() );
    // Baseline
    if( dur == noticeTime ) {
        u.add_msg_if_player( m_warning, _( "You feel a little strange." ) );
    } else if( dur == comeupTime ) {
        // Coming up
        if( one_in( 2 ) ) {
            u.add_msg_if_player( m_warning, _( "The world takes on a dreamlike quality." ) );
        } else if( one_in( 3 ) ) {
            u.add_msg_if_player( m_warning, _( "You have a sudden nostalgic feeling." ) );
        } else if( one_in( 5 ) ) {
            u.add_msg_if_player( m_warning, _( "Everything around you is starting to breathe." ) );
        } else {
            u.add_msg_if_player( m_warning, _( "Something feels very, very wrong." ) );
        }
    } else if( dur > peakTime && dur < comeupTime ) {
        if( u.stomach.contains() > 0_ml && ( one_in( 1200 ) || x_in_y( u.vomit_mod(), 300 ) ) ) {
            u.add_msg_if_player( m_bad, _( "You feel sick to your stomach." ) );
            u.mod_hunger( -2 );
            if( one_in( 6 ) ) {
                u.vomit();
            }
        }
        if( u.is_npc() && one_in( 1200 ) ) {
            static const std::array<std::string, 4> npc_hallu = {{
                    translate_marker( "\"I think it's starting to kick in.\"" ),
                    translate_marker( "\"Oh God, what's happening?\"" ),
                    translate_marker( "\"Of course... it's all fractals!\"" ),
                    translate_marker( "\"Huh?  What was that?\"" )
                }
            };

            ///\EFFECT_STR_NPC increases volume of hallucination sounds (NEGATIVE)

            ///\EFFECT_INT_NPC decreases volume of hallucination sounds
            int loudness = 20 + u.str_cur - u.int_cur;
            loudness = ( loudness > 5 ? loudness : 5 );
            loudness = ( loudness < 30 ? loudness : 30 );
            sounds::sound( u.pos(), loudness, sounds::sound_t::speech, _( random_entry_ref( npc_hallu ) ),
                           false, "speech",
                           loudness < 15 ? ( u.male ? "NPC_m" : "NPC_f" ) : ( u.male ? "NPC_m_loud" : "NPC_f_loud" ) );
        }
    } else if( dur == peakTime ) {
        // Visuals start
        u.add_msg_if_player( m_bad, _( "Fractal patterns dance across your vision." ) );
        u.add_effect( effect_visuals, time_duration::from_turns( peakTime - comedownTime ) );
    } else if( dur > comedownTime && dur < peakTime ) {
        // Full symptoms
        u.mod_per_bonus( -2 );
        u.mod_int_bonus( -1 );
        u.mod_dex_bonus( -2 );
        u.add_miss_reason( _( "Dancing fractals distract you." ), 2 );
        u.mod_str_bonus( -1 );
        if( u.is_player() && one_in( 50 ) ) {
            g->spawn_hallucination( u.pos() + tripoint( rng( -10, 10 ), rng( -10, 10 ), 0 ) );
        }
    } else if( dur == comedownTime ) {
        if( one_in( 42 ) ) {
            u.add_msg_if_player( _( "Everything looks SO boring now." ) );
        } else {
            u.add_msg_if_player( _( "Things are returning to normal." ) );
        }
    }
}

struct temperature_effect {
    int str_pen;
    int dex_pen;
    int int_pen;
    int per_pen;
    // Not translated (static string should not be translated)
    std::string msg;
    int msg_chance;
    // Note: NOT std::string because the pointer is stored so c_str() is not OK
    // Also not translated
    const char *miss_msg;

    temperature_effect( int sp, int dp, int ip, int pp, const std::string &ms, int mc,
                        const char *mm ) :
        str_pen( sp ), dex_pen( dp ), int_pen( ip ), per_pen( pp ), msg( ms ),
        msg_chance( mc ), miss_msg( mm ) {
    }

    void apply( player &u ) const {
        if( str_pen > 0 ) {
            u.mod_str_bonus( -str_pen );
        }
        if( dex_pen > 0 ) {
            u.mod_dex_bonus( -dex_pen );
            u.add_miss_reason( _( miss_msg ), dex_pen );
        }
        if( int_pen > 0 ) {
            u.mod_int_bonus( -int_pen );
        }
        if( per_pen > 0 ) {
            u.mod_per_bonus( -per_pen );
        }
        if( !msg.empty() && !u.has_effect( effect_sleep ) && one_in( msg_chance ) ) {
            u.add_msg_if_player( m_warning, "%s", _( msg ) );
        }
    }
};

static void eff_fun_cold( player &u, effect &it )
{
    // { body_part, intensity }, { str_pen, dex_pen, int_pen, per_pen, msg, msg_chance, miss_msg }
    static const std::map<std::pair<body_part, int>, temperature_effect> effs = {{
            { { bp_head, 3 }, { 0, 0, 3, 0, translate_marker( "Your thoughts are unclear." ), 2400, "" } },
            { { bp_head, 2 }, { 0, 0, 1, 0, "", 0, "" } },
            { { bp_mouth, 3 }, { 0, 0, 0, 3, translate_marker( "Your face is stiff from the cold." ), 2400, "" } },
            { { bp_mouth, 2 }, { 0, 0, 0, 1, "", 0, "" } },
            { { bp_torso, 3 }, { 0, 4, 0, 0, translate_marker( "Your torso is freezing cold. You should put on a few more layers." ), 400, translate_marker( "You quiver from the cold." ) } },
            { { bp_torso, 2 }, { 0, 2, 0, 0, "", 0, translate_marker( "Your shivering makes you unsteady." ) } },
            { { bp_arm_l, 3 }, { 0, 2, 0, 0, translate_marker( "Your left arm is shivering." ), 4800, translate_marker( "Your left arm trembles from the cold." ) } },
            { { bp_arm_l, 2 }, { 0, 1, 0, 0, translate_marker( "Your left arm is shivering." ), 4800, translate_marker( "Your left arm trembles from the cold." ) } },
            { { bp_arm_r, 3 }, { 0, 2, 0, 0, translate_marker( "Your right arm is shivering." ), 4800, translate_marker( "Your right arm trembles from the cold." ) } },
            { { bp_arm_r, 2 }, { 0, 1, 0, 0, translate_marker( "Your right arm is shivering." ), 4800, translate_marker( "Your right arm trembles from the cold." ) } },
            { { bp_hand_l, 3 }, { 0, 2, 0, 0, translate_marker( "Your left hand feels like ice." ), 4800, translate_marker( "Your left hand quivers in the cold." ) } },
            { { bp_hand_l, 2 }, { 0, 1, 0, 0, translate_marker( "Your left hand feels like ice." ), 4800, translate_marker( "Your left hand quivers in the cold." ) } },
            { { bp_hand_r, 3 }, { 0, 2, 0, 0, translate_marker( "Your right hand feels like ice." ), 4800, translate_marker( "Your right hand quivers in the cold." ) } },
            { { bp_hand_r, 2 }, { 0, 1, 0, 0, translate_marker( "Your right hand feels like ice." ), 4800, translate_marker( "Your right hand quivers in the cold." ) } },
            { { bp_leg_l, 3 }, { 2, 2, 0, 0, translate_marker( "Your left leg trembles against the relentless cold." ), 4800, translate_marker( "Your legs uncontrollably shake from the cold." ) } },
            { { bp_leg_l, 2 }, { 1, 1, 0, 0, translate_marker( "Your left leg trembles against the relentless cold." ), 4800, translate_marker( "Your legs uncontrollably shake from the cold." ) } },
            { { bp_leg_r, 3 }, { 2, 2, 0, 0, translate_marker( "Your right leg trembles against the relentless cold." ), 4800, translate_marker( "Your legs uncontrollably shake from the cold." ) } },
            { { bp_leg_r, 2 }, { 1, 1, 0, 0, translate_marker( "Your right leg trembles against the relentless cold." ), 4800, translate_marker( "Your legs uncontrollably shake from the cold." ) } },
            { { bp_foot_l, 3 }, { 2, 2, 0, 0, translate_marker( "Your left foot feels frigid." ), 4800, translate_marker( "Your left foot is as nimble as a block of ice." ) } },
            { { bp_foot_l, 2 }, { 1, 1, 0, 0, translate_marker( "Your left foot feels frigid." ), 4800, translate_marker( "Your freezing left foot messes up your balance." ) } },
            { { bp_foot_r, 3 }, { 2, 2, 0, 0, translate_marker( "Your right foot feels frigid." ), 4800, translate_marker( "Your right foot is as nimble as a block of ice." ) } },
            { { bp_foot_r, 2 }, { 1, 1, 0, 0, translate_marker( "Your right foot feels frigid." ), 4800, translate_marker( "Your freezing right foot messes up your balance." ) } },
        }
    };
    const auto iter = effs.find( { it.get_bp(), it.get_intensity() } );
    if( iter != effs.end() ) {
        iter->second.apply( u );
    }
}

static void eff_fun_hot( player &u, effect &it )
{
    // { body_part, intensity }, { str_pen, dex_pen, int_pen, per_pen, msg, msg_chance, miss_msg }
    static const std::map<std::pair<body_part, int>, temperature_effect> effs = {{
            { { bp_head, 3 }, { 0, 0, 0, 0, translate_marker( "Your head is pounding from the heat." ), 2400, "" } },
            { { bp_head, 2 }, { 0, 0, 0, 0, "", 0, "" } },
            { { bp_torso, 3 }, { 2, 0, 0, 0, translate_marker( "You are sweating profusely." ), 2400, "" } },
            { { bp_torso, 2 }, { 1, 0, 0, 0, "", 0, "" } },
            { { bp_hand_l, 3 }, { 0, 2, 0, 0, "", 0, translate_marker( "Your left hand's too sweaty to grip well." ) } },
            { { bp_hand_l, 2 }, { 0, 1, 0, 0, "", 0, translate_marker( "Your left hand's too sweaty to grip well." ) } },
            { { bp_hand_r, 3 }, { 0, 2, 0, 0, "", 0, translate_marker( "Your right hand's too sweaty to grip well." ) } },
            { { bp_hand_r, 2 }, { 0, 1, 0, 0, "", 0, translate_marker( "Your right hand's too sweaty to grip well." ) } },
            { { bp_leg_l, 3 }, { 0, 0, 0, 0, translate_marker( "Your left leg is cramping up." ), 4800, "" } },
            { { bp_leg_l, 2 }, { 0, 0, 0, 0, "", 0, "" } },
            { { bp_leg_r, 3 }, { 0, 0, 0, 0, translate_marker( "Your right leg is cramping up." ), 4800, "" } },
            { { bp_leg_r, 2 }, { 0, 0, 0, 0, "", 0, "" } },
            { { bp_foot_l, 3 }, { 0, 0, 0, 0, translate_marker( "Your left foot is swelling in the heat." ), 4800, "" } },
            { { bp_foot_l, 2 }, { 0, 0, 0, 0, "", 0, "" } },
            { { bp_foot_r, 3 }, { 0, 0, 0, 0, translate_marker( "Your right foot is swelling in the heat." ), 4800, "" } },
            { { bp_foot_r, 2 }, { 0, 0, 0, 0, "", 0, "" } },
        }
    };

    const body_part bp = it.get_bp();
    const int intense = it.get_intensity();
    const auto iter = effs.find( { it.get_bp(), it.get_intensity() } );
    if( iter != effs.end() ) {
        iter->second.apply( u );
    }
    // Hothead effects are a special snowflake
    if( bp == bp_head && intense >= 2 ) {
        if( one_in( std::max( 25, std::min( 89500, 90000 - u.temp_cur[bp_head] ) ) ) ) {
            u.vomit();
        }
        if( !u.has_effect( effect_sleep ) && one_in( 2400 ) ) {
            u.add_msg_if_player( m_bad, _( "The heat is making you see things." ) );
        }
    }
}

static void eff_fun_frostbite( player &u, effect &it )
{
    // { body_part, intensity }, { str_pen, dex_pen, int_pen, per_pen, msg, msg_chance, miss_msg }
    static const std::map<std::pair<body_part, int>, temperature_effect> effs = {{
            { { bp_hand_l, 2 }, { 0, 2, 0, 0, "", 0, translate_marker( "You have trouble grasping with your numb fingers." ) } },
            { { bp_hand_r, 2 }, { 0, 2, 0, 0, "", 0, translate_marker( "You have trouble grasping with your numb fingers." ) } },
            { { bp_foot_l, 2 }, { 0, 0, 0, 0, translate_marker( "Your foot has gone numb." ), 4800, "" } },
            { { bp_foot_l, 1 }, { 0, 0, 0, 0, translate_marker( "Your foot has gone numb." ), 4800, "" } },
            { { bp_foot_r, 2 }, { 0, 0, 0, 0, translate_marker( "Your foot has gone numb." ), 4800, "" } },
            { { bp_foot_r, 1 }, { 0, 0, 0, 0, translate_marker( "Your foot has gone numb." ), 4800, "" } },
            { { bp_mouth, 2 }, { 0, 0, 0, 3, translate_marker( "Your face feels numb." ), 4800, "" } },
            { { bp_mouth, 1 }, { 0, 0, 0, 1, translate_marker( "Your face feels numb." ), 4800, "" } },
        }
    };
    const auto iter = effs.find( { it.get_bp(), it.get_intensity() } );
    if( iter != effs.end() ) {
        iter->second.apply( u );
    }
}

void player::hardcoded_effects( effect &it )
{
    if( auto buff = ma_buff::from_effect( it ) ) {
        if( buff->is_valid_player( *this ) ) {
            buff->apply_player( *this );
        } else {
            it.set_duration( 0_turns ); // removes the effect
        }
        return;
    }
    using hc_effect_fun = std::function<void( player &, effect & )>;
    static const std::map<efftype_id, hc_effect_fun> hc_effect_map = {{
            { effect_onfire, eff_fun_onfire },
            { effect_spores, eff_fun_spores },
            { effect_fungus, eff_fun_fungus },
            { effect_rat, eff_fun_rat },
            { effect_bleed, eff_fun_bleed },
            { effect_hallu, eff_fun_hallu },
            { effect_cold, eff_fun_cold },
            { effect_hot, eff_fun_hot },
            { effect_frostbite, eff_fun_frostbite },
        }
    };
    const efftype_id &id = it.get_id();
    const auto &iter = hc_effect_map.find( id );
    if( iter != hc_effect_map.end() ) {
        iter->second( *this, it );
        return;
    }

    const time_point start = it.get_start_time();
    const time_duration dur = it.get_duration();
    int intense = it.get_intensity();
    body_part bp = it.get_bp();
    bool sleeping = has_effect( effect_sleep );
    if( id == effect_dermatik ) {
        bool triggered = false;
        int formication_chance = 3600;
        if( dur < 4_hours ) {
            formication_chance += 14400 - to_turns<int>( dur );
        }
        if( one_in( formication_chance ) ) {
            add_effect( effect_formication, 60_minutes, bp );
        }
        if( dur < 1_days && one_in( 14400 ) ) {
            vomit();
        }
        if( dur > 1_days ) {
            // Spawn some larvae!
            // Choose how many insects; more for large characters
            ///\EFFECT_STR_MAX increases number of insects hatched from dermatik infection
            int num_insects = rng( 1, std::min( 3, str_max / 3 ) );
            apply_damage( nullptr, bp, rng( 2, 4 ) * num_insects );
            // Figure out where they may be placed
            add_msg_player_or_npc( m_bad,
                                   _( "Your flesh crawls; insects tear through the flesh and begin to emerge!" ),
                                   _( "Insects begin to emerge from <npcname>'s skin!" ) );
            for( const tripoint &dest : g->m.points_in_radius( pos(), 1 ) ) {
                if( num_insects == 0 ) {
                    break;
                } else if( pos() == dest ) {
                    continue;
                }
                if( !g->critter_at( dest ) ) {
                    if( monster *const grub = g->summon_mon( mon_dermatik_larva, dest ) ) {
                        if( one_in( 3 ) ) {
                            grub->friendly = -1;
                        }
                    }
                    num_insects--;
                }
            }
            g->events().send<event_type::dermatik_eggs_hatch>( getID() );
            remove_effect( effect_formication, bp );
            moves -= 600;
            triggered = true;
        }
        if( triggered ) {
            // Set ourselves up for removal
            it.set_duration( 0_turns );
        } else {
            // Count duration up
            it.mod_duration( 1_turns );
        }
    } else if( id == effect_formication ) {
        ///\EFFECT_INT decreases occurrence of itching from formication effect
        if( x_in_y( intense, 600 + 300 * get_int() ) && !has_effect( effect_narcosis ) ) {
            if( !is_npc() ) {
                //~ %s is bodypart in accusative.
                add_msg( m_warning, _( "You start scratching your %s!" ), body_part_name_accusative( bp ) );
                g->u.cancel_activity();
            } else if( g->u.sees( pos() ) ) {
                //~ 1$s is NPC name, 2$s is bodypart in accusative.
                add_msg( _( "%1$s starts scratching their %2$s!" ), name, body_part_name_accusative( bp ) );
            }
            moves -= 150;
            apply_damage( nullptr, bp, 1 );
        }
    } else if( id == effect_evil ) {
        // Worn or wielded; diminished effects
        bool lesserEvil = weapon.has_effect_when_wielded( AEP_EVIL ) ||
                          weapon.has_effect_when_carried( AEP_EVIL );
        for( auto &w : worn ) {
            if( w.has_effect_when_worn( AEP_EVIL ) ) {
                lesserEvil = true;
                break;
            }
        }
        if( lesserEvil ) {
            // Only minor effects, some even good!
            mod_str_bonus( dur > 450_minutes ? 10.0 : dur / 45_minutes );
            if( dur < 1_hours ) {
                mod_dex_bonus( 1 );
            } else {
                int dex_mod = -( dur > 360_minutes ? 10.0 : ( dur - 1_hours ) / 30_minutes );
                mod_dex_bonus( dex_mod );
                add_miss_reason( _( "Why waste your time on that insignificant speck?" ), -dex_mod );
            }
            mod_int_bonus( -( dur > 300_minutes ? 10.0 : ( dur - 50_minutes ) / 25_minutes ) );
            mod_per_bonus( -( dur > 480_minutes ? 10.0 : ( dur - 80_minutes ) / 40_minutes ) );
        } else {
            // Major effects, all bad.
            mod_str_bonus( -( dur > 500_minutes ? 10.0 : dur / 50_minutes ) );
            int dex_mod = -( dur > 600_minutes ? 10.0 : dur / 60_minutes );
            mod_dex_bonus( dex_mod );
            add_miss_reason( _( "Why waste your time on that insignificant speck?" ), -dex_mod );
            mod_int_bonus( -( dur > 450_minutes ? 10.0 : dur / 45_minutes ) );
            mod_per_bonus( -( dur > 400_minutes ? 10.0 : dur / 40_minutes ) );
        }
    } else if( id == effect_attention ) {
        if( to_turns<int>( dur ) != 0 && one_in( 100000 / to_turns<int>( dur ) ) &&
            one_in( 100000 / to_turns<int>( dur ) ) && one_in( 250 ) ) {
            tripoint dest( 0, 0, posz() );
            int tries = 0;
            do {
                dest.x = posx() + rng( -4, 4 );
                dest.y = posy() + rng( -4, 4 );
                tries++;
            } while( g->critter_at( dest ) && tries < 10 );
            if( tries < 10 ) {
                if( g->m.impassable( dest ) ) {
                    g->m.make_rubble( dest, f_rubble_rock, true );
                }
                MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup(
                                                       mongroup_id( "GROUP_NETHER" ) );
                g->summon_mon( spawn_details.name, dest );
                if( g->u.sees( dest ) ) {
                    g->cancel_activity_or_ignore_query( distraction_type::hostile_spotted,
                                                        _( "A monster appears nearby!" ) );
                    add_msg_if_player( m_warning, _( "A portal opens nearby, and a monster crawls through!" ) );
                }
                it.mult_duration( .25 );
            }
        }
    } else if( id == effect_meth ) {
        if( intense == 1 ) {
            add_miss_reason( _( "The bees have started escaping your teeth." ), 2 );
            if( one_in( 900 ) ) {
                add_msg_if_player( m_bad, _( "You feel paranoid.  They're watching you." ) );
                mod_pain( 1 );
                mod_fatigue( dice( 1, 6 ) );
            } else if( one_in( 3000 ) ) {
                add_msg_if_player( m_bad,
                                   _( "You feel like you need less teeth.  You pull one out, and it is rotten to the core." ) );
                mod_pain( 1 );
            } else if( one_in( 3000 ) ) {
                add_msg_if_player( m_bad, _( "You notice a large abscess.  You pick at it." ) );
                body_part itch = random_body_part( true );
                add_effect( effect_formication, 60_minutes, itch );
                mod_pain( 1 );
            } else if( one_in( 3000 ) ) {
                add_msg_if_player( m_bad,
                                   _( "You feel so sick, like you've been poisoned, but you need more.  So much more." ) );
                vomit();
                mod_fatigue( dice( 1, 6 ) );
            }
        }
    } else if( id == effect_teleglow ) {
        // Default we get around 300 duration points per teleport (possibly more
        // depending on the source).
        // TODO: Include a chance to teleport to the nether realm.
        // TODO: This with regards to NPCS
        if( !is_player() ) {
            // NO, no teleporting around the player because an NPC has teleglow!
            return;
        }
        if( dur > 10_hours ) {
            // 20 teleports (no decay; in practice at least 21)
            if( one_in( 6000 - ( ( dur - 600_minutes ) / 1_minutes ) ) ) {
                if( !is_npc() ) {
                    add_msg( _( "Glowing lights surround you, and you teleport." ) );
                }
                teleport::teleport( this );
                g->events().send<event_type::teleglow_teleports>( getID() );
                if( one_in( 10 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                }
            }
            if( one_in( 7200 - ( dur - 360_minutes ) / 4_turns ) ) {
                add_msg_if_player( m_bad, _( "You are beset with a vision of a prowling beast." ) );
                for( const tripoint &dest : g->m.points_in_radius( pos(), 6 ) ) {
                    if( g->m.is_cornerfloor( dest ) ) {
                        g->m.add_field( dest, fd_tindalos_rift, 3 );
                        add_msg_if_player( m_info, _( "Your surroundings are permeated with a foul scent." ) );
                        break;
                    }
                }
                if( one_in( 2 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                }
            }
            if( one_in( 7200 - ( ( dur - 600_minutes ) / 30_seconds ) ) && one_in( 20 ) ) {
                if( !is_npc() ) {
                    add_msg( m_bad, _( "You pass out." ) );
                }
                fall_asleep( 2_hours );
                if( one_in( 6 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                }
            }
        }
        if( dur > 6_hours ) {
            // 12 teleports
            if( one_in( 24000 - ( dur - 360_minutes ) / 4_turns ) ) {
                tripoint dest( 0, 0, posz() );
                int &x = dest.x;
                int &y = dest.y;
                int tries = 0;
                do {
                    x = posx() + rng( -4, 4 );
                    y = posy() + rng( -4, 4 );
                    tries++;
                    if( tries >= 10 ) {
                        break;
                    }
                } while( g->critter_at( dest ) );
                if( tries < 10 ) {
                    if( g->m.impassable( dest ) ) {
                        g->m.make_rubble( dest, f_rubble_rock, true );
                    }
                    MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup(
                                                           mongroup_id( "GROUP_NETHER" ) );
                    g->summon_mon( spawn_details.name, dest );
                    if( g->u.sees( dest ) ) {
                        g->cancel_activity_or_ignore_query( distraction_type::hostile_spotted,
                                                            _( "A monster appears nearby!" ) );
                        add_msg( m_warning, _( "A portal opens nearby, and a monster crawls through!" ) );
                    }
                    if( one_in( 2 ) ) {
                        // Set ourselves up for removal
                        it.set_duration( 0_turns );
                    }
                }
            }
            if( one_in( 21000 - ( dur - 360_minutes ) / 4_turns ) ) {
                add_msg_if_player( m_bad, _( "You shudder suddenly." ) );
                mutate();
                if( one_in( 4 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                }
            }
        }
        if( dur > 4_hours ) {
            // 8 teleports
            if( one_in( 10000 - to_turns<int>( dur ) ) && !has_effect( effect_valium ) ) {
                add_effect( effect_shakes, rng( 4_minutes, 8_minutes ) );
            }
            if( one_in( 12000 - to_turns<int>( dur ) ) ) {
                add_msg_if_player( m_bad, _( "Your vision is filled with bright lights..." ) );
                add_effect( effect_blind, rng( 1_minutes, 2_minutes ) );
                if( one_in( 8 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                }
            }
            if( one_in( 5000 ) && !has_effect( effect_hallu ) ) {
                add_effect( effect_hallu, 6_hours );
                if( one_in( 5 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                }
            }
        }
        if( one_in( 4000 ) ) {
            add_msg_if_player( m_bad, _( "You're suddenly covered in ectoplasm." ) );
            add_effect( effect_boomered, 10_minutes );
            if( one_in( 4 ) ) {
                // Set ourselves up for removal
                it.set_duration( 0_turns );
            }
        }
        if( one_in( 10000 ) ) {
            if( !has_trait( trait_id( "M_IMMUNE" ) ) ) {
                add_effect( effect_fungus, 1_turns, num_bp, true );
            } else {
                add_msg_if_player( m_info, _( "We have many colonists awaiting passage." ) );
            }
            // Set ourselves up for removal
            it.set_duration( 0_turns );
        }
    } else if( id == effect_asthma ) {
        if( has_effect( effect_adrenaline ) || has_effect( effect_datura ) ) {
            add_msg_if_player( m_good, _( "Your asthma attack stops." ) );
            it.set_duration( 0_turns );
        } else if( dur > 2_hours ) {
            add_msg_if_player( m_bad, _( "Your asthma overcomes you.\nYou asphyxiate." ) );
            g->events().send<event_type::dies_from_asthma_attack>( getID() );
            hurtall( 500, nullptr );
        } else if( dur > 70_minutes ) {
            if( one_in( 120 ) ) {
                add_msg_if_player( m_bad, _( "You wheeze and gasp for air." ) );
            }
        }
    } else if( id == effect_brainworms ) {
        if( one_in( 1536 ) ) {
            add_msg_if_player( m_bad, _( "Your head aches faintly." ) );
        }
        if( one_in( 6144 ) ) {
            mod_healthy_mod( -10, -100 );
            apply_damage( nullptr, bp_head, rng( 0, 1 ) );
            if( !has_effect( effect_visuals ) ) {
                add_msg_if_player( m_bad, _( "Your vision is getting fuzzy." ) );
                add_effect( effect_visuals, rng( 1_minutes, 60_minutes ) );
            }
        }
        if( one_in( 24576 ) ) {
            mod_healthy_mod( -10, -100 );
            apply_damage( nullptr, bp_head, rng( 1, 2 ) );
            if( !is_blind() && !sleeping ) {
                add_msg_if_player( m_bad, _( "Your vision goes black!" ) );
                add_effect( effect_blind, rng( 5_turns, 20_turns ) );
            }
        }
    } else if( id == effect_tapeworm ) {
        if( one_in( 3072 ) ) {
            add_msg_if_player( m_bad, _( "Your bowels ache." ) );
        }
    } else if( id == effect_bloodworms ) {
        if( one_in( 3072 ) ) {
            add_msg_if_player( m_bad, _( "Your veins itch." ) );
        }
    } else if( id == effect_paincysts ) {
        if( one_in( 3072 ) ) {
            add_msg_if_player( m_bad, _( "Your muscles feel like they're knotted and tired." ) );
        }
    } else if( id == effect_tetanus ) {
        if( one_in( 1536 ) ) {
            add_msg_if_player( m_bad, _( "Your muscles are tight and sore." ) );
        }
        if( !has_effect( effect_valium ) ) {
            add_miss_reason( _( "Your muscles are locking up and you can't fight effectively." ), 4 );
            if( one_in( 3072 ) ) {
                add_msg_if_player( m_bad, _( "Your muscles spasm." ) );
                add_effect( effect_downed, rng( 1_turns, 4_turns ), num_bp, false, 0, true );
                add_effect( effect_stunned, rng( 1_turns, 4_turns ) );
                if( one_in( 10 ) ) {
                    mod_pain( rng( 1, 10 ) );
                }
            }
        }
    } else if( id == effect_datura ) {
        if( dur > 100_minutes && focus_pool >= 1 && one_in( 24 ) ) {
            focus_pool--;
        }
        if( dur > 200_minutes && one_in( 48 ) && stim < 20 ) {
            stim++;
        }
        if( dur > 300_minutes && focus_pool >= 1 && one_in( 12 ) ) {
            focus_pool--;
        }
        if( dur > 400_minutes && one_in( 384 ) ) {
            mod_pain( rng( -1, -8 ) );
        }
        if( ( !has_effect( effect_hallu ) ) && ( dur > 500_minutes && one_in( 24 ) ) ) {
            add_effect( effect_hallu, 6_hours );
        }
        if( dur > 600_minutes && one_in( 768 ) ) {
            mod_pain( rng( -3, -24 ) );
            if( dur > 800_minutes && one_in( 16 ) ) {
                add_msg_if_player( m_bad,
                                   _( "You're experiencing loss of basic motor skills and blurred vision.  Your mind recoils in horror, unable to communicate with your spinal column." ) );
                add_msg_if_player( m_bad, _( "You stagger and fall!" ) );
                add_effect( effect_downed, rng( 1_turns, 4_turns ), num_bp, false, 0, true );
                if( one_in( 8 ) || x_in_y( vomit_mod(), 10 ) ) {
                    vomit();
                }
            }
        }
        if( dur > 700_minutes && focus_pool >= 1 ) {
            focus_pool--;
        }
        if( dur > 800_minutes && one_in( 1536 ) ) {
            add_effect( effect_visuals, rng( 4_minutes, 20_minutes ) );
            mod_pain( rng( -8, -40 ) );
        }
        if( dur > 1200_minutes && one_in( 1536 ) ) {
            add_msg_if_player( m_bad, _( "There's some kind of big machine in the sky." ) );
            add_effect( effect_visuals, rng( 8_minutes, 40_minutes ) );
            if( one_in( 32 ) ) {
                add_msg_if_player( m_bad, _( "It's some kind of electric snake, coming right at you!" ) );
                mod_pain( rng( 4, 40 ) );
                vomit();
            }
        }
        if( dur > 1400_minutes && one_in( 768 ) ) {
            add_msg_if_player( m_bad,
                               _( "Order us some golf shoes, otherwise we'll never get out of this place alive." ) );
            add_effect( effect_visuals, rng( 40_minutes, 200_minutes ) );
            if( one_in( 8 ) ) {
                add_msg_if_player( m_bad,
                                   _( "The possibility of physical and mental collapse is now very real." ) );
                if( one_in( 2 ) || x_in_y( vomit_mod(), 10 ) ) {
                    add_msg_if_player( m_bad, _( "No one should be asked to handle this trip." ) );
                    vomit();
                    mod_pain( rng( 8, 40 ) );
                }
            }
        }

        if( dur > 1800_minutes && one_in( 300 * 512 ) ) {
            if( !has_trait( trait_id( "NOPAIN" ) ) ) {
                add_msg_if_player( m_bad,
                                   _( "Your heart spasms painfully and stops, dragging you back to reality as you die." ) );
            } else {
                add_msg_if_player(
                    _( "You dissolve into beautiful paroxysms of energy.  Life fades from your nebulae and you are no more." ) );
            }
            g->events().send<event_type::dies_from_drug_overdose>( getID(), id );
            hp_cur[hp_torso] = 0;
        }
    } else if( id == effect_grabbed ) {
        set_num_blocks_bonus( get_num_blocks_bonus() - 1 );
        int zed_number = 0;
        for( auto &dest : g->m.points_in_radius( pos(), 1, 0 ) ) {
            const monster *const mon = g->critter_at<monster>( dest );
            if( mon && mon->has_effect( effect_grabbing ) ) {
                zed_number += mon->get_grab_strength();
            }
        }
        if( zed_number > 0 ) {
            //If intensity isn't pass the cap, average it with # of zeds
            add_effect( effect_grabbed, 2_turns, bp_torso, false, ( intense + zed_number ) / 2 );
        }
    } else if( id == effect_bite ) {
        bool recovered = false;
        /* Recovery chances, use binomial distributions if balancing here. Healing in the bite
         * stage provides additional benefits, so both the bite stage chance of healing and
         * the cumulative chances for spontaneous healing are both given.
         * Cumulative heal chances for the bite + infection stages:
         * -200 health - 38.6%
         *    0 health - 46.8%
         *  200 health - 53.7%
         *
         * Heal chances in the bite stage:
         * -200 health - 23.4%
         *    0 health - 28.3%
         *  200 health - 32.9%
         *
         * Cumulative heal chances the bite + infection stages with the resistant mutation:
         * -200 health - 82.6%
         *    0 health - 84.5%
         *  200 health - 86.1%
         *
         * Heal chances in the bite stage with the resistant mutation:
         * -200 health - 60.7%
         *    0 health - 63.2%
         *  200 health - 65.6%
         */
        if( dur % 10_turns == 0_turns )  {
            int recover_factor = 100;
            if( has_effect( effect_recover ) ) {
                recover_factor -= get_effect_dur( effect_recover ) / 1_hours;
            }
            if( has_trait( trait_id( "INFRESIST" ) ) ) {
                recover_factor += 200;
            }
            if( has_effect( effect_panacea ) ) {
                recover_factor = 648000; //panacea is a guaranteed cure
            } else if( has_effect( effect_strong_antibiotic ) ) {
                recover_factor += 400;
            } else if( has_effect( effect_antibiotic ) ) {
                recover_factor += 200;
            } else if( has_effect( effect_weak_antibiotic ) ) {
                recover_factor += 100;
            }
            recover_factor += get_healthy() / 10;

            if( x_in_y( recover_factor, 648000 ) ) {
                //~ %s is bodypart name.
                add_msg_if_player( m_good, _( "Your %s wound begins to feel better!" ),
                                   body_part_name( bp ) );
                // Set ourselves up for removal
                it.set_duration( 0_turns );
                recovered = true;
            }
        }
        if( !recovered ) {
            // Move up to infection
            if( dur > 6_hours ) {
                add_effect( effect_infected, 1_turns, bp, true );
                // Set ourselves up for removal
                it.set_duration( 0_turns );
            } else if( has_effect( effect_strong_antibiotic ) ) {
                it.mod_duration( -1_turns ); //strong antibiotic reverses!
            } else if( has_effect( effect_antibiotic ) ) {
                if( calendar::once_every( 8_turns ) ) {
                    it.mod_duration( 1_turns ); //normal antibiotic slows down progression by a factor of 8
                }
            } else if( has_effect( effect_weak_antibiotic ) ) {
                if( calendar::once_every( 2_turns ) ) {
                    it.mod_duration( 1_turns ); //weak antibiotic slows down by half
                }
            } else {
                it.mod_duration( 1_turns );
            }
        }
    } else if( id == effect_infected ) {
        bool recovered = false;
        // Recovery chance, use binomial distributions if balancing here.
        // See "bite" for balancing notes on this.
        if( dur % 10_turns == 0_turns )  {
            int recover_factor = 100;
            if( has_effect( effect_recover ) ) {
                recover_factor -= get_effect_dur( effect_recover ) / 1_hours;
            }
            if( has_trait( trait_id( "INFRESIST" ) ) ) {
                recover_factor += 200;
            }
            if( has_effect( effect_panacea ) ) {
                recover_factor = 5184000;
            } else if( has_effect( effect_strong_antibiotic ) ) {
                recover_factor += 400;
            } else if( has_effect( effect_antibiotic ) ) {
                recover_factor += 200;
            } else if( has_effect( effect_weak_antibiotic ) ) {
                recover_factor += 100;
            }
            recover_factor += get_healthy() / 10;

            if( x_in_y( recover_factor, 5184000 ) ) {
                //~ %s is bodypart name.
                add_msg_if_player( m_good, _( "Your %s wound begins to feel better!" ),
                                   body_part_name( bp ) );
                add_effect( effect_recover, 4 * dur );
                // Set ourselves up for removal
                it.set_duration( 0_turns );
                recovered = true;
            }
        }
        if( !recovered ) {
            // Death happens
            if( dur > 1_days ) {
                add_msg_if_player( m_bad, _( "You succumb to the infection." ) );
                g->events().send<event_type::dies_of_infection>( getID() );
                hurtall( 500, nullptr );
            } else if( has_effect( effect_strong_antibiotic ) ) {
                it.mod_duration( -1_turns );
            } else if( has_effect( effect_antibiotic ) ) {
                if( calendar::once_every( 8_turns ) ) {
                    it.mod_duration( 1_turns );
                }
            } else if( has_effect( effect_weak_antibiotic ) ) {
                if( calendar::once_every( 2_turns ) ) {
                    it.mod_duration( 1_turns );
                }
            } else {
                it.mod_duration( 1_turns );
            }
        }
    } else if( id == effect_lying_down ) {
        set_moves( 0 );
        if( can_sleep() ) {
            fall_asleep();
            // Set ourselves up for removal
            it.set_duration( 0_turns );
        }
        if( dur == 1_turns && !sleeping ) {
            add_msg_if_player( _( "You try to sleep, but can't..." ) );
        }
    } else if( id == effect_sleep ) {
        set_moves( 0 );
#if defined(TILES)
        if( is_player() && calendar::once_every( 10_minutes ) ) {
            SDL_PumpEvents();
        }
#endif // TILES

        if( intense < 1 ) {
            it.set_intensity( 1 );
        } else if( intense < 24 ) {
            it.mod_intensity( 1 );
        }

        if( has_effect( effect_narcosis ) && get_fatigue() <= 25 ) {
            set_fatigue( 25 ); //Prevent us from waking up naturally while under anesthesia
        }

        if( get_fatigue() < -25 && it.get_duration() > 3_minutes && !has_effect( effect_narcosis ) ) {
            it.set_duration( 1_turns * dice( 3, 10 ) );
        }

        if( get_fatigue() <= 0 && get_fatigue() > -20 && !has_effect( effect_narcosis ) ) {
            mod_fatigue( -25 );
            if( get_sleep_deprivation() < SLEEP_DEPRIVATION_HARMLESS ) {
                add_msg_if_player( m_good, _( "You feel well rested." ) );
            } else {
                add_msg_if_player( m_warning,
                                   _( "You feel physically rested, but you haven't been able to catch up on your missed sleep yet." ) );
            }
            it.set_duration( 1_turns * dice( 3, 100 ) );
        }

        // TODO: Move this to update_needs when NPCs can mutate
        bool compatible_weather_types = g->weather.weather == WEATHER_CLEAR ||
                                        g->weather.weather == WEATHER_SUNNY
                                        || g->weather.weather == WEATHER_DRIZZLE || g->weather.weather == WEATHER_RAINY ||
                                        g->weather.weather == WEATHER_FLURRIES
                                        || g->weather.weather == WEATHER_CLOUDY || g->weather.weather == WEATHER_SNOW;

        if( calendar::once_every( 10_minutes ) && ( has_trait( trait_id( "CHLOROMORPH" ) ) ||
                has_trait( trait_id( "M_SKIN3" ) ) || has_trait( trait_id( "WATERSLEEP" ) ) ) &&
            g->m.is_outside( pos() ) ) {
            if( has_trait( trait_id( "CHLOROMORPH" ) ) ) {
                // Hunger and thirst fall before your Chloromorphic physiology!
                if( g->natural_light_level( posz() ) >= 12 && compatible_weather_types ) {
                    if( get_hunger() >= -30 ) {
                        mod_hunger( -5 );
                        // photosynthesis warrants absorbing kcal directly
                        mod_stored_nutr( -5 );
                    }
                    if( get_thirst() >= -30 ) {
                        mod_thirst( -5 );
                    }
                }
            }
            if( has_trait( trait_id( "M_SKIN3" ) ) ) {
                // Spores happen!
                if( g->m.has_flag_ter_or_furn( "FUNGUS", pos() ) ) {
                    if( get_fatigue() >= 0 ) {
                        mod_fatigue( -5 ); // Local guides need less sleep on fungal soil
                    }
                    if( calendar::once_every( 1_hours ) ) {
                        spores(); // spawn some P O O F Y   B O I S
                    }
                }
            }
            if( has_trait( trait_id( "WATERSLEEP" ) ) ) {
                mod_fatigue( -3 ); // Fish sleep less in water
            }
        }

        // Check mutation category strengths to see if we're mutated enough to get a dream
        std::string highcat = get_highest_category();
        int highest = mutation_category_level[highcat];

        // Determine the strength of effects or dreams based upon category strength
        int strength = 0; // Category too weak for any effect or dream
        if( crossed_threshold() ) {
            strength = 4; // Post-human.
        } else if( highest >= 20 && highest < 35 ) {
            strength = 1; // Low strength
        } else if( highest >= 35 && highest < 50 ) {
            strength = 2; // Medium strength
        } else if( highest >= 50 ) {
            strength = 3; // High strength
        }

        // Get a dream if category strength is high enough.
        if( strength != 0 ) {
            //Once every 6 / 3 / 2 hours, with a bit of randomness
            if( calendar::once_every( 6_hours / strength ) && one_in( 3 ) ) {
                // Select a dream
                std::string dream = get_category_dream( highcat, strength );
                if( !dream.empty() ) {
                    add_msg_if_player( dream );
                }
                // Mycus folks upgrade in their sleep.
                if( has_trait( trait_id( "THRESH_MYCUS" ) ) ) {
                    if( one_in( 8 ) ) {
                        mutate_category( "MYCUS" );
                        mod_stored_nutr( 10 );
                        mod_thirst( 10 );
                        mod_fatigue( 5 );
                    }
                }
            }
        }

        bool woke_up = false;
        int tirednessVal = rng( 5, 200 ) + rng( 0, abs( get_fatigue() * 2 * 5 ) );
        if( !is_blind() && !has_effect( effect_narcosis ) ) {
            if( !has_trait(
                    trait_id( "SEESLEEP" ) ) ) { // People who can see while sleeping are acclimated to the light.
                if( has_trait( trait_id( "HEAVYSLEEPER2" ) ) && !has_trait( trait_id( "HIBERNATE" ) ) ) {
                    // So you can too sleep through noon
                    if( ( tirednessVal * 1.25 ) < g->m.ambient_light_at( pos() ) && ( get_fatigue() < 10 ||
                            one_in( get_fatigue() / 2 ) ) ) {
                        add_msg_if_player( _( "It's too bright to sleep." ) );
                        // Set ourselves up for removal
                        it.set_duration( 0_turns );
                        woke_up = true;
                    }
                    // Ursine hibernators would likely do so indoors.  Plants, though, might be in the sun.
                } else if( has_trait( trait_id( "HIBERNATE" ) ) ) {
                    if( ( tirednessVal * 5 ) < g->m.ambient_light_at( pos() ) && ( get_fatigue() < 10 ||
                            one_in( get_fatigue() / 2 ) ) ) {
                        add_msg_if_player( _( "It's too bright to sleep." ) );
                        // Set ourselves up for removal
                        it.set_duration( 0_turns );
                        woke_up = true;
                    }
                } else if( tirednessVal < g->m.ambient_light_at( pos() ) && ( get_fatigue() < 10 ||
                           one_in( get_fatigue() / 2 ) ) ) {
                    add_msg_if_player( _( "It's too bright to sleep." ) );
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                    woke_up = true;
                }
            } else if( has_active_mutation( trait_id( "SEESLEEP" ) ) ) {
                Creature *hostile_critter = g->is_hostile_very_close();
                if( hostile_critter != nullptr ) {
                    add_msg_if_player( _( "You see %s approaching!" ),
                                       hostile_critter->disp_name() );
                    it.set_duration( 0_turns );
                    woke_up = true;
                }
            }
        }

        // Have we already woken up?
        if( !woke_up && !has_effect( effect_narcosis ) ) {
            // Cold or heat may wake you up.
            // Player will sleep through cold or heat if fatigued enough
            for( const body_part bp : all_body_parts ) {
                if( temp_cur[bp] < BODYTEMP_VERY_COLD - get_fatigue() / 2 ) {
                    if( one_in( 30000 ) ) {
                        add_msg_if_player( _( "You toss and turn trying to keep warm." ) );
                    }
                    if( temp_cur[bp] < BODYTEMP_FREEZING - get_fatigue() / 2 ||
                        one_in( temp_cur[bp] * 6 + 30000 ) ) {
                        add_msg_if_player( m_bad, _( "It's too cold to sleep." ) );
                        // Set ourselves up for removal
                        it.set_duration( 0_turns );
                        woke_up = true;
                        break;
                    }
                } else if( temp_cur[bp] > BODYTEMP_VERY_HOT + get_fatigue() / 2 ) {
                    if( one_in( 30000 ) ) {
                        add_msg_if_player( _( "You toss and turn in the heat." ) );
                    }
                    if( temp_cur[bp] > BODYTEMP_SCORCHING + get_fatigue() / 2 ||
                        one_in( 90000 - temp_cur[bp] ) ) {
                        add_msg_if_player( m_bad, _( "It's too hot to sleep." ) );
                        // Set ourselves up for removal
                        it.set_duration( 0_turns );
                        woke_up = true;
                        break;
                    }
                }
            }
            if( ( ( has_trait( trait_id( "SCHIZOPHRENIC" ) ) || has_artifact_with( AEP_SCHIZO ) ) &&
                  one_in( 43200 ) && is_player() ) ) {
                if( one_in( 2 ) ) {
                    sound_hallu();
                } else {
                    int max_count = rng( 1, 3 );
                    int count = 0;
                    for( const tripoint &mp : g->m.points_in_radius( pos(), 1 ) ) {
                        if( mp == pos() ) {
                            continue;
                        }
                        if( g->m.has_flag( "FLAT", mp ) &&
                            g->m.pl_sees( mp, 2 ) ) {
                            g->spawn_hallucination( mp );
                            if( ++count > max_count ) {
                                break;
                            }
                        }
                    }
                }
                it.set_duration( 0_turns );
                woke_up = true;
            }
        }

        // A bit of a hack: check if we are about to wake up for any reason, including regular timing out of sleep
        if( dur == 1_turns || woke_up ) {
            if( calendar::turn - start > 2_hours ) {
                print_health();
            }
            // alarm was set and player hasn't slept through the alarm.
            if( has_effect( effect_alarm_clock ) && !has_effect( effect_slept_through_alarm ) ) {
                add_msg_if_player( _( "It looks like you woke up just before your alarm." ) );
                remove_effect( effect_alarm_clock );
            } else if( has_effect( effect_slept_through_alarm ) ) { // slept though the alarm.
                if( has_bionic( bionic_id( "bio_watch" ) ) ) {
                    add_msg_if_player( m_warning, _( "It looks like you've slept through your internal alarm..." ) );
                } else {
                    add_msg_if_player( m_warning, _( "It looks like you've slept through the alarm..." ) );
                }
                get_effect( effect_slept_through_alarm ).set_duration( 0_turns );
                remove_effect( effect_alarm_clock );
            }
        }
    } else if( id == effect_alarm_clock ) {
        if( in_sleep_state() ) {
            const bool asleep = has_effect( effect_sleep );
            if( has_bionic( bionic_id( "bio_watch" ) ) ) {
                if( dur == 1_turns ) {
                    // Normal alarm is volume 12, tested against (2/3/6)d15 for
                    // normal/HEAVYSLEEPER/HEAVYSLEEPER2.
                    //
                    // It's much harder to ignore an alarm inside your own skull,
                    // so this uses an effective volume of 20.
                    const int volume = 20;
                    if( !asleep ) {
                        add_msg_if_player( _( "Your internal chronometer went off and you haven't slept a wink." ) );
                        activity.set_to_null();
                    } else if( ( !( has_trait( trait_id( "HEAVYSLEEPER" ) ) ||
                                    has_trait( trait_id( "HEAVYSLEEPER2" ) ) ) &&
                                 dice( 2, 15 ) < volume ) ||
                               ( has_trait( trait_id( "HEAVYSLEEPER" ) ) && dice( 3, 15 ) < volume ) ||
                               ( has_trait( trait_id( "HEAVYSLEEPER2" ) ) && dice( 6, 15 ) < volume ) ) {
                        // Secure the flag before wake_up() clears the effect
                        bool slept_through = has_effect( effect_slept_through_alarm );
                        wake_up();
                        if( slept_through ) {
                            add_msg_if_player( _( "Your internal chronometer finally wakes you up." ) );
                        } else {
                            add_msg_if_player( _( "Your internal chronometer wakes you up." ) );
                        }
                    } else {
                        if( !has_effect( effect_slept_through_alarm ) ) {
                            add_effect( effect_slept_through_alarm, 1_turns, num_bp, true );
                        }
                        // 10 minute cyber-snooze
                        it.mod_duration( 10_minutes );
                    }
                }
            } else {
                if( asleep && dur == 1_turns ) {
                    if( !has_effect( effect_slept_through_alarm ) ) {
                        add_effect( effect_slept_through_alarm, 1_turns, num_bp, true );
                    }
                    // 10 minute automatic snooze
                    it.mod_duration( 10_minutes );
                } else if( dur == 2_turns ) {
                    // let the sound code handle the wake-up part
                    sounds::sound( pos(), 16, sounds::sound_t::alarm, _( "beep-beep-beep!" ), false, "tool",
                                   "alarm_clock" );
                }
            }
        } else {
            if( dur == 1_turns ) {
                if( g->u.has_alarm_clock() ) {
                    sounds::sound( g->u.pos(), 16, sounds::sound_t::alarm, _( "beep-beep-beep!" ), false, "tool",
                                   "alarm_clock" );
                    const std::string alarm = _( "Your alarm is going off." );
                    g->cancel_activity_or_ignore_query( distraction_type::noise, alarm );
                    add_msg( "Your alarm went off." );
                }
            }
        }
    } else if( id == effect_mending ) {
        // TODO: Remove this and encapsulate hp_cur instead
        if( hp_cur[bp_to_hp( bp )] > 0 ) {
            it.set_duration( 0_turns );
        }
    } else if( id == effect_disabled ) {
        // TODO: Remove this and encapsulate hp_cur instead
        if( hp_cur[bp_to_hp( bp )] > 0 ) {
            // Just unpause, in case someone added it as a temporary effect (numbing poison etc.)
            it.unpause_effect();
        }
    } else if( id == effect_panacea ) {
        // restore health all body parts, dramatically reduce pain
        for( int i = 0; i < num_hp_parts; i++ ) {
            hp_cur[i] += 10;
        }
        mod_pain( -10 );
    }
}
