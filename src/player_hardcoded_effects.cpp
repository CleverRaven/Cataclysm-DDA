#include "player.h"
#include "effect.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "fungal_effects.h"
#include "sounds.h"
#include "martialarts.h"
#include "weather.h"
#include "messages.h"
#include "mapdata.h"
#include "monster.h"
#include "vitamin.h"
#include "mongroup.h"

#ifdef TILES
#include "SDL.h"
#endif // TILES

#include <functional>

const mtype_id mon_dermatik_larva( "mon_dermatik_larva" );

const efftype_id effect_adrenaline( "adrenaline" );
const efftype_id effect_alarm_clock( "alarm_clock" );
const efftype_id effect_asthma( "asthma" );
const efftype_id effect_attention( "attention" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_bloodworms( "bloodworms" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_brainworms( "brainworms" );
const efftype_id effect_cold( "cold" );
const efftype_id effect_datura( "datura" );
const efftype_id effect_dermatik( "dermatik" );
const efftype_id effect_disabled( "disabled" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_evil( "evil" );
const efftype_id effect_formication( "formication" );
const efftype_id effect_frostbite( "frostbite" );
const efftype_id effect_fungus( "fungus" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_hot( "hot" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_mending( "mending" );
const efftype_id effect_meth( "meth" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_paincysts( "paincysts" );
const efftype_id effect_rat( "rat" );
const efftype_id effect_recover( "recover" );
const efftype_id effect_shakes( "shakes" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_spores( "spores" );
const efftype_id effect_stemcell_treatment( "stemcell_treatment" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_tapeworm( "tapeworm" );
const efftype_id effect_teleglow( "teleglow" );
const efftype_id effect_tetanus( "tetanus" );
const efftype_id effect_valium( "valium" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_cough_suppress( "cough_suppress" );

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
            x_in_y( intense, 150 + u.get_healthy() / 10 ) ) ) {
        u.add_effect( effect_fungus, 1, num_bp, true );
    }
}
static void eff_fun_fungus( player &u, effect &it )
{
    const int dur = it.get_duration();
    const int intense = it.get_intensity();
    int bonus = u.get_healthy() / 10 + ( u.resists_effect( it ) ? 100 : 0 );
    switch( intense ) {
        case 1: // First hour symptoms
            if( one_in( 160 + bonus ) ) {
                u.cough( true );
            }
            if( one_in( 100 + bonus ) ) {
                u.add_msg_if_player( m_warning, _( "You feel nauseous." ) );
            }
            if( one_in( 100 + bonus ) ) {
                u.add_msg_if_player( m_warning, _( "You smell and taste mushrooms." ) );
            }
            it.mod_duration( 1 );
            if( dur > 600 ) {
                it.mod_intensity( 1 );
            }
            break;
        case 2: // Five hours of worse symptoms
            if( one_in( 600 + bonus * 3 ) ) {
                u.add_msg_if_player( m_bad,  _( "You spasm suddenly!" ) );
                u.moves -= 100;
                u.apply_damage( nullptr, bp_torso, 5 );
            }
            if( x_in_y( u.vomit_mod(), ( 800 + bonus * 4 ) ) || one_in( 2000 + bonus * 10 ) ) {
                u.add_msg_player_or_npc( m_bad, _( "You vomit a thick, gray goop." ),
                                         _( "<npcname> vomits a thick, gray goop." ) );

                int awfulness = rng( 0, 70 );
                u.moves = -200;
                u.mod_hunger( awfulness );
                u.mod_thirst( awfulness );
                ///\EFFECT_STR decreases damage taken by fungus effect
                u.apply_damage( nullptr, bp_torso, awfulness / std::max( u.str_cur, 1 ) ); // can't be healthy
            }
            it.mod_duration( 1 );
            if( dur > 3600 ) {
                it.mod_intensity( 1 );
            }
            break;
        case 3: // Permanent symptoms
            if( one_in( 1000 + bonus * 8 ) ) {
                u.add_msg_player_or_npc( m_bad,  _( "You vomit thousands of live spores!" ),
                                         _( "<npcname> vomits thousands of live spores!" ) );

                u.moves = -500;
                fungal_effects fe( *g, g->m );
                for( int i = -1; i <= 1; i++ ) {
                    for( int j = -1; j <= 1; j++ ) {
                        if( i == 0 && j == 0 ) {
                            continue;
                        }

                        tripoint sporep( u.posx() + i, u.posy() + j, u.posz() );
                        fe.fungalize( sporep, &u, 0.25 );
                    }
                }
                // We're fucked
            } else if( one_in( 6000 + bonus * 20 ) ) {
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
    const int dur = it.get_duration();
    it.set_intensity( dur / 10 );
    if( rng( 0, 100 ) < dur / 10 ) {
        if( !one_in( 5 ) ) {
            u.mutate_category( "MUTCAT_RAT" );
            it.mult_duration( .2 );
        } else {
            u.mutate_category( "MUTCAT_TROGLOBITE" );
            it.mult_duration( .33 );
        }
    } else if( rng( 0, 100 ) < dur / 8 ) {
        if( one_in( 3 ) ) {
            u.vomit();
            it.mod_duration( -10 );
        } else {
            u.add_msg_if_player( m_bad, _( "You feel nauseous!" ) );
            it.mod_duration( 3 );
        }
    }
}
static void eff_fun_bleed( player &u, effect &it )
{
    // Presuming that during the first-aid process you're putting pressure
    // on the wound or otherwise suppressing the flow. (Kits contain either
    // quikclot or bandages per the recipe.)
    const int intense = it.get_intensity();
    if( one_in( 6 / intense ) && u.activity.id() != activity_id( "ACT_FIRSTAID" ) ) {
        u.add_msg_player_or_npc( m_bad, _( "You lose some blood." ),
                                 _( "<npcname> loses some blood." ) );
        // Prolonged haemorrhage is a significant risk for developing anaemia
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
    constexpr int maxDuration = 3600;
    constexpr int comeupTime = int( maxDuration * 0.9 );
    constexpr int noticeTime = int( comeupTime + ( maxDuration - comeupTime ) / 2 );
    constexpr int peakTime = int( maxDuration * 0.8 );
    constexpr int comedownTime = int( maxDuration * 0.3 );
    const int dur = it.get_duration();
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
        if( u.get_stomach_food() > 0 && ( one_in( 200 ) || x_in_y( u.vomit_mod(), 50 ) ) ) {
            u.add_msg_if_player( m_bad, _( "You feel sick to your stomach." ) );
            u.mod_hunger( -2 );
            if( one_in( 6 ) ) {
                u.vomit();
            }
        }
        if( u.is_npc() && one_in( 200 ) ) {
            static const std::array<std::string, 4> npc_hallu = {{
                    _( "\"I think it's starting to kick in.\"" ),
                    _( "\"Oh God, what's happening?\"" ),
                    _( "\"Of course... it's all fractals!\"" ),
                    _( "\"Huh?  What was that?\"" )
                }
            };

            const std::string &npc_text = random_entry_ref( npc_hallu );
            ///\EFFECT_STR_NPC increases volume of hallucination sounds (NEGATIVE)

            ///\EFFECT_INT_NPC decreases volume of hallucination sounds
            int loudness = 20 + u.str_cur - u.int_cur;
            loudness = ( loudness > 5 ? loudness : 5 );
            loudness = ( loudness < 30 ? loudness : 30 );
            sounds::sound( u.pos(), loudness, npc_text );
        }
    } else if( dur == peakTime ) {
        // Visuals start
        u.add_msg_if_player( m_bad, _( "Fractal patterns dance across your vision." ) );
        u.add_effect( effect_visuals, peakTime - comedownTime );
    } else if( dur > comedownTime && dur < peakTime ) {
        // Full symptoms
        u.mod_per_bonus( -2 );
        u.mod_int_bonus( -1 );
        u.mod_dex_bonus( -2 );
        u.add_miss_reason( _( "Dancing fractals distract you." ), 2 );
        u.mod_str_bonus( -1 );
        if( u.is_player() && one_in( 50 ) ) {
            g->spawn_hallucination();
        }
    } else if( dur == comedownTime ) {
        if( one_in( 42 ) ) {
            u.add_msg_if_player( _( "Everything looks SO boring now." ) );
        } else {
            u.add_msg_if_player( _( "Things are returning to normal." ) );
        }
    }
}

void player::hardcoded_effects( effect &it )
{
    if( auto buff = ma_buff::from_effect( it ) ) {
        if( buff->is_valid_player( *this ) ) {
            buff->apply_player( *this );
        } else {
            it.set_duration( 0 ); // removes the effect
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
            { effect_hallu, eff_fun_hallu }
        }
    };
    const efftype_id &id = it.get_id();
    const auto &iter = hc_effect_map.find( id );
    if( iter != hc_effect_map.end() ) {
        iter->second( *this, it );
        return;
    }

    int start = it.get_start_turn();
    int dur = it.get_duration();
    int intense = it.get_intensity();
    body_part bp = it.get_bp();
    bool sleeping = has_effect( effect_sleep );
    bool msg_trig = one_in( 400 );
    if( id == effect_cold ) {
        switch( bp ) {
            case bp_head:
                switch( intense ) {
                    case 3:
                        mod_int_bonus( -2 );
                        if( !sleeping && msg_trig ) {
                            add_msg_if_player( _( "Your thoughts are unclear." ) );
                        }
                    case 2:
                        mod_int_bonus( -1 );
                    default:
                        break;
                }
                break;
            case bp_mouth:
                switch( intense ) {
                    case 3:
                        mod_per_bonus( -2 );
                    case 2:
                        mod_per_bonus( -1 );
                        if( !sleeping && msg_trig ) {
                            add_msg_if_player( m_bad, _( "Your face is stiff from the cold." ) );
                        }
                    default:
                        break;
                }
                break;
            case bp_torso:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -2 );
                        add_miss_reason( _( "You quiver from the cold." ), 2 );
                        if( !sleeping && msg_trig ) {
                            add_msg_if_player( m_bad,
                                               _( "Your torso is freezing cold. You should put on a few more layers." ) );
                        }
                    case 2:
                        mod_dex_bonus( -2 );
                        add_miss_reason( _( "Your shivering makes you unsteady." ), 2 );
                }
                break;
            case bp_arm_l:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                    case 2:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your left arm trembles from the cold." ), 1 );
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your left arm is shivering." ) );
                        }
                    default:
                        break;
                }
                break;
            case bp_arm_r:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                    case 2:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your right arm trembles from the cold." ), 1 );
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your right arm is shivering." ) );
                        }
                    default:
                        break;
                }
                break;
            case bp_hand_l:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                    case 2:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your left hand quivers in the cold." ), 1 );
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your left hand feels like ice." ) );
                        }
                    default:
                        break;
                }
                break;
            case bp_hand_r:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                    case 2:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your right hand trembles in the cold." ), 1 );
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your right hand feels like ice." ) );
                        }
                    default:
                        break;
                }
                break;
            case bp_leg_l:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your legs uncontrollably shake from the cold." ), 1 );
                        mod_str_bonus( -1 );
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your left leg trembles against the relentless cold." ) );
                        }
                    case 2:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your legs unsteadily shiver against the cold." ), 1 );
                        mod_str_bonus( -1 );
                    default:
                        break;
                }
                break;
            case bp_leg_r:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                        mod_str_bonus( -1 );
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your right leg trembles against the relentless cold." ) );
                        }
                    case 2:
                        mod_dex_bonus( -1 );
                        mod_str_bonus( -1 );
                    default:
                        break;
                }
                break;
            case bp_foot_l:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your left foot is as nimble as a block of ice." ), 1 );
                        mod_str_bonus( -1 );
                        break;
                    case 2:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your freezing left foot messes up your balance." ), 1 );
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your left foot feels frigid." ) );
                        }
                    default:
                        break;
                }
                break;
            case bp_foot_r:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your right foot is as nimble as a block of ice." ), 1 );
                        mod_str_bonus( -1 );
                        break;
                    case 2:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your freezing right foot messes up your balance." ), 1 );
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your right foot feels frigid." ) );
                        }
                    default:
                        break;
                }
                break;
            default: // Suppress compiler warning [-Wswitch]
                break;
        }
    } else if( id == effect_hot ) {
        switch( bp ) {
            case bp_head:
                switch( intense ) {
                    case 3:
                        if( !sleeping && msg_trig ) {
                            add_msg_if_player( m_bad, _( "Your head is pounding from the heat." ) );
                        }
                    // Fall-through
                    case 2:
                        // Hallucinations handled in game.cpp
                        if( one_in( std::min( 14500, 15000 - temp_cur[bp_head] ) ) ) {
                            vomit();
                        }
                        if( !sleeping && msg_trig ) {
                            add_msg_if_player( m_bad, _( "The heat is making you see things." ) );
                        }
                        break;
                    default: // Suppress compiler warning [-Wswitch]
                        break;
                }
                break;
            case bp_torso:
                switch( intense ) {
                    case 3:
                        mod_str_bonus( -1 );
                        if( !sleeping && msg_trig ) {
                            add_msg_if_player( m_bad, _( "You are sweating profusely." ) );
                        }
                    // Fall-through
                    case 2:
                        mod_str_bonus( -1 );
                        break;
                    default:
                        break;
                }
                break;
            case bp_hand_l:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                    // Fall-through
                    case 2:
                        add_miss_reason( _( "Your left hand's too sweaty to grip well." ), 1 );
                        mod_dex_bonus( -1 );
                    default:
                        break;
                }
                break;
            case bp_hand_r:
                switch( intense ) {
                    case 3:
                        mod_dex_bonus( -1 );
                    // Fall-through
                    case 2:
                        mod_dex_bonus( -1 );
                        add_miss_reason( _( "Your right hand's too sweaty to grip well." ), 1 );
                        break;
                    default:
                        break;
                }
                break;
            case bp_leg_l:
                switch( intense ) {
                    case 3 :
                        if( one_in( 2 ) ) {
                            if( !sleeping && msg_trig ) {
                                add_msg_if_player( m_bad, _( "Your left leg is cramping up." ) );
                            }
                        }
                        break;
                    default:
                        break;
                }
                break;
            case bp_leg_r:
                switch( intense ) {
                    case 3 :
                        if( one_in( 2 ) ) {
                            if( !sleeping && msg_trig ) {
                                add_msg_if_player( m_bad, _( "Your right leg is cramping up." ) );
                            }
                        }
                        break;
                    default:
                        break;
                }
                break;
            case bp_foot_l:
                switch( intense ) {
                    case 3:
                        if( one_in( 2 ) ) {
                            if( !sleeping && msg_trig ) {
                                add_msg_if_player( m_bad, _( "Your left foot is swelling in the heat." ) );
                            }
                        }
                        break;
                    default:
                        break;
                }
                break;
            case bp_foot_r:
                switch( intense ) {
                    case 3:
                        if( one_in( 2 ) ) {
                            if( !sleeping && msg_trig ) {
                                add_msg_if_player( m_bad, _( "Your right foot is swelling in the heat." ) );
                            }
                        }
                        break;
                    default:
                        break;
                }
                break;
            default: // Suppress compiler warning [-Wswitch]
                break;
        }
    } else if( id == effect_frostbite ) {
        switch( bp ) {
            case bp_hand_l:
            case bp_hand_r:
                switch( intense ) {
                    case 2:
                        add_miss_reason( _( "You have trouble grasping with your numb fingers." ), 2 );
                        mod_dex_bonus( -2 );
                    default:
                        break;
                }
                break;
            case bp_foot_l:
            case bp_foot_r:
                switch( intense ) {
                    case 2:
                    case 1:
                        if( !sleeping && msg_trig && one_in( 2 ) ) {
                            add_msg_if_player( m_bad, _( "Your foot has gone numb." ) );
                        }
                    default:
                        break;
                }
                break;
            case bp_mouth:
                switch( intense ) {
                    case 2:
                        mod_per_bonus( -2 );
                    case 1:
                        mod_per_bonus( -1 );
                        if( !sleeping && msg_trig ) {
                            add_msg_if_player( m_bad, _( "Your face feels numb." ) );
                        }
                    default:
                        break;
                }
                break;
            default: // Suppress compiler warnings [-Wswitch]
                break;
        }
    } else if( id == effect_dermatik ) {
        bool triggered = false;
        int formication_chance = 600;
        if( dur < 2400 ) {
            formication_chance += 2400 - dur;
        }
        if( one_in( formication_chance ) ) {
            add_effect( effect_formication, 600, bp );
        }
        if( dur < 14400 && one_in( 2400 ) ) {
            vomit();
        }
        if( dur > 14400 ) {
            // Spawn some larvae!
            // Choose how many insects; more for large characters
            ///\EFFECT_STR_MAX increases number of insects hatched from dermatik infection
            int num_insects = rng( 1, std::min( 3, str_max / 3 ) );
            apply_damage( nullptr, bp, rng( 2, 4 ) * num_insects );
            // Figure out where they may be placed
            add_msg_player_or_npc( m_bad,
                                   _( "Your flesh crawls; insects tear through the flesh and begin to emerge!" ),
                                   _( "Insects begin to emerge from <npcname>'s skin!" ) );
            for( int i = posx() - 1; i <= posx() + 1; i++ ) {
                for( int j = posy() - 1; j <= posy() + 1; j++ ) {
                    if( num_insects == 0 ) {
                        break;
                    } else if( i == 0 && j == 0 ) {
                        continue;
                    }
                    tripoint dest( i, j, posz() );
                    if( g->mon_at( dest ) == -1 ) {
                        if( g->summon_mon( mon_dermatik_larva, dest ) ) {
                            monster *grub = g->monster_at( dest );
                            if( one_in( 3 ) ) {
                                grub->friendly = -1;
                            }
                        }
                        num_insects--;
                    }
                }
                if( num_insects == 0 ) {
                    break;
                }
            }
            add_memorial_log( pgettext( "memorial_male", "Dermatik eggs hatched." ),
                              pgettext( "memorial_female", "Dermatik eggs hatched." ) );
            remove_effect( effect_formication, bp );
            moves -= 600;
            triggered = true;
        }
        if( triggered ) {
            // Set ourselves up for removal
            it.set_duration( 0 );
        } else {
            // Count duration up
            it.mod_duration( 1 );
        }
    } else if( id == effect_formication ) {
        ///\EFFECT_INT decreases occurence of itching from formication effect
        if( x_in_y( intense, 100 + 50 * get_int() ) ) {
            if( !is_npc() ) {
                //~ %s is bodypart in accusative.
                add_msg( m_warning, _( "You start scratching your %s!" ),
                         body_part_name_accusative( bp ).c_str() );
                g->cancel_activity();
            } else if( g->u.sees( pos() ) ) {
                //~ 1$s is NPC name, 2$s is bodypart in accusative.
                add_msg( _( "%1$s starts scratching their %2$s!" ), name.c_str(),
                         body_part_name_accusative( bp ).c_str() );
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
            mod_str_bonus( dur > 4500 ? 10 : int( dur / 450 ) );
            if( dur < 600 ) {
                mod_dex_bonus( 1 );
            } else {
                int dex_mod = -( dur > 3600 ? 10 : int( ( dur - 600 ) / 300 ) );
                mod_dex_bonus( dex_mod );
                add_miss_reason( _( "Why waste your time on that insignificant speck?" ), -dex_mod );
            }
            mod_int_bonus( -( dur > 3000 ? 10 : int( ( dur - 500 ) / 250 ) ) );
            mod_per_bonus( -( dur > 4800 ? 10 : int( ( dur - 800 ) / 400 ) ) );
        } else {
            // Major effects, all bad.
            mod_str_bonus( -( dur > 5000 ? 10 : int( dur / 500 ) ) );
            int dex_mod = -( dur > 6000 ? 10 : int( dur / 600 ) );
            mod_dex_bonus( dex_mod );
            add_miss_reason( _( "Why waste your time on that insignificant speck?" ), -dex_mod );
            mod_int_bonus( -( dur > 4500 ? 10 : int( dur / 450 ) ) );
            mod_per_bonus( -( dur > 4000 ? 10 : int( dur / 400 ) ) );
        }
    } else if( id == effect_attention ) {
        if( one_in( 100000 / dur ) && one_in( 100000 / dur ) && one_in( 250 ) ) {
            tripoint dest( 0, 0, posz() );
            int tries = 0;
            do {
                dest.x = posx() + rng( -4, 4 );
                dest.y = posy() + rng( -4, 4 );
                tries++;
            } while( ( dest == pos() || g->mon_at( dest ) != -1 ) && tries < 10 );
            if( tries < 10 ) {
                if( g->m.impassable( dest ) ) {
                    g->m.make_rubble( dest, f_rubble_rock, true );
                }
                MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup(
                                                       mongroup_id( "GROUP_NETHER" ) );
                g->summon_mon( spawn_details.name, dest );
                if( g->u.sees( dest ) ) {
                    g->cancel_activity_query( _( "A monster appears nearby!" ) );
                    add_msg_if_player( m_warning, _( "A portal opens nearby, and a monster crawls through!" ) );
                }
                it.mult_duration( .25 );
            }
        }
    } else if( id == effect_meth ) {
        if( intense == 1 ) {
            add_miss_reason( _( "The bees have started escaping your teeth." ), 2 );
            if( one_in( 150 ) ) {
                add_msg_if_player( m_bad, _( "You feel paranoid.  They're watching you." ) );
                mod_pain( 1 );
                mod_fatigue( dice( 1, 6 ) );
            } else if( one_in( 500 ) ) {
                add_msg_if_player( m_bad,
                                   _( "You feel like you need less teeth.  You pull one out, and it is rotten to the core." ) );
                mod_pain( 1 );
            } else if( one_in( 500 ) ) {
                add_msg_if_player( m_bad, _( "You notice a large abscess.  You pick at it." ) );
                body_part itch = random_body_part( true );
                add_effect( effect_formication, 600, itch );
                mod_pain( 1 );
            } else if( one_in( 500 ) ) {
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
        if( dur > 6000 ) {
            // 20 teles (no decay; in practice at least 21)
            if( one_in( 1000 - ( ( dur - 6000 ) / 10 ) ) ) {
                if( !is_npc() ) {
                    add_msg( _( "Glowing lights surround you, and you teleport." ) );
                    add_memorial_log( pgettext( "memorial_male", "Spontaneous teleport." ),
                                      pgettext( "memorial_female", "Spontaneous teleport." ) );
                }
                g->teleport();
                if( one_in( 10 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0 );
                }
            }
            if( one_in( 1200 - ( ( dur - 6000 ) / 5 ) ) && one_in( 20 ) ) {
                if( !is_npc() ) {
                    add_msg( m_bad, _( "You pass out." ) );
                }
                fall_asleep( 1200 );
                if( one_in( 6 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0 );
                }
            }
        }
        if( dur > 3600 ) {
            // 12 teles
            if( one_in( 4000 - int( .25 * ( dur - 3600 ) ) ) ) {
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
                } while( ( ( x == posx() && y == posy() ) || g->mon_at( dest ) != -1 ) );
                if( tries < 10 ) {
                    if( g->m.impassable( x, y ) ) {
                        g->m.make_rubble( tripoint( x, y, posz() ), f_rubble_rock, true );
                    }
                    MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup(
                                                           mongroup_id( "GROUP_NETHER" ) );
                    g->summon_mon( spawn_details.name, dest );
                    if( g->u.sees( x, y ) ) {
                        g->cancel_activity_query( _( "A monster appears nearby!" ) );
                        add_msg( m_warning, _( "A portal opens nearby, and a monster crawls through!" ) );
                    }
                    if( one_in( 2 ) ) {
                        // Set ourselves up for removal
                        it.set_duration( 0 );
                    }
                }
            }
            if( one_in( 3500 - int( .25 * ( dur - 3600 ) ) ) ) {
                add_msg_if_player( m_bad, _( "You shudder suddenly." ) );
                mutate();
                if( one_in( 4 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0 );
                }
            }
        }
        if( dur > 2400 ) {
            // 8 teleports
            if( one_in( 10000 - dur ) && !has_effect( effect_valium ) ) {
                add_effect( effect_shakes, rng( 40, 80 ) );
            }
            if( one_in( 12000 - dur ) ) {
                add_msg_if_player( m_bad, _( "Your vision is filled with bright lights..." ) );
                add_effect( effect_blind, rng( 10, 20 ) );
                if( one_in( 8 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0 );
                }
            }
            if( one_in( 5000 ) && !has_effect( effect_hallu ) ) {
                add_effect( effect_hallu, 3600 );
                if( one_in( 5 ) ) {
                    // Set ourselves up for removal
                    it.set_duration( 0 );
                }
            }
        }
        if( one_in( 4000 ) ) {
            add_msg_if_player( m_bad, _( "You're suddenly covered in ectoplasm." ) );
            add_effect( effect_boomered, 100 );
            if( one_in( 4 ) ) {
                // Set ourselves up for removal
                it.set_duration( 0 );
            }
        }
        if( one_in( 10000 ) ) {
            if( !has_trait( trait_id( "M_IMMUNE" ) ) ) {
                add_effect( effect_fungus, 1, num_bp, true );
            } else {
                add_msg_if_player( m_info, _( "We have many colonists awaiting passage." ) );
            }
            // Set ourselves up for removal
            it.set_duration( 0 );
        }
    } else if( id == effect_asthma ) {
        if( has_effect( effect_adrenaline ) || has_effect( effect_datura ) ) {
            add_msg_if_player( m_good, _( "Your asthma attack stops." ) );
            it.set_duration( 0 );
        } else if( dur > 1200 ) {
            add_msg_if_player( m_bad, _( "Your asthma overcomes you.\nYou asphyxiate." ) );
            if( is_player() ) {
                add_memorial_log( pgettext( "memorial_male", "Succumbed to an asthma attack." ),
                                  pgettext( "memorial_female", "Succumbed to an asthma attack." ) );
            }
            hurtall( 500, nullptr );
        } else if( dur > 700 ) {
            if( one_in( 20 ) ) {
                add_msg_if_player( m_bad, _( "You wheeze and gasp for air." ) );
            }
        }
    } else if( id == effect_stemcell_treatment ) {
        // slightly repair broken limbs. (also nonbroken limbs (unless they're too healthy))
        for( int i = 0; i < num_hp_parts; i++ ) {
            if( one_in( 6 ) ) {
                if( hp_cur[i] < rng( 0, 40 ) ) {
                    add_msg_if_player( m_good, _( "Your bones feel like rubber as they melt and remend." ) );
                    hp_cur[i] += rng( 1, 8 );
                } else if( hp_cur[i] > rng( 10, 2000 ) ) {
                    add_msg_if_player( m_bad, _( "Your bones feel like they're crumbling." ) );
                    hp_cur[i] -= rng( 0, 8 );
                }
            }
        }
    } else if( id == effect_brainworms ) {
        if( one_in( 256 ) ) {
            add_msg_if_player( m_bad, _( "Your head aches faintly." ) );
        }
        if( one_in( 1024 ) ) {
            mod_healthy_mod( -10, -100 );
            apply_damage( nullptr, bp_head, rng( 0, 1 ) );
            if( !has_effect( effect_visuals ) ) {
                add_msg_if_player( m_bad, _( "Your vision is getting fuzzy." ) );
                add_effect( effect_visuals, rng( 10, 600 ) );
            }
        }
        if( one_in( 4096 ) ) {
            mod_healthy_mod( -10, -100 );
            apply_damage( nullptr, bp_head, rng( 1, 2 ) );
            if( !has_effect( effect_blind ) ) {
                add_msg_if_player( m_bad, _( "Your vision goes black!" ) );
                add_effect( effect_blind, rng( 5, 20 ) );
            }
        }
    } else if( id == effect_tapeworm ) {
        if( one_in( 512 ) ) {
            add_msg_if_player( m_bad, _( "Your bowels ache." ) );
        }
    } else if( id == effect_bloodworms ) {
        if( one_in( 512 ) ) {
            add_msg_if_player( m_bad, _( "Your veins itch." ) );
        }
    } else if( id == effect_paincysts ) {
        if( one_in( 512 ) ) {
            add_msg_if_player( m_bad, _( "Your muscles feel like they're knotted and tired." ) );
        }
    } else if( id == effect_tetanus ) {
        if( one_in( 256 ) ) {
            add_msg_if_player( m_bad, _( "Your muscles are tight and sore." ) );
        }
        if( !has_effect( effect_valium ) ) {
            add_miss_reason( _( "Your muscles are locking up and you can't fight effectively." ), 4 );
            if( one_in( 512 ) ) {
                add_msg_if_player( m_bad, _( "Your muscles spasm." ) );
                add_effect( effect_downed, rng( 1, 4 ), num_bp, false, 0, true );
                add_effect( effect_stunned, rng( 1, 4 ) );
                if( one_in( 10 ) ) {
                    mod_pain( rng( 1, 10 ) );
                }
            }
        }
    } else if( id == effect_datura ) {
        if( dur > 1000 && focus_pool >= 1 && one_in( 4 ) ) {
            focus_pool--;
        }
        if( dur > 2000 && one_in( 8 ) && stim < 20 ) {
            stim++;
        }
        if( dur > 3000 && focus_pool >= 1 && one_in( 2 ) ) {
            focus_pool--;
        }
        if( dur > 4000 && one_in( 64 ) ) {
            mod_pain( rng( -1, -8 ) );
        }
        if( ( !has_effect( effect_hallu ) ) && ( dur > 5000 && one_in( 4 ) ) ) {
            add_effect( effect_hallu, 3600 );
        }
        if( dur > 6000 && one_in( 128 ) ) {
            mod_pain( rng( -3, -24 ) );
            if( dur > 8000 && one_in( 16 ) ) {
                add_msg_if_player( m_bad,
                                   _( "You're experiencing loss of basic motor skills and blurred vision.  Your mind recoils in horror, unable to communicate with your spinal column." ) );
                add_msg_if_player( m_bad, _( "You stagger and fall!" ) );
                add_effect( effect_downed, rng( 1, 4 ), num_bp, false, 0, true );
                if( one_in( 8 ) || x_in_y( vomit_mod(), 10 ) ) {
                    vomit();
                }
            }
        }
        if( dur > 7000 && focus_pool >= 1 ) {
            focus_pool--;
        }
        if( dur > 8000 && one_in( 256 ) ) {
            add_effect( effect_visuals, rng( 40, 200 ) );
            mod_pain( rng( -8, -40 ) );
        }
        if( dur > 12000 && one_in( 256 ) ) {
            add_msg_if_player( m_bad, _( "There's some kind of big machine in the sky." ) );
            add_effect( effect_visuals, rng( 80, 400 ) );
            if( one_in( 32 ) ) {
                add_msg_if_player( m_bad, _( "It's some kind of electric snake, coming right at you!" ) );
                mod_pain( rng( 4, 40 ) );
                vomit();
            }
        }
        if( dur > 14000 && one_in( 128 ) ) {
            add_msg_if_player( m_bad,
                               _( "Order us some golf shoes, otherwise we'll never get out of this place alive." ) );
            add_effect( effect_visuals, rng( 400, 2000 ) );
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

        if( dur > 18000 && one_in( MINUTES( 5 ) * 512 ) ) {
            if( !has_trait( trait_id( "NOPAIN" ) ) ) {
                add_msg_if_player( m_bad,
                                   _( "Your heart spasms painfully and stops, dragging you back to reality as you die." ) );
            } else {
                add_msg_if_player(
                    _( "You dissolve into beautiful paroxysms of energy.  Life fades from your nebulae and you are no more." ) );
            }
            add_memorial_log( pgettext( "memorial_male", "Died of datura overdose." ),
                              pgettext( "memorial_female", "Died of datura overdose." ) );
            hp_cur[hp_torso] = 0;
        }
    } else if( id == effect_grabbed ) {
        blocks_left -= 1;
        int zed_number = 0;
        for( auto &dest : g->m.points_in_radius( pos(), 1, 0 ) ) {
            if( g->mon_at( dest ) != -1 ) {
                zed_number ++;
            }
        }
        if( zed_number > 0 ) {
            add_effect( effect_grabbed, 2, bp_torso, false,
                        ( intense + zed_number ) / 2 ); //If intensity isn't pass the cap, average it with # of zeds
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
        if( dur % 10 == 0 )  {
            int recover_factor = 100;
            if( has_effect( effect_recover ) ) {
                recover_factor -= get_effect_dur( effect_recover ) / 600;
            }
            if( has_trait( trait_id( "INFRESIST" ) ) ) {
                recover_factor += 200;
            }
            recover_factor += get_healthy() / 10;

            if( x_in_y( recover_factor, 108000 ) ) {
                //~ %s is bodypart name.
                add_msg_if_player( m_good, _( "Your %s wound begins to feel better." ),
                                   body_part_name( bp ).c_str() );
                // Set ourselves up for removal
                it.set_duration( 0 );
                recovered = true;
            }
        }
        if( !recovered ) {
            // Move up to infection
            if( dur > 3600 ) {
                add_effect( effect_infected, 1, bp, true );
                // Set ourselves up for removal
                it.set_duration( 0 );
            } else {
                it.mod_duration( 1 );
            }
        }
    } else if( id == effect_infected ) {
        bool recovered = false;
        // Recovery chance, use binomial distributions if balancing here.
        // See "bite" for balancing notes on this.
        if( dur % 10 == 0 )  {
            int recover_factor = 100;
            if( has_effect( effect_recover ) ) {
                recover_factor -= get_effect_dur( effect_recover ) / 600;
            }
            if( has_trait( trait_id( "INFRESIST" ) ) ) {
                recover_factor += 200;
            }
            recover_factor += get_healthy() / 10;

            if( x_in_y( recover_factor, 864000 ) ) {
                //~ %s is bodypart name.
                add_msg_if_player( m_good, _( "Your %s wound begins to feel better." ),
                                   body_part_name( bp ).c_str() );
                add_effect( effect_recover, 4 * dur );
                // Set ourselves up for removal
                it.set_duration( 0 );
                recovered = true;
            }
        }
        if( !recovered ) {
            // Death happens
            if( dur > 14400 ) {
                add_msg_if_player( m_bad, _( "You succumb to the infection." ) );
                add_memorial_log( pgettext( "memorial_male", "Succumbed to the infection." ),
                                  pgettext( "memorial_female", "Succumbed to the infection." ) );
                hurtall( 500, nullptr );
            }
            it.mod_duration( 1 );
        }
    } else if( id == effect_lying_down ) {
        set_moves( 0 );
        if( can_sleep() ) {
            add_msg_if_player( _( "You fall asleep." ) );
            // Communicate to the player that he is using items on the floor
            std::string item_name = is_snuggling();
            if( item_name == "many" ) {
                if( one_in( 15 ) ) {
                    add_msg_if_player( _( "You nestle your pile of clothes for warmth." ) );
                } else {
                    add_msg_if_player( _( "You use your pile of clothes for warmth." ) );
                }
            } else if( item_name != "nothing" ) {
                if( one_in( 15 ) ) {
                    add_msg_if_player( _( "You snuggle your %s to keep warm." ), item_name.c_str() );
                } else {
                    add_msg_if_player( _( "You use your %s to keep warm." ), item_name.c_str() );
                }
            }
            if( has_active_mutation( trait_id( "HIBERNATE" ) ) && get_hunger() < -60 ) {
                add_memorial_log( pgettext( "memorial_male", "Entered hibernation." ),
                                  pgettext( "memorial_female", "Entered hibernation." ) );
                // 10 days' worth of round-the-clock Snooze.  Cata seasons default to 14 days.
                fall_asleep( 144000 );
                // If you're not fatigued enough for 10 days, you won't sleep the whole thing.
                // In practice, the fatigue from filling the tank from (no msg) to Time For Bed
                // will last about 8 days.
            }

            fall_asleep( 6000 ); //10 hours, default max sleep time.
            // Set ourselves up for removal
            it.set_duration( 0 );
        }
        if( dur == 1 && !sleeping ) {
            add_msg_if_player( _( "You try to sleep, but can't..." ) );
        }
    } else if( id == effect_sleep ) {
        set_moves( 0 );
#ifdef TILES
        if( is_player() && calendar::once_every( MINUTES( 10 ) ) ) {
            SDL_PumpEvents();
        }
#endif // TILES

        if( intense < 24 ) {
            it.mod_intensity( 1 );
        } else if( intense < 1 ) {
            it.set_intensity( 1 );
        }

        if( get_fatigue() < -25 && it.get_duration() > 30 ) {
            it.set_duration( dice( 3, 10 ) );
        }

        if( get_fatigue() <= 0 && get_fatigue() > -20 ) {
            mod_fatigue( -25 );
            add_msg_if_player( m_good, _( "You feel well rested." ) );
            it.set_duration( dice( 3, 100 ) );
        }

        // TODO: Move this to update_needs when NPCs can mutate
        if( calendar::once_every( MINUTES( 10 ) ) && has_trait( trait_id( "CHLOROMORPH" ) ) &&
            g->is_in_sunlight( pos() ) ) {
            // Hunger and thirst fall before your Chloromorphic physiology!
            if( get_hunger() >= -30 ) {
                mod_hunger( -5 );
            }
            if( get_thirst() >= -30 ) {
                mod_thirst( -5 );
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
            if( ( int( calendar::turn ) % ( 3600 / strength ) == 0 ) && one_in( 3 ) ) {
                // Select a dream
                std::string dream = get_category_dream( highcat, strength );
                if( !dream.empty() ) {
                    add_msg_if_player( "%s", dream.c_str() );
                }
                // Mycus folks upgrade in their sleep.
                if( has_trait( trait_id( "THRESH_MYCUS" ) ) ) {
                    if( one_in( 8 ) ) {
                        mutate_category( "MUTCAT_MYCUS" );
                        mod_hunger( 10 );
                        mod_thirst( 10 );
                        mod_fatigue( 5 );
                    }
                }
            }
        }

        bool woke_up = false;
        int tirednessVal = rng( 5, 200 ) + rng( 0, abs( get_fatigue() * 2 * 5 ) );
        if( !is_blind() ) {
            if( has_trait( trait_id( "HEAVYSLEEPER2" ) ) && !has_trait( trait_id( "HIBERNATE" ) ) ) {
                // So you can too sleep through noon
                if( ( tirednessVal * 1.25 ) < g->m.ambient_light_at( pos() ) && ( get_fatigue() < 10 ||
                        one_in( get_fatigue() / 2 ) ) ) {
                    add_msg_if_player( _( "It's too bright to sleep." ) );
                    // Set ourselves up for removal
                    it.set_duration( 0 );
                    woke_up = true;
                }
                // Ursine hibernators would likely do so indoors.  Plants, though, might be in the sun.
            } else if( has_trait( trait_id( "HIBERNATE" ) ) ) {
                if( ( tirednessVal * 5 ) < g->m.ambient_light_at( pos() ) && ( get_fatigue() < 10 ||
                        one_in( get_fatigue() / 2 ) ) ) {
                    add_msg_if_player( _( "It's too bright to sleep." ) );
                    // Set ourselves up for removal
                    it.set_duration( 0 );
                    woke_up = true;
                }
            } else if( tirednessVal < g->m.ambient_light_at( pos() ) && ( get_fatigue() < 10 ||
                       one_in( get_fatigue() / 2 ) ) ) {
                add_msg_if_player( _( "It's too bright to sleep." ) );
                // Set ourselves up for removal
                it.set_duration( 0 );
                woke_up = true;
            }
        }

        // Have we already woken up?
        if( !woke_up ) {
            // Cold or heat may wake you up.
            // Player will sleep through cold or heat if fatigued enough
            for( int i = 0 ; i < num_bp ; i++ ) {
                if( temp_cur[i] < BODYTEMP_VERY_COLD - get_fatigue() / 2 ) {
                    if( one_in( 5000 ) ) {
                        add_msg_if_player( _( "You toss and turn trying to keep warm." ) );
                    }
                    if( temp_cur[i] < BODYTEMP_FREEZING - get_fatigue() / 2 ||
                        one_in( temp_cur[i] + 5000 ) ) {
                        add_msg_if_player( m_bad, _( "It's too cold to sleep." ) );
                        // Set ourselves up for removal
                        it.set_duration( 0 );
                        woke_up = true;
                        break;
                    }
                } else if( temp_cur[i] > BODYTEMP_VERY_HOT + get_fatigue() / 2 ) {
                    if( one_in( 5000 ) ) {
                        add_msg_if_player( _( "You toss and turn in the heat." ) );
                    }
                    if( temp_cur[i] > BODYTEMP_SCORCHING + get_fatigue() / 2 ||
                        one_in( 15000 - temp_cur[i] ) ) {
                        add_msg_if_player( m_bad, _( "It's too hot to sleep." ) );
                        // Set ourselves up for removal
                        it.set_duration( 0 );
                        woke_up = true;
                        break;
                    }
                }
            }
        }

        // A bit of a hack: check if we are about to wake up for any reason,
        // including regular timing out of sleep
        if( ( it.get_duration() == 1 || woke_up ) && calendar::turn - start > HOURS( 2 ) ) {
            print_health();
        }
    } else if( id == effect_alarm_clock ) {
        if( has_effect( effect_sleep ) ) {
            if( dur == 1 ) {
                if( has_bionic( "bio_watch" ) ) {
                    // Normal alarm is volume 12, tested against (2/3/6)d15 for
                    // normal/HEAVYSLEEPER/HEAVYSLEEPER2.
                    //
                    // It's much harder to ignore an alarm inside your own skull,
                    // so this uses an effective volume of 20.
                    const int volume = 20;
                    if( ( !( has_trait( trait_id( "HEAVYSLEEPER" ) ) || has_trait( trait_id( "HEAVYSLEEPER2" ) ) ) &&
                          dice( 2, 15 ) < volume ) ||
                        ( has_trait( trait_id( "HEAVYSLEEPER" ) ) && dice( 3, 15 ) < volume ) ||
                        ( has_trait( trait_id( "HEAVYSLEEPER2" ) ) && dice( 6, 15 ) < volume ) ) {
                        wake_up();
                        add_msg_if_player( _( "Your internal chronometer wakes you up." ) );
                    } else {
                        // 10 minute cyber-snooze
                        it.mod_duration( 100 );
                    }
                } else {
                    sounds::sound( pos(), 16, _( "beep-beep-beep!" ) );
                    if( !can_hear( pos(), 16 ) ) {
                        // 10 minute automatic snooze
                        it.mod_duration( 100 );
                    } else {
                        add_msg_if_player( _( "You turn off your alarm-clock." ) );
                    }
                }
            }
        }
    } else if( id == effect_mending ) {
        // @todo Remove this and encapsulate hp_cur instead
        if( hp_cur[bp_to_hp( bp )] > 0 ) {
            it.set_duration( 0 );
        }
    } else if( id == effect_disabled ) {
        // @todo Remove this and encapsulate hp_cur instead
        if( hp_cur[bp_to_hp( bp )] > 0 ) {
            // Just unpause, in case someone added it as a temporary effect (numbing poison etc.)
            it.unpause_effect();
        }
    }
}
