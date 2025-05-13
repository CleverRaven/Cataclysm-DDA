#include "activity_actor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "advanced_inv.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bodypart.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "clzones.h"
#include "contents_change_handler.h"
#include "coordinates.h"
#include "craft_command.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "fault.h"
#include "field_type.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "game_constants.h"
#include "gates.h"
#include "gun_mode.h"
#include "handle_liquid.h"
#include "harvest.h"
#include "iexamine.h"
#include "inventory.h"
#include "item.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "json.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "mapdata.h"
#include "martialarts.h"
#include "math_parser_diag_value.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap_ui.h"
#include "pathfinding.h"
#include "pickup.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "point.h"
#include "ranged.h"
#include "recipe.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "safe_reference.h"
#include "shearing.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const activity_id ACT_AIM( "ACT_AIM" );
static const activity_id ACT_AUTODRIVE( "ACT_AUTODRIVE" );
static const activity_id ACT_BASH( "ACT_BASH" );
static const activity_id ACT_BIKERACK_RACKING( "ACT_BIKERACK_RACKING" );
static const activity_id ACT_BIKERACK_UNRACKING( "ACT_BIKERACK_UNRACKING" );
static const activity_id ACT_BINDER_COPY_RECIPE( "ACT_BINDER_COPY_RECIPE" );
static const activity_id ACT_BOLTCUTTING( "ACT_BOLTCUTTING" );
static const activity_id ACT_CHOP_LOGS( "ACT_CHOP_LOGS" );
static const activity_id ACT_CHOP_PLANKS( "ACT_CHOP_PLANKS" );
static const activity_id ACT_CHOP_TREE( "ACT_CHOP_TREE" );
static const activity_id ACT_CHURN( "ACT_CHURN" );
static const activity_id ACT_CLEAR_RUBBLE( "ACT_CLEAR_RUBBLE" );
static const activity_id ACT_CONSUME( "ACT_CONSUME" );
static const activity_id ACT_CONSUME_MEDS_MENU( "ACT_CONSUME_MEDS_MENU" );
static const activity_id ACT_CRACKING( "ACT_CRACKING" );
static const activity_id ACT_CRAFT( "ACT_CRAFT" );
static const activity_id ACT_DISABLE( "ACT_DISABLE" );
static const activity_id ACT_DISASSEMBLE( "ACT_DISASSEMBLE" );
static const activity_id ACT_DROP( "ACT_DROP" );
static const activity_id ACT_EAT_MENU( "ACT_EAT_MENU" );
static const activity_id ACT_EBOOKSAVE( "ACT_EBOOKSAVE" );
static const activity_id ACT_E_FILE( "ACT_E_FILE" );
static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_FORAGE( "ACT_FORAGE" );
static const activity_id ACT_FURNITURE_MOVE( "ACT_FURNITURE_MOVE" );
static const activity_id ACT_GLIDE( "ACT_GLIDE" );
static const activity_id ACT_GUNMOD_ADD( "ACT_GUNMOD_ADD" );
static const activity_id ACT_GUNMOD_REMOVE( "ACT_GUNMOD_REMOVE" );
static const activity_id ACT_HACKING( "ACT_HACKING" );
static const activity_id ACT_HACKSAW( "ACT_HACKSAW" );
static const activity_id ACT_HAIRCUT( "ACT_HAIRCUT" );
static const activity_id ACT_HARVEST( "ACT_HARVEST" );
static const activity_id ACT_HEATING( "ACT_HEATING" );
static const activity_id ACT_HOTWIRE_CAR( "ACT_HOTWIRE_CAR" );
static const activity_id ACT_INSERT_ITEM( "ACT_INSERT_ITEM" );
static const activity_id ACT_INVOKE_ITEM( "ACT_INVOKE_ITEM" );
static const activity_id ACT_LOCKPICK( "ACT_LOCKPICK" );
static const activity_id ACT_LONGSALVAGE( "ACT_LONGSALVAGE" );
static const activity_id ACT_MEDITATE( "ACT_MEDITATE" );
static const activity_id ACT_MIGRATION_CANCEL( "ACT_MIGRATION_CANCEL" );
static const activity_id ACT_MILK( "ACT_MILK" );
static const activity_id ACT_MOP( "ACT_MOP" );
static const activity_id ACT_MOVE_ITEMS( "ACT_MOVE_ITEMS" );
static const activity_id ACT_MULTIPLE_CHOP_TREES( "ACT_MULTIPLE_CHOP_TREES" );
static const activity_id ACT_OPEN_GATE( "ACT_OPEN_GATE" );
static const activity_id ACT_OXYTORCH( "ACT_OXYTORCH" );
static const activity_id ACT_PICKUP( "ACT_PICKUP" );
static const activity_id ACT_PLAY_WITH_PET( "ACT_PLAY_WITH_PET" );
static const activity_id ACT_PRYING( "ACT_PRYING" );
static const activity_id ACT_PULP( "ACT_PULP" );
static const activity_id ACT_READ( "ACT_READ" );
static const activity_id ACT_REEL_CABLE( "ACT_REEL_CABLE" );
static const activity_id ACT_RELOAD( "ACT_RELOAD" );
static const activity_id ACT_SHAVE( "ACT_SHAVE" );
static const activity_id ACT_SHEARING( "ACT_SHEARING" );
static const activity_id ACT_STASH( "ACT_STASH" );
static const activity_id ACT_TENT_DECONSTRUCT( "ACT_TENT_DECONSTRUCT" );
static const activity_id ACT_TENT_PLACE( "ACT_TENT_PLACE" );
static const activity_id ACT_TRY_SLEEP( "ACT_TRY_SLEEP" );
static const activity_id ACT_UNLOAD( "ACT_UNLOAD" );
static const activity_id ACT_UNLOAD_LOOT( "ACT_UNLOAD_LOOT" );
static const activity_id ACT_VEHICLE_FOLD( "ACT_VEHICLE_FOLD" );
static const activity_id ACT_VEHICLE_UNFOLD( "ACT_VEHICLE_UNFOLD" );
static const activity_id ACT_WAIT_STAMINA( "ACT_WAIT_STAMINA" );
static const activity_id ACT_WASH( "ACT_WASH" );
static const activity_id ACT_WEAR( "ACT_WEAR" );
static const activity_id ACT_WIELD( "ACT_WIELD" );
static const activity_id ACT_WORKOUT_ACTIVE( "ACT_WORKOUT_ACTIVE" );
static const activity_id ACT_WORKOUT_HARD( "ACT_WORKOUT_HARD" );
static const activity_id ACT_WORKOUT_LIGHT( "ACT_WORKOUT_LIGHT" );
static const activity_id ACT_WORKOUT_MODERATE( "ACT_WORKOUT_MODERATE" );

static const ammotype ammo_plutonium( "plutonium" );

static const efftype_id effect_docile( "docile" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_gliding( "gliding" );
static const efftype_id effect_paid( "paid" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_sensor_stun( "sensor_stun" );
static const efftype_id effect_sheared( "sheared" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_tied( "tied" );
static const efftype_id effect_took_thorazine( "took_thorazine" );
static const efftype_id effect_worked_on( "worked_on" );

static const faction_id faction_your_followers( "your_followers" );

static const flag_id json_flag_ALWAYS_AIMED( "ALWAYS_AIMED" );
static const flag_id json_flag_NO_RELOAD( "NO_RELOAD" );

static const furn_str_id furn_f_gunsafe_mj( "f_gunsafe_mj" );
static const furn_str_id furn_f_gunsafe_ml( "f_gunsafe_ml" );
static const furn_str_id furn_f_safe_c( "f_safe_c" );
static const furn_str_id furn_f_safe_o( "f_safe_o" );

static const gun_mode_id gun_mode_DEFAULT( "DEFAULT" );

static const item_group_id
Item_spawn_data_SUS_trash_forest_no_manmade( "SUS_trash_forest_no_manmade" );
static const item_group_id Item_spawn_data_forage_autumn( "forage_autumn" );
static const item_group_id Item_spawn_data_forage_spring( "forage_spring" );
static const item_group_id Item_spawn_data_forage_summer( "forage_summer" );
static const item_group_id Item_spawn_data_forage_winter( "forage_winter" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_detergent( "detergent" );
static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_efile_junk( "efile_junk" );
static const itype_id itype_efile_photos( "efile_photos" );
static const itype_id itype_liquid_soap( "liquid_soap" );
static const itype_id itype_log( "log" );
static const itype_id itype_paper( "paper" );
static const itype_id itype_pseudo_bio_picklock( "pseudo_bio_picklock" );
static const itype_id
itype_robofac_yrax_trifacet_deactivation_manual( "robofac_yrax_trifacet_deactivation_manual" );
static const itype_id itype_soap( "soap" );
static const itype_id itype_splinter( "splinter" );
static const itype_id itype_stick_long( "stick_long" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );

static const json_character_flag json_flag_READ_IN_DARKNESS( "READ_IN_DARKNESS" );
static const json_character_flag json_flag_SAFECRACK_NO_TOOL( "SAFECRACK_NO_TOOL" );

static const morale_type morale_book( "morale_book" );
static const morale_type morale_feeling_good( "morale_feeling_good" );
static const morale_type morale_haircut( "morale_haircut" );
static const morale_type morale_play_with_pet( "morale_play_with_pet" );
static const morale_type morale_shave( "morale_shave" );

static const move_mode_id move_mode_prone( "prone" );
static const move_mode_id move_mode_walk( "walk" );

static const mtype_id mon_yrax_trifacet( "mon_yrax_trifacet" );

static const proficiency_id proficiency_prof_lockpicking( "prof_lockpicking" );
static const proficiency_id proficiency_prof_lockpicking_expert( "prof_lockpicking_expert" );
static const proficiency_id proficiency_prof_safecracking( "prof_safecracking" );

static const quality_id qual_CUT( "CUT" );
static const quality_id qual_HACK( "HACK" );
static const quality_id qual_LOCKPICK( "LOCKPICK" );
static const quality_id qual_PRY( "PRY" );
static const quality_id qual_PRYING_NAIL( "PRYING_NAIL" );
static const quality_id qual_SAW_M( "SAW_M" );
static const quality_id qual_SHEAR( "SHEAR" );

static const skill_id skill_computer( "computer" );
static const skill_id skill_electronics( "electronics" );
static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_mechanics( "mechanics" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_swimming( "swimming" );
static const skill_id skill_traps( "traps" );

static const species_id species_ZOMBIE( "ZOMBIE" );

static const ter_str_id ter_t_card_reader_broken( "t_card_reader_broken" );
static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_dirtmound( "t_dirtmound" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_locked_alarm( "t_door_locked_alarm" );
static const ter_str_id ter_t_door_metal_c( "t_door_metal_c" );
static const ter_str_id ter_t_door_metal_locked( "t_door_metal_locked" );
static const ter_str_id ter_t_stump( "t_stump" );
static const ter_str_id ter_t_trunk( "t_trunk" );
static const ter_str_id ter_t_underbrush_harvested_autumn( "t_underbrush_harvested_autumn" );
static const ter_str_id ter_t_underbrush_harvested_spring( "t_underbrush_harvested_spring" );
static const ter_str_id ter_t_underbrush_harvested_summer( "t_underbrush_harvested_summer" );
static const ter_str_id ter_t_underbrush_harvested_winter( "t_underbrush_harvested_winter" );

static const trait_id trait_NUMB( "NUMB" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SCHIZOPHRENIC( "SCHIZOPHRENIC" );

static const vproto_id vehicle_prototype_none( "none" );

static const zone_type_id zone_type_LOOT_IGNORE( "LOOT_IGNORE" );
static const zone_type_id zone_type_LOOT_IGNORE_FAVORITES( "LOOT_IGNORE_FAVORITES" );
static const zone_type_id zone_type_STRIP_CORPSES( "STRIP_CORPSES" );
static const zone_type_id zone_type_UNLOAD_ALL( "UNLOAD_ALL" );

static const std::string gun_mechanical_simple( "gun_mechanical_simple" );

std::string activity_actor::get_progress_message( const player_activity &act ) const
{
    if( act.moves_total > 0 ) {
        const int pct = ( ( act.moves_total - act.moves_left ) * 100 ) / act.moves_total;
        return string_format( "%d%%", pct );
    } else {
        return std::string();
    }
}

aim_activity_actor::aim_activity_actor()
{
    initial_view_offset = get_avatar().view_offset;
}

aim_activity_actor aim_activity_actor::use_wielded()
{
    return aim_activity_actor();
}

aim_activity_actor aim_activity_actor::use_bionic( const item &fake_gun )
{
    aim_activity_actor act = aim_activity_actor();
    act.fake_weapon = fake_gun;
    return act;
}

aim_activity_actor aim_activity_actor::use_mutation( const item &fake_gun )
{
    aim_activity_actor act = aim_activity_actor();
    act.fake_weapon = fake_gun;
    return act;
}

void aim_activity_actor::start( player_activity &act, Character &who )
{
    item_location weapon = get_weapon();
    item &it = *weapon.get_item();

    if( !check_gun_ability_to_shoot( who, it ) ) {
        aborted = true;
        finish( act, who );
    }

    // Time spent on aiming is determined on the go by the player
    act.moves_total = 1;
    act.moves_left = 1;
}

bool aim_activity_actor::check_gun_ability_to_shoot( Character &who, item &it )
{

    if( it.has_fault_flag( "RUINED_GUN" ) ) {
        who.add_msg_if_player( m_bad, _( "Your %s is little more than an awkward club now." ), it.tname() );
        return false;
    }

    // if it's a simple fault, character can try to fix it on the fly
    if( faults::random_of_type_item_has( it, gun_mechanical_simple ) != fault_id::NULL_ID() ) {
        // fixing fault should cost more than 1 second
        // but until game running the next activity actor without ever verifying
        // was the previous one successful or not will be resolved,
        // it would be safer to limit it somewhat
        who.mod_moves( -who.get_speed() );
        who.recoil = MAX_RECOIL;
        if( one_in( std::max( 7.0f, ( 15.0f - ( 4.0f * who.get_skill_level( skill_gun ) ) ) ) ) ) {
            who.add_msg_if_player( m_good,
                                   _( "Your %s has some mechanical malfunction.  You tried to quickly fix it, and it works now!" ),
                                   it.tname() );
            it.remove_single_fault_of_type( gun_mechanical_simple );
            it.set_var( "u_know_round_in_chamber", true );
        } else {
            who.add_msg_if_player( m_bad,
                                   _( "Your %s has some mechanical malfunction.  You tried to quickly fix it, but failed!" ),
                                   it.tname() );
            return false;
        }
    }

    if( it.has_fault_flag( "OVERHEATED_GUN" ) ) {
        who.add_msg_if_player( m_warning,
                               _( "Your %s is too hot, and little screen signalizes the gun is inoperable." ), it.tname() );
        return false;
    }

    return true;
}

void aim_activity_actor::do_turn( player_activity &act, Character &who )
{
    map &here = get_map();

    if( !who.is_avatar() ) {
        debugmsg( "ACT_AIM not implemented for NPCs" );
        aborted = true;
        act.moves_left = 0;
        return;
    }
    avatar &you = get_avatar();

    item_location weapon = get_weapon();
    if( !weapon || !avatar_action::can_fire_weapon( you, here, *weapon ) ) {
        aborted = true;
        act.moves_left = 0;
        return;
    }

    gun_mode gun = weapon->gun_current_mode();
    // We need to make sure RAS weapon is loaded/reloaded in case the aim activity was temp. suspended
    // therefore the order of evaluation matters here
    if( gun->has_flag( flag_RELOAD_AND_SHOOT ) && !gun->ammo_remaining( ) && !reload_loc ) {
        if( !load_RAS_weapon() ) {
            aborted = true;
            act.moves_left = 0;
            return;
        }
    }

    if( gun->has_flag( json_flag_ALWAYS_AIMED ) ) {
        you.recoil = 0;
    }

    g->temp_exit_fullscreen();
    target_handler::trajectory trajectory = target_handler::mode_fire( you, *this );
    g->reenter_fullscreen();

    if( aborted ) {
        act.moves_left = 0;
    } else {
        if( !trajectory.empty() ) {
            fin_trajectory = trajectory;
            act.moves_left = 0;
        }

        // Allow interrupting activity only during 'aim and fire'.
        // Prevents '.' key for 'aim for 10 turns' from conflicting with '.' key for 'interrupt activity'
        // in case of high input lag (curses, sdl sometimes...), but allows to interrupt aiming
        // if a bug happens / stars align to cause an endless aiming loop.
        act.interruptable_with_kb = action != "AIM";
    }
}

void aim_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();

    act.set_to_null();
    item_location weapon = get_weapon();
    if( !weapon ) {
        restore_view();
        return;
    }
    if( aborted ) {
        unload_RAS_weapon();
        if( reload_requested ) {
            // Reload the gun / select different arrows
            // May assign ACT_RELOAD
            g->reload_wielded( true );
        }
        restore_view();
        return;
    }

    gun_mode gun = weapon->gun_current_mode();
    who.fire_gun( here, fin_trajectory.back(), gun.qty, *gun, reload_loc );

    if( !get_option<bool>( "AIM_AFTER_FIRING" ) ||
        weapon.get_item()->has_fault_of_type( gun_mechanical_simple ) ) {
        restore_view();
        return;
    }

    // re-enter aiming UI with same parameters
    aim_activity_actor aim_actor;
    aim_actor.abort_if_no_targets = true;
    aim_actor.fake_weapon = this->fake_weapon;
    aim_actor.initial_view_offset = this->initial_view_offset;

    // if invalid target or it's dead - reset it so a new one is acquired
    shared_ptr_fast<Creature> last_target = who.last_target.lock();
    if( !last_target || last_target->is_dead_state() ) {
        who.last_target.reset();
    }
    who.assign_activity( aim_actor );
}

void aim_activity_actor::canceled( player_activity &/*act*/, Character &/*who*/ )
{
    unload_RAS_weapon();
    restore_view();
}

void aim_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "fake_weapon", fake_weapon );
    jsout.member( "fin_trajectory", fin_trajectory );
    jsout.member( "action", action );
    jsout.member( "aif_duration", aif_duration );
    jsout.member( "aiming_at_critter", aiming_at_critter );
    jsout.member( "snap_to_target", snap_to_target );
    jsout.member( "reload_loc", reload_loc );
    jsout.member( "shifting_view", shifting_view );
    jsout.member( "initial_view_offset", initial_view_offset );
    jsout.member( "aborted", aborted );
    jsout.member( "reload_requested", reload_requested );
    jsout.member( "abort_if_no_targets", abort_if_no_targets );
    jsout.member( "should_unload_RAS", should_unload_RAS );

    jsout.end_object();
}

std::unique_ptr<activity_actor> aim_activity_actor::deserialize( JsonValue &jsin )
{
    aim_activity_actor actor = aim_activity_actor();

    JsonObject data = jsin.get_object();

    data.read( "fake_weapon", actor.fake_weapon );
    data.read( "fin_trajectory", actor.fin_trajectory );
    data.read( "action", actor.action );
    data.read( "aif_duration", actor.aif_duration );
    data.read( "aiming_at_critter", actor.aiming_at_critter );
    data.read( "snap_to_target", actor.snap_to_target );
    data.read( "reload_loc", actor.reload_loc );
    data.read( "shifting_view", actor.shifting_view );
    data.read( "initial_view_offset", actor.initial_view_offset );
    data.read( "aborted", actor.aborted );
    data.read( "reload_requested", actor.reload_requested );
    data.read( "abort_if_no_targets", actor.abort_if_no_targets );
    data.read( "should_unload_RAS", actor.should_unload_RAS );

    return actor.clone();
}

item_location aim_activity_actor::get_weapon()
{
    if( fake_weapon.has_value() ) {
        // TODO: check if the player lost relevant bionic/mutation
        return item_location( get_player_character(), &fake_weapon.value() );
    } else {
        // Check for lost gun (e.g. yanked by zombie technician)
        // TODO: check that this is the same gun that was used to start aiming
        return get_player_character().get_wielded_item();

    }
}

void aim_activity_actor::restore_view() const
{
    avatar &player_character = get_avatar();
    bool changed_z = player_character.view_offset.z() != initial_view_offset.z();
    player_character.view_offset = initial_view_offset;
    if( changed_z ) {
        // reality_bubble to indicate need to adjust when/if remote operation is implemented.
        reality_bubble().invalidate_map_cache( player_character.view_offset.z() );
        g->invalidate_main_ui_adaptor();
    }
}

bool aim_activity_actor::load_RAS_weapon()
{
    // TODO: use activity for fetching ammo and loading weapon
    Character &you = get_avatar();
    item_location weapon = get_weapon();
    gun_mode gun = weapon->gun_current_mode();
    // selected gun_mode might come from a gunmod
    item_location used_gun = &*gun != &*weapon ? item_location( weapon, &*gun ) : weapon;
    const auto ammo_location_is_valid = [&]() -> bool {
        if( !you.ammo_location )
        {
            return false;
        }
        if( !gun->can_reload_with( *you.ammo_location.get_item(), false ) )
        {
            return false;
        }
        if( square_dist( you.pos_abs(), you.ammo_location.pos_abs() ) > 1 )
        {
            return false;
        }
        return true;
    };
    item::reload_option opt = ammo_location_is_valid() ? item::reload_option( &you, weapon,
                              you.ammo_location ) : you.select_ammo( used_gun );
    if( !opt ) {
        // Menu canceled
        return false;
    }


    reload_loc = opt.ammo;
    return true;
}

void aim_activity_actor::unload_RAS_weapon()
{
    avatar &you = get_avatar();
    item_location weapon = get_weapon();
    if( !weapon || !should_unload_RAS ) {
        return;
    }

    gun_mode gun = weapon->gun_current_mode();
    if( gun->has_flag( flag_RELOAD_AND_SHOOT ) ) {
        if( gun->ammo_remaining( ) ) {
            item_location loc = item_location( you, gun.target );
            you.unload( loc, true );
        }
    }
}

void autodrive_activity_actor::update_player_vehicle( Character &who )
{
    const bool in_vehicle = who.in_vehicle && who.controlling_vehicle;
    const optional_vpart_position vp = get_map().veh_at( who.pos_bub() );
    if( !( vp && in_vehicle ) ) {
        player_vehicle = nullptr;
    } else {
        player_vehicle = &vp->vehicle();
    }
}

void autodrive_activity_actor::start( player_activity &, Character &who )
{
    update_player_vehicle( who );
    if( player_vehicle == nullptr ) {
        who.cancel_activity();
    } else {
        player_vehicle->is_autodriving = true;
    }
}

void autodrive_activity_actor::do_turn( player_activity &act, Character &who )
{
    map &here = get_map();

    if( who.in_vehicle && who.controlling_vehicle && player_vehicle ) {
        if( who.get_moves() <= 0 ) {
            // out of moves? the driver's not doing anything this turn
            // (but the vehicle will continue moving)
            return;
        }
        switch( player_vehicle->do_autodrive( here, who ) ) {
            case autodrive_result::ok:
                if( who.get_moves() > 0 ) {
                    // if do_autodrive() didn't eat up all our moves, end the turn
                    // equivalent to player pressing the "pause" button
                    who.set_moves( 0 );
                }
                sounds::reset_markers();
                break;
            case autodrive_result::abort:
                who.cancel_activity();
                break;
            case autodrive_result::finished:
                act.moves_left = 0;
                break;
        }
    } else {
        who.cancel_activity();
    }
}

void autodrive_activity_actor::canceled( player_activity &act, Character &who )
{
    // The activity might be canceled because the driver got unboarded, in which case we may not have a valid pointer anymore (e.g. summoned vehicle).
    update_player_vehicle( who );

    who.add_msg_if_player( m_info, _( "Auto drive canceled." ) );
    if( player_vehicle ) {
        player_vehicle->stop_autodriving( false );
    }
    ui::omap::force_quit();
    act.set_to_null();
}

void autodrive_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( m_info, _( "You have reached your destination." ) );
    player_vehicle->stop_autodriving( false );
    ui::omap::force_quit();
    act.set_to_null();
}

void autodrive_activity_actor::serialize( JsonOut &jsout ) const
{
    // Activity is not being saved but still provide some valid json if called.
    jsout.write_null();
}

std::unique_ptr<activity_actor> autodrive_activity_actor::deserialize( JsonValue & )
{
    return autodrive_activity_actor().clone();
}

void bash_activity_actor::do_turn( player_activity &, Character &who )
{
    avatar *player = who.as_avatar();
    if( player == nullptr ) {
        debugmsg( "An NPC (%s) has been assigned ACT_BASH", who.get_name() );
        who.cancel_activity();
        return;
    }
    while( who.get_moves() > 0 ) {
        avatar::smash_result res = player->smash( target );
        if( !res.did_smash ) {
            add_msg( m_info, _( "Something got in the way of smashing." ) );
            who.cancel_activity();
            return;
        }
        if( res.success ) {
            who.cancel_activity();
            return;
        }
        if( res.resistance < 0 || res.skill < res.resistance ) {
            add_msg( m_info, _( "You're no longer able to make progress smashing here." ) );
            who.cancel_activity();
            return;
        }
    }
}

void bash_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "target", target );
    jsout.end_object();
}

std::unique_ptr<activity_actor> bash_activity_actor::deserialize( JsonValue &jsin )
{
    bash_activity_actor actor( tripoint_bub_ms::zero );

    JsonObject data = jsin.get_object();
    data.read( "target", actor.target );

    return actor.clone();
}

void gunmod_remove_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void gunmod_remove_activity_actor::finish( player_activity &act, Character &who )
{
    item *it_gun = gun.get_item();
    if( !it_gun ) {
        debugmsg( "ACT_GUNMOD_REMOVE lost target gun" );
        act.set_to_null();
        return;
    }
    item *it_mod = nullptr;
    std::vector<item *> mods = it_gun->gunmods();
    if( gunmod_idx >= 0 && mods.size() > static_cast<size_t>( gunmod_idx ) ) {
        it_mod = mods[gunmod_idx];
    } else {
        debugmsg( "ACT_GUNMOD_REMOVE lost target gunmod" );
        act.set_to_null();
        return;
    }
    act.set_to_null();
    gunmod_remove( who, *it_gun, *it_mod );
    it_gun->on_contents_changed();
}

bool gunmod_remove_activity_actor::gunmod_unload( Character &who, item &gunmod )
{
    if( gunmod.has_flag( flag_BRASS_CATCHER ) ) {
        // Exclude brass catchers so that removing them wouldn't spill the casings
        return true;
    }
    // TODO: unloading gunmods happens instantaneously in some cases, but should take time
    // Also note, gunmod_unload is called twice, once in
    // gunmod_remove_activity_actor::gunmod_remove
    // and once in
    // Character::gunmod_remove
    // Remove one of them before making gunmod_unload take time
    item_location loc = item_location( who, &gunmod );
    return !( gunmod.ammo_remaining( ) && !who.unload( loc, true ) );
}

void gunmod_remove_activity_actor::gunmod_remove( Character &who, item &gun, item &mod )
{
    if( !gunmod_unload( who, mod ) ) {
        return;
    }

    gun.gun_set_mode( gun_mode_DEFAULT );
    const itype *modtype = mod.type;

    who.i_add_or_drop( mod );
    gun.remove_item( mod );

    //~ %1$s - gunmod, %2$s - gun.
    who.add_msg_if_player( _( "You remove your %1$s from your %2$s." ), modtype->nname( 1 ),
                           gun.tname() );
}

void gunmod_remove_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "gun", gun );
    jsout.member( "gunmod", gunmod_idx );

    jsout.end_object();
}

std::unique_ptr<activity_actor> gunmod_remove_activity_actor::deserialize( JsonValue &jsin )
{
    gunmod_remove_activity_actor actor( 0, item_location(), -1 );

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "gun", actor.gun );
    data.read( "gunmod", actor.gunmod_idx );

    return actor.clone();
}

void hacking_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 5_minutes );
    act.moves_left = to_moves<int>( 5_minutes );
}

enum class hack_result : int {
    UNABLE,
    FAIL,
    NOTHING,
    SUCCESS
};

enum class hack_type : int {
    SAFE,
    DOOR,
    GAS,
    NONE
};

static int hack_level( const Character &who, item_location &tool )
{
    // Quality of 2 is a base electrohack, which results in no modifier
    int tool_quality_modifier = tool->get_quality( qual_HACK ) - 2;
    ///\EFFECT_COMPUTER increases success chance of hacking
    // odds go up with int>8, down with int<8
    // 4 int stat is worth 1 computer skill here
    ///\EFFECT_INT increases success chance of hacking
    // tool_quality_modifier may increase or reduce the success chance of hacking
    return round( who.get_greater_skill_or_knowledge_level( skill_computer ) + tool_quality_modifier +
                  static_cast<float>
                  ( who.int_cur ) / 2.0f - 8 );
}

static hack_result hack_attempt( Character &who, item_location &tool )
{
    map &here = get_map();

    // TODO: Remove this once player -> Character migration is complete
    {
        who.practice( skill_computer, 20 );
    }

    // only skilled supergenius never cause short circuits, but the odds are low for people
    // with moderate skills
    const int hack_stddev = 5;
    int success = std::ceil( normal_roll( hack_level( who, tool ), hack_stddev ) );
    if( success < 0 ) {
        who.add_msg_if_player( _( "You cause a short circuit!" ) );
        tool->ammo_consume( tool->ammo_required(), here, tool.pos_bub( here ), &who );

        if( success <= -5 ) {
            tool->ammo_consume( ( tool->ammo_required() * 2 ), here, tool.pos_bub( here ), &who );
        }
        return hack_result::FAIL;
    } else if( success < 6 ) {
        return hack_result::NOTHING;
    } else {
        return hack_result::SUCCESS;
    }
}

static hack_type get_hack_type( const tripoint_bub_ms &examp )
{
    hack_type type = hack_type::NONE;
    map &here = get_map();
    const furn_t &xfurn_t = here.furn( examp ).obj();
    const ter_t &xter_t = here.ter( examp ).obj();
    if( xter_t.has_examine( iexamine::pay_gas ) || xfurn_t.has_examine( iexamine::pay_gas ) ) {
        type = hack_type::GAS;
    } else if( xter_t.has_examine( "cardreader" ) ||
               xfurn_t.has_examine( "cardreader" ) ) {
        type = hack_type::DOOR;
    } else if( xter_t.has_examine( iexamine::gunsafe_el ) ||
               xfurn_t.has_examine( iexamine::gunsafe_el ) ) {
        type = hack_type::SAFE;
    }
    return type;
}

void hacking_activity_actor::finish( player_activity &act, Character &who )
{
    tripoint_bub_ms examp = get_map().get_bub( act.placement );
    hack_type type = get_hack_type( examp );
    switch( hack_attempt( who, tool ) ) {
        case hack_result::UNABLE:
            who.add_msg_if_player( _( "You cannot hack this." ) );
            break;
        case hack_result::FAIL:
            // currently all things that can be hacked have equivalent alarm failure states.
            // this may not always be the case with new hackable things.
            get_event_bus().send<event_type::triggers_alarm>( who.getID() );
            sounds::sound( who.pos_bub(), MAX_VIEW_DISTANCE, sounds::sound_t::music, _( "an alarm sound!" ),
                           true,
                           "environment", "alarm" );
            break;
        case hack_result::NOTHING:
            who.add_msg_if_player( _( "You fail the hack, but no alarms are triggered." ) );
            break;
        case hack_result::SUCCESS:
            map &here = get_map();
            if( type == hack_type::GAS ) {
                int tankUnits;
                fuel_station_fuel_type fuelType;
                const std::optional<tripoint_bub_ms> pTank_ = iexamine::getNearFilledGasTank( examp, tankUnits,
                        fuelType );
                if( !pTank_ ) {
                    break;
                }
                const tripoint_bub_ms pTank = *pTank_;
                const std::optional<tripoint_bub_ms> pGasPump = iexamine::getGasPumpByNumber( examp,
                        uistate.ags_pay_gas_selected_pump );
                if( pGasPump && iexamine::toPumpFuel( pTank, *pGasPump, tankUnits ) ) {
                    who.add_msg_if_player( _( "You hack the terminal and route all available fuel to your pump!" ) );
                    sounds::sound( examp, 6, sounds::sound_t::activity,
                                   _( "Glug Glug Glug Glug Glug Glug Glug Glug Glug" ), true, "tool", "gaspump" );
                } else {
                    who.add_msg_if_player( _( "Nothing happens." ) );
                }
            } else if( type == hack_type::SAFE ) {
                who.add_msg_if_player( m_good, _( "The door on the safe swings open." ) );
                here.furn_set( examp, furn_f_safe_o );
            } else if( type == hack_type::DOOR ) {
                who.add_msg_if_player( _( "You activate the panel!" ) );
                who.add_msg_if_player( m_good, _( "The nearby doors unlock." ) );
                here.ter_set( examp, ter_t_card_reader_broken );
                for( const tripoint_bub_ms &tmp : here.points_in_radius( examp, 3 ) ) {
                    if( here.ter( tmp ) == ter_t_door_metal_locked ) {
                        here.ter_set( tmp, ter_t_door_metal_c );
                    }
                }
            }
            break;
    }
    act.set_to_null();
}

void hacking_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> hacking_activity_actor::deserialize( JsonValue &jsin )
{
    hacking_activity_actor actor( {} );
    JsonObject data = jsin.get_object();
    data.read( "tool", actor.tool );
    return actor.clone();
}

void bookbinder_copy_activity_actor::start( player_activity &act, Character & )
{
    pages = bookbinder_copy_activity_actor::pages_for_recipe( *rec_id );
    act.moves_total = to_moves<int>( pages * 10_minutes );
    act.moves_left = to_moves<int>( pages * 10_minutes );
}

void bookbinder_copy_activity_actor::do_turn( player_activity &, Character &p )
{
    if( p.fine_detail_vision_mod() > 4.0f ) {
        p.cancel_activity();
        p.add_msg_if_player( m_info, _( "It's too dark to write!" ) );
        return;
    }
}

void bookbinder_copy_activity_actor::finish( player_activity &act, Character &p )
{
    if( !book_binder->can_contain( item( itype_paper, calendar::turn, pages ) ).success() ) {
        debugmsg( "Book binder can not contain '%s' recipe when it should.", rec_id.str() );
        act.set_to_null();
        return;
    }

    std::set<recipe_id> recipes = book_binder->get_saved_recipes();

    if( recipes.emplace( rec_id ).second ) { // if .second is true then recipe was inserted
        book_binder->set_saved_recipes( recipes );
        p.add_msg_if_player( m_good, _( "You copy the recipe for %1$s into your recipe book." ),
                             rec_id->result_name() );

        p.use_charges( itype_paper, pages );

        const std::vector<const item *> writing_tools_filter =
        p.crafting_inventory().items_with( [&]( const item & it ) {
            return it.has_flag( flag_WRITE_MESSAGE ) && it.ammo_remaining( ) >= it.ammo_required() ;
        } );

        std::vector<tool_comp> writing_tools;
        writing_tools.reserve( writing_tools_filter.size() );
        for( const item *tool : writing_tools_filter ) {
            writing_tools.emplace_back( tool->typeId(), 1 );
        }

        p.consume_tools( writing_tools, pages );
        book_binder->put_in( item( itype_paper, calendar::turn, pages ),
                             pocket_type::MAGAZINE );
    } else {
        debugmsg( "Recipe book already has '%s' recipe when it should not.", rec_id.str() );
    }

    act.set_to_null();
}

void bookbinder_copy_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "book_binder", book_binder );
    jsout.member( "rec_id", rec_id );
    jsout.member( "pages", pages );
    jsout.end_object();
}

std::unique_ptr<activity_actor> bookbinder_copy_activity_actor::deserialize( JsonValue &jsin )
{
    bookbinder_copy_activity_actor actor;

    JsonObject jsobj = jsin.get_object();
    jsobj.read( "book_binder", actor.book_binder );
    jsobj.read( "rec_id", actor.rec_id );
    jsobj.read( "pages", actor.pages );

    return actor.clone();
}

void hotwire_car_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void hotwire_car_activity_actor::do_turn( player_activity &act, Character &who )
{
    map &here = get_map();
    if( calendar::once_every( 1_minutes ) ) {
        bool lost = !here.veh_at( here.get_bub( target ) ).has_value();
        if( lost ) {
            act.set_to_null();
            debugmsg( "Lost ACT_HOTWIRE_CAR target vehicle" );
        }
        if( calendar::once_every( 5_minutes ) ) {
            who.practice( skill_mechanics, 1, 2, true );
        }
    }
}

void hotwire_car_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();

    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( here.get_bub( target ) );
    if( !vp ) {
        debugmsg( "Lost ACT_HOTWIRE_CAR target vehicle" );
        return;
    }
    vehicle &veh = vp->vehicle();

    int skill = round( who.get_average_skill_level( skill_mechanics ) );
    if( skill > rng( 1, 6 ) ) {
        // Success
        who.add_msg_if_player( _( "You found the wire that starts the engine." ) );
        veh.is_locked = false;
    } else if( skill > rng( 0, 4 ) ) {
        // Soft fail
        who.add_msg_if_player( _( "You found a wire that looks like the right one." ) );
        veh.is_alarm_on = veh.has_security_working( here );
        veh.is_locked = false;
    } else if( !veh.is_alarm_on ) {
        // Hard fail
        who.add_msg_if_player( _( "The red wire always starts the engine, doesn't it?" ) );
        veh.is_alarm_on = veh.has_security_working( here );
    } else {
        // Already failed
        who.add_msg_if_player(
            _( "By process of elimination, you found the wire that starts the engine." ) );
        veh.is_locked = false;
    }
}

void hotwire_car_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "target", target );
    jsout.member( "moves_total", moves_total );

    jsout.end_object();
}

std::unique_ptr<activity_actor> hotwire_car_activity_actor::deserialize( JsonValue &jsin )
{
    hotwire_car_activity_actor actor( 0, tripoint_abs_ms::zero );

    JsonObject data = jsin.get_object();

    data.read( "target", actor.target );
    data.read( "moves_total", actor.moves_total );

    return actor.clone();
}

void hacksaw_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    const map &here = get_map();
    int moves_before_quality;

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->hacksaw->valid() ) {
            if( !testing ) {
                debugmsg( "%s hacksaw is invalid", furn_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        moves_before_quality = to_moves<int>( furn_type->hacksaw->duration() );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->hacksaw->valid() ) {
            if( !testing ) {
                debugmsg( "%s hacksaw is invalid", ter_type.id().str() );
            }
            act.set_to_null();
            return;
        }
        moves_before_quality = to_moves<int>( ter_type->hacksaw->duration() );
    } else {
        if( !testing ) {
            debugmsg( "hacksaw activity called on invalid terrain" );
        }
        act.set_to_null();
        return;
    }

    int qual = 0;
    if( type.has_value() ) {
        item veh_tool = item( type.value(), calendar::turn );
        for( const std::pair<const quality_id, int> &quality : type.value()->qualities ) {
            if( quality.first == qual_SAW_M ) {
                qual = quality.second;
            }
        }
        for( const std::pair<const quality_id, int> &quality : type.value()->charged_qualities ) {
            if( quality.first == qual_SAW_M ) {
                qual = std::max( qual, quality.second );
            }
        }
    } else {
        qual = tool->get_quality( qual_SAW_M );
    }
    if( qual < 2 ) {
        if( !testing ) {
            debugmsg( "Item %s with 'HACKSAW' use action requires SAW_M quality of at least 2.",
                      tool->typeId().c_str() );
        }
        act.set_to_null();
        return;
    }

    //Speed of hacksaw action is the SAW_M quality over 2, 2 being the hacksaw level.
    //3 makes the speed one-and-a-half times, and the total moves 66% of hacksaw. 4 is twice the speed, 50% the moves, etc, 5 is 2.5 times the speed, 40% the original time
    //done because it's easy to code, and diminising returns on cutting speed make sense as the limiting factor becomes aligning the tool and controlling it instead of the actual cutting
    act.moves_total = moves_before_quality / ( qual / 2 );
    add_msg_debug( debugmode::DF_ACTIVITY, "%s moves_total: %d", act.id().str(), act.moves_total );
    act.moves_left = act.moves_total;
}

void hacksaw_activity_actor::do_turn( player_activity &/*act*/, Character &who )
{
    map &here = get_map();

    std::string method = "HACKSAW";

    if( !veh_pos.has_value() ) {
        if( tool->ammo_sufficient( &who, method ) ) {
            int ammo_consumed = tool->ammo_required();
            std::map<std::string, int>::const_iterator iter = tool->type->ammo_scale.find( method );
            if( iter != tool->type->ammo_scale.end() ) {
                ammo_consumed *= iter->second;
            }

            tool->ammo_consume( ammo_consumed, here, tool.pos_bub( here ), &who );
            sfx::play_activity_sound( "tool", "hacksaw", sfx::get_heard_volume( target ) );
            if( calendar::once_every( 1_minutes ) ) {
                //~ Sound of a metal sawing tool at work!
                sounds::sound( target, 15, sounds::sound_t::destructive_activity, _( "grnd grnd grnd" ) );
            }
        } else {
            if( who.is_avatar() ) {
                who.add_msg_if_player( m_bad, _( "Your %1$s ran out of charges." ), tool->tname() );
            } else { // who.is_npc()
                add_msg_if_player_sees( who.pos_bub(), _( "%1$s %2$s ran out of charges." ), who.disp_name( false,
                                        true ), tool->tname() );
            }
            who.cancel_activity();
        }
    } else {
        map &here = get_map();
        const optional_vpart_position vp = here.veh_at( veh_pos.value() );
        if( !vp ) {
            debugmsg( "Lost ACT_HACKSAW vehicle tool" );
            return;
        }
        vehicle &veh = vp->vehicle();
        if( vehicle::use_vehicle_tool( veh, &here, veh_pos.value(), type.value(), true ) ) {
            sfx::play_activity_sound( "tool", "hacksaw", sfx::get_heard_volume( target ) );
            if( calendar::once_every( 1_minutes ) ) {
                //~ Sound of a metal sawing tool at work!
                sounds::sound( target, 15, sounds::sound_t::destructive_activity, _( "grnd grnd grnd" ) );
            }
        } else {
            if( who.is_avatar() ) {
                who.add_msg_if_player( m_bad, _( "Your %1$s ran out of charges." ), type.value()->nname( 1 ) );
            } else { // who.is_npc()
                add_msg_if_player_sees( who.pos_bub(), _( "%1$s %2$s ran out of charges." ), who.disp_name( false,
                                        true ), type.value()->nname( 1 ) );
            }
            who.cancel_activity();
        }
    }
}

void hacksaw_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    const activity_data_common *data;

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->hacksaw->valid() ) {
            if( !testing ) {
                debugmsg( "%s hacksaw is invalid", furn_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        const furn_str_id new_furn = furn_type->hacksaw->result();
        if( !new_furn.is_valid() ) {
            if( !testing ) {
                debugmsg( "hacksaw furniture: %s invalid furniture", new_furn.str() );
            }
            act.set_to_null();
            return;
        }

        data = static_cast<const activity_data_common *>( &*furn_type->hacksaw );
        here.furn_set( target, new_furn );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->hacksaw->valid() ) {
            if( !testing ) {
                debugmsg( "%s hacksaw is invalid", ter_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        const ter_str_id new_ter = ter_type->hacksaw->result();
        if( !new_ter.is_valid() ) {
            if( !testing ) {
                debugmsg( "hacksaw terrain: %s invalid terrain", new_ter.str() );
            }
            act.set_to_null();
            return;
        }

        data = static_cast<const activity_data_common *>( &*ter_type->hacksaw );
        here.ter_set( target, new_ter );
    } else {
        if( !testing ) {
            debugmsg( "hacksaw activity finished on invalid terrain" );
        }
        act.set_to_null();
        return;
    }

    for( const activity_byproduct &byproduct : data->byproducts() ) {
        const int amount = byproduct.roll();
        if( byproduct.item->count_by_charges() ) {
            item byproduct_item( byproduct.item, calendar::turn, amount );
            here.add_item_or_charges( target, byproduct_item );
        } else {
            item byproduct_item( byproduct.item, calendar::turn );
            for( int i = 0; i < amount; ++i ) {
                here.add_item_or_charges( target, byproduct_item );
            }
        }
    }

    if( !data->message().empty() ) {
        who.add_msg_if_player( m_info, data->message().translated() );
    }

    act.set_to_null();
}

//TODO: Make hacksawing resumable with different tools with the same SAW_M quality.
//Potentially make it possible to resume with different SAW_M quality and recalculate time to completion partway through.
//This is really not a big deal, and will cost a few minutes of in game time and part of a medium battery charge at worst as someone accidentally cancels the activity_actor and has to start again
//If a few minutes are life and death, sawing metal may not be the wise choice in the first place.
bool hacksaw_activity_actor::can_resume_with_internal( const activity_actor &other,
        const Character &/*who*/ ) const
{
    const hacksaw_activity_actor &actor = static_cast<const hacksaw_activity_actor &>
                                          ( other );
    return actor.target == target && ( ( veh_pos.has_value() &&
                                         veh_pos.value() == actor.veh_pos.value_or( tripoint_bub_ms::max ) ) ||
                                       actor.tool.operator == ( tool ) );
}

void hacksaw_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "target", target );
    jsout.member( "tool", tool );
    jsout.member( "type", type );
    jsout.member( "veh_pos", veh_pos );
    jsout.end_object();
}

std::unique_ptr<activity_actor> hacksaw_activity_actor::deserialize( JsonValue &jsin )
{
    hacksaw_activity_actor actor( {}, {} );
    JsonObject data = jsin.get_object();
    data.read( "target", actor.target );
    data.read( "tool", actor.tool );
    data.read( "type", actor.type );
    data.read( "veh_pos", actor.veh_pos );
    return actor.clone();
}

static std::string enumerate_ints_to_string( const std::vector<int> &vec )
{
    return enumerate_as_string( vec, []( const int &it ) {
        return std::to_string( it );
    } );
}

bikerack_racking_activity_actor::bikerack_racking_activity_actor( const vehicle &parent_vehicle,
        const vehicle &racked_vehicle, const std::vector<int> &racks )
    : racks( racks )
{
    map &here = get_map();
    parent_vehicle_pos = parent_vehicle.bub_part_pos( here,  0 );
    racked_vehicle_pos = racked_vehicle.bub_part_pos( here, 0 );
}

void bikerack_racking_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void bikerack_racking_activity_actor::finish( player_activity &act, Character & )
{
    map &here = get_map();

    const optional_vpart_position ovp_parent = here.veh_at( parent_vehicle_pos );
    if( !ovp_parent ) {
        debugmsg( "racking actor lost parent vehicle at %s", parent_vehicle_pos.to_string() );
        act.set_to_null();
        return;
    }

    const optional_vpart_position ovp_racked = here.veh_at( racked_vehicle_pos );
    if( !ovp_racked ) {
        debugmsg( "racking actor lost racked vehicle at %s", racked_vehicle_pos.to_string() );
        act.set_to_null();
        return;
    }

    vehicle &parent_veh = ovp_parent->vehicle();
    vehicle &racked_veh = ovp_racked->vehicle();

    if( !parent_veh.merge_rackable_vehicle( &here, &racked_veh, racks ) ) {
        debugmsg( "racking actor failed: failed racking %s on %s, racks: [%s].",
                  racked_veh.name, parent_veh.name, enumerate_ints_to_string( racks ) );
    }
    act.set_to_null();
}

void bikerack_racking_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "parent_vehicle_pos", parent_vehicle_pos );
    jsout.member( "racked_vehicle_pos", racked_vehicle_pos );
    jsout.member( "racks", racks );
    jsout.end_object();
}

std::unique_ptr<activity_actor> bikerack_racking_activity_actor::deserialize( JsonValue &jsin )
{
    bikerack_racking_activity_actor actor;
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "parent_vehicle_pos", actor.parent_vehicle_pos );
    data.read( "racked_vehicle_pos", actor.racked_vehicle_pos );
    data.read( "racks", actor.racks );

    return actor.clone();
}

void glide_activity_actor::do_turn( player_activity &act, Character &you )
{
    map &here = get_map();

    tripoint_rel_ms heading;
    if( jump_direction == 0 ) {
        heading = tripoint_rel_ms::south;
    }
    if( jump_direction == 1 ) {
        heading = tripoint_rel_ms::south_west;
    }
    if( jump_direction == 2 ) {
        heading = tripoint_rel_ms::west;
    }
    if( jump_direction == 3 ) {
        heading = tripoint_rel_ms::north_west;
    }
    if( jump_direction == 4 ) {
        heading = tripoint_rel_ms::north;
    }
    if( jump_direction == 5 ) {
        heading = tripoint_rel_ms::north_east;
    }
    if( jump_direction == 6 ) {
        heading = tripoint_rel_ms::east;
    }
    if( jump_direction == 7 ) {
        heading = tripoint_rel_ms::south_east;
    }
    const tripoint_abs_ms newpos = you.pos_abs() + heading;
    const tripoint_bub_ms checknewpos = you.pos_bub( here ) + heading;
    if( !here.is_open_air( you.pos_bub( here ) ) || heading == tripoint_rel_ms::zero ) {
        you.add_msg_player_or_npc( m_good,
                                   _( "You come to a gentle landing." ),
                                   _( "<npcname> comes to a gentle landing." ) );
        you.remove_effect( effect_gliding );
        you.gravity_check();
        act.set_to_null();
        return;
    }   // Have we crashed into a wall?
    if( here.impassable( checknewpos ) ) {
        you.add_msg_player_or_npc( m_bad,
                                   _( "You collide with %s, bringing an abrupt halt to your glide." ),
                                   _( "<npcname> collides with %s, bringing an abrupt halt to their glide." ),
                                   here.tername( checknewpos ) );
        you.remove_effect( effect_gliding );
        you.gravity_check();
        act.set_to_null();
        return;
    }
    if( !you.can_fly() ) {
        you.remove_effect( effect_gliding );
        you.gravity_check();
        act.set_to_null();
        return;
    }
    Creature *creature_ahead = get_creature_tracker().creature_at( newpos );
    if( creature_ahead && creature_ahead->get_size() >= creature_size::medium &&
        you.get_size() >= creature_size::medium ) {
        // Zombies are too stupid to avoid midair collision
        if( !you.dodge_check( 15, true ) || ( !creature_ahead->in_species( species_ZOMBIE ) &&
                                              !creature_ahead->dodge_check( 15, true ) ) ) {
            you.add_msg_player_or_npc( m_bad,
                                       _( "You collide with %s, bringing an abrupt halt to your glide." ),
                                       _( "<npcname> collides with %s, bringing an abrupt halt to their glide." ),
                                       creature_ahead->disp_name() );
            if( creature_ahead->get_size() < creature_size::huge ) {
                creature_ahead->add_effect( effect_downed, 2_turns, false );
            }
            you.remove_effect( effect_gliding );
            you.gravity_check();
            act.set_to_null();
            return;
        }
        you.add_msg_player_or_npc( m_good,
                                   _( "You deftly maneuver around %s." ),
                                   _( "<npcname> deftly maneuvers around %s." ), creature_ahead->disp_name() );
    }
    you.move_to( newpos );
    moved_tiles ++;
    if( moved_tiles >= glide_distance ) {
        g->vertical_move( -1, false, false );
        moved_tiles = 0;
    }
    you.mod_moves( -you.get_speed() * 0.5 );
    here.update_visibility_cache( you.posz() );
    if( you.is_avatar() ) {
        g->update_map( you );
    }
}

glide_activity_actor::glide_activity_actor( Character *you, int jump_direction, int glide_distance )
    : jump_direction( jump_direction ), glide_distance( glide_distance )
{
    you->add_effect( effect_gliding, 1_turns, true );
    // What's the purpose of assigning a variable that isn't used? Is some code AWOL?
    tripoint_rel_ms heading;
    if( jump_direction == 0 ) {
        heading = tripoint_rel_ms::south;
    }
    if( jump_direction == 1 ) {
        heading = tripoint_rel_ms::south_west;
    }
    if( jump_direction == 2 ) {
        heading = tripoint_rel_ms::west;
    }
    if( jump_direction == 3 ) {
        heading = tripoint_rel_ms::north_west;
    }
    if( jump_direction == 4 ) {
        heading = tripoint_rel_ms::north;
    }
    if( jump_direction == 5 ) {
        heading = tripoint_rel_ms::north_east;
    }
    if( jump_direction == 6 ) {
        heading = tripoint_rel_ms::east;
    }
    if( jump_direction == 7 ) {
        heading = tripoint_rel_ms::south_west;
    }
}

void glide_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moved_tiles", moved_tiles );
    jsout.member( "moves_total", moves_total );
    jsout.member( "jump_direction", jump_direction );
    jsout.member( "glide_distance", glide_distance );

    jsout.end_object();
}

std::unique_ptr<activity_actor> glide_activity_actor::deserialize( JsonValue &jsin )
{
    glide_activity_actor actor;
    JsonObject data = jsin.get_object();
    data.read( "moved_tiles", actor.moved_tiles );
    data.read( "moves_total", actor.moves_total );
    data.read( "jump_direction", actor.jump_direction );
    data.read( "glide_distance", actor.glide_distance );
    return actor.clone();
}
void glide_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void glide_activity_actor::finish( player_activity &act, Character &you )
{
    g->update_map( you );
    you.add_msg_player_or_npc( m_good,
                               _( "You come to a gentle landing." ),
                               _( "<npcname> comes to a gentle landing." ) );
    you.remove_effect( effect_gliding );
    you.gravity_check();
    act.set_to_null();
}

bikerack_unracking_activity_actor::bikerack_unracking_activity_actor( const vehicle &parent_vehicle,
        const std::vector<int> &parts, const std::vector<int> &racks )
    : parts( parts ), racks( racks )
{
    map &here = get_map();
    parent_vehicle_pos = parent_vehicle.bub_part_pos( here,  0 );
}

void bikerack_unracking_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void bikerack_unracking_activity_actor::finish( player_activity &act, Character & )
{
    map &here = get_map();

    const optional_vpart_position ovp = here.veh_at( parent_vehicle_pos );
    if( !ovp ) {
        debugmsg( "unracking actor lost vehicle." );
        act.set_to_null();
        return;
    }

    vehicle &parent_vehicle = ovp->vehicle();

    if( !parent_vehicle.remove_carried_vehicle( here, parts, racks ) ) {
        debugmsg( "unracking actor failed on %s, parts: [%s], racks: [%s]", parent_vehicle.name,
                  enumerate_ints_to_string( parts ), enumerate_ints_to_string( racks ) );
    }
    act.set_to_null();
}

void bikerack_unracking_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "parent_vehicle_pos", parent_vehicle_pos );
    jsout.member( "parts", parts );
    jsout.member( "racks", racks );
    jsout.end_object();
}

std::unique_ptr<activity_actor> bikerack_unracking_activity_actor::deserialize( JsonValue &jsin )
{
    bikerack_unracking_activity_actor actor;
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "parent_vehicle_pos", actor.parent_vehicle_pos );
    data.read( "parts", actor.parts );
    data.read( "racks", actor.racks );

    return actor.clone();
}

static bool is_valid_book( const item_location &book )
{
    // If book is invalid, perhaps we lost it during the activity
    // If book is valid but not a book, it's probably due to a bug where
    // `item_location` points to the wrong item after a save/load cycle.
    // In both cases cancel activity to avoid segfault.
    return book && book->is_book();
}

static bool cancel_if_book_invalid(
    player_activity &act, const item_location &book, const Character &who )
{
    if( !is_valid_book( book ) ) {
        who.add_msg_player_or_npc(
            _( "You no longer have the book!" ),
            _( "<npcname> no longer has the book!" ) );
        act.set_to_null();
        return true;
    }
    return false;
}

void read_activity_actor::start( player_activity &act, Character &who )
{
    if( cancel_if_book_invalid( act, book, who ) ) {
        return;
    }

    using_ereader = !!ereader;

    bktype = book->type->use_methods.count( "MA_MANUAL" ) ?
             book_type::martial_art : book_type::normal;

    // push copy of book for focus calculation
    // avatar::update_mental_focus and avatar::calc_focus_equilibrium
    act.targets.push_back( book );

    add_msg_debug( debugmode::DF_ACT_READ, "reading time = %s",
                   to_string_writable( time_duration::from_moves( moves_total ) ) );

    // starting the activity should cost a charge to boot up the ebook app
    if( using_ereader ) {
        ereader->ammo_consume( ereader->ammo_required(), who.pos_bub(), &who );
    }

    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void read_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( cancel_if_book_invalid( act, book, who ) ) {
        return;
    }

    if( !bktype.has_value() ) {
        bktype = book->type->use_methods.count( "MA_MANUAL" ) ?
                 book_type::martial_art : book_type::normal;
    }

    if( who.fine_detail_vision_mod() > 4 && !who.has_flag( json_flag_READ_IN_DARKNESS ) ) {
        // It got too dark during the process of reading, bail out.
        act.set_to_null();
        who.add_msg_if_player( m_bad, _( "It's too dark to read!" ) );
        return;
    }

    if( bktype.value() == book_type::martial_art && one_in( 3 ) ) {
        who.burn_energy_all( -1 );
    }

    // do not spam the message log
    if( calendar::once_every( 5_minutes ) ) {
        add_msg_debug( debugmode::DF_ACT_READ, "%s reading time = %s",
                       who.name, to_string_writable( time_duration::from_moves( act.moves_left ) ) );
    }

    if( using_ereader && !ereader ) {
        who.add_msg_player_or_npc(
            _( "You no longer have the e-book!" ),
            _( "<npcname> no longer has the e-book!" ) );
        who.cancel_activity();
        return;
    }

    if( using_ereader && !ereader->ammo_sufficient( &who ) ) {
        add_msg_if_player_sees(
            who,
            _( "%1$s %2$s ran out of batteries." ),
            who.disp_name( true, true ),
            item::nname( ereader->typeId() ) );
        who.cancel_activity();
        return;
    }

    if( using_ereader && calendar::once_every( 5_minutes ) ) {
        /** Expected battery life while reading with common ereaders:
                integrated_ar       - implant                                         = potentially infinite
                ar_glasses_advanced - max 300 charges from disposable light battery   = 25 hours of reading
                eink_tablet_pc      - max 300 charges from disposable light battery   = 25 hours of reading
                laptop              - max 1200 charges from disposable medium battery = 100 hours of reading
                smart_phone         - 120 UPS charges                                 = 10 hours of reading
        */
        ereader->ammo_consume( ereader->ammo_required(), who.pos_bub(), &who );
    }
}

void read_activity_actor::read_book( Character &learner,
                                     const cata::value_ptr<islot_book> &islotbook,
                                     SkillLevel &skill_level, double penalty )
{
    const int originalSkillLevel = skill_level.knowledgeLevel();

    // Calculate experience gained
    /** @EFFECT_INT increases reading comprehension */
    // Enhanced Memory Banks modestly boosts experience
    int min_ex = std::max( 1, to_minutes<int>( islotbook->time ) / 10 + learner.get_int() / 4 );
    int max_ex = to_minutes<int>( islotbook->time ) / 5 + learner.get_int() / 2 - originalSkillLevel;

    min_ex = learner.enchantment_cache->modify_value( enchant_vals::mod::READING_EXP, min_ex );

    min_ex = std::lround( learner.adjust_for_focus( min_ex ) );
    max_ex = std::lround( learner.adjust_for_focus( max_ex ) );

    max_ex = clamp( max_ex, 2, 10 );
    max_ex = std::max( min_ex, max_ex );

    min_ex *= ( originalSkillLevel + 1 ) * penalty;
    min_ex = std::max( min_ex, 1 );

    max_ex *= ( originalSkillLevel + 1 ) * penalty;
    max_ex = std::max( min_ex, max_ex );

    add_msg_debug( debugmode::DF_ACT_READ, "%s read exp: min_ex %d; max_ex %d",
                   learner.disp_name(), min_ex, max_ex );
    skill_level.readBook( min_ex, max_ex, islotbook->level );
}

bool read_activity_actor::player_read( avatar &you )
{
    if( !you.has_identified( book->typeId() ) ) {
        you.identify( *book );
        return true;
    }

    std::vector<std::string> fail_messages;
    const Character *reader = you.get_book_reader( *book, fail_messages );
    if( reader == nullptr ) {
        // We can't read, and neither can our followers
        for( const std::string &reason : fail_messages ) {
            add_msg( m_bad, reason );
        }
        return true;
    }

    const cata::value_ptr<islot_book> &islotbook = book->type->book;
    const skill_id &skill = islotbook->skill;

    std::vector<std::string> little_learned;
    std::vector<std::string> cant_learn;

    std::vector<Character *> learners;
    for( Character *learner : you.get_crafting_helpers() ) {
        const book_mastery mastery = learner->get_book_mastery( *book );
        const bool morale_req = learner->fun_to_read( *book ) || learner->has_morale_to_read();

        if( learner->is_deaf() && learner != reader ) {
            continue;
        }

        if( // fun readers
            ( mastery == book_mastery::MASTERED && learner->fun_to_read( *book ) )
            // reading for experience
            || ( mastery == book_mastery::LEARNING && morale_req )
        ) {
            learners.push_back( learner );
        }
    }

    // caller should check if player
    // can understand the book
    learners.push_back( you.as_character() );

    // Characters can move while learning from a book
    // Check ID of all learners to avoid
    // getting stuck in an infinite loop
    bool learner_left = true;
    for( Character *learner : learners ) {

        double penalty = 1.0;

        const bool reading_for_skill =
            learner->get_book_mastery( *book ) == book_mastery::LEARNING;

        const int book_fun = learner->book_fun_for( *book, *learner );
        if( book_fun != 0 ) {
            // Fun bonus is no longer calculated here.
            learner->add_morale( morale_book,
                                 book_fun * 5, book_fun * 15,
                                 1_hours, 30_minutes, true,
                                 book->type );
        }

        book->mark_chapter_as_read( *learner );
        if( reading_for_skill ) {
            if( !learner->is_avatar() ) {
                const time_duration read_time = you.time_to_read( *book, *reader, learner );
                penalty = static_cast<double>( moves_total ) / to_moves<int>( read_time );
            }
        } else {
            continue;   // reading for fun
        }

        if( skill &&
            learner->get_knowledge_level( skill ) < islotbook->level &&
            learner->get_skill_level_object( skill ).can_train() ) {

            SkillLevel &skill_level = learner->get_skill_level_object( skill );
            std::string skill_name = skill.obj().name();
            const int originalSkillLevel = skill_level.knowledgeLevel();

            read_book( *learner, islotbook, skill_level, penalty );
            const int newSkillLevel = skill_level.knowledgeLevel();

            // levels up the skill
            if( newSkillLevel != originalSkillLevel ) {
                get_event_bus().send<event_type::gains_skill_level>(
                    learner->getID(), skill, skill_level.knowledgeLevel() );

                if( learner->is_avatar() ) {
                    add_msg( m_good, _( "Your theoretical knowledge of %s increases to level %d." ), skill.obj().name(),
                             originalSkillLevel + 1 );
                } else {
                    add_msg( m_good, _( "%s increases their %s knowledge." ), learner->disp_name(),
                             skill.obj().name() );
                }

                if( learner->getID().get_value() == learner_id ) {
                    continuous = false;
                }

            } else {
                if( learner->is_avatar() ) {
                    add_msg( m_info, _( "You learn a little about %s!  (%d%%)" ), skill_name,
                             skill_level.knowledgeExperience() );
                } else {
                    little_learned.push_back( learner->disp_name() );
                }
            }

            if( ( skill_level == islotbook->level || !skill_level.can_train() ) ||
                ( learner->has_trait( trait_SCHIZOPHRENIC ) && !learner->has_effect( effect_took_thorazine ) &&
                  one_in( 25 ) ) ) {
                if( learner->is_avatar() ) {
                    add_msg( m_info, _( "You can no longer learn from %s." ), book->type_name() );
                } else {
                    cant_learn.push_back( learner->disp_name() );
                }
            }
        }

        if( learner_left && learner->getID().get_value() == learner_id ) {
            learner_left = false;
        }
    }   // end for all learners

    if( little_learned.size() == 1 ) {
        add_msg( m_info, _( "%s learns a little about %s!" ), little_learned.begin()->c_str(),
                 skill.obj().name() );

    } else if( !little_learned.empty() ) {
        const std::string little_learned_msg = enumerate_as_string( little_learned );
        add_msg( m_info, _( "%s learn a little about %s!" ), little_learned_msg, skill.obj().name() );
    }

    if( !cant_learn.empty() ) {
        const std::string names = enumerate_as_string( cant_learn );
        add_msg( m_info, _( "%s can no longer learn from %s." ), names, book->type_name() );
    }

    // read non-skill books only once
    if( learner_left || !skill ) {
        continuous = false;
    }

    return false;
}

bool read_activity_actor::player_readma( avatar &you )
{
    // NPCs can't learn martial arts from manuals (yet).

    if( !you.has_identified( book->typeId() ) ) {
        you.identify( *book );
        return true;
    }

    // std::map<std::string, use_function>::const_iterator
    auto mart_iter = book->type->use_methods.find( "MA_MANUAL" );
    if( mart_iter == book->type->use_methods.end() ) {
        debugmsg( "ACT_READ MARTIAL ARTS BOOK HAS NO MARTIAL ART" );
        return true;
    }

    const matype_id style_to_learn = martial_art_learned_from( *book->type );
    skill_id skill_used = style_to_learn->primary_skill;

    int difficulty = std::max( 1, style_to_learn->learn_difficulty );
    difficulty = std::max( 1, 20 + difficulty * 2 - you.get_knowledge_level( skill_used ) * 2 );
    add_msg_debug( debugmode::DF_ACT_READ, "Chance to learn one in: %d", difficulty );

    if( one_in( difficulty ) ) {
        // learn martial art
        mart_iter->second.call( &you, *book, you.pos_bub() );
        return true;
    } else if( continuous ) {
        switch( rng( 1, 5 ) ) {
            case 1:
                add_msg( m_info,
                         _( "You train the moves according to the book, but can't get a grasp of the style, so you start from the beginning." ) );
                break;
            case 2:
                add_msg( m_info,
                         _( "This martial art is not easy to grasp.  You start training the moves from the beginning." ) );
                break;
            case 3:
                add_msg( m_info,
                         _( "You decide to read the manual and train even more.  In martial arts, patience leads to mastery." ) );
                break;
            case 4: // intentionally empty
            case 5:
                add_msg( m_info, _( "You try again.  This training will finally pay off." ) );
                break;
        }
    } else {
        add_msg( m_info, _( "You train for a while." ) );
    }
    return false;
}

bool read_activity_actor::npc_read( npc &learner )
{
    map &here = get_map();

    const cata::value_ptr<islot_book> &islotbook = book->type->book;
    const skill_id &skill = islotbook->skill;

    // NPCs don't need to identify the book or learn recipes yet.
    // NPCs don't read to other NPCs yet.
    const bool display_messages = learner.get_fac_id() == faction_your_followers &&
                                  get_player_character().sees( here, learner );

    const int book_fun = learner.book_fun_for( *book, learner );
    if( book_fun != 0 ) {
        // Fun bonus is no longer calculated here.
        learner.add_morale( morale_book,
                            book_fun * 5, book_fun * 15,
                            1_hours, 30_minutes, true,
                            book->type );
    }

    book->mark_chapter_as_read( learner );

    if( skill &&
        learner.get_knowledge_level( skill ) < islotbook->level &&
        learner.get_skill_level_object( skill ).can_train() ) {

        SkillLevel &skill_level = learner.get_skill_level_object( skill );
        std::string skill_name = skill.obj().name();
        const int originalSkillLevel = skill_level.knowledgeLevel();

        read_book( learner, islotbook, skill_level, 1.0 );

        const int newSkillLevel = skill_level.knowledgeLevel();
        if( newSkillLevel != originalSkillLevel ) {
            get_event_bus().send<event_type::gains_skill_level>(
                learner.getID(), skill, skill_level.knowledgeLevel() );

            if( display_messages ) {
                add_msg( m_good, _( "%s increases their %s level." ), learner.disp_name(),
                         skill_name );
            }

            continuous = false;

        } else if( display_messages ) {
            add_msg( m_info, _( "%s learns a little about %s!" ), learner.disp_name(),
                     skill_name );
        }

        if( display_messages &&
            ( ( skill_level == islotbook->level || !skill_level.can_train() ) ||
              ( learner.has_trait( trait_SCHIZOPHRENIC ) && !learner.has_effect( effect_took_thorazine ) &&
                one_in( 25 ) ) ) ) {
            add_msg( m_info, _( "%s can no longer learn from %s." ), learner.disp_name(),
                     book->type_name() );
        }

    } else if( display_messages && skill ) {
        add_msg( m_info, _( "%s can no longer learn from %s." ), learner.disp_name(),
                 book->type_name() );
        continuous = false;
        // read non-skill books only once
    } else if( !skill ) {
        continuous = false;
    }

    return false;
}

void read_activity_actor::finish( player_activity &act, Character &who )
{
    if( cancel_if_book_invalid( act, book, who ) ) {
        return;
    }

    const bool is_mabook = bktype.value() == book_type::martial_art;
    if( who.is_avatar() ) {
        get_event_bus().send<event_type::reads_book>( who.getID(), book->typeId() );

        const bool should_null = is_mabook ?
                                 player_readma( *who.as_avatar() ) : player_read( *who.as_avatar() );

        if( should_null ) {
            act.set_to_null();
            return;
        }
    } else { // who.is_npc()

        // npcs can't read martial arts books yet
        if( is_mabook ) {
            act.set_to_null();
            return;
        }

        if( npc_read( static_cast<npc &>( who ) ) ) {
            act.set_to_null();
            return;
        }
    }

    if( continuous ) {
        time_duration time_taken;

        // caller should check if npc can read first
        if( who.is_npc() ) {
            npc &n = static_cast<npc &>( who );
            time_taken = n.time_to_read( *book, n );

        } else {    // who.is_avatar()

            // check if there can still be a reader before restarting
            // because npcs can move while reading for the player
            std::vector<std::string> fail_messages;
            const Character *reader = who.as_avatar()->get_book_reader( *book, fail_messages );
            if( reader == nullptr ) {
                // We can't read, and neither can our followers
                for( const std::string &reason : fail_messages ) {
                    add_msg( m_bad, reason );
                }
                act.set_to_null();
                return;
            }

            time_taken = who.as_avatar()->time_to_read( *book, *reader );
            add_msg_debug( debugmode::DF_ACT_READ, "reading time = %s",
                           to_string_writable( time_taken ) );
        }

        // restart the activity
        moves_total = to_moves<int>( time_taken );
        act.moves_total = to_moves<int>( time_taken );
        act.moves_left = to_moves<int>( time_taken );
        return;
    } else  {
        who.add_msg_if_player( m_info, _( "You finish reading." ) );
    }

    act.set_to_null();
}

bool read_activity_actor::can_resume_with_internal( const activity_actor &other,
        const Character & ) const
{
    const read_activity_actor &actor = static_cast<const read_activity_actor &>( other );

    // The ereader gets transformed into the _on version
    // which means the item_location of the book changes
    // this check is to prevent a segmentation fault
    if( !is_valid_book( book ) || !is_valid_book( actor.book ) ) {
        return false;
    }

    return continuous == actor.continuous &&
           learner_id == actor.learner_id &&
           book->typeId() == actor.book->typeId();
}

std::string read_activity_actor::get_progress_message( const player_activity & ) const
{
    if( !is_valid_book( book ) ) {
        return std::string();
    }

    Character &you = get_player_character();
    const cata::value_ptr<islot_book> &islotbook = book->type->book;
    const skill_id &skill = islotbook->skill;

    if( skill &&
        you.get_knowledge_level( skill ) < islotbook->level &&
        you.get_skill_level_object( skill ).can_train() &&
        you.has_identified( book->typeId() ) ) {
        const SkillLevel &skill_level = you.get_skill_level_object( skill );
        //~ skill_name current_skill_level -> next_skill_level (% to next level)
        return string_format( pgettext( "reading progress", "%1$s %2$d -> %3$d (%4$d%%)" ),
                              skill.obj().name(),
                              skill_level.knowledgeLevel(),
                              skill_level.knowledgeLevel() + 1,
                              skill_level.knowledgeExperience() );
    }

    return std::string();
}

void read_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    if( is_valid_book( book ) ) {
        jsout.member( "book", book );
    }
    if( ereader ) {
        jsout.member( "ereader", ereader );
    }
    jsout.member( "using_ereader", using_ereader );
    jsout.member( "continuous", continuous );
    jsout.member( "learner_id", learner_id );

    jsout.end_object();
}

std::unique_ptr<activity_actor> read_activity_actor::deserialize( JsonValue &jsin )
{
    read_activity_actor actor;
    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "book", actor.book );
    data.read( "ereader", actor.ereader );
    data.read( "using_ereader", actor.using_ereader );
    data.read( "continuous", actor.continuous );
    data.read( "learner_id", actor.learner_id );

    return actor.clone();
}

void move_items_activity_actor::do_turn( player_activity &act, Character &who )
{
    map &here = get_map();

    const tripoint_bub_ms dest = relative_destination + who.pos_bub();

    while( who.get_moves() > 0 && !target_items.empty() ) {
        item_location target = std::move( target_items.back() );
        const int quantity = quantities.back();
        target_items.pop_back();
        quantities.pop_back();

        if( !target ) {
            debugmsg( "Lost target item of ACT_MOVE_ITEMS" );
            continue;
        }

        // Check that we can pick it up.
        if( !target->made_of_from_type( phase_id::SOLID ) ) {
            continue;
        }
        // Spill out any invalid contents first
        target.overflow( here );

        item &leftovers = *target;
        // Make a copy to be put in the destination location
        item newit = leftovers;

        if( newit.is_owned_by( who, true ) ) {
            newit.set_owner( who );
        } else {
            continue;
        }

        // Handle charges, quantity == 0 means move all
        if( quantity != 0 && newit.count_by_charges() ) {
            newit.charges = std::min( newit.charges, quantity );
            leftovers.charges -= quantity;
        } else {
            leftovers.charges = 0;
        }

        // This is for hauling across zlevels, remove when going up and down stairs
        // is no longer teleportation
        const tripoint_bub_ms src = target.pos_bub( here );
        const int distance = src.z() == dest.z() ? std::max( rl_dist( src, dest ), 1 ) : 1;
        // Yuck, I'm sticking weariness scaling based on activity level here
        const float weary_mult = who.exertion_adjusted_move_multiplier( exertion_level() );
        who.mod_moves( -Pickup::cost_to_move_item( who, newit ) * distance / weary_mult );
        if( to_vehicle ) {
            put_into_vehicle_or_drop( who, item_drop_reason::deliberate, { newit }, &here, dest );
        } else {
            std::vector<item_location> dropped_items = drop_on_map( who, item_drop_reason::deliberate, { newit },
                    &here, dest );
            if( hauling_mode ) {
                who.haul_list.insert( who.haul_list.end(), dropped_items.begin(), dropped_items.end() );
            }
        }
        // If we picked up a whole stack, remove the leftover item
        if( leftovers.charges <= 0 ) {
            target.remove_item();
        }
    }

    if( target_items.empty() ) {
        // Nuke the current activity, leaving the backlog alone.
        act.set_to_null();
        if( hauling_mode ) {
            std::vector<item_location> haulable_items = here.get_haulable_items( who.pos_bub() );
            bool overflow = who.trim_haul_list( haulable_items );
            if( who.is_hauling() && haulable_items.empty() ) {
                who.stop_hauling();
            }

            if( overflow ) {
                add_msg( m_warning,
                         _( "You lose track of some hauled items as they didn't fit on the current tile." ) );
            }
        }
    }
}

void move_items_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "target_items", target_items );
    jsout.member( "quantities", quantities );
    jsout.member( "to_vehicle", to_vehicle );
    jsout.member( "relative_destination", relative_destination );
    jsout.member( "hauling_mode", hauling_mode );

    jsout.end_object();
}

std::unique_ptr<activity_actor> move_items_activity_actor::deserialize( JsonValue &jsin )
{
    move_items_activity_actor actor( {}, {}, false, tripoint_rel_ms::zero );

    JsonObject data = jsin.get_object();

    data.read( "target_items", actor.target_items );
    data.read( "quantities", actor.quantities );
    data.read( "to_vehicle", actor.to_vehicle );
    data.read( "relative_destination", actor.relative_destination );
    data.read( "hauling_mode", actor.hauling_mode );

    return actor.clone();
}

static void cancel_pickup( Character &who )
{
    who.cancel_activity();
    if( who.is_hauling() && !get_map().has_haulable_items( who.pos_bub() ) ) {
        who.stop_hauling();
    }
    who.drop_invalid_inventory();
}

void pickup_activity_actor::do_turn( player_activity &, Character &who )
{
    // If we don't have target items bail out
    if( target_items.empty() ) {
        cancel_pickup( who );
        return;
    }

    // If the player moves while picking up (i.e.: in a moving vehicle) cancel
    // the activity, only populate starting_pos when grabbing from the ground
    if( starting_pos && *starting_pos != who.pos_bub() ) {
        who.add_msg_if_player( _( "Moving canceled auto-pickup." ) );
        cancel_pickup( who );
        return;
    }

    // False indicates that the player canceled pickup when met with some prompt
    const bool keep_going = Pickup::do_pickup( target_items, quantities, autopickup,
                            stash_successful, info );

    // If there are items left we ran out of moves, so continue the activity
    // Otherwise, we are done.
    if( !keep_going || target_items.empty() ) {
        if( !stash_successful && !autopickup ) {
            add_msg( m_bad, _( "Some items were not picked up!" ) );
        }

        cancel_pickup( who );

        if( who.get_value( "THIEF_MODE_KEEP" ).str() != "YES" ) {
            who.set_value( "THIEF_MODE", "THIEF_ASK" );
        }

        if( !keep_going ) {
            // The user canceled the activity, so we're done
            // AIM might have more pickup activities pending, also cancel them.
            // TODO: Move this to advanced inventory instead of hacking it in here
            cancel_aim_processing();
        }
    }
}

void pickup_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "target_items", target_items );
    jsout.member( "quantities", quantities );
    jsout.member( "starting_pos", starting_pos );
    jsout.member( "stash_successful", stash_successful );
    jsout.member( "autopickup", autopickup );
    jsout.member( "info", info );

    jsout.end_object();
}

std::unique_ptr<activity_actor> pickup_activity_actor::deserialize( JsonValue &jsin )
{
    pickup_activity_actor actor( {}, {}, std::nullopt, false );

    JsonObject data = jsin.get_object();

    data.read( "target_items", actor.target_items );
    data.read( "quantities", actor.quantities );
    data.read( "starting_pos", actor.starting_pos );
    data.read( "stash_successful", actor.stash_successful );
    data.read( "autopickup", actor.autopickup );
    data.read( "info", actor.info );

    return actor.clone();
}

void boltcutting_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    const map &here = get_map();

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->boltcut->valid() ) {
            if( !testing ) {
                debugmsg( "%s boltcut is invalid", furn_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        act.moves_total = to_moves<int>( furn_type->boltcut->duration() );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->boltcut->valid() ) {
            if( !testing ) {
                debugmsg( "%s boltcut is invalid", ter_type.id().str() );
            }
            act.set_to_null();
            return;
        }
        act.moves_total = to_moves<int>( ter_type->boltcut->duration() );
    } else {
        if( !testing ) {
            debugmsg( "boltcut activity called on invalid terrain" );
        }
        act.set_to_null();
        return;
    }

    add_msg_debug( debugmode::DF_ACTIVITY, "%s moves_total: %d", act.id().str(), act.moves_total );
    act.moves_left = act.moves_total;
}

void boltcutting_activity_actor::do_turn( player_activity &/*act*/, Character &who )
{
    map &here = get_map();

    if( tool->ammo_sufficient( &who ) ) {
        tool->ammo_consume( tool->ammo_required(), here, tool.pos_bub( here ), &who );
    } else {
        if( who.is_avatar() ) {
            who.add_msg_if_player( m_bad, _( "Your %1$s ran out of charges." ), tool->tname() );
        } else { // who.is_npc()
            add_msg_if_player_sees( who.pos_bub(), _( "%1$s %2$s ran out of charges." ), who.disp_name( false,
                                    true ), tool->tname() );
        }
        who.cancel_activity();
    }
}

void boltcutting_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    const activity_data_common *data;

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->boltcut->valid() ) {
            if( !testing ) {
                debugmsg( "%s boltcut is invalid", furn_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        const furn_str_id new_furn = furn_type->boltcut->result();
        if( !new_furn.is_valid() ) {
            if( !testing ) {
                debugmsg( "boltcut furniture: %s invalid furniture", new_furn.str() );
            }
            act.set_to_null();
            return;
        }

        data = static_cast<const activity_data_common *>( &*furn_type->boltcut );
        here.furn_set( target, new_furn );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->boltcut->valid() ) {
            if( !testing ) {
                debugmsg( "%s boltcut is invalid", ter_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        const ter_str_id new_ter = ter_type->boltcut->result();
        if( !new_ter.is_valid() ) {
            if( !testing ) {
                debugmsg( "boltcut terrain: %s invalid terrain", new_ter.str() );
            }
            act.set_to_null();
            return;
        }

        data = static_cast<const activity_data_common *>( &*ter_type->boltcut );
        here.ter_set( target, new_ter );
    } else {
        if( !testing ) {
            debugmsg( "boltcut activity finished on invalid terrain" );
        }
        act.set_to_null();
        return;
    }

    if( data->sound().empty() ) {
        sounds::sound( target, 5, sounds::sound_t::combat, _( "Snick, snick, gachunk!" ),
                       true, "tool", "boltcutters" );
    } else {
        sounds::sound( target, 5, sounds::sound_t::combat, data->sound().translated(),
                       true, "tool", "boltcutters" );
    }

    for( const activity_byproduct &byproduct : data->byproducts() ) {
        const int amount = byproduct.roll();
        if( byproduct.item->count_by_charges() ) {
            item byproduct_item( byproduct.item, calendar::turn, amount );
            here.add_item_or_charges( target, byproduct_item );
        } else {
            item byproduct_item( byproduct.item, calendar::turn );
            for( int i = 0; i < amount; ++i ) {
                here.add_item_or_charges( target, byproduct_item );
            }
        }
    }

    if( !data->message().empty() ) {
        who.add_msg_if_player( m_info, data->message().translated() );
    }

    act.set_to_null();
}

void boltcutting_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "target", target );
    jsout.member( "tool", tool );
    jsout.end_object();
}

std::unique_ptr<activity_actor> boltcutting_activity_actor::deserialize( JsonValue &jsin )
{
    boltcutting_activity_actor actor( {}, {} );
    JsonObject data = jsin.get_object();
    data.read( "target", actor.target );
    data.read( "tool", actor.tool );
    return actor.clone();
}

lockpick_activity_actor lockpick_activity_actor::use_item(
    int moves_total,
    const item_location &lockpick,
    const tripoint_abs_ms &target
)
{
    return lockpick_activity_actor {
        moves_total,
        lockpick,
        std::nullopt,
        target
    };
}

lockpick_activity_actor lockpick_activity_actor::use_bionic(
    const tripoint_abs_ms &target
)
{
    return lockpick_activity_actor {
        to_moves<int>( 4_seconds ),
        std::nullopt,
        item( itype_pseudo_bio_picklock ),
        target
    };
}

void lockpick_activity_actor::start( player_activity &act, Character & )
{
    act.moves_left = moves_total;
    act.moves_total = moves_total;

    const time_duration lockpicking_time = time_duration::from_moves( moves_total );
    add_msg_debug( debugmode::DF_ACT_LOCKPICK, "lockpicking time = %s",
                   to_string_writable( lockpicking_time ) );
}

void lockpick_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();

    item *it;
    if( lockpick.has_value() ) {
        it = ( *lockpick ).get_item();
    } else {
        it = &*fake_lockpick;
    }

    if( !it ) {
        debugmsg( "Lost ACT_LOCKPICK item" );
        return;
    }

    map &here = get_map();
    const tripoint_bub_ms target = here.get_bub( this->target );
    const ter_id &ter_type = here.ter( target );
    const furn_id &furn_type = here.furn( target );
    optional_vpart_position const veh = here.veh_at( target );
    int locked_part = -1;
    ter_id new_ter_type;
    furn_id new_furn_type;
    std::string open_message = _( "The lock opens…" );

    if( veh ) {
        std::vector<vehicle_part *> parts_at_target = veh->vehicle().get_parts_at(
                    &here, target, "LOCKABLE_DOOR", part_status_flag::available );
        if( !parts_at_target.empty() ) {
            locked_part = veh->vehicle().next_part_to_unlock(
                              veh->vehicle().index_of_part( parts_at_target.front() ) );
        }
    }

    if( here.has_furn( target ) ) {
        if( furn_type->lockpick_result.is_null() ) {
            who.add_msg_if_player( m_bad, _( "You can't open this lock." ) );
            return;
        }

        new_furn_type = furn_type->lockpick_result;
        if( !furn_type->lockpick_message.empty() ) {
            open_message = furn_type->lockpick_message.translated();
        }
    } else if( locked_part >= 0 ) {
        // no-op.
    } else {
        if( ter_type->lockpick_result.is_null() ) {
            who.add_msg_if_player( m_bad, _( "You can't open this lock." ) );
            return;
        }

        new_ter_type = ter_type->lockpick_result;
        if( !ter_type->lockpick_message.empty() ) {
            open_message = ter_type->lockpick_message.translated();
        }
    }

    bool perfect = it->has_flag( flag_PERFECT_LOCKPICK );
    bool destroy = false;

    // Your devices skill is the primary skill that applies to your roll. Your mechanics skill has a little input.
    const float weighted_skill_average = ( 3.0f * who.get_skill_level(
            skill_traps ) + who.get_skill_level( skill_mechanics ) ) / 4.0f;

    // Your dexterity determines most of your stat contribution, but your intelligence and perception combined are about half as much.
    const float weighted_stat_average = ( 6.0f * who.dex_cur + 2.0f * who.per_cur +
                                          who.int_cur ) / 9.0f;

    // Get a bonus from your lockpick quality if the quality is higher than 3, or a penalty if it is lower. For a bobby pin this puts you at -2, for a locksmith kit, +2.
    const float tool_effect = ( it->get_quality( qual_LOCKPICK ) - 3 ) - ( it->damage() / 2000.0 );

    // Without at least a basic lockpick proficiency, your skill level is effectively 6 levels lower.
    int proficiency_effect = -3;
    int duration_proficiency_factor = 10;
    if( who.has_proficiency( proficiency_prof_lockpicking ) ) {
        // If you have the basic lockpick prof, negate the above penalty
        proficiency_effect = 0;
        duration_proficiency_factor = 5;
    }
    if( who.has_proficiency( proficiency_prof_lockpicking_expert ) ) {
        // If you have the locksmith proficiency, your skill level is effectively 4 levels higher.
        proficiency_effect = 3;
        duration_proficiency_factor = 1;
    }

    // We get our average roll by adding the above factors together. For a person with no skill, average stats, no proficiencies, and an improvised lockpick, mean_roll will be 2.
    const float mean_roll = weighted_skill_average + ( weighted_stat_average / 4 ) + proficiency_effect
                            + tool_effect;

    // A standard deviation of 2 means that about 2/3 of rolls will come within 2 points of your mean_roll. Lockpicking
    int pick_roll = std::round( normal_roll( mean_roll, 2 ) );

    // Lock_roll should be replaced with a flat value defined by the door, soon.
    // In the meantime, let's roll 3d5-3, giving us a range of 0-12.
    int lock_roll = rng( 0, 4 ) + rng( 0, 4 ) + rng( 0, 4 );

    add_msg_debug( debugmode::DF_ACT_LOCKPICK, "Rolled %i. Mean_roll %g. Difficulty %i.",
                   pick_roll, mean_roll, lock_roll );

    // Your base skill XP gain is derived from the lock difficulty (which is currently random but shouldn't be).
    int xp_gain = 3 * lock_roll;
    if( perfect || ( pick_roll >= lock_roll ) ) {
        if( !perfect ) {
            // Increase your XP if you successfully pick the lock, unless you were using a Perfect Lockpick.
            xp_gain = xp_gain * 2;
        }
        if( here.has_furn( target ) ) {
            here.furn_set( target, new_furn_type );
        } else if( locked_part >= 0 ) {
            veh->vehicle().unlock( locked_part );
        } else {
            here.ter_set( target, new_ter_type );
        }
        who.add_msg_if_player( m_good, open_message );
    } else if( furn_type == furn_f_gunsafe_ml && lock_roll > ( 3 * pick_roll ) ) {
        who.add_msg_if_player( m_bad, _( "Your clumsy attempt jams the lock!" ) );
        here.furn_set( target, furn_f_gunsafe_mj );
    } else if( lock_roll > ( 1.5 * pick_roll ) ) {
        if( it->inc_damage() ) {
            who.add_msg_if_player( m_bad,
                                   _( "The lock stumps your efforts to pick it, and you destroy your tool." ) );
            destroy = true;
        } else {
            who.add_msg_if_player( m_bad,
                                   _( "The lock stumps your efforts to pick it, and you damage your tool." ) );
        }
    } else {
        who.add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it." ) );
    }

    if( avatar *you = dynamic_cast<avatar *>( &who ) ) {
        // Gives another boost to XP, reduced by your skill level.
        // Higher skill levels require more difficult locks to gain a meaningful amount of xp.
        // Again, we're using randomized lock_roll until a defined lock difficulty is implemented.
        if( lock_roll > round( you->get_skill_level( skill_traps ) ) ) {
            xp_gain += lock_roll + ( xp_gain / ( you->get_skill_level( skill_traps ) + 1 ) );
        } else {
            xp_gain += xp_gain / ( you->get_skill_level( skill_traps ) + 1 );
        }
        you->practice( skill_traps, xp_gain );
    }

    if( !perfect && ter_type == ter_t_door_locked_alarm && ( lock_roll + dice( 1, 30 ) ) > pick_roll ) {
        sounds::sound( who.pos_bub(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), true,
                       "environment",
                       "alarm" );
    }

    if( destroy && lockpick.has_value() ) {
        ( *lockpick ).remove_item();
    }

    who.practice_proficiency( proficiency_prof_lockpicking,
                              time_duration::from_moves( act.moves_total ) / duration_proficiency_factor );
    who.practice_proficiency( proficiency_prof_lockpicking_expert,
                              time_duration::from_moves( act.moves_total ) / duration_proficiency_factor );
}

std::optional<tripoint_bub_ms> lockpick_activity_actor::select_location( avatar &you )
{
    map &here = get_map();

    if( you.cant_do_mounted() ) {
        return std::nullopt;
    }

    const std::function<bool( const tripoint_bub_ms & )> is_pickable = [&you, &here](
    const tripoint_bub_ms & p ) {
        const optional_vpart_position vpart = here.veh_at( p );
        if( p == you.pos_bub() ) {
            return false;
        }
        return here.has_flag( ter_furn_flag::TFLAG_PICKABLE, p ) ||
               ( vpart && vpart->vehicle().next_part_to_unlock( vpart->part_index() ) >= 0 );
    };

    std::optional<tripoint_bub_ms> target = choose_adjacent_highlight(
            here, _( "Use your lockpick where?" ), _( "There is nothing to lockpick nearby." ), is_pickable,
            false );
    if( !target ) {
        return std::nullopt;
    }

    if( is_pickable( *target ) ) {
        return target;
    }

    const ter_id &terr_type = here.ter( *target );
    if( *target == you.pos_bub() ) {
        you.add_msg_if_player( m_info, _( "You pick your nose and your sinuses swing open." ) );
    } else if( get_creature_tracker().creature_at<npc>( *target ) ) {
        you.add_msg_if_player( m_info,
                               _( "You can pick your friends, and you can\npick your nose, but you can't pick\nyour friend's nose." ) );
    } else if( terr_type == ter_t_door_c ) {
        you.add_msg_if_player( m_info, _( "That door isn't locked." ) );
    } else {
        you.add_msg_if_player( m_info, _( "That cannot be picked." ) );
    }
    return std::nullopt;
}

void lockpick_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "lockpick", lockpick );
    jsout.member( "fake_lockpick", fake_lockpick );
    jsout.member( "target", target );

    jsout.end_object();
}

std::unique_ptr<activity_actor> lockpick_activity_actor::deserialize( JsonValue &jsin )
{
    lockpick_activity_actor actor( 0, std::nullopt, std::nullopt, tripoint_abs_ms::zero );

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "lockpick", actor.lockpick );
    data.read( "fake_lockpick", actor.fake_lockpick );
    data.read( "target", actor.target );

    return actor.clone();
}

int ebooksave_activity_actor::total_pages(
    const std::vector<item_location> &books )
{
    int pages = 0;
    for( const item_location &book : books ) {
        pages += item::pages_in_book( *book->type );
    }
    return pages;
}

time_duration ebooksave_activity_actor::required_time(
    const std::vector<item_location> &books )
{
    return total_pages( books ) * time_per_page;
}

int ebooksave_activity_actor::required_charges(
    const std::vector<item_location> &books,
    const item_location &ereader )
{
    const int charges = total_pages( books ) / pages_per_charge;
    return charges * ereader->ammo_required();
}

void ebooksave_activity_actor::start_scanning_next_book( player_activity &act )
{
    while( !books.empty() && !books.back() ) {
        add_msg_debug( debugmode::DF_ACT_EBOOK, "Skipping unavailable book at %s",
                       books.back().describe() );
        books.pop_back();
    }
    if( books.empty() ) {
        act.moves_left = 0;
        return;
    }
    const item_location &current_book = books.back();
    const int pages_in_current_book = item::pages_in_book( *current_book->type );
    turns_left_on_current_book = to_turns<int>( pages_in_current_book * time_per_page );
    act.moves_left = to_moves<int>( required_time( books ) );
}

void ebooksave_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    const time_duration scanning_time = required_time( books );
    add_msg_debug( debugmode::DF_ACT_EBOOK, "scanning_time time = %s",
                   to_string_writable( scanning_time ) );
    act.moves_total = to_moves<int>( scanning_time );
    start_scanning_next_book( act );
}

void ebooksave_activity_actor::do_turn( player_activity &act, Character &who )
{
    // only consume charges every pages_per_charge pages
    if( calendar::once_every( pages_per_charge * time_per_page ) ) {
        if( !ereader->ammo_sufficient( &who ) ) {
            add_msg_if_player_sees(
                who,
                _( "%1$s %2$s ran out of batteries." ),
                who.disp_name( true, true ),
                item::nname( ereader->typeId() ) );
            who.cancel_activity();
            return;
        }

        ereader->ammo_consume( ereader->ammo_required(), who.pos_bub(), &who );
    }

    turns_left_on_current_book--;
    if( turns_left_on_current_book > 0 ) {
        return; // not fully scanned yet
    }

    // We have now spent enough time to fully scan current book
    completed_scanning_current_book( act, who );
}

void ebooksave_activity_actor::completed_scanning_current_book( player_activity &act,
        Character &who )
{
    item_location scanned_book = books.back();
    books.pop_back();
    if( scanned_book ) {
        ereader->put_in( *scanned_book, pocket_type::E_FILE_STORAGE );
        if( who.is_avatar() ) {
            if( scanned_book->is_identifiable() && !who.has_identified( scanned_book->typeId() ) ) {
                who.identify( *scanned_book );
            }
        }
        handled_books++;
    } else {
        add_msg_debug( debugmode::DF_ACT_EBOOK,
                       "Spent time to scan book at %s, but it is no longer available?",
                       scanned_book.describe() );
    }
    start_scanning_next_book( act );
}

void ebooksave_activity_actor::finish( player_activity &act, Character &who )
{
    while( !books.empty() ) {
        // We can end up here with books left if the player has speed>100
        completed_scanning_current_book( act, who );
    }
    if( who.is_avatar() ) {
        add_msg( m_info, _( "You scan %d %s into your device." ), handled_books,
                 n_gettext( "book", "books", handled_books ) );
    } else { // who.is_npc()
        add_msg_if_player_sees( who, _( "%s scans %d %s into their device." ),
                                who.disp_name( false, true ), handled_books, n_gettext( "book", "books", handled_books ) );
    }
    act.set_to_null();
}

void ebooksave_activity_actor::canceled( player_activity &act, Character &who )
{
    if( who.is_avatar() ) {
        add_msg( m_info, _( "You stop scanning the remaining books." ) );
    } else { // who.is_npc()
        add_msg_if_player_sees( who, _( "%s stops scanning books." ),
                                who.disp_name( false, true ) );
    }
    act.set_to_null();
}

void ebooksave_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "books", books );
    jsout.member( "ereader", ereader );
    jsout.member( "handled_books", handled_books );
    jsout.member( "turns_left_on_current_book", turns_left_on_current_book );
    jsout.end_object();
}

std::unique_ptr<activity_actor> ebooksave_activity_actor::deserialize( JsonValue &jsin )
{
    ebooksave_activity_actor actor = ebooksave_activity_actor( {}, {} );

    JsonObject data = jsin.get_object();
    data.read( "books", actor.books );
    if( data.has_member( "book" ) ) {
        // legacy compatibility: in 0.G, this activity only handled one book each
        item_location book;
        data.read( "book", book );
        actor.books.push_back( book );
    }
    data.read( "ereader", actor.ereader );
    data.read( "handled_books", actor.handled_books );
    data.read( "turns_left_on_current_book", actor.turns_left_on_current_book );
    return actor.clone();
}

namespace io
{
template<>
std::string enum_to_string<efile_action>( efile_action data )
{
    return efile_activity_actor::efile_action_name( data );
}
template<>
std::string enum_to_string<efile_combo>( efile_combo data )
{
    switch( data ) {
            // *INDENT-OFF*
        case efile_combo::COMBO_MOVE_ONTO_BROWSE: return "COMBO_MOVE_ONTO_BROWSE";
        case efile_combo::COMBO_NONE: return "COMBO_NONE";
            // *INDENT-ON*
        default:
            cata_fatal( "Invalid based_on_type in enum_to_string" );
    }
}
} // namespace io

bool efile_activity_actor::processed_edevices_remain() const
{
    return target_edevices.empty();
}

bool  efile_activity_actor::processed_efiles_remain() const
{
    return currently_processed_efiles.empty();
}

item_location &efile_activity_actor::get_currently_processed_edevice()
{
    return target_edevices.back();
}

item_location &efile_activity_actor::get_currently_processed_efile()
{
    return currently_processed_efiles.back();
}
void efile_activity_actor::start( player_activity &act, Character &who )
{
    if( combo_type == COMBO_MOVE_ONTO_BROWSE ) {
        target_edevices_copy = target_edevices;
    }
    //handle combo move e-device (browsing may have included used e-device)
    if( action_type == EF_MOVE_ONTO_THIS ) {
        auto i = target_edevices.begin();
        while( i != target_edevices.end() ) {
            if( *i == used_edevice ) {
                i = target_edevices.erase( i );
                break;
            } else {
                i++;
            }
        }
    }
    for( item_location &i : target_edevices ) {
        if( !i->has_pocket_type( pocket_type::E_FILE_STORAGE ) ) {
            debugmsg( "invalid item %s provided to efile activity; must have \"E_FILE_STORAGE\" pocket",
                      i->tname() );
        }
        add_msg_debug( debugmode::DF_ACT_EBOOK, "initialized with edevice %s with %d efiles",
                       i->display_name(), i->efiles().size() );
    }
    //only skip if loaded through deserialization
    if( !started_processing ) {
        started_processing = true;
        computer_low_skill = who.get_skill_level( skill_computer ) < 1;
    }

    if( !processed_edevices_remain() ) {
        const time_duration total_time = total_processing_time( used_edevice, target_edevices,
                                         selected_efiles, action_type, who, computer_low_skill );
        act.moves_total = to_moves<int>( total_time );
        target_edevices_count = target_edevices.size();
        add_msg_debug( debugmode::DF_ACT_EBOOK, "total processing moves: %d",
                       act.moves_total );
    } else {
        act.moves_total = 0;
        done_processing = true;
        add_msg_debug( debugmode::DF_ACT_EBOOK, "no e-devices found to process!" );
    }
    act.moves_left = act.moves_total;
}

void efile_activity_actor::do_turn( player_activity &act, Character &who )
{
    map &here = get_map();

    auto edevice_reduce_charge = [&]( item_location & edevice ) {
        if( calendar::once_every( charge_time( action_type ) ) ) {
            if( !edevice->ammo_sufficient( &who ) ) {
                add_msg_if_player_sees(
                    who, m_warning, string_format( _( "The %s ran out of batteries!" ),
                                                   edevice->display_name() ) );
                return false;
            }
            edevice->ammo_consume( edevice->ammo_required(), here, edevice.pos_bub( here ), &who );
            add_msg_debug( debugmode::DF_ACT_EBOOK, "%s power check", edevice->display_name() );
        }
        return true;
    };

    //check for zero devices selected, for combo call
    if( act.moves_left > 0 ) {
        //if an e-device was booted, make sure it still exists
        if( !!turns_left_on_current_edevice ) {
            if( !used_edevice || !edevice_reduce_charge( used_edevice ) ) {
                //if the used device runs out of power or is missing, fail all remaining devices
                do {
                    failed_processing_current_edevice();
                } while( !done_processing );
            } else {
                item_location next_edevice = get_currently_processed_edevice();
                if( !next_edevice || !edevice_reduce_charge( next_edevice ) ) {
                    failed_processing_current_edevice();
                }
            }
        }
    }
    //done check (handles return)
    if( done_processing ) {
        act.moves_left = 0;
        add_msg_debug( debugmode::DF_ACT_EBOOK, "efile_transfer completed through done_processing" );
        return;
    }
    //computer practice
    if( one_in( 3 ) && computer_low_skill ) {
        if( who.practice( skill_computer, 1 ) ) {
            computer_low_skill = false;
        }
    }

    if( !next_edevice_booted ) {
        if( !turns_left_on_current_edevice ) {
            start_processing_next_edevice(); //only sets if device exists
        }
        ( *turns_left_on_current_edevice )--;
        if( turns_left_on_current_edevice == 0 ) {
            next_edevice_booted = true;
            start_processing_next_efile( act, who ); //sets turns_left_file if file exists
        }
    }
    if( next_edevice_booted ) { //should not be an "else" because files start processing in same turn
        //current file exists check
        if( !get_currently_processed_efile() ) {
            failed_processing_current_efile( act, who );
        } else if( turns_left_on_current_efile > 0 ) {
            turns_left_on_current_efile--;
        }
        if( turns_left_on_current_efile == 0 ) {
            completed_processing_current_efile( act, who );
            start_processing_next_efile( act, who );
        }

    }
}

void efile_activity_actor::start_processing_next_edevice()
{
    item_location &current_edevice = get_currently_processed_edevice();
    const time_duration process_time = device_bootup_time;
    bool ignore_filter = efile_action_is_from( action_type );
    if( current_edevice ) {
        turns_left_on_current_edevice = to_turns<int>( process_time );
        item_location &edevice_filter = ignore_filter ? used_edevice : current_edevice;
        currently_processed_efiles = ignore_filter ? selected_efiles :
                                     filter_edevice_efiles( edevice_filter, selected_efiles );
    }
    add_msg_debug( debugmode::DF_ACT_EBOOK, string_format( "started processing edevice %s",
                   current_edevice->display_name() ) );
}

void efile_activity_actor::start_processing_next_efile( player_activity &/*act*/, Character &who )
{
    if( !processed_efiles_remain() ) {
        //separate for easy debugging
        item_location &current_efile = get_currently_processed_efile();
        item_location &current_edevice = get_currently_processed_edevice();
        efile_transfer transfer( used_edevice, current_edevice );
        time_duration transfer_time = efile_processing_time( current_efile, transfer, action_type, who,
                                      computer_low_skill );
        turns_left_on_current_efile = to_turns<int>( transfer_time );
        add_msg_debug( debugmode::DF_ACT_EBOOK, string_format( "started processing efile %s",
                       current_efile->display_name() ) );
    } else {
        completed_processing_current_edevice();
    }
}

void efile_activity_actor::completed_processing_current_edevice()
{
    if( action_type == EF_BROWSE ) {
        get_currently_processed_edevice()->set_browsed( true );
    }
    processed_edevices++;
    add_msg_debug( debugmode::DF_ACT_EBOOK, string_format( "completed processing edevice %s",
                   get_currently_processed_edevice()->display_name() ) );

    next_edevice_booted = false;
    turns_left_on_current_edevice.reset();
    target_edevices.pop_back();
    if( processed_edevices_remain() ) {
        done_processing = true;
    }
}

void efile_activity_actor::completed_processing_current_efile( player_activity &/*act*/,
        Character &who )
{
    item_location &current_edevice = get_currently_processed_edevice();
    item_location current_efile = get_currently_processed_efile();
    efile_transfer current_transfer( used_edevice, current_edevice );

    auto remove_efile = []( item_location & edevice, const item & efile_remove ) {
        edevice->get_contents().remove_items_if( [&efile_remove]( item & efile ) {
            return &efile == &efile_remove;
        } );
    };
    auto add_efile = [&current_transfer]( item_location & edevice,
    item & efile, bool copy ) {
        if( !efile_skip_copy( current_transfer, efile ) ) {
            item &added_efile = efile;
            if( copy ) {
                const item &new_efile = efile;
                added_efile = new_efile;
            }
            //instead of moving the recipe e-file, instead try to combine it with an existing one
            if( added_efile.typeId()->memory_card_data ) {
                item *edevice_recipe_catalog = edevice->get_recipe_catalog();
                if( edevice_recipe_catalog != nullptr ) {
                    std::set<recipe_id> merged_catalogs = edevice_recipe_catalog->get_saved_recipes();
                    for( const auto &recipe : added_efile.get_saved_recipes() ) {
                        merged_catalogs.emplace( recipe );
                    }
                    edevice_recipe_catalog->set_saved_recipes( merged_catalogs );
                    return;
                }
            } else if( added_efile.typeId() == itype_efile_photos ) {
                const std::function<bool( const item &i )> filter = []( const item & i ) {
                    return i.typeId() == itype_efile_photos;
                };
                item *edevice_photos = edevice->get_contents().get_item_with( filter );
                if( edevice_photos != nullptr ) {
                    std::vector<item::extended_photo_def> extended_photos;
                    edevice_photos->read_extended_photos( extended_photos, "CAMERA_EXTENDED_PHOTOS", false );
                    added_efile.read_extended_photos( extended_photos, "CAMERA_EXTENDED_PHOTOS", true );
                    edevice_photos->write_extended_photos( extended_photos, "CAMERA_EXTENDED_PHOTOS" );
                    return;
                }

            }
            edevice->get_contents().insert_item( added_efile, pocket_type::E_FILE_STORAGE );
        }
    };

    add_msg_if_player_sees( who, m_info, string_format( _( "%s %s %s." ),
                            who.disp_name( false, true ), efile_action_name( action_type, true, false ),
                            current_efile->display_name() ) );
    switch( action_type ) {
        case EF_BROWSE:
            if( current_efile->typeId() == itype_efile_junk ) {
                remove_efile( current_edevice, *current_efile );
                break;
            }
            if( current_efile->get_recipe_catalog() != nullptr ) {
                if( current_efile->get_saved_recipes().empty() ) {
                    current_efile->generate_recipes();
                }
            }
            current_efile->set_browsed( true );
            break;
        case EF_MOVE_ONTO_THIS:
            add_efile( used_edevice, *current_efile, false );
            remove_efile( current_edevice, *current_efile );
            break;
        case EF_MOVE_FROM_THIS:
            add_efile( current_edevice, *current_efile, false );
            remove_efile( used_edevice, *current_efile );
            break;
        case EF_COPY_ONTO_THIS:
            add_efile( used_edevice, *current_efile, true );
            break;
        case EF_COPY_FROM_THIS:
            add_efile( current_edevice, *current_efile, true );
            break;
        case EF_WIPE:
            remove_efile( current_edevice, *current_efile );
            break;
        default:
            break;
    }
    processed_efiles++;
    currently_processed_efiles.pop_back();
}

void efile_activity_actor::failed_processing_current_edevice()
{
    failed_edevices++;

    turns_left_on_current_edevice.reset();
    next_edevice_booted = false;
    target_edevices.pop_back();
    if( processed_edevices_remain() ) {
        done_processing = true;
    }
}

void efile_activity_actor::failed_processing_current_efile( player_activity &act,
        Character &who )
{
    turns_left_on_current_edevice.reset();
    start_processing_next_efile( act, who );
}

void efile_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    std::string action_name = efile_action_name( action_type, true, true );
    int total_edevices = target_edevices_count;
    if( total_edevices == 0 && processed_edevices == 0 ) {
        add_msg_if_player_sees( who, m_info, _( "No devices needed to be processed." ) );
    } else if( processed_edevices == 0 ) {
        add_msg_if_player_sees( who, m_warning, _( "You failed to %s %d device(s)." ),
                                efile_action_name( action_type, false, true ), total_edevices );
    } else {
        add_msg_if_player_sees( who, m_good, _( "You successfully %s %d/%d device(s)." ),
                                action_name, processed_edevices, total_edevices );
    }
    combo_next_activity( who );
}

std::vector<item_location> efile_activity_actor::filter_edevice_efiles(
    const item_location &edevice, std::vector<item_location> &filter_files )
{
    std::vector<item_location> filtered_efiles;
    std::vector<item_location>::iterator it = filter_files.begin();
    while( it != filter_files.end() ) {
        if( it->parent_item() == edevice ) {
            filtered_efiles.emplace_back( *it );
            it = filter_files.erase( it );
        } else {
            ++it;
        }
    }
    return filtered_efiles;
}

item_location efile_activity_actor::find_external_transfer_estorage( Character &p,
        const item_location &efile, const item_location &ed1, const item_location &ed2 )
{
    item_location fastest_edevice;
    if( edevices_compatible( ed1, ed2 ) == ECOMPAT_FAST ) {
        return fastest_edevice;
    }
    units::ememory largest_efile_size = efile->ememory_size();
    //search for fastest, large-enough, browsed, non-tool, estorage device in radius or on person
    units::ememory fastest_rate = 0_KB;
    const std::function<bool( const item *it, const item * )> func = [&]( const item * it,
    const item * ) {
        return it->is_browsed() &&
               !edevice_has_use( it ) && //is not a usable e-device (e.g. a USB drive)
               it->is_estorage() &&
               it->remaining_ememory() >= largest_efile_size &&
               it->is_tool();
    };
    std::vector<item_location> nearby_etds = p.nearby( func, PICKUP_RANGE );
    for( item_location &loc : nearby_etds ) {
        if( loc->type->tool->etransfer_rate > fastest_rate ) {
            fastest_edevice = loc;
        }
    }
    return fastest_edevice;
}

bool efile_activity_actor::edevice_has_use( const item *edevice )
{
    return edevice->type->can_use( "E_FILE_DEVICE" );
}

efile_activity_actor::edevice_compatible efile_activity_actor::edevices_compatible(
    const item_location &ed1, const item_location &ed2 )
{
    return edevices_compatible( *ed1.get_item(), *ed2.get_item() );
}

efile_activity_actor::edevice_compatible efile_activity_actor::edevices_compatible(
    const item &ed1, const item &ed2 )
{
    if( !ed1.is_tool() || !ed2.is_tool() ) {
        return efile_activity_actor::ECOMPAT_NONE;
    }
    std::string &port1 = ed1.type->tool->e_port;
    std::string &port2 = ed2.type->tool->e_port;
    if( port1 == port2 ) {
        return efile_activity_actor::ECOMPAT_SLOW;
    }
    std::vector<std::string> &banned1 = ed1.type->tool->e_ports_banned;
    std::vector<std::string> &banned2 = ed2.type->tool->e_ports_banned;
    for( std::string &ban : banned1 ) {
        if( ban == port2 ) {
            return efile_activity_actor::ECOMPAT_NONE;
        }
    }
    for( std::string &ban : banned2 ) {
        if( ban == port1 ) {
            return efile_activity_actor::ECOMPAT_NONE;
        }
    }
    return efile_activity_actor::ECOMPAT_FAST;
}

void efile_activity_actor::combo_next_activity( Character &who )
{
    efile_combo new_combo = COMBO_NONE;
    efile_action new_action = EF_INVALID;
    std::vector<item_location> all_updated_files;

    if( combo_type == COMBO_MOVE_ONTO_BROWSE ) {
        for( item_location &edevice : target_edevices_copy ) {
            for( item *efile : edevice->efiles() ) {
                all_updated_files.emplace_back( edevice, efile );
            }
        }

        units::ememory total_ememory;
        for( item_location &edevice : target_edevices_copy ) {
            if( edevice->is_browsed() ) {
                for( item *efile : edevice->efiles() ) {
                    total_ememory += efile->ememory_size();
                }
            }
        }
        if( total_ememory <= used_edevice->remaining_ememory() ) {
            new_action = EF_MOVE_ONTO_THIS;
        } else {
            add_msg_if_player_sees( who, m_warning,
                                    _( "File size exceeds available memory; canceling file move." ) );
        }
    }
    if( new_action != EF_INVALID ) {
        const efile_activity_actor new_activity( used_edevice, target_edevices_copy,
                all_updated_files, new_action, new_combo );
        who.assign_activity( player_activity( new_activity ) );
    }
}

time_duration efile_activity_actor::charge_time( efile_action action_type )
{
    if( action_type == EF_BROWSE ) {
        return charge_per_browse_time;
    }
    return charge_per_transfer_time;
}

void efile_activity_actor::canceled( player_activity &act, Character &who )
{
    if( who.is_avatar() ) {
        add_msg( m_info, _( "You stop processing the remaining devices." ) );
    } else {
        add_msg_if_player_sees( who, _( "%s stops processing devices." ),
                                who.disp_name( false, true ) );
    }
    act.set_to_null();
}

void efile_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "used_edevice", used_edevice );
    jsout.member( "target_edevices", target_edevices );
    jsout.member( "target_edevices_copy", target_edevices_copy );
    jsout.member( "action_type", action_type );
    jsout.member( "combo_type", combo_type );
    jsout.member( "selected_efiles", selected_efiles );
    jsout.member( "currently_processed_efiles", currently_processed_efiles );
    jsout.member( "target_edevices_count", target_edevices_count );
    jsout.member( "processed_edevices", processed_edevices );
    jsout.member( "failed_edevices", failed_edevices );
    jsout.member( "processed_efiles", processed_efiles );
    jsout.member( "failed_efiles", failed_efiles );
    jsout.member( "started_processing", started_processing );
    jsout.member( "done_processing", done_processing );
    jsout.member( "next_edevice_booted", next_edevice_booted );
    jsout.member( "computer_low_skill", computer_low_skill );
    jsout.member( "turns_left_on_current_edevice", turns_left_on_current_edevice );
    jsout.member( "turns_left_on_current_efile", turns_left_on_current_efile );
    jsout.end_object();
}

std::unique_ptr<activity_actor> efile_activity_actor::deserialize( JsonValue &jsin )
{
    efile_activity_actor actor = efile_activity_actor();

    JsonObject data = jsin.get_object();
    data.read( "used_edevice", actor.used_edevice );
    data.read( "target_edevices", actor.target_edevices );
    data.read( "target_edevices_copy", actor.target_edevices_copy );
    data.read( "turns_left_on_current_edevice", actor.turns_left_on_current_edevice );
    data.read( "action_type", actor.action_type );
    data.read( "combo_type", actor.combo_type );
    data.read( "target_edevices_count", actor.target_edevices_count );
    data.read( "processed_edevices", actor.processed_edevices );
    data.read( "selected_efiles", actor.selected_efiles );
    data.read( "currently_processed_efiles", actor.currently_processed_efiles );
    data.read( "failed_edevices", actor.failed_edevices );
    data.read( "processed_efiles", actor.processed_efiles );
    data.read( "failed_efiles", actor.failed_efiles );
    data.read( "started_processing", actor.started_processing );
    data.read( "done_processing", actor.done_processing );
    data.read( "next_edevice_booted", actor.next_edevice_booted );
    data.read( "computer_low_skill", actor.computer_low_skill );
    data.read( "turns_left_on_current_efile", actor.turns_left_on_current_efile );
    return actor.clone();
}

time_duration efile_activity_actor::total_processing_time(
    const item_location &used_edevice, const std::vector<item_location> &currently_processed_edevices,
    const std::vector<item_location> &selected_efiles,
    efile_action action_type, Character &who, bool computer_low_skill )
{
    time_duration total_time = device_bootup_time * currently_processed_edevices.size();
    for( const item_location &edevice : currently_processed_edevices ) {
        for( const item_location &efile : selected_efiles ) {
            efile_transfer transfer( used_edevice, edevice );
            total_time += efile_processing_time( efile, transfer, action_type, who, computer_low_skill );
        }
    }

    return total_time;
}

time_duration efile_activity_actor::efile_processing_time( const item_location &efile,
        const efile_transfer &transfer, efile_action action_type, Character &who, bool computer_low_skill )
{
    time_duration processing_time;
    if( action_type == EF_BROWSE ) {
        processing_time = time_per_browsed_efile;
    } else if( action_type == EF_WIPE ) {
        processing_time = 1_seconds + ( efile->ememory_size() / local_etransfer_rate ) * 1_seconds;
    } else if( efile_skip_copy( transfer, *efile ) ) {
        return 1_seconds;
    } else {
        //otherwise, this is a basic file transfer
        processing_time = 1_seconds + ( efile->ememory_size() / current_etransfer_rate(
                                            who, transfer, efile ) ) * 1_seconds;
    }
    if( computer_low_skill ) {
        processing_time *= 3;
    }
    return processing_time;
}

units::ememory efile_activity_actor::current_etransfer_rate( Character &who,
        const efile_transfer &transfer, const item_location &efile )
{
    const item_location &target_edevice = transfer.target_edevice;
    const item_location &used_edevice = transfer.used_edevice;
    const item_location &external_transfer_edevice =
        find_external_transfer_estorage( who, efile, target_edevice, used_edevice );

    auto get_etransfer_rate = []( const item_location & loc ) {
        units::ememory rate = loc->type->tool->etransfer_rate;
        if( rate == 0_KB ) {
            debugmsg( "%s is candidate for transferring e-files but has no etransfer_rate", loc->tname() );
            rate = 1_KB;
        }
        return rate;
    };

    units::ememory used_rate = get_etransfer_rate( used_edevice );
    units::ememory target_rate = get_etransfer_rate( target_edevice );
    units::ememory external_transfer_rate = external_transfer_edevice ?
                                            get_etransfer_rate( external_transfer_edevice ) / 2 : 0_KB;
    units::ememory slow_transfer_rate = 1_MB; //bluetooth

    units::ememory final_rate = std::min( used_rate, target_rate );

    edevice_compatible compat = edevices_compatible( used_edevice, target_edevice );
    if( compat == ECOMPAT_FAST ) {
        //do nothing
    } else if( compat == ECOMPAT_SLOW ) {
        if( external_transfer_rate > slow_transfer_rate ) {
            final_rate = external_transfer_rate;
        } else {
            final_rate = slow_transfer_rate; //bluetooth
        }
    } else {
        debugmsg( "incompatible devices %s, %s provided to efile_activity_actor",
                  used_edevice->display_name(), target_edevice->display_name() );
    }
    return final_rate;
}

std::string efile_activity_actor::efile_action_name( efile_action action_type, bool past_tense,
        bool extended )
{
    std::vector<std::string> base_names = { _( "browse" ), _( "read" ), _( "move" ), _( "move" ), _( "copy" ), _( "copy" ), _( "wipe" ) };
    std::vector<std::string> past_names = { _( "browsed" ), _( "read" ), _( "moved" ), _( "moved" ), _( "copied" ), _( "copied" ), _( "wiped" ) };
    std::vector<std::string> extensions = { "", "", _( " files onto" ), _( " files off of" ), _( " files onto" ), _( " files off of" ), "" };
    std::string name_output;
    past_tense ? name_output += past_names[action_type] : name_output += base_names[action_type];
    if( extended ) {
        name_output += extensions[action_type];
    }
    return name_output;
}

bool efile_activity_actor::efile_action_exclude_used( efile_action action_type )
{
    return action_type == EF_MOVE_FROM_THIS || action_type == EF_COPY_FROM_THIS ||
           action_type == EF_MOVE_ONTO_THIS || action_type == EF_COPY_ONTO_THIS;
}

bool efile_activity_actor::efile_action_is_from( efile_action action_type )
{
    return action_type == EF_MOVE_FROM_THIS || action_type == EF_COPY_FROM_THIS;
}

bool efile_activity_actor::efile_skip_copy( const efile_transfer &transfer, const item &efile )
{
    auto check_for_file = [&efile]( const item_location & edevice ) {
        for( const item *i : edevice->efiles() ) {
            if( i->typeId() == efile.typeId() ) {
                return true;
            }
        }
        return false;
    };
    if( efile.is_ecopiable() && !efile.has_flag( flag_E_FILE_COLLECTION ) ) {
        return check_for_file( transfer.used_edevice ) && check_for_file( transfer.target_edevice );
    }
    return false;
}
void migration_cancel_activity_actor::do_turn( player_activity &act, Character &who )
{
    // Stop the activity
    act.set_to_null();

    // Ensure that neither avatars nor npcs end up in an invalid state
    if( who.is_npc() ) {
        npc &npc_who = dynamic_cast<npc &>( who );
        npc_who.revert_after_activity();
    } else {
        avatar &avatar_who = dynamic_cast<avatar &>( who );
        avatar_who.clear_destination();
        avatar_who.backlog.clear();
    }
}

void migration_cancel_activity_actor::serialize( JsonOut &jsout ) const
{
    // This will probably never be called, but write null to avoid invalid json in
    // the case that it is
    jsout.write_null();
}

std::unique_ptr<activity_actor> migration_cancel_activity_actor::deserialize( JsonValue & )
{
    return migration_cancel_activity_actor().clone();
}

void open_gate_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void open_gate_activity_actor::finish( player_activity &act, Character & )
{
    gates::open_gate( placement );
    act.set_to_null();
}

void open_gate_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves_total );
    jsout.member( "placement", placement );

    jsout.end_object();
}

std::unique_ptr<activity_actor> open_gate_activity_actor::deserialize( JsonValue &jsin )
{
    open_gate_activity_actor actor( 0, tripoint_bub_ms::zero );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves_total );
    data.read( "placement", actor.placement );

    return actor.clone();
}

void consume_activity_actor::start( player_activity &act, Character &guy )
{
    int moves = 0;
    Character &player_character = get_player_character();
    if( consume_location ) {
        ret_val<edible_rating> ret = player_character.will_eat( *consume_location, true );
        if( !ret.success() ) {
            canceled = true;
            consume_menu_selections = std::vector<int>();
            consume_menu_selected_items.clear();
            consume_menu_filter.clear();
        } else {
            moves = to_moves<int>( guy.get_consume_time( *consume_location ) );
        }
    } else if( !consume_item.is_null() ) {
        ret_val<edible_rating> ret = player_character.will_eat( consume_item, true );
        if( !ret.success() ) {
            canceled = true;
            consume_menu_selections = std::vector<int>();
            consume_menu_selected_items.clear();
            consume_menu_filter.clear();
        } else {
            moves = to_moves<int>( guy.get_consume_time( consume_item ) );
        }
    } else {
        debugmsg( "Item/location to be consumed should not be null." );
        canceled = true;
    }

    act.moves_total = moves;
    act.moves_left = moves;
}

void consume_activity_actor::finish( player_activity &act, Character & )
{
    // Prevent interruptions from this point onwards, so that e.g. pain from
    // injecting serum doesn't pop up messages about cancelling consuming (it's
    // too late; we've already consumed).
    act.interruptable = false;

    // Consuming an item may cause various effects, including cancelling our activity.
    // Back up these values since this activity actor might be destroyed.
    std::vector<int> temp_selections = consume_menu_selections;
    const std::vector<item_location> temp_selected_items = consume_menu_selected_items;
    const std::string temp_filter = consume_menu_filter;
    item_location consume_loc = consume_location;
    activity_id new_act = type;

    avatar &player_character = get_avatar();
    if( !canceled ) {
        if( consume_loc ) {
            player_character.consume( consume_loc, /*force=*/true );
        } else if( !consume_item.is_null() ) {
            player_character.consume( consume_item, /*force=*/true );
        } else {
            debugmsg( "Item location/name to be consumed should not be null." );
        }
        if( player_character.get_value( "THIEF_MODE_KEEP" ).str() != "YES" ) {
            player_character.set_value( "THIEF_MODE", "THIEF_ASK" );
        }
    }

    if( act.id() == ACT_CONSUME ) {
        act.set_to_null();
    }

    if( act.id() == ACT_FIRSTAID && consume_loc ) {
        act.targets.clear();
        act.targets.push_back( consume_loc );
    }

    if( !temp_selections.empty() || !temp_selected_items.empty() || !temp_filter.empty() ) {
        if( act.is_null() ) {
            player_character.assign_activity( new_act );
            player_character.activity.values = temp_selections;
            player_character.activity.targets = temp_selected_items;
            player_character.activity.str_values = { temp_filter, "true" };
        } else {
            // Warning: this can add a redundant menu activity to the backlog.
            // It will prevent deleting the smart pointer of the selected item_location
            // if the backlog is not sanitized.
            player_activity eat_menu( new_act );
            eat_menu.values = temp_selections;
            eat_menu.targets = temp_selected_items;
            eat_menu.str_values = { temp_filter, "true" };
            player_character.backlog.push_back( eat_menu );
        }
    }
}

void consume_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "consume_location", consume_location );
    jsout.member( "consume_item", consume_item );
    jsout.member( "consume_menu_selections", consume_menu_selections );
    jsout.member( "consume_menu_selected_items", consume_menu_selected_items );
    jsout.member( "consume_menu_filter", consume_menu_filter );
    jsout.member( "canceled", canceled );
    jsout.member( "type", type );

    jsout.end_object();
}

std::unique_ptr<activity_actor> consume_activity_actor::deserialize( JsonValue &jsin )
{
    item_location null;
    consume_activity_actor actor( null );

    JsonObject data = jsin.get_object();

    data.read( "consume_location", actor.consume_location );
    data.read( "consume_item", actor.consume_item );
    data.read( "consume_menu_selections", actor.consume_menu_selections );
    data.read( "consume_menu_selected_items", actor.consume_menu_selected_items );
    data.read( "consume_menu_filter", actor.consume_menu_filter );
    data.read( "canceled", actor.canceled );
    data.read( "type", actor.type );

    return actor.clone();
}

void try_sleep_activity_actor::start( player_activity &act, Character &who )
{
    act.moves_total = to_moves<int>( duration );
    act.moves_left = act.moves_total;
    get_event_bus().send<event_type::character_attempt_to_fall_asleep>( who.getID() );
    who.set_movement_mode( move_mode_prone );
    who.add_msg_if_player( _( "You lie down preparing to fall asleep." ) );
}

void try_sleep_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( who.has_effect( effect_sleep ) ) {
        return;
    }
    if( who.can_sleep() ) {
        who.fall_asleep(); // calls act.set_to_null()
        if( !who.has_effect( effect_sleep ) ) {
            // Character can potentially have immunity for 'effect_sleep'
            who.add_msg_if_player(
                _( "You feel you should've fallen asleep by now, but somehow you're still awake." ) );
        }
        return;
    }
    if( one_in( 1000 ) ) {
        who.add_msg_if_player( _( "You toss and turn…" ) );
    }
    if( calendar::once_every( 30_minutes ) ) {
        query_keep_trying( act, who );
    }
}

void try_sleep_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    if( !who.has_effect( effect_sleep ) ) {
        who.add_msg_if_player( _( "You try to sleep, but can't." ) );
    }
    who.set_movement_mode( move_mode_walk );
}

void try_sleep_activity_actor::canceled( player_activity &, Character &who )
{
    if( who.movement_mode_is( move_mode_prone ) ) {
        who.set_movement_mode( move_mode_walk );
    }
}

void try_sleep_activity_actor::query_keep_trying( player_activity &act, Character &who )
{
    if( disable_query || !who.is_avatar() ) {
        return;
    }

    uilist sleep_query;
    sleep_query.text = _( "You have trouble sleeping, keep trying?" );
    sleep_query.addentry( 1, true, 'S', _( "Stop trying to fall asleep and get up." ) );
    sleep_query.addentry( 2, true, 'c', _( "Continue trying to fall asleep." ) );
    sleep_query.addentry( 3, true, 'C',
                          _( "Continue trying to fall asleep and don't ask again." ) );
    sleep_query.query();
    switch( sleep_query.ret ) {
        case UILIST_CANCEL:
        case 1:
            act.set_to_null();
            who.set_movement_mode( move_mode_walk );
            break;
        case 3:
            disable_query = true;
            break;
        case 2:
        default:
            break;
    }
}

void try_sleep_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "disable_query", disable_query );
    jsout.member( "duration", duration );

    jsout.end_object();
}

std::unique_ptr<activity_actor> try_sleep_activity_actor::deserialize( JsonValue &jsin )
{
    try_sleep_activity_actor actor = try_sleep_activity_actor( 0_seconds );

    JsonObject data = jsin.get_object();

    data.read( "disable_query", actor.disable_query );
    data.read( "duration", actor.duration );

    return actor.clone();
}

time_duration safecracking_activity_actor::safecracking_time( const Character &who )
{
    time_duration cracking_time = 150_minutes;
    /** @EFFECT_DEVICES decreases time needed for safe cracking */
    cracking_time -= 20_minutes * ( who.get_skill_level( skill_traps ) - 3 ) ;
    /** @EFFECT_PER decreases time needed for safe cracking */
    cracking_time -= 10_minutes * ( who.get_per() - 8 );
    cracking_time = std::max( 30_minutes, cracking_time );

    if( !who.has_proficiency( proficiency_prof_safecracking ) ) {
        cracking_time *= 3;
    }

    return cracking_time;
}

void safecracking_activity_actor::start( player_activity &act, Character &who )
{
    time_duration cracking_time = safecracking_time( who );

    if( who.is_avatar() ) {
        add_msg( m_info, _( "You start cracking the safe." ) );
    } else { // who.is_npc
        // npcs can't crack safes yet, this is for when they are able to
        add_msg_if_player_sees( who, _( "%1$s starts cracking the safe." ), who.disp_name() );
    }

    add_msg_debug( debugmode::DF_ACT_SAFECRACKING, "safecracking time = %s",
                   to_string_writable( cracking_time ) );

    act.moves_total = to_moves<int>( cracking_time );
    act.moves_left = act.moves_total;
}

void safecracking_activity_actor::do_turn( player_activity &act, Character &who )
{
    bool can_crack = who.has_flag( json_flag_SAFECRACK_NO_TOOL );
    // short-circuit to avoid the more expensive iteration over items
    can_crack = can_crack || who.cache_has_item_with( flag_SAFECRACK );

    if( !can_crack ) {
        // We lost our cracking tool somehow, bail out.
        act.set_to_null();
        return;
    }

    if( calendar::once_every( 5_minutes ) ) {
        add_msg_debug( debugmode::DF_ACT_SAFECRACKING, "safecracking time left = %s",
                       to_string_writable( time_duration::from_moves( act.moves_left ) ) );
    }

    /* Split experience gain in 20 parts, each giving 5% of the total*/
    const int current_step =
        ( 100 * ( act.moves_total - act.moves_left ) / act.moves_total ) / 5;
    const int difference = current_step - exp_step ;
    if( difference > 0 ) {
        exp_step += difference;
        if( !who.has_proficiency( proficiency_prof_safecracking ) ) {
            who.practice_proficiency( proficiency_prof_safecracking, 3_minutes * difference );
            // player gained the proficiency mid-way through cracking
            if( who.has_proficiency( proficiency_prof_safecracking ) ) {
                int new_time = to_moves<int>( safecracking_time( who ) );
                act.moves_total = act.moves_total > new_time ? new_time : act.moves_total;
                act.moves_left = act.moves_left > new_time ? new_time : act.moves_left;
            }
        }

        const float skill_level = who.get_skill_level( skill_traps );
        who.practice( skill_traps, std::max( 1.0f,  skill_level / 2 ) );
        if( who.get_skill_level( skill_traps ) > skill_level ) {
            int new_time = to_moves<int>( safecracking_time( who ) );
            act.moves_total = act.moves_total > new_time ? new_time : act.moves_total;
            act.moves_left = act.moves_left > new_time ? new_time : act.moves_left;
        }
    }
}

void safecracking_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( m_good, _( "With a satisfying click, the lock on the safe opens!" ) );
    get_map().furn_set( safe, furn_f_safe_c );
    act.set_to_null();
}

void safecracking_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "safe", safe );
    jsout.member( "exp_step", exp_step );
    jsout.end_object();
}

std::unique_ptr<activity_actor> safecracking_activity_actor::deserialize( JsonValue &jsin )
{
    safecracking_activity_actor actor = safecracking_activity_actor( {} );

    JsonObject data = jsin.get_object();
    data.read( "safe", actor.safe );
    data.read( "exp_step", actor.exp_step );
    return actor.clone();
}

void unload_activity_actor::start( player_activity &act, Character & )
{
    act.moves_left = moves_total;
    act.moves_total = moves_total;
}

void unload_activity_actor::finish( player_activity &act, Character &who )
{
    // Need to copy `target` here because `::unload` is static and `this` might be invalidated
    // after `act.set_to_null()` has been called (so `this.target` might also be invalidated)
    item_location target_copy = target;
    act.set_to_null();
    unload( who, target_copy );
}

void unload_activity_actor::unload( Character &who, item_location &target )
{
    int qty = 0;
    item &it = *target.get_item();
    bool actually_unloaded = false;

    if( it.is_container() ) {
        contents_change_handler handler;
        bool changed = false;

        for( pocket_type ptype : {
                 pocket_type::CONTAINER,
                 pocket_type::MAGAZINE_WELL,
                 pocket_type::MAGAZINE
             } ) {

            for( item *contained : it.all_items_top( ptype, true ) ) {
                int old_charges = contained->charges;
                const bool consumed = who.add_or_drop_with_msg( *contained, true, &it, contained );
                if( consumed || contained->charges != old_charges ) {
                    changed = true;
                    handler.unseal_pocket_containing( item_location( target, contained ) );
                }
                if( consumed ) {
                    it.remove_item( *contained );
                }
            }

            if( changed ) {
                it.on_contents_changed();
                who.invalidate_weight_carried_cache();
                handler.handle_by( who );
                // Warning: the above call to `contents_change_handler::handle_by` will
                // call `Character::handle_contents_changed`, which might invalidate items
                // and item_locations. See description for `::handle_contents_changed`
                // in character.h .
                // Therefore, it is important that we don't use `target` or `it` after here.
                break;
            }
        }

        return;
    }

    std::vector<item *> remove_contained;
    for( item *contained : it.all_items_top() ) {
        if( contained->ammo_type() == ammo_plutonium ) {
            contained->charges /= PLUTONIUM_CHARGES;
        }
        if( who.add_or_drop_with_msg( *contained, true, &it, contained ) ) {
            qty += contained->charges;
            remove_contained.push_back( contained );
            actually_unloaded = true;
        }
    }
    // remove the ammo leads in the belt
    for( item *remove : remove_contained ) {
        it.remove_item( *remove );
        actually_unloaded = true;
    }

    // remove the belt linkage
    if( it.is_ammo_belt() ) {
        if( it.type->magazine->linkage ) {
            item link( *it.type->magazine->linkage, calendar::turn, qty );
            if( who.add_or_drop_with_msg( link, true ) ) {
                actually_unloaded = true;
            }
        }
        if( actually_unloaded ) {
            who.add_msg_if_player( _( "You disassemble your %s." ), it.tname() );
        }
    } else if( actually_unloaded ) {
        who.add_msg_if_player( _( "You unload your %s." ), it.tname() );
    }

    if( it.has_flag( flag_MAG_DESTROY ) && it.ammo_remaining( ) == 0 ) {
        target.remove_item();
    }

    who.recoil = MAX_RECOIL;
}

void unload_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "target", target );

    jsout.end_object();
}

std::unique_ptr<activity_actor> unload_activity_actor::deserialize( JsonValue &jsin )
{
    unload_activity_actor actor = unload_activity_actor( 0, item_location::nowhere );

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "target", actor.target );

    return actor.clone();
}

craft_activity_actor::craft_activity_actor( item_location &it, const bool is_long ) :
    craft_item( it ), is_long( is_long )
{
    cached_crafting_speed = 0;
    cached_assistants = 0;
    cached_base_total_moves = 1;
    cached_cur_total_moves = 1;
    cached_workbench_multiplier = 0;
    use_cached_workbench_multiplier = false;
}

bool craft_activity_actor::check_if_craft_okay( item_location &craft_item, Character &crafter )
{
    item *craft = craft_item.get_item();

    // item_location::get_item() will return nullptr if the item is lost
    if( !craft || square_dist( craft_item.pos_abs(), crafter.pos_abs() ) > 1 ) {
        crafter.add_msg_player_or_npc(
            _( "You no longer have the in progress craft in your possession.  "
               "You stop crafting.  "
               "Reactivate the in progress craft to continue crafting." ),
            _( "<npcname> no longer has the in progress craft in their possession.  "
               "<npcname> stops crafting." ) );
        return false;
    }

    if( !craft->is_craft() ) {
        debugmsg( "ACT_CRAFT target '%s' is not a craft.  Aborting ACT_CRAFT.", craft->tname() );
        return false;
    }

    if( !cached_continuation_requirements ) {
        cached_continuation_requirements = craft->get_continue_reqs();
    }
    return crafter.can_continue_craft( *craft, *cached_continuation_requirements );
}

void craft_activity_actor::start( player_activity &act, Character &crafter )
{
    if( !check_if_craft_okay( craft_item, crafter ) ) {
        act.set_to_null();
    }
    activity_override = craft_item.get_item()->get_making().exertion_level();
    cached_crafting_speed = 0;
    cached_workbench_multiplier = 0;
    use_cached_workbench_multiplier = false;
    act.targets.clear();
    act.targets.push_back( craft_item );
}

void craft_activity_actor::do_turn( player_activity &act, Character &crafter )
{
    map &here = get_map();

    if( !check_if_craft_okay( craft_item, crafter ) ) {
        act.set_to_null();
        return;
    }

    // We already checked if this is nullptr above
    item &craft = *craft_item.get_item();

    const std::optional<tripoint_bub_ms> location = craft_item.where() == item_location::type::character
            ? std::optional<tripoint_bub_ms>() : std::optional<tripoint_bub_ms>( craft_item.pos_bub( here ) );
    const recipe &rec = craft.get_making();
    if( !use_cached_workbench_multiplier ) {
        cached_workbench_multiplier = crafter.workbench_crafting_speed_multiplier( craft, location );
        use_cached_workbench_multiplier = true;
    }
    const float crafting_speed = crafter.crafting_speed_multiplier( craft, location,
                                 use_cached_workbench_multiplier,
                                 cached_workbench_multiplier );
    const int assistants = crafter.available_assistant_count( craft.get_making() );

    if( crafting_speed <= 0.0f ) {
        craft.erase_var( "crafter" );
        crafter.cancel_activity();
        return;
    }

    if( cached_crafting_speed != crafting_speed || cached_assistants != assistants ) {
        cached_crafting_speed = crafting_speed;
        cached_assistants = assistants;

        // Base moves for batch size with no speed modifier or assistants
        // Must ensure >= 1 so we don't divide by 0;
        cached_base_total_moves = std::max( static_cast<int64_t>( 1 ),
                                            rec.batch_time( crafter, craft.get_making_batch_size(), 1.0f, 0 ) );
        // Current expected total moves, includes crafting speed modifiers and assistants
        cached_cur_total_moves = std::max( static_cast<int64_t>( 1 ),
                                           rec.batch_time( crafter, craft.get_making_batch_size(), crafting_speed,
                                                   assistants ) );
    }
    const double base_total_moves = cached_base_total_moves;
    const double cur_total_moves = cached_cur_total_moves;

    // item_counter represents the percent progress relative to the base batch time
    // stored precise to 5 decimal places ( e.g. 67.32 percent would be stored as 6732000 )
    const int old_counter = craft.item_counter;

    // Delta progress in moves adjusted for current crafting speed /
    //crafter.exertion_adjusted_move_multiplier( exertion_level() )
    int spent_moves = crafter.get_moves() * crafter.exertion_adjusted_move_multiplier(
                          exertion_level() );
    const double delta_progress = spent_moves * base_total_moves / cur_total_moves;
    // Current progress in moves
    const double current_progress = craft.item_counter * base_total_moves / 10000000.0 +
                                    delta_progress;
    // Current progress as a percent of base_total_moves to 2 decimal places
    craft.item_counter = std::round( current_progress / base_total_moves * 10000000.0 );
    crafter.set_moves( 0 );

    // This is to ensure we don't over count skill steps
    craft.item_counter = std::min( craft.item_counter, 10000000 );

    // This nominal craft time is also how many practice ticks to perform
    // spread out evenly across the actual duration.
    const double total_practice_ticks = rec.time_to_craft_moves( crafter,
                                        recipe_time_flag::ignore_proficiencies ) / 100.0;

    const int ticks_per_practice = 10000000.0 / total_practice_ticks;
    int num_practice_ticks = craft.item_counter / ticks_per_practice -
                             old_counter / ticks_per_practice;
    bool level_up = false;
    if( num_practice_ticks > 0 ) {
        level_up |= crafter.craft_skill_gain( craft, num_practice_ticks );
    }
    // Proficiencies and tools are gained/consumed after every 5% progress
    int five_percent_steps = craft.item_counter / 500000 - old_counter / 500000;
    if( five_percent_steps > 0 ) {
        // Divide by 100 for seconds, 20 for 5%
        const time_duration pct_time = time_duration::from_seconds( base_total_moves / 2000 );
        level_up |= crafter.craft_proficiency_gain( craft, pct_time * five_percent_steps );
        // Invalidate the crafting time cache because proficiencies may have changed
        cached_crafting_speed = 0;
        // Also reset the multiplier
        use_cached_workbench_multiplier = false;
    }

    // Unlike skill, tools are consumed once at the start and should not be consumed at the end
    if( craft.item_counter >= 10000000 ) {
        --five_percent_steps;
    }

    if( five_percent_steps > 0 ) {
        if( !crafter.craft_consume_tools( craft, five_percent_steps, false ) ) {
            // So we don't skip over any tool comsuption
            craft.item_counter -= craft.item_counter % 500000 + 1;
            craft.erase_var( "crafter" );
            crafter.cancel_activity();
            return;
        }
    }

    // if item_counter has reached 100% or more
    if( craft.item_counter >= 10000000 ) {
        if( rec.is_practice() && !is_long && craft.get_making_batch_size() == 1 ) {
            if( crafter.is_avatar() && query_yn( _( "Keep practicing until proficiency increases?" ) ) ) {
                is_long = true;
                *( crafter.last_craft ) = craft_command( &craft.get_making(), 1, is_long, &crafter, location );
            }
        }
        item craft_copy = craft;
        craft_item.remove_item();
        // We need to cache this before we cancel the activity else we risk Use After Free
        const bool will_continue = is_long;
        crafter.cancel_activity();
        crafter.complete_craft( craft_copy, location );
        if( will_continue ) {
            if( crafter.making_would_work( crafter.lastrecipe, craft_copy.get_making_batch_size() ) ) {
                crafter.last_craft->execute( location );
            }
        }
    } else {
        if( level_up && craft.get_making().is_practice() ) {
            if( crafter.is_avatar() ) {
                if( query_yn( _( "Your proficiency has increased.  Stop practicing?" ) ) ) {
                    crafter.cancel_activity();
                    return;
                }
            } else if( crafter.is_npc() ) {
                crafter.cancel_activity();
                return;
            }
        }

        if( craft.item_counter >= craft.get_next_failure_point() ) {
            bool destroy = craft.handle_craft_failure( crafter );
            // If the craft needs to be destroyed, do it and stop crafting.
            if( destroy ) {
                crafter.add_msg_player_or_npc( _( "There is nothing left of the %s to craft from." ),
                                               _( "There is nothing left of the %s <npcname> was crafting." ), craft.tname() );
                craft_item.remove_item();
                crafter.cancel_activity();
            }
        }
    }
}

void craft_activity_actor::finish( player_activity &act, Character & )
{
    act.set_to_null();
}

void craft_activity_actor::canceled( player_activity &/*act*/, Character &/*who*/ )
{
    item *craft = craft_item.get_item();
    // item_location::get_item() will return nullptr if the item is lost
    if( !craft ) {
        return;
    }
    const recipe item_recipe = craft->get_making();
    // recipe items with no components and results/result_eocs can be safely removed
    if( item_recipe.result() == itype_id::NULL_ID() && item_recipe.result_eocs.empty() &&
        craft->components.empty() ) {
        craft_item.remove_item();
    }
}

std::string craft_activity_actor::get_progress_message( const player_activity & ) const
{
    if( !craft_item ) {
        //We have somehow lost the craft item.  This will be handled in do_turn in the check_if_craft_is_ok call.
        return "";
    }
    return craft_item.get_item()->tname();
}

float craft_activity_actor::exertion_level() const
{
    return activity_override;
}

void craft_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "craft_loc", craft_item );
    jsout.member( "long", is_long );
    jsout.member( "activity_override", activity_override );

    jsout.end_object();
}

std::unique_ptr<activity_actor> craft_activity_actor::deserialize( JsonValue &jsin )
{
    JsonObject data = jsin.get_object();

    craft_activity_actor actor = craft_activity_actor();

    data.read( "craft_loc", actor.craft_item );
    data.read( "long", actor.is_long );
    data.read( "activity_override", actor.activity_override );

    return actor.clone();
}

void workout_activity_actor::start( player_activity &act, Character &who )
{
    if( who.get_sleepiness() > sleepiness_levels::DEAD_TIRED ) {
        who.add_msg_if_player( _( "You are too tired to exercise." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( who.get_thirst() > 240 ) {
        who.add_msg_if_player( _( "You are too dehydrated to exercise." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( who.is_armed() ) {
        who.add_msg_if_player( _( "Empty your hands first." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    map &here = get_map();
    // free training requires all limbs intact, but specialized workout machines
    // train upper or lower parts of body only and may permit workout with
    // broken limbs as long as they are not involved by the machine
    bool hand_equipment = here.has_flag_furn( ter_furn_flag::TFLAG_WORKOUT_ARMS, location );
    bool leg_equipment = here.has_flag_furn( ter_furn_flag::TFLAG_WORKOUT_LEGS, location );

    if( hand_equipment && ( who.is_limb_broken( body_part_arm_l ) ||
                            who.is_limb_broken( body_part_arm_r ) ) ) {
        who.add_msg_if_player( _( "You cannot train here with a broken arm." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( leg_equipment && ( who.is_limb_broken( body_part_leg_l ) ||
                           who.is_limb_broken( body_part_leg_r ) ) ) {
        who.add_msg_if_player( _( "You cannot train here with a broken leg." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    if( !hand_equipment && !leg_equipment &&
        ( who.is_limb_broken( body_part_arm_l ) ||
          who.is_limb_broken( body_part_arm_r ) ||
          who.is_limb_broken( body_part_leg_l ) ||
          who.is_limb_broken( body_part_leg_r ) ) ) {
        who.add_msg_if_player( _( "You cannot train freely with a broken limb." ) );
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    uilist workout_query;
    workout_query.desc_enabled = true;
    workout_query.text =
        _( "Physical effort determines workout efficiency, but also rate of exhaustion." );
    workout_query.title = _( "Choose training intensity:" );
    workout_query.addentry_desc( 1, true, 'l', pgettext( "training intensity", "Light" ),
                                 _( "Light exercise comparable in intensity to walking, but more focused and methodical." ) );
    workout_query.addentry_desc( 2, true, 'm', pgettext( "training intensity", "Moderate" ),
                                 _( "Moderate exercise without excessive exertion, but with enough effort to break a sweat." ) );
    workout_query.addentry_desc( 3, true, 'a', pgettext( "training intensity", "Active" ),
                                 _( "Active exercise with full involvement.  Strenuous, but in a controlled manner." ) );
    workout_query.addentry_desc( 4, true, 'h', pgettext( "training intensity", "High" ),
                                 _( "High intensity exercise with maximum effort and full power.  Exhausting in the long run." ) );
    workout_query.query();
    switch( workout_query.ret ) {
        case UILIST_CANCEL:
            act.set_to_null();
            act_id = activity_id::NULL_ID();
            return;
        case 4:
            act_id = ACT_WORKOUT_HARD;
            intensity_modifier = 4;
            break;
        case 3:
            act_id = ACT_WORKOUT_ACTIVE;
            intensity_modifier = 3;
            break;
        case 2:
            act_id = ACT_WORKOUT_MODERATE;
            intensity_modifier = 2;
            break;
        case 1:
        default:
            act_id = ACT_WORKOUT_LIGHT;
            intensity_modifier = 1;
            break;
    }
    int length = 0;
    query_int( length, false, _( "Train for how long (minutes): " ) );
    if( length > 0 ) {
        duration = length * 1_minutes;
    } else {
        act_id = activity_id::NULL_ID();
        act.set_to_null();
        return;
    }
    act.moves_total = to_moves<int>( duration );
    act.moves_left = act.moves_total;
    if( who.male ) {
        sfx::play_activity_sound( "plmove", "sleepiness_m_med", sfx::get_heard_volume( location ) );
    } else {
        sfx::play_activity_sound( "plmove", "sleepiness_f_med", sfx::get_heard_volume( location ) );
    }
    who.add_msg_if_player( _( "You start your workout session." ) );
}

void workout_activity_actor::do_turn( player_activity &act, Character &who )
{
    if( who.get_sleepiness() > sleepiness_levels::DEAD_TIRED ) {
        who.add_msg_if_player( _( "You are exhausted so you finish your workout early." ) );
        act.set_to_null();
        return;
    }
    if( who.get_thirst() > 240 ) {
        who.add_msg_if_player( _( "You are dehydrated so you finish your workout early." ) );
        act.set_to_null();
        return;
    }
    if( !rest_mode && who.get_stamina() > who.get_stamina_max() / 3 ) {
        who.mod_stamina( -25 - intensity_modifier );
        if( one_in( 180 / intensity_modifier ) ) {
            who.mod_sleepiness( 1 );
            who.mod_thirst( 1 );
        }
        if( calendar::once_every( 16_minutes / intensity_modifier ) ) {
            //~ heavy breathing when exercising
            std::string huff = _( "yourself huffing and puffing!" );
            sounds::sound( location + tripoint::east, 2 * intensity_modifier, sounds::sound_t::speech, huff,
                           true );
        }
        // morale bonus kicks in gradually after 5 minutes of exercise
        if( calendar::once_every( 2_minutes ) &&
            ( ( elapsed + act.moves_total - act.moves_left ) * 1_turns / 100 ) > 5_minutes ) {
            who.add_morale( morale_feeling_good, intensity_modifier, 20, 6_hours, 30_minutes );
        }
        if( calendar::once_every( 2_minutes ) ) {
            add_msg_debug_if( who.is_avatar(), debugmode::DF_ACT_WORKOUT, who.activity_level_str() );
            add_msg_debug_if( who.is_avatar(), debugmode::DF_ACT_WORKOUT, act.id().c_str() );
        }
    } else if( !rest_mode ) {
        rest_mode = true;
        who.add_msg_if_player( _( "You catch your breath for few moments." ) );
    } else if( who.get_stamina() >= who.get_stamina_max() ) {
        rest_mode = false;
        who.add_msg_if_player( _( "You get back to your training." ) );
    }
}

void workout_activity_actor::finish( player_activity &act, Character &who )
{
    if( !query_keep_training( act, who ) ) {
        act.set_to_null();
        who.add_msg_if_player( _( "You finish your workout session." ) );
    }
}

void workout_activity_actor::canceled( player_activity &/*act*/, Character &/*who*/ )
{
    stop_time = calendar::turn;
}

bool workout_activity_actor::query_keep_training( player_activity &act, Character &who )
{
    if( disable_query || !who.is_avatar() ) {
        elapsed += act.moves_total - act.moves_left;
        act.moves_total = to_moves<int>( 60_minutes );
        act.moves_left = act.moves_total;
        return true;
    }
    int length;
    uilist workout_query;
    workout_query.text = _( "You have finished your training cycle, keep training?" );
    workout_query.addentry( 1, true, 'S', _( "Stop training." ) );
    workout_query.addentry( 2, true, 'c', _( "Continue training." ) );
    workout_query.addentry( 3, true, 'C', _( "Continue training and don't ask again." ) );
    workout_query.query();
    switch( workout_query.ret ) {
        case UILIST_CANCEL:
        case 1:
            act_id = activity_id::NULL_ID();
            act.set_to_null();
            return false;
        case 3:
            disable_query = true;
            elapsed += act.moves_total - act.moves_left;
            act.moves_total = to_moves<int>( 60_minutes );
            act.moves_left = act.moves_total;
            return true;
        case 2:
        default:
            query_int( length, false, _( "Train for how long (minutes): " ) );
            elapsed += act.moves_total - act.moves_left;
            duration = 0_minutes;
            if( length > 0 ) {
                duration = length * 1_minutes;
            }
            act.moves_total = to_moves<int>( duration );
            act.moves_left = act.moves_total;
            return true;
    }
}

void workout_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "disable_query", disable_query );
    jsout.member( "act_id", act_id );
    jsout.member( "duration", duration );
    jsout.member( "location", location );
    jsout.member( "stop_time", stop_time );
    jsout.member( "elapsed", elapsed );
    jsout.member( "intensity_modifier", intensity_modifier );
    jsout.member( "rest_mode", rest_mode );

    jsout.end_object();
}

std::unique_ptr<activity_actor> workout_activity_actor::deserialize( JsonValue &jsin )
{
    workout_activity_actor actor{ tripoint_bub_ms::invalid };

    JsonObject data = jsin.get_object();

    data.read( "disable_query", actor.disable_query );
    data.read( "act_id", actor.act_id );
    data.read( "duration", actor.duration );
    data.read( "location", actor.location );
    data.read( "stop_time", actor.stop_time );
    data.read( "elapsed", actor.elapsed );
    data.read( "intensity_modifier", actor.intensity_modifier );
    data.read( "rest_mode", actor.rest_mode );

    return actor.clone();
}

// TODO: Display costs in the multidrop menu
static void debug_drop_list( const std::vector<drop_or_stash_item_info> &items )
{
    if( !debug_mode ) {
        return;
    }

    std::string res( "Items ordered to drop:\n" );
    for( const drop_or_stash_item_info &it : items ) {
        item_location loc = it.loc();
        if( !loc ) {
            // some items could have been destroyed by e.g. monster attack
            continue;
        }
        res += string_format( "Drop %d %s\n", it.count(), loc->display_name( it.count() ) );
    }
    popup( res, PF_GET_KEY );
}

static bool is_bulk_unload( const item_location &lhs, const item_location &rhs )
{
    if( lhs.where() == item_location::type::container &&
        rhs.where() == item_location::type::container && lhs.parent_item() == rhs.parent_item() ) {
        return true;
    }
    return false;
}

// Return a list of items to be dropped by the given item-dropping activity in the current turn.
static std::list<item> obtain_activity_items( std::vector<drop_or_stash_item_info> &items,
        contents_change_handler &handler, Character &who, bool &bulk_unload )
{
    std::list<item> res;

    debug_drop_list( items );

    auto it = items.begin();
    for( ; it != items.end(); ++it ) {
        item_location loc = it->loc();
        if( !loc ) {
            // some items could have been destroyed by e.g. monster attack
            continue;
        }

        int consumed_moves;
        if( !bulk_unload ) {
            // Cost to take an item from a container or map
            consumed_moves = loc.obtain_cost( who, it->count() );
        } else {
            // Pure cost to handling item excluding overhead.
            consumed_moves = std::max( who.item_handling_cost( *loc, true, 0, it->count(), true ), 1 );
        }
        if( !who.is_npc() && who.get_moves() <= 0 && consumed_moves > 0 ) {
            break;
        }

        // Check if next is bulk unload before moving item.
        auto it_next = std::next( it );
        if( it_next != items.end() && it_next->loc() ) {
            bulk_unload = is_bulk_unload( loc, it_next->loc() );
        }

        who.mod_moves( -consumed_moves );

        // If item is inside another (container/pocket), unseal it, and update encumbrance
        if( loc.has_parent() ) {
            // Save parent items to be handled when finishing or canceling activity,
            // in case there are still other items to drop from the same container.
            // This assumes that an item and any of its parents/ancestors are not
            // dropped at the same time.
            handler.unseal_pocket_containing( loc );
        } else {
            // when parent's encumbrance cannot be marked as dirty,
            // mark character's encumbrance as dirty instead (correctness over performance)
            who.set_check_encumbrance( true );
        }

        // Take off the item or remove it from where it's been
        if( loc.where_recursive() == item_location::type::map ||
            loc.where_recursive() == item_location::type::vehicle ) {
            item copy( *loc );
            if( loc->count_by_charges() && loc->count() > it->count() ) {
                copy.charges -= it->count();
            }
            loc.remove_item();
            res.push_back( copy );
        } else if( who.is_worn( *loc ) ) {
            who.takeoff( loc, &res );
        } else if( loc->count_by_charges() ) {
            res.push_back( who.reduce_charges( &*loc, it->count() ) );
        } else {
            res.push_back( who.i_rem( &*loc ) );
        }
    }

    // Remove handled items from activity
    items.erase( items.begin(), it );
    return res;
}

void drop_or_stash_item_info::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "loc", _loc );
    jsout.member( "count", _count );
    jsout.end_object();
}

void drop_or_stash_item_info::deserialize( const JsonObject &jsobj )
{
    jsobj.read( "loc", _loc );
    jsobj.read( "count", _count );
}

void drop_activity_actor::do_turn( player_activity &, Character &who )
{
    map &here = get_map();
    const tripoint_bub_ms pos = placement + who.pos_bub();
    put_into_vehicle_or_drop( who, item_drop_reason::deliberate,
                              obtain_activity_items( items, handler, who, current_bulk_unload ),
                              &here, pos, force_ground );
    // Cancel activity if items is empty. Otherwise, we modified in place and we will continue
    // to resolve the drop next turn. This is different from the pickup logic which creates
    // a brand new activity every turn and cancels the old activity
    if( items.empty() ) {
        who.cancel_activity();
    }
}

void drop_activity_actor::canceled( player_activity &, Character &who )
{
    handler.handle_by( who );
}

void drop_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "items", items );
    jsout.member( "unhandled_containers", handler );
    jsout.member( "placement", placement );
    jsout.member( "force_ground", force_ground );
    jsout.member( "current_bulk_unload", current_bulk_unload );
    jsout.end_object();
}

std::unique_ptr<activity_actor> drop_activity_actor::deserialize( JsonValue &jsin )
{
    drop_activity_actor actor;

    JsonObject jsobj = jsin.get_object();
    jsobj.read( "items", actor.items );
    jsobj.read( "unhandled_containers", actor.handler );
    jsobj.read( "placement", actor.placement );
    jsobj.read( "force_ground", actor.force_ground );
    jsobj.read( "current_bulk_unload", actor.current_bulk_unload );

    return actor.clone();
}

void harvest_activity_actor::start( player_activity &act, Character &who )
{
    map &here = get_map();

    if( here.has_furn( target ) ) {
        const furn_id &furn = here.furn( target );

        if( furn->has_examine( iexamine::harvest_furn ) ) {
            // TODO: Should be generified as a harvest field
            if( furn->has_flag( ter_furn_flag::TFLAG_HARVEST_REQ_CUT1 ) && who.max_quality( qual_CUT ) < 1 ) {
                if( !auto_forage ) {
                    who.add_msg_if_player( m_info,
                                           _( "You'll need a cutting tool to harvest this." ) );
                }
                act.set_to_null();
                return;
            }
            exam_furn = true;
        } else if( furn->has_examine( iexamine::harvest_furn_nectar ) )  {
            exam_furn = true;
            nectar = true;
        }
    }

    const harvest_id harvest = here.get_harvest( target );
    if( harvest.is_null() || harvest->empty() ) {
        if( !auto_forage ) {
            who.add_msg_if_player( m_info,
                                   _( "Nothing can be harvested from this plant in the current season." ) );
        }

        if( who.manual_examine ) {
            iexamine::none( who, target );
        }

        act.set_to_null();
        return;
    }

    const time_duration forage_time = rng( 5_seconds, 15_seconds );
    add_msg_debug( debugmode::DF_ACT_HARVEST, "harvest time: %s", to_string_writable( forage_time ) );
    act.moves_total = to_moves<int>( forage_time );
    act.moves_left = act.moves_total;
}

void harvest_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();

    map &here = get_map();

    // If nothing can be harvested, neither can nectar
    // Incredibly low priority TODO: Allow separating nectar seasons
    if( nectar && iexamine_helper::drink_nectar( who ) ) {
        return;
    }

    const float survival_skill = who.get_skill_level( skill_survival );
    bool got_anything = false;
    for( const harvest_entry &entry : here.get_harvest( target ).obj() ) {
        const float min_num = entry.scale_num.first * survival_skill + entry.base_num.first;
        const float max_num = entry.scale_num.second * survival_skill + entry.base_num.second;
        const int roll = std::min<int>( entry.max, std::round( rng_float( min_num, max_num ) ) );
        got_anything = roll > 0;
        for( int i = 0; i < roll; i++ ) {
            iexamine_helper::handle_harvest( who, itype_id( entry.drop ), false );
        }
    }

    if( !got_anything ) {
        who.add_msg_if_player( m_bad, _( "You couldn't harvest anything." ) );
    }

    if( exam_furn ) {
        here.furn_set( target, furn_str_id::NULL_ID() );
    } else {
        here.ter_set( target, here.get_ter_transforms_into( target ) );
    }

    iexamine::practice_survival_while_foraging( who );
}

void harvest_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "target", target );
    jsout.member( "exam_furn", exam_furn );
    jsout.member( "nectar", nectar );
    jsout.member( "auto_forage", auto_forage );
    jsout.end_object();
}

std::unique_ptr<activity_actor> harvest_activity_actor::deserialize( JsonValue &jsin )
{
    harvest_activity_actor actor{ tripoint_bub_ms::invalid };

    JsonObject jsobj = jsin.get_object();
    jsobj.read( "target", actor.target );
    jsobj.read( "exam_furn", actor.exam_furn );
    jsobj.read( "nectar", actor.nectar );
    jsobj.read( "auto_forage", actor.auto_forage );
    return actor.clone();
}

static void stash_on_pet( const std::list<item> &items, monster &pet, Character &who )
{
    units::volume remaining_volume = pet.storage_item->get_total_capacity() - pet.get_carried_volume();
    units::mass remaining_weight = pet.weight_capacity() - pet.get_carried_weight();
    map &here = get_map();

    for( const item &it : items ) {
        if( it.volume() > remaining_volume ) {
            add_msg( m_bad, _( "%1$s did not fit and fell to the %2$s." ), it.display_name(),
                     here.name( pet.pos_bub() ) );
            here.add_item_or_charges( pet.pos_bub(), it );
        } else if( it.weight() > remaining_weight ) {
            add_msg( m_bad, _( "%1$s is too heavy and fell to the %2$s." ), it.display_name(),
                     here.name( pet.pos_bub() ) );
            here.add_item_or_charges( pet.pos_bub(), it );
        } else {
            pet.add_item( it );
            remaining_volume -= it.volume();
            remaining_weight -= it.weight();
        }
        // TODO: if NPCs can have pets or move items onto pets
        item( it ).handle_pickup_ownership( who );
    }
}

void stash_activity_actor::do_turn( player_activity &, Character &who )
{
    const tripoint_bub_ms pos = placement + who.pos_bub();

    monster *pet = get_creature_tracker().creature_at<monster>( pos );
    if( pet != nullptr && pet->has_effect( effect_pet ) ) {
        stash_on_pet( obtain_activity_items( items, handler, who, current_bulk_unload ),
                      *pet, who );
        if( items.empty() ) {
            who.cancel_activity();
        }
    } else {
        who.add_msg_if_player( _( "The pet has moved somewhere else." ) );
        who.cancel_activity();
    }
}

void stash_activity_actor::canceled( player_activity &, Character &who )
{
    handler.handle_by( who );
}

void stash_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "items", items );
    jsout.member( "unhandled_containers", handler );
    jsout.member( "placement", placement );
    jsout.member( "current_bulk_unload", current_bulk_unload );
    jsout.end_object();
}

std::unique_ptr<activity_actor> stash_activity_actor::deserialize( JsonValue &jsin )
{
    stash_activity_actor actor;

    JsonObject jsobj = jsin.get_object();
    jsobj.read( "items", actor.items );
    jsobj.read( "unhandled_containers", actor.handler );
    jsobj.read( "placement", actor.placement );
    jsobj.read( "current_bulk_unload", actor.current_bulk_unload );

    return actor.clone();
}

void disable_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
    monster &critter = *get_creature_tracker().creature_at<monster>( target );
    critter.add_effect( effect_worked_on, 1_turns );
}

void disable_activity_actor::do_turn( player_activity &, Character &who )
{
    monster *const mon_ptr = get_creature_tracker().creature_at<monster>( target );
    if( !mon_ptr ) {
        who.add_msg_if_player( _( "The robot has moved somewhere else." ) );
        who.cancel_activity();
        return;
    }

    monster &critter = *mon_ptr;
    if( !can_disable_or_reprogram( critter ) ) {
        // I think recovery from stunned is the only reason this could happen
        who.add_msg_if_player( _( "The %s recovers before you can finish." ), critter.name() );
        who.cancel_activity();
        return;
    }

    critter.add_effect( effect_worked_on, 1_turns );
}

void disable_activity_actor::finish( player_activity &act, Character &/*who*/ )
{
    map &here = get_map();

    // Should never be null as we just checked in do_turn
    monster &critter = *get_creature_tracker().creature_at<monster>( target );

    if( reprogram ) {
        if( critter.has_effect( effect_docile ) ) {
            critter.remove_effect( effect_docile );
            if( one_in( 3 ) ) {
                add_msg( _( "The %s hovers momentarily as it surveys the area." ),
                         critter.name() );
            }
        } else {
            critter.add_effect( effect_docile, 1_turns, true );
            if( one_in( 3 ) ) {
                add_msg( _( "The %s lets out a whirring noise and starts to follow you." ),
                         critter.name() );
            }
        }
    } else {
        here.add_item_or_charges( target, critter.to_item() );
        if( !critter.has_flag( mon_flag_INTERIOR_AMMO ) ) {
            for( std::pair<const itype_id, int> &ammodef : critter.ammo ) {
                if( ammodef.second > 0 ) {
                    here.spawn_item( target.xy(), ammodef.first, 1, ammodef.second, calendar::turn );
                }
            }
        }
        g->remove_zombie( critter );
    }

    act.set_to_null();
}

bool disable_activity_actor::can_disable_or_reprogram( const monster &monster )
{
    if( get_avatar().get_knowledge_level( skill_electronics ) + get_avatar().get_knowledge_level(
            skill_mechanics ) <= 0 ) {
        return false;
    }

    return ( ( monster.friendly != 0 || monster.has_effect( effect_sensor_stun ) ) &&
             !monster.has_flag( mon_flag_RIDEABLE_MECH ) &&
             !( monster.has_flag( mon_flag_PAY_BOT ) && monster.has_effect( effect_paid ) ) ) &&
           ( !monster.type->revert_to_itype.is_empty() ) &&
           ( get_avatar().has_identified( itype_robofac_yrax_trifacet_deactivation_manual ) ||
             monster.type->id != mon_yrax_trifacet );
}

int disable_activity_actor::get_disable_turns()
{
    const float elec_skill = get_avatar().get_skill_level( skill_electronics );
    const float mech_skill = get_avatar().get_skill_level( skill_mechanics );
    const float time_scale = std::max( elec_skill + mech_skill, 1.0f );
    return 2000 / time_scale;
}

void disable_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "target", target );
    jsout.member( "reprogram", reprogram );
    jsout.member( "moves_total", moves_total );
    jsout.end_object();
}

std::unique_ptr<activity_actor> disable_activity_actor::deserialize( JsonValue &jsin )
{
    disable_activity_actor actor = disable_activity_actor();

    JsonObject data = jsin.get_object();

    data.read( "target", actor.target );
    data.read( "reprogram", actor.reprogram );
    data.read( "moves_total", actor.moves_total );

    return actor.clone();
}

void move_furniture_activity_actor::start( player_activity &act, Character & )
{
    int moves = g->grabbed_furn_move_time( dp );
    act.moves_left = moves;
    act.moves_total = moves;
}

void move_furniture_activity_actor::finish( player_activity &act, Character &who )
{
    if( !g->grabbed_furn_move( dp ) ) {
        g->walk_move( who.pos_bub() + dp, via_ramp, true );
    }
    act.set_to_null();
}

void move_furniture_activity_actor::canceled( player_activity &, Character & )
{
    add_msg( m_warning, _( "You let go of the grabbed object." ) );
    get_avatar().grab( object_type::NONE );
}

void move_furniture_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "dp", dp );
    jsout.member( "via_ramp", via_ramp );
    jsout.end_object();
}

std::unique_ptr<activity_actor> move_furniture_activity_actor::deserialize( JsonValue &jsin )
{
    move_furniture_activity_actor actor = move_furniture_activity_actor( tripoint_rel_ms::zero, false );

    JsonObject data = jsin.get_object();

    data.read( "dp", actor.dp );
    data.read( "via_ramp", actor.via_ramp );

    return actor.clone();
}

static int item_move_cost( Character &who, item_location &item, bool bulk_cost )
{
    if( !bulk_cost ) {
        // Cost to take an item from a container or map
        const int obtain_cost = item.obtain_cost( who );
        // Cost to move an item to a container, vehicle or the ground
        const int move_cost = Pickup::cost_to_move_item( who, *item );
        return obtain_cost + move_cost;
    } else {
        // Pure cost to handling item excluding overhead.
        return std::max( who.item_handling_cost( *item, true, 0, -1, true ), 1 );
    }
}

static bool is_bulk_load( const item_location &lhs, const item_location &rhs )
{
    if( lhs.where() == rhs.where() && lhs->stacks_with( *rhs ) ) {
        switch( lhs.where() ) {
            case item_location::type::container:
                return lhs.parent_item() == rhs.parent_item();
                break;
            case item_location::type::map:
            case item_location::type::vehicle:
                return lhs.pos_abs() == rhs.pos_abs();
                break;
            default:
                break;
        }
    }
    return false;
}

void insert_item_activity_actor::start( player_activity &act, Character &who )
{
    if( items.empty() ) {
        debugmsg( "ACT_INSERT_ITEM was passed an empty list" );
        act.set_to_null();
    }

    all_pockets_rigid = holster->all_pockets_rigid();

    const int total_moves = item_move_cost( who, items.front().first, false );
    act.moves_left = total_moves;
    act.moves_total = total_moves;
}

static ret_val<void> try_insert( item_location &holster, drop_location &holstered_item,
                                 int *charges_added, Character *carrier, bool allow_fill_charge_item_nested )
{
    item &it = *holstered_item.first;
    ret_val<void> ret = ret_val<void>::make_failure( _( "item can't be stored there" ) );

    if( holster.get_item()->has_flag( json_flag_NO_RELOAD ) ) {
        return ret_val<void>::make_failure( _( "destination can't be reloaded" ) );
    }

    if( charges_added == nullptr ) {
        if( it.is_bucket_nonempty() ) {
            debugmsg( "ACT_INSERT_ITEM tried to stash a spillable container without spilling its contents first" );
            return ret_val<void>::make_failure( _( "item would spill" ) );
        }
        ret = holster->can_contain_directly( it );
        if( !ret.success() ) {
            return ret;
        }
        ret = holster.parents_can_contain_recursive( &it );
        if( !ret.success() ) {
            if( carrier != nullptr && carrier->is_avatar() && query_yn(
                    _( "The parent container does not have enough space to fit more items.  Would you like to first wield %s to fit more?" ),
                    holster->tname() )
                &&
                // wield the container
                carrier->wield( *holster.get_item() ) ) {
                holster = carrier->get_wielded_item();

                // and recheck if parent can hold (there should be no parent)
                ret = holster.parents_can_contain_recursive( &it );

            }
        }
        if( !ret.success() ) {
            return ret;
        }

        return holster->put_in( it, pocket_type::CONTAINER, /*unseal_pockets=*/true, carrier );
    }

    ret = holster->can_contain_partial_directly( it );
    if( !ret.success() ) {
        return ret;
    }
    ret_val<int> max_parent_charges = holster.max_charges_by_parent_recursive( it );
    if( !max_parent_charges.success() ) {
        return ret_val<void>::make_failure( max_parent_charges.str() );
    } else if( carrier != nullptr && carrier->is_avatar() &&
               max_parent_charges.value() < holstered_item.second ) {
        // if you cannot fit all items because parent container is too small
        if( query_yn(
                _( "The parent container does not have enough space to fit more than %i %s.  Would you like to first wield %s to fit more?" ),
                max_parent_charges.value(), it.tname( max_parent_charges.value() ), holster->tname() )
            &&
            // wield the container
            carrier->wield( *holster.get_item() ) ) {
            holster = carrier->get_wielded_item();

            // and recalc max_parent_charges
            max_parent_charges = holster.max_charges_by_parent_recursive( it );
        }
    }
    int charges_to_insert = std::min( holstered_item.second, max_parent_charges.value() );
    *charges_added = holster->fill_with( it, charges_to_insert, /*unseal_pockets=*/true,
                                         /*allow_sealed=*/true, /*ignore_settings*/true, /*into_bottom*/true, /*allow_nested*/allow_fill_charge_item_nested,
                                         carrier );
    if( *charges_added <= 0 ) {
        return ret_val<void>::make_failure( _( "item can't be stored there" ) );
    }

    return ret;
}

void insert_item_activity_actor::finish( player_activity &act, Character &who )
{
    bool success = false;
    bool bulk_load = false;
    Character *carrier = holster.carrier();
    drop_location &holstered_item = items.front();
    if( holstered_item.first ) {
        success = true;
        item &it = *holstered_item.first;
        // Check if next is bulk load before moving item.
        auto itr = std::next( items.begin() );
        if( itr != items.end() && itr->first ) {
            bulk_load = is_bulk_load( holstered_item.first, itr->first );
        }
        ret_val<void> ret = ret_val<void>::make_failure( _( "item can't be stored there" ) );

        if( !it.count_by_charges() ) {
            ret = try_insert( holster, holstered_item, nullptr, carrier, false );

            if( ret.success() ) {
                //~ %1$s: item to put in the container, %2$s: container to put item in
                who.add_msg_if_player( string_format( _( "You put your %1$s into the %2$s." ),
                                                      holstered_item.first->display_name(), holster->type->nname( 1 ) ) );
                handler.add_unsealed( holster );
                handler.unseal_pocket_containing( holstered_item.first );
                holstered_item.first.remove_item();
            }
        } else {
            int charges_added = 0;
            ret = try_insert( holster, holstered_item, &charges_added, carrier,
                              allow_fill_count_by_charge_item_nested );

            if( ret.success() ) {
                item copy( it );
                copy.charges = charges_added;
                //~ %1$s: item to put in the container, %2$s: container to put item in
                who.add_msg_if_player( string_format( _( "You put your %1$s into the %2$s." ),
                                                      copy.display_name(), holster->type->nname( 1 ) ) );
                handler.add_unsealed( holster );
                handler.unseal_pocket_containing( holstered_item.first );
                it.charges -= charges_added;
                if( it.charges == 0 ) {
                    holstered_item.first.remove_item();
                }
            }
        }

        if( !ret.success() ) {
            success = false;
            std::string error = ret.str().empty() ?
                                //~ %1$s: item we failed to put in the container, %2$s: container to put item in
                                string_format( _( "Could not put %1$s into %2$s." ),
                                               it.tname(), holster->tname() ) :
                                //~ %1$s: item we failed to put in the container, %2$s: container to put item in, %3$s: reason it failed
                                string_format( _( "Could not put %1$s into %2$s, %3$s." ),
                                               it.tname(), holster->tname(), ret.str() );
            who.add_msg_if_player( error );
        }
    } else {
        // item was lost, so just go to next item
        success = true;
    }

    drop_locations items_remain;
    if( who.is_avatar() && reopen_menu && !success ) {
        items_remain.insert( items_remain.end(), items.begin(), items.end() );
    }

    items.pop_front();
    if( items.empty() || !success || items.front().first == item_location::nowhere ) {
        holster.make_active();
        handler.handle_by( who );
        act.set_to_null();
        if( !items_remain.empty() ) {
            g->insert_item( items_remain );
        }
        return;
    }

    // Restart the activity
    const int total_moves = item_move_cost( who, items.front().first, bulk_load );
    act.moves_left = total_moves;
    act.moves_total = total_moves;
}

void insert_item_activity_actor::canceled( player_activity &/*act*/, Character &who )
{
    handler.handle_by( who );
}

void insert_item_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "holster", holster );
    jsout.member( "items", items );
    jsout.member( "handler", handler );
    jsout.member( "all_pockets_rigid", all_pockets_rigid );
    jsout.member( "reopen_menu", reopen_menu );
    jsout.member( "allow_fill_charge_item_nested", allow_fill_count_by_charge_item_nested );

    jsout.end_object();
}

std::unique_ptr<activity_actor> insert_item_activity_actor::deserialize( JsonValue &jsin )
{
    insert_item_activity_actor actor;

    JsonObject data = jsin.get_object();

    data.read( "holster", actor.holster );
    data.read( "items", actor.items );
    data.read( "handler", actor.handler );
    data.read( "all_pockets_rigid", actor.all_pockets_rigid );
    data.read( "reopen_menu", actor.reopen_menu );
    data.read( "allow_fill_charge_item_nested", actor.allow_fill_count_by_charge_item_nested );

    return actor.clone();
}

reload_activity_actor::reload_activity_actor( item::reload_option &&opt, int extra_moves )
{
    moves_total = opt.moves() + extra_moves;
    quantity = opt.qty();
    target_loc = std::move( opt.target );
    ammo_loc = std::move( opt.ammo );
}

bool reload_activity_actor::can_reload() const
{
    if( quantity <= 0 ) {
        debugmsg( "invalid arguments to ACT_RELOAD" );
        return false;
    }

    if( !target_loc ) {
        debugmsg( "reload target is null, failed to reload" );
        return false;
    }

    if( !ammo_loc ) {
        debugmsg( "ammo target is null, failed to reload" );
        return false;
    }

    return true;
}

void reload_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void reload_activity_actor::make_reload_sound( Character &who, item &reloadable )
{
    if( reloadable.type->gun->reload_noise_volume > 0 ) {
        sfx::play_variant_sound( "reload", reloadable.typeId().str(),
                                 sfx::get_heard_volume( who.pos_bub() ) );
        sounds::ambient_sound( who.pos_bub(), reloadable.type->gun->reload_noise_volume,
                               sounds::sound_t::activity, reloadable.type->gun->reload_noise.translated() );
    }
}

void reload_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();

    act.set_to_null();
    if( !can_reload() ) {
        return;
    }

    item &reloadable = *target_loc;
    item &ammo = *ammo_loc;
    const std::string reloadable_name = reloadable.tname();
    // cache check results because reloading deletes the ammo item
    const std::string ammo_name = ammo.tname();
    const bool ammo_is_filthy = ammo.is_filthy();
    const bool ammo_uses_speedloader = ammo.has_flag( flag_SPEEDLOADER );
    const bool ammo_uses_speedloader_clip = ammo.has_flag( flag_SPEEDLOADER_CLIP );

    if( !reloadable.reload( who, std::move( ammo_loc ), quantity ) ) {
        add_msg( m_info, _( "Can't reload the %s." ), reloadable_name );
        return;
    }

    if( ammo_is_filthy ) {
        reloadable.set_flag( flag_FILTHY );
    }

    if( reloadable.get_var( "dirt", 0 ) > 7800 ) {
        add_msg( m_neutral, _( "You manage to loosen some debris and make your %s somewhat operational." ),
                 reloadable_name );
        reloadable.set_var( "dirt", ( reloadable.get_var( "dirt", 0 ) - rng( 790, 2750 ) ) );
    }

    if( reloadable.is_gun() ) {
        if( reloadable.has_flag( flag_RELOAD_ONE ) && !ammo_uses_speedloader &&
            !ammo_uses_speedloader_clip ) {
            add_msg( m_neutral, _( "You insert %dx %s into the %s." ), quantity, ammo_name, reloadable_name );
        }
        make_reload_sound( who, reloadable );
    } else if( reloadable.is_watertight_container() ) {
        add_msg( m_neutral, _( "You refill the %s." ), reloadable_name );
    } else {
        add_msg( m_neutral, _( "You reload the %1$s with %2$s." ), reloadable_name, ammo_name );
    }

    who.recoil = MAX_RECOIL;

    item_location loc = target_loc;
    // Reload may have caused the item to increase in size more than the pocket/location can contain.
    // We want to avoid this because items will be deleted on a save/load.
    if( loc.check_parent_capacity_recursive() ) {
        return;
    }

    // Attempt to put item in another pocket before prompting
    if( who.try_add( reloadable, nullptr, nullptr, false ) != item_location::nowhere ) {
        // try_add copied the old item, so remove it now.
        loc.remove_item();
        return;
    }

    // Build prompt
    uilist reload_query;
    const bool wield_check = who.can_wield( reloadable ).success();
    reload_query.text = string_format( _( "The %s no longer fits in your inventory" ),
                                       reloadable_name );
    if( who.has_wield_conflicts( reloadable ) ) {
        reload_query.addentry( 1, wield_check, 'w',
                               _( "Dispose of %s and wield %s" ), who.get_wielded_item()->display_name(),
                               reloadable_name );
    } else {
        reload_query.addentry( 1, wield_check, 'w', _( "Wield %s" ), reloadable_name );
    }
    reload_query.addentry( 2, true, 'd', _( "Drop %s" ), reloadable_name );
    reload_query.query();

    switch( reload_query.ret ) {
        case 1:
            // This case is only reachable if wield_check is true

            who.wield( target_loc );
            add_msg( m_neutral, _( "The %s no longer fits in your inventory so you wield it instead." ),
                     reloadable_name );
            break;
        case 2:
        default:
            // In player inventory and player is wielding something.
            if( loc.carrier() ) {
                loc.carrier()->add_msg_if_player( m_neutral,
                                                  _( "The %s no longer fits in your inventory so you drop it instead." ),
                                                  reloadable_name );
            }
            here.add_item_or_charges( loc.pos_bub( here ), reloadable );
            loc.remove_item();
            break;
    }
}

void reload_activity_actor::canceled( player_activity &act, Character &/*who*/ )
{
    act.moves_total = 0;
    act.moves_left = 0;
}

void reload_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "qty", quantity );
    jsout.member( "target_loc", target_loc );
    jsout.member( "ammo_loc", ammo_loc );

    jsout.end_object();
}

std::unique_ptr<activity_actor> reload_activity_actor::deserialize( JsonValue &jsin )
{
    reload_activity_actor actor;

    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.moves_total );
    data.read( "qty", actor.quantity );
    data.read( "target_loc", actor.target_loc );
    data.read( "ammo_loc", actor.ammo_loc );
    return actor.clone();
}

void milk_activity_actor::start( player_activity &act, Character &/*who*/ )
{
    act.moves_total = total_moves;
    act.moves_left = total_moves;
    next_unit_move = calendar::turn + time_duration::from_moves( moves_per_unit );
}

void milk_activity_actor::do_turn( player_activity &act, Character &who )
{
    map &here = get_map();
    const tripoint_bub_ms source_pos = here.get_bub( monster_coords );
    monster *source_mon = get_creature_tracker().creature_at<monster>( source_pos );
    if( source_mon == nullptr ) {
        // We might end up here if the creature dies while being milked
        who.add_msg_if_player( m_bad, _( "The udders slip out of your hands." ) );
        act.set_to_null();
        return;
    }

    if( target.dest_opt != LD_NULL ) { // Backward compatibility check 2025-04-29.
        if( calendar::turn >= next_unit_move ) {
            // Add one since we've included that in next_unit_move.
            int units = to_moves<int>( calendar::turn - next_unit_move ) / moves_per_unit + 1;
            next_unit_move += time_duration::from_moves( moves_per_unit );

            auto milked_item = source_mon->ammo.find( source_mon->type->starting_ammo.begin()->first );
            units = std::min( units, milked_item->second );
            item milk( milked_item->first, calendar::turn, units );
            milk.set_item_temperature( units::from_celsius( 38.6 ) );

            if( liquid_handler::handle_liquid( milk, target, nullptr, 1, nullptr, nullptr, -1,
                                               source_mon, true ) ) {
                milked_item->second--;
                if( milked_item->second == 0 ) {
                    who.add_msg_if_player( _( "The %s's udders run dry." ), source_mon->get_name() );
                }
            }
        }
    }
}

void milk_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    map &here = get_map();
    const tripoint_bub_ms source_pos = here.get_bub( monster_coords );
    monster *source_mon = get_creature_tracker().creature_at<monster>( source_pos );
    if( source_mon == nullptr ) {
        debugmsg( "could not find source creature for liquid transfer" );
        return;
    }
    auto milked_item = source_mon->ammo.find( source_mon->type->starting_ammo.begin()->first );
    if( milked_item == source_mon->ammo.end() ) {
        debugmsg( "animal has no milkable ammo type" );
        return;
    }
    if( milked_item->second <= 0 ) {
        debugmsg( "started milking but udders are now empty before milking finishes" );
        return;
    }
    item milk( milked_item->first, calendar::turn, milked_item->second );
    milk.set_item_temperature( units::from_celsius( 38.6 ) );
    if( liquid_handler::handle_liquid( milk, target, nullptr, 1, nullptr, nullptr, -1,
                                       source_mon ) ) {
        milked_item->second = 0;
        if( milk.charges > 0 ) {
            milked_item->second = milk.charges;
        } else {
            who.add_msg_if_player( _( "The %s's udders run dry." ), source_mon->get_name() );
        }
    }
    // if the monster was not manually tied up, but needed to be fixed in place temporarily then
    // remove that now.
    if( milking_tie ) {
        source_mon->remove_effect( effect_tied );
    }
}

void milk_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "total_moves", total_moves );
    jsout.member( "monster_coords", monster_coords );
    jsout.member( "milking_tie", milking_tie );
    jsout.member( "targer_dest_opt", static_cast<int>( target.dest_opt ) );

    switch( target.dest_opt ) {
        case LD_ITEM:
            jsout.member( "target_item_loc", target.item_loc );
            break;

        case LD_KEG:
            jsout.member( "target_pos", target.pos );
            break;

        case LD_NULL:
        case LD_CONSUME:
        case LD_GROUND:
        case LD_VEH:
            break;
    }
    jsout.member( "moves_per_unit", moves_per_unit );
    jsout.member( "next_unit_move", next_unit_move );

    jsout.end_object();
}

std::unique_ptr<activity_actor> milk_activity_actor::deserialize( JsonValue &jsin )
{
    milk_activity_actor actor;
    JsonObject data = jsin.get_object();

    data.read( "moves_total", actor.total_moves );
    data.read( "monster_coords", actor.monster_coords );
    // Backward compatibility introduced 2025-04-29.
    // When removing it, there's no need for the 'if' check.
    if( data.read( "milking_tie", actor.milking_tie ) ) {
        int temp;
        data.read( "target_dest_opt", temp );
        actor.target.dest_opt = static_cast<liquid_dest>( temp );

        switch( actor.target.dest_opt ) {
            case LD_ITEM:
                data.read( "target_item_loc", actor.target.item_loc );
                break;

            case LD_KEG:
                data.read( "target_pos", actor.target.pos );
                break;

            case LD_NULL:
            case LD_CONSUME:
            case LD_GROUND:
            case LD_VEH:
                actor.target.dest_opt = LD_NULL;
                break;
        }

        data.read( "moves_per_unit", actor.moves_per_unit );
        data.read( "next_unit_move", actor.next_unit_move );
    } else {
        std::string temp;
        data.read( "string_values", temp );
        actor.milking_tie = !temp.empty();
        actor.target.dest_opt = LD_NULL;
        actor.moves_per_unit = 3000;
        actor.next_unit_move = calendar::turn;
    }

    return actor.clone();
}

void shearing_activity_actor::start( player_activity &act, Character &who )
{
    monster *mon = get_creature_tracker().creature_at<monster>( mon_coords );
    if( !mon ) {
        debugmsg( "shearing lost monster when starting" );
        act.set_to_null();
        return;
    }

    std::string pet_name_capitalized = mon->unique_name.empty() ? mon->disp_name( false,
                                       true ) : mon->unique_name;

    if( !mon->shearable() ) {
        add_msg( _( "%1$s has nothing %2$s could shear." ), pet_name_capitalized, who.disp_name() );
        if( shearing_tie ) {
            mon->remove_effect( effect_tied );
        }
        act.set_to_null();
        return;
    }

    const int shearing_quality = who.max_quality( qual_SHEAR );
    if( !( shearing_quality > 0 ) ) {
        if( who.is_avatar() ) {
            add_msg( m_info, _( "%1$s don't have a shearing tool." ), who.disp_name( false, true ) );
        } else { // who.is_npc
            // npcs can't shear monsters yet, this is for when they are able to
            add_msg_if_player_sees( who, _( "%1$s doesn't have a shearing tool." ), who.disp_name(),
                                    mon->disp_name() );
        }
        if( shearing_tie ) {
            mon->remove_effect( effect_tied );
        }
        act.set_to_null();
        return;
    }

    const time_duration shearing_time = 30_minutes / shearing_quality;
    add_msg_debug( debugmode::DF_ACT_SHEARING, "shearing_time time = %s",
                   to_string_writable( shearing_time ) );

    if( who.is_avatar() ) {
        add_msg( m_info,
                 _( "%1$s start shearing %2$s." ), who.disp_name( false, true ), mon->disp_name() );
    } else { // who.is_npc
        // npcs can't shear monsters yet, this is for when they are able to
        add_msg_if_player_sees( who, _( "%1$s starts shearing %2$s." ), who.disp_name(),
                                mon->disp_name() );
    }

    act.moves_total = to_moves<int>( shearing_time );
    act.moves_left = act.moves_total;
}

void shearing_activity_actor::do_turn( player_activity &, Character &who )
{
    if( !who.has_quality( qual_SHEAR ) ) {
        if( who.is_avatar() ) {
            add_msg(
                m_bad,
                _( "%1$s don't have a shearing tool anymore." ),
                who.disp_name( false, true ) );
        } else {
            add_msg_if_player_sees(
                who,
                _( "%1$s doesn't have a shearing tool anymore." ),
                who.disp_name() );
        }
        who.cancel_activity();
    }
}

void shearing_activity_actor::finish( player_activity &act, Character &who )
{
    monster *mon = get_creature_tracker().creature_at<monster>( mon_coords );
    if( !mon ) {
        debugmsg( "shearing monster at position disappeared" );
        act.set_to_null();
        return;
    }

    const shearing_data &shear_data = mon->type->shearing;
    if( !shear_data.valid() ) {
        debugmsg( "shearing monster does not have shearing_data" );
        if( shearing_tie ) {
            mon->remove_effect( effect_tied );
        }
        act.set_to_null();
        return;
    }

    map &mp = get_map();

    add_msg_if_player_sees( who,
                            string_format(
                                _( "%1$s finished shearing %2$s and got:" ),
                                who.disp_name( false, true ),
                                mon->unique_name.empty() ? mon->disp_name() : mon->unique_name ) );

    const std::vector<shearing_roll> shear_roll = shear_data.roll_all( *mon );
    for( const shearing_roll &roll : shear_roll ) {
        if( roll.result->count_by_charges() ) {
            item shear_item( roll.result, calendar::turn, roll.amount );
            mp.add_item_or_charges( who.pos_bub(), shear_item );
            add_msg_if_player_sees( who, shear_item.display_name() );
        } else {
            item shear_item( roll.result, calendar::turn );
            for( int i = 0; i < roll.amount; ++i ) {
                mp.add_item_or_charges( who.pos_bub(), shear_item );
            }
            add_msg_if_player_sees( who,
                                    //~ %1$s - item, %2$d - amount
                                    string_format( _( "%1$s x%2$d" ),
                                                   shear_item.display_name(), roll.amount ) );
        }
    }

    mon->add_effect( effect_sheared, calendar::season_length() );
    if( shearing_tie ) {
        mon->remove_effect( effect_tied );
    }

    act.set_to_null();
}

void shearing_activity_actor::canceled( player_activity &, Character & )
{
    monster *mon = get_creature_tracker().creature_at<monster>( mon_coords );
    if( !mon ) {
        return;
    }

    if( shearing_tie ) {
        // free the poor critter
        mon->remove_effect( effect_tied );
    }
}

void shearing_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "mon_coords", mon_coords );
    jsout.member( "shearing_tie", shearing_tie );
    jsout.end_object();
}

std::unique_ptr<activity_actor> shearing_activity_actor::deserialize( JsonValue &jsin )
{
    shearing_activity_actor actor = shearing_activity_actor( {} );

    JsonObject data = jsin.get_object();
    data.read( "mon_coords", actor.mon_coords );
    data.read( "shearing_tie", actor.shearing_tie );
    return actor.clone();
}

static bool check_if_disassemble_okay( item_location target, Character &who )
{
    item *disassembly = target.get_item();

    // item_location::get_item() will return nullptr if the item is lost
    if( !disassembly ) {
        who.add_msg_player_or_npc(
            _( "You no longer have the in progress disassembly in your possession.  "
               "You stop disassembling.  "
               "Reactivate the in progress disassembly to continue disassembling." ),
            _( "<npcname> no longer has the in progress disassembly in their possession.  "
               "<npcname> stops disassembling." ) );
        return false;
    }
    return true;
}

void disassemble_activity_actor::start( player_activity &act, Character &who )
{
    if( act.targets.empty() ) {
        act.set_to_null();
        return;
    }
    if( act.targets.back()->typeId() == itype_disassembly ) {
        act.position = act.targets.back()->get_making_batch_size();
    }
    target = who.create_in_progress_disassembly( act.targets.back() );
    act.targets.pop_back();

    if( !check_if_disassemble_okay( target, who ) ) {
        act.set_to_null();
        return;
    }

    // Mark the item, not available for other characters
    target->set_var( "activity_var", who.name );

    activity_override = target->get_making().exertion_level();

    cached_workbench_multiplier = 0;
    use_cached_workbench_multiplier = false;
}

void disassemble_activity_actor::do_turn( player_activity &act, Character &who )
{
    map &here = get_map();

    if( !check_if_disassemble_okay( target, who ) ) {
        act.set_to_null();
        return;
    }

    // We already checked if this is nullptr above
    item &craft = *target;

    const std::optional<tripoint_bub_ms> location = target.where() == item_location::type::character
            ? std::optional<tripoint_bub_ms>() : std::optional<tripoint_bub_ms>( target.pos_bub( here ) );
    if( !use_cached_workbench_multiplier ) {
        cached_workbench_multiplier = who.workbench_crafting_speed_multiplier( craft, location );
        use_cached_workbench_multiplier = true;
    }
    const float crafting_speed = who.crafting_speed_multiplier( craft, location,
                                 use_cached_workbench_multiplier, cached_workbench_multiplier );

    if( crafting_speed <= 0.0f ) {
        who.cancel_activity();
        return;
    }

    if( moves_total == 0 ) {
        craft.item_counter = 10000000;
    } else {
        const int spent_moves = who.get_moves() * who.exertion_adjusted_move_multiplier( exertion_level() );
        craft.item_counter += std::round( spent_moves * crafting_speed * 10000000.0 / moves_total );
        craft.item_counter = std::min( craft.item_counter, 10000000 );
        who.set_moves( 0 );
    }

    if( craft.item_counter >= 10000000 ) {
        who.complete_disassemble( target );
    }
}

void disassemble_activity_actor::finish( player_activity &act, Character & )
{
    act.set_to_null();
}

void disassemble_activity_actor::canceled( player_activity &, Character & )
{
    if( target.get_item() ) {
        target->erase_var( "activity_var" );
    }
}

float disassemble_activity_actor::exertion_level() const
{
    return activity_override;
}

std::string disassemble_activity_actor::get_progress_message( const player_activity & ) const
{
    if( !target ) {
        // We have somehow lost the disassembly item.  This will be handled in do_turn in the check_if_disassemble_okay call.
        return "";
    }
    return target->tname();
}

void disassemble_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves_total", moves_total );
    jsout.member( "target", target );
    jsout.member( "activity_override", activity_override );

    jsout.end_object();
}

std::unique_ptr<activity_actor> disassemble_activity_actor::deserialize( JsonValue &jsin )
{
    disassemble_activity_actor actor = disassemble_activity_actor( 0 );

    JsonObject data = jsin.get_object();

    data.read( "target", actor.target );
    data.read( "moves_total", actor.moves_total );
    data.read( "activity_override", actor.activity_override );

    return actor.clone();
}

void oxytorch_activity_actor::start( player_activity &act, Character &who )
{
    const map &here = get_map();

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->oxytorch->valid() ) {
            if( !testing ) {
                debugmsg( "%s oxytorch is invalid", furn_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        act.moves_total = to_moves<int>( furn_type->oxytorch->duration() );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->oxytorch->valid() ) {
            if( !testing ) {
                debugmsg( "%s oxytorch is invalid", ter_type.id().str() );
            }
            act.set_to_null();
            return;
        }
        act.moves_total = to_moves<int>( ter_type->oxytorch->duration() );
    } else {
        if( !testing ) {
            debugmsg( "oxytorch activity called on invalid terrain" );
        }
        act.set_to_null();
        return;
    }
    if( tool->ammo_sufficient( &who, act.moves_total / 100 ) ||
        query_yn(
            _( "Your %1$s doesn't have enough charges to complete the job.  Continue anyway?" ), tool->tname()
        ) ||
        test_mode // In the tests, we want to check that the activity can be resumed.
      ) {
        add_msg_debug( debugmode::DF_ACTIVITY, "%s moves_total: %d", act.id().str(), act.moves_total );
        act.moves_left = act.moves_total;
    } else {
        act.set_to_null();
        return;
    }
}

void oxytorch_activity_actor::do_turn( player_activity &/*act*/, Character &who )
{
    map &here = get_map();

    if( tool->ammo_sufficient( &who ) ) {
        tool->ammo_consume( tool->ammo_required(), here, tool.pos_bub( here ), &who );
        sfx::play_activity_sound( "tool", "oxytorch", sfx::get_heard_volume( target ) );
        if( calendar::once_every( 2_turns ) ) {
            sounds::sound( target, 10, sounds::sound_t::destructive_activity, _( "hissssssssss!" ) );
        }
    } else {
        if( who.is_avatar() ) {
            who.add_msg_if_player( m_bad, _( "Your %1$s ran out of charges." ), tool->tname() );
        } else { // who.is_npc()
            add_msg_if_player_sees( who.pos_bub(), _( "%1$s %2$s ran out of charges." ), who.disp_name( false,
                                    true ), tool->tname() );
        }
        who.cancel_activity();
    }
}

void oxytorch_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    const activity_data_common *data;

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->oxytorch->valid() ) {
            if( !testing ) {
                debugmsg( "%s oxytorch is invalid", furn_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        const furn_str_id new_furn = furn_type->oxytorch->result();
        if( !new_furn.is_valid() ) {
            if( !testing ) {
                debugmsg( "oxytorch furniture: %s invalid furniture", new_furn.str() );
            }
            act.set_to_null();
            return;
        }

        data = static_cast<const activity_data_common *>( &*furn_type->oxytorch );
        here.furn_set( target, new_furn );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->oxytorch->valid() ) {
            if( !testing ) {
                debugmsg( "%s oxytorch is invalid", ter_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        const ter_str_id new_ter = ter_type->oxytorch->result();
        if( !new_ter.is_valid() ) {
            if( !testing ) {
                debugmsg( "oxytorch terrain: %s invalid terrain", new_ter.str() );
            }
            act.set_to_null();
            return;
        }

        data = static_cast<const activity_data_common *>( &*ter_type->oxytorch );
        here.ter_set( target, new_ter );
    } else {
        if( !testing ) {
            debugmsg( "oxytorch activity finished on invalid terrain" );
        }
        act.set_to_null();
        return;
    }

    for( const activity_byproduct &byproduct : data->byproducts() ) {
        const int amount = byproduct.roll();
        if( byproduct.item->count_by_charges() ) {
            item byproduct_item( byproduct.item, calendar::turn, amount );
            here.add_item_or_charges( target, byproduct_item );
        } else {
            item byproduct_item( byproduct.item, calendar::turn );
            for( int i = 0; i < amount; ++i ) {
                here.add_item_or_charges( target, byproduct_item );
            }
        }
    }

    // 50% chance of starting a fire.
    if( one_in( 2 ) && here.flammable_items_at( target ) ) {
        here.add_field( target, fd_fire, 1, 10_minutes );
    }

    if( !data->message().empty() ) {
        who.add_msg_if_player( m_info, data->message().translated() );
    }

    act.set_to_null();
}

void oxytorch_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "target", target );
    jsout.member( "tool", tool );
    jsout.end_object();
}

std::unique_ptr<activity_actor> oxytorch_activity_actor::deserialize( JsonValue &jsin )
{
    oxytorch_activity_actor actor( {}, {} );
    JsonObject data = jsin.get_object();
    data.read( "target", actor.target );
    data.read( "tool", actor.tool );
    return actor.clone();
}

void tent_placement_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void tent_placement_activity_actor::finish( player_activity &act, Character &p )
{
    map &here = get_map();
    const tripoint_bub_ms center = p.pos_bub() + point( ( radius + 1 ) * target.x(),
                                   ( radius + 1 ) * target.y() );

    // Make a square of floor surrounded by wall.
    for( const tripoint_bub_ms &dest : here.points_in_radius( center, radius ) ) {
        here.furn_set( dest, wall );
    }
    for( const tripoint_bub_ms &dest : here.points_in_radius( center, radius - 1 ) ) {
        here.furn_set( dest, floor );
    }
    // Place the center floor and the door.
    if( floor_center ) {
        here.furn_set( center, *floor_center );
    }
    here.furn_set( p.pos_bub() + target, door_closed );

    add_msg( m_info, _( "You set up the %s on the ground." ), it.tname() );
    add_msg( m_info, _( "Examine the center square to pack it up again." ) );
    act.set_to_null();
}

void tent_placement_activity_actor::canceled( player_activity &, Character &p )
{
    map &here = get_map();
    here.add_item_or_charges( p.pos_bub() + target, it, true );
}

void tent_placement_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "wall", wall );
    jsout.member( "floor", floor );
    jsout.member( "floor_center", floor_center );
    jsout.member( "door_closed", door_closed );
    jsout.member( "radius", radius );
    jsout.member( "it", it );
    jsout.member( "target", target );
    jsout.end_object();
}

std::unique_ptr<activity_actor> tent_placement_activity_actor::deserialize( JsonValue &jsin )
{
    item it;
    tent_placement_activity_actor actor( 0, {}, 0, it, {}, {}, {}, {} );
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "wall", actor.wall );
    data.read( "floor", actor.floor );
    data.read( "floor_center", actor.floor_center );
    data.read( "door_closed", actor.door_closed );
    data.read( "radius", actor.radius );
    data.read( "it", actor.it );
    data.read( "target", actor.target );
    return actor.clone();
}

void tent_deconstruct_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void tent_deconstruct_activity_actor::finish( player_activity &act, Character & )
{
    map &here = get_map();
    for( const tripoint_bub_ms &pt : here.points_in_radius( target, radius ) ) {
        here.furn_set( pt, furn_str_id::NULL_ID() );
    }
    here.add_item_or_charges( target, item( tent, calendar::turn ) );
    act.set_to_null();
}

void tent_deconstruct_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "radius", radius );
    jsout.member( "target", target );
    jsout.member( "tent", tent );
    jsout.end_object();
}

std::unique_ptr<activity_actor> tent_deconstruct_activity_actor::deserialize( JsonValue &jsin )
{
    tent_deconstruct_activity_actor actor( 0, 0, {}, {} );
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "radius", actor.radius );
    data.read( "target", actor.target );
    data.read( "tent", actor.tent );

    return actor.clone();
}

void reel_cable_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves_total;
    act.moves_left = moves_total;
}

void reel_cable_activity_actor::finish( player_activity &act, Character &who )
{
    cable->reset_link( false, &who, -2 );
    who.add_msg_if_player( m_info, _( "You reel in the %s and wind it up." ), cable->link_name() );
    if( cable->has_flag( flag_NO_DROP ) ) {
        cable.remove_item();
    }
    act.set_to_null();
}

void reel_cable_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "moves_total", moves_total );
    jsout.member( "cable", cable );
    jsout.end_object();
}

std::unique_ptr<activity_actor> reel_cable_activity_actor::deserialize( JsonValue &jsin )
{
    reel_cable_activity_actor actor( 0, {} );
    JsonObject data = jsin.get_object();
    data.read( "moves_total", actor.moves_total );
    data.read( "cable", actor.cable );

    return actor.clone();
}

void outfit_swap_actor::start( player_activity &act, Character &who )
{
    if( !who.is_wielding( *outfit_item ) && !who.wield( *outfit_item ) ) {
        who.add_msg_if_player( m_warning,
                               _( "You'll need the outfit in your hands before you can change outfits." ) );
        act.set_to_null();
        return;
    }
    if( !outfit_item ) { // if we wielded it in the last if, it's invalidated and reacquired here
        outfit_item = who.get_wielded_item();
    }
    item fake_storage( outfit_item->typeId() );
    for( const item_location &worn_item : who.get_visible_worn_items() ) {
        act.moves_total += who.item_handling_cost( *worn_item ); // Cost of taking it off
        act.moves_total += who.item_store_cost( *worn_item, fake_storage ); // And putting it away
    }
    for( const item *clothing : outfit_item->all_items_top() ) {
        auto ret = who.can_wear( *clothing );
        // Note this is checking if we can put something new on, but it might conflict with our current clothing, causing a
        // spurious failure. Maybe we should strip the player first?
        if( ret.success() ) {
            act.moves_total += who.item_wear_cost( *clothing );
        } else {
            act.moves_total += who.item_retrieve_cost( *clothing, *outfit_item );
            // Dropping takes no time? So I guess that's all we need
        }
    }
    act.moves_left = act.moves_total;
}

void outfit_swap_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    // First, make a new outfit and shove all our existing clothes into it.
    item new_outfit( outfit_item->typeId() );
    item_location ground = here.add_item_or_charges_ret_loc( who.pos_bub(), new_outfit, true );
    if( !ground ) {
        debugmsg( "Failed to swap outfits during outfit_swap_actor::finish" );
        act.set_to_null();
        return;
    }
    // Taken-off items are put in this temporary list, then naturally deleted from the world when the function returns.
    std::list<item> it_list;
    for( item_location &worn_item : who.get_visible_worn_items() ) {
        if( !static_cast<bool>( worn_item ) ) {
            //Due to the eoc triggered who.takeoff, the item may become invalid.
            continue;
        }
        item outfit_component( *worn_item );
        std::size_t old_it_list_size = it_list.size();
        if( who.takeoff( worn_item, &it_list ) && it_list.size() > old_it_list_size ) {
            ground->force_insert_item( outfit_component, pocket_type::CONTAINER );
        }
    }

    // Now we have to take clothes out of the one we activated
    for( item *component : outfit_item->all_items_top() ) {
        auto ret = who.can_wear( *component );
        if( ret.success() ) {
            item_location new_clothes( who, component );
            who.wear( new_clothes );
        } else {
            // For some reason we couldn't wear this item. Maybe the player mutated in the meanwhile, but
            // drop the item instead of deleting it.
            here.add_item( who.pos_bub(), *component );
        }
    }

    who.i_rem( outfit_item.get_item() );

    // Now we just did a whole bunch of wearing and taking off at once, but we had already paid that movecost by doing the activity
    // So we reset our moves
    who.set_moves( 0 );
    // TODO: Granularize this and allow resumable swapping if you were interrupted during the activity

    act.set_to_null();
}

void outfit_swap_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "outfit_item", outfit_item );
    jsout.end_object();
}

std::unique_ptr<activity_actor> outfit_swap_actor::deserialize( JsonValue &jsin )
{
    outfit_swap_actor actor( {} );

    JsonObject data = jsin.get_object();

    data.read( "outfit_item", actor.outfit_item );
    return actor.clone();
}

void meditate_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 20_minutes );
    act.moves_left = act.moves_total;
}

void meditate_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( m_good, _( "You pause to engage in spiritual contemplation." ) );
    who.add_morale( morale_feeling_good, 5, 10 );
    act.set_to_null();
}

void meditate_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> meditate_activity_actor::deserialize( JsonValue & )
{
    return meditate_activity_actor().clone();
}

void play_with_pet_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = rng( 50, 125 ) * 100;
    act.moves_left = act.moves_total;
}

void play_with_pet_activity_actor::finish( player_activity &act, Character &who )
{
    if( !playstr.empty() ) {
        who.add_msg_if_player( m_good, playstr, pet_name );
    }

    if( !who.has_trait( trait_PSYCHOPATH ) && !who.has_trait( trait_NUMB ) ) {
        who.add_morale( morale_play_with_pet, rng( 3, 10 ), 10, 5_hours, 25_minutes );
        who.add_msg_if_player( m_good, _( "Playing with your %s has lifted your spirits a bit." ),
                               pet_name );
    } else {
        who.add_msg_if_player( _( "Your %s seems to enjoy the interaction, but you feel nothing." ),
                               pet_name );
    }
    act.set_to_null();
}

void play_with_pet_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "pet_name", pet_name );
    jsout.member( "playstr", playstr );
    jsout.end_object();
}

std::unique_ptr<activity_actor> play_with_pet_activity_actor::deserialize( JsonValue &jsin )
{
    play_with_pet_activity_actor actor = play_with_pet_activity_actor();
    JsonObject data = jsin.get_object();
    data.read( "pet_name", actor.pet_name );
    data.read( "playstr", actor.playstr );
    return actor.clone();
}

time_duration prying_activity_actor::prying_time( const activity_data_common &data,
        const item_location &tool, const Character &who )
{
    const pry_data &pdata = data.prying_data();
    if( data.duration() > 1_seconds ) {
        return data.duration();
    }

    int difficulty = pdata.difficulty;
    difficulty -= tool->get_quality( qual_PRY ) - pdata.prying_level;
    return time_duration::from_seconds(
               /** @ARM_STR speeds up crowbar prying attempts */
               std::max( 5, 2 * ( 4 * difficulty - who.get_arm_str() ) ) );
}

void prying_activity_actor::start( player_activity &act, Character &who )
{
    const map &here = get_map();

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->prying->valid() ) {
            if( !testing ) {
                debugmsg( "%s prying is invalid", furn_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        prying_nails = furn_type->prying->prying_data().prying_nails;
        act.moves_total = to_moves<int>(
                              prying_time( *furn_type->prying, tool, who ) );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->prying->valid() ) {
            if( !testing ) {
                debugmsg( "%s prying is invalid", ter_type.id().str() );
            }
            act.set_to_null();
            return;
        }

        prying_nails = ter_type->prying->prying_data().prying_nails;
        act.moves_total = to_moves<int>(
                              prying_time( *ter_type->prying, tool, who ) );
    } else {
        if( !testing ) {
            debugmsg( "prying activity called on invalid terrain" );
        }
        act.set_to_null();
        return;
    }

    if( prying_nails && !tool->has_quality( qual_PRYING_NAIL ) ) {
        who.add_msg_if_player( _( "You can't use your %1$s to pry up the nails." ), tool->tname() );
        act.set_to_null();
        return;
    }

    add_msg_debug( debugmode::DF_ACTIVITY, "%s moves_total: %d", act.id().str(), act.moves_total );
    act.moves_left = act.moves_total;
}

void prying_activity_actor::do_turn( player_activity &/*act*/, Character &who )
{
    map &here = get_map();

    std::string method = "CROWBAR";

    if( tool->ammo_sufficient( &who, method ) ) {
        int ammo_consumed = tool->ammo_required();
        std::map<std::string, int>::const_iterator iter = tool->type->ammo_scale.find( method );
        if( iter != tool->type->ammo_scale.end() ) {
            ammo_consumed *= iter->second;
        }
        tool->ammo_consume( ammo_consumed, here, tool.pos_bub( here ), &who );
        if( prying_nails ) {
            sfx::play_activity_sound( "tool", "hammer", sfx::get_heard_volume( target ) );
        }
    } else {
        if( who.is_avatar() ) {
            who.add_msg_if_player( m_bad, _( "Your %1$s ran out of charges." ), tool->tname() );
        } else { // who.is_npc()
            add_msg_if_player_sees( who.pos_bub(), _( "%1$s %2$s ran out of charges." ), who.disp_name( false,
                                    true ), tool->tname() );
        }
        who.cancel_activity();
    }
}

void prying_activity_actor::handle_prying( Character &who )
{
    map &here = get_map();
    const activity_data_common *data;

    // check if Character passed the difficulty check
    auto difficulty_check = [&]( const activity_data_common & data ) -> bool {
        const pry_data &pdata = data.prying_data();

        int difficulty = pdata.difficulty;
        difficulty -= tool->get_quality( qual_PRY ) - pdata.prying_level;

        /** @ARM_STR increases chance of prying success */
        const int dice_str = dice( 4, who.get_arm_str() );

        return dice( 4, difficulty ) < dice_str;
    };

    auto handle_failure = [&]( const pry_data & pdata ) -> void {
        if( pdata.breakable )
        {
            int difficulty = pdata.difficulty;
            difficulty -= tool->get_quality( qual_PRY ) - pdata.prying_level;

            /** @EFFECT_MECHANICS reduces chance of breaking when prying */
            const int dice_mech = dice( 2,
                                        static_cast<int>( round( who.get_skill_level( skill_mechanics ) ) ) );
            /** @ARM_STR reduces chance of breaking when prying */
            const int dice_str = dice( 2, who.get_arm_str() );

            if( dice( 4, difficulty ) > dice_mech + dice_str ) {
                // bash will always succeed
                here.bash( target, 0, false, true );
            }
        }

        if( !data->prying_data().failure.empty() )
        {
            who.add_msg_if_player( m_info, data->prying_data().failure.translated() );
        }
    };

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->prying->valid() ) {
            if( !testing ) {
                debugmsg( "%s prying is invalid", furn_type.id().str() );
            }
            return;
        }

        data = static_cast<const activity_data_common *>( &*furn_type->prying );
        if( !difficulty_check( *furn_type->prying ) ) {
            handle_failure( furn_type->prying->prying_data() );
            return;
        }

        const furn_str_id new_furn = furn_type->prying->result();
        if( !new_furn.is_valid() ) {
            if( !testing ) {
                debugmsg( "prying furniture: %s invalid furniture", new_furn.str() );
            }
            return;
        }

        here.furn_set( target, new_furn );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->prying->valid() ) {
            if( !testing ) {
                debugmsg( "%s prying is invalid", ter_type.id().str() );
            }
            return;
        }

        data = static_cast<const activity_data_common *>( &*ter_type->prying );
        if( !difficulty_check( *ter_type->prying ) ) {
            handle_failure( ter_type->prying->prying_data() );
            return;
        }

        const ter_str_id new_ter = ter_type->prying->result();
        if( !new_ter.is_valid() ) {
            if( !testing ) {
                debugmsg( "prying terrain: %s invalid terrain", new_ter.str() );
            }
            return;
        }

        here.ter_set( target, new_ter );
    } else {
        if( !testing ) {
            debugmsg( "prying activity finished on invalid terrain" );
        }
        return;
    }

    for( const activity_byproduct &byproduct : data->byproducts() ) {
        const int amount = byproduct.roll();
        if( byproduct.item->count_by_charges() ) {
            item byproduct_item( byproduct.item, calendar::turn, amount );
            here.add_item_or_charges( target, byproduct_item );
        } else {
            item byproduct_item( byproduct.item, calendar::turn );
            for( int i = 0; i < amount; ++i ) {
                here.add_item_or_charges( target, byproduct_item );
            }
        }
    }

    const pry_data &pdata = data->prying_data();
    if( pdata.noisy ) {
        sounds::sound( target, 12, sounds::sound_t::combat, _( "crunch!" ), true, "tool", "crowbar" );
    }

    if( pdata.alarm ) {
        get_event_bus().send<event_type::triggers_alarm>( who.getID() );
        sounds::sound( who.pos_bub(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), true,
                       "environment",
                       "alarm" );
    }

    if( !data->message().empty() ) {
        who.add_msg_if_player( m_info, data->message().translated() );
    }
}

void prying_activity_actor::handle_prying_nails( Character &who )
{
    map &here = get_map();
    const activity_data_common *data;

    if( here.has_furn( target ) ) {
        const furn_id &furn_type = here.furn( target );
        if( !furn_type->prying->valid() ) {
            if( !testing ) {
                debugmsg( "%s prying is invalid", furn_type.id().str() );
            }
            return;
        }

        const furn_str_id new_furn = furn_type->prying->result();
        if( !new_furn.is_valid() ) {
            if( !testing ) {
                debugmsg( "prying furniture: %s invalid furniture", new_furn.str() );
            }
            return;
        }

        data = static_cast<const activity_data_common *>( &*furn_type->prying );
        here.furn_set( target, new_furn );
    } else if( const ter_id &ter_type = here.ter( target ); !ter_type->is_null() ) {
        if( !ter_type->prying->valid() ) {
            if( !testing ) {
                debugmsg( "%s prying is invalid", ter_type.id().str() );
            }
            return;
        }

        const ter_str_id new_ter = ter_type->prying->result();
        if( !new_ter.is_valid() ) {
            if( !testing ) {
                debugmsg( "prying terrain: %s invalid terrain", new_ter.str() );
            }
            return;
        }

        data = static_cast<const activity_data_common *>( &*ter_type->prying );
        here.ter_set( target, new_ter );
    } else {
        if( !testing ) {
            debugmsg( "prying nails activity finished on invalid terrain" );
        }
        return;
    }

    for( const activity_byproduct &byproduct : data->byproducts() ) {
        const int amount = byproduct.roll();
        if( byproduct.item->count_by_charges() ) {
            item byproduct_item( byproduct.item, calendar::turn, amount );
            here.add_item_or_charges( target, byproduct_item );
        } else {
            item byproduct_item( byproduct.item, calendar::turn );
            for( int i = 0; i < amount; ++i ) {
                here.add_item_or_charges( target, byproduct_item );
            }
        }
    }

    if( !data->message().empty() ) {
        who.add_msg_if_player( m_info, data->message().translated() );
    }

    who.practice( skill_fabrication, 1, 1 );
}

void prying_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();

    if( prying_nails ) {
        handle_prying_nails( who );
    } else {
        handle_prying( who );
    }
}

void prying_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "target", target );
    jsout.member( "tool", tool );
    jsout.member( "prying_nails", prying_nails );
    jsout.end_object();
}

std::unique_ptr<activity_actor> prying_activity_actor::deserialize( JsonValue &jsin )
{
    prying_activity_actor actor( {}, {} );
    JsonObject data = jsin.get_object();
    data.read( "target", actor.target );
    data.read( "tool", actor.tool );
    data.read( "prying_nails", actor.prying_nails );
    return actor.clone();
}

void shave_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 5_minutes );
    act.moves_left = act.moves_total;
}

void shave_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( _( "You open up your kit and shave." ) );
    who.add_morale( morale_shave, 8, 8, 240_minutes, 3_minutes );
    act.set_to_null();
}

void shave_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> shave_activity_actor::deserialize( JsonValue & )
{
    return shave_activity_actor().clone();
}

void haircut_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = to_moves<int>( 30_minutes );
    act.moves_left = act.moves_total;
}

void haircut_activity_actor::finish( player_activity &act, Character &who )
{
    who.add_msg_if_player( _( "You give your hair a trim." ) );
    who.add_morale( morale_haircut, 3, 3, 480_minutes, 3_minutes );
    if( who.is_avatar() ) {
        uilist amenu;
        amenu.title = _( "Change what?" );
        amenu.addentry( 0, true, MENU_AUTOASSIGN, _( "Change hairstyle" ) );
        amenu.addentry( 1, true, MENU_AUTOASSIGN, _( "Change facial hairstyle" ) );

        amenu.query();
        if( amenu.ret == 0 ) {
            who.customize_appearance( customize_appearance_choice::HAIR );
        } else if( amenu.ret == 1 ) {
            who.customize_appearance( customize_appearance_choice::HAIR_F );
        }
    }
    act.set_to_null();
}

void haircut_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> haircut_activity_actor::deserialize( JsonValue & )
{
    return haircut_activity_actor().clone();
}

static bool check_stealing( Character &who, item &it )
{
    if( !it.is_owned_by( who, true ) ) {
        // Has the player given input on if stealing is ok?
        if( who.get_value( "THIEF_MODE" ).str() == "THIEF_ASK" ) {
            Pickup::query_thief();
        }
        if( who.get_value( "THIEF_MODE" ).str() == "THIEF_HONEST" ) {
            if( who.get_value( "THIEF_MODE_KEEP" ).str() != "YES" ) {
                who.set_value( "THIEF_MODE", "THIEF_ASK" );
            }
            return false;
        }
    }

    return true;
}

bool avatar_action::check_stealing( Character &who, item &weapon )
{
    return ::check_stealing( who, weapon );
}

void wield_activity_actor::do_turn( player_activity &, Character &who )
{
    if( who.get_moves() > 0 ) {
        if( target_item ) {
            // Make copies so the original remains untouched if wielding fails
            item newit = *target_item;
            item leftovers = newit;

            // Handle charges, quantity == 0 means move all
            if( quantity != 0 && newit.count_by_charges() ) {
                leftovers.charges = newit.charges - quantity;
                if( leftovers.charges > 0 ) {
                    newit.charges = quantity;
                }
            } else {
                leftovers.charges = 0;
            }

            if( check_stealing( who, newit ) ) {
                handler.unseal_pocket_containing( target_item );
                if( who.wield( newit ) ) {
                    // If we wielded up a whole stack, remove the original item
                    // Otherwise, replace the item with the leftovers
                    if( leftovers.charges > 0 ) {
                        *target_item = std::move( leftovers );
                    } else {
                        target_item.remove_item();
                    }
                }
                handler.handle_by( who );
            }
        }

        who.cancel_activity();
    }
}

void wield_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "target_items", target_item );
    jsout.member( "quantities", quantity );
    jsout.member( "handler", handler );

    jsout.end_object();
}

std::unique_ptr<activity_actor> wield_activity_actor::deserialize( JsonValue &jsin )
{
    wield_activity_actor actor( {}, {} );

    JsonObject data = jsin.get_object();

    data.read( "target_items", actor.target_item );
    data.read( "quantities", actor.quantity );
    data.read( "handler", actor.handler );

    return actor.clone();
}

void wear_activity_actor::do_turn( player_activity &, Character &who )
{
    // ACT_WEAR has item_location targets, and int quantities
    while( who.get_moves() > 0 && !target_items.empty() ) {
        item_location target = std::move( target_items.back() );
        int quantity = quantities.back();
        target_items.pop_back();
        quantities.pop_back();

        if( !target ) {
            debugmsg( "Lost target item of ACT_WEAR" );
            continue;
        }

        // Make copies so the original remains untouched if wearing fails
        item newit = *target;
        item leftovers = newit;

        // Handle charges, quantity == 0 means move all
        if( quantity != 0 && newit.count_by_charges() ) {
            leftovers.charges = newit.charges - quantity;
            if( leftovers.charges > 0 ) {
                newit.charges = quantity;
            }
        } else {
            leftovers.charges = 0;
        }

        if( check_stealing( who, newit ) ) {
            handler.unseal_pocket_containing( target );
            if( who.wear_item( newit ) ) {
                // If we wore up a whole stack, remove the original item
                // Otherwise, replace the item with the leftovers
                if( leftovers.charges > 0 ) {
                    *target = std::move( leftovers );
                } else {
                    target.remove_item();
                }
            }
        }
    }

    // If there are no items left we are done
    if( target_items.empty() ) {
        handler.handle_by( who );
        who.cancel_activity();
    }
}

void wear_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "target_items", target_items );
    jsout.member( "quantities", quantities );
    jsout.member( "handler", handler );

    jsout.end_object();
}

std::unique_ptr<activity_actor> wear_activity_actor::deserialize( JsonValue &jsin )
{
    wear_activity_actor actor( {}, {} );

    JsonObject data = jsin.get_object();

    data.read( "target_items", actor.target_items );
    data.read( "quantities", actor.quantities );
    data.read( "handler", actor.handler );

    return actor.clone();
}

void invoke_item_activity_actor::do_turn( player_activity &, Character &who )
{
    item_location _item = item;
    std::string _method = method;
    if( method.empty() ) {
        who.cancel_activity();
        who.invoke_item( _item.get_item() );
        return;
    }
    who.cancel_activity();
    who.invoke_item( _item.get_item(), _method );
}

void invoke_item_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "item", item );
    jsout.member( "method", method );

    jsout.end_object();
}

std::unique_ptr<activity_actor> invoke_item_activity_actor::deserialize( JsonValue &jsin )
{
    invoke_item_activity_actor actor( {}, {} );

    JsonObject data = jsin.get_object();

    data.read( "item", actor.item );
    data.read( "method", actor.method );

    return actor.clone();
}

static void chop_single_do_turn( player_activity &act )
{
    const map &here = get_map();
    sfx::play_activity_sound( "tool", "axe",
                              sfx::get_heard_volume( here.get_bub( act.placement ) ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a wood chopping tool at work!
        sounds::sound( here.get_bub( act.placement ), 15, sounds::sound_t::activity, _( "CHK!" ) );
    }
}

void chop_logs_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
}

void chop_logs_activity_actor::do_turn( player_activity &act, Character & )
{
    chop_single_do_turn( act );
}

void chop_logs_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    const tripoint_bub_ms &pos = here.get_bub( act.placement );
    int log_quan;
    int stick_quan;
    int splint_quan;
    const ter_id &t = here.ter( pos );
    if( t == ter_t_trunk ) {
        log_quan = rng( 2, 3 );
        stick_quan = rng( 0, 3 );
        splint_quan = 0;
    } else if( t == ter_t_stump ) {
        log_quan = rng( 0, 2 );
        stick_quan = 0;
        splint_quan = rng( 5, 15 );
    } else {
        log_quan = 0;
        stick_quan = 0;
        splint_quan = 0;
    }
    for( int i = 0; i != log_quan; ++i ) {
        item obj( itype_log, calendar::turn );
        obj.set_var( "activity_var", who.name );
        //The item may exceed the capacity of the pos and move to another coordinate.So get loc.
        item_location loc = here.add_item_or_charges_ret_loc( pos, obj );
        if( loc ) {
            who.may_activity_occupancy_after_end_items_loc.push_back( loc );
        }

    }
    for( int i = 0; i != stick_quan; ++i ) {
        item obj( itype_stick_long, calendar::turn );
        obj.set_var( "activity_var", who.name );
        //The item may exceed the capacity of the pos and move to another coordinate.So get loc.
        item_location loc = here.add_item_or_charges_ret_loc( pos, obj );
        if( loc ) {
            who.may_activity_occupancy_after_end_items_loc.push_back( loc );
        }
    }
    for( int i = 0; i != splint_quan; ++i ) {
        item obj( itype_splinter, calendar::turn );
        obj.set_var( "activity_var", who.name );
        //The item may exceed the capacity of the pos and move to another coordinate.So get loc.
        item_location loc = here.add_item_or_charges_ret_loc( pos, obj );
        if( loc ) {
            who.may_activity_occupancy_after_end_items_loc.push_back( loc );
        }
    }
    here.ter_set( pos, ter_t_dirt );
    who.add_msg_if_player( m_good, _( "You finish chopping wood." ) );

    act.set_to_null();
    activity_handlers::resume_for_multi_activities( who );
}

void chop_logs_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );
    jsout.member( "tool", tool );

    jsout.end_object();
}

std::unique_ptr<activity_actor> chop_logs_activity_actor::deserialize( JsonValue &jsin )
{
    chop_logs_activity_actor actor( {}, {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );
    data.read( "tool", actor.tool );

    return actor.clone();
}

void chop_planks_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
}

void chop_planks_activity_actor::do_turn( player_activity &act, Character & )
{
    chop_single_do_turn( act );
}

void chop_planks_activity_actor::finish( player_activity &act, Character &who )
{
    const int max_planks = 10;
    /** @EFFECT_FABRICATION increases number of planks cut from a log */
    int planks = normal_roll( 2 + who.get_skill_level( skill_fabrication ), 1 );
    int wasted_planks = max_planks - planks;
    int scraps = rng( wasted_planks, wasted_planks * 3 );
    planks = std::min( planks, max_planks );

    map &here = get_map();
    if( planks > 0 ) {
        here.spawn_item( here.get_bub( act.placement ), itype_2x4, planks, 0, calendar::turn );
        who.add_msg_if_player( m_good, n_gettext( "You produce %d plank.", "You produce %d planks.",
                               planks ), planks );
    }
    if( scraps > 0 ) {
        here.spawn_item( here.get_bub( act.placement ), itype_splinter, scraps, 0, calendar::turn );
        who.add_msg_if_player( m_good, n_gettext( "You produce %d splinter.", "You produce %d splinters.",
                               scraps ), scraps );
    }
    if( planks < max_planks / 2 ) {
        who.add_msg_if_player( m_bad, _( "You waste a lot of the wood." ) );
    }
    act.set_to_null();
    activity_handlers::resume_for_multi_activities( who );
}

void chop_planks_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );

    jsout.end_object();
}

std::unique_ptr<activity_actor> chop_planks_activity_actor::deserialize( JsonValue &jsin )
{
    chop_planks_activity_actor actor( {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );

    return actor.clone();
}

void chop_tree_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
}

void chop_tree_activity_actor::do_turn( player_activity &act, Character & )
{
    chop_single_do_turn( act );
}

void chop_tree_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    const tripoint_bub_ms &pos = here.get_bub( act.placement );

    tripoint_rel_ms direction;
    if( !who.is_npc() &&
        ( who.backlog.empty() || who.backlog.front().id() != ACT_MULTIPLE_CHOP_TREES ) ) {
        while( true ) {
            if( const std::optional<tripoint_rel_ms> dir = choose_direction(
                        _( "Select a direction for the tree to fall in." ) ) ) {
                direction = *dir;
                break;
            }
            // try again
        }
    } else {
        creature_tracker &creatures = get_creature_tracker();
        const point_rel_ms main_dir = pos.xy() - who.pos_bub().xy();
        static const int circle_size = 8;
        static constexpr const std::array<point, circle_size> circle = {
            point::east, point::south_east, point::south, point::south_west, point::west,
            point::north_west, point::north, point::north_east
        };
        int circle_center = 0;  //  Initialized as the compiler complained
        for( int i = 0; i < circle_size; i++ ) {
            if( main_dir.raw() == circle[i] ) {
                circle_center = i;
                break;
            }
        }
        std::vector<point> candidates;
        candidates = { main_dir.raw(),                                  // Fall straight away
                       circle[( circle_center + circle_size - 1 ) % circle_size], // Fall away to the left. Adding the full circle to ensure modulo gives a non negative result
                       circle[( circle_center + 1 ) % circle_size],              // Fall away to the right
                       circle[( circle_center + circle_size - 2 ) % circle_size], // Fall to the left
                       circle[( circle_center + 2 ) % circle_size],              // Fall to the right
                       circle[( circle_center + circle_size - 3 ) % circle_size], // Fall towards to the left
                       circle[( circle_center + 3 ) % circle_size]
                     };              // Fall towards to the right
        for( const point &direc : candidates ) {
            bool cantuse = false;
            tripoint_bub_ms proposed_to = pos + point( 3 * direc.x, 3 * direc.y );
            std::vector<tripoint_bub_ms> rough_tree_line = line_to( pos, proposed_to );
            for( const tripoint_bub_ms &elem : rough_tree_line ) {
                if( creatures.creature_at( elem ) ) {
                    cantuse = true;
                    break;
                }
            }
            if( !cantuse ) {
                direction = tripoint_rel_ms( { direc, pos.z() } );
                break;
            }
        }
    }

    here.cut_down_tree( pos, direction.xy() );

    who.add_msg_if_player( m_good, _( "You finish chopping down a tree." ) );
    // sound of falling tree
    here.collapse_at( pos, false, true, false );
    sfx::play_variant_sound( "misc", "timber",
                             sfx::get_heard_volume( here.get_bub( act.placement ) ) );
    get_event_bus().send<event_type::cuts_tree>( who.getID() );
    act.set_to_null();
    activity_handlers::resume_for_multi_activities( who );
}

void chop_tree_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );
    jsout.member( "tool", tool );

    jsout.end_object();
}

std::unique_ptr<activity_actor> chop_tree_activity_actor::deserialize( JsonValue &jsin )
{
    chop_tree_activity_actor actor( {}, {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );
    data.read( "tool", actor.tool );

    return actor.clone();
}

void churn_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
}

void churn_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    who.add_msg_if_player( _( "You finish churning up the earth here." ) );
    here.ter_set( here.get_bub( act.placement ), ter_t_dirtmound );
    // Go back to what we were doing before
    // could be player zone activity, or could be NPC multi-farming
    act.set_to_null();
    activity_handlers::resume_for_multi_activities( who );
}

void churn_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );
    jsout.member( "tool", tool );

    jsout.end_object();
}

std::unique_ptr<activity_actor> churn_activity_actor::deserialize( JsonValue &jsin )
{
    churn_activity_actor actor( {}, {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );
    data.read( "tool", actor.tool );

    return actor.clone();
}

void clear_rubble_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
}

void clear_rubble_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();
    const tripoint_bub_ms &pos = here.get_bub( act.placement );
    who.add_msg_if_player( m_info, _( "You clear up the %s." ), here.furnname( pos ) );
    here.furn_set( pos, furn_str_id::NULL_ID() );

    act.set_to_null();

    here.maybe_trigger_trap( pos, who, true );
}

void clear_rubble_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );

    jsout.end_object();
}

std::unique_ptr<activity_actor> clear_rubble_activity_actor::deserialize( JsonValue &jsin )
{
    clear_rubble_activity_actor actor( {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );

    return actor.clone();
}

void firstaid_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
    act.name = name;
}

void firstaid_activity_actor::finish( player_activity &act, Character &who )
{
    map &here = get_map();

    static const std::string iuse_name_string( "heal" );

    item_location it = act.targets.front();
    if( !it ) {
        debugmsg( "Lost tool used for healing" );
        act.set_to_null();
        return;
    }
    item *used_tool = it->get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for healing" );
        act.set_to_null();
        return;
    }

    const use_function *use_fun = used_tool->get_use( iuse_name_string );
    const heal_actor *actor = dynamic_cast<const heal_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act.set_to_null();
        return;
    }

    Character *patient = patientID == get_avatar().getID() ? &get_avatar() :
                         dynamic_cast<Character *>( g->find_npc( patientID ) );
    if( !patient ) {
        debugmsg( "Your patient can no longer be found so you stop using the %s.", name );
        act.set_to_null();
        act.values.clear();
        return;
    }
    const bodypart_id healed = bodypart_id( act.str_values[0] );
    int charges_consumed = actor->finish_using( who, *patient,
                           *used_tool, healed );
    std::list<item>used;
    if( used_tool->has_flag( flag_SINGLE_USE ) ) {
        it.remove_item();
    } else if( used_tool->is_medication() ) {
        if( !it->count_by_charges() ||
            it->use_charges( it->typeId(), charges_consumed, used, it.pos_bub( here ) ) ) {
            it.remove_item();
        }
    } else if( used_tool->is_tool() ) {
        if( used_tool->type->charges_to_use() ) {
            it->activation_consume( charges_consumed, it.pos_bub( here ), &who );
        }
    }

    // Erase activity and values.
    act.set_to_null();
    act.values.clear();

    // Return to first eat or consume meds menu activity in the backlog.
    for( player_activity &backlog_act : who.backlog ) {
        if( backlog_act.id() == ACT_EAT_MENU ||
            backlog_act.id() == ACT_CONSUME_MEDS_MENU ) {
            backlog_act.auto_resume = true;
            break;
        }
    }
    // Clear the backlog of any activities that will not auto resume.
    for( auto iter = who.backlog.begin(); iter != who.backlog.end(); ) {
        if( !iter->auto_resume ) {
            iter = who.backlog.erase( iter );
        } else {
            ++iter;
        }
    }
}

void firstaid_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );
    jsout.member( "name", name );
    jsout.member( "patientID", patientID );

    jsout.end_object();
}

std::unique_ptr<activity_actor> firstaid_activity_actor::deserialize( JsonValue &jsin )
{
    firstaid_activity_actor actor( {},  {}, {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );
    data.read( "name", actor.name );
    data.read( "patientID", actor.patientID );

    return actor.clone();
}

void forage_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
}

void forage_activity_actor::finish( player_activity &act, Character &who )
{
    // Don't forage if we aren't next to the bush - otherwise we get weird bugs
    bool next_to_bush = false;
    map &here = get_map();
    for( const tripoint_bub_ms &pnt : here.points_in_radius( who.pos_bub(), 1 ) ) {
        if( here.get_abs( pnt ) == act.placement ) {
            next_to_bush = true;
            break;
        }
    }

    if( !next_to_bush ) {
        act.set_to_null();
        return;
    }

    const int veggy_chance = rng( 1, 100 );
    bool found_something = false;

    item_group_id group_id;
    ter_str_id next_ter;

    switch( season_of_year( calendar::turn ) ) {
        case SPRING:
            group_id = Item_spawn_data_forage_spring;
            next_ter = ter_t_underbrush_harvested_spring;
            break;
        case SUMMER:
            group_id = Item_spawn_data_forage_summer;
            next_ter = ter_t_underbrush_harvested_summer;
            break;
        case AUTUMN:
            group_id = Item_spawn_data_forage_autumn;
            next_ter = ter_t_underbrush_harvested_autumn;
            break;
        case WINTER:
            group_id = Item_spawn_data_forage_winter;
            next_ter = ter_t_underbrush_harvested_winter;
            break;
        default:
            debugmsg( "Invalid season" );
    }

    const tripoint_bub_ms bush_pos = here.get_bub( act.placement );
    here.ter_set( bush_pos, next_ter );

    // Survival gives a bigger boost, and Perception is leveled a bit.
    // Both survival and perception affect time to forage

    ///\EFFECT_PER slightly increases forage success chance
    ///\EFFECT_SURVIVAL increases forage success chance
    if( veggy_chance < round( who.get_skill_level( skill_survival ) * 3 + who.per_cur - 2 ) ) {
        const std::vector<item *> dropped =
            here.put_items_from_loc( group_id, who.pos_bub(), calendar::turn );
        // map::put_items_from_loc can create multiple items and merge them into one stack.
        // That stack will then appear multiple times in the vector.
        // Remember and check the already handled items to prevent rolling
        // poison/hallucinogen multiple times and print misleading messages.
        std::vector<item *> handled;
        for( item *it : dropped ) {
            if( std::find( handled.rbegin(), handled.rend(), it ) != handled.rend() )  {
                continue;
            }
            handled.push_back( it );
            found_something = true;
            if( it->has_flag( flag_FORAGE_POISON ) && one_in( 10 ) ) {
                it->set_flag( flag_HIDDEN_POISON );
                it->poison = rng( 2, 7 );
            } else if( it->has_flag( flag_FORAGE_HALLU ) && one_in( 10 ) ) {
                it->set_flag( flag_HIDDEN_HALLU );
            }
            add_msg( m_good, _( "You found: %s!" ), it->display_name() );
        }
    }
    // 10% to drop a item/items from this group.
    if( one_in( 10 ) ) {
        const std::vector<item *> dropped =
            here.put_items_from_loc( Item_spawn_data_SUS_trash_forest_no_manmade, who.pos_bub(),
                                     calendar::turn );
        // same as above
        std::vector<item *> handled;
        for( item * const &it : dropped ) {
            if( std::find( handled.rbegin(), handled.rend(), it ) != handled.rend() ) {
                continue;
            }
            handled.push_back( it );
            add_msg( m_good, _( "You found: %s!" ), it->display_name() );
            found_something = true;
        }
    }

    if( !found_something ) {
        add_msg( _( "You didn't find anything." ) );
    }

    iexamine::practice_survival_while_foraging( who );

    act.set_to_null();

    here.maybe_trigger_trap( bush_pos, who, true );
}

void forage_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );

    jsout.end_object();
}

std::unique_ptr<activity_actor> forage_activity_actor::deserialize( JsonValue &jsin )
{
    forage_activity_actor actor( {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );

    return actor.clone();
}

void gunmod_add_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
    act.name = name;
}

void gunmod_add_activity_actor::finish( player_activity &act, Character &who )
{
    act.set_to_null();
    // first unpack all of our arguments
    if( act.values.size() != 4 ) {
        debugmsg( "Insufficient arguments to ACT_GUNMOD_ADD" );
        return;
    }

    item &gun = *act.targets.at( 0 );
    item &mod = *act.targets.at( 1 );

    // chance of success (%)
    const int roll = act.values[1];
    // chance of damage (%)
    const int risk = act.values[2];

    // any tool charges used during installation
    const itype_id tool( act.name );
    const int qty = act.values[3];

    if( !gun.is_gunmod_compatible( mod ).success() ) {
        debugmsg( "Invalid arguments in ACT_GUNMOD_ADD" );
        return;
    }

    if( !tool.is_empty() && qty > 0 ) {
        who.use_charges( tool, qty );
    }

    if( rng( 0, 100 ) <= roll ) {
        add_msg( m_good, _( "You successfully attached the %1$s to your %2$s." ), mod.tname(),
                 gun.tname() );
        gun.put_in( who.i_rem( &mod ), pocket_type::MOD );
        gun.on_contents_changed();

    } else if( rng( 0, 100 ) <= risk ) {
        if( gun.inc_damage() ) {
            // Remove irremovable mods prior to destroying the gun
            for( item *mod : gun.gunmods() ) {
                if( mod->is_irremovable() ) {
                    who.remove_item( *mod );
                }
            }
            add_msg( m_bad, _( "You failed at installing the %s and destroyed your %s!" ), mod.tname(),
                     gun.tname() );
            who.i_rem( &gun );
        } else {
            add_msg( m_bad, _( "You failed at installing the %s and damaged your %s!" ), mod.tname(),
                     gun.tname() );
        }

    } else {
        add_msg( m_info, _( "You failed at installing the %s." ), mod.tname() );
    }
}

void gunmod_add_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );
    jsout.member( "name", name );

    jsout.end_object();
}

std::unique_ptr<activity_actor> gunmod_add_activity_actor::deserialize( JsonValue &jsin )
{
    gunmod_add_activity_actor actor( {}, {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );
    data.read( "name", actor.name );

    return actor.clone();
}

void longsalvage_activity_actor::start( player_activity &act, Character & )
{
    act.index = index;

    //todo: refactor the actor to process items in ::do_turn, then remove setting the moves to 0
    //this currently still can't get interrupted before you get attacked
    act.moves_left = 0;
}

void longsalvage_activity_actor::finish( player_activity &act, Character &who )
{
    static const std::string salvage_string = "salvage";
    item &main_tool = who.i_at( act.index );
    map &here = get_map();
    map_stack items = here.i_at( who.pos_bub() );
    item *salvage_tool = main_tool.get_usable_item( salvage_string );
    if( salvage_tool == nullptr ) {
        debugmsg( "Lost tool used for long salvage" );
        act.set_to_null();
        return;
    }

    const use_function *use_fun = salvage_tool->get_use( salvage_string );
    const salvage_actor *actor = dynamic_cast<const salvage_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act.set_to_null();
        return;
    }

    for( item &it : items ) {
        // Check first and only if possible attempt it with player char
        // This suppresses warnings unless it is an item the player wears
        if( actor->valid_to_cut_up( nullptr, it ) ) {
            item_location item_loc( map_cursor( who.pos_abs() ), &it );
            actor->try_to_cut_up( who, *salvage_tool, item_loc );
            return;
        }
    }

    add_msg( _( "You finish salvaging." ) );
    act.set_to_null();
}

void longsalvage_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "index", index );

    jsout.end_object();
}

std::unique_ptr<activity_actor> longsalvage_activity_actor::deserialize( JsonValue &jsin )
{
    longsalvage_activity_actor actor( {} );

    JsonObject data = jsin.get_object();

    data.read( "index", actor.index );

    return actor.clone();
}

void mop_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
}

void mop_activity_actor::finish( player_activity &act, Character &who )
{
    // Blind character have a 1/3 chance of actually mopping.
    const bool will_mop = one_in( who.is_blind() ? 1 : 3 );
    if( will_mop ) {
        map &here = get_map();
        here.mop_spills( here.get_bub( act.placement ) );
    }
    activity_handlers::resume_for_multi_activities( who );
}

void mop_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );

    jsout.end_object();
}

std::unique_ptr<activity_actor> mop_activity_actor::deserialize( JsonValue &jsin )
{
    mop_activity_actor actor( {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );

    return actor.clone();
}

void unload_loot_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );
    jsout.member( "num_processed", num_processed );
    jsout.member( "stage", stage );
    jsout.member( "coord_set", coord_set );
    jsout.member( "placement", placement );

    jsout.end_object();
}

std::unique_ptr<activity_actor> unload_loot_activity_actor::deserialize( JsonValue &jsin )
{
    unload_loot_activity_actor actor( {} );

    JsonObject data = jsin.get_object();

    data.read( "moves", actor.moves );
    data.read( "num_processed", actor.num_processed );
    data.read( "stage", actor.stage );
    data.read( "coord_set", actor.coord_set );
    data.read( "placement", actor.placement );

    return actor.clone();
}

void unload_loot_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = moves;
    act.moves_left = moves;
}

static void move_item( Character &you, item &it, const int quantity, const tripoint_bub_ms &src,
                       const tripoint_bub_ms &dest, vehicle *src_veh, int src_part )
{
    item leftovers = it;

    if( quantity != 0 && it.count_by_charges() ) {
        // Reinserting leftovers happens after item removal to avoid stacking issues.
        leftovers.charges = it.charges - quantity;
        if( leftovers.charges > 0 ) {
            it.charges = quantity;
        }
    } else {
        leftovers.charges = 0;
    }

    map &here = get_map();
    // Check that we can pick it up.
    if( it.made_of_from_type( phase_id::SOLID ) ) {
        you.mod_moves( -activity_handlers::move_cost( it, src, dest ) );

        put_into_vehicle_or_drop( you, item_drop_reason::deliberate, { it }, &here, dest );
        // Remove from map or vehicle.
        if( src_veh ) {
            vehicle_part &vp_src = src_veh->part( src_part );
            src_veh->remove_item( vp_src, &it );
        } else {
            here.i_rem( src, &it );
        }
    }

    // If we didn't pick up a whole stack, put the remainder back where it came from.
    if( leftovers.charges > 0 ) {
        if( src_veh ) {
            vehicle_part &vp_src = src_veh->part( src_part );
            if( !src_veh->add_item( here, vp_src, leftovers ) ) {
                debugmsg( "SortLoot: Source vehicle failed to receive leftover charges." );
            }
        } else {
            here.add_item_or_charges( src, leftovers );
        }
    }
}

void unload_loot_activity_actor::do_turn( player_activity &act, Character &you )
{
    enum activity_stage : int {
        //Initial stage
        INIT = 0,
        //Think about what to do first: choose destination
        THINK,
        //Do activity
        DO,
    };

    faction const *fac = you.get_faction();
    faction_id fac_id = fac == nullptr ? faction_id() : fac->id;

    map &here = get_map();
    const tripoint_abs_ms abspos = you.pos_abs();
    zone_manager &mgr = zone_manager::get_manager();
    if( here.check_vehicle_zones( here.get_abs_sub().z() ) ) {
        mgr.cache_vzones();
    }

    if( stage == INIT ) {
        coord_set.clear();
        for( const tripoint_abs_ms &p :
             mgr.get_near( zone_type_UNLOAD_ALL, abspos, MAX_VIEW_DISTANCE, nullptr,
                           fac_id ) ) {
            coord_set.insert( p );
        }

        for( const tripoint_abs_ms &p :
             mgr.get_near( zone_type_STRIP_CORPSES, abspos, MAX_VIEW_DISTANCE, nullptr,
                           fac_id ) ) {
            coord_set.insert( p );
        }
        stage = THINK;
    }

    if( stage == THINK ) {
        // initialize num_processed
        num_processed = 0;
        std::vector<tripoint_abs_ms> src_set;
        src_set.reserve( coord_set.size() );
        for( const tripoint_abs_ms &p : coord_set ) {
            src_set.emplace_back( p );
        }
        // sort source tiles by distance
        const auto &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );

        for( const tripoint_abs_ms &src : src_sorted ) {
            placement = src;
            coord_set.erase( src );

            const tripoint_bub_ms &src_loc = here.get_bub( src );
            if( !here.inbounds( src_loc ) ) {
                if( !here.inbounds( you.pos_bub() ) ) {
                    // p is implicitly an NPC that has been moved off the map, so reset the activity
                    // and unload them
                    you.cancel_activity();
                    you.assign_activity( ACT_UNLOAD_LOOT );
                    you.set_moves( 0 );
                    g->reload_npcs();
                    return;
                }
                std::vector<tripoint_bub_ms> route;
                route = here.route( you, pathfinding_target::point( src_loc ) );
                if( route.empty() ) {
                    // can't get there, can't do anything, skip it
                    continue;
                }
                stage = DO;
                you.set_destination( route, act );
                you.activity.set_to_null();
                return;
            }

            // skip tiles in IGNORE zone and tiles on fire
            // (to prevent taking out wood off the lit brazier)
            // and inaccessible furniture, like filled charcoal kiln
            if( mgr.has( zone_type_LOOT_IGNORE, src, fac_id ) ||
                here.get_field( src_loc, fd_fire ) != nullptr ||
                !here.can_put_items_ter_furn( src_loc ) || here.impassable_field_at( src_loc ) ) {
                continue;
            }

            //nothing to sort?
            const std::optional<vpart_reference> ovp = here.veh_at( src_loc ).cargo();
            if( ( !ovp || ovp->items().empty() ) && here.i_at( src_loc ).empty() ) {
                continue;
            }

            bool is_adjacent_or_closer = square_dist( you.pos_bub(), src_loc ) <= 1;
            // before we unload any item, check if player is at or
            // adjacent to the loot source tile
            if( !is_adjacent_or_closer ) {
                std::vector<tripoint_bub_ms> route;

                // get either direct route or route to nearest adjacent tile if
                // source tile is impassable
                route = here.route( you, pathfinding_target::adjacent( src_loc ) );

                // check if we found path to source / adjacent tile
                if( route.empty() ) {
                    add_msg( m_info, _( "%s can't reach the source tile." ),
                             you.disp_name() );
                    continue;
                }

                // set the destination and restart activity after player arrives there
                // we don't need to check for safe mode,
                // activity will be restarted only if
                // player arrives on destination tile
                stage = DO;
                you.set_destination( route, act );
                you.activity.set_to_null();
                return;
            }
            stage = DO;
            break;
        }
    }
    if( stage == DO ) {
        const tripoint_abs_ms src( placement );
        const tripoint_bub_ms src_loc = here.get_bub( src );

        bool is_adjacent_or_closer = square_dist( you.pos_bub(), src_loc ) <= 1;
        // before we move any item, check if player is at or
        // adjacent to the loot source tile
        if( !is_adjacent_or_closer ) {
            stage = THINK;
            return;
        }

        // the boolean in this pair being true indicates the item is from a vehicle storage space
        auto items = std::vector<std::pair<item *, bool>>();
        vehicle *src_veh;
        int src_part;

        //Check source for cargo part
        //map_stack and vehicle_stack are different types but inherit from item_stack
        // TODO: use one for loop
        if( const std::optional<vpart_reference> ovp = here.veh_at( src_loc ).cargo() ) {
            src_veh = &ovp->vehicle();
            src_part = ovp->part_index();
            vehicle_stack vp_items = ovp->items();
            items.reserve( vp_items.size() );
            for( item &it : vp_items ) {
                items.emplace_back( &it, true );
            }
        } else {
            src_veh = nullptr;
            src_part = -1;
        }
        map_stack map_items = here.i_at( src_loc );
        items.reserve( items.size() + map_items.size() );
        for( item &it : map_items ) {
            items.emplace_back( &it, false );
        }

        bool unload_mods = false;
        bool unload_molle = false;
        bool unload_sparse_only = false;
        int unload_sparse_threshold = 20;

        std::vector<zone_data const *> const zones = mgr.get_zones_at( src, zone_type_UNLOAD_ALL,
                fac_id );

        // get most open rules out of all stacked zones
        for( zone_data const *zone : zones ) {
            unload_options const &options = dynamic_cast<const unload_options &>( zone->get_options() );
            unload_molle |= options.unload_molle();
            unload_mods |= options.unload_mods();
            unload_sparse_only |= options.unload_sparse_only();
            unload_sparse_threshold |= options.unload_sparse_threshold();
        }

        //Skip items that have already been processed
        for( auto it = items.begin() + num_processed; it < items.end(); ++it ) {

            item &thisitem = *it->first;

            // skip unpickable liquid
            if( thisitem.made_of_from_type( phase_id::LIQUID ) ) {
                ++num_processed;
                continue;
            }

            // skip favorite items in ignore favorite zones
            if( thisitem.is_favorite && mgr.has( zone_type_LOOT_IGNORE_FAVORITES, src, fac_id ) ) {
                ++num_processed;
                continue;
            }

            // Only if it's from a vehicle do we use the vehicle source location information.
            vehicle *this_veh = it->second ? src_veh : nullptr;
            const int this_part = it->second ? src_part : -1;

            // if this item isn't going anywhere and its not sealed
            // check if it is in a unload zone or a strip corpse zone
            // then we should unload it and see what is inside
            if( mgr.has_near( zone_type_UNLOAD_ALL, abspos, 1, fac_id ) ||
                ( mgr.has_near( zone_type_STRIP_CORPSES, abspos, 1, fac_id ) && it->first->is_corpse() ) ) {
                if( you.rate_action_unload( *it->first ) == hint_rating::good &&
                    !it->first->any_pockets_sealed() ) {
                    std::unordered_map<itype_id, int> item_counts;
                    if( unload_sparse_only ) {
                        for( item *contained : it->first->all_items_top( pocket_type::CONTAINER ) ) {
                            if( !contained->made_of( phase_id::LIQUID ) && !contained->made_of( phase_id::GAS ) ) {
                                item_counts[contained->typeId()]++;
                            }
                        }
                    }
                    for( item *contained : it->first->all_items_top( pocket_type::CONTAINER ) ) {
                        // no liquids don't want to spill stuff
                        if( !contained->made_of( phase_id::LIQUID ) && !contained->made_of( phase_id::GAS ) ) {
                            if( unload_sparse_only &&
                                item_counts[contained->typeId()] > unload_sparse_threshold ) {
                                continue;
                            }
                            move_item( you, *contained, contained->count(), src_loc, src_loc, this_veh, this_part );
                            it->first->remove_item( *contained );
                        }
                        if( you.get_moves() <= 0 ) {
                            return;
                        }
                    }
                    for( item *contained : it->first->all_items_top( pocket_type::MAGAZINE ) ) {
                        // no liquids don't want to spill stuff
                        if( !contained->made_of( phase_id::LIQUID ) && !contained->made_of( phase_id::GAS ) ) {
                            if( it->first->is_ammo_belt() ) {
                                if( it->first->type->magazine->linkage ) {
                                    item link( *it->first->type->magazine->linkage, calendar::turn, contained->count() );
                                    if( this_veh != nullptr ) {
                                        vehicle_part &vp_this = this_veh->part( this_part );
                                        this_veh->add_item( here, vp_this, link );
                                    } else {
                                        here.add_item_or_charges( src_loc, link );
                                    }
                                }
                            }
                            move_item( you, *contained, contained->count(), src_loc, src_loc, this_veh, this_part );
                            it->first->remove_item( *contained );

                            if( it->first->has_flag( flag_MAG_DESTROY ) && it->first->ammo_remaining( ) == 0 ) {
                                if( this_veh != nullptr ) {
                                    vehicle_part &vp_this = this_veh->part( this_part );
                                    this_veh->remove_item( vp_this, it->first );
                                } else {
                                    here.i_rem( src_loc, it->first );
                                }
                            }
                        }
                        if( you.get_moves() <= 0 ) {
                            return;
                        }
                    }
                    for( item *contained : it->first->all_items_top( pocket_type::MAGAZINE_WELL ) ) {
                        // no liquids don't want to spill stuff
                        if( !contained->made_of( phase_id::LIQUID ) && !contained->made_of( phase_id::GAS ) ) {
                            move_item( you, *contained, contained->count(), src_loc, src_loc, this_veh, this_part );
                            it->first->remove_item( *contained );
                        }
                        if( you.get_moves() <= 0 ) {
                            return;
                        }
                    }
                }
                // if unloading mods
                if( unload_mods ) {
                    // remove each mod, skip irremovable
                    for( item *mod : it->first->gunmods() ) {
                        if( mod->is_irremovable() ) {
                            continue;
                        }
                        you.gunmod_remove( *it->first, *mod );
                        if( you.get_moves() <= 0 ) {
                            return;
                        }
                    }
                }

                // if unloading molle
                if( unload_molle ) {
                    while( !it->first->get_contents().get_added_pockets().empty() ) {
                        item removed = it->first->get_contents().remove_pocket( 0 );
                        move_item( you, removed, 1, src_loc, src_loc, this_veh, this_part );
                        if( you.get_moves() <= 0 ) {
                            return;
                        }
                    }
                }

                // after dumping items go back to start of activity loop
                // so that can re-assess the items in the tile
                ++num_processed;
                return;
            }

            if( you.get_moves() <= 0 ) {
                return;
            }
        }

        //this location is sorted
        stage = THINK;
        return;
    }

    // If we got here without restarting the activity, it means we're done
    add_msg( m_info, _( "%s sorted out every item possible." ), you.disp_name( false, true ) );
    if( you.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &you );
        guy->revert_after_activity();
    }
    act.set_to_null();
}

bool vehicle_folding_activity_actor::fold_vehicle( Character &p, bool check_only ) const
{
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( target_pos );
    if( !vp ) {
        debugmsg( "Lost ACT_VEHICLE_FOLD target vehicle" );
        return false;
    }
    vehicle &veh = vp->vehicle();

    if( !veh.is_foldable() ) {
        p.add_msg_if_player( m_warning, _( "%s cannot be folded." ), veh.name );
        return false;
    }

    const inventory &inv = p.crafting_inventory();
    for( const vpart_reference &vp : veh.get_all_parts() ) {
        for( const itype_id &tool : vp.info().get_folding_tools() ) {
            if( !inv.has_tools( tool, 1 ) ) {
                p.add_msg_if_player( _( "You need %s to do it!" ), item::nname( tool ) );
                return false;
            }
        }
    }

    if( veh.velocity != 0 ) {
        p.add_msg_if_player( m_warning, _( "You can't fold the %s while it's in motion." ), veh.name );
        return false;
    }

    if( check_only ) {
        return true; // bail out early if we're just checking
    }

    // Drop cargo to ground
    // TODO: make cargo shuffling add time to activity
    for( const vpart_reference &vpr : veh.get_any_parts( VPFLAG_CARGO ) ) {
        vehicle_stack cargo = vpr.items();
        for( const item &elem : cargo ) {
            here.add_item_or_charges( veh.pos_bub( here ), elem );
        }
        cargo.clear();
    }

    veh.unboard_all( here );
    p.add_msg_if_player( _( "You fold the %s." ), veh.name );
    here.add_item_or_charges( veh.pos_bub( here ), veh.get_folded_item( here ) );
    here.destroy_vehicle( &veh );

    return true;
}

vehicle_folding_activity_actor::vehicle_folding_activity_actor( const vehicle &target )
{
    map &here = get_map();
    folding_time = target.folding_time();
    target_pos = target.bub_part_pos( here,  0 );
}

void vehicle_folding_activity_actor::start( player_activity &act, Character &p )
{
    avatar *pc = p.as_avatar();
    if( pc && pc->get_grab_type() == object_type::VEHICLE ) {
        optional_vpart_position grabbed_veh = get_map().veh_at( pc->pos_bub() + pc->grab_point );
        optional_vpart_position target_veh = get_map().veh_at( target_pos );
        if( grabbed_veh && target_veh && ( &grabbed_veh->vehicle() == &target_veh->vehicle() ) ) {
            pc->grab( object_type::NONE );
            pc->add_msg_if_player( _( "You let go of %s." ), grabbed_veh->vehicle().name );
        }
    }

    act.moves_total = to_moves<int>( folding_time );
    act.moves_left = to_moves<int>( folding_time );

    if( !fold_vehicle( p, /* check_only = */ true ) ) {
        act.set_to_null();
    }
}

void vehicle_folding_activity_actor::do_turn( player_activity &act, Character &p )
{
    if( calendar::once_every( 30_seconds ) ) {
        if( !fold_vehicle( p, /* check_only = */ true ) ) {
            act.set_to_null();
        }
    }
}

void vehicle_folding_activity_actor::finish( player_activity &act, Character &p )
{
    fold_vehicle( p, /* check_only = */ false );
    act.set_to_null();
}

void vehicle_folding_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "target_pos", target_pos );
    jsout.member( "folding_time", folding_time );
    jsout.end_object();
}

std::unique_ptr<activity_actor> vehicle_folding_activity_actor::deserialize( JsonValue &jsin )
{
    vehicle_folding_activity_actor actor;
    JsonObject data = jsin.get_object();
    data.read( "target_pos", actor.target_pos );
    data.read( "folding_time", actor.folding_time );
    return actor.clone();
}

bool vehicle_unfolding_activity_actor::unfold_vehicle( Character &p, bool check_only ) const
{
    if( p.cant_do_underwater() ) {
        return false;
    }
    if( p.cant_do_mounted() ) {
        return false;
    }
    map &here = get_map();
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, p.pos_bub(), 0_degrees, 0, 0, false );
    if( veh == nullptr ) {
        p.add_msg_if_player( m_info, _( "There's no room to unfold the %s." ), it.tname() );
        return false;
    }
    if( !veh->restore_folded_parts( it ) ) {
        here.destroy_vehicle( veh );
        return false;
    }
    const bool cant_float = !veh->can_float( here );
    const auto invalid_pos = [&here, &cant_float]( const tripoint_bub_ms & p ) {
        return ( cant_float && here.has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, p ) )
               || here.veh_at( p )
               || here.impassable( p );
    };

    const inventory &inv = p.crafting_inventory();
    for( const vpart_reference &vp : veh->get_all_parts() ) {
        if( vp.info().location != "structure" ) {
            continue;
        }
        if( invalid_pos( vp.pos_bub( here ) ) ) {
            p.add_msg_if_player( m_info, _( "There's no room to unfold the %s." ), it.tname() );
            here.destroy_vehicle( veh );
            return false;
        }
        for( const itype_id &tool : vp.info().get_unfolding_tools() ) {
            if( !inv.has_tools( tool, 1 ) ) {
                p.add_msg_if_player( _( "You need %s to do it!" ), item::nname( tool ) );
                here.destroy_vehicle( veh );
                return false;
            }
        }
    }
    if( check_only ) { // if we're just checking bail out at this point - this is a valid location.
        here.destroy_vehicle( veh );
        return true;
    }
    veh->name = it.get_var( "vehicle_name" );
    veh->set_owner( p );
    here.add_vehicle_to_cache( veh );
    if( here.veh_at( p.pos_bub() ).part_with_feature( "BOARDABLE", true ) ) {
        here.board_vehicle( p.pos_bub(), &p ); // if boardable unbroken part is present -> get on it
    }
    if( p.is_avatar() && it.get_var( "tracking", 0 ) == 1 ) {
        veh->toggle_tracking(); // restore position tracking state
    }

    std::string unfold_msg = it.get_var( "unfold_msg" );
    if( unfold_msg.empty() ) {
        unfold_msg = _( "You unfold the %s." );
    } else {
        unfold_msg = _( unfold_msg );
    }
    p.add_msg_if_player( m_neutral, unfold_msg, veh->name );

    return true;
}

vehicle_unfolding_activity_actor::vehicle_unfolding_activity_actor( const item &it )
    : it( it )
{
    constexpr int legacy_unfold_moves = 500;
    unfolding_time = time_duration::from_moves( it.get_var( "unfolding_time", legacy_unfold_moves ) );
}

void vehicle_unfolding_activity_actor::start( player_activity &act, Character &p )
{
    act.moves_total = to_moves<int>( unfolding_time );
    act.moves_left = to_moves<int>( unfolding_time );
    if( !unfold_vehicle( p, /* check_only = */ true ) ) {
        act.canceled( p );
    }
}

void vehicle_unfolding_activity_actor::do_turn( player_activity &act, Character &p )
{
    if( calendar::once_every( 30_seconds ) ) {
        if( !unfold_vehicle( p, /* check_only = */ true ) ) {
            act.canceled( p );
        }
    }
}

void vehicle_unfolding_activity_actor::finish( player_activity &act, Character &p )
{
    if( !unfold_vehicle( p, /* check_only = */ false ) ) {
        debugmsg( "failed unfolding %s", it.display_name() );
        act.canceled( p );
    }
    act.set_to_null();
}

void vehicle_unfolding_activity_actor::canceled( player_activity &act, Character &p )
{
    get_map().add_item_or_charges( p.pos_bub(), it, true );
    act.set_to_null();
}

void vehicle_unfolding_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "it", it );
    jsout.member( "unfolding_time", unfolding_time );
    jsout.end_object();
}

std::unique_ptr<activity_actor> vehicle_unfolding_activity_actor::deserialize( JsonValue &jsin )
{
    vehicle_unfolding_activity_actor actor;
    JsonObject data = jsin.get_object();
    data.read( "it", actor.it );
    data.read( "unfolding_time", actor.unfolding_time );
    return actor.clone();
}

int heat_activity_actor::get_available_heater( Character &p, item_location &loc ) const
{
    const map &here = get_map();

    int available_heater = 0;
    if( !loc->has_no_links() ) {
        available_heater = loc->link().t_veh->connected_battery_power_level( here ).first;
    } else if( !loc->has_flag( flag_USE_UPS ) ) {
        available_heater = loc->ammo_remaining( );
    } else if( loc->has_flag( flag_USE_UPS ) ) {
        available_heater = units::to_kilojoule( p.available_ups() );
    }
    return available_heater;
}

void heat_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = requirements.time;
    act.moves_left = requirements.time;
}

void heat_activity_actor::do_turn( player_activity &act, Character &p )
{
    map &here = get_map();

    // use a hack in use_vehicle_tool vehicle_use.cpp
    if( !act.coords.empty() ) {
        h.vpt = act.coords[0];
    }
    std::optional<vpart_position> vp = here.veh_at( h.vpt );
    if( h.pseudo_flag ) {
        if( !vp ) {
            p.add_msg_if_player( _( "You can't find the appliance any more." ) );
            act.set_to_null();
            return;
        }
        if( vp.value().vehicle().connected_battery_power_level( here ).first < requirements.ammo *
            h.heating_effect ) {
            p.add_msg_if_player( _( "You need more energy to heat these items." ) );
            act.set_to_null();
            return;
        }
    } else {
        if( !h.loc ) {
            p.add_msg_if_player( _( "You can't find the heater any more." ) );
            act.set_to_null();
            return;
        }
        if( get_available_heater( p, h.loc ) < requirements.ammo * h.heating_effect ) {
            p.add_msg_if_player( _( "You need more energy to heat these items." ) );
            act.set_to_null();
            return;
        }
    }
    for( drop_location &ait : to_heat ) {
        if( !ait.first ) {
            p.add_msg_if_player( _( "Some of the food you selected is gone." ) );
            act.set_to_null();
            return;
        }
    }
}

void heat_activity_actor::finish( player_activity &act, Character &p )
{
    map &here = get_map();

    for( drop_location &ait : to_heat ) {
        item_location cold_item = ait.first;
        if( cold_item->count_by_charges() ) {
            item copy( *cold_item );
            copy.charges = ait.second;
            copy.unset_flag( flag_FROZEN );
            copy.set_flag( flag_HOT );
            cold_item->charges -= ait.second;
            if( cold_item->charges <= 0 ) {
                cold_item.remove_item();
            }
            if( copy.made_of( phase_id::LIQUID ) ) {
                liquid_handler::handle_all_liquid( copy, PICKUP_RANGE );
            } else {
                p.i_add_or_drop( copy );
            }
        } else {
            cold_item->unset_flag( flag_FROZEN );
            cold_item->set_flag( flag_HOT );
            if( cold_item.get_item()->made_of( phase_id::LIQUID ) ) {
                liquid_handler::handle_all_liquid( *cold_item, PICKUP_RANGE );
            } else {
                p.i_add_or_drop( *cold_item );
                cold_item.remove_item();
            }
        }
    }
    if( h.consume_flag ) {
        if( h.pseudo_flag ) {
            here.veh_at( h.vpt ).value().vehicle().discharge_battery( here, requirements.ammo *
                    h.heating_effect );
        } else {
            h.loc->activation_consume( requirements.ammo, h.loc.pos_bub( here ), &p );
        }
    }
    p.add_msg_if_player( m_good, _( "You heated your items." ) );

    p.invalidate_crafting_inventory();

    act.set_to_null();
}

void heat_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "to_heat", to_heat );
    jsout.member( "heating_effect", h.heating_effect );
    jsout.member( "loc", h.loc );
    jsout.member( "consume_flag", h.consume_flag );
    jsout.member( "pseudo_flag", h.pseudo_flag );
    jsout.member( "time", requirements.time );
    jsout.member( "ammo", requirements.ammo );
    jsout.member( "vpt", h.vpt );
    jsout.end_object();
}

std::unique_ptr<activity_actor> heat_activity_actor::deserialize( JsonValue &jsin )
{
    heat_activity_actor actor;
    JsonObject data = jsin.get_object();
    data.read( "to_heat", actor.to_heat );
    data.read( "heating_effect", actor.h.heating_effect );
    data.read( "loc", actor.h.loc );
    data.read( "consume_flag", actor.h.consume_flag );
    data.read( "pseudo_flag", actor.h.pseudo_flag );
    data.read( "time", actor.requirements.time );
    data.read( "ammo", actor.requirements.ammo );
    data.read( "vpt", actor.h.vpt );
    return actor.clone();
}

void wash_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = requirements.time;
    act.moves_left = requirements.time;
}

void wash_activity_actor::finish( player_activity &act, Character &p )
{
    const auto is_liquid_crafting_component = []( const item & it ) {
        return is_crafting_component( it ) && ( !it.count_by_charges() || it.made_of( phase_id::LIQUID ) );
    };
    const inventory &crafting_inv = p.crafting_inventory();
    if( !crafting_inv.has_charges( itype_water, requirements.water, is_liquid_crafting_component ) &&
        !crafting_inv.has_charges( itype_water_clean, requirements.water, is_liquid_crafting_component ) ) {
        p.add_msg_if_player( _( "You need %1$i charges of water or clean water to wash these items." ),
                             requirements.water );
        act.set_to_null();
        return;
    } else if( !crafting_inv.has_charges( itype_soap, requirements.cleanser ) &&
               !crafting_inv.has_charges( itype_detergent, requirements.cleanser ) &&
               !crafting_inv.has_charges( itype_liquid_soap, requirements.cleanser,
                                          is_liquid_crafting_component ) ) {
        p.add_msg_if_player( _( "You need %1$i charges of cleansing agent to wash these items." ),
                             requirements.cleanser );
        act.set_to_null();
        return;
    }

    for( drop_location &ait : to_wash ) {
        item_location filthy_item = ait.first;
        if( filthy_item->count_by_charges() ) {
            item copy( *filthy_item );
            copy.charges = ait.second;
            copy.unset_flag( flag_FILTHY );
            filthy_item->charges -= ait.second;
            if( filthy_item->charges <= 0 ) {
                filthy_item.remove_item();
            }
            p.i_add_or_drop( copy );
        } else {
            filthy_item->unset_flag( flag_FILTHY );
            p.on_worn_item_washed( *filthy_item );
        }
    }

    std::vector<item_comp> comps;
    comps.reserve( 2 );
    comps.emplace_back( itype_water, requirements.water );
    comps.emplace_back( itype_water_clean, requirements.water );
    p.consume_items( comps, 1, is_liquid_crafting_component );

    std::vector<item_comp> comps1;
    comps1.reserve( 3 );
    comps1.emplace_back( itype_soap, requirements.cleanser );
    comps1.emplace_back( itype_detergent, requirements.cleanser );
    comps1.emplace_back( itype_liquid_soap, requirements.cleanser );
    p.consume_items( comps1 );

    p.add_msg_if_player( m_good, _( "You washed your items." ) );

    // Make sure newly washed components show up as available if player attempts to craft immediately
    p.invalidate_crafting_inventory();

    act.set_to_null();
}

void wash_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "to_wash", to_wash );
    jsout.member( "time", requirements.time );
    jsout.member( "water", requirements.water );
    jsout.member( "cleanser", requirements.cleanser );
    jsout.end_object();
}

std::unique_ptr<activity_actor> wash_activity_actor::deserialize( JsonValue &jsin )
{
    wash_activity_actor actor;
    JsonObject data = jsin.get_object();
    data.read( "to_wash", actor.to_wash );
    data.read( "time", actor.requirements.time );
    data.read( "water", actor.requirements.water );
    data.read( "cleanser", actor.requirements.cleanser );
    return actor.clone();
}

void pulp_activity_actor::start( player_activity &act, Character &you )
{
    // indefinitely long so activity won't end until we pulp all the corpses
    // we then end the activity manually
    act.moves_total = calendar::INDEFINITELY_LONG;
    act.moves_left = calendar::INDEFINITELY_LONG;
    you.recoil = MAX_RECOIL;

    pd = g->calculate_character_ability_to_pulp( you );
}

void pulp_activity_actor::do_turn( player_activity &act, Character &you )
{
    map &here = get_map();

    for( const tripoint_abs_ms &pos_abs : placement ) {
        tripoint_bub_ms pos = here.get_bub( pos_abs );
        map_stack corpse_pile = here.i_at( pos );
        for( item &corpse : corpse_pile ) {
            if( !corpse.is_corpse() || !corpse.can_revive() ) {
                // Don't smash non-rezing corpses or random items
                continue;
            }

            if( corpse.damage() < corpse.max_damage() ) {
                bool can_pulp = punch_corpse_once( corpse, you, pos, here );
                if( !can_pulp ) {
                    ++unpulped_corpses_qty;
                    continue;
                }
                return; // pulp at most one corpse per turn
            }
        }
    }

    // if we are here, that means we survived the whole previous loop
    // without pulping anything. I.e. we are done.
    // Stop the activity.
    act.moves_total = 0;
    act.moves_left = 0;
}

bool pulp_activity_actor::punch_corpse_once( item &corpse, Character &you,
        tripoint_bub_ms pos, map &here )
{
    const mtype *corpse_mtype = corpse.get_mtype();
    if( corpse_mtype == nullptr ) {
        debugmsg( string_format( "Tried to pulp not-a-corpse (id %s)", corpse.typeId().c_str() ) );
        return false;
    }

    if( !g->can_pulp_acid_corpse( you, *corpse_mtype ) ) {
        acid_corpse = true;
        return false;
    }

    pd = g->calculate_pulpability( you, *corpse_mtype, pd );

    if( !g->can_pulp_corpse( pd ) ) {
        way_too_long_to_pulp = true;
        return false;
    }

    add_msg_debug( debugmode::DF_ACTIVITY, "time to pulp: %s", pd.time_to_pulp );

    // 10 minutes
    if( pd.time_to_pulp > 600 && !too_long_to_pulp ) {
        if( you.query_yn( "Pulping one or more corpses in this pile would take too much time.  Continue?" ) ) {
            too_long_to_pulp = true;
        } else {
            too_long_to_pulp_interrupted = true;
            return false;
        }
    }

    // how much damage you need to deal each second to match expected pulp time
    const float corpse_damage = 4000.0 / pd.time_to_pulp + float_corpse_damage_accum;
    float_corpse_damage_accum = corpse_damage - static_cast<int>( corpse_damage ) ;

    // Increase damage as we keep smashing ensuring we eventually smash the target.
    corpse.mod_damage( static_cast<int>( corpse_damage ) );
    if( corpse.damage() == corpse.max_damage() ) {
        ++num_corpses;
        corpse.set_flag( flag_PULPED );
    }

    // Magic number to adjust splatter density to subjective balance.
    if( one_in( 15000 / pd.pulp_power ) ) {
        // Splatter some blood around
        // Splatter a bit more randomly, so that it looks cooler
        const int radius = pd.mess_radius + x_in_y( pd.pulp_power, 500 ) + x_in_y( pd.pulp_power, 1000 );
        const tripoint_bub_ms dest( pos + point( rng( -radius, radius ), rng( -radius, radius ) ) );
        const field_type_id type_blood = ( pd.mess_radius > 1 && x_in_y( pd.pulp_power, 10000 ) ) ?
                                         corpse.get_mtype()->gibType() :
                                         corpse.get_mtype()->bloodType();
        here.add_splatter_trail( type_blood, pos, dest );
    }

    if( pd.stomps_only ) {
        you.burn_energy_legs( -pd.pulp_effort );
    } else if( pd.weapon_only ) {
        you.burn_energy_arms( -pd.pulp_effort );
    } else {
        you.burn_energy_all( -pd.pulp_effort );
    }

    if( one_in( 128 ) ) {
        you.practice( skill_survival, 1, 2, true );
        you.practice( skill_swimming, 1, 5, true );
        if( pd.unknown_prof.has_value() ) {
            you.practice_proficiency( pd.unknown_prof.value(), 5_seconds );
        }
    }

    return true;
}

void pulp_activity_actor::finish( player_activity &act, Character &you )
{
    act.moves_left = 0;

    send_final_message( you );

    if( you.is_npc() ) {
        you.as_npc()->revert_after_activity();
        you.as_npc()->pulp_location.reset();
    } else {
        act.set_to_null();
    }
}

void pulp_activity_actor::send_final_message( Character &you ) const
{

    if( acid_corpse ) {
        you.add_msg_if_player( m_bad,
                               _( "You cannot pulp acid-filled corpses without appropriate protection." ) );
        return;
    }

    if( way_too_long_to_pulp && num_corpses == 0 ) {
        you.add_msg_player_or_npc( n_gettext(
                                       _( "You cannot pulp this corpse, for you have no tool heavy enough to deal with it quickly." ),
                                       _( "You cannot pulp these corpses, for you have no tool heavy enough to deal with it quickly." ),
                                       unpulped_corpses_qty ),
                                   n_gettext(
                                       _( "<npcname> cannot pulp this corpse without a heavier tool." ),
                                       _( "<npcname> cannot pulp these corpses without a heavier tool." ),
                                       unpulped_corpses_qty ) );
        return;
    }

    if( too_long_to_pulp_interrupted && num_corpses == 0 ) {
        you.add_msg_if_player( n_gettext(
                                   _( "You interrupted pulping of this corpse, for you have no tool heavy enough to deal with it quickly." ),
                                   _( "You interrupted pulping of these corpses, for you have no tool heavy enough to deal with it quickly." ),
                                   unpulped_corpses_qty ) );
        return;
    }

    std::string tools;
    if( pd.stomps_only ) {
        tools = string_format( ", with nothing but your %s",
                               you.get_random_body_part_of_type( body_part_type::type::leg )->accusative_multiple );
    } else if( pd.weapon_only ) {
        tools = string_format( " with your %s", pd.bash_tool );
    } else {
        tools = string_format( " mixing your %s with powerful stomps", pd.bash_tool );
    }
    if( pd.can_cut_precisely ) {
        tools += string_format( " and a trusty %s", pd.cut_tool );
    }

    std::string skill;
    if( you.get_skill_level( skill_survival ) < 2 ) {
        skill = string_format( "even if your moves still lack the experience" );
        if( you.get_skill_level( skill_swimming ) < 2 ) {
            skill += " and your body is not fit";
        }
    } else if( you.get_skill_level( skill_swimming ) < 2 ) {
        skill = "even if your body is not fit";
    } else {
        skill = "";
    }

    std::string pry = ".";
    if( pd.pry_tool == pd.bash_tool ) {
        // use default assignment
    } else if( pd.used_pry ) {
        pry = string_format( ".  You also were lucky to carry %, it helped a lot with more armoured bodies.",
                             pd.pry_tool );
    } else if( pd.couldnt_use_pry ) {
        pry = string_format( ".  You wish you had anything like a crowbar, you had a really hard time without it." );
    }

    you.add_msg_player_or_npc(
        //~ %1$s is message about tools used in pulping, %2$s is a note if your skill is too low (optional, can be empty),
        //~ 3$s is message about you using/not being able to use prying tool (optional, can be `.`).
        //~ see pulp_activity_actor::send_final_message() for full context
        n_gettext( "You finished pulping the corpse%1$s, %2$s%3$s",
                   "You finished pulping the corpses%1$s, %2$s%3$s", unpulped_corpses_qty ),
        n_gettext( "<npcname> finished pulping the corpse.",
                   "<npcname> finished pulping the corpses.", unpulped_corpses_qty ),
        tools, skill, pry
    );

    if( way_too_long_to_pulp ) {
        you.add_msg_player_or_npc(
            n_gettext( "You left one corpse unpulped, for you have no tool heavy enough to deal with it.",
                       "You left some corpses unpulped, for you have no tool heavy enough to deal with it.",
                       unpulped_corpses_qty ),
            n_gettext( "<npcname> left one corpse unpulped, for %1$s has no tool heavy enough to deal with it.",
                       "<npcname> left some corpses unpulped, for %1$s has no tool heavy enough to deal with it.",
                       unpulped_corpses_qty ),
            you.male ? pgettext( "npc", "he" ) : pgettext( "npc", "she" )
        );
    }
}

void pulp_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "num_corpses", num_corpses );
    jsout.member( "placement", placement );
    jsout.member( "nominal_pulp_power", pd.nominal_pulp_power );
    jsout.member( "pulp_power", pd.pulp_power );
    jsout.member( "pulp_effort", pd.pulp_effort );
    jsout.member( "mess_radius", pd.mess_radius );
    jsout.member( "unpulped_corpses_qty", unpulped_corpses_qty );
    jsout.member( "can_pry_armor", pd.can_pry_armor );
    jsout.member( "too_long_to_pulp", too_long_to_pulp );
    jsout.member( "too_long_to_pulp_interrupted", too_long_to_pulp_interrupted );
    jsout.member( "way_too_long_to_pulp", way_too_long_to_pulp );
    jsout.member( "acid_corpse", acid_corpse );
    jsout.member( "stomps_only", pd.stomps_only );
    jsout.member( "weapon_only", pd.weapon_only );
    jsout.member( "can_cut_precisely", pd.can_cut_precisely );
    jsout.member( "used_pry", pd.used_pry );
    jsout.member( "couldnt_use_pry", pd.couldnt_use_pry );
    jsout.member( "bash_tool", pd.bash_tool );
    jsout.member( "cut_tool", pd.cut_tool );
    jsout.member( "pry_tool", pd.pry_tool );

    jsout.end_object();
}

std::unique_ptr<activity_actor> pulp_activity_actor::deserialize( JsonValue &jsin )
{
    pulp_activity_actor actor;

    JsonObject data = jsin.get_object();

    data.read( "num_corpses", actor.num_corpses );
    data.read( "placement", actor.placement );
    data.read( "pulp_power", actor.pd.pulp_power );
    // Backward compatibility introduced 2025-04-09
    if( !data.read( "nominal_pulp_power", actor.pd.nominal_pulp_power ) ) {
        actor.pd.nominal_pulp_power = actor.pd.pulp_power;
    }
    data.read( "pulp_effort", actor.pd.pulp_effort );
    data.read( "mess_radius", actor.pd.mess_radius );
    data.read( "unpulped_corpses_qty", actor.unpulped_corpses_qty );
    data.read( "can_pry_armor", actor.pd.can_pry_armor );
    data.read( "too_long_to_pulp", actor.too_long_to_pulp );
    data.read( "too_long_to_pulp_interrupted", actor.too_long_to_pulp_interrupted );
    data.read( "way_too_long_to_pulp", actor.way_too_long_to_pulp );
    data.read( "acid_corpse", actor.acid_corpse );
    data.read( "stomps_only", actor.pd.stomps_only );
    data.read( "weapon_only", actor.pd.weapon_only );
    data.read( "can_cut_precisely", actor.pd.can_cut_precisely );
    data.read( "used_pry", actor.pd.used_pry );
    data.read( "couldnt_use_pry", actor.pd.couldnt_use_pry );
    data.read( "bash_tool", actor.pd.bash_tool );
    data.read( "cut_tool", actor.pd.cut_tool );
    data.read( "pry_tool", actor.pd.pry_tool );

    return actor.clone();
}


void wait_stamina_activity_actor::start( player_activity &act, Character & )
{
    act.moves_total = calendar::INDEFINITELY_LONG;
    act.moves_left = calendar::INDEFINITELY_LONG;
}

void wait_stamina_activity_actor::do_turn( player_activity &act, Character &you )
{
    if( stamina_threshold != -1 ) {
        // Record intial stamina only when waiting until a given threshold.
        initial_stamina = you.get_stamina();
    }

    int stamina_max = stamina_threshold == -1 ? you.get_stamina_max() : stamina_threshold;
    if( you.get_stamina() >= stamina_max ) {
        finish( act, you );
    }
}

void wait_stamina_activity_actor::finish( player_activity &act, Character &you )
{
    if( stamina_threshold != -1 ) {
        const int stamina_initial = initial_stamina != -1 ? initial_stamina : you.get_stamina();
        if( you.get_stamina() < stamina_threshold && you.get_stamina() <= stamina_initial ) {
            debugmsg( "Failed to wait until stamina threshold %d reached, only at %d. You may not be regaining stamina.",
                      act.values.front(), you.get_stamina() );
        }
    } else if( you.get_stamina() < you.get_stamina_max() ) {
        you.add_msg_if_player( _( "You are bored of waiting, so you stop." ) );
    } else {
        you.add_msg_if_player( _( "You finish waiting and feel refreshed." ) );
    }
    act.set_to_null();
}

void wait_stamina_activity_actor::serialize( JsonOut &jsout ) const
{
    jsout.write_null();
}

std::unique_ptr<activity_actor> wait_stamina_activity_actor::deserialize( JsonValue & )
{
    return wait_stamina_activity_actor().clone();
}


namespace activity_actors
{

// Please keep this alphabetically sorted
const std::unordered_map<activity_id, std::unique_ptr<activity_actor>( * )( JsonValue & )>
deserialize_functions = {
    { ACT_AIM, &aim_activity_actor::deserialize },
    { ACT_AUTODRIVE, &autodrive_activity_actor::deserialize },
    { ACT_BASH, &bash_activity_actor::deserialize },
    { ACT_BIKERACK_RACKING, &bikerack_racking_activity_actor::deserialize },
    { ACT_BIKERACK_UNRACKING, &bikerack_unracking_activity_actor::deserialize },
    { ACT_BINDER_COPY_RECIPE, &bookbinder_copy_activity_actor::deserialize },
    { ACT_BOLTCUTTING, &boltcutting_activity_actor::deserialize },
    { ACT_CHOP_LOGS, &chop_logs_activity_actor::deserialize },
    { ACT_CHOP_PLANKS, &chop_planks_activity_actor::deserialize },
    { ACT_CHOP_TREE, &chop_tree_activity_actor::deserialize },
    { ACT_CHURN, &churn_activity_actor::deserialize },
    { ACT_CLEAR_RUBBLE, &clear_rubble_activity_actor::deserialize },
    { ACT_CONSUME, &consume_activity_actor::deserialize },
    { ACT_CRACKING, &safecracking_activity_actor::deserialize },
    { ACT_CRAFT, &craft_activity_actor::deserialize },
    { ACT_DISABLE, &disable_activity_actor::deserialize },
    { ACT_DISASSEMBLE, &disassemble_activity_actor::deserialize },
    { ACT_DROP, &drop_activity_actor::deserialize },
    { ACT_E_FILE, &efile_activity_actor::deserialize },
    { ACT_EBOOKSAVE, &ebooksave_activity_actor::deserialize },
    { ACT_FIRSTAID, &firstaid_activity_actor::deserialize },
    { ACT_FORAGE, &forage_activity_actor::deserialize },
    { ACT_FURNITURE_MOVE, &move_furniture_activity_actor::deserialize },
    { ACT_GLIDE, &glide_activity_actor::deserialize },
    { ACT_GUNMOD_ADD, &gunmod_add_activity_actor::deserialize },
    { ACT_GUNMOD_REMOVE, &gunmod_remove_activity_actor::deserialize },
    { ACT_HACKING, &hacking_activity_actor::deserialize },
    { ACT_HACKSAW, &hacksaw_activity_actor::deserialize },
    { ACT_HAIRCUT, &haircut_activity_actor::deserialize },
    { ACT_HARVEST, &harvest_activity_actor::deserialize},
    { ACT_HEATING, &heat_activity_actor::deserialize },
    { ACT_HOTWIRE_CAR, &hotwire_car_activity_actor::deserialize },
    { ACT_INSERT_ITEM, &insert_item_activity_actor::deserialize },
    { ACT_INVOKE_ITEM, &invoke_item_activity_actor::deserialize },
    { ACT_LOCKPICK, &lockpick_activity_actor::deserialize },
    { ACT_LONGSALVAGE, &longsalvage_activity_actor::deserialize },
    { ACT_MEDITATE, &meditate_activity_actor::deserialize },
    { ACT_MIGRATION_CANCEL, &migration_cancel_activity_actor::deserialize },
    { ACT_MILK, &milk_activity_actor::deserialize },
    { ACT_MOP, &mop_activity_actor::deserialize },
    { ACT_MOVE_ITEMS, &move_items_activity_actor::deserialize },
    { ACT_OPEN_GATE, &open_gate_activity_actor::deserialize },
    { ACT_OXYTORCH, &oxytorch_activity_actor::deserialize },
    { ACT_PICKUP, &pickup_activity_actor::deserialize },
    { ACT_PLAY_WITH_PET, &play_with_pet_activity_actor::deserialize },
    { ACT_PRYING, &prying_activity_actor::deserialize },
    { ACT_PULP, &pulp_activity_actor::deserialize },
    { ACT_READ, &read_activity_actor::deserialize },
    { ACT_REEL_CABLE, &reel_cable_activity_actor::deserialize },
    { ACT_RELOAD, &reload_activity_actor::deserialize },
    { ACT_SHAVE, &shave_activity_actor::deserialize },
    { ACT_SHEARING, &shearing_activity_actor::deserialize },
    { ACT_STASH, &stash_activity_actor::deserialize },
    { ACT_TENT_DECONSTRUCT, &tent_deconstruct_activity_actor::deserialize },
    { ACT_TENT_PLACE, &tent_placement_activity_actor::deserialize },
    { ACT_TRY_SLEEP, &try_sleep_activity_actor::deserialize },
    { ACT_UNLOAD, &unload_activity_actor::deserialize },
    { ACT_UNLOAD_LOOT, &unload_loot_activity_actor::deserialize },
    { ACT_VEHICLE_FOLD, &vehicle_folding_activity_actor::deserialize },
    { ACT_WAIT_STAMINA, &wait_stamina_activity_actor::deserialize },
    { ACT_VEHICLE_UNFOLD, &vehicle_unfolding_activity_actor::deserialize },
    { ACT_WASH, &wash_activity_actor::deserialize },
    { ACT_WEAR, &wear_activity_actor::deserialize },
    { ACT_WIELD, &wield_activity_actor::deserialize},
    { ACT_WORKOUT_ACTIVE, &workout_activity_actor::deserialize },
    { ACT_WORKOUT_HARD, &workout_activity_actor::deserialize },
    { ACT_WORKOUT_LIGHT, &workout_activity_actor::deserialize },
    { ACT_WORKOUT_MODERATE, &workout_activity_actor::deserialize },
};
} // namespace activity_actors

void serialize( const cata::clone_ptr<activity_actor> &actor, JsonOut &jsout )
{
    if( !actor ) {
        jsout.write_null();
    } else {
        jsout.start_object();

        jsout.member( "actor_type", actor->get_type() );
        jsout.member( "actor_data", *actor );

        jsout.end_object();
    }
}

void deserialize( cata::clone_ptr<activity_actor> &actor, const JsonValue &jsin )
{
    if( jsin.test_null() ) {
        actor = nullptr;
    } else {
        JsonObject data = jsin.get_object();
        if( data.has_member( "actor_data" ) ) {
            activity_id actor_type;
            data.read( "actor_type", actor_type );
            auto deserializer = activity_actors::deserialize_functions.find( actor_type );
            if( deserializer != activity_actors::deserialize_functions.end() ) {
                JsonValue jv = data.get_member( "actor_data" );
                actor = deserializer->second( jv );
            } else {
                debugmsg( "Failed to find activity actor deserializer for type \"%s\"", actor_type.c_str() );
                actor = nullptr;
            }
        } else {
            debugmsg( "Failed to load activity actor" );
            actor = nullptr;
        }
    }
}
