#include "character.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "action.h"
#include "activity_handlers.h"
#include "activity_tracker.h"
#include "addiction.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "character_attire.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "disease.h"
#include "effect.h"
#include "effect_source.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field_type.h"
#include "flag.h"
#include "fungal_effects.h"
#include "game.h"
#include "game_constants.h"
#include "input_context.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "line.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "math_parser_diag_value.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "morale.h"
#include "move_mode.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "sleep.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "viewer.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_type.h"
#include "weighted_list.h"
#include "wound.h"

static const activity_id ACT_READ( "ACT_READ" );
static const activity_id ACT_TREE_COMMUNION( "ACT_TREE_COMMUNION" );
static const activity_id ACT_TRY_SLEEP( "ACT_TRY_SLEEP" );

static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_sleep_shutdown( "bio_sleep_shutdown" );
static const bionic_id bio_synlungs( "bio_synlungs" );

static const character_modifier_id
character_modifier_melee_stamina_cost_mod( "melee_stamina_cost_mod" );
static const character_modifier_id
character_modifier_move_mode_move_cost_mod( "move_mode_move_cost_mod" );
static const character_modifier_id
character_modifier_stamina_move_cost_mod( "stamina_move_cost_mod" );
static const character_modifier_id
character_modifier_stamina_recovery_breathing_mod( "stamina_recovery_breathing_mod" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_electric( "electric" );
static const damage_type_id damage_stab( "stab" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_beartrap( "beartrap" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_chafing( "chafing" );
static const efftype_id effect_common_cold( "common_cold" );
static const efftype_id effect_cough_suppress( "cough_suppress" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_disrupted_sleep( "disrupted_sleep" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_flu( "flu" );
static const efftype_id effect_foodpoison( "foodpoison" );
static const efftype_id effect_heavysnare( "heavysnare" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_jetinjector( "jetinjector" );
static const efftype_id effect_lack_sleep( "lack_sleep" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_melatonin( "melatonin" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_pkill1_acetaminophen( "pkill1_acetaminophen" );
static const efftype_id effect_pkill1_generic( "pkill1_generic" );
static const efftype_id effect_pkill1_nsaid( "pkill1_nsaid" );
static const efftype_id effect_pkill2( "pkill2" );
static const efftype_id effect_pkill3( "pkill3" );
static const efftype_id effect_recently_coughed( "recently_coughed" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
static const efftype_id effect_took_thorazine( "took_thorazine" );
static const efftype_id effect_winded( "winded" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id itype_beartrap( "beartrap" );
static const itype_id itype_foodperson_mask( "foodperson_mask" );
static const itype_id itype_foodperson_mask_on( "foodperson_mask_on" );
static const itype_id itype_rope_6( "rope_6" );
static const itype_id itype_snare_trigger( "snare_trigger" );

static const json_character_flag json_flag_ACIDBLOOD( "ACIDBLOOD" );
static const json_character_flag json_flag_ALWAYS_HEAL( "ALWAYS_HEAL" );
static const json_character_flag json_flag_BIONIC_FAULTY( "BIONIC_FAULTY" );
static const json_character_flag json_flag_BIONIC_LIMB( "BIONIC_LIMB" );
static const json_character_flag json_flag_BIONIC_SHOCKPROOF( "BIONIC_SHOCKPROOF" );
static const json_character_flag json_flag_BLIND( "BLIND" );
static const json_character_flag json_flag_CANNIBAL( "CANNIBAL" );
static const json_character_flag json_flag_CANNOT_TAKE_DAMAGE( "CANNOT_TAKE_DAMAGE" );
static const json_character_flag json_flag_DEAF( "DEAF" );
static const json_character_flag json_flag_GRAB( "GRAB" );
static const json_character_flag json_flag_NUMB( "NUMB" );
static const json_character_flag json_flag_HEAL_OVERRIDE( "HEAL_OVERRIDE" );
static const json_character_flag json_flag_NO_BODY_HEAT( "NO_BODY_HEAT" );
static const json_character_flag json_flag_NO_RADIATION( "NO_RADIATION" );
static const json_character_flag json_flag_NO_THIRST( "NO_THIRST" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );
static const json_character_flag json_flag_PSYCHOPATH( "PSYCHOPATH" );
static const json_character_flag json_flag_SAPIOVORE( "SAPIOVORE" );
static const json_character_flag json_flag_SPIRITUAL( "SPIRITUAL" );
static const json_character_flag json_flag_STOP_SLEEP_DEPRIVATION( "STOP_SLEEP_DEPRIVATION" );
static const json_character_flag json_flag_WALK_UNDERWATER( "WALK_UNDERWATER" );
static const json_character_flag json_flag_WATERWALKING( "WATERWALKING" );

static const limb_score_id limb_score_breathing( "breathing" );
static const limb_score_id limb_score_vision( "vision" );

static const morale_type morale_cold( "morale_cold" );
static const morale_type morale_hot( "morale_hot" );
static const morale_type morale_killed_innocent( "morale_killed_innocent" );
static const morale_type morale_killer_has_killed( "morale_killer_has_killed" );
static const morale_type morale_pyromania_nofire( "morale_pyromania_nofire" );
static const morale_type morale_pyromania_startfire( "morale_pyromania_startfire" );

static const move_mode_id move_mode_prone( "prone" );
static const move_mode_id move_mode_walk( "walk" );

static const mtype_id mon_player_blob( "mon_player_blob" );

static const skill_id skill_swimming( "swimming" );

static const trait_id trait_ADRENALINE( "ADRENALINE" );
static const trait_id trait_BADBACK( "BADBACK" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CHEMIMBALANCE( "CHEMIMBALANCE" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_DEBUG_LS( "DEBUG_LS" );
static const trait_id trait_DEBUG_STAMINA( "DEBUG_STAMINA" );
static const trait_id trait_DISRESISTANT( "DISRESISTANT" );
static const trait_id trait_GLASSJAW( "GLASSJAW" );
static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );
static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_PROF_FOODP( "PROF_FOODP" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_ROOTS2( "ROOTS2" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_SCHIZOPHRENIC( "SCHIZOPHRENIC" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHELL3( "SHELL3" );
static const trait_id trait_SLIMESPAWNER( "SLIMESPAWNER" );
static const trait_id trait_STRONGBACK( "STRONGBACK" );
static const trait_id trait_TRANSPIRATION( "TRANSPIRATION" );
static const trait_id trait_UNDINE_SLEEP_WATER( "UNDINE_SLEEP_WATER" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );

static const vitamin_id vitamin_calcium( "calcium" );
static const vitamin_id vitamin_iron( "iron" );

namespace io
{

template<>
std::string enum_to_string<blood_type>( blood_type data )
{
    switch( data ) {
            // *INDENT-OFF*
        case blood_type::blood_O: return "O";
        case blood_type::blood_A: return "A";
        case blood_type::blood_B: return "B";
        case blood_type::blood_AB: return "AB";
            // *INDENT-ON*
        case blood_type::num_bt:
            break;
    }
    cata_fatal( "Invalid blood_type" );
}

} // namespace io

bool Character::can_recover_oxygen() const
{
    return get_limb_score( limb_score_breathing ) > 0.5f && !is_underwater() &&
           !has_effect_with_flag( json_flag_GRAB ) && !( has_bionic( bio_synlungs ) &&
                   !has_active_bionic( bio_synlungs ) );
}

bool Character::is_warm() const
{
    if( has_flag( json_flag_NO_BODY_HEAT ) ) {
        return false;
    } else {
        return true;
    }
}

int Character::get_fat_to_hp() const
{
    int fat_to_hp = ( get_bmi_fat() - character_weight_category::normal );
    fat_to_hp = enchantment_cache->modify_value( enchant_vals::mod::FAT_TO_MAX_HP, fat_to_hp );
    return fat_to_hp;
}

void Character::react_to_felt_pain( int intensity )
{
    if( intensity <= 0 ) {
        return;
    }
    if( uistate.distraction_pain && is_avatar() && intensity >= 2 ) {
        g->cancel_activity_or_ignore_query( distraction_type::pain, _( "Ouch, something hurts!" ) );
    }
    // Only a large pain burst will actually wake people while sleeping.
    if( has_effect( effect_sleep ) && get_effect( effect_sleep ).get_duration() > 0_turns &&
        !has_effect( effect_narcosis ) ) {
        int pain_thresh = rng( 3, 5 );

        if( has_bionic( bio_sleep_shutdown ) ) {
            pain_thresh += 999;
        } else if( has_trait( trait_HEAVYSLEEPER ) ) {
            pain_thresh += 2;
        } else if( has_trait( trait_HEAVYSLEEPER2 ) ) {
            pain_thresh += 5;
        }

        if( intensity >= pain_thresh ) {
            wake_up();
        }
    }
}

bool Character::is_dead_state() const
{
    if( cached_dead_state.has_value() ) {
        return cached_dead_state.value();
    }

    cached_dead_state = false;
    // we want to warn the player with a debug message if they are invincible. this should be unimportant once wounds exist and bleeding is how you die.
    bool has_vitals = false;
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( bp->is_vital ) {
            has_vitals = true;
            if( get_part_hp_cur( bp ) <= 0 ) {
                cached_dead_state = true;
                return cached_dead_state.value();
            }
        }
    }
    if( !has_vitals ) {
        debugmsg( _( "WARNING!  %s has no vital part and is invincible." ), disp_name() );
    }
    return false;
}

void Character::set_part_hp_cur( const bodypart_id &id, int set )
{
    bool is_broken_before = get_part_hp_cur( id ) <= 0;
    Creature::set_part_hp_cur( id, set );
    bool is_broken_after = get_part_hp_cur( id ) <= 0;
    // detect unbroken->broken (possible death) as well as broken->unbroken (possible revival)
    if( is_broken_before != is_broken_after ) {
        cached_dead_state.reset();
    }
}

void Character::mod_part_hp_cur( const bodypart_id &id, int set )
{
    bool is_broken_before = get_part_hp_cur( id ) <= 0;
    Creature::mod_part_hp_cur( id, set );
    bool is_broken_after = get_part_hp_cur( id ) <= 0;
    // detect unbroken->broken (possible death) as well as broken->unbroken (possible revival)
    if( is_broken_before != is_broken_after ) {
        cached_dead_state.reset();
    }
}

void Character::expose_to_disease( const diseasetype_id &dis_type )
{
    const std::optional<int> &healt_thresh = dis_type->health_threshold;
    if( healt_thresh && healt_thresh.value() < get_lifestyle() ) {
        return;
    }
    const std::set<bodypart_str_id> &bps = dis_type->affected_bodyparts;
    if( !bps.empty() ) {
        for( const bodypart_str_id &bp : bps ) {
            add_effect( dis_type->symptoms, rng( dis_type->min_duration, dis_type->max_duration ), bp.id(),
                        false,
                        rng( dis_type->min_intensity, dis_type->max_intensity ) );
        }
    } else {
        add_effect( dis_type->symptoms, rng( dis_type->min_duration, dis_type->max_duration ),
                    bodypart_str_id::NULL_ID(),
                    false,
                    rng( dis_type->min_intensity, dis_type->max_intensity ) );
    }
}

void Character::recalc_hp()
{
    // Mutated toughness stacks with starting, by design.
    float hp_mod = 1.0f + enchantment_cache->get_value_multiply( enchant_vals::mod::MAX_HP );
    float hp_adjustment = enchantment_cache->get_value_add( enchant_vals::mod::MAX_HP );
    calc_all_parts_hp( hp_mod, hp_adjustment, get_str_base(), get_dex_base(), get_per_base(),
                       get_int_base(), get_lifestyle(), get_fat_to_hp() );
    cached_dead_state.reset();
}

void Character::calc_all_parts_hp( float hp_mod, float hp_adjustment, int str_max, int dex_max,
                                   int per_max, int int_max, int healthy_mod, int fat_to_max_hp )
{
    for( std::pair<const bodypart_str_id, bodypart> &part : body ) {
        int new_max = ( part.first->base_hp + str_max * part.first->hp_mods.str_mod + dex_max *
                        part.first->hp_mods.dex_mod + int_max * part.first->hp_mods.int_mod + per_max *
                        part.first->hp_mods.per_mod + part.first->hp_mods.health_mod * healthy_mod + fat_to_max_hp +
                        hp_adjustment ) * hp_mod;

        if( has_trait( trait_GLASSJAW ) && part.first == body_part_head ) {
            new_max *= 0.8;
        }

        new_max = std::max( 1, new_max );

        float max_hp_ratio = static_cast<float>( new_max ) /
                             static_cast<float>( part.second.get_hp_max() );

        int new_cur = std::ceil( static_cast<float>( part.second.get_hp_cur() ) * max_hp_ratio );

        part.second.set_hp_max( std::max( new_max, 1 ) );
        part.second.set_hp_cur( std::max( std::min( new_cur, new_max ), 0 ) );
    }
}

void Character::conduct_blood_analysis()
{
    std::vector<std::string> effect_descriptions;
    std::vector<nc_color> colors;

    for( auto &elem : *effects ) {
        if( elem.first->get_blood_analysis_description().empty() ) {
            continue;
        }
        effect_descriptions.emplace_back( elem.first->get_blood_analysis_description() );
        colors.emplace_back( elem.first->get_rating() == m_good ? c_green : c_red );
    }

    const int win_w = 46;
    size_t win_h = 0;
    catacurses::window w;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        win_h = std::min( static_cast<size_t>( TERMY ),
                          std::max<size_t>( 1, effect_descriptions.size() ) + 2 );
        w = catacurses::newwin( win_h, win_w,
                                point( ( TERMX - win_w ) / 2, ( TERMY - win_h ) / 2 ) );
        ui.position_from_window( w );
    } );
    ui.mark_resize();
    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border( w, c_red, string_format( " %s ", _( "Blood Test Results" ) ) );
        if( effect_descriptions.empty() ) {
            trim_and_print( w, point( 2, 1 ), win_w - 3, c_white, _( "No effects." ) );
        } else {
            for( size_t line = 1; line < ( win_h - 1 ) && line <= effect_descriptions.size(); ++line ) {
                trim_and_print( w, point( 2, line ), win_w - 3, colors[line - 1], effect_descriptions[line - 1] );
            }
        }
        wnoutrefresh( w );
    } );
    input_context ctxt( "BLOOD_TEST_RESULTS" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    bool stop = false;
    // Display new messages
    g->invalidate_main_ui_adaptor();
    while( !stop ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "CONFIRM" || action == "QUIT" ) {
            stop = true;
        }
    }
}

int Character::get_standard_stamina_cost( const item *thrown_item ) const
{
    // Previously calculated as 2_gram * std::max( 1, str_cur )
    // using 16_gram normalizes it to 8 str. Same effort expenditure
    // for each strike, regardless of weight. This is compensated
    // for by the additional move cost as weapon weight increases
    //If the item is thrown, override with the thrown item instead.

    item current_weapon = used_weapon() ? *used_weapon() : null_item_reference();

    const int weight_cost = ( thrown_item == nullptr ) ? current_weapon.weight() /
                            16_gram : thrown_item->weight() / 16_gram;
    return ( weight_cost + 50 ) * -1 * get_modifier( character_modifier_melee_stamina_cost_mod );
}

// Actual player death is mostly handled in game::is_game_over
void Character::die( map *, Creature *nkiller )
{
    g->set_critter_died();
    set_all_parts_hp_cur( 0 );
    set_killer( nkiller );
    set_time_died( calendar::turn );

    if( has_effect( effect_heavysnare ) ) {
        inv->add_item( item( itype_rope_6, calendar::turn_zero ) );
        inv->add_item( item( itype_snare_trigger, calendar::turn_zero ) );
    }
    if( has_effect( effect_beartrap ) ) {
        inv->add_item( item( itype_beartrap, calendar::turn_zero ) );
    }
    mission::on_creature_death( *this );
}

void Character::prevent_death()
{
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( bp->is_vital ) {
            if( get_part_hp_cur( bp ) <= 0 ) {
                set_part_hp_cur( bp, 1 );
            }
        }
    }
    cached_dead_state.reset();
}

int Character::focus_equilibrium_sleepiness_cap( int equilibrium ) const
{
    if( get_sleepiness() >= sleepiness_levels::MASSIVE_SLEEPINESS && equilibrium > 20 ) {
        return 20;
    } else if( get_sleepiness() >= sleepiness_levels::EXHAUSTED && equilibrium > 40 ) {
        return 40;
    } else if( get_sleepiness() >= sleepiness_levels::DEAD_TIRED && equilibrium > 60 ) {
        return 60;
    } else if( get_sleepiness() >= sleepiness_levels::TIRED && equilibrium > 80 ) {
        return 80;
    }
    return equilibrium;
}

int Character::calc_focus_equilibrium( bool ignore_pain ) const
{
    int focus_equilibrium = 100;

    if( activity.id() == ACT_READ ) {
        const item_location book = activity.targets[0];
        if( book && book->is_book() ) {
            const cata::value_ptr<islot_book> &bt = book->type->book;
            // apply a penalty when we're actually learning something
            const SkillLevel &skill_level = get_skill_level_object( bt->skill );
            if( skill_level.can_train() && skill_level < bt->level ) {
                focus_equilibrium -= 50;
            }
        }
    }

    int eff_morale = get_morale_level();
    // Factor in perceived pain, since it's harder to rest your mind while your body hurts.
    // Cenobites don't mind, though
    if( !ignore_pain && !has_trait( trait_CENOBITE ) ) {
        int perceived_pain = get_perceived_pain();
        if( has_trait( trait_MASOCHIST ) ) {
            if( perceived_pain > 20 ) {
                eff_morale = eff_morale - ( perceived_pain - 20 );
            }
        } else {
            eff_morale = eff_morale - perceived_pain;
        }
    }

    if( eff_morale < -99 ) {
        // At very low morale, focus is at it's minimum
        focus_equilibrium = 1;
    } else if( eff_morale <= 50 ) {
        // At -99 to +50 morale, each point of morale gives or takes 1 point of focus
        focus_equilibrium += eff_morale;
    } else {
        /* Above 50 morale, we apply strong diminishing returns.
        * Each block of 50 takes twice as many morale points as the previous one:
        * 150 focus at 50 morale (as before)
        * 200 focus at 150 morale (100 more morale)
        * 250 focus at 350 morale (200 more morale)
        * ...
        * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
        */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while( focus_equilibrium < 400 ) {
            if( morale_left > 50 * block_multiplier ) {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_equilibrium += 50;
                block_multiplier *= 2;
            } else {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1 focus, and then we're done.
                focus_equilibrium += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if( focus_equilibrium < 1 ) {
        focus_equilibrium = 1;
    } else if( focus_equilibrium > 400 ) {
        focus_equilibrium = 400;
    }
    return focus_equilibrium;
}

int Character::calc_focus_change() const
{
    return focus_equilibrium_sleepiness_cap( calc_focus_equilibrium() ) - get_focus();
}

void Character::update_mental_focus()
{
    // calc_focus_change() returns percentile focus, applying it directly
    // to focus pool is an implicit / 100.
    focus_pool += 10 * calc_focus_change();
}

void Character::calc_discomfort()
{
    // clear all instances of discomfort
    remove_effect( effect_chafing );
    for( const bodypart_id &bp : worn.where_discomfort( *this ) ) {
        if( bp->feels_discomfort ) {
            add_effect( effect_chafing, 1_turns, bp, true, 1 );
        }
    }
}

void Character::apply_murder_penalties( Creature *victim )
{
    Character &player_character = get_player_character();
    // Currently no support for handling anyone else's murdering
    cata_assert( this == &player_character );
    if( !victim || !victim->is_dead_state() || victim->get_killer() != &player_character ) {
        return; // Nothing to do here
    }
    if( player_character.has_trait( trait_PACIFIST ) ) {
        add_msg( _( "A cold shock of guilt washes over you." ) );
        player_character.add_morale( morale_killer_has_killed, -15, 0, 1_days, 1_hours );
    }
    if( victim->as_monster() || ( victim->as_npc() && victim->as_npc()->hit_by_player ) ) {
        int morale_effect = -90;
        // Just because you like eating people doesn't mean you love killing innocents
        if( player_character.has_flag( json_flag_CANNIBAL ) && morale_effect < 0 ) {
            morale_effect = std::min( 0, morale_effect + 50 );
        } // Pacifists double dip on penalties if they kill an innocent
        if( player_character.has_trait( trait_PACIFIST ) ) {
            morale_effect -= 15;
        }
        if( player_character.has_flag( json_flag_PSYCHOPATH ) ||
            player_character.has_flag( json_flag_SAPIOVORE ) ||
            player_character.has_flag( json_flag_NUMB ) ) {
            morale_effect = 0;
        } // only god can juge me
        if( player_character.has_flag( json_flag_SPIRITUAL ) &&
            !player_character.has_flag( json_flag_PSYCHOPATH ) &&
            !player_character.has_flag( json_flag_SAPIOVORE ) &&
            !player_character.has_flag( json_flag_NUMB ) ) {
            add_msg( _( "You feel ashamed of your actions." ) );
            morale_effect -= 10;
        }
        cata_assert( morale_effect <= 0 );
        if( morale_effect == 0 ) {
            // No morale effect
        } else if( morale_effect <= -50 ) {
            player_character.add_morale( morale_killed_innocent, morale_effect, 0, 14_days, 7_days );
        } else if( morale_effect > -50 && morale_effect < 0 ) {
            player_character.add_morale( morale_killed_innocent, morale_effect, 0, 10_days, 7_days );
        }
    }
}

std::pair<int, int> Character::climate_control_strength() const
{
    // In warmth points
    const int DEFAULT_STRENGTH = 50;

    int power_heat = enchantment_cache->get_value_add( enchant_vals::mod::CLIMATE_CONTROL_HEAT );
    int power_chill = enchantment_cache->get_value_add( enchant_vals::mod::CLIMATE_CONTROL_CHILL );

    map &here = get_map();
    if( has_trait( trait_M_SKIN3 ) &&
        here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, pos_bub() ) &&
        in_sleep_state() ) {
        power_heat += DEFAULT_STRENGTH;
        power_chill += DEFAULT_STRENGTH;
    }

    bool regulated_area = false;
    if( calendar::turn >= next_climate_control_check ) {
        // save CPU and simulate acclimation.
        next_climate_control_check = calendar::turn + 20_turns;
        if( const optional_vpart_position vp = here.veh_at( pos_bub() ) ) {
            // TODO: (?) Force player to scrounge together an AC unit
            regulated_area = (
                                 (
                                     vp->is_inside() && // Already checks for opened doors
                                     vp->vehicle().engine_on &&
                                     vp->vehicle().has_engine_type_not( fuel_type_muscle, true ) &&
                                     vp->vehicle().has_engine_type_not( fuel_type_animal, true )
                                 ) ||
                                 // Also check for a working alternator. Muscle or animal could be powering it.
                                 (
                                     vp->is_inside() &&
                                     vp->vehicle().total_alternator_epower( here ) > 0_W
                                 )
                             );
        }
        // TODO: AC check for when building power is implemented
        last_climate_control_ret = regulated_area;
        if( !regulated_area ) {
            // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
            next_climate_control_check += 40_turns;
        }
    } else {
        regulated_area = last_climate_control_ret;
    }

    if( regulated_area ) {
        power_heat += DEFAULT_STRENGTH;
        power_chill += DEFAULT_STRENGTH;
    }

    return { power_heat, power_chill };
}

std::map<bodypart_id, int> Character::get_wind_resistance( const std::map <bodypart_id,
        std::vector<const item *>> &clothing_map ) const
{

    std::map<bodypart_id, int> ret;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        ret.emplace( bp, 0 );
    }
    bool in_shell = has_active_mutation( trait_SHELL2 ) ||
                    has_active_mutation( trait_SHELL3 );
    // Your shell provides complete wind protection if you're inside it
    if( in_shell ) { // NOLINT(bugprone-branch-clone)
        for( std::pair<const bodypart_id, int> &this_bp : ret ) {
            this_bp.second = 100;
        }
        return ret;
    }

    for( const std::pair<const bodypart_id, std::vector<const item *>> &on_bp : clothing_map ) {
        const bodypart_id &bp = on_bp.first;

        int coverage = 0;
        float totalExposed = 1.0f;
        int penalty = 100;

        for( const item *it : on_bp.second ) {
            const item &i = *it;
            penalty = 100 - i.wind_resist();
            coverage = std::max( 0, i.get_coverage( bp ) - penalty );
            totalExposed *= ( 1.0 - coverage / 100.0 ); // Coverage is between 0 and 1?
        }

        ret[bp] = 100 - totalExposed * 100;
    }

    return ret;
}

int Character::get_lifestyle() const
{
    // gets your health_tally variable + the factor of your bmi on your healthiness.

    // being over or underweight makes your "effective healthiness" lower.
    // for example, you have a lifestyle of 0 and a bmi_fat of 15 (border between obese and very obese)
    // 0 - 25 - 0 = -25. So your "effective lifestyle" is -25 (feel a little cruddy)
    // ex2 you have a lifestyle of 50 and a bmi_fat of 15 because you get exercise and eat well
    // 50 - 25 - 0 = 25. So your "effective lifestyle" is 25 (feel decent despite being obese)
    // ex3 you have a lifestyle of -50 and a bmi_fat of 15 because you eat candy and don't exercise
    // -50 - 25 - 0 = -75. So your "effective lifestyle" is -75 (you'd feel crappy normally but because of your weight you feel worse)

    const float bmi = get_bmi_fat();
    int over_factor = std::round( std::max( 0.0f,
                                            5 * ( bmi - character_weight_category::obese ) ) );
    int under_factor = std::round( std::max( 0.0f,
                                   50 * ( character_weight_category::normal - bmi ) ) );
    return std::max( lifestyle - ( over_factor + under_factor ), -200 );
}

int Character::get_daily_health() const
{
    return daily_health;
}

int Character::get_health_tally() const
{
    return health_tally;
}

void Character::set_lifestyle( int nhealthy )
{
    lifestyle = nhealthy;
}
void Character::mod_livestyle( int nhealthy )
{
    lifestyle += enchantment_cache->modify_value( enchant_vals::mod::HEALTHY_RATE, nhealthy );
    // Clamp lifestyle between [-200, 200]
    lifestyle = std::max( lifestyle, -200 );
    lifestyle = std::min( lifestyle, 200 );
}
void Character::set_daily_health( int nhealthy_mod )
{
    daily_health = nhealthy_mod;
}
void Character::mod_daily_health( int nhealthy_mod, int cap )
{
    // TODO: This really should be a full morale-like system, with per-effect caps
    //       and durations.  This version prevents any single effect from exceeding its
    //       intended ceiling, but multiple effects will overlap instead of adding.

    // Cap indicates how far the mod is allowed to shift in this direction.
    // It can have a different sign to the mod, e.g. for items that treat
    // extremely low health, but can't make you healthy.
    if( nhealthy_mod == 0 || cap == 0 ) {
        return;
    }
    int low_cap;
    int high_cap;
    if( nhealthy_mod < 0 ) {
        low_cap = cap;
        high_cap = 200;
    } else {
        low_cap = -200;
        high_cap = cap;
    }

    // If we're already out-of-bounds, we don't need to do anything.
    if( ( daily_health <= low_cap && nhealthy_mod < 0 ) ||
        ( daily_health >= high_cap && nhealthy_mod > 0 ) ) {
        return;
    }

    daily_health += nhealthy_mod;

    // Since we already bailed out if we were out-of-bounds, we can
    // just clamp to the boundaries here.
    daily_health = std::min( daily_health, high_cap );
    daily_health = std::max( daily_health, low_cap );
}

void Character::mod_health_tally( int mod )
{
    health_tally += mod;
}

int Character::get_stored_kcal() const
{
    return stored_calories / 1000;
}

int Character::kcal_speed_penalty() const
{
    static const std::vector<std::pair<float, float>> starv_thresholds = { {
            std::make_pair( 0.0f, -90.0f ),
            std::make_pair( character_weight_category::emaciated, -50.f ),
            std::make_pair( character_weight_category::underweight, -25.0f ),
            std::make_pair( character_weight_category::normal, 0.0f )
        }
    };
    if( get_kcal_percent() > 0.95f ) {
        return 0;
    } else {
        return std::round( multi_lerp( starv_thresholds, get_bmi_fat() ) );
    }
}

std::string Character::debug_weary_info() const
{
    int amt = activity_history.weariness();
    std::string max_act = activity_level_str( maximum_exertion_level() );
    float move_mult = exertion_adjusted_move_multiplier( EXTRA_EXERCISE );

    int bmr = base_bmr();
    std::string weary_internals = activity_history.debug_weary_info();
    int cardio_mult = get_cardiofit();
    int thresh = weary_threshold();
    int current = weariness_level();
    int morale = get_morale_level();
    int weight = units::to_gram<int>( bodyweight() );
    float bmi = get_bmi();

    return string_format( "Weariness: %s Max Full Exert: %s Mult: %g\n BMR: %d CARDIO FITNESS: %d\n %s Thresh: %d At: %d\n Kcal: %d/%d Sleepiness: %d Morale: %d Wgt: %d (BMI %.1f)",
                          amt, max_act, move_mult, bmr, cardio_mult, weary_internals, thresh, current,
                          get_stored_kcal(), get_healthy_kcal(), sleepiness, morale, weight, bmi );
}

void Character::mod_stored_kcal( int nkcal, const bool ignore_weariness )
{
    if( needs_food() ) {
        mod_stored_calories( nkcal * 1000, ignore_weariness );
    }
}

void Character::mod_stored_calories( int ncal, const bool ignore_weariness )
{
    int nkcal = ncal / 1000;
    if( nkcal > 0 ) {
        add_gained_calories( nkcal );
    } else {
        add_spent_calories( -nkcal );
    }

    if( !ignore_weariness ) {
        activity_history.calorie_adjust( ncal );
    }
    if( ncal < 0 || std::numeric_limits<int>::max() - ncal > stored_calories ) {
        set_stored_calories( stored_calories + ncal );
    }
}

void Character::set_stored_kcal( int kcal )
{
    set_stored_calories( kcal * 1000 );
}

void Character::set_stored_calories( int cal )
{
    if( stored_calories != cal ) {
        int cached_bmi = std::floor( get_bmi_fat() );
        stored_calories = cal;

        //some mutant change their max_hp according to their bmi
        recalc_hp();
        //need to check obesity penalties when this happens if BMI changed
        if( std::floor( get_bmi_fat() ) != cached_bmi ) {
            calc_encumbrance();
        }
    }
}

int Character::get_healthy_kcal() const
{
    float healthy_weight = get_cached_organic_size() * 5.0f * std::pow( height() / 100.0f, 2 );
    return std::floor( KCAL_PER_KG * healthy_weight );
}

float Character::get_kcal_percent() const
{
    return static_cast<float>( get_stored_kcal() ) / static_cast<float>( get_healthy_kcal() );
}

int Character::get_hunger() const
{
    return hunger;
}

void Character::mod_hunger( int nhunger )
{
    if( needs_food() ) {
        set_hunger( hunger + nhunger );
    }
}

void Character::set_hunger( int nhunger )
{
    if( hunger != nhunger ) {
        // cap hunger at 300, just below famished
        hunger = std::min( 300, nhunger );
        on_stat_change( "hunger", hunger );
    }
}

// this is a translation from a legacy value
int Character::get_starvation() const
{
    static const std::vector<std::pair<float, float>> starv_thresholds = { {
            std::make_pair( 0.0f, 6000.0f ),
            std::make_pair( 0.8f, 300.0f ),
            std::make_pair( 0.95f, 100.0f )
        }
    };
    if( get_kcal_percent() < 0.95f ) {
        return std::round( multi_lerp( starv_thresholds, get_kcal_percent() ) );
    }
    return 0;
}

int Character::get_thirst() const
{
    return thirst;
}

int Character::get_instant_thirst() const
{
    return thirst - std::max( units::to_milliliter<int>
                              ( stomach.get_water() / 10 ), 0 );
}

void Character::mod_thirst( int nthirst )
{
    if( has_flag( json_flag_NO_THIRST ) || !needs_food() ) {
        return;
    }
    set_thirst( std::max( -100, thirst + nthirst ) );
}

void Character::set_thirst( int nthirst )
{
    if( thirst != nthirst ) {
        thirst = nthirst;
        on_stat_change( "thirst", thirst );
    }
}

void Character::mod_sleepiness( int nsleepiness )
{
    set_sleepiness( sleepiness + nsleepiness );
}

void Character::mod_sleep_deprivation( int nsleep_deprivation )
{
    set_sleep_deprivation( sleep_deprivation + nsleep_deprivation );
}

void Character::set_sleepiness( int nsleepiness )
{
    nsleepiness = std::max( nsleepiness, -1000 );
    if( sleepiness != nsleepiness ) {
        sleepiness = nsleepiness;
        on_stat_change( "sleepiness", sleepiness );
    }
}

void Character::set_sleepiness( sleepiness_levels nsleepiness )
{
    set_sleepiness( static_cast<int>( nsleepiness ) );
}

void Character::set_sleep_deprivation( int nsleep_deprivation )
{
    sleep_deprivation = std::min( static_cast< int >( SLEEP_DEPRIVATION_MASSIVE ), std::max( 0,
                                  nsleep_deprivation ) );
}

time_duration Character::get_daily_sleep() const
{
    return daily_sleep;
}

void Character::mod_daily_sleep( time_duration mod )
{
    daily_sleep += mod;
}

void Character::reset_daily_sleep()
{
    daily_sleep = 0_turns;
}

time_duration Character::get_continuous_sleep() const
{
    return continuous_sleep;
}

void Character::mod_continuous_sleep( time_duration mod )
{
    continuous_sleep += mod;
}

void Character::reset_continuous_sleep()
{
    continuous_sleep = 0_turns;
}

int Character::get_sleepiness() const
{
    return sleepiness;
}

int Character::get_sleep_deprivation() const
{
    return sleep_deprivation;
}

bool Character::is_deaf() const
{
    return get_effect_int( effect_deaf ) > 2 || worn_with_flag( flag_DEAF ) ||
           has_flag( json_flag_DEAF ) || has_effect( effect_narcosis ) ||
           ( has_trait( trait_M_SKIN3 ) &&
             get_map().has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, pos_bub() )
             && in_sleep_state() );
}

bool Character::is_mute() const
{
    return has_flag( flag_MUTE ) || worn_with_flag( flag_MUTE ) ||
           ( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_foodperson_mask ) ||
                   is_wearing( itype_foodperson_mask_on ) ) );
}

void Character::on_damage_of_type( const effect_source &source, int adjusted_damage,
                                   const damage_type_id &dt, const bodypart_id &bp )
{
    // Handle bp onhit effects
    bodypart *body_part = get_part( bp );
    const bool is_minor = bp->main_part != bp->id;
    add_msg_debug( debugmode::DF_CHARACTER, "Targeting limb %s with %d %s type damage", bp->name,
                   adjusted_damage, dt->name.translated() );
    // Damage as a percent of the limb's max hp or raw dmg for minor parts
    float dmg_ratio = is_minor ? adjusted_damage : 100 * adjusted_damage / body_part->get_hp_max();
    add_msg_debug( debugmode::DF_CHARACTER, "Main BP max hp %d, damage ratio %.1f",
                   body_part->get_hp_max(), dmg_ratio );
    for( const bp_onhit_effect &eff : body_part->get_onhit_effects( dt ) ) {
        if( dmg_ratio >= eff.dmg_threshold ) {
            float scaling = ( dmg_ratio - eff.dmg_threshold ) / eff.scale_increment;
            int scaled_chance = roll_remainder( std::max( static_cast<float>( eff.chance ),
                                                eff.chance + eff.chance_dmg_scaling * scaling ) );
            add_msg_debug( debugmode::DF_CHARACTER, "Scaling factor %.1f, base chance %d%%, scaled chance %d%%",
                           scaling, eff.chance, scaled_chance );
            if( x_in_y( scaled_chance, 100 ) ) {

                // Scale based on damage done
                int scaled_dur = roll_remainder( std::max( static_cast<float>( eff.duration ),
                                                 eff.duration + eff.duration_dmg_scaling * scaling ) );
                int scaled_int = roll_remainder( std::max( static_cast<float>( eff.intensity ),
                                                 eff.intensity + eff.intensity_dmg_scaling * scaling ) );

                // go through all effects. if they modify effect duration, scale dur accordingly.
                // obviously inefficient, but probably fast enough since this only is calculated once per attack.
                float eff_scale = 1.0f;
                for( const effect &e : get_effects() ) {
                    for( const effect_dur_mod &eff_dur_mod : e.get_effect_dur_scaling() ) {
                        // debugmsg( "Checking effect %s, with same_bp, modifier %f", eff.id.c_str(), eff_dur_mod.modifier);
                        if( eff_dur_mod.effect_id == eff.id ) {
                            if( eff_dur_mod.same_bp ) {
                                if( e.get_bp() != body_part->get_id() ) {
                                    continue;
                                }
                            }
                            eff_scale *= eff_dur_mod.modifier;
                        }
                    }
                }

                scaled_dur = static_cast<int>( scaled_dur * eff_scale );

                add_msg_debug( debugmode::DF_CHARACTER,
                               "Atempting to add effect %s, scaled duration %d turns, scaled intensity %d, effect scaling %f",
                               eff.id.c_str(),
                               scaled_dur,
                               scaled_int,
                               eff_scale );

                // Handle intensity/duration clamping
                if( eff.max_intensity != INT_MAX ) {
                    int prev_int = get_effect( eff.id, bp ).get_intensity();
                    add_msg_debug( debugmode::DF_CHARACTER,
                                   "Max intensity %d, current intensity %d", eff.max_intensity, prev_int );
                    if( ( prev_int + scaled_int ) > eff.max_intensity ) {
                        scaled_int = eff.max_intensity - prev_int;
                        add_msg_debug( debugmode::DF_CHARACTER,
                                       "Limiting added intensity to %d", scaled_int );
                    }
                }
                if( eff.max_duration != INT_MAX ) {
                    int prev_dur = to_turns<int>( get_effect( eff.id, bp ).get_duration() );
                    add_msg_debug( debugmode::DF_CHARACTER,
                                   "Max duration %d, current duration %d", eff.max_duration, prev_dur );
                    if( ( prev_dur + scaled_dur ) > eff.max_duration ) {
                        scaled_dur = eff.max_duration - prev_dur;
                        add_msg_debug( debugmode::DF_CHARACTER,
                                       "Limiting added duration to %d seconds", scaled_dur );
                    }
                }
                // Add the effect globally if defined
                add_effect( source, eff.id, 1_turns * scaled_dur, eff.global ? bodypart_str_id::NULL_ID() : bp,
                            false, scaled_int );
            }
        }
    }
    // Electrical damage has a chance to temporarily incapacitate bionics in the damaged body_part.
    if( dt == damage_electric ) {
        const time_duration min_disable_time = 10_turns * adjusted_damage;
        for( bionic &i : *my_bionics ) {
            if( !i.powered ) {
                // Unpowered bionics are protected from power surges.
                continue;
            }
            const bionic_data &info = i.info();
            if( info.has_flag( json_flag_BIONIC_SHOCKPROOF ) ||
                info.has_flag( json_flag_BIONIC_FAULTY ) ) {
                continue;
            }
            const std::map<bodypart_str_id, size_t> &bodyparts = info.occupied_bodyparts;
            if( bodyparts.find( bp.id() ) != bodyparts.end() ) {
                const int bp_hp = get_part_hp_cur( bp );
                // The chance to incapacitate is as high as 50% if the attack deals damage equal to one third of the body part's current health.
                if( x_in_y( adjusted_damage * 3, bp_hp ) && one_in( 2 ) ) {
                    if( i.incapacitated_time == 0_turns ) {
                        add_msg_if_player( m_bad, _( "Your %s bionic shorts out!" ), info.name );
                    }
                    i.incapacitated_time += rng( min_disable_time, 10 * min_disable_time );
                }
            }
        }
    }
}

int Character::get_max_healthy() const
{
    const float bmi = get_bmi_fat();
    int over_factor = std::round( std::max( 0.0f,
                                            25 * ( bmi - character_weight_category::overweight ) ) );
    int under_factor = std::round( std::max( 0.0f,
                                   200 * ( character_weight_category::normal - bmi ) ) );
    return std::max( 200 - over_factor - under_factor, -200 );
}

void Character::regen( int rate_multiplier )
{
    int pain_ticks = std::max( 1, roll_remainder( enchantment_cache->modify_value(
                                   enchant_vals::mod::PAIN_REMOVE, rate_multiplier ) ) );
    while( get_pain() > 0 && pain_ticks-- > 0 ) {
        mod_pain( -roll_remainder( 0.2f + get_pain() / 50.0f ) );
    }

    float rest = rest_quality();
    float heal_rate = healing_rate( rest ) * to_turns<int>( 5_minutes );
    if( heal_rate > 0.0f ) {
        healall( roll_remainder( rate_multiplier * heal_rate ) );
    } else if( heal_rate < 0.0f ) {
        int rot_rate = roll_remainder( rate_multiplier * -heal_rate );
        // Has to be in loop because some effects depend on rounding
        while( rot_rate-- > 0 ) {
            hurtall( 1, nullptr, false );
        }
    }

    // include healing effects
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        float healing = healing_rate_medicine( rest, bp ) * to_turns<int>( 5_minutes );
        int healing_apply = roll_remainder( healing );
        mod_part_healed_total( bp, healing_apply );
        heal( bp, healing_apply );
        // Consume 1 "health" for every Hit Point healed via medicine healing.
        // This has a significant effect on long-term healing.
        mod_daily_health( -healing_apply, -200 );
        if( get_part_damage_bandaged( bp ) > 0 ) {
            mod_part_damage_bandaged( bp, -healing_apply );
            if( get_part_damage_bandaged( bp ) <= 0 ) {
                set_part_damage_bandaged( bp, 0 );
                remove_effect( effect_bandaged, bp );
                add_msg_if_player( _( "Bandaged wounds on your %s healed." ), body_part_name( bp ) );
            }
        }
        if( get_part_damage_disinfected( bp ) > 0 ) {
            mod_part_damage_disinfected( bp, -healing_apply );
            if( get_part_damage_disinfected( bp ) <= 0 ) {
                set_part_damage_disinfected( bp, 0 );
                remove_effect( effect_disinfected, bp );
                add_msg_if_player( _( "Disinfected wounds on your %s healed." ), body_part_name( bp ) );
            }
        }

        // remove effects if the limb was healed by other way
        if( has_effect( effect_bandaged, bp.id() ) && is_part_at_max_hp( bp ) ) {
            set_part_damage_bandaged( bp, 0 );
            remove_effect( effect_bandaged, bp );
            add_msg_if_player( _( "Bandaged wounds on your %s healed." ), body_part_name( bp ) );
        }
        if( has_effect( effect_disinfected, bp.id() ) && is_part_at_max_hp( bp ) ) {
            set_part_damage_disinfected( bp, 0 );
            remove_effect( effect_disinfected, bp );
            add_msg_if_player( _( "Disinfected wounds on your %s healed." ), body_part_name( bp ) );
        }
    }

    if( get_rad() > 0 ) {
        mod_rad( -roll_remainder( rate_multiplier / 50.0f ) );
    }
}

void Character::update_health()
{
    // Limit daily_health to [-200, 200].
    // This also sets approximate bounds for the character's health.
    if( get_daily_health() > 200 ) {
        set_daily_health( 200 );
    } else if( get_daily_health() < -200 ) {
        set_daily_health( -200 );
    }

    // Active leukocyte breeder will keep your mean health_tally near 100
    int effective_daily_health = enchantment_cache->modify_value(
                                     enchant_vals::mod::EFFECTIVE_HEALTH_MOD, 0 );
    if( effective_daily_health == 0 ) {
        effective_daily_health = get_daily_health();
    }

    if( calendar::once_every( 1_days ) ) {
        mod_health_tally( effective_daily_health );
        int mean_daily_health = get_health_tally() / 7;
        mod_livestyle( mean_daily_health );
        mod_health_tally( -mean_daily_health );
        set_daily_health( 0 );
    }
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "Lifestyle: %d, Daily health: %d", get_lifestyle(),
                   get_daily_health() );
}

int Character::weariness() const
{
    return activity_history.weariness();
}

int Character::weary_threshold() const
{
    const int bmr = base_bmr();
    int threshold = bmr * get_option<float>( "WEARY_BMR_MULT" );
    // reduce by 1% per 14 points of sleepiness after 150 points
    threshold *= 1.0f - ( ( std::max( sleepiness, -20 ) - 150 ) / 1400.0f );
    // Each 2 points of morale increase or decrease by 1%
    threshold *= 1.0f + ( get_morale_level() / 200.0f );
    // TODO: Hunger effects this

    return std::max( threshold, bmr / 10 );
}

std::pair<int, int> Character::weariness_transition_progress() const
{
    // Mostly a duplicate of the below function. No real way to clean this up
    int amount = weariness();
    int threshold = weary_threshold();
    amount -= threshold * get_option<float>( "WEARY_INITIAL_STEP" );
    // failsafe if threshold is zero; see #72242
    if( threshold == 0 ) {
        return { std::abs( amount ), threshold };
    } else {
        while( amount >= 0 ) {
            amount -= threshold;
            if( threshold > 20 ) {
                threshold *= get_option<float>( "WEARY_THRESH_SCALING" );
            }
        }
    }

    // If we return the absolute value of the amount, it will work better
    // Because as it decreases, we will approach a transition
    return {std::abs( amount ), threshold};
}

int Character::weariness_level() const
{
    int amount = weariness();
    int threshold = weary_threshold();
    int level = 0;
    amount -= threshold * get_option<float>( "WEARY_INITIAL_STEP" );
    // failsafe if threshold is zero; see #72242
    if( threshold == 0 ) {
        return level;
    } else {
        while( amount >= 0 ) {
            amount -= threshold;
            if( threshold > 20 ) {
                threshold *= get_option<float>( "WEARY_THRESH_SCALING" );
            }
            ++level;
        }
    }

    return level;
}


int Character::weariness_transition_level() const
{
    int amount = weariness();
    int threshold = weary_threshold();
    amount -= threshold * get_option<float>( "WEARY_INITIAL_STEP" );
    // failsafe if threshold is zero; see #72242
    if( threshold == 0 ) {
        return std::abs( amount );
    } else {
        while( amount >= 0 ) {
            amount -= threshold;
            if( threshold > 20 ) {
                threshold *= get_option<float>( "WEARY_THRESH_SCALING" );
            }
        }
    }

    return std::abs( amount );
}

float Character::maximum_exertion_level() const
{
    switch( weariness_level() ) {
        case 0:
            return EXTRA_EXERCISE;
        case 1:
            return ACTIVE_EXERCISE;
        case 2:
            return BRISK_EXERCISE;
        case 3:
            return MODERATE_EXERCISE;
        case 4:
            return LIGHT_EXERCISE;
        case 5:
        default:
            return NO_EXERCISE;
    }
}

float Character::exertion_adjusted_move_multiplier( float level ) const
{
    // The default value for level is -1.0
    // And any values we get that are negative or 0
    // will cause incorrect behavior
    if( level <= 0 ) {
        level = activity_history.activity( in_sleep_state() );
    }
    const float max = maximum_exertion_level();
    if( level < max ) {
        return 1.0f;
    }
    return max / level;
}

bool Character::needs_food() const
{
    return !( is_npc() && get_option<bool>( "NO_NPC_FOOD" ) );
}

void Character::update_needs( int rate_multiplier )
{
    const int current_stim = get_stim();
    // Hunger, thirst, & sleepiness up every 5 minutes
    effect &sleep = get_effect( effect_sleep );
    // No food/thirst/sleepiness clock at all
    const bool debug_ls = has_trait( trait_DEBUG_LS );
    // No food/thirst, capped sleepiness clock (only up to tired)
    const bool asleep = !sleep.is_null();
    const bool lying = asleep || has_effect( effect_lying_down ) ||
                       activity.id() == ACT_TRY_SLEEP;

    needs_rates rates = calc_needs_rates();

    const bool wasnt_sleepinessd = get_sleepiness() <= sleepiness_levels::DEAD_TIRED;
    // Don't increase sleepiness if sleeping or trying to sleep or if we're at the cap.
    if( get_sleepiness() < 1050 && !asleep && !debug_ls ) {
        if( rates.sleepiness > 0.0f ) {
            int sleepiness_roll = roll_remainder( rates.sleepiness * rate_multiplier );
            mod_sleepiness( sleepiness_roll );

            // Synaptic regen bionic stops SD while awake and boosts it while sleeping
            if( !has_flag( json_flag_STOP_SLEEP_DEPRIVATION ) ) {
                // sleepiness_roll should be around 1 - so the counter increases by 1 every minute on average,
                // but characters who need less sleep will also get less sleep deprived, and vice-versa.

                // Note: Since needs are updated in 5-minute increments, we have to multiply the roll again by
                // 5. If rate_multiplier is > 1, sleepiness_roll will be higher and this will work out.
                mod_sleep_deprivation( sleepiness_roll * 5 );
            }

            if( !needs_food() && get_sleepiness() > sleepiness_levels::TIRED ) {
                set_sleepiness( sleepiness_levels::TIRED );
                set_sleep_deprivation( 0 );
            }
        }
    } else if( asleep ) {
        if( rates.recovery > 0.0f ) {
            int recovered = roll_remainder( rates.recovery * rate_multiplier );
            if( get_sleepiness() - recovered < -20 ) {
                // Should be wake up, but that could prevent some retroactive regeneration
                sleep.set_duration( 1_turns );
                mod_sleepiness( -25 );
            } else {
                if( has_effect( effect_disrupted_sleep ) || has_effect( effect_recently_coughed ) ) {
                    recovered *= .5;
                }
                mod_sleepiness( -recovered );

                // Sleeping on the ground, no bionic = 1x rest_modifier
                // Sleeping on a bed, no bionic      = 2x rest_modifier
                // Sleeping on a comfy bed, no bionic= 3x rest_modifier

                // Sleeping on the ground, bionic    = 3x rest_modifier
                // Sleeping on a bed, bionic         = 6x rest_modifier
                // Sleeping on a comfy bed, bionic   = 9x rest_modifier
                float rest_modifier = ( has_flag( json_flag_STOP_SLEEP_DEPRIVATION ) ? 3 : 1 );
                // Melatonin supplements also add a flat bonus to recovery speed
                if( has_effect( effect_melatonin ) ) {
                    rest_modifier += 1;
                }

                const int comfort = get_comfort_at( pos_bub() ).comfort;
                if( comfort >= comfort_data::COMFORT_VERY_COMFORTABLE ) {
                    rest_modifier *= 3;
                } else if( comfort >= comfort_data::COMFORT_COMFORTABLE ) {
                    rest_modifier *= 2.5;
                } else if( comfort >= comfort_data::COMFORT_SLIGHTLY_COMFORTABLE ) {
                    rest_modifier *= 2;
                }

                // If we're just tired, we'll get a decent boost to our sleep quality.
                // The opposite is true for very tired characters.
                if( get_sleepiness() < sleepiness_levels::DEAD_TIRED ) {
                    rest_modifier += 2;
                } else if( get_sleepiness() >= sleepiness_levels::EXHAUSTED ) {
                    rest_modifier = ( rest_modifier > 2 ) ? rest_modifier - 2 : 1;
                }

                // Recovered is multiplied by 2 as well, since we spend 1/3 of the day sleeping
                mod_sleep_deprivation( -rest_modifier * ( recovered * 2 ) );

            }
        }
        if( calendar::once_every( 10_minutes ) &&
            ( has_trait( trait_WATERSLEEP ) || has_trait( trait_UNDINE_SLEEP_WATER ) ) ) {
            mod_sleepiness( -3 ); // Fish sleep less in water
        }
    }
    if( is_avatar() && wasnt_sleepinessd && get_sleepiness() > sleepiness_levels::DEAD_TIRED &&
        !lying ) {
        if( !activity ) {
            add_msg_if_player( m_warning, _( "You're feeling tired.  %s to lie down for sleep." ),
                               press_x( ACTION_SLEEP ) );
        } else {
            g->cancel_activity_query( _( "You're feeling tired." ) );
        }
    }

    if( current_stim < 0 ) {
        set_stim( std::min( current_stim + rate_multiplier, 0 ) );
    } else if( current_stim > 0 ) {
        set_stim( std::max( current_stim - rate_multiplier, 0 ) );
    }

    if( get_painkiller() > 0 ) {
        mod_painkiller( -std::min( get_painkiller(), rate_multiplier ) );
    }

    if( get_bp_effect_mod() > 0 ) {
        modify_bp_effect_mod( -std::min( get_bp_effect_mod(), rate_multiplier ) );
    } else if( get_bp_effect_mod() < 0 ) {
        modify_bp_effect_mod( std::min( -get_bp_effect_mod(), rate_multiplier ) );
    }

    if( get_heartrate_effect_mod() > 0 ) {
        modify_heartrate_effect_mod( -std::min( get_heartrate_effect_mod(), rate_multiplier ) );
    } else if( get_heartrate_effect_mod() < 0 ) {
        modify_heartrate_effect_mod( std::min( -get_heartrate_effect_mod(), rate_multiplier ) );
    }

    if( get_respiration_effect_mod() > 0 ) {
        modify_respiration_effect_mod( -std::min( get_respiration_effect_mod(), rate_multiplier ) );
    } else if( get_respiration_effect_mod() < 0 ) {
        modify_respiration_effect_mod( std::min( -get_respiration_effect_mod(), rate_multiplier ) );
    }

}

needs_rates Character::calc_needs_rates() const
{
    const effect &sleep = get_effect( effect_sleep );
    const bool asleep = !sleep.is_null();

    needs_rates rates;
    rates.hunger = metabolic_rate();

    rates.kcal = get_bmr();

    add_msg_debug_if( is_avatar(), debugmode::DF_CHAR_CALORIES, "Metabolic rate: %.2f", rates.hunger );

    static const std::string player_thirst_rate( "PLAYER_THIRST_RATE" );
    rates.thirst = get_option< float >( player_thirst_rate );

    static const std::string player_sleepiness_rate( "PLAYER_SLEEPINESS_RATE" );
    rates.sleepiness = get_option< float >( player_sleepiness_rate );

    if( asleep ) {
        calc_sleep_recovery_rate( rates );
    } else {
        rates.recovery = 0;
    }

    if( has_activity( ACT_TREE_COMMUNION ) ) {
        // Much of the body's needs are taken care of by the trees.
        // Hair Roots don't provide any bodily needs.
        if( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) {
            rates.hunger *= 0.5f;
            rates.thirst *= 0.5f;
            rates.sleepiness *= 0.5f;
        }
    }

    if( has_trait( trait_TRANSPIRATION ) ) {
        // Transpiration, the act of moving nutrients with evaporating water, can take a very heavy toll on your thirst when it's really hot.
        rates.thirst *= ( ( units::to_fahrenheit( get_weather().get_temperature(
                                pos_bub() ) ) - 32.5f ) / 40.0f );
    }

    rates.hunger = enchantment_cache->modify_value( enchant_vals::mod::HUNGER, rates.hunger );
    rates.sleepiness = enchantment_cache->modify_value( enchant_vals::mod::SLEEPINESS,
                       rates.sleepiness );
    rates.thirst = enchantment_cache->modify_value( enchant_vals::mod::THIRST, rates.thirst );

    return rates;
}

void Character::calc_sleep_recovery_rate( needs_rates &rates ) const
{
    const effect &sleep = get_effect( effect_sleep );
    rates.recovery = enchantment_cache->modify_value( enchant_vals::mod::SLEEPINESS_REGEN, 1 );

    // -5% sleep recovery rate for every main part below cold
    float temp_mod = 0.0f;
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( get_part_temp_cur( bp ) <= BODYTEMP_COLD ) {
            temp_mod -= 0.05f;
        }
    }
    rates.recovery += temp_mod;

    if( !is_hibernating() ) {
        // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
        rates.hunger *= 0.5f;
        rates.thirst *= 0.5f;
        const int intense = sleep.is_null() ? 0 : sleep.get_intensity();
        // Accelerated recovery capped to 2x over 2 hours
        // After 16 hours of activity, equal to 7.25 hours of rest
        const int accelerated_recovery_chance = 24 - intense + 1;
        const float accelerated_recovery_rate = 1.0f / accelerated_recovery_chance;
        rates.recovery += accelerated_recovery_rate;
    } else {
        // Hunger and thirst advance *much* more slowly whilst we hibernate.
        rates.hunger *= ( 2.0f / 7.0f );
        rates.thirst *= ( 2.0f / 7.0f );
    }
    rates.recovery -= static_cast<float>( get_perceived_pain() ) / 60;
}

void Character::check_needs_extremes()
{
    // Check if we've overdosed... in any deadly way.
    if( get_stim() > 250 ) {
        add_msg_player_or_npc( m_bad,
                               _( "You have a sudden heart attack!" ),
                               _( "<npcname> has a sudden heart attack!" ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), efftype_id() );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_stim() < -200 || get_painkiller() > 240 ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your breathing stops completely." ),
                               _( "<npcname>'s breathing stops completely." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), efftype_id() );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( has_effect( effect_jetinjector ) && get_effect_dur( effect_jetinjector ) > 40_minutes ) {
        if( !has_flag( json_flag_PAIN_IMMUNE ) ) {
            add_msg_player_or_npc( m_bad,
                                   _( "Your heart spasms painfully and stops." ),
                                   _( "<npcname>'s heart spasms painfully and stops." ) );
        } else {
            add_msg_player_or_npc( _( "Your heart spasms and stops." ),
                                   _( "<npcname>'s heart spasms and stops." ) );
        }
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_jetinjector );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_effect_dur( effect_adrenaline ) > 50_minutes ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your heart spasms and stops." ),
                               _( "<npcname>'s heart spasms and stops." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_adrenaline );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_effect_int( effect_drunk ) > 4 ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your breathing slows down to a stop." ),
                               _( "<npcname>'s breathing slows down to a stop." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_drunk );
        set_part_hp_cur( body_part_torso, 0 );
    }

    // check if we've starved
    if( needs_food() ) {
        if( get_stored_kcal() <= 0 ) {
            add_msg_if_player( m_bad, _( "You have starved to death." ) );
            get_event_bus().send<event_type::dies_of_starvation>( getID() );
            set_part_hp_cur( body_part_torso, 0 );
        } else {
            if( calendar::once_every( 12_hours ) ) {
                std::string category;
                if( stomach.contains() <= stomach.capacity( *this ) / 4 ) {
                    if( get_kcal_percent() < 0.1f ) {
                        category = "starving";
                    } else if( get_kcal_percent() < 0.25f ) {
                        category = "emaciated";
                    } else if( get_kcal_percent() < 0.5f ) {
                        category = "malnutrition";
                    } else if( get_kcal_percent() < 0.8f ) {
                        category = "low_cal";
                    }
                } else {
                    if( get_kcal_percent() < 0.1f ) {
                        category = "empty_starving";
                    } else if( get_kcal_percent() < 0.25f ) {
                        category = "empty_emaciated";
                    } else if( get_kcal_percent() < 0.5f ) {
                        category = "empty_malnutrition";
                    } else if( get_kcal_percent() < 0.8f ) {
                        category = "empty_low_cal";
                    }
                }
                if( !category.empty() ) {
                    const translation message = SNIPPET.random_from_category( category ).value_or( translation() );
                    add_msg_if_player( m_warning, message );
                }

            }
        }
    }

    // Check if we're dying of thirst
    if( needs_food() && get_thirst() >= 600 && ( stomach.get_water() == 0_ml ||
            guts.get_water() == 0_ml ) ) {
        if( get_thirst() >= 1200 ) {
            add_msg_if_player( m_bad, _( "You have died of dehydration." ) );
            get_event_bus().send<event_type::dies_of_thirst>( getID() );
            set_part_hp_cur( body_part_torso, 0 );
        } else if( get_thirst() >= 1000 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Even your eyes feel dry…" ) );
        } else if( get_thirst() >= 800 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "You are THIRSTY!" ) );
        } else if( calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Your mouth feels so dry…" ) );
        }
    }

    // Check if we're falling asleep, unless we're sleeping
    if( get_sleepiness() >= sleepiness_levels::EXHAUSTED + 25 && !in_sleep_state() ) {
        if( get_sleepiness() >= sleepiness_levels::MASSIVE_SLEEPINESS ) {
            add_msg_if_player( m_bad, _( "Survivor sleep now." ) );
            get_event_bus().send<event_type::falls_asleep_from_exhaustion>( getID() );
            mod_sleepiness( -10 );
            fall_asleep();
        } else if( get_sleepiness() >= 800 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Anywhere would be a good place to sleep…" ) );
        } else if( calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "You feel like you haven't slept in days." ) );
        }
    }

    // Even if we're not Exhausted, we really should be feeling lack/sleep earlier
    // Penalties start at Dead Tired and go from there
    if( get_sleepiness() >= sleepiness_levels::DEAD_TIRED && !in_sleep_state() ) {
        if( get_sleepiness() >= 700 ) {
            if( calendar::once_every( 30_minutes ) ) {
                add_msg_if_player( m_warning, _( "You're too physically tired to stop yawning." ) );
                add_effect( effect_lack_sleep, 30_minutes + 1_turns );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when dead tired */
            if( one_in( 50 + int_cur ) ) {
                // Rivet's idea: look out for microsleeps!
                fall_asleep( 30_seconds );
            }
        } else if( get_sleepiness() >= sleepiness_levels::EXHAUSTED ) {
            if( calendar::once_every( 30_minutes ) ) {
                add_msg_if_player( m_warning, _( "How much longer until bedtime?" ) );
                add_effect( effect_lack_sleep, 30_minutes + 1_turns );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when exhausted */
            if( one_in( 100 + int_cur ) ) {
                fall_asleep( 30_seconds );
            }
        } else if( get_sleepiness() >= sleepiness_levels::DEAD_TIRED &&
                   calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "*yawn* You should really get some sleep." ) );
            add_effect( effect_lack_sleep, 30_minutes + 1_turns );
        }
    }

    // Sleep deprivation kicks in if lack of sleep is avoided with stimulants or otherwise for long periods of time
    int sleep_deprivation = get_sleep_deprivation();
    float sleep_deprivation_pct = sleep_deprivation / static_cast<float>( SLEEP_DEPRIVATION_MASSIVE );

    if( sleep_deprivation >= SLEEP_DEPRIVATION_HARMLESS && !in_sleep_state() ) {
        if( calendar::once_every( 60_minutes ) ) {
            if( sleep_deprivation < SLEEP_DEPRIVATION_MINOR ) {
                add_msg_if_player( m_warning,
                                   _( "Your mind feels tired.  It's been a while since you've slept well." ) );
                mod_sleepiness( 1 );
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_SERIOUS ) {
                add_msg_if_player( m_bad,
                                   _( "Your mind feels foggy from a lack of good sleep, and your eyes keep trying to close against your will." ) );
                mod_sleepiness( 5 );

                if( one_in( 10 ) ) {
                    mod_daily_health( -1, -10 );
                }
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_MAJOR ) {
                add_msg_if_player( m_bad,
                                   _( "Your mind feels weary, and you dread every wakeful minute that passes.  You crave sleep, and feel like you're about to collapse." ) );
                mod_sleepiness( 10 );

                if( one_in( 5 ) ) {
                    mod_daily_health( -2, -10 );
                }
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_MASSIVE ) {
                add_msg_if_player( m_bad,
                                   _( "You haven't slept decently for so long that your whole body is screaming for mercy.  It's a miracle that you're still awake, but it feels more like a curse now." ) );
                mod_sleepiness( 40 );

                mod_daily_health( -5, -10 );
            }
            // else you pass out for 20 hours, guaranteed

            // Microsleeps are slightly worse if you're sleep deprived, but not by much. (chance: 1 in (75 + int_cur) at lethal sleep deprivation)
            // Note: these can coexist with sleepiness-related microsleeps
            /** @EFFECT_INT slightly decreases occurrence of short naps when sleep deprived */
            if( one_in( static_cast<int>( sleep_deprivation_pct * 75 ) + int_cur ) ) {
                fall_asleep( 30_seconds );
            }

            // Stimulants can be used to stay awake a while longer, but after a while you'll just collapse.
            bool can_pass_out = ( get_stim() < 30 && sleep_deprivation >= SLEEP_DEPRIVATION_MINOR ) ||
                                sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR;

            if( can_pass_out && calendar::once_every( 10_minutes ) ) {
                /** @EFFECT_PER slightly increases resilience against passing out from sleep deprivation */
                if( one_in( static_cast<int>( ( 1 - sleep_deprivation_pct ) * 100 ) + per_cur ) ||
                    sleep_deprivation >= SLEEP_DEPRIVATION_MASSIVE ) {
                    add_msg_player_or_npc( m_bad,
                                           _( "Your body collapses due to sleep deprivation, your neglected fatigue rushing back all at once, and you pass out on the spot." )
                                           , _( "<npcname> collapses to the ground from exhaustion." ) );
                    if( get_sleepiness() < sleepiness_levels::EXHAUSTED ) {
                        set_sleepiness( sleepiness_levels::EXHAUSTED );
                    }

                    if( sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR ) {
                        fall_asleep( 20_hours );
                    } else if( sleep_deprivation >= SLEEP_DEPRIVATION_SERIOUS ) {
                        fall_asleep( 16_hours );
                    } else {
                        fall_asleep( 12_hours );
                    }
                }
            }

        }
    }
}

void Character::get_sick( bool is_flu )
{
    // Health is in the range [-200,200].
    // Diseases are half as common for every 50 health you gain.
    const float health_factor = std::pow( 2.0f, get_lifestyle() / 50.0f );
    const float env_factor = 2.0f + std::pow( get_env_resist( body_part_mouth ), 0.3f );
    const int disease_rarity = static_cast<int>( health_factor + env_factor );
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "disease_rarity = %d", disease_rarity );

    if( one_in( disease_rarity ) ) {
        // Normal people get sick about 2-4 times/year. If they are disease resistant, it's 1 time per year.
        const int base_diseases_per_year = has_trait( trait_DISRESISTANT ) ? 1 : 3;
        const time_duration immunity_duration = calendar::season_length() * 4 / base_diseases_per_year;

        bool already_has_flu = has_effect( effect_flu ) ||
                               count_queued_effects( "pre_flu" ) ||
                               count_queued_effects( "flu" );
        bool already_has_cold = has_effect( effect_common_cold ) ||
                                count_queued_effects( "pre_common_cold" ) ||
                                count_queued_effects( "common_cold" );

        if( is_flu && !already_has_flu ) {
            // Total duration of symptoms
            time_duration total_duration = rng( 3_days, 10_days );
            // Time before first symptoms
            time_duration incubation_period = rng( 18_hours, 36_hours );
            // Time from first symptoms to full blown disease
            time_duration warning_period = std::min( rng( 18_hours, 36_hours ), total_duration );

            add_msg_debug( debugmode::DF_CHAR_HEALTH, "Queuing flu: incubation %d h, warning %d h, total %d h",
                           to_hours<int>( incubation_period ),
                           to_hours<int>( warning_period ),
                           to_hours<int>( total_duration ) );

            queue_effect( "pre_flu", incubation_period, warning_period );
            queue_effect( "flu", incubation_period + warning_period, total_duration - warning_period );
            queue_effect( "flushot", incubation_period + total_duration, immunity_duration );

        } else if( !already_has_cold ) {
            // Total duration of symptoms
            time_duration total_duration = rng( 1_days, 14_days );
            // Time before first symptoms
            time_duration incubation_period = rng( 18_hours, 36_hours );
            // Time from first symptoms to full blown disease
            time_duration warning_period = std::min( rng( 18_hours, 36_hours ), total_duration );

            add_msg_debug( debugmode::DF_CHAR_HEALTH, "Queuing cold: incubation %d h, warning %d h, total %d h",
                           to_hours<int>( incubation_period ),
                           to_hours<int>( warning_period ),
                           to_hours<int>( total_duration ) );

            queue_effect( "pre_common_cold", incubation_period, warning_period );
            queue_effect( "common_cold", incubation_period + warning_period, total_duration - warning_period );
            queue_effect( "common_cold_immunity", incubation_period + total_duration, immunity_duration );
        }
    }
}

bool Character::is_hibernating() const
{
    // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
    // as a safety catch.  One test subject managed to get two Colds during hibernation;
    // since those add sleepiness and dry out the character, the subject went for the full 10 days plus
    // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
    // life like that--but since there's much less fluid reserve than food reserve,
    // simply using the same numbers won't work.
    return has_effect( effect_sleep ) && get_kcal_percent() > 0.8f &&
           get_thirst() <= 80 && has_active_mutation( trait_HIBERNATE );
}

bool Character::is_blind() const
{
    return worn_with_flag( flag_BLIND ) ||
           has_flag( json_flag_BLIND ) || get_limb_score( limb_score_vision ) <= 0;
}

float Character::rest_quality() const
{
    map &here = get_map();
    const tripoint_bub_ms your_pos = pos_bub();
    float rest = 0.0f;
    const float ur_act_level = instantaneous_activity_level();
    // Negative morales are penalties
    int cold_penalty = -has_morale( morale_cold );
    int heat_penalty = -has_morale( morale_hot );
    if( cold_penalty > 0 || heat_penalty > 0 ) {
        // Extreme temperatures have the body working harder to fight them, with less energy dedicated to healing, even just laying around will have you shivering or sweating.
        rest -= cold_penalty / 10.0f;
        rest -= heat_penalty / 10.0f;
    }

    if( has_effect( effect_sleep ) ) {
        // Beds should normally have a maximum of 2C warmth, rest == 1.0 on a pristine bed. Less comfortable beds provide less rest quality.
        // Cannot be lower than 0, even though some beds(like *the bare ground*) have negative floor_bedding_warmth. Maybe this should change?
        rest += ( std::max( units::to_kelvin_delta( floor_bedding_warmth( your_pos ) ),
                            0.f ) / 2.0f ) ;
        return std::max( rest, 0.0f );
    }
    const optional_vpart_position veh_part = here.veh_at( your_pos );
    bool has_vehicle_seat = !!veh_part.part_with_feature( "SEAT", true );
    if( ur_act_level <= LIGHT_EXERCISE ) {
        rest += 0.1f;
        if( here.has_flag_ter_or_furn( "CAN_SIT", your_pos.xy() ) || has_vehicle_seat ) {
            // If not performing any real exercise (not even moving around), chairs allow you to rest a little bit.
            rest += 0.2f;
        } else if( floor_bedding_warmth( your_pos ) > 0_C_delta ) {
            // Any comfortable bed can substitute for a chair, but only if you don't have one.
            rest += 0.2f * ( units::to_celsius_delta( floor_bedding_warmth( your_pos ) ) / 2.0f );
        }
        if( ur_act_level <= NO_EXERCISE ) {
            rest += 0.2f;
        }
    }
    // These stack!
    if( ur_act_level >= BRISK_EXERCISE ) {
        rest -= 0.1f;
    }
    if( ur_act_level >= ACTIVE_EXERCISE ) {
        rest -= 0.2f;
    }
    if( ur_act_level >= EXTRA_EXERCISE ) {
        rest -= 0.3f;
    }
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "%s resting quality: %.6f", get_name(), rest );
    return rest;
}

namespace
{
float _hp_modified_rate( Character const &who, float rate )
{
    float const primary_hp_mod = who.enchantment_cache->get_value_multiply( enchant_vals::mod::MAX_HP );
    if( primary_hp_mod < 0.0f ) {
        cata_assert( primary_hp_mod >= -1.0f );
        return rate * ( 1.0f + primary_hp_mod );
    }

    return rate;
}
} // namespace

float Character::healing_rate( float at_rest_quality ) const
{
    float const rest = clamp( at_rest_quality, 0.0f, 1.0f );
    // TODO: Cache
    float const base_heal_rate = is_avatar() ? get_option<float>( "PLAYER_HEALING_RATE" )
                                 : get_option<float>( "NPC_HEALING_RATE" );
    float const heal_rate = enchantment_cache->modify_value( enchant_vals::mod::REGEN_HP,
                            base_heal_rate );
    float awake_rate = ( 1.0f - rest ) * heal_rate;
    awake_rate *= enchantment_cache->modify_value( enchant_vals::mod::REGEN_HP_AWAKE,
                  1 ) - 1;
    float const asleep_rate = rest * heal_rate * ( 1.0f + get_lifestyle() / 200.0f );
    float final_rate = awake_rate + asleep_rate;
    // Most common case: awake player with no regenerative abilities
    // ~7e-5 is 1 hp per day, anything less than that is totally negligible
    static constexpr float eps = 0.000007f;
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "%s healing: %.6f", get_name(), final_rate );
    if( std::abs( final_rate ) < eps ) {
        return 0.0f;
    }

    return final_rate;
}

float Character::healing_rate_medicine( float at_rest_quality, const bodypart_id &bp ) const
{
    float rate_medicine = 0.0f;

    for( const auto &elem : *effects ) {
        for( const std::pair<const bodypart_id, effect> &i : elem.second ) {
            if( i.first != bp ) {
                continue;
            }
            const effect &eff = i.second;
            float tmp_rate = static_cast<float>( eff.get_amount( "HEAL_RATE" ) ) / to_turns<int>
                             ( 24_hours );

            if( bp == body_part_head ) {
                tmp_rate *= eff.get_amount( "HEAL_HEAD" ) / 100.0f;
            }
            if( bp == body_part_torso ) {
                tmp_rate *= eff.get_amount( "HEAL_TORSO" ) / 100.0f;
            }
            rate_medicine += tmp_rate;
        }
    }

    rate_medicine = enchantment_cache->modify_value( enchant_vals::mod::REGEN_HP,
                    rate_medicine );
    // Sufficiently negative rest quality can completely eliminate your healing, but never turn it negative.
    rate_medicine *= 1.0f + std::max( at_rest_quality, -1.0f );

    // increase healing if character has both effects
    if( has_effect( effect_bandaged ) && has_effect( effect_disinfected ) ) {
        rate_medicine *= 1.25;
    }

    if( get_lifestyle() > 0.0f ) {
        rate_medicine *= 1.0f + get_lifestyle() / 200.0f;
    } else {
        rate_medicine *= 1.0f + get_lifestyle() / 400.0f;
    }
    return _hp_modified_rate( *this, rate_medicine );
}

float Character::get_bmi() const
{
    return get_bmi_lean() + get_bmi_fat();
}

float Character::get_bmi_lean() const
{
    //strength BMIs decrease to zero as you starve (muscle atrophy)
    if( get_bmi_fat() < character_weight_category::normal ) {
        const stat_mod wpen = get_weight_penalty();
        return 12.0f + get_str_base() - wpen.strength;
    }
    return 12.0f + get_str_base();
}

float Character::get_bmi_fat() const
{
    return ( get_stored_kcal() / KCAL_PER_KG ) / ( std::pow( height() / 100.0f,
            2 ) * get_cached_organic_size() );
}

bool Character::has_calorie_deficit() const
{
    return get_bmi_fat() < character_weight_category::normal;
}

units::mass Character::bodyweight() const
{
    return std::max( 1_gram, enchantment_cache->modify_value( enchant_vals::mod::WEIGHT,
                     bodyweight_fat() + bodyweight_lean() ) );
}

units::mass Character::bodyweight_fat() const
{
    return units::from_kilogram( get_stored_kcal() / KCAL_PER_KG );
}

units::mass Character::bodyweight_lean() const
{
    //12 plus base strength gives non fat bmi, adjusted by starvation in get_bmi_lean()
    //this is multiplied by our total hit size from mutated body parts (or lack of parts thereof)
    //for example a tail with a hit size of 10 means our lean mass is 10% greater
    //or if we chop off our arms and legs to get bionic replacements, we're down to about 42% of our original lean mass
    return get_cached_organic_size() * units::from_kilogram( get_bmi_lean() * std::pow(
                height() / 100.0f,
                2 ) );
}

float Character::fat_ratio() const
{
    return bodyweight_fat() / bodyweight();
}

int Character::ugliness() const
{
    int ugliness = 0;
    for( trait_id &mut : get_functioning_mutations() ) {
        ugliness += mut.obj().ugliness;
    }
    for( const bodypart_id &bp : get_all_body_parts() ) {
        if( bp->ugliness == 0 && bp->ugliness_mandatory == 0 ) {
            continue;
        }
        ugliness += bp->ugliness_mandatory;
        ugliness += bp->ugliness - ( bp->ugliness * worn.get_coverage( bp ) / 100 );
    }
    ugliness = enchantment_cache->modify_value( enchant_vals::mod::UGLINESS, ugliness );
    return ugliness;
}

int Character::get_bmr() const
{
    return base_bmr() * std::ceil( clamp( activity_history.average_activity(), NO_EXERCISE,
                                          maximum_exertion_level() ) );
}

int Character::get_stim() const
{
    return stim;
}

void Character::set_stim( int new_stim )
{
    stim = new_stim;
}

void Character::mod_stim( int mod )
{
    stim += mod;
}

int Character::get_rad() const
{
    return radiation;
}

void Character::set_rad( int new_rad )
{
    if( radiation != new_rad ) {
        radiation = new_rad;
        on_stat_change( "radiation", radiation );
    }
}

void Character::mod_rad( int mod )
{
    if( has_flag( json_flag_NO_RADIATION ) ) {
        return;
    }
    set_rad( std::max( 0, get_rad() + mod ) );
}

void Character::calculate_leak_level()
{
    float ret = 0.0f;
    // This is bad way to calculate radiation and should be rewritten some day.
    for( const item_location &item_loc : const_cast<Character *>( this )->all_items_loc() ) {
        const item *it = item_loc.get_item();
        if( it->has_flag( flag_RADIOACTIVE ) ) {
            if( it->has_flag( flag_LEAK_ALWAYS ) ) {
                ret += to_gram( it->weight() ) / 250.f;
            } else if( it->has_flag( flag_LEAK_DAM ) ) {
                ret += it->damage_level();
            }
        }
    }
    leak_level_dirty = false;
    leak_level = ret;
}

float Character::get_leak_level() const
{
    return leak_level;
}

void Character::invalidate_leak_level_cache()
{
    leak_level_dirty = true;
}

int Character::get_stamina() const
{
    if( is_npc() ) {
        // No point in doing a bunch of checks on NPCs for now since they can't use stamina.
        return get_stamina_max();
    }
    return stamina;
}

int Character::get_stamina_max() const
{
    if( is_npc() ) {
        // No point in doing a bunch of checks on NPCs for now since they can't use stamina.
        return 10000;
    }
    // Since adding cardio, 'player_max_stamina' is really 'base max stamina' and gets further modified
    // by your CV fitness.  Name has been kept this way to avoid needing to change the code.
    // Default base maximum stamina and cardio scaling are defined in data/core/game_balance.json
    static const std::string player_max_stamina( "PLAYER_MAX_STAMINA_BASE" );
    static const std::string player_cardiofit_stamina_scale( "PLAYER_CARDIOFIT_STAMINA_SCALING" );

    // Cardiofit stamina mod defaults to 5, and get_cardiofit() should return a value in the vicinity
    // of 1000-3000, so this should add somewhere between 3000 to 15000 stamina.
    int max_stamina = get_option<int>( player_max_stamina ) +
                      get_option<int>( player_cardiofit_stamina_scale ) * get_cardiofit();
    max_stamina = enchantment_cache->modify_value( enchant_vals::mod::MAX_STAMINA, max_stamina );

    return max_stamina;
}

void Character::set_stamina( int new_stamina )
{
    stamina = new_stamina;
}

void Character::mod_stamina( int mod )
{
    // TODO: Make NPCs smart enough to use stamina
    if( is_npc() || has_trait( trait_DEBUG_STAMINA ) ) {
        return;
    }

    float quarter_thresh = 0.25 * get_stamina_max();
    float half_thresh = 0.5 * get_stamina_max();

    int quarter_stam_counter = get_value( "quarter_stam_counter" ).dbl();

    if( stamina > half_thresh && stamina + mod < half_thresh ) {
        set_value( "got_to_half_stam", "true" );
    }

    if( stamina > quarter_thresh && stamina + mod < quarter_thresh && quarter_stam_counter < 5 ) {
        quarter_stam_counter++;
        set_value( "quarter_stam_counter", quarter_stam_counter );
        mod_daily_health( 1, 5 );
    }

    stamina += mod;
    if( stamina < 0 ) {
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( !bp->windage_effect.is_null() ) {
                add_effect( bp->windage_effect, 10_turns );
            }
        }
    }
    stamina = clamp( stamina, 0, get_stamina_max() );
}

void Character::burn_move_stamina( int moves )
{
    map &here = get_map();
    int overburden_percentage = 0;
    //add half the difference between current stored kcal weight and healthy stored kcal weight to weight of carried gear
    units::mass fat_penalty = units::from_kilogram( 0.5f * std::max( 0.0f,
                              ( get_healthy_kcal() - get_stored_kcal() ) / KCAL_PER_KG ) );
    units::mass current_weight = weight_carried() + fat_penalty;
    // Make it at least 1 gram to avoid divide-by-zero warning
    units::mass max_weight = std::max( weight_capacity(), 1_gram );
    if( current_weight > max_weight ) {
        overburden_percentage = ( current_weight - max_weight ) * 100 / max_weight;
    }

    int burn_ratio = get_option<int>( "PLAYER_BASE_STAMINA_BURN_RATE" );
    for( const bionic_id &bid : get_bionic_fueled_with_muscle() ) {
        if( has_active_bionic( bid ) ) {
            burn_ratio = burn_ratio * 2 - 3;
        }
    }
    burn_ratio += overburden_percentage;

    ///\EFFECT_SWIMMING decreases stamina burn when swimming
    //Appropriate traits let you walk along the bottom without getting as tired
    if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos_bub( here ) ) &&
        !has_flag( json_flag_WATERWALKING ) &&
        ( !has_flag( json_flag_WALK_UNDERWATER ) ||
          here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, pos_bub( here ) ) ) &&
        !here.has_flag_furn( "BRIDGE", pos_bub( here ) ) &&
        !( in_vehicle && here.veh_at( pos_bub( here ) )->vehicle().can_float( here ) ) ) {
        burn_ratio += 100 / std::pow( 1.1, get_skill_level( skill_swimming ) );
    }

    burn_ratio *= move_mode->stamina_mult();
    burn_energy_legs( -( ( moves * burn_ratio ) / 100.0 ) * get_modifier(
                          character_modifier_stamina_move_cost_mod ) * get_modifier(
                          character_modifier_move_mode_move_cost_mod ) );
    add_msg_debug( debugmode::DF_CHARACTER, "Stamina burn: %d", -( ( moves * burn_ratio ) / 100 ) );
    // Chance to suffer pain if overburden and stamina runs out or has trait BADBACK
    // Starts at 1 in 25, goes down by 5 for every 50% more carried
    if( ( current_weight > max_weight ) && ( has_trait( trait_BADBACK ) || get_stamina() == 0 ) &&
        one_in( 35 - 5 * current_weight / ( max_weight / 2 ) ) ) {
        add_msg_if_player( m_bad, _( "Your body strains under the weight!" ) );
        // 1 more pain for every 800 grams more (5 per extra STR needed)
        if( ( ( current_weight - max_weight ) / 800_gram > get_pain() && get_pain() < 100 ) ) {
            mod_pain( 1 );
        }
    }
}

void Character::update_stamina( int turns )
{
    static const std::string player_base_stamina_regen_rate( "PLAYER_BASE_STAMINA_REGEN_RATE" );
    const float base_regen_rate = get_option<float>( player_base_stamina_regen_rate );
    // Your stamina regen rate works as a function of how fit you are compared to your body size.
    // This allows it to scale more quickly than your stamina, so that at higher fitness levels you
    // recover stamina faster.
    const float effective_regen_rate = base_regen_rate * get_cardiofit() / get_cardio_acc_base();
    const int current_stim = get_stim();
    // Values above or below normal will increase or decrease stamina regen
    const float mod_regen = enchantment_cache->modify_value( enchant_vals::mod::STAMINA_REGEN_MOD, 0 );
    // Mutated stamina works even when winded
    const float base_multiplier = mod_regen + ( has_effect( effect_winded ) ? 0.1f : 1.0f );
    // Ensure multiplier is at least 0.1
    const float stamina_multiplier = std::max<float>( 0.1f, base_multiplier );

    // Recover some stamina every turn. Start with zero, then increase recovery factor based on
    // mutations, stimulants, and bionics before rolling random recovery based on this factor.
    float stamina_recovery = 0.0f;
    // But mouth encumbrance interferes, even with mutated stamina.
    stamina_recovery += stamina_multiplier * std::max( 1.0f,
                        effective_regen_rate * get_modifier( character_modifier_stamina_recovery_breathing_mod ) );
    // only apply stim-related and mutant stamina boosts if you don't have bionic lungs
    if( !has_bionic( bio_synlungs ) ) {
        stamina_recovery = enchantment_cache->modify_value( enchant_vals::mod::REGEN_STAMINA,
                           stamina_recovery );
        // TODO: recovering stamina causes hunger/thirst/sleepiness.
        // TODO: Tiredness slowing recovery

        // stim recovers stamina (or impairs recovery)
        if( current_stim > 0 ) {
            // TODO: Make stamina recovery with stims cost health
            stamina_recovery += std::min( 5.0f, current_stim / 15.0f );
        } else if( current_stim < 0 ) {
            // Affect it less near 0 and more near full
            // Negative stim kill at -200
            // At -100 stim it inflicts -20 malus to regen at 100%  stamina,
            // effectivly countering stamina gain of default 20,
            // at 50% stamina its -10 (50%), cuts by 25% at 25% stamina
            stamina_recovery += current_stim / 5.0f * get_stamina() / get_stamina_max();
        }
    }
    const int max_stam = get_stamina_max();
    if( get_power_level() >= 3_kJ && has_active_bionic( bio_gills ) ) {
        int bonus = std::min<int>( units::to_kilojoule( get_power_level() ) / 3,
                                   max_stam - get_stamina() - stamina_recovery * turns );
        // so the effective recovery is up to 5x default
        bonus = std::min( bonus, 4 * static_cast<int>( effective_regen_rate ) );
        if( bonus > 0 ) {
            stamina_recovery += bonus;
            bonus /= 10;
            bonus = std::max( bonus, 1 );
            mod_power_level( units::from_kilojoule( static_cast<std::int64_t>( -bonus ) ) );
        }
    }

    // Roll to determine actual stamina recovery over this period
    int recover_amount = std::ceil( stamina_recovery * turns );
    mod_stamina( recover_amount );
    add_msg_debug( debugmode::DF_CHARACTER, "Stamina recovery: %d", recover_amount );

    // Cap at max
    set_stamina( std::min( std::max( get_stamina(), 0 ), max_stam ) );
}

int Character::get_cardiofit() const
{
    if( is_npc() ) {
        // No point in doing a bunch of checks on NPCs for now since they can't use cardio.
        return 2 * get_cardio_acc_base();
    }

    if( has_bionic( bio_synlungs ) ) {
        // If you have the synthetic lungs bionic your cardioacc is forced to a specific value
        return 3 * get_cardio_acc_base();
    }

    const int cardio_base = get_cardio_acc();

    // Mut mod contains the base 1.0f for all modifiers
    const float mut_mod = enchantment_cache->modify_value( enchant_vals::mod::CARDIO_MULTIPLIER, 1 );
    // 1 point of athletics skill = 1% more cardio, up to 10% cardio
    const float athletics_mod = get_skill_level( skill_swimming ) / 100.0f;
    // At some point we might have proficiencies that affect this.
    const float prof_mod = 0.0f;
    // 10 points of health = 1% cardio, up to 20% cardio
    float health_mod = get_lifestyle() / 1000.0f;

    // Negative effects of health are doubled, up to 40% cardio
    if( health_mod < 0.0f ) {
        health_mod *= 2.0f;
    }

    // Add up all of our cardio mods.
    float cardio_modifier = mut_mod + athletics_mod + prof_mod + health_mod;
    // Modify cardio accumulator by our cardio mods.
    const int cardio_fitness = static_cast<int>( cardio_base * cardio_modifier );

    return cardio_fitness;
}

int Character::get_cardio_acc() const
{
    return cardio_acc;
}

void Character::set_cardio_acc( int ncardio_acc )
{
    cardio_acc = ncardio_acc;
}

void Character::reset_cardio_acc()
{
    set_cardio_acc( get_cardio_acc_base() );
}

int Character::get_cardio_acc_base() const
{
    return base_cardio_acc;
}

void Character::cough( bool harmful, int loudness )
{
    if( has_effect( effect_cough_suppress ) ) {
        return;
    }

    if( harmful ) {
        const int stam = get_stamina();
        const int malus = get_stamina_max() * 0.05; // 5% max stamina
        mod_stamina( -malus );
        if( stam < malus && x_in_y( malus - stam, malus ) && one_in( 6 ) ) {
            apply_damage( nullptr, body_part_torso, 1 );
        }
    }

    if( !is_npc() ) {
        add_msg( m_bad, _( "You cough heavily." ) );
    }
    sounds::sound( pos_bub(), loudness, sounds::sound_t::speech, _( "a hacking cough." ), false, "misc",
                   "cough" );

    mod_moves( -get_speed() * 0.8 );

    add_effect( effect_disrupted_sleep, 5_minutes );
}

void Character::wake_up()
{
    //Can't wake up if under anesthesia
    if( has_effect( effect_narcosis ) ) {
        return;
    }

    // Do not remove effect_sleep or effect_alarm_clock now otherwise it invalidates an effect
    // iterator in player::process_effects().
    // We just set it for later removal (also happening in player::process_effects(), so no side
    // effects) with a duration of 0 turns.

    if( has_effect( effect_sleep ) ) {
        get_effect( effect_sleep ).set_duration( 0_turns );
        get_event_bus().send<event_type::character_wakes_up>( getID() );
    }
    remove_effect( effect_slept_through_alarm );
    remove_effect( effect_lying_down );
    if( has_effect( effect_alarm_clock ) ) {
        get_effect( effect_alarm_clock ).set_duration( 0_turns );
    }
    recalc_sight_limits();

    if( movement_mode_is( move_mode_prone ) ) {
        set_movement_mode( move_mode_walk );
    }
}

void Character::vomit()
{
    const units::volume stomach_contents_before_vomit = stomach.contains();
    get_event_bus().send<event_type::throws_up>( getID() );

    if( stomach_contents_before_vomit != 0_ml ) {
        get_map().add_field( adjacent_tile(), fd_bile, 1 );
        add_msg_player_or_npc( m_bad, _( "You throw up heavily!" ), _( "<npcname> throws up heavily!" ) );
    }
    // needed to ensure digesting vitamin_type::DRUGs are also emptied, even on an empty stomach.
    stomach.empty();


    if( !has_effect( effect_nausea ) ) {  // Prevents never-ending nausea
        const effect dummy_nausea( effect_source( this ), &effect_nausea.obj(), 0_turns,
                                   bodypart_str_id::NULL_ID(), false, 1, calendar::turn );
        add_effect( effect_nausea, std::max( dummy_nausea.get_max_duration() * units::to_milliliter(
                stomach.contains() ) / 21, dummy_nausea.get_int_dur_factor() ) );
    }

    mod_moves( -100 );
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            effect &it = _effect_it.second;
            if( it.get_id() == effect_foodpoison ) {
                it.mod_duration( -30_minutes );
            } else if( it.get_id() == effect_drunk ) {
                it.mod_duration( rng( -10_minutes, -50_minutes ) );
            }
        }
    }
    remove_effect( effect_pkill1_generic );
    remove_effect( effect_pkill1_acetaminophen );
    remove_effect( effect_pkill1_nsaid );
    remove_effect( effect_pkill2 );
    remove_effect( effect_pkill3 );
    // Don't wake up when just retching
    if( stomach_contents_before_vomit > 0_ml ) {
        wake_up();
    }
}

/// Returns a weighted list of all mutation categories with current blood vitamin levels
weighted_int_list<mutation_category_id> Character::get_vitamin_weighted_categories() const
{
    weighted_int_list<mutation_category_id> weighted_output;
    for( const auto &elem : mutation_category_trait::get_all() ) {
        add_msg_debug( debugmode::DF_MUTATION, "get_vitamin_weighted_categories: category %s weight %d",
                       elem.second.id.c_str(), vitamin_get( elem.second.vitamin ) );
        weighted_output.add( elem.first, vitamin_get( elem.second.vitamin ) );
    }
    return weighted_output;
}

int Character::vitamin_RDA( const vitamin_id &vitamin, int amount ) const
{
    const double multiplier = vitamin_rate( vitamin ) * 100 / 1_days;
    return std::lround( amount * multiplier );
}

void Character::apply_wound( bodypart_id bp, wound_type_id wd )
{
    bodypart &body_bp = body.at( bp.id() );
    body_bp.add_wound( wd );
    morale->on_stat_change( "perceived_pain", get_perceived_pain() );
}

void Character::update_wounds( time_duration time_passed )
{
    for( auto &bp : body ) {
        bp.second.update_wounds( time_passed );
    }
    morale->on_stat_change( "perceived_pain", get_perceived_pain() );
}

/*
    Where damage to character is actually applied to hit body parts
    Might be where to put bleed stuff rather than in player::deal_damage()
 */
void Character::apply_damage( Creature *source, bodypart_id hurt, int dam,
                              const bool bypass_med )
{
    if( is_dead_state() || has_effect( effect_incorporeal ) ||
        has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        // don't do any more damage if we're already dead
        // Or if we're debugging and don't want to die
        // Or we are intangible
        return;
    }

    if( hurt == bodypart_str_id::NULL_ID() ) {
        debugmsg( "Wacky body part hurt!" );
        hurt = body_part_torso;
    }

    int pain = 0;
    if( !hurt->has_flag( json_flag_BIONIC_LIMB ) ) {
        pain = mod_pain( dam / 2 );
    }

    const bodypart_id &part_to_damage = hurt->main_part;

    const int dam_to_bodypart = std::min( dam, get_part_hp_cur( part_to_damage ) );

    mod_part_hp_cur( part_to_damage, - dam_to_bodypart );
    if( source ) {
        cata::event e = cata::event::make<event_type::character_takes_damage>( getID(), dam_to_bodypart,
                        part_to_damage.id(), pain );
        get_event_bus().send_with_talker( this, source, e );
    } else {
        get_event_bus().send<event_type::character_takes_damage>( getID(), dam_to_bodypart,
                part_to_damage.id(), pain );
    }

    if( !weapon.is_null() && !can_wield( weapon ).success() &&
        can_drop( weapon ).success() ) {
        add_msg_if_player( _( "You are no longer able to wield your %s and drop it!" ),
                           weapon.display_name() );
        put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { weapon } );
        i_rem( &weapon );
    }
    if( has_effect( effect_mending, part_to_damage.id() ) && ( source == nullptr ||
            !source->is_hallucination() ) ) {
        effect &e = get_effect( effect_mending, part_to_damage );
        float remove_mend = dam / 20.0f;
        e.mod_duration( -e.get_max_duration() * remove_mend );
    }

    if( dam > get_painkiller() ) {
        on_hurt( source );
    }

    if( !bypass_med ) {
        // remove healing effects if damaged
        int remove_med = roll_remainder( dam / 5.0f );
        if( remove_med > 0 && has_effect( effect_bandaged, part_to_damage.id() ) ) {
            remove_med -= reduce_healing_effect( effect_bandaged, remove_med, part_to_damage );
        }
        if( remove_med > 0 && has_effect( effect_disinfected, part_to_damage.id() ) ) {
            reduce_healing_effect( effect_disinfected, remove_med, part_to_damage );
        }
    }
}

void Character::apply_random_wound( bodypart_id bp, const damage_instance &d )
{
    if( x_in_y( 1.0f - get_option<float>( "WOUND_CHANCE" ), 1.0f ) ) {
        return;
    }

    weighted_int_list<wound_type_id> possible_wounds;
    for( const std::pair<bp_wounds, int> &wd : bp->potential_wounds ) {
        for( const damage_unit &du : d.damage_units ) {
            const bool damage_within_limits = du.amount >= wd.first.damage_required.first &&
                                              du.amount <= wd.first.damage_required.second;
            const bool damage_type_matches = std::find( wd.first.damage_type.begin(),
                                             wd.first.damage_type.end(), du.type ) != wd.first.damage_type.end();
            if( damage_within_limits && damage_type_matches ) {
                possible_wounds.add( wd.first.id, wd.second );
            }
        }
    }
    if( !possible_wounds.empty() ) {
        apply_wound( bp, *possible_wounds.pick() );
    }
}

dealt_damage_instance Character::deal_damage( Creature *source, bodypart_id bp,
        const damage_instance &d, const weakpoint_attack &attack, const weakpoint & )
{
    const map &here = get_map();

    if( has_effect( effect_incorporeal ) || has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        return dealt_damage_instance();
    }

    //damage applied here
    dealt_damage_instance dealt_dams = Creature::deal_damage( source, bp, d, attack );
    //block reduction should be by applied this point
    int dam = dealt_dams.total_damage();

    const tripoint_bub_ms pos = pos_bub( here );

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if( dam > 0 && get_player_view().sees( here, pos ) ) {
        g->draw_hit_player( *this, dam );

        if( is_avatar() && source ) {
            const tripoint_bub_ms source_pos = source->pos_bub( here );

            //monster hits player melee
            SCT.add( pos.xy().raw(),
                     direction_from( point::zero, point( pos.x() - source_pos.x(), pos.y() - source_pos.y() ) ),
                     get_hp_bar( dam, get_hp_max( bp ) ).first, m_bad, body_part_name( bp ), m_neutral );
        }
    }

    // And slimespawners too
    if( has_trait( trait_SLIMESPAWNER ) && ( dam >= 10 ) && one_in( 20 - dam ) ) {
        if( monster *const slime = g->place_critter_around( mon_player_blob, pos, 1 ) ) {
            slime->friendly = -1;
            add_msg_if_player( m_warning, _( "A mass of slime is torn from you, and moves on its own!" ) );
        }
    }

    Character &player_character = get_player_character();
    //Acid blood effects.
    bool u_see = player_character.sees( here, *this );
    // FIXME: Hardcoded damage type
    int cut_dam = dealt_dams.type_damage( damage_cut );
    if( source && has_flag( json_flag_ACIDBLOOD ) && !bp->has_flag( json_flag_BIONIC_LIMB ) &&
        !one_in( 3 ) &&
        ( dam >= 4 || cut_dam > 0 ) && ( rl_dist( player_character.pos_abs(), source->pos_abs() ) <= 1 ) ) {
        if( is_avatar() ) {
            add_msg( m_good, _( "Your acidic blood splashes %s in mid-attack!" ),
                     source->disp_name() );
        } else if( u_see ) {
            add_msg( _( "%1$s's acidic blood splashes on %2$s in mid-attack!" ),
                     disp_name(), source->disp_name() );
        }
        damage_instance acidblood_damage;
        // FIXME: Hardcoded damage type
        acidblood_damage.add_damage( damage_acid, rng( 4, 16 ) );
        if( !one_in( 4 ) ) {
            source->deal_damage( this, body_part_arm_l, acidblood_damage );
            source->deal_damage( this, body_part_arm_r, acidblood_damage );
        } else {
            source->deal_damage( this, body_part_torso, acidblood_damage );
            source->deal_damage( this, body_part_head, acidblood_damage );
        }
    }

    int recoil_mul = 100;

    if( bp == body_part_hand_l || bp == body_part_arm_l ||
        bp == body_part_hand_r || bp == body_part_arm_r ) {
        recoil_mul = 200;
    } else if( bp == bodypart_str_id::NULL_ID() ) {
        debugmsg( "Wacky body part hit!" );
    }

    // TODO: Scale with damage in a way that makes sense for power armors, plate armor and naked skin.
    recoil += recoil_mul * weapon.volume() / 250_ml;
    recoil = std::min( MAX_RECOIL, recoil );

    int sum_cover = 0;
    bool dealt_melee = false;
    bool dealt_ranged = false;
    for( const damage_unit &du : d ) {
        if( !du.type->physical ) {
            continue;
        }
        // Assume that ranged == getting shot
        if( !du.type->melee_only ) {
            dealt_ranged = true;
        } else {
            dealt_melee = true;
        }
    }
    sum_cover += worn.sum_filthy_cover( dealt_ranged, dealt_melee, bp );

    // Chance of infection is damage (with cut and stab x4) * sum of coverage on affected body part, in percent.
    // i.e. if the body part has a sum of 100 coverage from filthy clothing,
    // each point of damage has a 1% change of causing infection.
    // if the bodypart doesn't bleed, it doesn't have blood, and cannot get infected
    if( sum_cover > 0 && !bp->has_flag( json_flag_BIONIC_LIMB ) ) {
        // FIXME: Hardcoded damage types
        const int cut_type_dam = dealt_dams.type_damage( damage_cut ) +
                                 dealt_dams.type_damage( damage_stab );
        const int combined_dam = dealt_dams.type_damage( damage_bash ) + ( cut_type_dam * 4 );
        const int infection_chance = ( combined_dam * sum_cover ) / 100;
        if( x_in_y( infection_chance, 100 ) ) {
            if( has_effect( effect_bite, bp.id() ) ) {
                add_effect( effect_bite, 40_minutes, bp, true );
            } else if( has_effect( effect_infected, bp.id() ) ) {
                add_effect( effect_infected, 25_minutes, bp, true );
            } else {
                add_effect( effect_bite, 1_turns, bp, true );
            }
            add_msg_if_player( _( "Filth from your clothing has been embedded deep in the wound." ) );
        }
    }

    apply_random_wound( bp, d );

    on_hurt( source );
    return dealt_dams;
}

int Character::reduce_healing_effect( const efftype_id &eff_id, int remove_med,
                                      const bodypart_id &hurt )
{
    effect &e = get_effect( eff_id, hurt );
    int intensity = e.get_intensity();
    if( remove_med < intensity ) {
        if( eff_id == effect_bandaged ) {
            add_msg_if_player( m_bad, _( "Bandages on your %s were damaged!" ), body_part_name( hurt ) );
        } else  if( eff_id == effect_disinfected ) {
            add_msg_if_player( m_bad, _( "You got some filth on your disinfected %s!" ),
                               body_part_name( hurt ) );
        }
    } else {
        if( eff_id == effect_bandaged ) {
            add_msg_if_player( m_bad, _( "Bandages on your %s were destroyed!" ),
                               body_part_name( hurt ) );
        } else  if( eff_id == effect_disinfected ) {
            add_msg_if_player( m_bad, _( "Your %s is no longer disinfected!" ), body_part_name( hurt ) );
        }
    }
    e.mod_duration( -6_hours * remove_med );
    return intensity;
}

void Character::heal_bp( bodypart_id bp, int dam )
{
    heal( bp, dam );
}

void Character::heal( const bodypart_id &healed, int dam )
{
    if( !is_limb_broken( healed ) && ( dam != 0 || healed->has_flag( json_flag_ALWAYS_HEAL ) ) ) {
        add_msg_debug( debugmode::DF_CHAR_HEALTH, "Base healing of %s = %d", body_part_name( healed ),
                       dam );
        if( healed->has_flag( json_flag_HEAL_OVERRIDE ) ) {
            dam = healed->heal_bonus;
            add_msg_debug( debugmode::DF_CHAR_HEALTH, "Heal override, new healing %d", dam );
        } else {
            dam += healed->heal_bonus;
            add_msg_debug( debugmode::DF_CHAR_HEALTH, "Healing after bodypart heal bonus %d", dam );
        }
        int effective_heal = std::min( dam,
                                       get_part_hp_max( healed ) - get_part_hp_cur( healed ) ) ;
        mod_part_hp_cur( healed, effective_heal );
        add_msg_debug( debugmode::DF_CHAR_HEALTH, "Final healing of %s = %d", body_part_name( healed ),
                       dam );
        get_event_bus().send<event_type::character_heals_damage>( getID(), effective_heal );
    }
}

void Character::healall( int dam )
{
    for( const bodypart_id &bp : get_all_body_parts() ) {
        heal( bp, dam );
        mod_part_healed_total( bp, dam );
    }
}

void Character::hurtall( int dam, Creature *source, bool disturb /*= true*/ )
{
    if( is_dead_state() || has_effect( effect_incorporeal ) ||
        dam <= 0 || has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        return;
    }

    // Low pain: damage is spread all over the body, so not as painful as 6 hits in one part
    int pain = mod_pain( dam );

    std::vector<bodypart_id> body_parts = get_all_body_parts( get_body_part_flags::only_main );
    for( const bodypart_id &bp : body_parts ) {
        // Don't use apply_damage here or it will annoy the player with 6 queries
        const int dam_to_bodypart = std::min( dam, get_part_hp_cur( bp ) );
        mod_part_hp_cur( bp, - dam_to_bodypart );

        if( source ) {
            cata::event e = cata::event::make<event_type::character_takes_damage>( getID(), dam_to_bodypart,
                            bp.id(), pain / body_parts.size() );
            get_event_bus().send_with_talker( this, source == nullptr ? nullptr : source, e );
        } else {
            get_event_bus().send<event_type::character_takes_damage>( getID(), dam_to_bodypart, bp.id(),
                    pain / body_parts.size() );
        }

    }

    on_hurt( source, disturb );
}

int Character::hitall( int dam, int vary, Creature *source )
{
    int damage_taken = 0;
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        int ddam = vary ? dam * rng( 100 - vary, 100 ) / 100 : dam;
        // FIXME: Hardcoded damage type
        damage_instance damage( damage_bash, ddam );
        damage_taken += deal_damage( source, bp, damage ).total_damage();
    }
    return damage_taken;
}

void Character::on_hurt( Creature *source, bool disturb /*= true*/ )
{
    const map &here = get_map();

    if( has_trait( trait_ADRENALINE ) && !has_effect( effect_adrenaline ) &&
        ( get_part_hp_cur( body_part_head ) < 25 ||
          get_part_hp_cur( body_part_torso ) < 15 ) ) {
        add_effect( effect_adrenaline, 20_minutes );
    }

    if( disturb ) {
        if( has_effect( effect_sleep ) && !has_bionic( bio_sleep_shutdown ) ) {
            wake_up();
        }
        if( uistate.distraction_attack && !is_npc() && !has_effect( effect_narcosis ) ) {
            if( source != nullptr ) {
                if( sees( here, *source ) ) {
                    g->cancel_activity_or_ignore_query( distraction_type::attacked,
                                                        string_format( _( "You were attacked by %s!" ), source->disp_name() ) );
                } else {
                    g->cancel_activity_or_ignore_query( distraction_type::attacked,
                                                        _( "You were attacked by something you can't see!" ) );
                }
            } else {
                g->cancel_activity_or_ignore_query( distraction_type::attacked, _( "You were hurt!" ) );
            }
        }
    }

    if( is_dead_state() ) {
        set_killer( source );
    }
}

void Character::spores()
{
    map &here = get_map();
    fungal_effects fe;
    //~spore-release sound
    sounds::sound( pos_bub(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    for( const tripoint_bub_ms &sporep : here.points_in_radius( pos_bub(), 1 ) ) {
        if( sporep == pos_bub() ) {
            continue;
        }
        fe.fungalize( sporep, this, 0.25 );
    }
}

void Character::blossoms()
{
    // Player blossoms are shorter-ranged, but you can fire much more frequently if you like.
    sounds::sound( pos_bub(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    map &here = get_map();
    for( const tripoint_bub_ms &tmp : here.points_in_radius( pos_bub(), 2 ) ) {
        here.add_field( tmp, fd_fungal_haze, rng( 1, 2 ) );
    }
}

void Character::update_vitamins( const vitamin_id &vit )
{
    if( !needs_food() ) {
        return; // NPCs can only develop vitamin diseases if their needs are enabled
    }

    efftype_id def = vit.obj().deficiency();
    efftype_id exc = vit.obj().excess();

    int lvl = vit.obj().severity( vitamin_get( vit ) );
    if( lvl <= 0 ) {
        remove_effect( def );
    }
    if( lvl >= 0 ) {
        remove_effect( exc );
    }
    if( lvl > 0 ) {
        if( has_effect( def ) ) {
            get_effect( def ).set_intensity( lvl, is_avatar() );
        } else {
            add_effect( def, 1_turns, true, lvl );
        }
    }
    if( lvl < 0 ) {
        if( has_effect( exc ) ) {
            get_effect( exc ).set_intensity( -lvl, is_avatar() );
        } else {
            add_effect( exc, 1_turns, true, -lvl );
        }
    }
}

void Character::rooted_message() const
{
    if( ( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) &&
        get_map().has_flag( ter_furn_flag::TFLAG_PLOWABLE, pos_bub() ) &&
        is_barefoot() ) {
        add_msg( m_info, _( "You sink your roots into the soil." ) );
    }
}

void Character::rooted()
// This assumes that daily Iron, Calcium, and Thirst needs should be filled at the same rate.
// Baseline humans need 96 Iron and Calcium, and 288 Thirst per day.
// Thirst level -40 puts it right in the middle of the 'Hydrated' zone.
// TODO: The rates for iron, calcium, and thirst should probably be pulled from the nutritional data rather than being hardcoded here, so that future balance changes don't break this.
{
    if( ( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) &&
        get_map().has_flag( ter_furn_flag::TFLAG_PLOWABLE, pos_bub() ) && is_barefoot() ) {
        int time_to_full = 43200; // 12 hours
        if( has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) {
            time_to_full += -14400;    // -4 hours
        }
        if( x_in_y( 96, time_to_full ) ) {
            vitamin_mod( vitamin_iron, 1 );
            vitamin_mod( vitamin_calcium, 1 );
            mod_daily_health( 5, 50 );
        }
        if( get_thirst() > -40 && x_in_y( 288, time_to_full ) ) {
            mod_thirst( -1 );
        }
    }
}

units::temperature Character::temp_corrected_by_climate_control( units::temperature temperature,
        int heat_strength, int chill_strength ) const
{
    const units::temperature_delta base_variation = BODYTEMP_NORM - 27_C;
    const units::temperature_delta variation_heat = base_variation * ( heat_strength / 100.0f );
    const units::temperature_delta variation_chill = -base_variation * ( chill_strength / 100.0f );

    if( temperature > BODYTEMP_NORM ) {
        return std::max( BODYTEMP_NORM, temperature + variation_chill );
    } else {
        return std::min( BODYTEMP_NORM, temperature + variation_heat );
    }
}

void Character::mod_painkiller( int npkill )
{
    set_painkiller( pkill + npkill );
}

void Character::set_painkiller( int npkill )
{
    npkill = std::max( npkill, 0 );
    if( pkill != npkill ) {
        const int prev_pain = get_perceived_pain();
        pkill = npkill;
        on_stat_change( "pkill", pkill );
        const int cur_pain = get_perceived_pain();

        if( cur_pain != prev_pain ) {
            react_to_felt_pain( cur_pain - prev_pain );
            on_stat_change( "perceived_pain", cur_pain );
        }
    }
}

int Character::get_painkiller() const
{
    return pkill;
}

int Character::heartrate_bpm() const
{
    //Dead have no heartbeat usually and no heartbeat in omnicell
    if( is_dead_state() || has_trait( trait_SLIMESPAWNER ) ) {
        return 0;
    }

    int average_heartbeat = avg_nat_bpm * get_heartrate_index();

    // minor moment-to-moment variation
    average_heartbeat += rng( -5, 5 );

    //Chemical imbalance makes this less predictable. It's possible this range needs tweaking
    if( has_trait( trait_CHEMIMBALANCE ) ) {
        average_heartbeat += rng( -15, 15 );
    }

    return average_heartbeat;
}

int Character::get_effective_focus() const
{
    int effective_focus = get_focus();
    effective_focus = enchantment_cache->modify_value( enchant_vals::mod::LEARNING_FOCUS,
                      effective_focus );
    effective_focus *= 1.0 + ( 0.01f * ( get_int() -
                                         get_option<int>( "INT_BASED_LEARNING_BASE_VALUE" ) ) *
                               get_option<int>( "INT_BASED_LEARNING_FOCUS_ADJUSTMENT" ) );
    return std::max( effective_focus, 1 );
}

float Character::adjust_for_focus( float amount ) const
{
    return amount * ( get_effective_focus() / 100.0f );
}

bool Character::can_hear( const tripoint_bub_ms &source, const int volume ) const
{
    if( is_deaf() ) {
        return false;
    }

    // source is in-ear and at our square, we can hear it
    if( source == pos_bub() && volume == 0 ) {
        return true;
    }
    const int dist = rl_dist( source, pos_bub() );
    const float volume_multiplier = hearing_ability();
    return ( volume - get_weather().weather_id->sound_attn ) * volume_multiplier >= dist;
}

float Character::hearing_ability() const
{
    float volume_multiplier = 1.0f;

    volume_multiplier = enchantment_cache->modify_value( enchant_vals::mod::HEARING_MULT,
                        volume_multiplier );

    if( has_effect( effect_deaf ) ) {
        // Scale linearly up to 30 minutes
        volume_multiplier *= ( 30_minutes - get_effect_dur( effect_deaf ) ) / 30_minutes;
    }

    return volume_multiplier;
}

double Character::vomit_mod()
{
    double mod = 1;

    mod = enchantment_cache->modify_value( enchant_vals::mod::VOMIT_MUL, mod );

    // If you're already nauseous, any food in your stomach greatly
    // increases chance of vomiting. Liquids don't provoke vomiting, though.
    if( stomach.contains() != 0_ml && has_effect( effect_nausea ) ) {
        mod *= get_effect_int( effect_nausea );
    }
    return mod;
}

int Character::get_lowest_hp() const
{
    // Set lowest_hp to an arbitrarily large number.
    int lowest_hp = INT_MAX;
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        const int cur_hp = elem.second.get_hp_cur();
        if( cur_hp < lowest_hp ) {
            lowest_hp = cur_hp;
        }
    }
    return lowest_hp;
}

int Character::get_highest_hp() const
{
    // Set lowest_hp to an arbitrarily large number.
    int highest_hp = INT_MIN;
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        const int cur_hp = elem.second.get_hp_cur();
        if( cur_hp > highest_hp ) {
            highest_hp = cur_hp;
        }
    }
    return highest_hp;
}

bool Character::has_unfulfilled_pyromania() const
{
    return has_trait( trait_PYROMANIA ) && !has_morale( morale_pyromania_startfire );
}

void Character::fulfill_pyromania( int bonus, int max_bonus,
                                   const time_duration &duration, const time_duration &decay_start )
{
    add_morale( morale_pyromania_startfire, bonus, max_bonus, duration, decay_start );
    rem_morale( morale_pyromania_nofire );
}

void Character::fulfill_pyromania_msg( const std::string &message, int bonus, int max_bonus,
                                       const time_duration &duration, const time_duration &decay_start )
{
    add_msg_if_player( m_good, message );
    fulfill_pyromania( bonus, max_bonus, duration, decay_start );
}

void Character::fulfill_pyromania_msg_std( const std::string &target_name, int bonus, int max_bonus,
        const time_duration &duration, const time_duration &decay_start )
{
    if( target_name.empty() ) {
        fulfill_pyromania_msg( _( "You feel a surge of euphoria as flames burst out!" ), bonus, max_bonus,
                               duration, decay_start );
    } else {
        fulfill_pyromania_msg( string_format( _( "You feel a surge of euphoria as flame engulfs %s!" ),
                                              target_name ), bonus, max_bonus, duration, decay_start );
    }
}

bool Character::fulfill_pyromania_sees( const map &here, const Creature &target,
                                        int bonus, int max_bonus,
                                        const time_duration &duration, const time_duration &decay_start )
{
    if( sees( here, target ) ) {
        fulfill_pyromania_msg_std( target.get_name(), bonus, max_bonus, duration, decay_start );
        return true;
    }
    return false;
}

bool Character::fulfill_pyromania_sees( const map &here, const tripoint_bub_ms &target,
                                        const std::string &target_str, bool std_msg, int bonus, int max_bonus,
                                        const time_duration &duration, const time_duration &decay_start )
{
    if( sees( here, target ) ) {
        if( std_msg ) {
            fulfill_pyromania_msg_std( target_str, bonus, max_bonus, duration, decay_start );
        } else {
            fulfill_pyromania_msg( target_str, bonus, max_bonus, duration, decay_start );
        }
        return true;
    }
    return false;
}

bool Character::schizo_symptoms( int chance ) const
{
    return has_trait( trait_SCHIZOPHRENIC ) && !has_effect( effect_took_thorazine ) && one_in( chance );
}

int Character::thirst_speed_penalty( int thirst )
{
    // We die at 1200 thirst
    // Start by dropping speed really fast, but then level it off a bit
    static const std::vector<std::pair<float, float>> thirst_thresholds = {{
            std::make_pair( 40.0f, 0.0f ),
            std::make_pair( 300.0f, -25.0f ),
            std::make_pair( 600.0f, -50.0f ),
            std::make_pair( 1200.0f, -75.0f )
        }
    };
    return static_cast<int>( multi_lerp( thirst_thresholds, thirst ) );
}

stat_mod Character::get_weight_penalty() const
{
    stat_mod ret;
    const float bmi = get_bmi_fat();
    ret.strength = std::floor( ( 1.0f - ( bmi / character_weight_category::normal ) ) * str_max );
    ret.dexterity = std::floor( ( character_weight_category::normal - bmi ) * 3.0f );
    ret.intelligence = ret.dexterity;
    return ret;
}


stat_mod Character::get_pain_penalty() const
{
    stat_mod ret;
    int pain = get_perceived_pain();
    // if less than 10 pain, do not apply any penalties
    if( pain <= 10 ) {
        return ret;
    }

    float penalty_str = pain * get_option<float>( "PAIN_PENALTY_MOD_STR" );
    float penalty_dex = pain * get_option<float>( "PAIN_PENALTY_MOD_DEX" );
    float penalty_int = pain * get_option<float>( "PAIN_PENALTY_MOD_INT" );
    float penalty_per = pain * get_option<float>( "PAIN_PENALTY_MOD_PER" );


    ret.strength = enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_STR,
                   get_str() * penalty_str );

    ret.dexterity = enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_DEX,
                    get_dex() * penalty_dex );

    ret.intelligence = enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_INT,
                       get_int() * penalty_int );

    ret.perception = enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_PER,
                     get_per() * penalty_per );


    // Prevent negative penalties, there is better ways to give bonuses for pain
    // Also not make character has 0 stats
    ret.strength = get_str() > 2 ? std::clamp( ret.strength, 1, get_str() - 1 ) :
                   std::max( 0, get_str() - 1 );
    ret.dexterity = get_dex() > 2 ? std::clamp( ret.dexterity, 1, get_dex() - 1 ) :
                    std::max( 0, get_dex() - 1 );
    ret.intelligence = get_int() > 2 ? std::clamp( ret.intelligence, 1, get_int() - 1 ) :
                       std::max( 0, get_int() - 1 );
    ret.perception = get_per() > 2 ? std::clamp( ret.perception, 1, get_per() - 1 ) :
                     std::max( 0, get_per() - 1 );


    int speed_penalty = std::pow( pain, 0.7f );

    ret.speed = std::max( enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_SPEED,
                          speed_penalty ), 0.0 );

    ret.speed = std::min( ret.speed, 50 );
    return ret;
}

stat_mod Character::read_pain_penalty() const
{
    stat_mod ret;
    ret.strength = ppen_str;
    ret.dexterity = ppen_dex;
    ret.intelligence = ppen_int;
    ret.perception = ppen_per;
    ret.speed = ppen_spd;
    return ret;
}

int Character::get_lift_str() const
{
    int str = get_arm_str();
    if( has_trait( trait_STRONGBACK ) ) {
        str *= 1.35;
    } else if( has_trait( trait_BADBACK ) ) {
        str /= 1.35;
    }
    return str;
}

int Character::get_lift_assist() const
{
    int result = 0;
    for( const Character *guy : get_crafting_helpers() ) {
        result += guy->get_lift_str();
    }
    return result;
}

bool Character::immune_to( const bodypart_id &bp, damage_unit dam ) const
{
    if( is_immune_damage( dam.type ) ||
        has_effect( effect_incorporeal ) || has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        return true;
    }

    passive_absorb_hit( bp, dam );

    worn.damage_mitigate( bp, dam );

    return dam.amount <= 0;
}

int Character::get_pain() const
{
    int p = 0;
    for( const std::pair<const bodypart_str_id, bodypart> &bp : get_body() ) {
        for( const wound &wd : bp.second.get_wounds() ) {
            p += wd.get_pain();
        }
    }

    return p + Creature::get_pain();
}

int Character::mod_pain( int npain )
{
    if( npain > 0 ) {
        double mult = enchantment_cache->get_value_multiply( enchant_vals::mod::PAIN );
        if( has_flag( json_flag_PAIN_IMMUNE ) || has_effect( effect_narcosis ) ) {
            return 0;
        }
        // if there is a positive multiplier we always want to add at least 1 pain
        if( mult > 0 ) {
            npain += std::max( 1, roll_remainder( npain * mult ) );
        }
        if( mult < 0 ) {
            npain = roll_remainder( npain * ( 1 + mult ) );
        }
        npain += enchantment_cache->get_value_add( enchant_vals::mod::PAIN );

        // no matter how powerful the enchantment if we are gaining pain we don't lose any
        npain = std::max( 0, npain );
    }
    Creature::mod_pain( npain );
    return npain;
}

void Character::set_pain( int npain )
{
    const int prev_pain = get_perceived_pain();
    Creature::set_pain( npain );
    const int cur_pain = get_perceived_pain();

    if( cur_pain != prev_pain ) {
        react_to_felt_pain( cur_pain - prev_pain );
        on_stat_change( "perceived_pain", cur_pain );
    }
}

int Character::get_perceived_pain() const
{
    if( get_effect_int( effect_adrenaline ) > 1 ) {
        return 0;
    }

    return std::max( get_pain() - get_painkiller(), 0 );
}

int Character::hp_percentage() const
{
    const bodypart_id head_id = bodypart_id( "head" );
    const bodypart_id torso_id = bodypart_id( "torso" );
    int total_cur = 0;
    int total_max = 0;
    // Head and torso HP are weighted 3x and 2x, respectively
    total_cur = get_part_hp_cur( head_id ) * 3 + get_part_hp_cur( torso_id ) * 2;
    total_max = get_part_hp_max( head_id ) * 3 + get_part_hp_max( torso_id ) * 2;
    for( const std::pair< const bodypart_str_id, bodypart> &elem : get_body() ) {
        total_cur += elem.second.get_hp_cur();
        total_max += elem.second.get_hp_max();
    }

    return ( 100 * total_cur ) / total_max;
}

void Character::environmental_revert_effect()
{
    addictions.clear();
    morale->clear();

    set_all_parts_hp_to_max();
    set_hunger( 0 );
    set_thirst( 0 );
    set_sleepiness( 0 );
    set_lifestyle( 0 );
    set_daily_health( 0 );
    set_stim( 0 );
    set_pain( 0 );
    set_painkiller( 0 );
    set_rad( 0 );

    recalc_sight_limits();
    calc_encumbrance();
}
