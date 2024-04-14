#include <algorithm>
#include <array>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

#include "activity_handlers.h"
#include "activity_type.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "creature_tracker.h"
#include "damage.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field_type.h"
#include "fungal_effects.h"
#include "game.h"
#include "input.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "player_activity.h"
#include "rng.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "teleport.h"
#include "translations.h"
#include "uistate.h"
#include "units.h"
#include "vitamin.h"
#include "weather.h"

static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );

static const bionic_id bio_sleep_shutdown( "bio_sleep_shutdown" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_anemia( "anemia" );
static const efftype_id effect_antibiotic( "antibiotic" );
static const efftype_id effect_antifungal( "antifungal" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_attention( "attention" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_cold( "cold" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_disabled( "disabled" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_evil( "evil" );
static const efftype_id effect_fake_common_cold( "fake_common_cold" );
static const efftype_id effect_fake_flu( "fake_flu" );
static const efftype_id effect_formication( "formication" );
static const efftype_id effect_frostbite( "frostbite" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_hot( "hot" );
static const efftype_id effect_hypovolemia( "hypovolemia" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_motor_seizure( "motor_seizure" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_panacea( "panacea" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_rat( "rat" );
static const efftype_id effect_recover( "recover" );
static const efftype_id effect_redcells_anemia( "redcells_anemia" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
static const efftype_id effect_spores( "spores" );
static const efftype_id effect_strong_antibiotic( "strong_antibiotic" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_teleglow( "teleglow" );
static const efftype_id effect_tetanus( "tetanus" );
static const efftype_id effect_tindrift( "tindrift" );
static const efftype_id effect_toxin_buildup( "toxin_buildup" );
static const efftype_id effect_valium( "valium" );
static const efftype_id effect_visuals( "visuals" );
static const efftype_id effect_weak_antibiotic( "weak_antibiotic" );
static const efftype_id effect_winded( "winded" );

static const furn_str_id furn_f_rubble_rock( "f_rubble_rock" );

static const json_character_flag json_flag_ALARMCLOCK( "ALARMCLOCK" );
static const json_character_flag json_flag_BIONIC_LIMB( "BIONIC_LIMB" );
static const json_character_flag json_flag_BLEEDSLOW( "BLEEDSLOW" );
static const json_character_flag json_flag_BLEEDSLOW2( "BLEEDSLOW2" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );
static const json_character_flag json_flag_SEESLEEP( "SEESLEEP" );

static const mongroup_id GROUP_NETHER( "GROUP_NETHER" );

static const mtype_id mon_dermatik_larva( "mon_dermatik_larva" );

static const mutation_category_id mutation_category_MYCUS( "MYCUS" );
static const mutation_category_id mutation_category_RAT( "RAT" );
static const mutation_category_id mutation_category_TROGLOBITE( "TROGLOBITE" );

static const proficiency_id proficiency_prof_wound_care( "prof_wound_care" );
static const proficiency_id proficiency_prof_wound_care_expert( "prof_wound_care_expert" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );
static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_INFRESIST( "INFRESIST" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );

static const vitamin_id vitamin_blood( "blood" );
static const vitamin_id vitamin_calcium( "calcium" );
static const vitamin_id vitamin_iron( "iron" );
static const vitamin_id vitamin_redcells( "redcells" );

static void eff_fun_onfire( Character &u, effect &it )
{
    const int intense = it.get_intensity();
    u.deal_damage( nullptr, it.get_bp(), damage_instance( STATIC( damage_type_id( "heat" ) ),
                   rng( intense, intense * 2 ) ) );
}
static void eff_fun_spores( Character &u, effect &it )
{
    // Equivalent to X in 150000 + health * 100
    const int intense = it.get_intensity();
    if( ( !u.has_trait( trait_M_IMMUNE ) ) && ( one_in( 100 ) &&
            x_in_y( intense, 900 + u.get_lifestyle() * 0.6 ) ) ) {
        u.add_effect( effect_fungus, 1_turns, true );
    }
}
static void eff_fun_antifungal( Character &u, effect & )
{
    // antifungal drugs are deadly poison for Marloss people
    if( u.has_trait( trait_THRESH_MYCUS ) && one_in( 30 ) ) {
        if( one_in( 10 ) ) {
            u.add_msg_player_or_npc( m_bad, _( "Something burns you from the inside." ),
                                     _( "<npcname> shivers from pain." ) );
        }
        u.mod_pain( 1 );
        // not using u.get_random_body_part() as it is weighted & not fully random
        std::vector<bodypart_id> bparts = u.get_all_body_parts( get_body_part_flags::only_main );
        bodypart_id random_bpart = bparts[ rng( 0, bparts.size() - 1 ) ];
        u.apply_damage( nullptr, random_bpart, 1 );
    }
}
static void eff_fun_fake_common_cold( Character &u, effect & )
{
    if( calendar::once_every( time_duration::from_seconds( rng( 30, 300 ) ) ) && one_in( 2 ) ) {
        u.cough( true );
    }

    avatar &you = get_avatar(); // No NPCs for now.
    if( rl_dist( u.pos(), you.pos() ) <= 1 ) {
        you.get_sick( false );
    }
}
static void eff_fun_fake_flu( Character &u, effect & )
{
    if( calendar::once_every( time_duration::from_seconds( rng( 30, 300 ) ) ) && one_in( 2 ) ) {
        u.cough( true );
    }

    avatar &you = get_avatar(); // No NPCs for now.
    if( rl_dist( u.pos(), you.pos() ) <= 1 ) {
        you.get_sick( true );
    }
}
static void eff_fun_fungus( Character &u, effect &it )
{
    const int intense = it.get_intensity();
    const bool resists = u.resists_effect( it );
    const int bonus = u.get_lifestyle() / 10 + ( resists ? 100 : 0 );

    // clock the progress
    // hard reverse the clock if you resist fungus
    if( resists ) {
        it.mod_duration( -5_turns );
    } else {
        it.mod_duration( 1_turns );
    }

    switch( intense ) {
        case 1:
            // 0-6 hours symptoms
            if( one_in( 960 + bonus * 6 ) ) {
                u.cough( true );
            }
            if( one_in( 600 + bonus * 6 ) ) {
                u.add_msg_if_player( m_warning, _( "You feel nauseous." ) );
            }
            if( one_in( 600 + bonus * 6 ) ) {
                u.add_msg_if_player( m_warning, _( "You smell and taste mushrooms." ) );
            }
            break;
        case 2:
            // 6-12 hours of worse symptoms
            if( one_in( 3600 + bonus * 18 ) ) {
                u.add_msg_if_player( m_bad,  _( "You spasm suddenly!" ) );
                u.mod_moves( -to_moves<int>( 1_seconds ) );
                u.apply_damage( nullptr, bodypart_id( "torso" ), resists ? rng( 1, 5 ) : 5 );
            }
            if( x_in_y( u.vomit_mod(), ( 4800 + bonus * 24 ) ) || one_in( 12000 + bonus * 60 ) ) {
                u.add_msg_player_or_npc( m_bad, _( "You vomit a thick, gray goop." ),
                                         _( "<npcname> vomits a thick, gray goop." ) );

                const int awfulness = rng( 0, resists ? rng( 1, 70 ) : 70 );
                u.set_moves( -to_moves<int>( 2_seconds ) );
                u.mod_hunger( awfulness );
                u.mod_thirst( awfulness );
                ///\EFFECT_STR decreases damage taken by fungus effect
                u.apply_damage( nullptr, bodypart_id( "torso" ), awfulness / std::max( u.str_cur,
                                1 ) ); // can't be healthy
            }
            break;
        case 3:
            // Permanent symptoms, 12+ hours
            if( one_in( 6000 + bonus * 48 ) ) {
                u.add_msg_player_or_npc( m_bad,  _( "You vomit thousands of live spores!" ),
                                         _( "<npcname> vomits thousands of live spores!" ) );

                u.set_moves( -to_moves<int>( 5_seconds ) );
                map &here = get_map();
                fungal_effects fe;
                for( const tripoint &sporep : here.points_in_radius( u.pos(), 1 ) ) {
                    if( sporep == u.pos() ) {
                        continue;
                    }
                    fe.fungalize( sporep, &u, 0.25 );
                }
                // We're fucked
            } else if( one_in( 36000 + bonus * 240 ) ) {
                // determine if we have arms to channel the fungal stalks out of
                bool has_arms_outlet = true;
                for( const bodypart_id &part : u.get_all_body_parts_of_type( body_part_type::type::arm ) ) {
                    if( part->has_flag( json_flag_BIONIC_LIMB ) ) {
                        has_arms_outlet = false;
                    }
                }
                if( has_arms_outlet ) {
                    if( u.is_limb_broken( bodypart_id( "arm_l" ) ) || u.is_limb_broken( bodypart_id( "arm_r" ) ) ) {
                        if( u.is_limb_broken( bodypart_id( "arm_l" ) ) && u.is_limb_broken( bodypart_id( "arm_r" ) ) ) {
                            u.add_msg_player_or_npc( m_bad,
                                                     _( "The flesh on your broken arms bulges.  Fungus stalks burst through!" ),
                                                     _( "<npcname>'s broken arms bulge.  Fungus stalks burst out of the bulges!" ) );
                        } else {
                            u.add_msg_player_or_npc( m_bad,
                                                     _( "The flesh on your broken and unbroken arms bulge.  Fungus stalks burst through!" ),
                                                     _( "<npcname>'s arms bulge.  Fungus stalks burst out of the bulges!" ) );
                        }
                    } else {
                        u.add_msg_player_or_npc( m_bad, _( "Your hands bulge.  Fungus stalks burst through the bulge!" ),
                                                 _( "<npcname>'s hands bulge.  Fungus stalks burst through the bulge!" ) );
                    }
                    u.apply_damage( nullptr, bodypart_id( "arm_l" ), 999 );
                    u.apply_damage( nullptr, bodypart_id( "arm_r" ), 999 );
                } else {
                    // we don't have viable arms, so the fungus does a little chestbursting
                    u.add_msg_player_or_npc( m_bad,
                                             _( "Your chest bulges, and fungal stalks burst out of your skin!" ),
                                             _( "<npcname>'s chest bulges.  Fungus stalks burst out of their skin!" ) );
                    u.apply_damage( nullptr, bodypart_id( "torso" ), 40 );
                    u.add_effect( effect_bleed, 30_minutes, bodypart_id( "torso" ) );
                }
            }
            break;
    }
}
static void eff_fun_rat( Character &u, effect &it )
{
    const int dur = to_turns<int>( it.get_duration() );
    it.set_intensity( dur / 10 );
    if( rng( 0, 100 ) < dur / 10 ) {
        if( !one_in( 5 ) ) {
            u.mutate_category( mutation_category_RAT, false, true );
            it.mult_duration( .2 );
        } else {
            u.mutate_category( mutation_category_TROGLOBITE, false, true );
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
static void eff_fun_bleed( Character &u, effect &it )
{
    // Presuming that during the first-aid process you're putting pressure
    // on the wound or otherwise suppressing the flow. (Kits contain either
    // QuikClot or bandages per the recipe.)
    const int intense = it.get_intensity();
    // tourniquet reduces effective bleeding by 2/3 but doesn't modify the effect's intensity
    // proficiency improves that factor to 3/4 and 4/5 respectively
    bool tourniquet = u.worn_with_flag( STATIC( flag_id( "TOURNIQUET" ) ),  it.get_bp() );
    int prof_bonus = 3;
    prof_bonus = u.has_proficiency( proficiency_prof_wound_care ) ? prof_bonus + 1 : prof_bonus;
    prof_bonus = u.has_proficiency( proficiency_prof_wound_care_expert ) ? prof_bonus + 1 : prof_bonus;

    if( ( !tourniquet || one_in( prof_bonus ) ) && u.activity.id() != ACT_FIRSTAID ) {
        // Prolonged hemorrhage is a significant risk for developing anemia
        if( u.has_flag( json_flag_BLEEDSLOW2 ) ) {
            u.vitamin_mod( vitamin_redcells, -( intense / 3 ) );
            u.vitamin_mod( vitamin_blood, -( intense / 3 ) );
        } else if( u.has_flag( json_flag_BLEEDSLOW ) ) {
            u.vitamin_mod( vitamin_redcells, -( intense / 1.5 ) );
            u.vitamin_mod( vitamin_blood, -( intense / 1.5 ) );
        } else {
            u.vitamin_mod( vitamin_redcells, -intense );
            u.vitamin_mod( vitamin_blood, -intense );
        }
        if( one_in( 400 / intense ) ) {
            u.mod_pain( 1 );
        }
        if( one_in( 120 / intense ) ) {
            static const translation blood_str = !u.has_trait( trait_ACIDBLOOD ) ?
                                                 to_translation( "bleed_message", "Blood" ) : to_translation( "bleed_message", "Acid" );
            // the numerical values here coincide with the intensity thresholds at which the name of the effect changes
            // i.e. 0-5 intensity is displayed as "Minor Bleeding", 11-20 intensity is displayed as "Bad Bleeding", etc
            static const std::map<int, translation> intensity_strings = {
                { 0, to_translation( "%1s drips from your %2s." ) },
                { 6, to_translation( "%1s leaks from your %2s." ) },
                { 11, to_translation( "%1s flows from your %2s." ) },
                { 21, to_translation( "%1s pours from your %2s!" ) },
                { 31, to_translation( "%1s gushes from your %2s!" ) }
            };
            translation suffer_string = intensity_strings.at( 0 );
            // iterate through intensity levels after the first, stopping before we reach one higher than the actual intensity
            for( auto iter = next( intensity_strings.begin() ); iter != intensity_strings.end(); ++iter ) {
                if( intense < iter->first ) {
                    break;
                }
                suffer_string = iter->second;
            }
            u.bleed();
            bodypart_id bp = it.get_bp();
            // piece together the final displayed message here instead of inline, for readability's sake
            // format the chosen string with the relevant variables to make it human-readable, then translate everything we have so far
            // we maintain a generic part-less fallback just in case the effect is added without a target body part, in order to avoid crashes
            const std::string final_message = bp != bodypart_str_id::NULL_ID() ? string_format(
                                                  suffer_string,
                                                  blood_str, body_part_name( bp ) ) : _( "You lose some blood." );
            // display the final message
            u.add_msg_player_or_npc( m_bad,
                                     final_message,
                                     _( "<npcname> loses some blood." ) );
            if( u.is_avatar() && one_in( 10 ) && it.get_duration() > 1_turns ) {
                bodypart_id bp_id = u.most_staunchable_bp();
                if( bp_id == bp ) {
                    if( u.controlling_vehicle || u.is_armed() ) {
                        u.add_msg_if_player( m_warning,
                                             _( "You could reduce the bleeding by emptying your hands and pausing to apply pressure." ) );
                    } else {
                        u.add_msg_if_player( m_warning,
                                             _( "You could reduce the bleeding by pausing to apply pressure." ) );
                    }

                }
            }
        }
    }
}
static void eff_fun_hallu( Character &u, effect &it )
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
                    translate_marker( "\"Of course… it's all fractals!\"" ),
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
        if( u.is_avatar() && one_in( 50 ) ) {
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
    translation msg;
    int msg_chance;
    translation miss_msg;

    temperature_effect( int sp, int dp, int ip, int pp, const translation &ms, int mc,
                        const translation &mm ) :
        str_pen( sp ), dex_pen( dp ), int_pen( ip ), per_pen( pp ), msg( ms ),
        msg_chance( mc ), miss_msg( mm ) {
    }

    void apply( Character &u ) const {
        if( str_pen > 0 ) {
            u.mod_str_bonus( -str_pen );
        }
        if( dex_pen > 0 ) {
            u.mod_dex_bonus( -dex_pen );
            u.add_miss_reason( miss_msg.translated(), dex_pen );
        }
        if( int_pen > 0 ) {
            u.mod_int_bonus( -int_pen );
        }
        if( per_pen > 0 ) {
            u.mod_per_bonus( -per_pen );
        }
        if( !msg.empty() && !u.has_effect( effect_sleep ) && one_in( msg_chance ) ) {
            u.add_msg_if_player( m_warning, "%s", msg.translated() );
        }
    }
};

static void eff_fun_cold( Character &u, effect &it )
{
    // { body_part, intensity }, { str_pen, dex_pen, int_pen, per_pen, msg, msg_chance, miss_msg }
    static const std::map<std::pair<bodypart_str_id, int>, temperature_effect> effs = {{
            { { body_part_head, 3 }, { 0, 0, 3, 0, to_translation( "Your thoughts are unclear." ), 2400, translation() } },
            { { body_part_head, 2 }, { 0, 0, 1, 0, translation(), 0, translation() } },
            { { body_part_mouth, 3 }, { 0, 0, 0, 3, to_translation( "Your face is stiff from the cold." ), 2400, translation() } },
            { { body_part_mouth, 2 }, { 0, 0, 0, 1, translation(), 0, translation() } },
            { { body_part_torso, 3 }, { 0, 4, 0, 0, to_translation( "Your torso is freezing cold.  You should put on a few more layers." ), 400, to_translation( "You quiver from the cold." ) } },
            { { body_part_torso, 2 }, { 0, 2, 0, 0, translation(), 0, to_translation( "Your shivering makes you unsteady." ) } },
            { { body_part_arm_l, 3 }, { 0, 2, 0, 0, to_translation( "Your left arm is shivering." ), 4800, to_translation( "Your left arm trembles from the cold." ) } },
            { { body_part_arm_l, 2 }, { 0, 1, 0, 0, to_translation( "Your left arm is shivering." ), 4800, to_translation( "Your left arm trembles from the cold." ) } },
            { { body_part_arm_r, 3 }, { 0, 2, 0, 0, to_translation( "Your right arm is shivering." ), 4800, to_translation( "Your right arm trembles from the cold." ) } },
            { { body_part_arm_r, 2 }, { 0, 1, 0, 0, to_translation( "Your right arm is shivering." ), 4800, to_translation( "Your right arm trembles from the cold." ) } },
            { { body_part_hand_l, 3 }, { 0, 2, 0, 0, to_translation( "Your left hand feels like ice." ), 4800, to_translation( "Your left hand quivers in the cold." ) } },
            { { body_part_hand_l, 2 }, { 0, 1, 0, 0, to_translation( "Your left hand feels like ice." ), 4800, to_translation( "Your left hand quivers in the cold." ) } },
            { { body_part_hand_r, 3 }, { 0, 2, 0, 0, to_translation( "Your right hand feels like ice." ), 4800, to_translation( "Your right hand quivers in the cold." ) } },
            { { body_part_hand_r, 2 }, { 0, 1, 0, 0, to_translation( "Your right hand feels like ice." ), 4800, to_translation( "Your right hand quivers in the cold." ) } },
            { { body_part_leg_l, 3 }, { 2, 2, 0, 0, to_translation( "Your left leg trembles against the relentless cold." ), 4800, to_translation( "Your legs uncontrollably shake from the cold." ) } },
            { { body_part_leg_l, 2 }, { 1, 1, 0, 0, to_translation( "Your left leg trembles against the relentless cold." ), 4800, to_translation( "Your legs uncontrollably shake from the cold." ) } },
            { { body_part_leg_r, 3 }, { 2, 2, 0, 0, to_translation( "Your right leg trembles against the relentless cold." ), 4800, to_translation( "Your legs uncontrollably shake from the cold." ) } },
            { { body_part_leg_r, 2 }, { 1, 1, 0, 0, to_translation( "Your right leg trembles against the relentless cold." ), 4800, to_translation( "Your legs uncontrollably shake from the cold." ) } },
            { { body_part_foot_l, 3 }, { 2, 2, 0, 0, to_translation( "Your left foot feels frigid." ), 4800, to_translation( "Your left foot is as nimble as a block of ice." ) } },
            { { body_part_foot_l, 2 }, { 1, 1, 0, 0, to_translation( "Your left foot feels frigid." ), 4800, to_translation( "Your freezing left foot messes up your balance." ) } },
            { { body_part_foot_r, 3 }, { 2, 2, 0, 0, to_translation( "Your right foot feels frigid." ), 4800, to_translation( "Your right foot is as nimble as a block of ice." ) } },
            { { body_part_foot_r, 2 }, { 1, 1, 0, 0, to_translation( "Your right foot feels frigid." ), 4800, to_translation( "Your freezing right foot messes up your balance." ) } },
        }
    };
    const auto iter = effs.find( { it.get_bp().id(), it.get_intensity() } );
    if( iter != effs.end() ) {
        iter->second.apply( u );
    }
}

static void eff_fun_hot( Character &u, effect &it )
{
    // { body_part, intensity }, { str_pen, dex_pen, int_pen, per_pen, msg, msg_chance, miss_msg }
    static const std::map<std::pair<bodypart_str_id, int>, temperature_effect> effs = {{
            { { body_part_head, 3 }, { 0, 0, 0, 0, to_translation( "Your head is pounding from the heat." ), 2400, translation() } },
            { { body_part_head, 2 }, { 0, 0, 0, 0, translation(), 0, translation() } },
            { { body_part_torso, 3 }, { 2, 0, 0, 0, to_translation( "You are sweating profusely." ), 2400, translation() } },
            { { body_part_torso, 2 }, { 1, 0, 0, 0, translation(), 0, translation() } },
            { { body_part_hand_l, 3 }, { 0, 2, 0, 0, translation(), 0, to_translation( "Your left hand's too sweaty to grip well." ) } },
            { { body_part_hand_l, 2 }, { 0, 1, 0, 0, translation(), 0, to_translation( "Your left hand's too sweaty to grip well." ) } },
            { { body_part_hand_r, 3 }, { 0, 2, 0, 0, translation(), 0, to_translation( "Your right hand's too sweaty to grip well." ) } },
            { { body_part_hand_r, 2 }, { 0, 1, 0, 0, translation(), 0, to_translation( "Your right hand's too sweaty to grip well." ) } },
            { { body_part_leg_l, 3 }, { 0, 0, 0, 0, to_translation( "Your left leg is cramping up." ), 4800, translation() } },
            { { body_part_leg_l, 2 }, { 0, 0, 0, 0, translation(), 0, translation() } },
            { { body_part_leg_r, 3 }, { 0, 0, 0, 0, to_translation( "Your right leg is cramping up." ), 4800, translation() } },
            { { body_part_leg_r, 2 }, { 0, 0, 0, 0, translation(), 0, translation() } },
            { { body_part_foot_l, 3 }, { 0, 0, 0, 0, to_translation( "Your left foot is swelling in the heat." ), 4800, translation() } },
            { { body_part_foot_l, 2 }, { 0, 0, 0, 0, translation(), 0, translation() } },
            { { body_part_foot_r, 3 }, { 0, 0, 0, 0, to_translation( "Your right foot is swelling in the heat." ), 4800, translation() } },
            { { body_part_foot_r, 2 }, { 0, 0, 0, 0, translation(), 0, translation() } },
        }
    };

    const bodypart_id bp = it.get_bp();
    const int intense = it.get_intensity();
    const auto iter = effs.find( { it.get_bp().id(), it.get_intensity() } );
    if( iter != effs.end() ) {
        iter->second.apply( u );
    }
    // Hothead effects are a special snowflake

    if( bp == bodypart_id( "head" ) && intense >= 2 ) {
        if( one_in( std::max( 25, std::min( 89500,
                                            90000 - units::to_legacy_bodypart_temp( u.get_part_temp_cur( bodypart_id( "head" ) ) ) ) ) ) ) {
            u.vomit();
        }
        if( !u.has_effect( effect_sleep ) && one_in( 2400 ) ) {
            u.add_msg_if_player( m_bad, _( "The heat is making you see things." ) );
        }
    }
}

static void eff_fun_frostbite( Character &u, effect &it )
{
    // { body_part, intensity }, { str_pen, dex_pen, int_pen, per_pen, msg, msg_chance, miss_msg }
    static const std::map<std::pair<bodypart_str_id, int>, temperature_effect> effs = {{
            { { body_part_hand_l, 2 }, { 0, 2, 0, 0, translation(), 0, to_translation( "You have trouble grasping with your numb fingers." ) } },
            { { body_part_hand_r, 2 }, { 0, 2, 0, 0, translation(), 0, to_translation( "You have trouble grasping with your numb fingers." ) } },
            { { body_part_foot_l, 2 }, { 0, 0, 0, 0, to_translation( "Your foot has gone numb." ), 4800, translation() } },
            { { body_part_foot_l, 1 }, { 0, 0, 0, 0, to_translation( "Your foot has gone numb." ), 4800, translation() } },
            { { body_part_foot_r, 2 }, { 0, 0, 0, 0, to_translation( "Your foot has gone numb." ), 4800, translation() } },
            { { body_part_foot_r, 1 }, { 0, 0, 0, 0, to_translation( "Your foot has gone numb." ), 4800, translation() } },
            { { body_part_mouth, 2 }, { 0, 0, 0, 3, to_translation( "Your face feels numb." ), 4800, translation() } },
            { { body_part_mouth, 1 }, { 0, 0, 0, 1, to_translation( "Your face feels numb." ), 4800, translation() } },
        }
    };
    const auto iter = effs.find( { it.get_bp().id(), it.get_intensity() } );
    if( iter != effs.end() ) {
        iter->second.apply( u );
    }
}

static void eff_fun_teleglow( Character &u, effect &it )
{
    // Default we get around 300 duration points per teleport (possibly more
    // depending on the source).
    // TODO: Include a chance to teleport to the nether realm.
    // TODO: This with regards to NPCS
    if( !u.is_avatar() ) {
        // NO, no teleporting around the player because an NPC has teleglow!
        return;
    }
    const time_duration dur = it.get_duration();
    Character &player_character = get_player_character();
    map &here = get_map();
    if( dur > 10_hours ) {
        // 20 teleports (no decay; in practice at least 21)
        if( one_in( 6000 - ( ( dur - 600_minutes ) / 1_minutes ) ) ) {
            if( !u.is_npc() ) {
                add_msg( _( "Glowing lights surround you, and you teleport." ) );
            }
            teleport::teleport( u );
            get_event_bus().send<event_type::teleglow_teleports>( u.getID() );
            if( one_in( 10 ) ) {
                // Set ourselves up for removal
                it.set_duration( 0_turns );
            }
        }
    }
    if( one_in( 7200 - ( dur - 360_minutes ) / 4_turns ) ) {
        //Spawn a tindalos rift via effect_tindrift rather than it being hard-coded to teleglow
        u.add_effect( effect_tindrift, 5_turns );

        if( one_in( 2 ) ) {
            // Set ourselves up for removal
            it.set_duration( 0_turns );
        }
    }
    if( one_in( 7200 - ( ( dur - 600_minutes ) / 30_seconds ) ) && one_in( 20 ) ) {
        u.add_msg_if_player( m_bad, _( "You pass out." ) );
        u.fall_asleep( 2_hours );
        if( one_in( 6 ) ) {
            // Set ourselves up for removal
            it.set_duration( 0_turns );
        }
    }
    if( dur > 6_hours ) {
        // 12 teleports
        if( one_in( 24000 - ( dur - 360_minutes ) / 4_turns ) ) {
            creature_tracker &creatures = get_creature_tracker();
            tripoint dest( 0, 0, u.posz() );
            int &x = dest.x;
            int &y = dest.y;
            int tries = 0;
            do {
                x = u.posx() + rng( -4, 4 );
                y = u.posy() + rng( -4, 4 );
                tries++;
                if( tries >= 10 ) {
                    break;
                }
            } while( creatures.creature_at( dest ) );
            if( tries < 10 ) {
                if( here.impassable( dest ) ) {
                    here.make_rubble( dest, furn_f_rubble_rock, true );
                }
                std::vector<MonsterGroupResult> spawn_details =
                    MonsterGroupManager::GetResultFromGroup( GROUP_NETHER );
                for( const MonsterGroupResult &mgr : spawn_details ) {
                    g->place_critter_at( mgr.name, dest );
                }
                if( uistate.distraction_hostile_spotted && player_character.sees( dest ) ) {
                    g->cancel_activity_or_ignore_query( distraction_type::hostile_spotted_far,
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
            u.add_msg_if_player( m_bad, _( "You shudder suddenly." ) );
            u.mutate();
            if( one_in( 4 ) ) {
                // Set ourselves up for removal
                it.set_duration( 0_turns );
            }
        }
    }
    if( dur > 4_hours ) {
        // 8 teleports
        if( one_turn_in( 1000_minutes - dur ) && !u.has_effect( effect_valium ) ) {
            u.add_effect( effect_shakes, rng( 4_minutes, 8_minutes ) );
        }
        if( one_turn_in( 1200_minutes - dur ) ) {
            u.add_msg_if_player( m_bad, _( "Your vision is filled with bright lights…" ) );
            u.add_effect( effect_blind, rng( 1_minutes, 2_minutes ) );
            if( one_in( 8 ) ) {
                // Set ourselves up for removal
                it.set_duration( 0_turns );
            }
        }
        if( one_in( 5000 ) && !u.has_effect( effect_hallu ) ) {
            u.add_effect( effect_hallu, 6_hours );
            if( one_in( 5 ) ) {
                // Set ourselves up for removal
                it.set_duration( 0_turns );
            }
        }
    }
    if( one_in( 4000 ) ) {
        u.add_msg_if_player( m_bad, _( "You're suddenly covered in ectoplasm." ) );
        u.add_effect( effect_boomered, 10_minutes );
        if( one_in( 4 ) ) {
            // Set ourselves up for removal
            it.set_duration( 0_turns );
        }
    }
    if( one_in( 10000 ) ) {
        if( !u.has_trait( trait_M_IMMUNE ) ) {
            u.add_effect( effect_fungus, 1_turns, true );
        } else {
            u.add_msg_if_player( m_info, _( "We have many colonists awaiting passage." ) );
        }
        // Set ourselves up for removal
        it.set_duration( 0_turns );
    }
}

static void eff_fun_datura( Character &u, effect &it )
{
    const time_duration dur = it.get_duration();
    if( dur > 100_minutes && u.get_focus() >= 1 && one_in( 24 ) ) {
        u.mod_focus( -1 );
    }
    if( dur > 200_minutes && one_in( 48 ) && u.get_stim() < 20 ) {
        u.mod_stim( 1 );
    }
    if( dur > 300_minutes && u.get_focus() >= 1 && one_in( 12 ) ) {
        u.mod_focus( -1 );
    }
    if( dur > 400_minutes && one_in( 384 ) ) {
        u.mod_pain( rng( -1, -8 ) );
    }
    if( ( !u.has_effect( effect_hallu ) ) && ( dur > 500_minutes && one_in( 24 ) ) ) {
        u.add_effect( effect_hallu, 6_hours );
    }
    if( dur > 600_minutes && one_in( 768 ) ) {
        u.mod_pain( rng( -3, -24 ) );
        if( dur > 800_minutes && one_in( 16 ) && !u.is_on_ground() ) {
            u.add_msg_if_player( m_bad,
                                 _( "You're experiencing loss of basic motor skills and blurred vision.  Your mind recoils in horror, unable to communicate with your spinal column." ) );
            u.add_msg_if_player( m_bad, _( "You stagger and fall!" ) );
            u.add_effect( effect_downed, rng( 1_turns, 4_turns ), false, 0, true );
            if( one_in( 8 ) || x_in_y( u.vomit_mod(), 10 ) ) {
                u.vomit();
            }
        }
    }
    if( dur > 700_minutes && u.get_focus() >= 1 ) {
        u.mod_focus( -1 );
    }
    if( dur > 800_minutes && one_in( 1536 ) ) {
        u.add_effect( effect_visuals, rng( 4_minutes, 20_minutes ) );
        u.mod_pain( rng( -8, -40 ) );
    }
    if( dur > 1200_minutes && one_in( 1536 ) ) {
        u.add_msg_if_player( m_bad, _( "There's some kind of big machine in the sky." ) );
        u.add_effect( effect_visuals, rng( 8_minutes, 40_minutes ) );
        if( one_in( 32 ) ) {
            u.add_msg_if_player( m_bad, _( "It's some kind of electric snake, coming right at you!" ) );
            u.mod_pain( rng( 4, 40 ) );
            u.vomit();
        }
    }
    if( dur > 1400_minutes && one_in( 768 ) ) {
        u.add_msg_if_player( m_bad,
                             _( "Order us some golf shoes, otherwise we'll never get out of this place alive." ) );
        u.add_effect( effect_visuals, rng( 40_minutes, 200_minutes ) );
        if( one_in( 8 ) ) {
            u.add_msg_if_player( m_bad,
                                 _( "The possibility of physical and mental collapse is now very real." ) );
            if( one_in( 2 ) || x_in_y( u.vomit_mod(), 10 ) ) {
                u.add_msg_if_player( m_bad, _( "No one should be asked to handle this trip." ) );
                u.vomit();
                u.mod_pain( rng( 8, 40 ) );
            }
        }
    }

    if( dur > 1800_minutes && one_in( 300 * 512 ) ) {
        if( !u.has_flag( json_flag_PAIN_IMMUNE ) ) {
            u.add_msg_if_player( m_bad,
                                 _( "Your heart spasms painfully and stops, dragging you back to reality as you die." ) );
        } else {
            u.add_msg_if_player(
                _( "You dissolve into beautiful paroxysms of energy.  Life fades from your nebulae and you are no more." ) );
        }
        get_event_bus().send<event_type::dies_from_drug_overdose>( u.getID(), it.get_id() );
        u.set_part_hp_cur( bodypart_id( "torso" ), 0 );
    }
}

static void eff_fun_hypovolemia( Character &u, effect &it )
{
    // hypovolemia and dehydration are closely related so it will pull water
    // from your system to replenish blood quantity
    int intense = it.get_intensity();

    if( calendar::once_every( -u.vitamin_rate( vitamin_blood ) ) && one_in( 5 ) &&
        u.get_thirst() <= 240 ) {
        u.mod_thirst( rng( 0, intense ) );
    }
    // bleed out lambda
    auto bleed_out = [&] {
        if( u.has_effect( effect_bleed ) )
        {
            u.add_msg_player_or_npc( m_bad,
                                     _( "You bleed to death!" ),
                                     _( "<npcname> bleeds to death!" ) );
            get_event_bus().send<event_type::dies_from_bleeding>( u.getID() );
        } else
        {
            u.add_msg_player_or_npc( m_bad,
                                     _( "Your heart can't keep up the pace and fails!" ),
                                     _( "<npcname> has a sudden heart attack!" ) );
            get_event_bus().send<event_type::dies_from_hypovolemia>( u.getID() );
        }
        u.set_part_hp_cur( bodypart_id( "torso" ), 0 );
    };
    // this goes first because beyond minimum threshold you just die without delay,
    // while stage 4 is on a timer check with an rng grace period

    if( u.vitamin_get( vitamin_blood ) == vitamin_blood->min() ) {
        bleed_out();
    }

    // Hypovolemic shock
    // stage 1 - early symptoms include headache, sleepiness, weakness, thirst, and dizziness.
    // stage 2 - person may begin sweating and feeling more anxious and restless.
    // stage 3 - heart rate will increase to over 120 bpm; rapid breathing
    // mental distress, including anxiety and agitation; skin is pale and cold + cyanosis, sweating
    // stage 4 is a life threatening condition; extremely rapid heart rate, breathing very fast and difficult
    // drifting in and out of consciousness, sweating heavily, feeling cool to the touch, looking extremely pale

    if( one_in( 1200 / intense ) && !u.in_sleep_state() ) {
        std::string warning;

        if( one_in( 5 ) ) {
            // no-effect message block
            if( intense == 1 ) {
                warning = _( "Your skin looks pale and you feel anxious and thirsty.  Blood loss?" );
            } else if( intense == 2 ) {
                warning = _( "Your pale skin is sweating, your heart is beating fast, and you feel restless.  Maybe you lost too much blood?" );
            } else if( intense == 3 ) {
                warning = _( "You're unsettlingly white, but your fingertips are bluish.  You are agitated and your heart is racing.  Your blood loss must be serious." );
            } else { //intense == 4
                warning = _( "You are pale as a ghost, dripping wet from the sweat, and sluggish - despite your heart racing like a train.  You are on the brink of collapse from the effects of blood loss." );
            }
            u.add_msg_if_player( m_bad, warning );
        } else {
            // effect dice, with progression of effects, 3 possible effects per tier
            int dice_roll = rng( 0, 2 ) + intense;
            switch( dice_roll ) {
                case 1:
                    warning = _( "You feel dizzy and lightheaded." );
                    u.add_effect( effect_stunned, rng( 5_seconds * intense, 2_minutes * intense ) );
                    break;
                case 2:
                    warning = _( "You feel tired and you breathe heavily." );
                    u.mod_sleepiness( 3 * intense );
                    break;
                case 3:
                    warning = _( "You are anxious and cannot collect your thoughts." );
                    u.mod_focus( -rng( 1, u.get_focus() * intense / it.get_max_intensity() ) );
                    break;
                case 4: {
                    warning = _( "You are sweating profusely, but you feel cold." );
                    const units::temperature_delta delta = -2_C_delta * intense;
                    u.mod_part_temp_conv( bodypart_id( "hand_l" ), delta );
                    u.mod_part_temp_conv( bodypart_id( "hand_r" ), delta );
                    u.mod_part_temp_conv( bodypart_id( "foot_l" ), delta );
                    u.mod_part_temp_conv( bodypart_id( "foot_r" ), delta );
                    break;
                }
                case 5:
                    warning = _( "You huff and puff.  Your breath is rapid and shallow." );
                    u.mod_stamina( -500 * intense );
                    break;
                case 6:
                    if( one_in( 2 ) && !u.is_on_ground() ) {
                        warning = _( "You drop to the ground, fighting to keep yourself conscious." );
                        u.add_effect( effect_downed, rng( 1_minutes, 2_minutes ) );
                        break;
                    } else {
                        warning = _( "Your mind slips away." );
                        u.fall_asleep( rng( 2_minutes, 5_minutes ) );
                        break;
                    }
            }
            u.add_msg_if_player( m_bad, warning );
        }
    }
    // this goes last because we don't want in_sleep_state to prevent you from dying
    if( intense == 4 && one_in( 900 ) &&
        rng( 1, -vitamin_blood->min() * 3 / 5 ) > ( -vitamin_blood->min() + u.vitamin_get(
                    vitamin_blood ) ) ) {
        bleed_out();
    }
}

static void eff_fun_redcells_anemia( Character &u, effect &it )
{
    // Lack of iron impairs production of hemoglobin and therefore ability to carry
    // oxygen by red blood cells. Alternatively hemorrhage causes physical loss of red blood cells.
    // This triggers variety of symptoms, focusing on weakness,
    // sleepiness, cold limbs, later in dizziness, soreness, breathlessness,
    // and severe malaise and lethargy.
    // Base anemia symptoms: sleepiness, loss of stamina, loss of strength, impact on health
    // are placed in effect JSON

    int intense = it.get_intensity();

    // you can only lose as much red blood cells before your body fails to function
    if( u.vitamin_get( vitamin_redcells ) <= vitamin_redcells->min() + 5 ) {
        u.add_msg_player_or_npc( m_bad,
                                 _( "You cannot breathe and your body gives out!" ),
                                 _( "<npcname> gasps for air and dies!" ) );
        get_event_bus().send<event_type::dies_from_redcells_loss>( u.getID() );
        u.set_part_hp_cur( bodypart_id( "torso" ), 0 );
    }
    if( one_in( 900 / intense ) && !u.in_sleep_state() ) {
        // level 1 symptoms are cold limbs, pale skin, and weakness
        switch( dice( 1, 9 ) ) {
            case 1:
                u.add_msg_if_player( m_bad, _( "Your hands feel unusually cold." ) );
                u.mod_part_temp_conv( bodypart_id( "hand_l" ), -4_C_delta );
                u.mod_part_temp_conv( bodypart_id( "hand_r" ), -4_C_delta );
                break;
            case 2:
                u.add_msg_if_player( m_bad, _( "Your feet feel unusually cold." ) );
                u.mod_part_temp_conv( bodypart_id( "foot_l" ), -4_C_delta );
                u.mod_part_temp_conv( bodypart_id( "foot_r" ), -4_C_delta );
                break;
            case 3:
                u.add_msg_if_player( m_bad, _( "Your skin looks very pale." ) );
                break;
            case 4:
                u.add_msg_if_player( m_bad, _( "You feel weak.  Where has your strength gone?" ) );
                break;
            case 5:
                u.add_msg_if_player( m_bad, _( "You feel feeble.  A gust of wind could make you stumble." ) );
                break;
            case 6:
                u.add_msg_if_player( m_bad, _( "There is an overwhelming aura of tiredness inside of you." ) );
                u.mod_sleepiness( intense * 3 );
                break;
            case 7: // 7-9 empty for variability, as messages stack on higher intensity
            case 8:
            case 9:
                break;
        }
        // level 2 anemia introduces dizziness, shakes, headaches, cravings for non-comestibles,
        // mouth and tongue soreness
        if( intense > 1 ) {
            switch( dice( 1, 9 ) ) {
                case 1:
                    u.add_msg_if_player( m_bad, _( "Rest is what you want.  Rest is what you need." ) );
                    break;
                case 2:
                    u.add_msg_if_player( m_bad, _( "You feel dizzy and can't coordinate the movement of your feet." ) );
                    u.add_effect( effect_stunned, rng( 1_minutes, 2_minutes ) );
                    break;
                case 3:
                    u.add_msg_if_player( m_bad, _( "Your muscles are quivering." ) );
                    u.add_effect( effect_shakes, rng( 4_minutes, 8_minutes ) );
                    break;
                case 4:
                    u.add_msg_if_player( m_bad, _( "You crave for ice.  The dirt under your feet looks tasty too." ) );
                    break;
                case 5:
                    u.add_msg_if_player( m_bad, _( "Your whole mouth is sore, and your tongue is swollen." ) );
                    break;
                case 6:
                    u.add_msg_if_player( m_bad, _( "You feel lightheaded.  A migraine follows." ) );
                    u.mod_pain( intense * 9 );
                    break;
                case 7: // 7-9 empty for variability, as messages stack on higher intensity
                case 8:
                case 9:
                    break;
            }
        }
        // level 3 anemia introduces restless legs, severe tiredness, breathlessness
        if( intense > 2 ) {
            switch( dice( 1, 9 ) ) {
                case 1:
                    u.add_msg_if_player( m_bad, _( "Your legs are restless.  The urge to move them is so strong." ) );
                    break;
                case 2:
                    u.add_msg_if_player( m_bad, _( "You feel like you could sleep on a rock." ) );
                    u.mod_sleepiness( intense * 3 );
                    break;
                case 3:
                    u.add_msg_if_player( m_bad, _( "You gasp for air!" ) );
                    u.set_stamina( 0 );
                    u.add_effect( effect_winded, rng( 30_seconds, 3_minutes ) );
                    break;
                case 4:
                    u.add_msg_if_player( m_bad, _( "Can't breathe.  Must rest." ) );
                    u.set_stamina( 0 );
                    break;
                case 5:
                    u.add_msg_if_player( m_bad,
                                         _( "You can't take it any more.  Rest first; everything else later." ) );
                    u.add_effect( effect_lying_down, rng( 2_minutes, 5_minutes ) );
                    break;
                case 6:
                    if( !u.is_on_ground() ) {
                        u.add_msg_if_player( m_bad, _( "You must sit down for a moment.  Just a moment." ) );
                        u.add_effect( effect_downed, rng( 1_minutes, 2_minutes ) );
                    }
                    break;
                case 7: // 7-9 empty for variability, as messages stack on higher intensity
                case 8:
                case 9:
                    break;
            }
        }
    }
}

static void eff_fun_sleep( Character &u, effect &it )
{
    int intense = it.get_intensity();
    map &here = get_map();

    u.set_moves( 0 );
    if( u.is_avatar() ) {
        inp_mngr.pump_events();
    }

    if( intense < 1 ) {
        it.set_intensity( 1 );
    } else if( intense < 24 ) {
        it.mod_intensity( 1 );
    }

    if( u.has_effect( effect_narcosis ) && u.get_sleepiness() <= 25 ) {
        u.set_sleepiness( 25 ); //Prevent us from waking up naturally while under anesthesia
    }

    if( u.get_sleepiness() < -25 && it.get_duration() > 3_minutes &&
        !u.has_effect( effect_narcosis ) ) {
        it.set_duration( 1_turns * dice( 3, 10 ) );
    }

    if( u.get_sleepiness() <= 0 && u.get_sleepiness() > -20 && !u.has_effect( effect_narcosis ) ) {
        u.mod_sleepiness( -25 );
        if( u.get_sleep_deprivation() < SLEEP_DEPRIVATION_HARMLESS ) {
            u.add_msg_if_player( m_good, _( "You feel well rested." ) );
        } else {
            u.add_msg_if_player( m_warning,
                                 _( "You feel physically rested, but you haven't been able to catch up on your missed sleep yet." ) );
        }
        it.set_duration( 1_turns * dice( 3, 100 ) );
    }

    // TODO: Move this to update_needs when NPCs can mutate
    if( calendar::once_every( 10_minutes ) && ( u.has_trait( trait_CHLOROMORPH ) ||
            u.has_trait( trait_M_SKIN3 ) || u.has_trait( trait_WATERSLEEP ) ) &&
        here.is_outside( u.pos() ) ) {
        if( u.has_trait( trait_CHLOROMORPH ) ) {
            // Hunger and thirst fall before your Chloromorphic physiology!
            if( incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::low ) {
                if( u.has_active_mutation( trait_CHLOROMORPH ) && ( u.get_sleepiness() <= 25 ) ) {
                    u.set_sleepiness( 25 );
                }
                if( u.get_hunger() >= -30 ) {
                    u.mod_hunger( -5 );
                    // photosynthesis warrants absorbing kcal directly
                    u.mod_stored_kcal( 43 );
                }
            }
            if( u.get_thirst() >= -40 ) {
                u.mod_thirst( -5 );
            }
            // Assuming eight hours of sleep, this will take care of Iron and Calcium needs
            u.vitamin_mod( vitamin_iron, 2 );
            u.vitamin_mod( vitamin_calcium, 2 );
        }
        if( u.has_trait( trait_M_SKIN3 ) ) {
            // Spores happen!
            if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, u.pos() ) ) {
                if( u.get_sleepiness() >= 0 ) {
                    u.mod_sleepiness( -5 ); // Local guides need less sleep on fungal soil
                }
                if( calendar::once_every( 1_hours ) ) {
                    u.spores(); // spawn some P O O F Y   B O I S
                }
            }
        }
        if( u.has_trait( trait_WATERSLEEP ) ) {
            u.mod_sleepiness( -3 ); // Fish sleep less in water
        }
    }

    // Check mutation category strengths to see if we're mutated enough to get a dream
    // If we've crossed a threshold, always show dreams for that category
    // Otherwise, check for the category that we have the most vitamins in our blood for
    mutation_category_id cat = u.get_threshold_category();
    weighted_int_list<mutation_category_id> cat_list = u.get_vitamin_weighted_categories();
    if( cat.is_null() && cat_list.get_weight() > 0 ) {
        cat = *cat_list.pick();
    }
    int cat_strength = u.mutation_category_level[cat];

    // Determine the strength of effects or dreams based upon category strength
    int strength = 0; // Category too weak for any effect or dream
    if( u.crossed_threshold() ) {
        strength = 4; // Post-human.
    } else if( cat_strength >= 15 && cat_strength < 22 ) {
        strength = 1; // Low strength
    } else if( cat_strength >= 22 && cat_strength < 30 ) {
        strength = 2; // Medium strength
    } else if( cat_strength >= 30 ) {
        strength = 3; // High strength
    }

    // Get a dream if category strength is high enough.
    if( strength != 0 ) {
        //Once every 6 / 3 / 2 hours, with a bit of randomness
        if( calendar::once_every( 6_hours / strength ) && one_in( 3 ) ) {
            // Select a dream
            std::string dream = u.get_category_dream( cat, strength );
            if( !dream.empty() ) {
                u.add_msg_if_player( dream );
            }
            // Mycus folks upgrade in their sleep.
            if( u.has_trait( trait_THRESH_MYCUS ) ) {
                if( one_in( 8 ) ) {
                    u.mutate_category( mutation_category_MYCUS, false, true );
                    u.mod_stored_kcal( -87 );
                    u.mod_thirst( 10 );
                    u.mod_sleepiness( 5 );
                }
            }
        }
    }

    bool woke_up = false;
    int tirednessVal = rng( 5, 200 ) + rng( 0, std::abs( u.get_sleepiness() * 2 * 5 ) );
    if( !u.is_blind() && !u.has_effect( effect_narcosis ) &&
        !u.has_active_mutation( trait_CHLOROMORPH ) && !u.has_bionic( bio_sleep_shutdown ) ) {
        // People who can see while sleeping are acclimated to the light.
        if( !u.has_flag( json_flag_SEESLEEP ) ) {
            if( u.has_trait( trait_HEAVYSLEEPER2 ) && !u.has_trait( trait_HIBERNATE ) ) {
                // So you can too sleep through noon
                if( ( tirednessVal * 1.25 ) < here.ambient_light_at( u.pos() ) && ( u.get_sleepiness() < 10 ||
                        one_in( u.get_sleepiness() / 2 ) ) ) {
                    u.add_msg_if_player( _( "It's too bright to sleep." ) );
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                    woke_up = true;
                }
                // Ursine hibernators would likely do so indoors.  Plants, though, might be in the sun.
            } else if( u.has_trait( trait_HIBERNATE ) ) {
                if( ( tirednessVal * 5 ) < here.ambient_light_at( u.pos() ) && ( u.get_sleepiness() < 10 ||
                        one_in( u.get_sleepiness() / 2 ) ) ) {
                    u.add_msg_if_player( _( "It's too bright to sleep." ) );
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                    woke_up = true;
                }
            } else if( tirednessVal < here.ambient_light_at( u.pos() ) && ( u.get_sleepiness() < 10 ||
                       one_in( u.get_sleepiness() / 2 ) ) ) {
                u.add_msg_if_player( _( "It's too bright to sleep." ) );
                // Set ourselves up for removal
                it.set_duration( 0_turns );
                woke_up = true;
            }
        } else if( u.has_flag( json_flag_SEESLEEP ) ) {
            Creature *hostile_critter = g->is_hostile_very_close();
            if( hostile_critter != nullptr ) {
                u.add_msg_if_player( _( "You see %s approaching!" ),
                                     hostile_critter->disp_name() );
                it.set_duration( 0_turns );
                woke_up = true;
            }
        }
    }

    // Have we already woken up?
    if( !woke_up && !u.has_effect( effect_narcosis ) ) {
        // Cold or heat may wake you up.
        // Player will sleep through cold or heat if sleepy enough
        for( const bodypart_id &bp : u.get_all_body_parts() ) {
            const units::temperature curr_temp = u.get_part_temp_cur( bp );
            const units::temperature_delta sleepiness_modifier = units::from_celsius_delta(
                        u.get_sleepiness() / 1000.0 );
            if( curr_temp < BODYTEMP_VERY_COLD - sleepiness_modifier ) {
                if( one_in( 30000 ) ) {
                    u.add_msg_if_player( _( "You toss and turn trying to keep warm." ) );
                }
                if( curr_temp < BODYTEMP_FREEZING - sleepiness_modifier ||
                    one_in( units::to_celsius( curr_temp ) * 3000 + 16500 ) ) {
                    u.add_msg_if_player( m_bad, _( "It's too cold to sleep." ) );
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                    woke_up = true;
                    break;
                }
            } else if( curr_temp > BODYTEMP_VERY_HOT + sleepiness_modifier ) {
                if( one_in( 30000 ) ) {
                    u.add_msg_if_player( _( "You toss and turn in the heat." ) );
                }
                if( curr_temp > BODYTEMP_SCORCHING + sleepiness_modifier ||
                    one_in( 76500 - units::to_celsius( curr_temp ) * 500 ) ) {
                    u.add_msg_if_player( m_bad, _( "It's too hot to sleep." ) );
                    // Set ourselves up for removal
                    it.set_duration( 0_turns );
                    woke_up = true;
                    break;
                }
            }
        }
    }

    // A bit of a hack: check if we are about to wake up for any reason, including regular
    // timing out of sleep
    if( it.get_duration() == 1_turns || woke_up ) {
        u.wake_up();
    }
}

void Character::hardcoded_effects( effect &it )
{
    if( const ma_buff *buff = ma_buff::from_effect( it ) ) {
        if( buff->is_valid_character( *this ) ) {
            buff->apply_character( *this );
        } else {
            it.set_duration( 0_turns ); // removes the effect
        }
        return;
    }
    using hc_effect_fun = std::function<void( Character &, effect & )>;
    static const std::map<efftype_id, hc_effect_fun> hc_effect_map = {{
            { effect_onfire, eff_fun_onfire },
            { effect_spores, eff_fun_spores },
            { effect_fungus, eff_fun_fungus },
            { effect_antifungal, eff_fun_antifungal },
            { effect_rat, eff_fun_rat },
            { effect_bleed, eff_fun_bleed },
            { effect_hallu, eff_fun_hallu },
            { effect_cold, eff_fun_cold },
            { effect_hot, eff_fun_hot },
            { effect_frostbite, eff_fun_frostbite },
            { effect_teleglow, eff_fun_teleglow },
            { effect_datura, eff_fun_datura },
            { effect_hypovolemia, eff_fun_hypovolemia },
            { effect_redcells_anemia, eff_fun_redcells_anemia },
            { effect_sleep, eff_fun_sleep },
            { effect_fake_common_cold, eff_fun_fake_common_cold },
            { effect_fake_flu, eff_fun_fake_flu },
        }
    };
    const efftype_id &id = it.get_id();
    const auto &iter = hc_effect_map.find( id );
    if( iter != hc_effect_map.end() ) {
        iter->second( *this, it );
        return;
    }

    const time_duration dur = it.get_duration();
    int intense = it.get_intensity();
    const bodypart_id &bp = it.get_bp();
    bool sleeping = has_effect( effect_sleep );
    map &here = get_map();
    Character &player_character = get_player_character();
    creature_tracker &creatures = get_creature_tracker();
    if( id == effect_dermatik ) {
        bool triggered = false;
        int formication_chance = 3600;
        if( dur < 4_hours ) {
            formication_chance += 14400 - to_turns<int>( dur );
        }
        if( one_in( formication_chance ) ) {
            schedule_effect( effect_formication, 60_minutes, bp );
        }
        if( dur < 1_days && one_in( 14400 ) ) {
            vomit();
        }
        if( dur > 1_days ) {
            // Spawn some larvae!
            // Choose how many insects; more for large characters
            ///\EFFECT_STR_MAX increases number of insects hatched from dermatik infection
            int num_insects = rng( 1, std::min( 3, str_max / 3 ) );
            apply_damage( nullptr,  bp, rng( 2, 4 ) * num_insects );
            // Figure out where they may be placed
            add_msg_player_or_npc( m_bad,
                                   _( "Your flesh crawls; insects tear through the flesh and begin to emerge!" ),
                                   _( "Insects begin to emerge from <npcname>'s skin!" ) );
            for( ; num_insects > 0; num_insects-- ) {
                if( monster *const grub = g->place_critter_around( mon_dermatik_larva, pos(), 1 ) ) {
                    if( one_in( 3 ) ) {
                        grub->friendly = -1;
                        grub->add_effect( effect_pet, 1_turns, true );
                    }
                }
            }
            get_event_bus().send<event_type::dermatik_eggs_hatch>( getID() );
            schedule_effect_removal( effect_formication, bp );
            mod_moves( -to_moves<int>( 6_seconds ) );
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
                add_msg( m_warning, _( "You start scratching your %s!" ),
                         body_part_name_accusative( bp ) );
            } else {
                //~ 1$s is NPC name, 2$s is bodypart in accusative.
                add_msg_if_player_sees( pos(), _( "%1$s starts scratching their %2$s!" ), get_name(),
                                        body_part_name_accusative( bp ) );
            }
            mod_moves( -to_moves<int>( 1_seconds ) * 1.5 );
            mod_pain( 1 );
            apply_damage( nullptr, bp, 1 );
        }
    } else if( id == effect_evil ) {
        // Major effects, all bad.
        mod_str_bonus( -( dur > 500_minutes ? 10.0 : dur / 50_minutes ) );
        int dex_mod = -( dur > 600_minutes ? 10.0 : dur / 60_minutes );
        mod_dex_bonus( dex_mod );
        add_miss_reason( _( "Why waste your time on that insignificant speck?" ), -dex_mod );
        mod_int_bonus( -( dur > 450_minutes ? 10.0 : dur / 45_minutes ) );
        mod_per_bonus( -( dur > 400_minutes ? 10.0 : dur / 40_minutes ) );
    } else if( id == effect_attention ) {
        if( to_turns<int>( dur ) != 0 && one_in( 100000 / to_turns<int>( dur ) ) &&
            one_in( 100000 / to_turns<int>( dur ) ) && one_in( 250 ) ) {
            tripoint dest( 0, 0, posz() );
            int tries = 0;
            do {
                dest.x = posx() + rng( -4, 4 );
                dest.y = posy() + rng( -4, 4 );
                tries++;
            } while( creatures.creature_at( dest ) && tries < 10 );
            if( tries < 10 ) {
                if( here.impassable( dest ) ) {
                    here.make_rubble( dest, furn_f_rubble_rock, true );
                }
                std::vector<MonsterGroupResult> spawn_details =
                    MonsterGroupManager::GetResultFromGroup( GROUP_NETHER );
                for( const MonsterGroupResult &mgr : spawn_details ) {
                    g->place_critter_at( mgr.name, dest );
                }
                if( uistate.distraction_hostile_spotted && player_character.sees( dest ) ) {
                    g->cancel_activity_or_ignore_query( distraction_type::hostile_spotted_far,
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
                mod_sleepiness( dice( 1, 6 ) );
            } else if( one_in( 3000 ) ) {
                add_msg_if_player( m_bad,
                                   _( "You feel like you need less teeth.  You pull one out, and it is rotten to the core." ) );
                mod_pain( 1 );
            } else if( one_in( 3000 ) ) {
                add_msg_if_player( m_bad, _( "You notice a large abscess.  You pick at it." ) );
                const bodypart_id &itch = random_body_part( true );
                schedule_effect( effect_formication, 60_minutes, itch );
                mod_pain( 1 );
            } else if( one_in( 3000 ) ) {
                add_msg_if_player( m_bad,
                                   _( "You feel so sick, like you've been poisoned, but you need more.  So much more." ) );
                vomit();
                mod_sleepiness( dice( 1, 6 ) );
            }
        }
    } else if( id == effect_tindrift ) {
        add_msg_if_player( m_bad, _( "You are beset with a vision of a prowling beast." ) );
        for( const tripoint &dest : here.points_in_radius( pos(), 6 ) ) {
            if( here.is_cornerfloor( dest ) ) {
                here.add_field( dest, fd_tindalos_rift, 3 );
                add_msg_if_player( m_info, _( "Your surroundings are permeated with a foul scent." ) );
                // Queue the effect for removal, since it's done all it needs to do to the target.
                it.set_duration( 0_turns );
            }
        }
    } else if( id == effect_asthma ) {
        if( has_effect( effect_adrenaline ) || has_effect( effect_datura ) ) {
            add_msg_if_player( m_good, _( "Your asthma attack stops." ) );
            it.set_duration( 0_turns );
        } else if( dur > 2_hours ) {
            add_msg_if_player( m_bad, _( "Your asthma overcomes you.\nYou asphyxiate." ) );
            get_event_bus().send<event_type::dies_from_asthma_attack>( getID() );
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
            mod_daily_health( -10, -100 );
            apply_damage( nullptr, bodypart_id( "head" ), rng( 0, 1 ) );
            if( !has_effect( effect_visuals ) ) {
                add_msg_if_player( m_bad, _( "Your vision is getting fuzzy." ) );
                schedule_effect( effect_visuals, rng( 1_minutes, 60_minutes ) );
            }
        }
        if( one_in( 24576 ) ) {
            mod_daily_health( -10, -100 );
            apply_damage( nullptr, bodypart_id( "head" ), rng( 1, 2 ) );
            if( !is_blind() && !sleeping ) {
                add_msg_if_player( m_bad, _( "Your vision goes black!" ) );
                schedule_effect( effect_blind, rng( 5_turns, 20_turns ) );
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
        // to do: make muscle spasms not as dangerous if you have bionic limbs
        if( !has_effect( effect_valium ) ) {
            add_miss_reason( _( "Your muscles are locking up and you can't fight effectively." ), 4 );
            if( one_in( 3072 ) ) {
                add_msg_if_player( m_bad, _( "Your muscles spasm." ) );
                if( !is_on_ground() ) {
                    schedule_effect( effect_downed, rng( 1_turns, 4_turns ), false, 0, true );
                }
                schedule_effect( effect_stunned, rng( 1_turns, 4_turns ) );
                if( one_in( 10 ) ) {
                    mod_pain( rng( 1, 10 ) );
                }
            }
        }
    } else if( id == effect_anemia ) {
        // effects: reduces effective redcells regen and depletes redcells at high intensity
        if( calendar::once_every( vitamin_rate( vitamin_redcells ) ) ) {
            vitamin_mod( vitamin_redcells, -rng( 0, intense ) );
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
            if( has_trait( trait_INFRESIST ) ) {
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
            recover_factor += get_lifestyle() / 10;

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
                schedule_effect( effect_infected, 1_turns, bp, true );
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
            if( has_trait( trait_INFRESIST ) ) {
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
            recover_factor += get_lifestyle() / 10;

            if( x_in_y( recover_factor, 5184000 ) ) {
                //~ %s is bodypart name.
                add_msg_if_player( m_good, _( "Your %s wound begins to feel better!" ),
                                   body_part_name( bp ) );
                schedule_effect( effect_recover, 4 * dur );
                // Set ourselves up for removal
                it.set_duration( 0_turns );
                recovered = true;
            }
        }
        if( !recovered ) {
            // Death happens
            if( dur > 1_days ) {
                add_msg_if_player( m_bad, _( "You succumb to the infection." ) );
                get_event_bus().send<event_type::dies_of_infection>( getID() );
                set_all_parts_hp_cur( 0 );
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
            add_msg_if_player( _( "You try to sleep, but can't…" ) );
        }
    } else if( id == effect_alarm_clock ) {
        if( in_sleep_state() ) {
            const bool asleep = has_effect( effect_sleep );
            if( has_flag( json_flag_ALARMCLOCK ) ) {
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
                    } else if( ( !( has_trait( trait_HEAVYSLEEPER ) ||
                                    has_trait( trait_HEAVYSLEEPER2 ) ||
                                    has_bionic( bio_sleep_shutdown ) ) &&
                                 dice( 2, 15 ) < volume ) ||
                               ( has_trait( trait_HEAVYSLEEPER ) && dice( 3, 15 ) < volume ) ||
                               ( has_trait( trait_HEAVYSLEEPER2 ) && dice( 6, 15 ) < volume ) ||
                               has_bionic( bio_sleep_shutdown ) ) {
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
                            schedule_effect( effect_slept_through_alarm, 1_turns, true );
                        }
                        // 10 minute cyber-snooze
                        it.mod_duration( 10_minutes );
                    }
                }
            } else {
                if( asleep && dur == 1_turns ) {
                    if( !has_effect( effect_slept_through_alarm ) ) {
                        schedule_effect( effect_slept_through_alarm, 1_turns, true );
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
                if( player_character.has_alarm_clock() ) {
                    sounds::sound( player_character.pos(), 16, sounds::sound_t::alarm,
                                   _( "beep-beep-beep!" ), false, "tool", "alarm_clock" );
                    const std::string alarm = _( "Your alarm is going off." );
                    g->cancel_activity_or_ignore_query( distraction_type::noise, alarm );
                    add_msg( _( "Your alarm went off." ) );
                }
            }
        }
    } else if( id == effect_mending ) {
        if( !is_limb_broken( bp ) ) {
            it.set_duration( 0_turns );
        }
    } else if( id == effect_disabled ) {
        if( !is_limb_broken( bp ) ) {
            // Just unpause, in case someone added it as a temporary effect (numbing poison etc.)
            it.unpause_effect();
        }
    } else if( id == effect_toxin_buildup ) {
        // Loosely based on toxic man-made compounds (mostly pesticides) which don't degrade
        // easily, leading to build-up in muscle and fat tissue through bioaccumulation.
        // Symptoms vary, and many are too long-term to be relevant in C:DDA (i.e. carcinogens),
        // but lowered immune response and neurotoxicity (i.e. seizures, migraines) are common.
        // to do: make these interact with bionic limbs? if they are neurotoxins your brain might be sending faulty signals to your mecahnical limbs anyways however

        if( in_sleep_state() ) {
            return;
        }
        // Modifier for symptom frequency.
        // Each symptom is twice as frequent for each level of intensity above the one it first appears for.
        int mod = 1;
        switch( intense ) {
            case 3:
                // Tonic-clonic seizure (full body convulsive seizure)
                if( one_turn_in( 3_days ) && !has_effect( effect_valium ) ) {
                    add_msg_if_player( m_bad, _( "You lose control of your body as it begins to convulse!" ) );
                    time_duration td = rng( 30_seconds, 4_minutes );
                    schedule_effect( effect_motor_seizure, td );
                    if( !is_on_ground() ) {
                        schedule_effect( effect_downed, td );
                    }
                    schedule_effect( effect_stunned, td );
                    if( one_in( 3 ) ) {
                        add_msg_if_player( m_bad, _( "You lose consciousness!" ) );
                        fall_asleep( td );
                    }
                }
                mod *= 2;
            /* fallthrough */
            case 2:
                // Myoclonic seizure (muscle spasm)
                if( one_turn_in( 2_hours / mod ) && !has_effect( effect_valium ) ) {
                    std::string limb = random_entry<std::vector<std::string>>( {
                        translate_marker( "arm" ), translate_marker( "hand" ), translate_marker( "leg" )
                    } );
                    add_msg_if_player( m_bad, string_format(
                                           _( "Your %s suddenly jerks in an unexpected direction!" ), _( limb ) ) );
                    if( limb == "arm" ) {
                        mod_dex_bonus( -8 );
                        recoil = MAX_RECOIL;
                    } else if( limb == "hand" ) {
                        if( is_armed() && can_drop( *get_wielded_item() ).success() ) {
                            if( dice( 4, 4 ) > get_dex() ) {
                                cancel_activity();  //Prevent segfaults from activities trying to access missing item
                                put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { remove_weapon() } );
                            } else {
                                add_msg_if_player( m_neutral, _( "However, you manage to keep hold of your weapon." ) );
                            }
                        }
                    } else if( limb == "leg" ) {
                        if( dice( 4, 4 ) > get_dex() && !is_on_ground() ) {
                            schedule_effect( effect_downed, rng( 5_seconds, 10_seconds ) );
                        } else {
                            add_msg_if_player( m_neutral, _( "However, you manage to keep your footing." ) );
                        }
                    }
                }
                // Atonic seizure (a.k.a. drop seizure)
                if( one_turn_in( 2_days / mod ) && !has_effect( effect_valium ) ) {
                    add_msg_if_player( m_bad,
                                       _( "You suddenly lose all muscle tone, and can't support your own weight!" ) );
                    schedule_effect( effect_motor_seizure, rng( 1_seconds, 2_seconds ) );
                    if( !is_on_ground() ) {
                        schedule_effect( effect_downed, rng( 5_seconds, 10_seconds ) );
                    }
                }
                mod *= 2;
            /* fallthrough */
            case 1:
                // Migraine
                if( one_turn_in( 2_days / mod ) ) {
                    add_msg_if_player( m_bad, _( "You have a splitting headache." ) );
                    mod_pain( 12 );
                }

                break;
        }
    }
}
