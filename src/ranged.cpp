#include "ranged.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "ballistics.h"
#include "bionics.h"
#include "bodypart.h"
#include "cached_options.h" // IWYU pragma: keep
#include "calendar.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "color.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "dispersion.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "gun_mode.h"
#include "input.h"
#include "input_context.h"
#include "input_enums.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_tname.h"
#include "itype.h"
#include "line.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "magic_type.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "math_defines.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "panels.h"
#include "pimpl.h"
#include "pocket_type.h"
#include "point.h"
#include "projectile.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "ui_manager.h"
#include "uilist.h"
#include "units.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weakpoint.h"

static const ammo_effect_str_id ammo_effect_ACT_ON_RANGED_HIT( "ACT_ON_RANGED_HIT" );
static const ammo_effect_str_id ammo_effect_BLACKPOWDER( "BLACKPOWDER" );
static const ammo_effect_str_id ammo_effect_BOUNCE( "BOUNCE" );
static const ammo_effect_str_id ammo_effect_BURST( "BURST" );
static const ammo_effect_str_id ammo_effect_CUSTOM_EXPLOSION( "CUSTOM_EXPLOSION" );
static const ammo_effect_str_id ammo_effect_EMP( "EMP" );
static const ammo_effect_str_id ammo_effect_EXPLOSIVE( "EXPLOSIVE" );
static const ammo_effect_str_id ammo_effect_HEAVY_HIT( "HEAVY_HIT" );
static const ammo_effect_str_id ammo_effect_IGNITE( "IGNITE" );
static const ammo_effect_str_id ammo_effect_LASER( "LASER" );
static const ammo_effect_str_id ammo_effect_LIGHTNING( "LIGHTNING" );
static const ammo_effect_str_id ammo_effect_MATCHHEAD( "MATCHHEAD" );
static const ammo_effect_str_id ammo_effect_NON_FOULING( "NON_FOULING" );
static const ammo_effect_str_id ammo_effect_NO_EMBED( "NO_EMBED" );
static const ammo_effect_str_id ammo_effect_NO_ITEM_DAMAGE( "NO_ITEM_DAMAGE" );
static const ammo_effect_str_id ammo_effect_PLASMA( "PLASMA" );
static const ammo_effect_str_id ammo_effect_SHATTER_SELF( "SHATTER_SELF" );
static const ammo_effect_str_id ammo_effect_SHOT( "SHOT" );
static const ammo_effect_str_id ammo_effect_TANGLE( "TANGLE" );
static const ammo_effect_str_id ammo_effect_WHIP( "WHIP" );
static const ammo_effect_str_id ammo_effect_WIDE( "WIDE" );

static const ammotype ammo_120mm( "120mm" );
static const ammotype ammo_12mm( "12mm" );
static const ammotype ammo_40x46mm( "40x46mm" );
static const ammotype ammo_40x53mm( "40x53mm" );
static const ammotype ammo_66mm( "66mm" );
static const ammotype ammo_84x246mm( "84x246mm" );
static const ammotype ammo_RPG_7( "RPG-7" );
static const ammotype ammo_arrow( "arrow" );
static const ammotype ammo_atgm( "atgm" );
static const ammotype ammo_atlatl( "atlatl" );
static const ammotype ammo_bolt( "bolt" );
static const ammotype ammo_bolt_ballista( "bolt_ballista" );
static const ammotype ammo_flammable( "flammable" );
static const ammotype ammo_homebrew_rocket( "homebrew_rocket" );
static const ammotype ammo_m235( "m235" );
static const ammotype ammo_metal_rail( "metal_rail" );
static const ammotype ammo_strange_arrow( "strange_arrow" );

static const bionic_id bio_railgun( "bio_railgun" );

static const character_modifier_id
character_modifier_melee_thrown_move_balance_mod( "melee_thrown_move_balance_mod" );
static const character_modifier_id
character_modifier_melee_thrown_move_lift_mod( "melee_thrown_move_lift_mod" );
static const character_modifier_id
character_modifier_ranged_dispersion_manip_mod( "ranged_dispersion_manip_mod" );
static const character_modifier_id character_modifier_thrown_dex_mod( "thrown_dex_mod" );

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_downed( "downed" );
static const efftype_id effect_hit_by_player( "hit_by_player" );
static const efftype_id effect_on_roof( "on_roof" );
static const efftype_id effect_quadruped_full( "quadruped_full" );

static const fault_id fault_gun_blackpowder( "fault_gun_blackpowder" );
static const fault_id fault_gun_chamber_spent( "fault_gun_chamber_spent" );
static const fault_id fault_gun_dirt( "fault_gun_dirt" );
static const fault_id fault_overheat_explosion( "fault_overheat_explosion" );
static const fault_id fault_overheat_melting( "fault_overheat_melting" );
static const fault_id fault_overheat_safety( "fault_overheat_safety" );
static const fault_id fault_overheat_venting( "fault_overheat_venting" );

static const flag_id json_flag_CANNOT_MOVE( "CANNOT_MOVE" );
static const flag_id json_flag_FILTHY( "FILTHY" );

static const material_id material_budget_steel( "budget_steel" );
static const material_id material_case_hardened_steel( "case_hardened_steel" );
static const material_id material_glass( "glass" );
static const material_id material_high_steel( "high_steel" );
static const material_id material_iron( "iron" );
static const material_id material_low_steel( "low_steel" );
static const material_id material_med_steel( "med_steel" );
static const material_id material_steel( "steel" );
static const material_id material_tempered_steel( "tempered_steel" );

static const morale_type morale_pyromania_nofire( "morale_pyromania_nofire" );
static const morale_type morale_pyromania_startfire( "morale_pyromania_startfire" );

static const proficiency_id proficiency_prof_bow_basic( "prof_bow_basic" );
static const proficiency_id proficiency_prof_bow_expert( "prof_bow_expert" );
static const proficiency_id proficiency_prof_bow_master( "prof_bow_master" );

static const skill_id skill_archery( "archery" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_driving( "driving" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_launcher( "launcher" );
static const skill_id skill_swimming( "swimming" );
static const skill_id skill_throw( "throw" );

static const trait_id trait_BRAWLER( "BRAWLER" );
static const trait_id trait_GUNSHY( "GUNSHY" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );

static const trap_str_id tr_practice_target( "tr_practice_target" );

static const std::string gun_mechanical_simple( "gun_mechanical_simple" );

static const std::set<material_id> ferric = { material_iron, material_steel, material_budget_steel, material_case_hardened_steel, material_high_steel, material_low_steel, material_med_steel, material_tempered_steel };

// Maximum duration of aim-and-fire loop, in turns
static constexpr int AIF_DURATION_LIMIT = 10;

static projectile make_gun_projectile( const item &gun );
static int time_to_attack( const Character &p, const itype &firing );
static int RAS_time( const Character &p, const item_location &loc );
/**
* Handle spent ammo casings and linkages.
* @param weap   Weapon.
* @param ammo   Ammo used.
* @param pos    Character position.
*/
static void cycle_action( item &weap, const itype_id &ammo, map *here, const tripoint_bub_ms &pos );
static void make_gun_sound_effect( const Character &p, bool burst, item *weapon );

class target_ui
{
    public:
        /* None of the public members (except ammo and range) should be modified during execution */

        enum class TargetMode : int {
            Fire,
            Throw,
            ThrowBlind,
            Turrets,
            TurretManual,
            Reach,
            Spell,
            SelectOnly
        };

        // Avatar
        avatar *you;
        // Interface mode
        TargetMode mode = TargetMode::Fire;
        // Weapon being fired/thrown
        item *relevant = nullptr;
        // Cached selection range from player's position
        int range = 0;
        // Cached current ammo to display
        const itype *ammo = nullptr;
        // Turret being manually fired
        turret_data *turret = nullptr;
        // Turrets being fired (via vehicle controls)
        const std::vector<vehicle_part *> *vturrets = nullptr;
        // Vehicle that turrets belong to
        vehicle *veh = nullptr;
        // Spell being cast
        spell *casting = nullptr;
        // Spell cannot fail
        bool no_fail = false;
        // Spell does not require mana
        bool no_mana = false;
        // Relevant activity
        aim_activity_actor *activity = nullptr;

        enum class ExitCode : int {
            Abort,
            Fire,
            Timeout,
            Reload
        };

        enum class LegalTargets : int {
            Creatures, // Only target creatures
            Vehicles, // Only target vehicles
            Both // Target either
        };
        LegalTargets targeting_Mode = LegalTargets::Creatures;

        // Initialize UI and run the event loop
        target_handler::trajectory run();

        // returns the currently selected aim type (immediate/careful/precise etc)
        aim_type get_selected_aim_type() const;

        int get_sight_dispersion() const;

    private:
        enum class Status : int {
            Good, // All UI elements are enabled
            BadTarget, // Bad 'dst' selected; forbid aiming/firing
            OutOfAmmo, // Selected gun mode is out of ammo; forbid moving cursor,aiming and firing
            OutOfRange // Selected target is out of range of current gun mode; forbid aiming/firing
        };

        // Ui status (affects which UI controls are temporarily disabled)
        Status status = Status::Good;

        // Current trajectory
        std::vector<tripoint_bub_ms> traj;
        // Aiming source (player's position)
        tripoint_bub_ms src;
        // Aiming destination (cursor position)
        // Use set_cursor_pos() to modify
        tripoint_bub_ms dst;
        // Creature currently under cursor. nullptr if aiming at empty tile,
        // yourself or a creature you cannot see, or targeting mode doesn't allow creatures
        Creature *dst_critter = nullptr;
        // Vehicle currently under cursor. nullptr if aiming at empty tile or if targeting mode doesn't allow vehicle targets
        vehicle *dst_veh = nullptr;
        // List of visible hostile targets
        std::vector<tripoint_bub_ms> targets;

        // Window
        catacurses::window w_target;
        // Input context
        input_context ctxt;

        /* These members are relevant for TargetMode::Fire */
        // Weapon sight dispersion
        int sight_dispersion = 0;
        // List of available weapon aim types
        std::vector<aim_type> aim_types;
        // Currently selected aim mode
        std::vector<aim_type>::iterator aim_mode;
        // 'Recoil' value the player will reach if they
        // start aiming at cursor position. Equals player's
        // 'recoil' while they are actively spending moves to aim,
        // but increases the further away the new aim point will be
        // relative to the current one.
        double predicted_recoil = 0;

        // For AOE spells, list of tiles affected by the spell
        // relevant for TargetMode::Spell
        std::set<tripoint_bub_ms> spell_aoe;

        // Represents a turret and a straight line from that turret to target
        struct turret_with_lof {
            vehicle_part *turret;
            std::vector<tripoint_bub_ms> line;
        };

        // List of vehicle turrets in range (out of those listed in 'vturrets')
        std::vector<turret_with_lof> turrets_in_range;

        // If true, draws turret lines
        // relevant for TargetMode::Turrets
        bool draw_turret_lines = false;
        // Snap camera to cursor. Can be permanently toggled in settings
        // or temporarily in this window
        bool snap_to_target = false;
        bool unload_RAS_weapon = true;
        // If true, LEVEL_UP, LEVEL_DOWN and directional keys
        // responsible for moving cursor will shift view instead.
        bool shifting_view = false;

        // Compact layout
        bool compact = false;
        // Tiny layout - when extremely short on height
        bool tiny = false;
        // Narrow layout - when short on width
        bool narrow = false;

        // Create window and set up input context
        void init_window_and_input();

        // Handle input related to cursor movement.
        // Returns 'true' if action was recognized and processed.
        // 'skip_redraw' is set to 'true' if there is no need to redraw the UI.
        bool handle_cursor_movement( const std::string &action, bool &skip_redraw );

        // Set cursor position. If new position is out of range,
        // selects closest position in range.
        // Returns 'false' if cursor position did not change
        bool set_cursor_pos( const tripoint_bub_ms &new_pos );

        // Called when range/ammo changes (or may have changed)
        void on_range_ammo_changed();

        // Updates 'targets' for current range
        void update_target_list();

        // Choose where to position the cursor when opening the ui
        tripoint_bub_ms choose_initial_target();

        /**
         * Try to re-acquire target for aim-and-fire.
         * @param critter whether were aiming at a critter, or a tile
         * @param new_dst where to move aim cursor (if e.g. critter moved)
         * @returns true on success
         */
        bool try_reacquire_target( bool critter, tripoint_bub_ms &new_dst );

        // Update 'status' variable
        void update_status();

        // Calculates distance from 'src'. For consistency, prefer using this over rl_dist.
        int dist_fn( const tripoint_bub_ms &p );

        // Set creature (or tile) under cursor as player's last target
        void set_last_target();

        // Prompts player to confirm attack on neutral NPC
        // Returns 'true' if attack should proceed
        bool confirm_non_enemy_target();

        // Prompts player to re-confirm an ongoing attack if
        // a non-hostile NPC / friendly creatures enters line of fire.
        // Returns 'true' if attack should proceed
        bool prompt_friendlies_in_lof();

        // List friendly creatures currently occupying line of fire.
        std::vector<weak_ptr_fast<Creature>> list_friendlies_in_lof();

        // Toggle snap-to-target
        void toggle_snap_to_target();

        // Cycle targets. 'direction' is either 1 or -1
        void cycle_targets( int direction );

        // Set new view offset. Updates map cache if necessary
        void set_view_offset( const tripoint_rel_ms &new_offset ) const;

        // Updates 'turrets_in_range'
        void update_turrets_in_range();

        // Recalculate 'recoil' penalty. This should be called if
        // avatar's 'recoil' value has been modified
        // Relevant for TargetMode::Fire
        void recalc_aim_turning_penalty();

        // Apply penalty to avatar's 'recoil' value based on
        // how much they moved their aim point.
        // Relevant for TargetMode::Fire
        void apply_aim_turning_penalty() const;

        // Update range & ammo from current gun mode
        void update_ammo_range_from_gun_mode();

        // Switch firing mode.
        bool action_switch_mode();

        // Switch ammo. Returns 'false' if requires a reloading UI.
        bool action_switch_ammo();

        // Aim for 10 turns. Returns 'false' if ran out of moves
        bool action_aim();

        // drop aim so you can look around
        bool action_drop_aim();

        // Aim and shoot. Returns 'false' if ran out of moves
        bool action_aim_and_shoot( const std::string &action );

        // Draw UI-specific terrain overlays
        void draw_terrain_overlay();

        // Draw aiming window
        void draw_ui_window();

        // Generate ui window title
        std::string uitext_title() const;

        // Generate flavor text for 'Fire!' key
        std::string uitext_fire() const;

        void draw_window_title();
        void draw_help_notice();

        // Draw list of available controls at the bottom of the window.
        // text_y - first free line counting from the top
        void draw_controls_list( int text_y );

        void panel_cursor_info( int &text_y );
        void panel_gun_info( int &text_y );
        void panel_recoil( int &text_y );
        void panel_spell_info( int &text_y );
        void panel_target_info( int &text_y, bool fill_with_blank_if_no_target );
        void panel_turret_list( int &text_y );

        // On-selected-as-target checks that act as if they are on-hit checks.
        // `harmful` is `false` if using a non-damaging spell
        void on_target_accepted( bool harmful ) const;
};

target_handler::trajectory target_handler::mode_select_only( avatar &you, int range )
{
    target_ui ui = target_ui();
    ui.you = &you;
    ui.mode = target_ui::TargetMode::SelectOnly;
    ui.range = range;

    restore_on_out_of_scope view_offset_prev( you.view_offset );
    return ui.run();
}

target_handler::trajectory target_handler::mode_fire( avatar &you, aim_activity_actor &activity )
{
    target_ui ui = target_ui();
    ui.you = &you;
    ui.mode = target_ui::TargetMode::Fire;
    ui.activity = &activity;
    ui.relevant = &*activity.get_weapon();
    gun_mode gun = ui.relevant->gun_current_mode();
    ui.range = gun.target->gun_range( &you );
    ui.ammo = gun->ammo_data();

    return ui.run();
}

target_handler::trajectory target_handler::mode_throw( avatar &you, item &relevant,
        bool blind_throwing )
{
    target_ui ui = target_ui();
    ui.you = &you;
    ui.mode = blind_throwing ? target_ui::TargetMode::ThrowBlind : target_ui::TargetMode::Throw;
    ui.relevant = &relevant;
    ui.range = you.throw_range( relevant );

    restore_on_out_of_scope view_offset_prev( you.view_offset );
    return ui.run();
}

target_handler::trajectory target_handler::mode_reach( avatar &you, item_location weapon )
{
    target_ui ui = target_ui();
    ui.you = &you;
    ui.mode = target_ui::TargetMode::Reach;
    ui.relevant = weapon.get_item();
    ui.range = weapon ? weapon->current_reach_range( you ) : 1;

    restore_on_out_of_scope view_offset_prev( you.view_offset );
    return ui.run();
}

target_handler::trajectory target_handler::mode_unarmed_reach( avatar &you )
{
    target_ui ui = target_ui();
    ui.you = &you;
    ui.mode = target_ui::TargetMode::Reach;
    ui.range = std::max( 1, static_cast<int>( you.calculate_by_enchantment( 1,
                         enchant_vals::mod::MELEE_RANGE_MODIFIER ) ) );

    restore_on_out_of_scope view_offset_prev( you.view_offset );
    return ui.run();
}

target_handler::trajectory target_handler::mode_turret_manual( avatar &you, turret_data &turret )
{
    target_ui ui = target_ui();
    ui.you = &you;
    ui.mode = target_ui::TargetMode::TurretManual;
    ui.turret = &turret;
    ui.relevant = &*turret.base();
    ui.range = turret.range();
    ui.ammo = turret.ammo_data();

    restore_on_out_of_scope view_offset_prev( you.view_offset );
    return ui.run();
}

target_handler::trajectory target_handler::mode_turrets( avatar &you, vehicle &veh,
        const std::vector<vehicle_part *> &turrets )
{
    map &here = get_map();
    // Find radius of a circle centered at u encompassing all points turrets can aim at
    // FIXME: this calculation is fine for square distances, but results in an underestimation
    //        when used with real circles
    int range_total = 0;
    for( vehicle_part *t : turrets ) {
        int range = veh.turret_query( *t ).range();
        tripoint_bub_ms pos = veh.bub_part_pos( here, *t );

        int res = 0;
        res = std::max( res, rl_dist( you.pos_bub(), pos + point( range, 0 ) ) );
        res = std::max( res, rl_dist( you.pos_bub(), pos + point( -range, 0 ) ) );
        res = std::max( res, rl_dist( you.pos_bub(), pos + point( 0, range ) ) );
        res = std::max( res, rl_dist( you.pos_bub(), pos + point( 0, -range ) ) );
        range_total = std::max( range_total, res );
    }

    target_ui ui = target_ui();
    ui.you = &you;
    ui.mode = target_ui::TargetMode::Turrets;
    ui.veh = &veh;
    ui.vturrets = &turrets;
    ui.range = range_total;

    restore_on_out_of_scope view_offset_prev( you.view_offset );
    return ui.run();
}

target_handler::trajectory target_handler::mode_spell( avatar &you, spell &casting, bool no_fail,
        bool no_mana )
{
    target_ui ui = target_ui();
    ui.you = &you;
    ui.mode = target_ui::TargetMode::Spell;
    ui.casting = &casting;
    ui.range = casting.range( you );
    ui.no_fail = no_fail;
    ui.no_mana = no_mana;

    bool target_Creatures = casting.is_valid_target( spell_target::ally ) ||
                            casting.is_valid_target( spell_target::hostile ) ||
                            casting.is_valid_target( spell_target::self );
    bool target_Vehicles = casting.is_valid_target( spell_target::vehicle );
    if( target_Creatures && target_Vehicles ) {
        ui.targeting_Mode = target_ui::LegalTargets::Both;
    } else if( target_Vehicles ) {
        ui.targeting_Mode = target_ui::LegalTargets::Vehicles;
    }

    restore_on_out_of_scope view_offset_prev( you.view_offset );
    return ui.run();
}

double occupied_tile_fraction( creature_size target_size )
{
    switch( target_size ) {
        case creature_size::tiny:
            return 0.1;
        case creature_size::small:
            return 0.25;
        case creature_size::medium:
            return 0.5;
        case creature_size::large:
            return 0.75;
        case creature_size::huge:
            return 1.0;
        case creature_size::num_sizes:
            debugmsg( "ERROR: Invalid Creature size class." );
            break;
    }

    return 0.5;
}

double Creature::ranged_target_size() const
{
    double stance_factor = 1.0;
    if( const Character *character = this->as_character() ) {
        if( character->is_crouching() || ( character->has_effect( effect_quadruped_full ) &&
                                           character->is_running() ) ) {
            stance_factor = 0.6;
        } else if( character->is_prone() ) {
            stance_factor = 0.25;
        }
    }
    if( has_flag( mon_flag_HARDTOSHOOT ) ) {
        switch( get_size() ) {
            case creature_size::tiny:
            case creature_size::small:
                return stance_factor * occupied_tile_fraction( creature_size::tiny );
            case creature_size::medium:
                return stance_factor * occupied_tile_fraction( creature_size::small );
            case creature_size::large:
                return stance_factor * occupied_tile_fraction( creature_size::medium );
            case creature_size::huge:
                return stance_factor * occupied_tile_fraction( creature_size::large );
            case creature_size::num_sizes:
                debugmsg( "ERROR: Invalid Creature size class." );
                break;
        }
    }
    return stance_factor * occupied_tile_fraction( get_size() );
}

int range_with_even_chance_of_good_hit( int dispersion )
{
    int even_chance_range = 0;
    while( static_cast<unsigned>( even_chance_range ) <
           Creature::dispersion_for_even_chance_of_good_hit.size() &&
           dispersion < Creature::dispersion_for_even_chance_of_good_hit[ even_chance_range ] ) {
        even_chance_range++;
    }
    return even_chance_range;
}

dispersion_sources Character::total_gun_dispersion( const item &gun, double recoil,
        int spread ) const
{
    dispersion_sources dispersion = get_weapon_dispersion( gun );
    dispersion.add_range( recoil );
    dispersion.add_spread( spread );
    return dispersion;
}

int Character::gun_engagement_moves( const item &gun, int target, int start,
                                     const Target_attributes &attributes ) const
{
    int mv = 0;
    double penalty = start;
    const aim_mods_cache aim_cache = gen_aim_mods_cache( gun );
    while( penalty > target ) {
        const double adj = aim_per_move( gun, penalty, attributes, { std::ref( aim_cache ) } );
        if( adj <= MIN_RECOIL_IMPROVEMENT ) {
            break;
        }
        penalty -= adj;
        mv++;
    }

    return mv;
}

bool Character::handle_gun_damage( item &it )
{
    // Below item (maximum dirt possible) should be greater than or equal to dirt range in item_group.cpp. Also keep in mind that monster drops can have specific ranges and these should be below the max!
    const double dirt_max_dbl = 10000;
    if( !it.is_gun() ) {
        debugmsg( "Tried to handle_gun_damage of a non-gun %s", it.tname() );
        return false;
    }

    int dirt = it.get_var( "dirt", 0 );
    int dirtadder = 0;
    double dirt_dbl = static_cast<double>( dirt );

    const auto &curammo_effects = it.ammo_effects();
    const islot_gun &firing = *it.type->gun;
    // misfire chance based on dirt accumulation. Formula is designed to make chance of jam highly unlikely at low dirt levels, but levels increase geometrically as the dirt level reaches max (10,000). The number used is just a figure I found reasonable after plugging the number into excel and changing it until the probability made sense at high, medium, and low levels of dirt.
    if( !it.has_flag( flag_NEVER_JAMS ) &&
        x_in_y( dirt_dbl * dirt_dbl * dirt_dbl,
                1000000000000.0 ) ) {
        add_msg_player_or_npc(
            _( "Your %s misfires with a muffled click!" ),
            _( "<npcname>'s %s misfires with a muffled click!" ),
            it.tname() );
        // at high dirt levels the chance to misfire gets to significant levels. 10,000 is max and 7800 is quite high so above that the player gets some relief in the form of exchanging time for some dirt reduction. Basically jiggling the parts loose to remove some dirt and get a few more shots out.
        if( dirt_dbl >
            7800 ) {
            add_msg_player_or_npc(
                _( "Perhaps taking the ammo out of your %s and reloading will help." ),
                _( "Perhaps taking the ammo out of <npcname>'s %s and reloading will help." ),
                it.tname() );
        }
        return false;
    }


    // i am bad at math, so we will use vibes instead
    double gun_jam_chance;
    int gun_damage = it.damage() / 1000.0;
    switch( gun_damage ) {
        case 0:
            gun_jam_chance = 0.0005 * firing.gun_jam_mult;
            break;
        case 1:
            gun_jam_chance = 0.03 * firing.gun_jam_mult;
            break;
        case 2:
            gun_jam_chance = 0.15 * firing.gun_jam_mult;
            break;
        case 3:
            gun_jam_chance = 0.45 * firing.gun_jam_mult;
            break;
        case 4:
            gun_jam_chance = 0.8 * firing.gun_jam_mult;
            break;
    }

    int mag_damage;
    double mag_jam_chance = 0;
    if( it.magazine_current() ) {
        mag_damage = it.magazine_current()->damage() / 1000.0;
        switch( mag_damage ) {
            case 0:
                mag_jam_chance = 0.00027 * it.magazine_current()->type->magazine->mag_jam_mult;
                break;
            case 1:
                mag_jam_chance = 0.05 * it.magazine_current()->type->magazine->mag_jam_mult;
                break;
            case 2:
                mag_jam_chance = 0.24 * it.magazine_current()->type->magazine->mag_jam_mult;
                break;
            case 3:
                mag_jam_chance = 0.96 * it.magazine_current()->type->magazine->mag_jam_mult;
                break;
            case 4:
                mag_jam_chance = 2.5 * it.magazine_current()->type->magazine->mag_jam_mult;
                break;
        }
    }

    const double jam_chance = gun_jam_chance + mag_jam_chance;

    add_msg_debug( debugmode::DF_RANGED,
                   "Gun jam chance: %s\nMagazine jam chance: %s\nGun damage level: %d\nMagazine damage level: %d\nFail to feed chance: %s",
                   gun_jam_chance, mag_jam_chance, gun_damage, mag_damage, jam_chance );

    // Here we check if we're underwater and whether we should misfire.
    // As a result this causes no damage to the firearm, note that some guns are waterproof
    // and so are immune to this effect, note also that WATERPROOF_GUN status does not
    // mean the gun will actually be accurate underwater.
    int effective_durability = firing.durability;
    if( is_underwater() && !it.has_flag( flag_WATERPROOF_GUN ) && one_in( effective_durability ) ) {
        add_msg_player_or_npc( _( "Your %s misfires with a wet click!" ),
                               _( "<npcname>'s %s misfires with a wet click!" ),
                               it.tname() );
        return false;
        // Here we check for a chance for the weapon to suffer a mechanical malfunction.
        // Note that some weapons never jam up 'NEVER_JAMS' and thus are immune to this
        // effect as current guns have a durability between 5 and 9 this results in
        // a chance of mechanical failure between 1/(64*3) and 1/(1024*3) on any given shot.
        // the malfunction can't cause damage
    } else if( one_in( ( 2 << effective_durability ) * 3 ) && !it.has_flag( flag_NEVER_JAMS ) ) {
        add_msg_player_or_npc( _( "Your %s malfunctions!" ),
                               _( "<npcname>'s %s malfunctions!" ),
                               it.tname() );
        return false;

        // Chance for the weapon to suffer a failure, caused by the magazine size, quality, or condition
    } else if( x_in_y( jam_chance, 1 ) && !it.has_var( "u_know_round_in_chamber" ) ) {
        add_msg_player_or_npc( m_bad, _( "Your %s malfunctions!" ),
                               _( "<npcname>'s %s malfunctions!" ),
                               it.tname() );
        it.set_random_fault_of_type( gun_mechanical_simple );
        return false;

        // Here we check for a chance for attached mods to get damaged if they are flagged as 'CONSUMABLE'.
        // This is mostly for crappy handmade expedient stuff  or things that rarely receive damage during normal usage.
        // Default chance is 1/10000 unless set via json, damage is proportional to caliber(see below).
        // Can be toned down with 'consume_divisor.'

    } else if( it.has_flag( flag_CONSUMABLE ) && !curammo_effects.count( ammo_effect_LASER ) &&
               !curammo_effects.count( ammo_effect_PLASMA ) && !curammo_effects.count( ammo_effect_EMP ) ) {
        int uncork = ( ( 10 * it.ammo_data()->ammo->loudness )
                       + ( it.ammo_data()->ammo->recoil / 2 ) ) / 100;
        uncork = std::pow( uncork, 3 ) * 6.5;
        for( item *mod : it.gunmods() ) {
            if( mod->has_flag( flag_CONSUMABLE ) ) {
                int dmgamt = uncork / mod->type->gunmod->consume_divisor;
                int modconsume = mod->type->gunmod->consume_chance;
                int initstate = it.damage();
                // fuzz damage if it's small
                if( dmgamt < 1000 ) {
                    dmgamt = rng( dmgamt, dmgamt + 200 );
                    // ignore damage if inconsequential.
                }
                if( dmgamt < 800 ) {
                    dmgamt = 0;
                }
                if( one_in( modconsume ) ) {
                    if( mod->mod_damage( dmgamt ) ) {
                        add_msg_player_or_npc( m_bad, _( "Your attached %s is destroyed by your shot!" ),
                                               _( "<npcname>'s attached %s is destroyed by their shot!" ),
                                               mod->tname() );
                        i_rem( mod );
                    } else if( it.damage() > initstate ) {
                        add_msg_player_or_npc( m_bad, _( "Your attached %s is damaged by your shot!" ),
                                               _( "<npcname>'s %s is damaged by their shot!" ),
                                               mod->tname() );
                    }
                }
            }
        }
    }
    if( it.has_fault_flag( "UNLUBRICATED" ) &&
        x_in_y( dirt_dbl, dirt_max_dbl ) ) {
        add_msg_player_or_npc( m_bad, _( "Your %s emits a grimace-inducing screech!" ),
                               _( "<npcname>'s %s emits a grimace-inducing screech!" ),
                               it.tname() );
        it.inc_damage();
    }
    if( !it.has_flag( flag_PRIMITIVE_RANGED_WEAPON ) ) {
        if( it.has_ammo() && ( ( it.ammo_data()->ammo->recoil < it.min_cycle_recoil() ) ||
                               ( it.has_fault_flag( "BAD_CYCLING" ) && one_in( 16 ) ) ) &&
            it.faults_potential().count( fault_gun_chamber_spent ) ) {
            add_msg_player_or_npc( m_bad, _( "Your %s fails to cycle!" ),
                                   _( "<npcname>'s %s fails to cycle!" ),
                                   it.tname() );
            it.set_fault( fault_gun_chamber_spent );
            // Don't return false in this case; this shot happens, follow-up ones won't.
        }
        // These are the dirtying/fouling mechanics
        if( !curammo_effects.count( ammo_effect_NON_FOULING ) && !it.has_flag( flag_NON_FOULING ) ) {
            if( dirt < static_cast<int>( dirt_max_dbl ) ) {
                dirtadder = curammo_effects.count( ammo_effect_BLACKPOWDER ) * ( 200 -
                            firing.blackpowder_tolerance *
                            2 );
                // dirtadder is the dirt-increasing number for shots fired with gunpowder-based ammo. Usually dirt level increases by 1, unless it's blackpowder, in which case it increases by a higher number, but there is a reduction for blackpowder resistance of a weapon.
                if( dirtadder < 0 ) {
                    dirtadder = 0;
                }
                // in addition to increasing dirt level faster, regular gunpowder fouling is also capped at 7,150, not 10,000. So firing with regular gunpowder can never make the gun quite as bad as firing it with black gunpowder. At 7,150 the chance to jam is significantly lower (though still significant) than it is at 10,000, the absolute cap.
                if( curammo_effects.count( ammo_effect_BLACKPOWDER ) ||
                    dirt < 7150 ) {
                    it.set_var( "dirt", std::min( static_cast<int>( dirt_max_dbl ), dirt + dirtadder + 1 ) );
                }
            }
            dirt = it.get_var( "dirt", 0 );
            dirt_dbl = static_cast<double>( dirt );
            if( dirt > 0 && !it.has_fault_flag( "NO_DIRTYING" ) ) {
                it.set_fault( fault_gun_dirt );
            }
            if( dirt > 0 && curammo_effects.count( ammo_effect_BLACKPOWDER ) ) {
                it.remove_fault( fault_gun_dirt );
                it.set_fault( fault_gun_blackpowder );
            }
            // end fouling mechanics
        }
    }
    // chance to damage gun due to high levels of dirt. Very unlikely, especially at lower levels and impossible below 5,000. Lower than the chance of a jam at the same levels. 555555... is an arbitrary number that I came up with after playing with the formula in excel. It makes sense at low, medium, and high levels of dirt.
    // if you have a bullet loaded with match head powder this can happen randomly at any time. The chances are about the same as a recycled ammo jamming the gun
    if( ( dirt_dbl > 5000 && x_in_y( dirt_dbl * dirt_dbl * dirt_dbl, 5555555555555 ) ) ||
        ( curammo_effects.count( ammo_effect_MATCHHEAD ) && one_in( 256 ) ) ) {
        add_msg_player_or_npc( m_bad, _( "Your %s is damaged by the high pressure!" ),
                               _( "<npcname>'s %s is damaged by the high pressure!" ),
                               it.tname() );
        // Don't increment until after the message
        it.inc_damage();
    }

    if( it.has_var( "u_know_round_in_chamber" ) ) {
        it.erase_var( "u_know_round_in_chamber" );
    }

    return true;
}

bool Character::handle_gun_overheat( item &it )
{
    double overheat_modifier = 0;
    float overheat_multiplier = 1.0f;
    double heat_modifier = 0;
    float heat_multiplier = 1.0f;
    for( const item *mod : it.gunmods() ) {
        overheat_modifier += mod->type->gunmod->overheat_threshold_modifier;
        overheat_multiplier *= mod->type->gunmod->overheat_threshold_multiplier;
        heat_modifier += mod->type->gunmod->heat_per_shot_modifier;
        heat_multiplier *= mod->type->gunmod->heat_per_shot_multiplier;
    }
    double heat = it.get_var( "gun_heat", 0.0 );
    double threshold = std::max( ( it.type->gun->overheat_threshold * overheat_multiplier ) +
                                 overheat_modifier, 5.0 );
    const islot_gun &gun_type = *it.type->gun;

    if( threshold < 0.0 ) {
        return true;
    }

    if( heat > threshold ) {
        if( it.faults_potential().count( fault_overheat_safety ) && !one_in( gun_type.durability ) ) {
            add_msg_if_player( m_bad,
                               _( "Your %s displays a warning sequence as its active cooling cycle engages." ),
                               it.tname() );
            it.set_fault( fault_overheat_safety );
            return false;
        }

        //Overall the durability of the gun greatly conditions what sort of failures are possible.
        //A durability above 8 prevents the most serious failures
        int fault_roll = rng( 5, 15 ) - gun_type.durability;
        if( it.faults_potential().count( fault_overheat_explosion ) && fault_roll > 9 ) {
            add_msg_if_player( m_bad,
                               _( "Your %s revs and chokes violently as its internal containment fields detune!" ),
                               it.tname() );
            add_msg_if_player( m_bad, _( "Your %s detonates!" ),
                               it.tname() );
            it.set_fault( fault_overheat_melting );
            explosion_handler::explosion( this, this->pos_bub(), 1200, 0.4 );
            return false;
        } else if( it.faults_potential().count( fault_overheat_melting ) && fault_roll > 6 ) {
            add_msg_if_player( m_bad, _( "Acrid smoke pours from your %s as its internals fuse together." ),
                               it.tname() );
            it.set_fault( fault_overheat_melting );
            return false;
        } else if( it.faults_potential().count( fault_overheat_venting ) && fault_roll > 2 ) {
            map &here = get_map();
            add_msg_if_player( m_bad,
                               _( "The cooling system of your %s chokes and vents a dense cloud of superheated coolant." ),
                               it.tname() );
            for( int i = 0; i < 3; i++ ) {
                here.add_field( pos_bub() + point( rng( -1, 1 ), rng( -1, 1 ) ), field_type_id( "fd_nuke_gas" ),
                                3 );
            }
            it.set_var( "gun_heat", heat - gun_type.cooling_value * 4.0 );
            return false;
        }
    }
    it.set_var( "gun_heat", heat + std::max( ( gun_type.heat_per_shot * heat_multiplier ) +
                heat_modifier, 0.0 ) );
    return true;
}

void npc::pretend_fire( npc *source, int shots, item &gun )
{
    int curshot = 0;
    if( one_in( 50 ) ) {
        add_msg_if_player_sees( *source, m_info, _( "%s shoots something." ), source->disp_name() );
    }
    while( curshot != shots ) {
        const int required = gun.ammo_required();
        if( gun.ammo_consume( required, pos_bub(), this ) != required ) {
            debugmsg( "Unexpected shortage of ammo whilst firing %s", gun.tname().c_str() );
            break;
        }

        item *weapon = &gun;
        const item::sound_data data = weapon->gun_noise( shots > 1 );

        add_msg_if_player_sees( *source, m_warning, _( "You hear %s." ), data.sound );
        curshot++;
        mod_moves( -get_speed() );
    }
}

int Character::fire_gun( const tripoint_bub_ms &target, int shots )
{
    map &here = get_map();

    item_location gun = get_wielded_item();
    if( !gun ) {
        debugmsg( "%s doesn't have a gun to fire", get_name() );
        return 0;
    }
    return fire_gun( here, target, shots, *gun, item_location() );
}

int Character::fire_gun( map &here, const tripoint_bub_ms &target, int shots, item &gun,
                         item_location ammo )
{
    if( !gun.is_gun() ) {
        debugmsg( "%s tried to fire non-gun (%s).", get_name(), gun.tname() );
        return 0;
    }
    if( gun.has_flag( flag_CHOKE ) && !gun.ammo_effects().count( ammo_effect_SHOT ) ) {
        add_msg_if_player( _( "A shotgun equipped with choke cannot fire slugs." ) );
        return 0;
    }
    if( gun.ammo_required() > 0 && !gun.ammo_remaining( ) && !ammo ) {
        debugmsg( "%s is empty and has no ammo for reloading.", gun.tname() );
        return 0;
    }
    bool is_mech_weapon = false;
    if( is_mounted() &&
        mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) ) {
        is_mech_weapon = true;
    }

    // cap our maximum burst size by ammo and energy
    if( !gun.has_flag( flag_VEHICLE ) && !ammo ) {
        shots = std::min( shots, gun.shots_remaining( here, this ) );
    } else if( gun.ammo_required() ) {
        // This checks ammo only. Vehicle turret energy drain is handled elsewhere.
        const int ammo_left = ammo ? ammo.get_item()->count() : gun.ammo_remaining( );
        shots = std::min( shots, ammo_left / gun.ammo_required() );
    }

    if( shots <= 0 ) {
        debugmsg( "Attempted to fire zero or negative shots using %s", gun.tname() );
        return 0;
    }

    // usage of any attached bipod is dependent upon terrain or on being prone
    bool bipod = here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_MOUNTABLE, pos_bub( here ) ) ||
                 is_prone();
    if( !bipod ) {
        if( const optional_vpart_position vp = here.veh_at( pos_abs( ) ) ) {
            bipod = vp->vehicle().has_part( pos_abs( ), "MOUNTABLE" );
        }
    }

    // Up to 50% of recoil can be delayed until end of burst dependent upon relevant skill
    /** @EFFECT_PISTOL delays effects of recoil during automatic fire */
    /** @EFFECT_SMG delays effects of recoil during automatic fire */
    /** @EFFECT_RIFLE delays effects of recoil during automatic fire */
    /** @EFFECT_SHOTGUN delays effects of recoil during automatic fire */
    double absorb = std::min( get_skill_level( gun.gun_skill() ),
                              static_cast<float>( MAX_SKILL ) ) / static_cast<double>( MAX_SKILL * 2 );

    itype_id gun_id = gun.typeId();
    int attack_moves = time_to_attack( *this, *gun_id ) + RAS_time( *this, ammo );
    skill_id gun_skill = gun.gun_skill();
    add_msg_debug( debugmode::DF_RANGED, "Gun skill (%s) %g", gun_skill.c_str(),
                   get_skill_level( gun_skill ) ) ;
    tripoint_bub_ms aim = target;
    int curshot = 0;
    int hits = 0; // total shots on target
    int delay = 0; // delayed recoil that has yet to be applied
    while( curshot != shots ) {
        // Special handling for weapons where we supply the ammo separately (i.e. ammo is populated)
        // instead of it being loaded into the weapon, reload right before firing.
        if( !!ammo && !gun.ammo_remaining( ) ) {
            Character &you = get_avatar();
            gun.reload( you, ammo, 1 );
            you.burn_energy_arms( - gun.get_min_str() * static_cast<int>( 0.006f *
                                  get_option<int>( "PLAYER_MAX_STAMINA_BASE" ) ) );
        }

        if( !handle_gun_damage( gun ) ) {
            break;
        }
        if( !handle_gun_overheat( gun ) ) {
            break;
        }

        // If this is a vehicle mounted turret, which vehicle is it mounted on?
        const vehicle *in_veh = has_effect( effect_on_roof ) ? veh_pointer_or_null( here.veh_at(
                                    pos_bub( here ) ) ) : nullptr;

        // Add gunshot noise
        make_gun_sound_effect( *this, shots > 1, &gun );
        sfx::generate_gun_sound( *this, gun );

        weakpoint_attack wp_attack;
        wp_attack.weapon = &gun;
        // get ammo_id in gun for event character_ranged_attacks_monster. If no ammo, use itype_id::NULL_ID()
        itype_id projectile_use_ammo_id = gun.has_ammo_data() ? gun.ammo_data()->get_id() :
                                          itype_id::NULL_ID();
        projectile proj = make_gun_projectile( gun );

        for( damage_unit &elem : proj.impact.damage_units ) {
            elem.amount = enchantment_cache->modify_value( enchant_vals::mod::RANGED_DAMAGE, elem.amount );
            elem.res_pen = enchantment_cache->modify_value( enchant_vals::mod::RANGED_ARMOR_PENETRATION,
                           elem.res_pen );
        }

        dispersion_sources dispersion = total_gun_dispersion( gun, recoil_total(), proj.shot_spread );

        dealt_projectile_attack shot;
        projectile_attack( shot, proj, &here, pos_bub( here ), aim, dispersion, this, in_veh, wp_attack );
        if( !shot.targets_hit.empty() ) {
            hits++;
        }

        for( std::pair<Creature *const, std::pair<int, int>> &hit_entry : shot.targets_hit ) {
            if( hit_entry.second.first == 0 ) {
                continue;
            }
            if( monster *const m = hit_entry.first->as_monster() ) {
                cata::event e = cata::event::make<event_type::character_ranged_attacks_monster>( getID(), gun_id,
                                projectile_use_ammo_id,
                                false,
                                m->type->id );
                get_event_bus().send_with_talker( this, m, e );
            } else if( Character *const c = hit_entry.first->as_character() ) {
                cata::event e = cata::event::make<event_type::character_ranged_attacks_character>( getID(), gun_id,
                                projectile_use_ammo_id,
                                false,
                                c->getID(), c->get_name() );
                get_event_bus().send_with_talker( this, c, e );
            }
            if( shot.proj.multishot ) {
                // TODO: Pull projectile name from the ammo entry.
                multi_projectile_hit_message( hit_entry.first, hit_entry.second.first, hit_entry.second.second,
                                              n_gettext( "projectile", "projectiles", hit_entry.second.first ) );
            }
        }
        if( shot.headshot ) {
            get_event_bus().send<event_type::character_gets_headshot>( getID() );
        }
        curshot++;

        if( !gun.is_gun() ) {
            // If we lose our gun as a side effect of firing it, skip the rest of the loop body.
            break;
        }

        int qty = gun.gun_recoil( *this, bipod );
        delay  += qty * absorb;
        // Temporarily scale by 5x as we adjust MAX_RECOIL, factoring in the recoil enchantment also.
        recoil += enchantment_cache->modify_value( enchant_vals::mod::RECOIL_MODIFIER, 5.0 ) *
                  ( qty * ( 1.0 - absorb ) );

        const itype_id current_ammo = gun.ammo_current();

        if( has_trait( trait_PYROMANIA ) && !has_morale( morale_pyromania_startfire ) ) {
            const std::set<ammotype> &at = gun.ammo_types();
            if( at.count( ammo_flammable ) ) {
                add_msg_if_player( m_good, _( "You feel a surge of euphoria as flames roar out of the %s!" ),
                                   gun.tname() );
                add_morale( morale_pyromania_startfire, 15, 15, 8_hours, 6_hours );
                rem_morale( morale_pyromania_nofire );
            } else if( at.count( ammo_66mm ) || at.count( ammo_120mm ) || at.count( ammo_84x246mm ) ||
                       at.count( ammo_m235 ) || at.count( ammo_atgm ) || at.count( ammo_RPG_7 ) ||
                       at.count( ammo_homebrew_rocket ) ) {
                add_msg_if_player( m_good, _( "You feel a surge of euphoria as flames burst out!" ) );
                add_morale( morale_pyromania_startfire, 15, 15, 8_hours, 6_hours );
                rem_morale( morale_pyromania_nofire );
            }
        }

        const int required = gun.ammo_required();
        if( gun.ammo_consume( required, here, pos_bub( here ), this ) != required ) {
            debugmsg( "Unexpected shortage of ammo whilst firing %s", gun.tname() );
            break;
        }

        // Vehicle turrets drain vehicle battery and do not care about this
        if( !gun.has_flag( flag_VEHICLE ) ) {
            const units::energy energ_req = gun.get_gun_energy_drain();
            const units::energy drained = gun.energy_consume( energ_req, &here, pos_bub( here ), this );
            if( drained < energ_req ) {
                debugmsg( "Unexpected shortage of energy whilst firing %s. Required: %i J, drained: %i J",
                          gun.tname(), units::to_joule( energ_req ), units::to_joule( drained ) );
                break;
            }
        }

        if( !current_ammo.is_null() ) {
            cycle_action( gun, current_ammo, &here, pos_bub( here ) );
        }

        if( gun_skill == skill_launcher ) {
            continue; // skip retargeting for launchers
        }
    }
    // Use different amounts of time depending on the type of gun and our skill
    mod_moves( -attack_moves );

    const islot_gun &firing = *gun.type->gun;
    for( const std::pair<const bodypart_str_id, int> &hurt_part : firing.hurt_part_when_fired ) {
        apply_damage( nullptr, bodypart_id( hurt_part.first ), hurt_part.second );
        add_msg_player_or_npc( _( "Your %s is hurt by the recoil!" ),
                               _( "<npcname>'s %s is hurt by the recoil!" ),
                               body_part_name_accusative( bodypart_id( hurt_part.first ) ) );
    }

    // Practice the base gun skill proportionally to number of hits, but always by one.
    if( !gun.has_flag( flag_WONT_TRAIN_MARKSMANSHIP ) ) {
        practice( skill_gun, ( hits + 1 ) * 5 );
    }
    // launchers train weapon skill for both hits and misses.
    int practice_units = gun_skill == skill_launcher ? curshot : hits;
    practice( gun_skill, ( practice_units + 1 ) * 5 );

    if( !gun.is_gun() ) {
        // If we lose our gun as a side effect of firing it, skip the rest of the function.
        return curshot;
    }

    // apply shot counter to gun and its mods.
    gun.set_var( "shot_counter", gun.get_var( "shot_counter", 0 ) + curshot );
    for( item *mod : gun.gunmods() ) {
        mod->set_var( "shot_counter", mod->get_var( "shot_counter", 0 ) + curshot );
    }
    if( gun.has_flag( flag_RELOAD_AND_SHOOT ) ) {
        // Reset aim for bows and other reload-and-shoot weapons.
        recoil = MAX_RECOIL;
    } else {
        // apply delayed recoil, factor in recoil enchantments
        recoil += enchantment_cache->modify_value( enchant_vals::mod::RECOIL_MODIFIER, delay );
        if( is_mech_weapon ) {
            // mechs can handle recoil far better. they are built around their main gun.
            // TODO: shouldn't this affect only recoil accumulated during this function?
            recoil = recoil / 2;
        }
        // Cap
        recoil = std::min( MAX_RECOIL, recoil );
    }

    if( is_avatar() ) {
        as_avatar()->aim_cache_dirty = true;
    }

    return curshot;
}

int throw_cost( const Character &c, const item &to_throw )
{
    // Very similar to player::attack_speed
    // TODO: Extract into a function?
    // Differences:
    // Dex is more (2x) important for throwing speed
    // At 10 skill, the cost is down to 0.75%, not 0.66%
    const int base_move_cost = to_throw.attack_time( c ) / 2;
    const float throw_skill = std::min( static_cast<float>( MAX_SKILL ),
                                        c.get_skill_level( skill_throw ) );
    ///\EFFECT_THROW increases throwing speed
    const int skill_cost = static_cast<int>( ( base_move_cost * ( 20 - throw_skill ) / 20 ) );
    ///\EFFECT_DEX increases throwing speed
    const int dexbonus = c.get_dex();
    const float stamina_ratio = static_cast<float>( c.get_stamina() ) / c.get_stamina_max();
    const float stamina_penalty = 1.0 + std::max( ( 0.25f - stamina_ratio ) * 4.0f, 0.0f );

    int move_cost = base_move_cost;
    move_cost *= c.get_modifier( character_modifier_melee_thrown_move_lift_mod );
    move_cost *= c.get_modifier( character_modifier_melee_thrown_move_balance_mod );
    // Stamina penalty only affects base/2 and encumbrance parts of the cost
    move_cost *= stamina_penalty;
    move_cost += skill_cost;
    move_cost -= dexbonus;
    move_cost = c.enchantment_cache->modify_value( enchant_vals::mod::ATTACK_SPEED, move_cost );

    return std::max( 25, move_cost );
}

static double calculate_aim_cap_without_target( const Character &you,
        const tripoint_bub_ms &target )
{
    const int range = rl_dist( you.pos_bub(), target );
    // Get angle of triangle that spans the target square.
    const double angle = 2 * atan2( 0.5, range );
    // Convert from radians to arcmin.
    return 60 * 180 * angle / M_PI;
}


// Handle capping aim level when the player cannot see the target tile or there is nothing to aim at.
double calculate_aim_cap( const Character &you, const tripoint_bub_ms &target )
{
    const map &here = get_map();

    const Creature *victim = get_creature_tracker().creature_at( target, true );
    // nothing here, nothing to aim
    if( victim == nullptr ) {
        return calculate_aim_cap_without_target( you, target );
    }

    const_dialogue d( get_const_talker_for( you ), get_const_talker_for( *victim ) );
    enchant_cache::special_vision sees_with_special = you.enchantment_cache->get_vision( d );

    // you do not see it, but your sense it in other ways
    if( !you.sees( here,  *victim ) && !sees_with_special.is_empty() ) {
        if( sees_with_special.precise ) {
            // your senses are precise enough to aim it with no issues
            return 0.0;
        } else {
            // not as good as seeing it clearly, but still enough to pre-aim
            return calculate_aim_cap_without_target( you, target ) / 3;
        }
    }
    // it is utterly difficult to test this code while 78091 exists; if resolved, check this code once again
    return 0.0;
}

double calc_steadiness( const Character &you, const item &weapon, const tripoint_bub_ms &pos,
                        double predicted_recoil )
{
    const double min_recoil = calculate_aim_cap( you, pos );
    const double effective_recoil = you.most_accurate_aiming_method_limit( weapon );
    const double min_dispersion = std::max( min_recoil, effective_recoil );
    const double steadiness_range = MAX_RECOIL - min_dispersion;
    // This is a relative measure of how steady the player's aim is,
    // 0 is the best the player can do.
    const double steady_score = std::max( 0.0, predicted_recoil - min_dispersion );
    // Fairly arbitrary cap on steadiness...
    return 1.0 - ( steady_score / steadiness_range );
}

int Character::throw_dispersion_per_dodge( bool /* add_encumbrance */ ) const
{
    // +200 per dodge point at 0 dexterity
    // +100 at 8, +80 at 12, +66.6 at 16, +57 at 20, +50 at 24
    // Each 10 encumbrance on either hand is like -1 dex (can bring penalty to +400 per dodge)
    ///\EFFECT_DEX increases throwing accuracy against targets with good dodge stat
    float effective_dex = 2 + get_dex() / 4.0f;
    effective_dex *= get_modifier( character_modifier_thrown_dex_mod );
    return static_cast<int>( 100.0f / std::max( 1.0f, effective_dex ) );
}

// Perfect situation gives us 1000 dispersion at lvl 0
// This goes down linearly to 250  dispersion at lvl 10
int Character::throwing_dispersion( const item &to_throw, Creature *critter,
                                    bool is_blind_throw ) const
{
    units::mass weight = to_throw.weight();
    units::volume volume = to_throw.volume();
    if( to_throw.count_by_charges() && to_throw.charges > 1 ) {
        weight /= to_throw.charges;
        volume /= to_throw.charges;
    }

    int throw_difficulty = 1000;
    // 1000 penalty for every liter after the first
    // TODO: Except javelin type items
    throw_difficulty += std::max<int>( 0, units::to_milliliter( volume - 1_liter ) );
    // 1 penalty for gram above str*100 grams (at 0 skill)
    ///\ARM_STR decreases throwing dispersion when throwing heavy objects
    const int weight_in_gram = units::to_gram( weight );
    throw_difficulty += std::max( 0, weight_in_gram - get_arm_str() * 100 );

    // Dispersion from difficult throws goes from 100% at lvl 0 to 25% at lvl 10
    ///\EFFECT_THROW increases throwing accuracy
    const float throw_skill = std::min( static_cast<float>( MAX_SKILL ),
                                        get_skill_level( skill_throw ) );
    int dispersion = 10 * throw_difficulty / ( 8 * throw_skill + 4 );
    // If the target is a creature, it moves around and ruins aim
    // TODO: Inform projectile functions if the attacker actually aims for the critter or just the tile
    if( critter != nullptr ) {
        // It's easier to dodge at close range (thrower needs to adjust more)
        // Dodge x10 at point blank, x5 at 1 dist, then flat
        float effective_dodge = critter->get_dodge() * std::max( 1, 10 - 5 * rl_dist( pos_bub(),
                                critter->pos_bub() ) );
        dispersion += throw_dispersion_per_dodge( true ) * effective_dodge;
    }
    // 1 perception per 1 eye encumbrance
    ///\EFFECT_PER decreases throwing accuracy penalty from eye encumbrance
    dispersion += std::max( 0, ( encumb( bodypart_id( "eyes" ) ) - get_per() ) * 10 );

    // If throwing blind, we're assuming they mechanically can't achieve the
    // accuracy of a normal throw.
    if( is_blind_throw ) {
        dispersion *= 4;
    }

    return std::max( 0, dispersion );
}

static std::optional<int> character_throw_assist( const Character &guy )
{
    std::optional<int> throw_assist = std::nullopt;
    if( guy.is_mounted() ) {
        auto *mons = guy.mounted_creature.get();
        if( mons->mech_str_addition() != 0 ) {
            throw_assist = mons->mech_str_addition();
            mons->use_mech_power( 3_kJ );
        }
    }
    return throw_assist;
}

static float throwing_skill_adjusted( const Character &guy )
{
    float skill_level = std::min( static_cast<float>( MAX_SKILL ), guy.get_skill_level( skill_throw ) );
    // if you are lying on the floor, you can't really throw that well
    if( guy.has_effect( effect_downed ) ) {
        skill_level = std::max( 0.0f, skill_level - 5 );
    }
    return skill_level;
}

int Character::thrown_item_adjusted_damage( const item &thrown ) const
{
    const std::optional<int> throw_assist = character_throw_assist( *this );
    const bool do_railgun = has_active_bionic( bio_railgun ) && thrown.made_of_any( ferric ) &&
                            !throw_assist;

    // The damage dealt due to item's weight, player's strength, and skill level
    // Up to str/2 or weight/100g (lower), so 10 str is 5 damage before multipliers
    // Railgun doubles the effective strength
    ///\ARM_STR increases throwing damage
    double stats_mod = do_railgun ? get_str() : ( get_arm_str() / 2.0 );
    stats_mod = throw_assist ? *throw_assist / 2.0 : stats_mod;
    // modify strength impact based on skill level, clamped to [0.15 - 1]
    // mod = mod * [ ( ( skill / max_skill ) * 0.85 ) + 0.15 ]
    stats_mod *= ( std::min( static_cast<float>( MAX_SKILL ),
                             get_skill_level( skill_throw ) ) /
                   static_cast<double>( MAX_SKILL ) ) * 0.85 + 0.15;
    return stats_mod;
}

projectile Character::thrown_item_projectile( const item &thrown ) const
{
    // We'll be constructing a projectile
    projectile proj;
    proj.impact = thrown.base_damage_thrown();
    proj.speed = 10 + round( throwing_skill_adjusted( *this ) );
    return proj;
}

int Character::thrown_item_total_damage_raw( const item &thrown ) const
{
    projectile proj = thrown_item_projectile( thrown );
    proj.impact.add_damage( damage_bash, std::min( thrown.weight() / 100.0_gram,
                            static_cast<double>( thrown_item_adjusted_damage( thrown ) ) ) );
    const int glass_portion = thrown.made_of( material_glass );
    const float glass_fraction = glass_portion / static_cast<float>( thrown.type->mat_portion_total );
    const units::volume volume = thrown.volume() * glass_fraction;
    // Item will shatter upon landing, destroying the item, dealing damage, and making noise
    if( !thrown.active && glass_portion &&
        rng( 0, units::to_milliliter( 2_liter - volume ) ) < get_arm_str() * 100 ) {
        proj.impact.add_damage( damage_cut, units::to_milliliter( volume ) / 500.0f );
    }
    // Some minor (skill/2) armor piercing for skillful throws
    // Not as much as in melee, though
    const float skill_level = throwing_skill_adjusted( *this );
    for( damage_unit &du : proj.impact.damage_units ) {
        du.res_pen += skill_level / 2.0f;
    }

    int total_damage = 0;
    for( damage_unit &du : proj.impact.damage_units ) {
        total_damage += du.amount * du.damage_multiplier;
    }
    return total_damage;
}

dealt_projectile_attack Character::throw_item( const tripoint_bub_ms &target, const item &to_throw,
        const std::optional<tripoint_bub_ms> &blind_throw_from_pos )
{
    // Copy the item, we may alter it before throwing
    item thrown = to_throw;
    thrown.set_var( "last_act_by_char_id", getID().get_value() );

    const int move_cost = throw_cost( *this, to_throw );
    mod_moves( -move_cost );

    const int throwing_skill = round( get_skill_level( skill_throw ) );
    const units::volume volume = to_throw.volume();
    const units::mass weight = to_throw.weight();
    const std::optional<int> throw_assist = character_throw_assist( *this );

    if( !throw_assist ) {
        const int stamina_cost = get_standard_stamina_cost( &thrown );
        mod_stamina( stamina_cost + throwing_skill );
    }

    const float skill_level = throwing_skill_adjusted( *this );
    add_msg_debug( debugmode::DF_RANGED, "Adjusted throw skill %g", skill_level );
    projectile proj = thrown_item_projectile( thrown );
    damage_instance &impact = proj.impact;
    std::set<ammo_effect_str_id> &proj_effects = proj.proj_effects;

    const bool do_railgun = has_active_bionic( bio_railgun ) && thrown.made_of_any( ferric ) &&
                            !throw_assist;

    impact.add_damage( damage_bash, std::min( weight / 100.0_gram,
                       static_cast<double>( thrown_item_adjusted_damage( thrown ) ) ) );

    impact.add_damage( damage_bash,
                       enchantment_cache->get_value_add( enchant_vals::mod::THROW_DAMAGE ) );
    impact.mult_damage( 1 + enchantment_cache->get_value_multiply( enchant_vals::mod::THROW_DAMAGE ) );
    if( thrown.has_flag( flag_ACT_ON_RANGED_HIT ) ) {
        proj_effects.insert( ammo_effect_ACT_ON_RANGED_HIT );
        thrown.active = true;
    }

    // Item will shatter upon landing, destroying the item, dealing damage, and making noise
    const int glass_portion = thrown.made_of( material_glass );
    const float glass_fraction = glass_portion / static_cast<float>( thrown.type->mat_portion_total );
    const units::volume glass_vol = volume * glass_fraction;
    /** @ARM_STR increases chance of shattering thrown glass items (NEGATIVE) */
    const bool shatter = !thrown.active && glass_portion &&
                         rng( 0, units::to_milliliter( 2_liter - glass_vol ) ) < get_arm_str() * 100;

    // Item will burst upon landing, destroying the item, and spilling its contents
    const bool burst = thrown.has_property( "burst_when_filled" ) && thrown.is_container() &&
                       thrown.get_property_int64_t( "burst_when_filled" ) <= static_cast<double>
                       ( thrown.total_contained_volume().value() ) / thrown.get_total_capacity().value() *
                       100;

    // Add some flags to the projectile
    if( weight > 500_gram ) {
        proj_effects.insert( ammo_effect_HEAVY_HIT );
    }

    proj_effects.insert( ammo_effect_NO_ITEM_DAMAGE );

    if( thrown.active ) {
        // Can't have Molotovs embed into monsters
        // Monsters don't have inventory processing
        proj_effects.insert( ammo_effect_NO_EMBED );
    }

    if( do_railgun ) {
        proj_effects.insert( ammo_effect_LIGHTNING );

        const units::energy trigger_cost = bio_railgun->power_trigger;
        mod_power_level( -trigger_cost );
    }

    if( volume > 500_ml ) {
        proj_effects.insert( ammo_effect_WIDE );
    }

    // Deal extra cut damage if the item breaks
    if( shatter ) {
        impact.add_damage( damage_cut, units::to_milliliter( volume ) / 500.0f );
        proj_effects.insert( ammo_effect_SHATTER_SELF );
    }

    // TODO: Add wet effect if other things care about that
    if( burst ) {
        proj_effects.insert( ammo_effect_BURST );
    }

    // Some minor (skill/2) armor piercing for skillful throws
    // Not as much as in melee, though
    for( damage_unit &du : impact.damage_units ) {
        du.res_pen += skill_level / 2.0f;
    }
    // handling for tangling thrown items
    if( thrown.has_flag( flag_TANGLE ) ) {
        proj_effects.insert( ammo_effect_TANGLE );
    }

    Creature *critter = get_creature_tracker().creature_at( target, true );
    const dispersion_sources dispersion( throwing_dispersion( thrown, critter,
                                         blind_throw_from_pos.has_value() ) );
    const itype *thrown_type = thrown.type;

    // Put the item into the projectile
    proj.set_drop( std::move( thrown ) );
    if( thrown_type->has_flag( flag_CUSTOM_EXPLOSION ) ) {
        proj.set_custom_explosion( thrown_type->explosion );
    }

    // Throw from the player's position, unless we're blind throwing, in which case
    // throw from the the blind throw position instead.
    const tripoint_bub_ms throw_from = blind_throw_from_pos ? *blind_throw_from_pos : pos_bub();

    float range = rl_dist( throw_from, target );
    proj.range = range;
    float skill_lvl = get_skill_level( skill_throw );
    // Avoid awarding tons of xp for lucky throws against hard to hit targets
    const float range_factor = std::min<float>( range, skill_lvl + 3 );
    // We're aiming to get a damaging hit, not just an accurate one - reward proper weapons
    const float damage_factor = 5.0f * std::sqrt( proj.impact.total_damage() / 5.0f );
    // This should generally have values below ~20*sqrt(skill_lvl)
    const float final_xp_mult = range_factor * damage_factor;

    itype_id to_throw_id = to_throw.type->get_id();
    weakpoint_attack wp_attack;
    wp_attack.weapon = &to_throw;
    wp_attack.is_thrown = true;
    dealt_projectile_attack dealt_attack;
    projectile_attack( dealt_attack, proj, throw_from, target, dispersion,
                       this, nullptr, wp_attack );
    for( std::pair<Creature *const, std::pair<int, int>> &hit_entry : dealt_attack.targets_hit ) {
        if( hit_entry.second.first == 0 ) {
            continue;
        }
        if( monster *const m = hit_entry.first->as_monster() ) {
            cata::event e = cata::event::make<event_type::character_ranged_attacks_monster>( getID(),
                            itype_id::NULL_ID(),
                            to_throw_id,
                            true,
                            m->type->id );
            get_event_bus().send_with_talker( this, m, e );
        } else if( Character *const c = hit_entry.first->as_character() ) {
            cata::event e = cata::event::make<event_type::character_ranged_attacks_character>( getID(),
                            itype_id::NULL_ID(),
                            to_throw_id,
                            true,
                            c->getID(), c->get_name() );
            get_event_bus().send_with_talker( this, c, e );
        }
    }
    const double missed_by = dealt_attack.missed_by;

    if( critter && dealt_attack.last_hit_critter != nullptr && dealt_attack.headshot &&
        !critter->has_flag( mon_flag_IMMOBILE ) &&
        !critter->has_flag( json_flag_CANNOT_MOVE ) ) {
        practice( skill_throw, final_xp_mult, MAX_SKILL );
        get_event_bus().send<event_type::character_gets_headshot>( getID() );
    } else if( critter && dealt_attack.last_hit_critter != nullptr && missed_by > 0.0f &&
               !critter->has_flag( mon_flag_IMMOBILE ) &&
               !critter->has_flag( json_flag_CANNOT_MOVE ) ) {
        practice( skill_throw, final_xp_mult / ( 1.0f + missed_by ), MAX_SKILL );
    } else {
        // Pure grindy practice - cap gain at lvl 2
        practice( skill_throw, 5, 2 );
    }
    // Reset last target pos
    last_target_pos = std::nullopt;
    recoil = MAX_RECOIL;

    return dealt_attack;
}

void practice_archery_proficiency( Character &p, const item &relevant )
{
    // Do nothing, we are not doing archery
    if( relevant.gun_skill() != skill_archery ) {
        return;
    }

    // We are already a master, practice has no effect
    if( p.has_proficiency( proficiency_prof_bow_master ) ) {
        return;
    }

    // Only train archery in multiples of 100 moves (or 1 turn) for performance, focus, and simplifying calculations
    p.archery_aim_counter++;
    if( p.archery_aim_counter > 100 ) {
        // Reset our counter
        p.archery_aim_counter = 0;

        // Intended to train prof_bow_basic from 0 to 100% in ~200 arrows in 5 hours from max aim, or 90s/arrow.
        // Additionally, a shortbow is used as the basis for number of moves per max aim shot at ~250 moves.
        // 90s/arrow / 2.5turns/arrow = 36s/turn. It is important to note, this is 200 arrows pre-focus.
        time_duration prof_exp = 36_seconds;

        // We don't know how to handle a bow yet
        if( !p.has_proficiency( proficiency_prof_bow_basic ) ) {
            p.practice_proficiency( proficiency_prof_bow_basic, prof_exp );
            return;
        }
        // We know how to handle a bow, but we are not an expert yet
        else if( !p.has_proficiency( proficiency_prof_bow_expert ) ) {
            p.practice_proficiency( proficiency_prof_bow_expert, prof_exp );
            return;
        }
        // We are an expert, lets practice to become a master
        else {
            p.practice_proficiency( proficiency_prof_bow_master, prof_exp );
            return;
        }
    }
}

// Apply stamina cost to archery which decreases due to proficiency
static void mod_stamina_archery( Character &you, const item &relevant )
{
    // Set activity level to 10 * str_ratio, with 10 being max (EXTRA_EXERCISE)
    // This ratio should never be below 0 and above 1
    const float str_ratio = static_cast<float>( relevant.get_min_str() ) / you.str_cur;
    you.set_activity_level( 10 * str_ratio );

    // Calculate stamina drain based on archery, athletics skill, and effective bow strength ratio
    const float archery_skill = you.get_skill_level( skill_archery );
    const float athletics_skill = you.get_skill_level( skill_swimming );
    const float skill_modifier = ( 2.0f * archery_skill + athletics_skill ) / 3.0f;
    const int stamina_cost = pow( 10.0f * str_ratio + 10 - skill_modifier, 2 );

    you.mod_stamina( -stamina_cost );
    add_msg_debug( debugmode::DF_RANGED,
                   "-%i stamina: %.1f str ratio, %.1f skill mod",
                   stamina_cost, str_ratio, skill_modifier );
}

static void do_aim( Character &you, const item &relevant, const double min_recoil )
{
    const double aim_amount = you.aim_per_move( relevant, you.recoil );
    if( aim_amount > 0 && you.recoil > min_recoil ) {
        // Increase aim at the cost of moves
        you.mod_moves( -1 );
        you.recoil = std::max( min_recoil, you.recoil - aim_amount );

        // Train archery proficiencies if we are doing archery
        if( relevant.gun_skill() == skill_archery ) {
            practice_archery_proficiency( you, relevant );

            // Only drain stamina on initial draw
            if( you.get_moves() == 1 ) {
                mod_stamina_archery( you, relevant );
            }
        }
    } else {
        // If aim is already maxed, we're just waiting, so pass the turn.
        you.set_moves( 0 );
    }
}

struct confidence_rating {
    double aim_level;
    char symbol;
    std::string color;
    std::string label;
};

static int print_steadiness( const catacurses::window &w, int line_number, double steadiness )
{
    const int window_width = getmaxx( w ) - 2; // Window width minus borders.

    if( get_option<std::string>( "ACCURACY_DISPLAY" ) == "numbers" ) {
        std::string steadiness_s = string_format( "%s: %d%%", _( "Steadiness" ),
                                   static_cast<int>( 100.0 * steadiness ) );
        mvwprintw( w, point( 1, line_number++ ), steadiness_s );
    } else {
        const std::string &steadiness_bar = get_labeled_bar( steadiness, window_width,
                                            _( "Steadiness" ), '*' );
        mvwprintw( w, point( 1, line_number++ ), steadiness_bar );
    }

    return line_number;
}

static double confidence_estimate( const Target_attributes &attributes,
                                   const dispersion_sources &dispersion )
{
    // This is a rough estimate of accuracy based on a linear distribution across min and max
    // dispersion.  It is highly inaccurate probability-wise, but this is intentional, the player
    // is not doing Gaussian integration in their head while aiming.  The result gives the player
    // correct relative measures of chance to hit, and corresponds with the actual distribution at
    // min, max, and mean.
    if( attributes.range == 0 ) {
        return 2 * attributes.size;
    }
    double max_lateral_offset = iso_tangent( attributes.range, units::from_arcmin( dispersion.max() ) );
    return 1 / ( max_lateral_offset / attributes.size );
}

static aim_type get_default_aim_type()
{
    // dummy aim type for unaimed shots
    return { _( "Immediate" ), "", "", false, static_cast<int>( MAX_RECOIL ) };
}

using RatingVector = std::vector<std::tuple<double, char, std::string>>;

static std::string get_colored_bar( const double val, const int width, const std::string &label,
                                    RatingVector::iterator begin, RatingVector::iterator end )
{
    return get_labeled_bar<RatingVector::iterator>( val, width, label, begin, end,
    []( RatingVector::iterator it, int seg_width ) -> std::string {
        std::string result = string_format( "<color_%s>", std::get<2>( *it ) );
        result.insert( result.end(), seg_width, std::get<1>( *it ) );
        result += "</color>";
        return result;} );
}

static double target_size_in_moa( int range, double size )
{
    return atan2( size, range ) * 60 * 180 / M_PI;
}

Target_attributes::Target_attributes( tripoint_bub_ms src, tripoint_bub_ms target )
{
    const map &here = get_map();

    Creature *target_critter = get_creature_tracker().creature_at( target );
    Creature *shooter = get_creature_tracker().creature_at( src );
    range = rl_dist( src, target );
    size = target_critter != nullptr ?
           target_critter->ranged_target_size() :
           here.ranged_target_size( target );
    size_in_moa = target_size_in_moa( range, size ) ;
    light = here.ambient_light_at( target );
    visible = shooter->sees( here, target );

}
Target_attributes::Target_attributes( int rng, double target_size, float light_target,
                                      bool can_see )
{
    range = rng;
    size = target_size;
    size_in_moa = target_size_in_moa( range, size );
    light = light_target;
    visible = can_see;
}

/*
* struct used to hold the information on entire aim_type prediction;
* all the properties and odds for every 'confidence' outcome
*/
struct aim_type_prediction {
    struct aim_confidence {
        std::string label;
        std::string color;
        int chance;
    };

    std::string name;
    std::string hotkey;
    std::vector<confidence_rating> ratings; // this is read back in UI
    std::vector<aim_confidence> chances;
    bool is_default;
    int moves;
    int chance_to_hit; // all hit probabilities summed up for sorting
    double confidence;
    double steadiness;
};

// struct used for returning values from predict_recoil()
// recoil is the either the sight dispersion or the aim mode's threshold
// moves it the amount of moves it'll take to reach that aim state
struct recoil_prediction {
    double recoil;
    int moves;
};

/*
* This method tries to estimate the amount of moves required to reach
* the aim mode's recoil threshold or the sight's dispersion value
*/
static recoil_prediction predict_recoil( const Character &you, const item &weapon,
        const Target_attributes &target, int sight_dispersion,
        const aim_type &aim_mode, double start_recoil )
{
    if( !aim_mode.has_threshold || aim_mode.threshold > start_recoil ) {
        return { start_recoil, 0 };
    }

    double predicted_recoil = start_recoil;
    int predicted_delay = 0;
    const aim_mods_cache &aim_cache = you.gen_aim_mods_cache( weapon );
    auto aim_cache_opt = std::make_optional( std::ref( aim_cache ) );
    // next loop simulates aiming until either aim mode threshold or sight_dispersion is reached
    do {
        const double aim_amount = you.aim_per_move( weapon, predicted_recoil, target, aim_cache_opt );
        if( aim_amount <= MIN_RECOIL_IMPROVEMENT ) {
            break;
        }
        predicted_delay++;
        predicted_recoil = std::max( predicted_recoil - aim_amount, 0.0 );
    } while( predicted_recoil > aim_mode.threshold && predicted_recoil > sight_dispersion );

    return { predicted_recoil, predicted_delay };
}

/*
* This method calculates the ranged to-hit chances, split by
* confidence ratings for UI display purposes.
* The returned vector contains all the
* a mapping of all weapon aiming modes to their chance predictions.
* Inside each prediction there is a vector of "confidences";
* they represent the great/hit/graze/miss chances.
*/
static std::vector<aim_type_prediction> calculate_ranged_chances(
    const target_ui &ui, const Character &you,
    target_ui::TargetMode mode, const input_context &ctxt, const item &weapon,
    const dispersion_sources &dispersion, const std::vector<confidence_rating> &confidence_ratings,
    const Target_attributes &target, const tripoint_bub_ms &pos, const item_location &load_loc )
{
    std::vector<aim_type> aim_types { get_default_aim_type() };
    std::vector<aim_type_prediction> aim_outputs;

    if( mode != target_ui::TargetMode::Throw && mode != target_ui::TargetMode::ThrowBlind ) {
        aim_types = you.get_aim_types( weapon );
    }

    // predict how long it'll take to reach from current recoil
    // to the ui's selected default aim mode threshold.
    const recoil_prediction aim_to_selected = predict_recoil( you, weapon, target,
            ui.get_sight_dispersion(), ui.get_selected_aim_type(), you.recoil );

    const double selected_steadiness = calc_steadiness( you, weapon, pos, aim_to_selected.recoil );

    const int throw_moves = throw_cost( you, weapon );

    for( const aim_type &aim_type : aim_types ) {
        const std::vector<input_event> keys = ctxt.keys_bound_to( aim_type.action.empty() ? "FIRE" :
                                              aim_type.action, /*maximum_modifier_count=*/1 );

        aim_type_prediction prediction = {};
        prediction.name = aim_type.has_threshold ? aim_type.name : _( "Current" );
        prediction.is_default = aim_type.action.empty(); // default mode defaults to FIRE hotkey
        prediction.hotkey = ( keys.empty() ? input_event() : keys.front() ).short_description();

        if( mode == target_ui::TargetMode::Throw || mode == target_ui::TargetMode::ThrowBlind ) {
            prediction.moves = throw_moves;
        } else {
            prediction.moves = predict_recoil( you, weapon, target, ui.get_sight_dispersion(), aim_type,
                                               you.recoil ).moves + time_to_attack( you, *weapon.type )
                               + RAS_time( you, load_loc );
        }

        // if the default method is "behind" the selected; e.g. you are in immediate
        // firing mode with almost close no chances of hitting, but UI has selected
        // "precise" this will "catch up" the default method adding the required moves
        // and steadiness that it will gain if player presses fire, so pressing fire
        // hotkey in this case will actually use 'precise' aim mode.
        // in case where it already surpassed the UI selected mode this should be a
        // no-op.
        if( prediction.is_default ) {
            prediction.moves += aim_to_selected.moves;
            prediction.steadiness = selected_steadiness;
        } else {
            // predict how long it'll take to reach from current recoil
            // to the current aim mode's threshold.
            const recoil_prediction aim_to_type = ( aim_type == ui.get_selected_aim_type() ) ? aim_to_selected :
                                                  predict_recoil( you, weapon, target, ui.get_sight_dispersion(), aim_type, you.recoil );
            prediction.steadiness = calc_steadiness( you, weapon, pos, aim_to_type.recoil );
        }

        // make a copy of the given dispersion, apply the aiming and calculate hit confidence
        dispersion_sources current_dispersion = dispersion;
        current_dispersion.add_range( aim_type.has_threshold ? aim_type.threshold :
                                      aim_to_selected.recoil );

        // this loop fills in the "confidence" values; the chances of great/good/graze outcomes
        prediction.confidence = confidence_estimate( target, current_dispersion );
        for( const confidence_rating &rating : confidence_ratings ) {
            const int chance = std::min<int>( 100, 100 * rating.aim_level * prediction.confidence )
                               - prediction.chance_to_hit;
            prediction.ratings.push_back( rating );
            prediction.chances.push_back( {rating.label, rating.color, chance} );
            prediction.chance_to_hit += chance;
        }

        // Adds the "miss" outcome
        prediction.chances.push_back( { _( "Miss" ), "light_gray", 100 - prediction.chance_to_hit } );

        aim_outputs.push_back( prediction );
    }
    return aim_outputs;
}

static void print_confidence_ratings( const catacurses::window &w,
                                      const std::vector<confidence_rating> &ratings, int &line_number, int width, int &column_number,
                                      nc_color col )
{
    for( const confidence_rating &cr : ratings ) {
        std::string label = pgettext( "aim_confidence", cr.label.c_str() );
        std::string symbols = string_format( "<color_%s>%s</color> = %s", cr.color, cr.symbol, label );
        int line_len = utf8_width( label ) + 5; // 5 for '# = ' and whitespace at end
        if( ( width - column_number ) < line_len ) {
            column_number = 1;
            line_number++;
        }
        print_colored_text( w, point( column_number, line_number ), col, col, symbols );
        column_number += line_len;
    }
    line_number++;
}

static void print_confidence_rating_bar( const catacurses::window &w,
        aim_type_prediction prediction, int &line_number, int width, nc_color col )
{
    RatingVector confidence_ratings;
    std::transform( prediction.ratings.begin(), prediction.ratings.end(),
                    std::back_inserter( confidence_ratings ),
    [&]( const confidence_rating & config ) {
        return std::make_tuple( config.aim_level, config.symbol, config.color );
    } );

    const std::string &confidence_bar = get_colored_bar( prediction.confidence, width, "",
                                        confidence_ratings.begin(), confidence_ratings.end() );

    print_colored_text( w, point( 1, line_number++ ), col, col, confidence_bar );
}

static int print_ranged_chance( const catacurses::window &w, int line_number,
                                const std::vector<aim_type_prediction> &aim_chances, const int time )
{
    std::vector<aim_type_prediction> sorted = aim_chances;

    // Sort aim types so that 'current' mode is placed at the current probability it provides
    // TODO: consider removing it, but for now it demonstrates the odds changing pretty well
    std::sort( sorted.begin(), sorted.end(),
    []( const auto & lhs, const auto & rhs ) {
        return lhs.confidence <= rhs.confidence;
    } );

    int width = getmaxx( w ) - 2; // window width minus borders
    const int bars_pad = 3;
    bool display_numbers = get_option<std::string>( "ACCURACY_DISPLAY" ) == "numbers";
    bool narrow = panel_manager::get_manager().get_current_layout().panels().begin()->get_width() <= 42;
    nc_color col = c_light_gray;


    const auto &current_steadiness_it = std::find_if( sorted.begin(),
    sorted.end(), []( const aim_type_prediction & atp ) {
        return atp.is_default;
    } );
    if( current_steadiness_it != sorted.end() ) {
        line_number = print_steadiness( w, line_number, current_steadiness_it->steadiness );
    }

    // Start printing by available width of aim window
    if( narrow ) {
        std::vector<std::string> t_aims( 4 );
        std::vector<std::string> t_confidence( 20 );
        int aim_iter = 0;
        int conf_iter = 0;
        if( !display_numbers ) {
            int column_number = 1;
            print_confidence_ratings( w, aim_chances.front().ratings, line_number, width, column_number, col );
            width -= bars_pad;
        } else {
            std::string symbols = _( " <color_green>Great</color> <color_light_gray>Normal</color>"
                                     " <color_magenta>Graze</color> <color_dark_gray>Miss</color> <color_light_blue>Moves</color>" );
            fold_and_print( w, point( 1, line_number++ ), width + bars_pad, c_dark_gray, symbols );
            int len = utf8_width( symbols ) - 121; // to subtract color codes
            if( len > width + bars_pad ) {
                line_number++;
            }
            for( int i = 0; i < width; i++ ) {
                mvwprintw( w, point( i + 1, line_number ), "-" );
            }
        }
        for( const aim_type_prediction &out : sorted ) {
            if( display_numbers ) {
                t_aims[aim_iter] = string_format( "<color_dark_gray>%s:</color>", out.name );
                t_confidence[( aim_iter * 5 ) + 4] = string_format( "<color_light_blue>%d</color>", out.moves );
            } else {
                print_colored_text( w, point( 1, line_number ), col, col, string_format( _( "%s %s:" ), out.name,
                                    _( "Aim" ) ) );
                right_print( w, line_number++, 1, c_light_blue, _( "Moves" ) );
                right_print( w, line_number, 1, c_light_blue, string_format( "%d", out.moves ) );
            }

            if( display_numbers ) {
                int last_chance = 0;
                conf_iter = 0;
                for( const confidence_rating &cr : narrow ? aim_chances.front().ratings : out.ratings ) {
                    int chance = std::min<int>( 100, 100.0 * ( cr.aim_level ) * out.confidence ) - last_chance;
                    last_chance += chance;
                    t_confidence[conf_iter + ( aim_iter * 5 )] = string_format( "<color_%s>%3d%%</color>", cr.color,
                            chance );
                    conf_iter++;
                    if( conf_iter == 3 ) {
                        t_confidence[conf_iter + ( aim_iter * 5 )] = string_format( "<color_%s>%3d%%</color>", "dark_gray",
                                100 - last_chance );
                    }
                }
                aim_iter++;
            } else {
                print_confidence_rating_bar( w, out, line_number, width, col );
            }
        }

        // Draw tables for compact Numbers display
        if( display_numbers ) {
            const std::string divider = "|";
            int left_pad = 8;
            int columns = 5;
            insert_table( w, left_pad, ++line_number, columns, c_light_gray, divider, true, t_confidence );
            insert_table( w, 0, line_number, 1, c_light_gray, "", false, t_aims );
            line_number = line_number + 4; // 4 to account for the tables
        }
        return line_number;
    } else { // print normally

        int column_number = 1;
        if( !narrow ) {
            std::string label = _( "Symbols:" );
            mvwprintw( w, point( column_number, line_number ), label );
            column_number += utf8_width( label ) + 1; // 1 for whitespace after 'Symbols:'
        }

        print_confidence_ratings( w, sorted.front().ratings, line_number, width, column_number, col );

        for( const aim_type_prediction &out : sorted ) {
            std::string col_hl = out.is_default ? "light_green" : "light_gray";
            std::string desc = time ==  0 ?
                               string_format( "<color_white>[%s]</color> <color_%s>%s %s</color> | %s: <color_light_blue>%3d</color>",
                                              out.hotkey, col_hl, out.name, _( "Aim" ), _( "Moves to fire" ), out.moves ) :
                               string_format( "<color_white>[%s]</color> <color_%s>%s %s</color> | %s: <color_light_blue>%3d</color> (%d)",
                                              out.hotkey, col_hl, out.name, _( "Aim" ), _( "Moves to fire" ), out.moves, time );

            print_colored_text( w, point( 1, line_number++ ), col, col, desc );

            if( display_numbers ) {
                const std::string line = enumerate_as_string( out.chances.cbegin(), out.chances.cend(),
                []( const aim_type_prediction::aim_confidence & conf ) {
                    const std::string label_loc = pgettext( "aim_confidence", conf.label.c_str() );
                    return string_format( "%s: <color_%s>%3d%%</color>", label_loc, conf.color, conf.chance );
                }, enumeration_conjunction::none );

                line_number += fold_and_print_from( w, point( 1, line_number ), width, 0, c_dark_gray, line );
            } else {
                print_confidence_rating_bar( w, out, line_number, width, col );
            }
        }

        return line_number;
    }
}

// Whether player character knows creature's position and can roughly track it with the aim cursor
static bool pl_sees( const Creature &cr )
{
    const map &here = get_map();

    Character &u = get_player_character();
    return u.sees( here,  cr ) || u.sees_with_specials( cr );
}

// Whether player character knows vehicle's position and can roughly track it with the aim cursor
static bool pl_sees( const optional_vpart_position &ovp )
{
    const map &here = get_map();

    Character &u = get_player_character();
    return u.sees( here,  ovp.value().pos_bub( here ) );
}

static int print_aim( const target_ui &ui, Character &you, const catacurses::window &w,
                      int line_number, input_context &ctxt, const item &weapon, const tripoint_bub_ms &pos,
                      item_location &load_loc )
{
    // This is absolute accuracy for the player.
    // TODO: push the calculations duplicated from Creature::deal_projectile_attack() and
    // Creature::projectile_attack() into shared methods.
    // Dodge doesn't affect gun attacks

    dispersion_sources dispersion = you.get_weapon_dispersion( weapon );
    dispersion.add_range( you.recoil_vehicle() );

    // This could be extracted, to allow more/less verbose displays
    static const std::vector<confidence_rating> confidence_config = {{
            { accuracy_critical, '*', "green", translate_marker_context( "aim_confidence", "Great" ) },
            { accuracy_standard, '+', "light_gray", translate_marker_context( "aim_confidence", "Normal" ) },
            { accuracy_grazing, '|', "magenta", translate_marker_context( "aim_confidence", "Graze" ) }
        }
    };

    const std::vector<aim_type_prediction> aim_chances = calculate_ranged_chances( ui, you,
            target_ui::TargetMode::Fire, ctxt, weapon, dispersion, confidence_config,
            Target_attributes( you.pos_bub(), pos ), pos, load_loc );

    int time = RAS_time( you, load_loc );

    return print_ranged_chance( w, line_number, aim_chances, time );
}

static void draw_throw_aim( const target_ui &ui, const Character &you, const catacurses::window &w,
                            int &text_y, input_context &ctxt, const item &weapon, const tripoint_bub_ms &target_pos,
                            bool is_blind_throw )
{
    const map &here = get_map();

    Creature *target = get_creature_tracker().creature_at( target_pos, true );
    if( target != nullptr && !you.sees( here, *target ) ) {
        target = nullptr;
    }

    const dispersion_sources dispersion( you.throwing_dispersion( weapon, target, is_blind_throw ) );
    const double range = rl_dist( you.pos_bub(), target_pos );

    const double target_size = target != nullptr ? target->ranged_target_size() : 1.0f;

    static const std::vector<confidence_rating> confidence_config_critter = {{
            { accuracy_critical, '*', "green", translate_marker_context( "aim_confidence", "Great" ) },
            { accuracy_standard, '+', "light_gray", translate_marker_context( "aim_confidence", "Normal" ) },
            { accuracy_grazing, '|', "magenta", translate_marker_context( "aim_confidence", "Graze" ) }
        }
    };
    static const std::vector<confidence_rating> confidence_config_object = {{
            { accuracy_grazing, '*', "white", translate_marker_context( "aim_confidence", "Hit" ) }
        }
    };
    const auto &confidence_config = target != nullptr ?
                                    confidence_config_critter : confidence_config_object;

    const target_ui::TargetMode throwing_target_mode = is_blind_throw ?
            target_ui::TargetMode::ThrowBlind :
            target_ui::TargetMode::Throw;
    Target_attributes attributes( range, target_size,
                                  here.ambient_light_at( target_pos ),
                                  you.sees( here, target_pos ) );

    const std::vector<aim_type_prediction> aim_chances = calculate_ranged_chances( ui, you,
            throwing_target_mode, ctxt, weapon, dispersion, confidence_config, attributes, target_pos,
            item_location() );

    text_y = print_ranged_chance( w, text_y, aim_chances, 0 );
}

std::vector<aim_type> Character::get_aim_types( const item &gun ) const
{
    std::vector<aim_type> aim_types { get_default_aim_type() };
    if( !gun.is_gun() ) {
        return aim_types;
    }
    int sight_dispersion = most_accurate_aiming_method_limit( gun );
    // Aiming thresholds are dependent on weapon sight dispersion, attempting to place thresholds
    // at 10%, 5% and 0% of the difference between MAX_RECOIL and sight dispersion.
    std::vector<int> thresholds = {
        static_cast<int>( ( ( MAX_RECOIL - sight_dispersion ) / 10.0 ) + sight_dispersion ),
        static_cast<int>( ( ( MAX_RECOIL - sight_dispersion ) / 40.0 ) + sight_dispersion ),
        static_cast<int>( sight_dispersion )
    };
    // Remove duplicate thresholds.
    std::vector<int>::iterator thresholds_it = std::adjacent_find( thresholds.begin(),
            thresholds.end() );
    while( thresholds_it != thresholds.end() ) {
        thresholds.erase( thresholds_it );
        thresholds_it = std::adjacent_find( thresholds.begin(), thresholds.end() );
    }
    thresholds_it = thresholds.begin();
    aim_types.push_back( aim_type { _( "Regular" ), "AIMED_SHOT", _( "[%c] to aim and fire." ),
                                    true, *thresholds_it } );
    ++thresholds_it;
    if( thresholds_it != thresholds.end() ) {
        aim_types.push_back( aim_type { _( "Careful" ), "CAREFUL_SHOT",
                                        _( "[%c] to take careful aim and fire." ), true,
                                        *thresholds_it } );
        ++thresholds_it;
    }
    if( thresholds_it != thresholds.end() ) {
        aim_types.push_back( aim_type { _( "Precise" ), "PRECISE_SHOT",
                                        _( "[%c] to take precise aim and fire." ), true,
                                        *thresholds_it } );
    }
    return aim_types;
}

static projectile make_gun_projectile( const item &gun )
{
    projectile proj;
    proj.speed  = 1000;
    proj.impact = gun.gun_damage();
    proj.shot_impact = gun.gun_damage( true, true );

    proj.range = gun.gun_range();
    proj.proj_effects = gun.ammo_effects();

    auto &fx = proj.proj_effects;

    if( ( gun.has_ammo_data() && gun.ammo_data()->phase == phase_id::LIQUID ) ||
        fx.count( ammo_effect_SHOT ) || fx.count( ammo_effect_BOUNCE ) ) {
        fx.insert( ammo_effect_WIDE );
    }

    if( gun.has_ammo() ) {
        const auto &ammo = gun.ammo_data()->ammo;

        // Some projectiles have a chance of being recoverable
        bool recover = x_in_y( ammo->recovery_chance, 100 );

        if( recover && !fx.count( ammo_effect_IGNITE ) && !fx.count( ammo_effect_EXPLOSIVE ) ) {
            item drop( gun.ammo_current(), calendar::turn, 1 );
            drop.active = fx.count( ammo_effect_ACT_ON_RANGED_HIT );
            drop.set_favorite( gun.get_contents().first_ammo().is_favorite );
            proj.set_drop( drop );
        }

        proj.critical_multiplier = ammo->critical_multiplier;
        proj.count = ammo->count;
        proj.shot_spread = ammo->shot_spread * gun.gun_shot_spread_multiplier();
        if( !ammo->drop.is_null() && x_in_y( ammo->drop_chance, 1.0 ) ) {
            item drop( ammo->drop );
            if( ammo->drop_active ) {
                drop.activate();
            }
            proj.set_drop( drop );
        }

        if( fx.count( ammo_effect_CUSTOM_EXPLOSION ) > 0 ) {
            proj.set_custom_explosion( gun.ammo_data()->explosion );
        }
    }

    return proj;
}

int time_to_attack( const Character &p, const itype &firing )
{
    const skill_id &skill_used = firing.gun->skill_used;
    const time_info_t &info = skill_used->time_to_attack();
    return std::max( info.min_time,
                     static_cast<int>( round( info.base_time - info.time_reduction_per_level * p.get_skill_level(
                                           skill_used ) ) ) );
}

int RAS_time( const Character &p, const item_location &loc )
{
    int time = 0;
    if( loc ) {
        // At low stamina levels, firing starts getting slow.
        const item_location gun = p.get_wielded_item();
        int sta_percent = ( 100 * p.get_stamina() ) / p.get_stamina_max();
        time += ( sta_percent < 25 ) ? ( ( 25 - sta_percent ) * 2 ) : 0;
        item::reload_option opt = item::reload_option( &p, gun, loc );
        time += opt.moves();
    }
    return time;
}

static void cycle_action( item &weap, const itype_id &ammo, map *here, const tripoint_bub_ms &pos )
{
    // eject casings and linkages in random direction avoiding walls using player position as fallback
    std::vector<tripoint_bub_ms> tiles = closest_points_first( pos, 1 );
    tiles.erase( tiles.begin() );
    tiles.erase( std::remove_if( tiles.begin(), tiles.end(), [&]( const tripoint_bub_ms & e ) {
        return !here->passable( e );
    } ), tiles.end() );
    tripoint_bub_ms eject{ tiles.empty() ? pos : random_entry( tiles ) };


    // for turrets try and drop casings or linkages directly to any CARGO part on the same tile
    const std::optional<vpart_reference> ovp_cargo = weap.has_flag( flag_VEHICLE )
            ? here->veh_at( pos ).cargo()
            : std::nullopt;

    item *brass_catcher = weap.gunmod_find_by_flag( flag_BRASS_CATCHER );
    if( !!ammo->ammo->casing ) {
        item casing = item( *ammo->ammo->casing );
        // blackpowder can gum up casings too
        if( ( *ammo->ammo ).ammo_effects.count( ammo_effect_BLACKPOWDER ) ) {
            casing.set_flag( json_flag_FILTHY );
        }
        if( weap.has_flag( flag_RELOAD_EJECT ) ) {
            weap.force_insert_item( casing.set_flag( flag_CASING ),
                                    pocket_type::MAGAZINE );
            weap.on_contents_changed();
        } else {
            if( brass_catcher && brass_catcher->can_contain( casing ).success() ) {
                brass_catcher->put_in( casing, pocket_type::CONTAINER );
            } else if( ovp_cargo ) {
                ovp_cargo->vehicle().add_item( *here, ovp_cargo->part(), casing );
            } else {
                here->add_item_or_charges( eject, casing );
            }

            // TODO: Refine critera to handle overlapping maps.
            if( here == &reality_bubble() ) {
                sfx::play_variant_sound( "fire_gun", "brass_eject", sfx::get_heard_volume( eject ),
                                         sfx::get_heard_angle( eject ) );
            }
        }
    }

    // some magazines also eject disintegrating linkages
    const item *mag = weap.magazine_current();
    if( mag && mag->type->magazine->linkage ) {
        item linkage( *mag->type->magazine->linkage, calendar::turn, 1 );
        if( !( brass_catcher &&
               brass_catcher->put_in( linkage, pocket_type::CONTAINER ).success() ) ) {
            if( ovp_cargo ) {
                ovp_cargo->items().insert( *here, linkage );
            } else {
                here->add_item_or_charges( eject, linkage );
            }
        }
    }
}

void make_gun_sound_effect( const Character &p, bool burst, item *weapon )
{
    const item::sound_data data = weapon->gun_noise( burst );
    if( data.volume > 0 ) {
        sounds::sound( p.pos_bub(), data.volume, sounds::sound_t::combat,
                       data.sound.empty() ? _( "Bang!" ) : data.sound );
    }
    tname::segment_bitset segs( tname::unprefixed_tname );
    segs.set( tname::segments::CONTENTS, false );
    p.add_msg_if_player( _( "You shoot your %1$s.  %2$s" ), weapon->tname( 1, segs ),
                         uppercase_first_letter( data.sound ) );
}

item::sound_data item::gun_noise( const bool burst ) const
{
    if( !is_gun() ) {
        return { 0, "" };
    }

    int noise = type->gun->loudness;
    if( has_ammo() ) {
        noise += ammo_data()->ammo->loudness;
    }
    for( const item *mod : gunmods() ) {
        noise += mod->type->gunmod->loudness;
        noise *= mod->type->gunmod->loudness_multiplier;
    }

    noise = std::max( noise, 0 );

    const std::set<ammotype> &at = ammo_types();
    if( at.count( ammo_40x46mm ) || at.count( ammo_40x53mm ) ) {
        // Grenade launchers
        return { 8, _( "Thunk!" ) };

    } else if( at.count( ammo_12mm ) || at.count( ammo_metal_rail ) ) {
        // Railguns
        return { 24, _( "tz-CRACKck!" ) };

    } else if( at.count( ammo_flammable ) || at.count( ammo_66mm ) || at.count( ammo_120mm ) ||
               at.count( ammo_84x246mm ) || at.count( ammo_m235 ) || at.count( ammo_atgm ) ||
               at.count( ammo_RPG_7 ) || at.count( ammo_homebrew_rocket ) ) {
        // Rocket launchers and flamethrowers
        return { 4, _( "Fwoosh!" ) };
    } else if( at.count( ammo_arrow ) ) {
        return { noise, _( "whizz!" ) };
    } else if( at.count( ammo_strange_arrow ) ) {
        return { noise, _( "Crack!" ) };
    } else if( at.count( ammo_bolt ) || at.count( ammo_bolt_ballista ) ) {
        return { noise, _( "thonk!" ) };
    } else if( at.count( ammo_atlatl ) ) {
        return { noise, _( "swoosh!" ) };
    }

    auto fx = ammo_effects();

    if( fx.count( ammo_effect_LASER ) || fx.count( ammo_effect_PLASMA ) ) {
        if( noise < 20 ) {
            return { noise, _( "Fzzt!" ) };
        } else if( noise < 40 ) {
            return { noise, _( "Pew!" ) };
        } else if( noise < 60 ) {
            return { noise, _( "Tsewww!" ) };
        } else {
            return { noise, _( "Kra-kow!" ) };
        }

    } else if( fx.count( ammo_effect_LIGHTNING ) ) {
        if( noise < 20 ) {
            return { noise, _( "Bzzt!" ) };
        } else if( noise < 40 ) {
            return { noise, _( "Bzap!" ) };
        } else if( noise < 60 ) {
            return { noise, _( "Bzaapp!" ) };
        } else {
            return { noise, _( "Kra-koom!" ) };
        }

    } else if( fx.count( ammo_effect_WHIP ) ) {
        return { noise, _( "Crack!" ) };

    } else if( noise > 0 ) {
        if( noise < 10 ) {
            return { noise, burst ? _( "Brrrip!" ) : _( "plink!" ) };
        } else if( noise < 150 ) {
            return { noise, burst ? _( "Brrrap!" ) : _( "bang!" ) };
        } else if( noise < 175 ) {
            return { noise, burst ? _( "P-p-p-pow!" ) : _( "blam!" ) };
        } else {
            return { noise, burst ? _( "Kaboom!" ) : _( "kerblam!" ) };
        }
    }

    return { 0, "" }; // silent weapons
}

static double dispersion_from_skill( double skill, double weapon_dispersion )
{
    if( skill >= MAX_SKILL ) {
        return 0.0;
    }
    double skill_shortfall = static_cast<double>( MAX_SKILL ) - skill;
    double dispersion_penalty = 10 * skill_shortfall;
    double skill_threshold = 5;
    if( skill >= skill_threshold ) {
        double post_threshold_skill_shortfall = static_cast<double>( MAX_SKILL ) - skill;
        // Lack of mastery multiplies the dispersion of the weapon.
        return dispersion_penalty + ( weapon_dispersion * post_threshold_skill_shortfall * 1.25 ) /
               ( static_cast<double>( MAX_SKILL ) - skill_threshold );
    }
    // Unskilled shooters suffer greater penalties, still scaling with weapon penalties.
    double pre_threshold_skill_shortfall = skill_threshold - skill;
    dispersion_penalty += weapon_dispersion *
                          ( 1.25 + pre_threshold_skill_shortfall * 10.0 / skill_threshold );

    return dispersion_penalty;
}

// utility functions for projectile_attack
dispersion_sources Character::get_weapon_dispersion( const item &obj ) const
{
    int weapon_dispersion = obj.gun_dispersion();
    dispersion_sources dispersion( weapon_dispersion );
    dispersion.add_range( ranged_dex_mod() );

    dispersion.add_range( get_modifier( character_modifier_ranged_dispersion_manip_mod ) );

    if( is_driving() ) {
        // get volume of gun (or for auxiliary gunmods the parent gun)
        const item *parent = has_item( obj ) ? find_parent( obj ) : nullptr;
        const int vol = ( parent ? parent->volume() : obj.volume() ) / 250_ml;

        /** @EFFECT_DRIVING reduces the inaccuracy penalty when using guns whilst driving */
        dispersion.add_range( std::max( vol - get_skill_level( skill_driving ), 1.0f ) * 20 );
    }

    /** @EFFECT_GUN improves usage of accurate weapons and sights */
    double avgSkill = static_cast<double>( get_skill_level( skill_gun ) +
                                           get_skill_level( obj.gun_skill() ) ) / 2.0;
    avgSkill = std::min( avgSkill, static_cast<double>( MAX_SKILL ) );
    // If the value is affected by the accuracy of the firearm itself,
    // then beginners will rely heavily on high-precision weapons, while experts are not.
    // Obviously this is not true.
    // So use a constant instead.
    if( obj.gun_skill() == skill_archery ) {
        dispersion.add_range( dispersion_from_skill( avgSkill,
                              450 / get_option< float >( "GUN_DISPERSION_DIVIDER" ) ) );
    } else {
        dispersion.add_range( dispersion_from_skill( avgSkill,
                              300 / get_option< float >( "GUN_DISPERSION_DIVIDER" ) ) );
    }

    float disperation_mod = enchantment_cache->modify_value( enchant_vals::mod::WEAPON_DISPERSION,
                            1.0f );
    if( disperation_mod != 1.0f ) {
        dispersion.add_multiplier( disperation_mod );
    }

    // Range is effectively four times longer when shooting unflagged/flagged guns underwater/out of water.
    if( is_underwater() != obj.has_flag( flag_UNDERWATER_GUN ) ) {
        // Adding dispersion for additional debuff
        dispersion.add_range( 150 );
        dispersion.add_multiplier( 4 );
    }

    return dispersion;
}

double Character::gun_value( const item &weap, int ammo ) const
{
    // TODO: Mods
    // TODO: Allow using a specified type of ammo rather than default or current
    if( !weap.type->gun ) {
        return 0.0;
    }

    if( ammo <= 0 || !meets_requirements( weap ) ) {
        return 0.0;
    }

    const islot_gun &gun = *weap.type->gun;
    itype_id ammo_type = itype_id::NULL_ID();
    if( !weap.ammo_current().is_null() ) {
        ammo_type = weap.ammo_current();
    } else if( weap.magazine_current() ) {
        ammo_type = weap.common_ammo_default();
    } else if( weap.is_magazine() ) {
        ammo_type = weap.ammo_default();
    } else if( !weap.magazine_default().is_null() ) {
        ammo_type = item( weap.magazine_default() ).ammo_default();
    }
    const itype *def_ammo_i = !ammo_type.is_null() ?
                              item::find_type( ammo_type ) :
                              nullptr;

    damage_instance gun_damage = weap.gun_damage();
    item tmp = weap;
    if( tmp.is_magazine() || tmp.magazine_current() || tmp.magazine_default() ) {
        tmp.ammo_set( ammo_type );
    }
    int total_dispersion = get_weapon_dispersion( tmp ).max() + tmp.sight_dispersion( *this ).second;

    if( def_ammo_i != nullptr && def_ammo_i->ammo ) {
        const islot_ammo &def_ammo = *def_ammo_i->ammo;
        gun_damage.add( def_ammo.damage );
        total_dispersion += def_ammo.dispersion;
    }

    float damage_factor = gun_damage.total_damage();
    if( damage_factor > 0 && !gun_damage.empty() ) {
        // TODO: Multiple damage types
        damage_factor += 0.5f * gun_damage.damage_units.front().res_pen;
    }

    int move_cost = time_to_attack( *this, *weap.type );
    if( gun.clip != 0 && gun.clip < 10 ) {
        // TODO: RELOAD_ONE should get a penalty here
        int reload_cost = gun.reload_time + encumb( bodypart_id( "hand_l" ) ) + encumb(
                              bodypart_id( "hand_r" ) );
        reload_cost /= gun.clip;
        move_cost += reload_cost;
    }

    // "Medium range" below means 9 tiles, "short range" means 4
    // Those are guarantees (assuming maximum time spent aiming)
    static const std::vector<std::pair<float, float>> dispersion_thresholds = {{
            // Headshots all the time
            { 0.0f, 5.0f },
            // Critical at medium range
            { 100.0f, 4.5f },
            // Critical at short range or good hit at medium
            { 200.0f, 3.5f },
            // OK hits at medium
            { 300.0f, 3.0f },
            // Point blank headshots
            { 450.0f, 2.5f },
            // OK hits at short
            { 700.0f, 1.5f },
            // Glances at medium, criticals at point blank
            { 1000.0f, 1.0f },
            // Nothing guaranteed, pure gamble
            { 2000.0f, 0.1f },
        }
    };

    static const std::vector<std::pair<float, float>> move_cost_thresholds = {{
            { 10.0f, 4.0f },
            { 25.0f, 3.0f },
            { 100.0f, 1.0f },
            { 500.0f, 0.5f },
        }
    };

    float move_cost_factor = multi_lerp( move_cost_thresholds, move_cost );

    // Penalty for dodging in melee makes the gun unusable in melee
    // Until NPCs get proper kiting, at least
    int melee_penalty = weap.volume() / 250_ml - get_skill_level( skill_dodge );
    if( melee_penalty <= 0 ) {
        // Dispersion matters less if you can just use the gun in melee
        total_dispersion = std::min<int>( total_dispersion / move_cost_factor, total_dispersion );
    }

    float dispersion_factor = multi_lerp( dispersion_thresholds, total_dispersion );

    float damage_and_accuracy = damage_factor * dispersion_factor;

    // TODO: Some better approximation of the ability to keep on shooting
    static const std::vector<std::pair<float, float>> capacity_thresholds = {{
            { 1.0f, 0.5f },
            { 5.0f, 1.0f },
            { 10.0f, 1.5f },
            { 20.0f, 2.0f },
            { 50.0f, 3.0f },
        }
    };

    // How much until reload
    float capacity = gun.clip > 0 ? std::min<float>( gun.clip, ammo ) : ammo;
    // How much until dry and a new weapon is needed
    capacity += std::min<float>( 1.0, ammo / 20.0 );
    double capacity_factor = multi_lerp( capacity_thresholds, capacity );

    double gun_value = damage_and_accuracy * capacity_factor;

    add_msg_debug( debugmode::DF_RANGED,
                   "%s as gun: %.1f total, %.1f dispersion, %.1f damage, %.1f capacity",
                   weap.type->get_id().str(), gun_value, dispersion_factor, damage_factor,
                   capacity_factor );
    return std::max( 0.0, gun_value );
}

target_handler::trajectory target_ui::run()
{
    if( mode == TargetMode::Spell && !no_mana && !casting->can_cast( *you ) ) {
        you->add_msg_if_player( m_bad, _( "You don't have enough %s to cast this spell" ),
                                casting->energy_string() );
    } else if( mode == TargetMode::Fire ) {
        update_ammo_range_from_gun_mode();
        sight_dispersion = you->most_accurate_aiming_method_limit( *relevant );
    }

    map &here = get_map();
    // Load settings
    unload_RAS_weapon = get_option<bool>( "UNLOAD_RAS_WEAPON" );
    snap_to_target = get_option<bool>( "SNAP_TO_TARGET" );
    if( mode == TargetMode::Turrets ) {
        // Due to how cluttered the display would become, disable it by default
        // unless aiming a single turret.
        draw_turret_lines = vturrets->size() == 1;
    }

    avatar &player_character = get_avatar();
    on_out_of_scope cleanup( [&here, &player_character]() {
        here.invalidate_map_cache( player_character.posz() + player_character.view_offset.z() );
    } );

    shared_ptr_fast<game::draw_callback_t> target_ui_cb = make_shared_fast<game::draw_callback_t>(
    [&]() {
        draw_terrain_overlay();
    } );
    g->add_draw_callback( target_ui_cb );

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        init_window_and_input();
        ui.position_from_window( w_target );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_ui_window();
    } );

    // Handle multi-turn aiming
    std::string action;
    bool attack_was_confirmed = false;
    bool reentered = false;
    bool resume_critter = false;
    if( mode == TargetMode::Fire && !activity->action.empty() ) {
        // We were in this UI during previous turn...
        reentered = true;
        std::string act_data = activity->action;
        if( act_data == "AIM" ) {
            // ...and ran out of moves while aiming.
        } else {
            // ...and selected 'aim and shoot', but ran out of moves.
            // So, skip retrieving input and go straight to the action.
            action = act_data;
            attack_was_confirmed = true;
        }
        // Load state to keep the ui consistent across turns
        unload_RAS_weapon = activity->should_unload_RAS;
        snap_to_target = activity->snap_to_target;
        shifting_view = activity->shifting_view;
        resume_critter = activity->aiming_at_critter;
    }

    // Initialize cursor position
    src = you->pos_bub();
    update_target_list();

    if( activity && activity->abort_if_no_targets && targets.empty() ) {
        // this branch is taken when already shot once and re-entered
        // aiming, if no targets are available we want to abort so
        // players don't arrive at aiming ui with nothing to shoot at.
        activity->aborted = true;
        traj.clear();
        return traj;
    }
    tripoint_bub_ms initial_dst = src;
    if( reentered ) {
        if( !try_reacquire_target( resume_critter, initial_dst ) ) {
            // Target lost
            action.clear();
            attack_was_confirmed = false;
        }
    } else {
        initial_dst = choose_initial_target();
    }
    set_cursor_pos( initial_dst );
    if( dst != initial_dst ) {
        // Our target moved out of range
        action.clear();
        attack_was_confirmed = false;
    }
    if( mode == TargetMode::Fire ) {
        if( activity->aif_duration > AIF_DURATION_LIMIT ) {
            // Break long (potentially infinite) aim-and-fire loop.
            // May happen if e.g. avatar tries to get 'precise' shot while being
            // attacked by multiple zombies, which triggers dodges and corresponding aim loss.
            action.clear();
            attack_was_confirmed = false;
        }
        if( !action.empty() && !prompt_friendlies_in_lof() ) {
            // A friendly creature moved into line of fire during aim-and-shoot,
            // and player decided to stop aiming
            action.clear();
            attack_was_confirmed = false;
        }
        activity->acceptable_losses.clear();
        if( action.empty() ) {
            activity->aif_duration = 0;
        } else {
            activity->aif_duration += 1;
        }
    }

    // Event loop!
    ExitCode loop_exit_code;
    std::string timed_out_action;
    bool skip_redraw = false;
    for( ;; action.clear() ) {
        if( !skip_redraw ) {
            g->invalidate_main_ui_adaptor();
            ui_manager::redraw();
        }
        skip_redraw = false;

        // Wait for user input (or use value retrieved from activity)
        if( action.empty() ) {
            int timeout = get_option<int>( "EDGE_SCROLL" );
            action = ctxt.handle_input( timeout );
        }

        // If an aiming mode is selected, use "*_SHOT" instead of "FIRE"
        if( mode == TargetMode::Fire && action == "FIRE" && aim_mode->has_threshold ) {
            action = aim_mode->action;
        }

        // Handle received input
        if( handle_cursor_movement( action, skip_redraw ) ) {
            continue;
        } else if( action == "TOGGLE_UNLOAD_RAS_WEAPON" ) {
            unload_RAS_weapon = !unload_RAS_weapon;
            activity->should_unload_RAS = unload_RAS_weapon;
        } else if( action == "TOGGLE_SNAP_TO_TARGET" ) {
            toggle_snap_to_target();
        } else if( action == "TOGGLE_TURRET_LINES" ) {
            draw_turret_lines = !draw_turret_lines;
        } else if( action == "TOGGLE_MOVE_CURSOR_VIEW" ) {
            if( snap_to_target ) {
                toggle_snap_to_target();
            }
            shifting_view = !shifting_view;
        } else if( action == "zoom_in" ) {
            g->zoom_in();
            g->mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            g->zoom_out();
            g->mark_main_ui_adaptor_resize();
        } else if( action == "QUIT" ) {
            loop_exit_code = ExitCode::Abort;
            break;
        } else if( action == "SWITCH_MODE" ) {
            if( action_switch_mode() ) {
                loop_exit_code = ExitCode::Abort;
                break;
            }
        } else if( action == "SWITCH_AMMO" ) {
            if( !action_switch_ammo() ) {
                loop_exit_code = ExitCode::Reload;
                break;
            }
        } else if( action == "FIRE" ) {
            if( status != Status::Good ) {
                continue;
            }
            bool can_skip_confirm = mode == TargetMode::Spell && casting->damage( player_character ) <= 0;
            if( !can_skip_confirm && !confirm_non_enemy_target() ) {
                continue;
            }
            set_last_target();
            loop_exit_code = ExitCode::Fire;
            break;
        } else if( action == "AIM" ) {
            if( status != Status::Good ) {
                continue;
            }

            // No confirm_non_enemy_target here because we have not initiated the firing.
            // Aiming can be stopped / aborted at any time.

            if( !action_aim() ) {
                timed_out_action = "AIM";
                loop_exit_code = ExitCode::Timeout;
                break;
            }

            if( relevant->has_flag( flag_RELOAD_AND_SHOOT ) ) {
                if( !relevant->ammo_remaining( ) && activity->reload_loc ) {
                    you->mod_moves( -RAS_time( *you, activity->reload_loc ) );
                    relevant->reload( get_avatar(), activity->reload_loc, 1 );
                    you->burn_energy_arms( - relevant->get_min_str() * static_cast<int>( 0.006f *
                                           get_option<int>( "PLAYER_MAX_STAMINA_BASE" ) ) );
                    activity->reload_loc = item_location();
                }
            }
        } else if( action == "STOPAIM" ) {
            if( status != Status::Good ) {
                continue;
            }

            action_drop_aim();
        } else if( action == "AIMED_SHOT" || action == "CAREFUL_SHOT" || action == "PRECISE_SHOT" ) {
            if( status != Status::Good ) {
                continue;
            }

            // This action basically means "Fire" as well; the actual firing may be delayed
            // through aiming, but there is usually no means to abort it. Therefore we query now
            if( !attack_was_confirmed && !confirm_non_enemy_target() ) {
                continue;
            }

            if( action_aim_and_shoot( action ) ) {
                loop_exit_code = ExitCode::Fire;
            } else {
                timed_out_action = action;
                loop_exit_code = ExitCode::Timeout;
            }
            break;
        }
    } // for(;;)

    switch( loop_exit_code ) {
        case ExitCode::Abort: {
            traj.clear();
            if( mode == TargetMode::Fire || ( mode == TargetMode::Reach && activity ) ) {
                activity->aborted = true;
                activity->should_unload_RAS = unload_RAS_weapon;
            }
            break;
        }
        case ExitCode::Fire: {
            if( mode != TargetMode::SelectOnly ) {
                bool harmful = !( mode == TargetMode::Spell && casting->damage( player_character ) <= 0 );
                on_target_accepted( harmful );
            }
            break;
        }
        case ExitCode::Timeout: {
            // We've ran out of moves, save UI state
            activity->acceptable_losses = list_friendlies_in_lof();
            traj.clear();
            activity->action = timed_out_action;
            activity->should_unload_RAS = unload_RAS_weapon;
            activity->snap_to_target = snap_to_target;
            activity->shifting_view = shifting_view;
            activity->aiming_at_critter = !!dst_critter;
            break;
        }
        case ExitCode::Reload: {
            traj.clear();
            activity->aborted = true;
            activity->reload_requested = true;
            break;
        }
    }

    return traj;
}

void target_ui::init_window_and_input()
{
    int top = 0;
    int width = panel_manager::get_manager().get_current_layout().panels().begin()->get_width();
    int height;
    narrow = width <= 42;
    if( narrow ) {
        if( width < 34 ) {
            width = 34;
        }
        height = 24;
        compact = true;
    } else {
        width = 55;
        compact = TERMY < 41;
        tiny = TERMY < 28;
        bool use_whole_sidebar = TERMY < 32;
        if( use_whole_sidebar ) {
            // If we're extremely short on space, use the whole sidebar.
            height = TERMY;
        } else if( compact ) {
            // Cover up more low-value ui elements if we're tight on space.
            height = 28;
        } else {
            // Go all out
            height = 33;
        }
    }

    w_target = catacurses::newwin( height, width, point( TERMX - width, top ) );

    ctxt = input_context( "TARGET" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "FIRE" );
    ctxt.register_action( "NEXT_TARGET" );
    ctxt.register_action( "PREV_TARGET" );
    ctxt.register_action( "CENTER" );
    ctxt.register_action( "TOGGLE_UNLOAD_RAS_WEAPON" );
    ctxt.register_action( "TOGGLE_SNAP_TO_TARGET" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "TOGGLE_MOVE_CURSOR_VIEW" );
    if( fov_3d_z_range > 0 ) {
        ctxt.register_action( "LEVEL_UP" );
        ctxt.register_action( "LEVEL_DOWN" );
    }
    if( mode == TargetMode::Fire || mode == TargetMode::TurretManual || ( mode == TargetMode::Reach &&
            ( relevant && relevant->is_gun() ) && you->get_aim_types( *relevant ).size() > 1 ) ) {
        ctxt.register_action( "SWITCH_MODE" );
        if( mode == TargetMode::Fire || mode == TargetMode::TurretManual ) {
            ctxt.register_action( "SWITCH_AMMO" );
        }
    }
    if( mode == TargetMode::Fire ) {
        ctxt.register_action( "AIM" );
        ctxt.register_action( "STOPAIM" );

        aim_types = you->get_aim_types( *relevant );
        for( aim_type &type : aim_types ) {
            if( type.has_threshold ) {
                ctxt.register_action( type.action );
            }
        }
        aim_mode = aim_types.begin();
        for( auto it = aim_types.begin(); it != aim_types.end(); ++it ) {
            if( you->preferred_aiming_mode == it->action ) {
                aim_mode = it; // default to persisted mode if possible
            }
        }
    }
    if( mode == TargetMode::Turrets ) {
        ctxt.register_action( "TOGGLE_TURRET_LINES" );
    }
}

bool target_ui::handle_cursor_movement( const std::string &action, bool &skip_redraw )
{
    std::optional<tripoint_bub_ms> mouse_pos;
    const auto shift_view_or_cursor = [this]( const tripoint_rel_ms & delta ) {
        if( this->shifting_view ) {
            this->set_view_offset( this->you->view_offset + delta );
        } else {
            this->set_cursor_pos( dst + delta );
        }
    };

    if( action == "MOUSE_MOVE" || action == "TIMEOUT" ) {
        // Shift pos and/or view via edge scrolling
        tripoint_rel_ms edge_scroll = g->mouse_edge_scrolling_terrain( ctxt );
        if( edge_scroll == tripoint_rel_ms::zero ) {
            skip_redraw = true;
        } else {
            if( action == "MOUSE_MOVE" ) {
                edge_scroll.raw() *= 2; // TODO: Make *= etc. available to relative coordinates.
            }
            if( snap_to_target ) {
                set_cursor_pos( dst + edge_scroll );
            } else {
                set_view_offset( you->view_offset + edge_scroll );
            }
        }
    } else if( const std::optional<tripoint_rel_ms> delta = ctxt.get_direction_rel_ms( action ) ) {
        // Shift view/cursor with directional keys
        shift_view_or_cursor( delta.value() );
    } else if( action == "SELECT" &&
               ( mouse_pos = ctxt.get_coordinates( g->w_terrain, g->ter_view_p.raw().xy() ) ) ) {
        // Set pos by clicking with mouse
        mouse_pos->z() = you->posz() + you->view_offset.z();
        set_cursor_pos( *mouse_pos );
    } else if( action == "LEVEL_UP" || action == "LEVEL_DOWN" ) {
        // Shift view/cursor up/down one z level
        tripoint_rel_ms delta = tripoint_rel_ms(
                                    0,
                                    0,
                                    action == "LEVEL_UP" ? 1 : -1
                                );
        shift_view_or_cursor( delta );
    } else if( action == "NEXT_TARGET" ) {
        cycle_targets( 1 );
    } else if( action == "PREV_TARGET" ) {
        cycle_targets( -1 );
    } else if( action == "CENTER" ) {
        if( shifting_view ) {
            set_view_offset( tripoint_rel_ms::zero );
        } else {
            set_cursor_pos( src );
        }
    } else {
        return false;
    }

    return true;
}

bool target_ui::set_cursor_pos( const tripoint_bub_ms &new_pos )
{
    if( dst == new_pos ) {
        return false;
    }
    if( status == Status::OutOfAmmo && new_pos != src ) {
        // range == 0, no sense in moving cursor
        return false;
    }

    // Make sure new position is valid or find a closest valid position
    std::vector<tripoint_bub_ms> new_traj;
    tripoint_bub_ms valid_pos = new_pos;
    map &here = get_map();
    if( new_pos != src ) {
        // On Z axis, make sure we do not exceed map boundaries
        valid_pos.z() = clamp( valid_pos.z(), -OVERMAP_DEPTH, OVERMAP_HEIGHT );
        // Or current view range
        valid_pos.z() = clamp( valid_pos.z() - src.z(), -fov_3d_z_range, fov_3d_z_range ) + src.z();

        new_traj = here.find_clear_path( src, valid_pos );
        if( range == 1 ) {
            // We should always be able to hit adjacent squares
            if( square_dist( src, valid_pos ) > 1 ) {
                valid_pos = new_traj[0];
            }
        } else if( trigdist ) {
            if( dist_fn( valid_pos ) > range ) {
                // Find the farthest point that is still in range
                for( size_t i = new_traj.size(); i > 0; i-- ) {
                    if( dist_fn( new_traj[i - 1] ) <= range ) {
                        valid_pos = new_traj[i - 1];
                        break;
                    }
                }

                // FIXME: due to a bug in map::find_clear_path (#39693),
                //        returned trajectory is invalid in some cases.
                //        This bandaid stops us from exceeding range,
                //        but does not fix the issue.
                if( dist_fn( valid_pos ) > range ) {
                    debugmsg( "Exceeded allowed range!" );
                    valid_pos = src;
                }
            }
        } else {
            tripoint_rel_ms delta = valid_pos - src;
            valid_pos = src + tripoint(
                            clamp( delta.x(), -range, range ),
                            clamp( delta.y(), -range, range ),
                            clamp( delta.z(), -range, range )
                        );
        }
    } else {
        new_traj.push_back( src );
    }

    if( valid_pos == dst ) {
        // We don't need to move the cursor after all
        return false;
    } else if( new_pos == valid_pos ) {
        // We can reuse new_traj
        dst = valid_pos;
        traj = new_traj;
    } else {
        dst = valid_pos;
        traj = here.find_clear_path( src, dst );
    }

    if( snap_to_target ) {
        set_view_offset( dst - src );
    }

    // Make player's sprite flip to face the current target
    point_rel_ms d( dst.xy() - src.xy() );
    if( !g->is_tileset_isometric() ) {
        if( d.x() > 0 ) {
            you->facing = FacingDirection::RIGHT;
        } else if( d.x() < 0 ) {
            you->facing = FacingDirection::LEFT;
        }
    } else {
        if( d.x() >= 0 && d.y() >= 0 ) {
            you->facing = FacingDirection::RIGHT;
        }
        if( d.y() <= 0 && d.x() <= 0 ) {
            you->facing = FacingDirection::LEFT;
        }
    }

    // Cache creature under cursor
    if( src != dst ) {
        if( targeting_Mode == LegalTargets::Creatures ) {
            Creature *cr = get_creature_tracker().creature_at( dst, true );
            if( cr && pl_sees( *cr ) ) {
                dst_critter = cr;
            } else {
                dst_critter = nullptr;
            }
        } else if( targeting_Mode == LegalTargets::Vehicles ) {
            optional_vpart_position ovp = here.veh_at( dst );
            if( ovp.has_value() && pl_sees( ovp ) ) {
                dst_veh = &ovp.value().vehicle();
            } else {
                dst_veh = nullptr;
            }
        } else {
            Creature *cr = get_creature_tracker().creature_at( dst, true );
            if( cr && pl_sees( *cr ) ) {
                dst_critter = cr;
                dst_veh = nullptr;
            } else {
                optional_vpart_position ovp = here.veh_at( dst );
                if( ovp.has_value() && pl_sees( ovp ) ) {
                    dst_veh = &ovp.value().vehicle();
                    dst_critter = nullptr;
                } else {
                    dst_veh = nullptr;
                    dst_critter = nullptr;
                }
            }
        }
    } else {
        dst_critter = nullptr;
        dst_veh = nullptr;
    }

    // Update mode-specific stuff
    if( mode == TargetMode::Fire ) {
        recalc_aim_turning_penalty();
    } else if( mode == TargetMode::Spell ) {
        switch( casting->shape() ) {
            case spell_shape::blast:
                spell_aoe = spell_effect::spell_effect_blast(
                                spell_effect::override_parameters( *casting, get_player_character() ), src, dst );
                break;
            case spell_shape::cone:
                spell_aoe = spell_effect::spell_effect_cone(
                                spell_effect::override_parameters( *casting, get_player_character() ), src, dst );
                break;
            case spell_shape::line:
                spell_aoe = spell_effect::spell_effect_line(
                                spell_effect::override_parameters( *casting, get_player_character() ), src, dst );
                break;
            default:
                spell_aoe.clear();
                debugmsg( "%s does not have valid spell shape", casting->id().str() );
                break;
        }
    } else if( mode == TargetMode::Turrets ) {
        update_turrets_in_range();
    }

    // Update UI controls & colors
    update_status();

    return true;
}

void target_ui::on_range_ammo_changed()
{
    update_status();
    update_target_list();
}

void target_ui::update_target_list()
{
    if( range == 0 ) {
        targets.clear();
        return;
    }

    // Get targets in range and sort them by distance (targets[0] is the closest)
    if( targeting_Mode == LegalTargets::Both || targeting_Mode == LegalTargets::Vehicles ) {
        for( vehicle *target : you->get_visible_vehicles( range ) ) {
            targets.push_back( target->pos_bub( get_map() ) );
        }
    }
    if( targeting_Mode == LegalTargets::Both || targeting_Mode == LegalTargets::Creatures ) {
        for( Creature *target : you->get_targetable_creatures( range, mode == TargetMode::Reach ) ) {
            targets.push_back( target->pos_bub() );
        }
    }
    std::sort( targets.begin(), targets.end(), [&]( const tripoint_bub_ms lhs,
    const tripoint_bub_ms rhs ) {
        return rl_dist_exact( lhs, you->pos_bub() ) < rl_dist_exact( rhs, you->pos_bub() );
    } );
}

tripoint_bub_ms target_ui::choose_initial_target()
{
    map &here = get_map();

    // Try previously targeted creature
    shared_ptr_fast<Creature> cr = you->last_target.lock();
    if( cr && pl_sees( *cr ) && dist_fn( cr->pos_bub() ) <= range ) {
        return cr->pos_bub();
    }

    // Try closest creature
    if( !targets.empty() ) {
        return targets[0];
    }

    // Try closest practice target
    std::optional<tripoint_bub_ms> target_spot = find_point_closest_first( src, 0, range, [this,
    &here]( const tripoint_bub_ms & pt ) {
        return here.tr_at( pt ).id == tr_practice_target && this->you->sees( here, pt );
    } );

    if( target_spot != std::nullopt ) {
        return *target_spot;
    }

    // We've got nothing.
    return src;
}

bool target_ui::try_reacquire_target( bool critter, tripoint_bub_ms &new_dst )
{
    if( critter ) {
        // Try to re-acquire the creature
        shared_ptr_fast<Creature> cr = you->last_target.lock();
        if( cr && pl_sees( *cr ) && dist_fn( cr->pos_bub() ) <= range ) {
            new_dst = cr->pos_bub();
            return true;
        }
    }

    if( !you->last_target_pos.has_value() ) {
        // This shouldn't happen
        return false;
    }

    // Try to re-acquire target tile or tile where the target creature used to be
    tripoint_bub_ms local_lt = get_map().get_bub( *you->last_target_pos );
    if( dist_fn( local_lt ) <= range ) {
        new_dst = local_lt;
        // Abort aiming if a creature moved in
        return !critter && !get_creature_tracker().creature_at( local_lt, true );
    }

    // We moved out of range
    return false;
}

void target_ui::update_status()
{
    std::vector<std::string> msgbuf;
    if( mode == TargetMode::Turrets && turrets_in_range.empty() ) { // NOLINT(bugprone-branch-clone)
        // None of the turrets are in range
        status = Status::OutOfRange;
    } else if( mode == TargetMode::Fire &&
               ( !gunmode_checks_common( *you, get_map(), msgbuf, relevant->gun_current_mode() ) ||
                 !gunmode_checks_weapon( *you, get_map(), msgbuf, relevant->gun_current_mode() ) )
             ) { // NOLINT(bugprone-branch-clone)
        // Selected gun mode is empty
        // TODO: it might be some other error, but that's highly unlikely to happen, so a catch-all 'Out of ammo' is fine
        status = Status::OutOfAmmo;
    } else if( mode == TargetMode::TurretManual && ( turret->query() != turret_data::status::ready ||
               !gunmode_checks_common( *you, get_map(), msgbuf, relevant->gun_current_mode() ) ) ) {
        status = Status::OutOfAmmo;
    } else if( ( src == dst ) && !( mode == TargetMode::Spell &&
                                    casting->is_valid_target( spell_target::self ) ) ) {
        // TODO: consider allowing targeting yourself with turrets
        status = Status::BadTarget;
    } else if( dist_fn( dst ) > range ) {
        // We're out of range. This can happen if we switch from long-ranged
        // gun mode to short-ranged. We can, of course, move the cursor into range automatically,
        // but that would be rude. Instead, wait for directional keys/etc. and *then* move the cursor.
        status = Status::OutOfRange;
    } else {
        status = Status::Good;
    }
}

int target_ui::dist_fn( const tripoint_bub_ms &p )
{
    return static_cast<int>( std::round( rl_dist_exact( src, p ) ) );
}

void target_ui::set_last_target()
{
    map &here = get_map();

    if( !you->last_target_pos.has_value() ||
        you->last_target_pos.value() != here.get_abs( dst ) ) {
        you->aim_cache_dirty = true;
    }
    you->last_target_pos = here.get_abs( dst );
    if( dst_critter ) {
        you->last_target = g->shared_from( *dst_critter );
    } else {
        you->last_target.reset();
    }
}

bool target_ui::confirm_non_enemy_target()
{
    // Check if you are casting a spell at yourself.
    if( mode == TargetMode::Spell && ( src == dst || spell_aoe.count( src ) == 1 ) ) {
        if( !query_yn( _( "Really attack yourself?" ) ) ) {
            return false;
        }
    }
    if( targeting_Mode == LegalTargets::Vehicles ) {
        return true;
    }
    npc *const who = dynamic_cast<npc *>( dst_critter );
    if( who && !who->guaranteed_hostile() ) {
        return query_yn( _( "Really attack %s?" ), who->get_name().c_str() );
    }
    return true;
}

bool target_ui::prompt_friendlies_in_lof()
{
    if( mode != TargetMode::Fire ) {
        debugmsg( "Not implemented" );
        return true;
    }

    std::vector<weak_ptr_fast<Creature>> in_lof = list_friendlies_in_lof();
    std::vector<Creature *> new_in_lof;
    for( const weak_ptr_fast<Creature> &cr_ptr : in_lof ) {
        bool found = false;
        Creature *cr = cr_ptr.lock().get();
        for( const weak_ptr_fast<Creature> &cr2_ptr : activity->acceptable_losses ) {
            Creature *cr2 = cr2_ptr.lock().get();
            if( cr == cr2 ) {
                found = true;
                break;
            }
        }
        if( !found ) {
            new_in_lof.push_back( cr );
        }
    }

    if( new_in_lof.empty() ) {
        return true;
    }

    std::string msg = _( "There are friendly creatures in line of fire:\n" );
    for( Creature *cr : new_in_lof ) {
        msg += "  " + cr->disp_name() + "\n";
    }
    msg += _( "Proceed with the attack?" );
    return query_yn( msg );
}

std::vector<weak_ptr_fast<Creature>> target_ui::list_friendlies_in_lof()
{
    const map &here = get_map();

    std::vector<weak_ptr_fast<Creature>> ret;
    if( mode == TargetMode::Turrets || mode == TargetMode::Spell ) {
        debugmsg( "Not implemented" );
        return ret;
    }
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &p : traj ) {
        if( p != dst && p != src ) {
            Creature *cr = creatures.creature_at( p, true );
            if( cr && you->sees( here, *cr ) ) {
                Creature::Attitude a = cr->attitude_to( *this->you );
                if(
                    ( cr->is_npc() && a != Creature::Attitude::HOSTILE ) ||
                    ( !cr->is_npc() && a == Creature::Attitude::FRIENDLY )
                ) {
                    ret.emplace_back( g->shared_from( *cr ) );
                }
            }
        }
    }
    return ret;
}

void target_ui::toggle_snap_to_target()
{
    shifting_view = false;
    if( snap_to_target ) {
        // Keep current view offset
    } else {
        set_view_offset( dst - src );
    }
    snap_to_target = !snap_to_target;
}

void target_ui::cycle_targets( int direction )
{
    if( targets.empty() ) {
        // Nothing to cycle
        return;
    }

    tripoint_bub_ms dst_pos;
    if( dst_critter ) {
        dst_pos = dst_critter->pos_bub();
    } else if( dst_veh ) {
        dst_pos = dst_veh->pos_bub( get_map() );
    }
    if( dst_critter || dst_veh ) {
        auto t = std::find( targets.begin(), targets.end(), dst_pos );
        size_t new_target = 0;
        if( t != targets.end() ) {
            size_t idx = std::distance( targets.begin(), t );
            new_target = ( idx + targets.size() + direction ) % targets.size();
            set_cursor_pos( targets[new_target] );
            return;
        }
    }

    // There is either no creature under the cursor or the player can't see it.
    // Use the closest/farthest target in this case
    if( direction == 1 ) {
        set_cursor_pos( targets.front() );
    } else {
        set_cursor_pos( targets.back() );
    }
}

void target_ui::set_view_offset( const tripoint_rel_ms &new_offset ) const
{
    tripoint_rel_ms new_( new_offset.xy(), clamp( new_offset.z(), -fov_3d_z_range, fov_3d_z_range ) );
    new_.z() = clamp( new_.z() + src.z(), -OVERMAP_DEPTH, OVERMAP_HEIGHT ) - src.z();

    bool changed_z = you->view_offset.z() != new_.z();
    you->view_offset = new_;
    if( changed_z ) {
        // We need to do a bunch of cache updates since we're
        // looking at a different z-level.
        get_map().invalidate_map_cache( new_.z() );
    }
}

void target_ui::update_turrets_in_range()
{
    map &here = get_map();

    turrets_in_range.clear();
    for( vehicle_part *t : *vturrets ) {
        turret_data td = veh->turret_query( *t );
        if( td.in_range( here.get_abs( dst ) ) ) {
            tripoint_bub_ms src = veh->bub_part_pos( here, *t );
            turrets_in_range.push_back( {t, line_to( src, dst )} );
        }
    }
}

void target_ui::recalc_aim_turning_penalty()
{
    // since we are recalcing recoil dirty the aimm cache
    you->aim_cache_dirty = true;

    if( status != Status::Good ) {
        // We don't care about invalid situations
        predicted_recoil = MAX_RECOIL;
        return;
    }

    double curr_recoil = you->recoil;
    tripoint_bub_ms curr_recoil_pos;
    const Creature *lt_ptr = you->last_target.lock().get();
    if( lt_ptr ) {
        curr_recoil_pos = lt_ptr->pos_bub();
    } else if( you->last_target_pos ) {
        curr_recoil_pos = get_map().get_bub( *you->last_target_pos );
    } else {
        curr_recoil_pos = src;
    }

    if( curr_recoil_pos == dst ) {
        // We're aiming at that point right now, no penalty
        predicted_recoil = curr_recoil;
    } else if( curr_recoil_pos == src ) {
        // The player wasn't aiming anywhere, max it out
        predicted_recoil = MAX_RECOIL;
    } else {
        // Raise it proportionally to how much
        // the player has to turn from previous aiming point
        // the player loses their aim more quickly if less skilled, normalizing at 5 skill.
        /** @EFFECT_GUN increases the penalty for reorienting aim while below 5 */
        const double skill_penalty = std::max( 0.0, 5.0 - you->get_skill_level( skill_gun ) );
        const double recoil_per_degree = skill_penalty * MAX_RECOIL / 180.0;
        const units::angle angle_curr = coord_to_angle( src, curr_recoil_pos );
        const units::angle angle_desired = coord_to_angle( src, dst );
        const units::angle phi = normalize( angle_curr - angle_desired );
        const units::angle angle = std::min( phi, 360.0_degrees - phi );
        predicted_recoil =
            std::min( MAX_RECOIL, curr_recoil + to_degrees( angle ) * recoil_per_degree );
    }
}

void target_ui::apply_aim_turning_penalty() const
{
    you->recoil = predicted_recoil;
}

void target_ui::update_ammo_range_from_gun_mode()
{
    if( mode == TargetMode::TurretManual ) {
        itype_id ammo_current = turret->ammo_current();
        if( ammo_current.is_null() ) {
            ammo = nullptr;
            range = 0;
        } else {
            ammo = item::find_type( ammo_current );
            range = turret->range();
        }
    } else {
        if( relevant->gun_current_mode().melee() ) {
            range = relevant->current_reach_range( *you );
        } else {
            ammo = activity->reload_loc ? activity->reload_loc.get_item()->type :
                   relevant->gun_current_mode().target->ammo_data();
            if( activity->reload_loc ) {
                item temp_weapon = *relevant;
                temp_weapon.ammo_set( ammo->get_id() );
                range = temp_weapon.gun_current_mode().target->gun_range( you );
            } else {
                range = relevant->gun_current_mode().target->gun_range( you );
            }
        }
    }
}

bool target_ui::action_switch_mode()
{
    uilist menu;
    menu.settext( _( "Select preferences" ) );
    const std::pair<int, int> aim_modes_range = std::make_pair( 0, 100 );
    const std::pair<int, int> firing_modes_range = std::make_pair( 100, 200 );

    if( !aim_types.empty() ) {
        menu.addentry( -1, false, 0, "  " + std::string( _( "Default aiming mode" ) ) );
        menu.entries.back().force_color = true;
        menu.entries.back().text_color = c_cyan;

        for( auto it = aim_types.begin(); it != aim_types.end(); ++it ) {
            const bool is_active_aim_mode = aim_mode == it;
            const std::string text = ( it->name.empty() ? _( "Immediate" ) : it->name ) +
                                     ( is_active_aim_mode ? _( " (active)" ) : "" );
            menu.addentry( aim_modes_range.first + std::distance( aim_types.begin(), it ),
                           true, MENU_AUTOASSIGN, text );
            if( is_active_aim_mode ) {
                menu.entries.back().text_color = c_light_green;
            }
        }
    }

    const std::map<gun_mode_id, gun_mode> gun_modes = relevant->gun_all_modes();
    if( !gun_modes.empty() ) {
        menu.addentry( -1, false, 0, "  " + std::string( _( "Firing mode" ) ) );
        menu.entries.back().force_color = true;
        menu.entries.back().text_color = c_cyan;

        for( auto it = gun_modes.begin(); it != gun_modes.end(); ++it ) {
            const bool active_gun_mode = relevant->gun_get_mode_id() == it->first;

            // If gun mode is from a gunmod use gunmod's name, pay attention to the "->" on tname
            std::string text = ( it->second.target == relevant )
                               ? it->second.tname()
                               : it->second->tname() + " (" + std::to_string( it->second.qty ) + ")";

            text += ( active_gun_mode ? _( " (active)" ) : "" );

            menu.entries.emplace_back( firing_modes_range.first + static_cast<int>( std::distance(
                                           gun_modes.begin(), it ) ),
                                       true, MENU_AUTOASSIGN, text );
            if( active_gun_mode ) {
                menu.entries.back().text_color = c_light_green;
                if( menu.selected == 0 ) {
                    menu.selected = menu.entries.size() - 1;
                }
            }
        }
    }

    menu.query();
    bool refresh = false;
    if( menu.ret >= firing_modes_range.first && menu.ret < firing_modes_range.second ) {
        // gun mode select
        const std::map<gun_mode_id, gun_mode> all_gun_modes = relevant->gun_all_modes();
        int skip = menu.ret - firing_modes_range.first;
        for( const std::pair<const gun_mode_id, gun_mode> &it : all_gun_modes ) {
            if( skip-- == 0 ) {
                if( relevant->gun_current_mode().melee() ) {
                    refresh = true;
                }
                relevant->gun_set_mode( it.first );
                break;
            }
        }
    } else if( menu.ret >= aim_modes_range.first && menu.ret < aim_modes_range.second ) {
        // aiming mode select
        aim_mode = aim_types.begin();
        std::advance( aim_mode, menu.ret - aim_modes_range.first );
        you->preferred_aiming_mode = aim_mode->action;
    } // else - just refresh

    if( mode == TargetMode::TurretManual ) {
        itype_id ammo_current = turret->ammo_current();
        if( ammo_current.is_null() ) {
            ammo = nullptr;
            range = 0;
        } else {
            ammo = item::find_type( ammo_current );
            range = turret->range();
        }
    } else {
        if( relevant->gun_current_mode().melee() ) {
            refresh = true;
            range = relevant->current_reach_range( *you );
        } else {
            ammo = activity->reload_loc ? activity->reload_loc.get_item()->type :
                   relevant->gun_current_mode().target->ammo_data();
            if( activity->reload_loc ) {
                item temp_weapon = *relevant;
                temp_weapon.ammo_set( ammo->get_id() );
                range = temp_weapon.gun_current_mode().target->gun_range( you );
            } else {
                range = relevant->gun_current_mode().target->gun_range( you );
            }
        }
    }

    on_range_ammo_changed();
    return refresh;
}

bool target_ui::action_switch_ammo()
{
    if( mode == TargetMode::TurretManual ) {
        // For turrets that use vehicle tanks & can fire multiple liquids
        if( turret->ammo_options().size() > 1 ) {
            const auto opts = turret->ammo_options();
            auto iter = opts.find( turret->ammo_current() );
            turret->ammo_select( ++iter != opts.end() ? *iter : *opts.begin() );
            ammo = item::find_type( turret->ammo_current() );
            range = turret->range();
        }
    } else if( mode == TargetMode::Fire && relevant->has_flag( flag_RELOAD_AND_SHOOT ) ) {
        item_location gun = you->get_wielded_item();
        item::reload_option opt = you->select_ammo( gun );
        if( opt ) {
            activity->reload_loc = opt.ammo;
            update_ammo_range_from_gun_mode();
        }
    } else {
        // Leave aiming UI and open reloading UI since
        // reloading annihilates our aim anyway
        return false;
    }
    on_range_ammo_changed();
    return true;
}

bool target_ui::action_aim()
{
    set_last_target();
    apply_aim_turning_penalty();
    const double min_recoil = calculate_aim_cap( *you, dst );
    double hold_recoil = you->recoil;
    for( int i = 0; i < 10; ++i ) {
        do_aim( *you, *relevant, min_recoil );
    }
    add_msg_debug( debugmode::debug_filter::DF_BALLISTIC,
                   "you reduced recoil from %f to %f in 10 moves",
                   hold_recoil, you->recoil );
    // We've changed pc.recoil, update penalty
    recalc_aim_turning_penalty();

    return you->get_moves() > 0;
}

bool target_ui::action_drop_aim()
{
    you->recoil = MAX_RECOIL;

    // We've changed pc.recoil, update penalty
    recalc_aim_turning_penalty();

    return true;
}

bool target_ui::action_aim_and_shoot( const std::string &action )
{
    std::vector<aim_type>::iterator it;
    for( it = aim_types.begin(); it != aim_types.end(); it++ ) {
        if( action == it->action ) {
            break;
        }
    }
    if( it == aim_types.end() ) {
        debugmsg( "Could not find a valid aim_type for %s", action.c_str() );
        aim_mode = aim_types.begin();
    }
    int aim_threshold = it->threshold;
    set_last_target();
    apply_aim_turning_penalty();
    const double min_recoil = calculate_aim_cap( *you, dst );
    do {
        do_aim( *you, relevant ? *relevant : null_item_reference(), min_recoil );
    } while( you->get_moves() > 0 && you->recoil > aim_threshold &&
             you->recoil - sight_dispersion > min_recoil );

    // If we made it under the aim threshold, go ahead and fire.
    // Also fire if we're at our best aim level already.
    // If no critter is at dst then sight dispersion does not apply,
    // so it would lock into an infinite loop.
    bool done_aiming = you->recoil <= aim_threshold || you->recoil - sight_dispersion <= min_recoil ||
                       ( !get_creature_tracker().creature_at( dst ) && you->recoil <= min_recoil );
    return done_aiming;
}

void target_ui::draw_terrain_overlay()
{
    tripoint_bub_ms center = you->pos_bub() + you->view_offset;

    // Removes parts that don't belong to currently visible Z level
    const auto filter_this_z = [&center]( const std::vector<tripoint_bub_ms> &traj ) {
        std::vector<tripoint_bub_ms> this_z = traj;
        this_z.erase( std::remove_if( this_z.begin(), this_z.end(),
        [&center]( const tripoint_bub_ms & p ) {
            return p.z() != center.z();
        } ), this_z.end() );
        return this_z;
    };

    // FIXME: TILES version of g->draw_line helpfully draws a cursor at last point.
    //        This creates a fake cursor if 'dst' is on a z-level we cannot see.

    // Draw approximate line of fire for each turret in range
    if( mode == TargetMode::Turrets && draw_turret_lines ) {
        // TODO: TILES version doesn't know how to draw more than 1 line at a time.
        //       We merge all lines together and draw them as a big malformed one
        std::set<tripoint_bub_ms> points;
        for( const turret_with_lof &it : turrets_in_range ) {
            std::vector<tripoint_bub_ms> this_z = filter_this_z( it.line );
            for( const tripoint_bub_ms &p : this_z ) {
                points.insert( p );
            }
        }
        // Since "trajectory" for each turret is just a straight line,
        // we can draw it even if the player can't see some parts
        points.erase( dst ); // Workaround for fake cursor on TILES
        std::vector<tripoint_bub_ms> l( points.begin(), points.end() );
        if( dst.z() == center.z() ) {
            // Workaround for fake cursor bug on TILES
            l.push_back( dst );
        }
        g->draw_line( src, center, l, true );
    }

    // Draw trajectory
    if( mode != TargetMode::Turrets && dst != src ) {
        std::vector<tripoint_bub_ms> this_z = filter_this_z( traj );

        // Draw a highlighted trajectory only if we can see the endpoint.
        // Provides feedback to the player, but avoids leaking information
        // about tiles they can't see.
        g->draw_line( dst, center, this_z );
    }

    // Since draw_line does nothing if destination is not visible,
    // cursor also disappears. Draw it explicitly.
    if( dst.z() == center.z() ) {
        g->draw_cursor( dst );
    }

    // Draw spell AOE
    if( mode == TargetMode::Spell ) {
        drawsq_params params = drawsq_params().highlight( true ).center( center );
        for( const tripoint_bub_ms &tile : spell_aoe ) {
            if( tile.z() != center.z() ) {
                continue;
            }
#ifdef TILES
            if( use_tiles ) {
                g->draw_highlight( tile );
            } else {
#endif
                get_map().drawsq( g->w_terrain, tile, params );
#ifdef TILES
            }
#endif
        }
    }
}

void target_ui::draw_ui_window()
{
    // Clear target window and make it non-transparent.
    int width = getmaxx( w_target );
    int height = getmaxy( w_target );
    mvwrectf( w_target, point::zero, c_white, ' ', width, height );

    draw_border( w_target );
    draw_window_title();
    draw_help_notice();

    int text_y = 1; // Skip top border

    panel_cursor_info( text_y );
    text_y += compact ? 0 : 1;

    if( mode == TargetMode::Fire || mode == TargetMode::TurretManual ) {
        panel_gun_info( text_y );
        panel_recoil( text_y );
        text_y += compact ? 0 : 1;
    } else if( mode == TargetMode::Spell ) {
        panel_spell_info( text_y );
        text_y += compact ? 0 : 1;
    }

    bool fill_with_blank_if_no_target = !tiny;
    panel_target_info( text_y, fill_with_blank_if_no_target );
    text_y++;

    if( mode == TargetMode::Turrets ) {
        panel_turret_list( text_y );
    } else if( status == Status::Good ) {
        // TODO: these are old, consider refactoring
        if( mode == TargetMode::Fire ) {
            item_location load_loc = activity->reload_loc;
            text_y = print_aim( *this, *you, w_target, text_y, ctxt, *relevant->gun_current_mode(), dst,
                                load_loc );
        } else if( mode == TargetMode::Throw || mode == TargetMode::ThrowBlind ) {
            bool blind = mode == TargetMode::ThrowBlind;
            draw_throw_aim( *this, *you, w_target, text_y, ctxt, *relevant, dst, blind );
        }
    }

    // Narrow layout removes the list of controls. This allows us
    // to have small window size and not suffer from it.
    bool narrow = panel_manager::get_manager().get_current_layout().panels().begin()->get_width() <= 42;
    if( !narrow ) {
        draw_controls_list( text_y );
    }

    wnoutrefresh( w_target );
}

aim_type target_ui::get_selected_aim_type() const
{
    return this->aim_mode != this->aim_types.cend() ? *( this->aim_mode ) : get_default_aim_type();
}

int target_ui::get_sight_dispersion() const
{
    return sight_dispersion;
}

std::string target_ui::uitext_title() const
{
    switch( mode ) {
        case TargetMode::Fire:
        case TargetMode::TurretManual:
            return string_format( _( "Firing %s" ), relevant->tname() );
        case TargetMode::Throw:
            return string_format( _( "Throwing %s" ), relevant->tname() );
        case TargetMode::ThrowBlind:
            return string_format( _( "Blind throwing %s" ), relevant->tname() );
        default:
            return _( "Set target" );
    }
}

std::string target_ui::uitext_fire() const
{
    if( mode == TargetMode::Throw || mode == TargetMode::ThrowBlind ) {
        return to_translation( "[Hotkey] to throw", "to throw" ).translated();
    } else if( mode == TargetMode::Reach ) {
        return to_translation( "[Hotkey] to attack", "to attack" ).translated();
    } else if( mode == TargetMode::Spell ) {
        return to_translation( "[Hotkey] to cast the spell", "to cast" ).translated();
    } else {
        return to_translation( "[Hotkey] to fire", "to fire" ).translated();
    }
}

void target_ui::draw_window_title()
{
    mvwprintz( w_target, point( 2, 0 ), c_white, "< " );
    trim_and_print( w_target, point( 4, 0 ), getmaxx( w_target ) - 7, c_red, uitext_title() );
    wprintz( w_target, c_white, " >" );
}

void target_ui::draw_help_notice()
{
    int text_y = getmaxy( w_target ) - 1;
    int width = getmaxx( w_target );
    const std::string label_help = string_format(
                                       narrow ? _( "[%s] show help" ) : _( "[%s] show all controls" ),
                                       ctxt.get_desc( "HELP_KEYBINDINGS", 1 ) );
    int label_width = std::min( utf8_width( label_help ), width - 6 ); // 6 for borders and "< " + " >"
    int text_x = width - label_width - 6;
    mvwprintz( w_target, point( text_x + 1, text_y ), c_white, "< " );
    trim_and_print( w_target, point( text_x + 3, text_y ), label_width, c_white, label_help );
    wprintz( w_target, c_white, " >" );
}

void target_ui::draw_controls_list( int text_y )
{
    // Change UI colors for visual feedback
    // TODO: Colorize keys inside brackets to be consistent with other UI windows
    nc_color col_enabled = c_white;
    nc_color col_disabled = c_light_gray;
    nc_color col_move = ( status != Status::OutOfAmmo ? col_enabled : col_disabled );
    nc_color col_fire = ( status == Status::Good ? col_enabled : col_disabled );

    // Get first key bound to given action OR ' ' if there are none.
    const auto bound_key = [this]( const std::string & s ) {
        const std::vector<input_event> keys = this->ctxt.keys_bound_to( s, /*maximum_modifier_count=*/1 );
        return keys.empty() ? input_event() : keys.front();
    };
    const auto colored = [col_enabled]( nc_color color, const std::string & s ) {
        if( color == col_enabled ) {
            // col_enabled is the default one when printing
            return s;
        } else {
            return colorize( s, color );
        }
    };

    struct line {
        size_t order; // Lines with highest 'order' are removed first
        std::string str;
    };
    std::vector<line> lines;

    // Compile full list
    if( shifting_view ) {
        lines.push_back( {8, colored( col_move, _( "Shift view with directional keys" ) )} );
    } else {
        lines.push_back( {8, colored( col_move, _( "Move cursor with directional keys" ) )} );
    }
    if( is_mouse_enabled() ) {
        std::string move = _( "Mouse: LMB: Target, Wheel: Cycle," );
        std::string fire = _( "RMB: Fire" );
        lines.push_back( {7, colored( col_move, move ) + " " + colored( col_fire, fire )} );
    }
    {
        std::string cycle = string_format( _( "[%s] Cycle targets;" ), ctxt.get_desc( "NEXT_TARGET", 1 ) );
        std::string fire = string_format( _( "[%s] %s." ), bound_key( "FIRE" ).short_description(),
                                          uitext_fire() );
        lines.push_back( {0, colored( col_move, cycle ) + " " + colored( col_fire, fire )} );
    }
    {
        std::string text = string_format( _( "[%s] target self; [%s] toggle snap-to-target" ),
                                          bound_key( "CENTER" ).short_description(),
                                          bound_key( "TOGGLE_SNAP_TO_TARGET" ).short_description() );
        lines.push_back( {3, colored( col_enabled, text )} );
    }
    if( mode == TargetMode::Fire ) {
        std::string aim_and_fire;
        for( const aim_type &e : aim_types ) {
            if( e.has_threshold ) {
                aim_and_fire += string_format( "[%s] ", bound_key( e.action ).short_description() );
            }
        }
        aim_and_fire += _( "to aim and fire." );

        std::string aim = string_format( _( "[%s] to steady your aim.  (10 moves)" ),
                                         bound_key( "AIM" ).short_description() );

        std::string dropaim = string_format( _( "[%s] to stop aiming." ),
                                             bound_key( "STOPAIM" ).short_description() );

        lines.push_back( {2, colored( col_fire, aim )} );
        lines.push_back( { 2, colored( col_fire, dropaim ) } );
        lines.push_back( {4, colored( col_fire, aim_and_fire )} );
    }
    if( mode == TargetMode::Fire || mode == TargetMode::TurretManual || ( mode == TargetMode::Reach &&
            ( relevant && relevant->is_gun() ) && you->get_aim_types( *relevant ).size() > 1 ) ) {
        lines.push_back( {5, colored( col_enabled, string_format( _( "[%s] to switch firing modes." ),
                                      bound_key( "SWITCH_MODE" ).short_description() ) )
                         } );
        if( ( mode == TargetMode::Fire || mode == TargetMode::TurretManual ) &&
            ( !relevant->ammo_remaining( ) || !relevant->has_flag( flag_RELOAD_AND_SHOOT ) ) ) {
            lines.push_back( { 6, colored( col_enabled, string_format( _( "[%s] to reload/switch ammo." ),
                                           bound_key( "SWITCH_AMMO" ).short_description() ) )
                             } );
        } else {
            if( !unload_RAS_weapon ) {
                std::string unload = string_format( _( "[%s] Unload the %s after quitting." ),
                                                    bound_key( "TOGGLE_UNLOAD_RAS_WEAPON" ).short_description(),
                                                    relevant->tname() );
                lines.push_back( {3, colored( col_disabled, unload )} );
            } else {
                std::string unload = string_format( _( "[%s] Keep the %s loaded after quitting." ),
                                                    bound_key( "TOGGLE_UNLOAD_RAS_WEAPON" ).short_description(),
                                                    relevant->tname() );
                lines.push_back( {3, colored( col_enabled, unload )} );
            }
        }
    }
    if( mode == TargetMode::Turrets ) {
        const std::string label = draw_turret_lines
                                  ? _( "[%s] Hide lines of fire" )
                                  : _( "[%s] Show lines of fire" );
        lines.push_back( {1, colored( col_enabled, string_format( label, bound_key( "TOGGLE_TURRET_LINES" ).short_description() ) )} );
    }

    // Shrink the list until it fits
    int height = getmaxy( w_target );
    int available_lines = height - text_y - 1; // 1 for bottom border
    if( available_lines <= 0 ) {
        return;
    }
    while( lines.size() > static_cast<size_t>( available_lines ) ) {
        lines.erase( std::max_element( lines.begin(), lines.end(), []( const line & l1, const line & l2 ) {
            return l1.order < l2.order;
        } ) );
    }

    text_y = height - lines.size() - 1;
    for( const line &l : lines ) {
        nc_color col = col_enabled;
        print_colored_text( w_target, point( 1, text_y++ ), col, col, l.str );
    }
}

void target_ui::panel_cursor_info( int &text_y )
{
    std::string label_range;
    if( src == dst ) {
        label_range = string_format( _( "Range: %d" ), range );
    } else {
        label_range = string_format( _( "Range: %d/%d" ), dist_fn( dst ), range );
    }
    if( status == Status::OutOfRange && mode != TargetMode::Turrets ) {
        // Since each turret has its own range, highlighting cursor
        // range with red would be misleading
        label_range = colorize( label_range, c_red );
    }

    std::vector<std::string> labels;
    labels.push_back( label_range );
    if( fov_3d_z_range > 0 ) {
        labels.push_back( string_format( _( "Elevation: %d" ), dst.z() - src.z() ) );
    }
    labels.push_back( string_format( _( "Targets: %d" ), targets.size() ) );

    nc_color col = c_light_gray;
    int width = getmaxx( w_target );
    int text_x = 1;
    for( const std::string &s : labels ) {
        int x_left = width - text_x - 1;
        int len = utf8_width( s, true );
        if( len > x_left ) {
            text_x = 1;
            text_y++;
        }
        print_colored_text( w_target, point( text_x, text_y ), col, col, s );
        text_x += len + 1; // 1 for space
    }
    text_y++;
}

void target_ui::panel_gun_info( int &text_y )
{
    gun_mode m = relevant->gun_current_mode();
    std::string mode_name = m.tname();
    std::string gunmod_name;
    if( m.target != relevant ) {
        // Gun mode comes from a gunmod, not base gun. Add gunmod's name
        gunmod_name = m->tname() + " ";
    }
    std::string str = string_format( _( "Firing mode: <color_cyan>%s%s (%d)</color>" ),
                                     gunmod_name, mode_name, m.qty
                                   );
    nc_color clr = c_light_gray;
    print_colored_text( w_target, point( 1, text_y++ ), clr, clr, str );

    if( status == Status::OutOfAmmo ) {
        mvwprintz( w_target, point( 1, text_y++ ), c_red, _( "OUT OF AMMO" ) );
    } else if( ammo ) {
        bool is_favorite = relevant->is_ammo_container() && relevant->first_ammo().is_favorite;
        str = string_format( m->ammo_remaining( ) ? _( "Ammo: %s%s (%d/%d)" ) : _( "Ammo: %s%s" ),
                             colorize( ammo->nname( std::max( m->ammo_remaining( ), 1 ) ), ammo->color ),
                             colorize( is_favorite ? " *" : "", ammo->color ), m->ammo_remaining( ),
                             m->ammo_capacity( ammo->ammo->type ) );
        print_colored_text( w_target, point( 1, text_y++ ), clr, clr, str );
    } else {
        // Weapon doesn't use ammunition
        text_y++;
    }
}

void target_ui::panel_recoil( int &text_y )
{
    const int val = you->recoil_total();
    const int min_recoil = relevant->sight_dispersion( *you ).second;
    const int recoil_range = MAX_RECOIL - min_recoil;
    std::string str;
    if( val >= min_recoil + ( recoil_range * 2 / 3 ) ) {
        str = pgettext( "amount of backward momentum", "<color_red>High</color>" );
    } else if( val >= min_recoil + ( recoil_range / 2 ) ) {
        str = pgettext( "amount of backward momentum", "<color_yellow>Medium</color>" );
    } else if( val >= min_recoil + ( recoil_range / 4 ) ) {
        str = pgettext( "amount of backward momentum", "<color_light_green>Low</color>" );
    } else {
        str = pgettext( "amount of backward momentum", "<color_cyan>None</color>" );
    }
    str = string_format( _( "Recoil: %s" ), str );
    nc_color clr = c_light_gray;
    print_colored_text( w_target, point( 1, text_y++ ), clr, clr, str );
}

void target_ui::panel_spell_info( int &text_y )
{
    nc_color clr = c_light_gray;

    mvwprintz( w_target, point( 1, text_y++ ), c_light_green, _( "Casting: %s (Level %u)" ),
               casting->name(),
               casting->get_level() );
    if( !no_mana || casting->energy_source() == magic_energy_type::none ) {
        if( casting->energy_source() == magic_energy_type::hp ) {
            text_y += fold_and_print( w_target, point( 1, text_y ), getmaxx( w_target ) - 2,
                                      clr,
                                      _( "Cost: %s %s" ), casting->energy_cost_string( *you ), casting->energy_string() );
        } else {
            text_y += fold_and_print( w_target, point( 1, text_y ), getmaxx( w_target ) - 2,
                                      clr,
                                      _( "Cost: %s %s (Current: %s)" ), casting->energy_cost_string( *you ), casting->energy_string(),
                                      casting->energy_cur_string( *you ) );
        }
    }

    std::string fail_str;
    if( no_fail ) {
        fail_str = colorize( _( "0.0 % Failure Chance" ), c_light_green );
    } else {
        fail_str = casting->colorized_fail_percent( *you );
    }
    print_colored_text( w_target, point( 1, text_y++ ), clr, clr, fail_str );

    if( casting->aoe( get_player_character() ) > 0 ) {
        nc_color color = c_light_gray;
        const std::string fx = casting->effect();
        const std::string aoes = casting->aoe_string( get_player_character() );
        if( fx == "attack" || fx == "area_pull" || fx == "area_push" || fx == "ter_transform" ) {

            if( casting->shape() == spell_shape::cone ) {
                text_y += fold_and_print( w_target, point( 1, text_y ), getmaxx( w_target ) - 2, color,
                                          _( "Cone Arc: %s degrees" ), aoes );
            } else if( casting->shape() == spell_shape::line ) {
                text_y += fold_and_print( w_target, point( 1, text_y ), getmaxx( w_target ) - 2, color,
                                          _( "Line width: %s" ), aoes );
            } else {
                text_y += fold_and_print( w_target, point( 1, text_y ), getmaxx( w_target ) - 2, color,
                                          _( "Effective Spell Radius: %s%s" ), aoes, casting->in_aoe( src, dst,
                                                  get_player_character() ) ? colorize( _( " WARNING!  IN RANGE" ), c_red ) : "" );
            }
        }
    }

    mvwprintz( w_target, point( 1, text_y++ ), c_light_red, _( "Damage: %s" ),
               casting->damage_string( get_player_character() ) );

    text_y += fold_and_print( w_target, point( 1, text_y ), getmaxx( w_target ) - 2, clr,
                              casting->description() );
}

void target_ui::panel_target_info( int &text_y, bool fill_with_blank_if_no_target )
{
    const map &here = get_map();

    int max_lines = 6;
    if( dst_critter ) {
        if( you->sees( here, *dst_critter ) ) {
            // FIXME: print_info doesn't really care about line limit
            //        and can always occupy up to 4 of them (or even more?).
            //        To make things consistent, we ask it for 2 lines
            //        and somewhat reliably get 4.
            int fix_for_print_info = max_lines - 2;
            dst_critter->print_info( w_target, text_y, fix_for_print_info, 1 );
            text_y += max_lines;
        } else {
            std::vector<std::string> buf;
            static std::string raw_description;
            static std::string critter_name;
            const_dialogue d( get_const_talker_for( *you ), get_const_talker_for( *dst_critter ) );
            const enchant_cache::special_vision sees_with_special = you->enchantment_cache->get_vision( d );
            if( !sees_with_special.is_empty() ) {
                // handling against re-evaluation and snippet replacement on redraw
                if( critter_name != dst_critter->get_name() ) {
                    const enchant_cache::special_vision_descriptions special_vis_desc =
                        you->enchantment_cache->get_vision_description_struct( sees_with_special, d );
                    raw_description = special_vis_desc.description.translated();
                    parse_tags( raw_description, *you->as_character(), *dst_critter );
                    critter_name = dst_critter->get_name();
                }
                buf.emplace_back( raw_description );
            }
            for( size_t i = 0; i < static_cast<size_t>( max_lines ); i++, text_y++ ) {
                if( i >= buf.size() ) {
                    continue;
                }
                mvwprintw( w_target, point( 1, text_y ), buf[i] );
            }
        }
    } else if( dst_veh ) {
        if( you->sees( here, dst_veh->pos_bub( here ) ) ) {
            mvwprintz( w_target, point( 1, text_y ), c_light_gray, _( "Vehicle: " ) );
            mvwprintz( w_target, point( 1 + utf8_width( _( "Vehicle: " ) ), text_y ), c_white, "%s",
                       dst_veh->name );
            text_y ++;
        }
    } else if( fill_with_blank_if_no_target ) {
        // Fill with blank lines to prevent other panels from jumping around
        // when the cursor moves.
        text_y += max_lines;
        // TODO: print info about tile?
    }
}

void target_ui::panel_turret_list( int &text_y )
{
    mvwprintw( w_target, point( 1, text_y++ ), _( "Turrets in range: %d/%d" ), turrets_in_range.size(),
               vturrets->size() );

    for( const turret_with_lof &it : turrets_in_range ) {
        std::string str = string_format( "* %s", it.turret->name() );
        nc_color clr = c_white;
        print_colored_text( w_target, point( 1, text_y++ ), clr, clr, str );
    }
}

void target_ui::on_target_accepted( bool harmful ) const
{
    // TODO: all of this should be moved into on-hit code
    const auto lt_ptr = you->last_target.lock();
    if( npc *const guy = dynamic_cast<npc *>( lt_ptr.get() ) ) {
        if( harmful ) {
            if( !guy->guaranteed_hostile() ) {
                // TODO: get rid of this. Or combine it with effect_hit_by_player
                guy->hit_by_player = true; // used for morale penalty
            }
        }
    } else if( monster *const mon = dynamic_cast<monster *>( lt_ptr.get() ) ) {
        mon->add_effect( effect_hit_by_player, 10_minutes );
    }
}

bool gunmode_checks_common( avatar &you, const map &m, std::vector<std::string> &messages,
                            const gun_mode &gmode )
{
    bool result = true;
    if( you.has_trait( trait_BRAWLER ) ) {
        messages.push_back( string_format( _( "Pfft.  You are a brawler; using this %s is beneath you." ),
                                           gmode->tname() ) );
        result = false;
    }

    if( you.has_trait( trait_GUNSHY ) && gmode->is_firearm() ) {
        messages.push_back( string_format( _( "You're too gun-shy to use this." ) ) );
        result = false;
    }

    // Check that passed gun mode is valid and we are able to use it
    if( !( gmode && you.can_use( *gmode ) ) ) {
        messages.push_back( string_format( _( "You can't currently fire your %s." ),
                                           gmode->tname() ) );
        result = false;
    }

    const optional_vpart_position vp = m.veh_at( you.pos_bub() );
    if( vp && vp->vehicle().player_in_control( m, you ) && ( gmode->is_two_handed( you ) ||
            gmode->has_flag( flag_FIRE_TWOHAND ) ) ) {
        messages.push_back( string_format( _( "You can't fire your %s while driving." ),
                                           gmode->tname() ) );
        result = false;
    }

    if( gmode->has_flag( flag_FIRE_TWOHAND ) && ( !you.has_two_arms_lifting() ||
            you.worn_with_flag( flag_RESTRICT_HANDS ) ) ) {
        messages.push_back( string_format( _( "You need two free hands to fire your %s." ),
                                           gmode->tname() ) );
        result = false;
    }

    return result;
}

bool gunmode_checks_weapon( avatar &you, const map &m, std::vector<std::string> &messages,
                            const gun_mode &gmode )
{
    bool result = true;
    if( !gmode->ammo_sufficient( &you ) &&
        !gmode->has_flag( flag_RELOAD_AND_SHOOT ) ) {
        if( !gmode->ammo_remaining( ) ) {
            messages.push_back( string_format( _( "Your %s is empty!" ), gmode->tname() ) );
        } else {
            messages.push_back( string_format( _( "Your %s needs %i charges to fire!" ),
                                               gmode->tname(), gmode->ammo_required() ) );
        }
        result = false;
    }

    if( gmode->get_gun_energy_drain() > 0_kJ ) {
        const units::energy energy_drain = gmode->get_gun_energy_drain();
        bool is_mech_weapon = false;
        if( you.is_mounted() ) {
            monster *mons = get_player_character().mounted_creature.get();
            if( !mons->type->mech_weapon.is_empty() ) {
                is_mech_weapon = true;
            }
        }

        if( gmode->energy_remaining( &you ) < energy_drain ) {
            result = false;
            if( is_mech_weapon ) {
                messages.push_back( string_format( _( "Your mech has an empty battery, its %s will not fire." ),
                                                   gmode->tname() ) );
            } else if( gmode->has_flag( flag_USES_BIONIC_POWER ) ) {
                messages.push_back( string_format(
                                        _( "You need at least %2$d kJ of bionic power to fire the %1$s!" ),
                                        gmode->tname(), units::to_kilojoule( energy_drain ) ) );
            } else if( gmode->has_flag( flag_USE_UPS ) ) {
                messages.push_back( string_format(
                                        _( "You need a UPS with at least %2$d kJ to fire the %1$s!" ),
                                        gmode->tname(), units::to_kilojoule( energy_drain ) ) );
            }
        }
    }

    if( gmode->has_flag( flag_MOUNTED_GUN ) ) {
        const bool v_mountable = static_cast<bool>( m.veh_at(
                                     you.pos_bub() ).part_with_feature( "MOUNTABLE",
                                             true ) );
        bool t_mountable = m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_MOUNTABLE, you.pos_bub() );
        if( !t_mountable && !v_mountable ) {
            messages.push_back( string_format(
                                    _( "You must stand near acceptable terrain or furniture to fire the %s.  A table, a mound of dirt, a broken window, etc." ),
                                    gmode->tname() ) );
            result = false;
        }
    }

    return result;
}
