#include "activity_handlers.h"

#include <climits>
#include <cstddef>
#include <algorithm>
#include <cmath>
#include <queue>
#include <array>
#include <iterator>
#include <memory>

#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include "action.h"
#include "advanced_inv.h"
#include "avatar.h"
#include "avatar_action.h"
#include "clzones.h"
#include "construction.h"
#include "coordinate_conversions.h"
#include "craft_command.h"
#include "debug.h"
#include "event_bus.h"
#include "fault.h"
#include "field.h"
#include "game.h"
#include "game_inventory.h"
#include "gates.h"
#include "handle_liquid.h"
#include "harvest.h"
#include "iexamine.h"
#include "itype.h"
#include "iuse_actor.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "messages.h"
#include "mongroup.h"
#include "morale_types.h"
#include "mtype.h"
#include "npc.h"
#include "npctalk.h"
#include "output.h"
#include "overmapbuffer.h"
#include "player.h"
#include "ranged.h"
#include "recipe.h"
#include "requirements.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui.h"
#include "veh_interact.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "map_selector.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "creature.h"
#include "damage.h"
#include "enums.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "item_stack.h"
#include "iuse.h"
#include "line.h"
#include "monster.h"
#include "omdata.h"
#include "optional.h"
#include "pimpl.h"
#include "pldata.h"
#include "ret_val.h"
#include "string_id.h"
#include "units.h"
#include "type_id.h"
#include "timed_event.h"
#include "options.h"
#include "colony.h"
#include "color.h"
#include "flat_set.h"
#include "game_constants.h"
#include "point.h"
#include "weather.h"

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

const skill_id skill_survival( "survival" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_electronics( "electronics" );

const species_id HUMAN( "HUMAN" );
const species_id ZOMBIE( "ZOMBIE" );

const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_milked( "milked" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_under_op( "under_operation" );
const efftype_id effect_pet( "pet" );
const efftype_id effect_controlled( "controlled" );
const efftype_id effect_saddled( "saddled" );

using namespace activity_handlers;

const std::map< activity_id, std::function<void( player_activity *, player * )> >
activity_handlers::do_turn_functions = {
    { activity_id( "ACT_BURROW" ), burrow_do_turn },
    { activity_id( "ACT_CRAFT" ), craft_do_turn },
    { activity_id( "ACT_FILL_LIQUID" ), fill_liquid_do_turn },
    { activity_id( "ACT_PICKAXE" ), pickaxe_do_turn },
    { activity_id( "ACT_DROP" ), drop_do_turn },
    { activity_id( "ACT_STASH" ), stash_do_turn },
    { activity_id( "ACT_PULP" ), pulp_do_turn },
    { activity_id( "ACT_GAME" ), game_do_turn },
    { activity_id( "ACT_START_FIRE" ), start_fire_do_turn },
    { activity_id( "ACT_VIBE" ), vibe_do_turn },
    { activity_id( "ACT_HAND_CRANK" ), hand_crank_do_turn },
    { activity_id( "ACT_OXYTORCH" ), oxytorch_do_turn },
    { activity_id( "ACT_AIM" ), aim_do_turn },
    { activity_id( "ACT_PICKUP" ), pickup_do_turn },
    { activity_id( "ACT_WEAR" ), wear_do_turn },
    { activity_id( "ACT_MULTIPLE_FISH" ), multiple_fish_do_turn },
    { activity_id( "ACT_MULTIPLE_CONSTRUCTION" ), multiple_construction_do_turn },
    { activity_id( "ACT_MULTIPLE_BUTCHER" ), multiple_butcher_do_turn },
    { activity_id( "ACT_MULTIPLE_FARM" ), multiple_farm_do_turn },
    { activity_id( "ACT_FETCH_REQUIRED" ), fetch_do_turn },
    { activity_id( "ACT_BUILD" ), build_do_turn },
    { activity_id( "ACT_EAT_MENU" ), eat_menu_do_turn },
    { activity_id( "ACT_VEHICLE_DECONSTRUCTION" ), vehicle_deconstruction_do_turn },
    { activity_id( "ACT_VEHICLE_REPAIR" ), vehicle_repair_do_turn },
    { activity_id( "ACT_MULTIPLE_CHOP_TREES" ), chop_trees_do_turn },
    { activity_id( "ACT_CONSUME_FOOD_MENU" ), consume_food_menu_do_turn },
    { activity_id( "ACT_CONSUME_DRINK_MENU" ), consume_drink_menu_do_turn },
    { activity_id( "ACT_CONSUME_MEDS_MENU" ), consume_meds_menu_do_turn },
    { activity_id( "ACT_MOVE_ITEMS" ), move_items_do_turn },
    { activity_id( "ACT_MOVE_LOOT" ), move_loot_do_turn },
    { activity_id( "ACT_ADV_INVENTORY" ), adv_inventory_do_turn },
    { activity_id( "ACT_ARMOR_LAYERS" ), armor_layers_do_turn },
    { activity_id( "ACT_ATM" ), atm_do_turn },
    { activity_id( "ACT_CRACKING" ), cracking_do_turn },
    { activity_id( "ACT_FISH" ), fish_do_turn },
    { activity_id( "ACT_REPAIR_ITEM" ), repair_item_do_turn },
    { activity_id( "ACT_BUTCHER" ), butcher_do_turn },
    { activity_id( "ACT_BUTCHER_FULL" ), butcher_do_turn },
    { activity_id( "ACT_TRAVELLING" ), travel_do_turn },
    { activity_id( "ACT_AUTODRIVE" ), drive_do_turn },
    { activity_id( "ACT_CHURN" ), churn_do_turn },
    { activity_id( "ACT_FIELD_DRESS" ), butcher_do_turn },
    { activity_id( "ACT_SKIN" ), butcher_do_turn },
    { activity_id( "ACT_QUARTER" ), butcher_do_turn },
    { activity_id( "ACT_DISMEMBER" ), butcher_do_turn },
    { activity_id( "ACT_DISSECT" ), butcher_do_turn },
    { activity_id( "ACT_HACKSAW" ), hacksaw_do_turn },
    { activity_id( "ACT_CHOP_TREE" ), chop_tree_do_turn },
    { activity_id( "ACT_CHOP_LOGS" ), chop_tree_do_turn },
    { activity_id( "ACT_TIDY_UP" ), tidy_up_do_turn },
    { activity_id( "ACT_CHOP_PLANKS" ), chop_tree_do_turn },
    { activity_id( "ACT_TIDY_UP" ), tidy_up_do_turn },
    { activity_id( "ACT_JACKHAMMER" ), jackhammer_do_turn },
    { activity_id( "ACT_FIND_MOUNT" ), find_mount_do_turn },
    { activity_id( "ACT_DIG" ), dig_do_turn },
    { activity_id( "ACT_DIG_CHANNEL" ), dig_channel_do_turn },
    { activity_id( "ACT_FILL_PIT" ), fill_pit_do_turn },
    { activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ), multiple_chop_planks_do_turn },
    { activity_id( "ACT_TILL_PLOT" ), till_plot_do_turn },
    { activity_id( "ACT_HARVEST_PLOT" ), harvest_plot_do_turn },
    { activity_id( "ACT_PLANT_PLOT" ), plant_plot_do_turn },
    { activity_id( "ACT_FERTILIZE_PLOT" ), fertilize_plot_do_turn },
    { activity_id( "ACT_TRY_SLEEP" ), try_sleep_do_turn },
    { activity_id( "ACT_OPERATION" ), operation_do_turn },
    { activity_id( "ACT_ROBOT_CONTROL" ), robot_control_do_turn },
    { activity_id( "ACT_TREE_COMMUNION" ), tree_communion_do_turn },
    { activity_id( "ACT_STUDY_SPELL" ), study_spell_do_turn},
    { activity_id( "ACT_READ" ), read_do_turn},
    { activity_id( "ACT_WAIT_STAMINA" ), wait_stamina_do_turn }
};

const std::map< activity_id, std::function<void( player_activity *, player * )> >
activity_handlers::finish_functions = {
    { activity_id( "ACT_BURROW" ), burrow_finish },
    { activity_id( "ACT_BUTCHER" ), butcher_finish },
    { activity_id( "ACT_BUTCHER_FULL" ), butcher_finish },
    { activity_id( "ACT_FIELD_DRESS" ), butcher_finish },
    { activity_id( "ACT_SKIN" ), butcher_finish },
    { activity_id( "ACT_QUARTER" ), butcher_finish },
    { activity_id( "ACT_DISMEMBER" ), butcher_finish },
    { activity_id( "ACT_DISSECT" ), butcher_finish },
    { activity_id( "ACT_FIRSTAID" ), firstaid_finish },
    { activity_id( "ACT_FISH" ), fish_finish },
    { activity_id( "ACT_FORAGE" ), forage_finish },
    { activity_id( "ACT_HOTWIRE_CAR" ), hotwire_finish },
    { activity_id( "ACT_LONGSALVAGE" ), longsalvage_finish },
    { activity_id( "ACT_MAKE_ZLAVE" ), make_zlave_finish },
    { activity_id( "ACT_PICKAXE" ), pickaxe_finish },
    { activity_id( "ACT_RELOAD" ), reload_finish },
    { activity_id( "ACT_START_FIRE" ), start_fire_finish },
    { activity_id( "ACT_TRAIN" ), train_finish },
    { activity_id( "ACT_CHURN" ), churn_finish },
    { activity_id( "ACT_VEHICLE" ), vehicle_finish },
    { activity_id( "ACT_START_ENGINES" ), start_engines_finish },
    { activity_id( "ACT_OXYTORCH" ), oxytorch_finish },
    { activity_id( "ACT_PULP" ), pulp_finish },
    { activity_id( "ACT_CRACKING" ), cracking_finish },
    { activity_id( "ACT_OPEN_GATE" ), open_gate_finish },
    { activity_id( "ACT_REPAIR_ITEM" ), repair_item_finish },
    { activity_id( "ACT_MEND_ITEM" ), mend_item_finish },
    { activity_id( "ACT_GUNMOD_ADD" ), gunmod_add_finish },
    { activity_id( "ACT_TOOLMOD_ADD" ), toolmod_add_finish },
    { activity_id( "ACT_CLEAR_RUBBLE" ), clear_rubble_finish },
    { activity_id( "ACT_MEDITATE" ), meditate_finish },
    { activity_id( "ACT_READ" ), read_finish },
    { activity_id( "ACT_WAIT" ), wait_finish },
    { activity_id( "ACT_WAIT_WEATHER" ), wait_weather_finish },
    { activity_id( "ACT_WAIT_NPC" ), wait_npc_finish },
    { activity_id( "ACT_WAIT_STAMINA" ), wait_stamina_finish },
    { activity_id( "ACT_SOCIALIZE" ), socialize_finish },
    { activity_id( "ACT_TRY_SLEEP" ), try_sleep_finish },
    { activity_id( "ACT_OPERATION" ), operation_finish },
    { activity_id( "ACT_DISASSEMBLE" ), disassemble_finish },
    { activity_id( "ACT_VIBE" ), vibe_finish },
    { activity_id( "ACT_ATM" ), atm_finish },
    { activity_id( "ACT_AIM" ), aim_finish },
    { activity_id( "ACT_EAT_MENU" ), eat_menu_finish },
    { activity_id( "ACT_CONSUME_FOOD_MENU" ), eat_menu_finish },
    { activity_id( "ACT_CONSUME_DRINK_MENU" ), eat_menu_finish },
    { activity_id( "ACT_CONSUME_MEDS_MENU" ), eat_menu_finish },
    { activity_id( "ACT_WASH" ), washing_finish },
    { activity_id( "ACT_HACKSAW" ), hacksaw_finish },
    { activity_id( "ACT_CHOP_TREE" ), chop_tree_finish },
    { activity_id( "ACT_CHOP_LOGS" ), chop_logs_finish },
    { activity_id( "ACT_CHOP_PLANKS" ), chop_planks_finish },
    { activity_id( "ACT_JACKHAMMER" ), jackhammer_finish },
    { activity_id( "ACT_DIG" ), dig_finish },
    { activity_id( "ACT_DIG_CHANNEL" ), dig_channel_finish },
    { activity_id( "ACT_FILL_PIT" ), fill_pit_finish },
    { activity_id( "ACT_PLAY_WITH_PET" ), play_with_pet_finish },
    { activity_id( "ACT_SHAVE" ), shaving_finish },
    { activity_id( "ACT_HAIRCUT" ), haircut_finish },
    { activity_id( "ACT_UNLOAD_MAG" ), unload_mag_finish },
    { activity_id( "ACT_ROBOT_CONTROL" ), robot_control_finish },
    { activity_id( "ACT_MIND_SPLICER" ), mind_splicer_finish },
    { activity_id( "ACT_HACK_DOOR" ), hack_door_finish },
    { activity_id( "ACT_HACK_SAFE" ), hack_safe_finish },
    { activity_id( "ACT_SPELLCASTING" ), spellcasting_finish },
    { activity_id( "ACT_STUDY_SPELL" ), study_spell_finish }
};

bool activity_handlers::resume_for_multi_activities( player &p )
{
    if( !p.backlog.empty() ) {
        player_activity &back_act = p.backlog.front();
        if( back_act.is_multi_type() ) {
            p.assign_activity( p.backlog.front().id() );
            p.backlog.clear();
            return true;
        }
    }
    return false;
}

void activity_handlers::burrow_do_turn( player_activity *act, player * )
{
    sfx::play_activity_sound( "activity", "burrow", sfx::get_heard_volume( act->placement ) );
    if( calendar::once_every( 1_minutes ) ) {
        sounds::sound( act->placement, 10, sounds::sound_t::movement,
                       //~ Sound of a Rat mutant burrowing!
                       _( "ScratchCrunchScrabbleScurry." ) );
    }
}

void activity_handlers::burrow_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    if( g->m.is_bashable( pos ) && g->m.has_flag( "SUPPORTS_ROOF", pos ) &&
        g->m.ter( pos ) != t_tree ) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Not quite as bad as the pickaxe, though
        p->mod_stored_nutr( 10 );
        p->mod_thirst( 10 );
        p->mod_fatigue( 15 );
        p->mod_pain( 3 * rng( 1, 3 ) );
    } else if( g->m.move_cost( pos ) == 2 && g->get_levz() == 0 &&
               g->m.ter( pos ) != t_dirt && g->m.ter( pos ) != t_grass ) {
        //Breaking up concrete on the surface? not nearly as bad
        p->mod_stored_nutr( 5 );
        p->mod_thirst( 5 );
        p->mod_fatigue( 10 );
    }
    p->add_msg_if_player( m_good, _( "You finish burrowing." ) );
    g->m.destroy( pos, true );

    act->set_to_null();
}

static bool check_butcher_cbm( const int roll )
{
    // Failure rates for dissection rolls
    // 90% at roll 0, 72% at roll 1, 60% at roll 2, 51% @ 3, 45% @ 4, 40% @ 5, ... , 25% @ 10
    // Roll is roughly a rng(0, -3 + 1st_aid + fine_cut_quality + 1/2 electronics + small_dex_bonus)
    // Roll is reduced by corpse damage level, but to no less then 0
    add_msg( m_debug, _( "Roll = %i" ), roll );
    add_msg( m_debug, _( "Failure chance = %f%%" ), ( 9.0f / ( 10.0f + roll * 2.5f ) ) * 100.0f );
    const bool failed = x_in_y( 9, ( 10 + roll * 2.5 ) );
    return !failed;
}

static void butcher_cbm_item( const std::string &what, const tripoint &pos,
                              const time_point &age, const int roll, const std::vector<std::string> &flags,
                              const std::vector<fault_id> &faults )
{
    if( roll < 0 ) {
        return;
    }
    if( item::find_type( itype_id( what ) )->bionic.has_value() ) {
        item cbm( check_butcher_cbm( roll ) ? what : "burnt_out_bionic", age );
        for( const std::string &flg : flags ) {
            cbm.set_flag( flg );
        }
        for( const fault_id &flt : faults ) {
            cbm.faults.emplace( flt );
        }
        add_msg( m_good, _( "You discover a %s!" ), cbm.tname() );
        g->m.add_item( pos, cbm );
    } else if( check_butcher_cbm( roll ) ) {
        item something( what, age );
        for( const std::string &flg : flags ) {
            something.set_flag( flg );
        }
        for( const fault_id &flt : faults ) {
            something.faults.emplace( flt );
        }
        add_msg( m_good, _( "You discover a %s!" ), something.tname() );
        g->m.add_item( pos, something );
    } else {
        add_msg( m_bad, _( "You discover only damaged organs." ) );
    }
}

static void butcher_cbm_group( const std::string &group, const tripoint &pos,
                               const time_point &age, const int roll, const std::vector<std::string> flags,
                               const std::vector<fault_id> faults )
{
    if( roll < 0 ) {
        return;
    }

    //To see if it spawns a random additional CBM
    if( check_butcher_cbm( roll ) ) {
        //The CBM works
        const auto spawned = g->m.put_items_from_loc( group, pos, age );
        for( item *it : spawned ) {
            for( const std::string &flg : flags ) {
                it->set_flag( flg );
            }
            for( const fault_id &flt : faults ) {
                it->faults.emplace( flt );
            }
            add_msg( m_good, _( "You discover a %s!" ), it->tname() );
        }
    } else {
        //There is a burnt out CBM
        item cbm( "burnt_out_bionic", age );
        for( const std::string &flg : flags ) {
            cbm.set_flag( flg );
        }
        for( const fault_id &flt : faults ) {
            cbm.faults.emplace( flt );
        }
        add_msg( m_good, _( "You discover a %s!" ), cbm.tname() );
        g->m.add_item( pos, cbm );
    }
}

static void set_up_butchery( player_activity &act, player &u, butcher_type action )
{
    const int factor = u.max_quality( action == DISSECT ? quality_id( "CUT_FINE" ) :
                                      quality_id( "BUTCHER" ) );

    const item &corpse_item = *act.targets.back();
    const mtype &corpse = *corpse_item.get_mtype();

    if( action != DISSECT ) {
        if( factor == INT_MIN ) {
            u.add_msg_if_player( m_info,
                                 _( "None of your cutting tools are suitable for butchering." ) );
            act.set_to_null();
            return;
        } else if( factor < 0 && one_in( 3 ) ) {
            u.add_msg_if_player( m_bad,
                                 _( "You don't trust the quality of your tools, but carry on anyway." ) );
        }
    }

    if( action == DISSECT ) {
        switch( factor ) {
            case INT_MIN:
                u.add_msg_if_player( m_info, _( "None of your tools are sharp and precise enough to do that." ) );
                act.set_to_null();
                return;
            case 1:
                u.add_msg_if_player( m_info, _( "You could use a better tool, but this will do." ) );
                break;
            case 2:
                u.add_msg_if_player( m_info, _( "This tool is great, but you still would like a scalpel." ) );
                break;
            case 3:
                u.add_msg_if_player( m_info, _( "You dissect the corpse with a trusty scalpel." ) );
                break;
            case 5:
                u.add_msg_if_player( m_info,
                                     _( "You dissect the corpse with a sophisticated system of surgical grade scalpels." ) );
                break;
        }
    }

    bool has_table_nearby = false;
    for( const tripoint &pt : g->m.points_in_radius( u.pos(), 2 ) ) {
        if( g->m.has_flag_furn( "FLAT_SURF", pt ) || g->m.has_flag( "FLAT_SURF", pt ) ||
            ( ( g->m.veh_at( pt ) && ( g->m.veh_at( pt )->vehicle().has_part( "KITCHEN" ) ||
                                       g->m.veh_at( pt )->vehicle().has_part( "FLAT_SURF" ) ) ) ) ) {
            has_table_nearby = true;
        }
    }
    bool has_tree_nearby = false;
    for( const tripoint &pt : g->m.points_in_radius( u.pos(), 2 ) ) {
        if( g->m.has_flag( "TREE", pt ) ) {
            has_tree_nearby = true;
        }
    }
    bool b_rack_present = false;
    for( const tripoint &pt : g->m.points_in_radius( u.pos(), 2 ) ) {
        if( g->m.has_flag_furn( "BUTCHER_EQ", pt ) ) {
            b_rack_present = true;
        }
    }
    // workshop butchery (full) prequisites
    if( action == BUTCHER_FULL ) {
        const bool has_rope = u.has_amount( "rope_30", 1 ) || u.has_amount( "rope_makeshift_30", 1 ) ||
                              u.has_amount( "vine_30", 1 ) || u.has_amount( "grapnel", 1 );
        const bool big_corpse = corpse.size >= MS_MEDIUM;

        if( big_corpse ) {
            if( has_rope && !has_tree_nearby && !b_rack_present ) {
                u.add_msg_if_player( m_info,
                                     _( "You need to suspend this corpse to butcher it. While you have a rope to lift the corpse, there is no tree nearby to hang it from." ) );
                act.targets.pop_back();
                return;
            }
            if( !has_rope && !b_rack_present ) {
                u.add_msg_if_player( m_info,
                                     _( "To perform a full butchery on a corpse this big, you need either a butchering rack, a nearby hanging meathook, or both a long rope in your inventory and a nearby tree to hang the corpse from." ) );
                act.targets.pop_back();
                return;
            }
            if( !has_table_nearby ) {
                u.add_msg_if_player( m_info,
                                     _( "To perform a full butchery on a corpse this big, you need a table nearby or something else with a flat surface. A leather tarp spread out on the ground could suffice." ) );
                act.targets.pop_back();
                return;
            }
            if( !( u.has_quality( quality_id( "SAW_W" ) ) || u.has_quality( quality_id( "SAW_M" ) ) ) ) {
                u.add_msg_if_player( m_info,
                                     _( "For a corpse this big you need a saw to perform a full butchery." ) );
                act.targets.pop_back();
                return;
            }
        }
    }

    if( action == DISSECT && ( corpse_item.has_flag( "QUARTERED" ) ||
                               corpse_item.has_flag( "FIELD_DRESS_FAILED" ) ) ) {
        u.add_msg_if_player( m_info,
                             _( "It would be futile to search for implants inside this badly damaged corpse." ) );
        act.targets.pop_back();
        return;
    }

    if( action == F_DRESS && ( corpse_item.has_flag( "FIELD_DRESS" ) ||
                               corpse_item.has_flag( "FIELD_DRESS_FAILED" ) ) ) {
        u.add_msg_if_player( m_info, _( "This corpse is already field dressed." ) );
        act.targets.pop_back();
        return;
    }

    if( action == SKIN && corpse_item.has_flag( "SKINNED" ) ) {
        u.add_msg_if_player( m_info, _( "This corpse is already skinned." ) );
        act.targets.pop_back();
        return;
    }

    if( action == QUARTER ) {
        if( corpse.size == MS_TINY ) {
            u.add_msg_if_player( m_bad, _( "This corpse is too small to quarter without damaging." ),
                                 corpse.nname() );
            act.targets.pop_back();
            return;
        }
        if( corpse_item.has_flag( "QUARTERED" ) ) {
            u.add_msg_if_player( m_bad, _( "This is already quartered." ), corpse.nname() );
            act.targets.pop_back();
            return;
        }
        if( !( corpse_item.has_flag( "FIELD_DRESS" ) || corpse_item.has_flag( "FIELD_DRESS_FAILED" ) ) ) {
            u.add_msg_if_player( m_bad, _( "You need to perform field dressing before quartering." ),
                                 corpse.nname() );
            act.targets.pop_back();
            return;
        }
    }

    // applies to all butchery actions
    const bool is_human = corpse.id == mtype_id::NULL_ID() || ( corpse.in_species( HUMAN ) &&
                          !corpse.in_species( ZOMBIE ) );
    if( is_human && !( u.has_trait_flag( "CANNIBAL" ) || u.has_trait_flag( "PSYCHOPATH" ) ||
                       u.has_trait_flag( "SAPIOVORE" ) ) ) {

        if( u.is_player() ) {
            if( query_yn( _( "Would you dare desecrate the mortal remains of a fellow human being?" ) ) ) {
                switch( rng( 1, 3 ) ) {
                    case 1:
                        u.add_msg_if_player( m_bad, _( "You clench your teeth at the prospect of this gruesome job." ) );
                        break;
                    case 2:
                        u.add_msg_if_player( m_bad, _( "This will haunt you in your dreams." ) );
                        break;
                    case 3:
                        u.add_msg_if_player( m_bad,
                                             _( "You try to look away, but this gruesome image will stay on your mind for some time." ) );
                        break;
                }
                g->u.add_morale( MORALE_BUTCHER, -50, 0, 2_days, 3_hours );
            } else {
                u.add_msg_if_player( m_good, _( "It needs a coffin, not a knife." ) );
                act.targets.pop_back();
                return;
            }
        } else {
            u.add_morale( MORALE_BUTCHER, -50, 0, 2_days, 3_hours );
        }
    }

    act.moves_left = butcher_time_to_cut( u, corpse_item, action );

    // We have a valid target, so preform the full finish function
    // instead of just selecting the next valid target
    act.index = false;
}

int butcher_time_to_cut( const player &u, const item &corpse_item, const butcher_type action )
{
    const mtype &corpse = *corpse_item.get_mtype();
    const int factor = u.max_quality( action == DISSECT ? quality_id( "CUT_FINE" ) :
                                      quality_id( "BUTCHER" ) );

    int time_to_cut = 0;
    switch( corpse.size ) {
        // Time (roughly) in turns to cut up the corpse
        case MS_TINY:
            time_to_cut = 150;
            break;
        case MS_SMALL:
            time_to_cut = 300;
            break;
        case MS_MEDIUM:
            time_to_cut = 450;
            break;
        case MS_LARGE:
            time_to_cut = 600;
            break;
        case MS_HUGE:
            time_to_cut = 1800;
            break;
    }

    // At factor 0, base 100 time_to_cut remains 100. At factor 50, it's 50 , at factor 75 it's 25
    time_to_cut *= std::max( 25, 100 - factor );
    if( time_to_cut < 3000 ) {
        time_to_cut = 3000;
    }

    switch( action ) {
        case BUTCHER:
            break;
        case BUTCHER_FULL:
            if( !corpse_item.has_flag( "FIELD_DRESS" ) || corpse_item.has_flag( "FIELD_DRESS_FAILED" ) ) {
                time_to_cut *= 6;
            } else {
                time_to_cut *= 4;
            }
            break;
        case F_DRESS:
        case SKIN:
            time_to_cut *= 2;
            break;
        case QUARTER:
            time_to_cut /= 4;
            if( time_to_cut < 1200 ) {
                time_to_cut = 1200;
            }
            break;
        case DISMEMBER:
            time_to_cut /= 10;
            if( time_to_cut < 600 ) {
                time_to_cut = 600;
            }
            break;
        case DISSECT:
            time_to_cut *= 6;
            break;
    }

    if( corpse_item.has_flag( "QUARTERED" ) ) {
        time_to_cut /= 4;
    }
    time_to_cut = time_to_cut * ( 1 - ( g->u.get_num_crafting_helpers( 3 ) / 10 ) );
    return time_to_cut;
}

// this function modifies the input weight by its damage level, depending on the bodypart
static int corpse_damage_effect( int weight, const std::string &entry_type, int damage_level )
{
    const float slight_damage = 0.9;
    const float damage = 0.75;
    const float high_damage = 0.5;
    const int destroyed = 0;

    switch( damage_level ) {
        case 2: // "damaged"
            if( entry_type == "offal" ) {
                return round( weight * damage );
            }
            if( entry_type == "skin" ) {
                return round( weight * damage );
            }
            if( entry_type == "flesh" ) {
                return round( weight * slight_damage );
            }
            break;
        case 3: // "mangled"
            if( entry_type == "offal" ) {
                return destroyed;
            }
            if( entry_type == "skin" ) {
                return round( weight * high_damage );
            }
            if( entry_type == "bone" ) {
                return round( weight * slight_damage );
            }
            if( entry_type == "flesh" ) {
                return round( weight * damage );
            }
            break;
        case 4: // "pulped"
            if( entry_type == "offal" ) {
                return destroyed;
            }
            if( entry_type == "skin" ) {
                return destroyed;
            }
            if( entry_type == "bone" ) {
                return round( weight * damage );
            }
            if( entry_type == "flesh" ) {
                return round( weight * high_damage );
            }
            break;
        default: // "bruised" modifier is almost impossible to avoid; also includes no modifier (zero damage)
            break;
    }
    return weight;
}

static void butchery_drops_harvest( item *corpse_item, const mtype &mt, player &p,
                                    const std::function<int()> &roll_butchery, butcher_type action,
                                    const std::function<double()> &roll_drops )
{
    p.add_msg_if_player( m_neutral, _( mt.harvest->message() ) );
    int monster_weight = to_gram( mt.weight );
    monster_weight += round( monster_weight * rng_float( -0.1, 0.1 ) );
    if( corpse_item->has_flag( "QUARTERED" ) ) {
        monster_weight /= 4;
    }
    if( corpse_item->has_flag( "GIBBED" ) ) {
        monster_weight = round( 0.85 * monster_weight );
        if( action != F_DRESS ) {
            p.add_msg_if_player( m_bad,
                                 _( "You salvage what you can from the corpse, but it is badly damaged." ) );
        }
    }
    if( corpse_item->has_flag( "SKINNED" ) ) {
        monster_weight = round( 0.85 * monster_weight );
    }
    int monster_weight_remaining = monster_weight;
    int practice = 4 + roll_butchery();

    if( mt.harvest.is_null() ) {
        debugmsg( "ERROR: %s has no harvest entry.", mt.id.c_str() );
        return;
    }

    for( const harvest_entry &entry : *mt.harvest ) {
        const int butchery = roll_butchery();
        const float min_num = entry.base_num.first + butchery * entry.scale_num.first;
        const float max_num = entry.base_num.second + butchery * entry.scale_num.second;
        int roll = 0;
        // mass_ratio will override the use of base_num, scale_num, and max
        if( entry.mass_ratio != 0.00f ) {
            roll = static_cast<int>( round( entry.mass_ratio * monster_weight ) );
            roll = corpse_damage_effect( roll, entry.type, corpse_item->damage_level( 4 ) );
        } else if( entry.type != "bionic" && entry.type != "bionic_group" ) {
            roll = std::min<int>( entry.max, round( rng_float( min_num, max_num ) ) );
            // will not give less than min_num defined in the JSON
            roll = std::max<int>( corpse_damage_effect( roll, entry.type, corpse_item->damage_level( 4 ) ),
                                  entry.base_num.first );
        }
        const itype *drop = nullptr;
        if( entry.type != "bionic_group" ) {
            drop = item::find_type( entry.drop );
        }

        // BIONIC handling - no code for DISSECT to let the bionic drop fall through
        if( entry.type == "bionic" || entry.type == "bionic_group" ) {
            if( action == F_DRESS ) {
                if( drop != nullptr && !drop->bionic ) {
                    if( one_in( 3 ) ) {
                        p.add_msg_if_player( m_bad,
                                             _( "You notice some strange organs, perhaps harvestable via careful dissection." ) );
                    }
                    continue;
                }
                p.add_msg_if_player( m_bad,
                                     _( "You suspect there might be bionics implanted in this corpse, that careful dissection might reveal." ) );
                continue;
            }
            if( action == BUTCHER || action == BUTCHER_FULL || action == DISMEMBER ) {
                if( drop != nullptr && !drop->bionic ) {
                    if( one_in( 3 ) ) {
                        p.add_msg_if_player( m_bad,
                                             _( "Your butchering tool destroys a strange organ.  Perhaps a more surgical approach would allow harvesting it." ) );
                    }
                    continue;
                }
                switch( rng( 1, 3 ) ) {
                    case 1:
                        p.add_msg_if_player( m_bad,
                                             _( "Your butchering tool encounters something implanted in this corpse, but your rough cuts destroy it." ) );
                        break;
                    case 2:
                        p.add_msg_if_player( m_bad,
                                             _( "You find traces of implants in the body, but you care only for the flesh." ) );
                        break;
                    case 3:
                        p.add_msg_if_player( m_bad,
                                             _( "You found some bionics in the body, but harvesting them would require more surgical approach." ) );
                        break;
                }
                continue;
            }
        }
        if( action == DISSECT ) {
            int roll = roll_butchery() - corpse_item->damage_level( 4 );
            roll = roll < 0 ? 0 : roll;
            roll = std::min( entry.max, roll );
            add_msg( m_debug, _( "Roll penalty for corpse damage = %s" ), 0 - corpse_item->damage_level( 4 ) );
            if( entry.type == "bionic" ) {
                butcher_cbm_item( entry.drop, p.pos(), calendar::turn, roll, entry.flags, entry.faults );
            } else if( entry.type == "bionic_group" ) {
                butcher_cbm_group( entry.drop, p.pos(), calendar::turn, roll, entry.flags, entry.faults );
            }
            continue;
        }

        // Check if monster was gibbed, and handle accordingly
        if( corpse_item->has_flag( "GIBBED" ) && ( entry.type == "flesh" || entry.type == "bone" ) ) {
            roll /= 2;
        }

        if( corpse_item->has_flag( "SKINNED" ) && entry.type == "skin" ) {
            roll = 0;
        }

        // QUICK BUTCHERY
        if( action == BUTCHER ) {
            if( entry.type == "flesh" ) {
                roll = roll / 4;
            } else if( entry.type == "bone" ) {
                roll /= 2;
            } else if( corpse_item->get_mtype()->size >= MS_MEDIUM && ( entry.type == "skin" ) ) {
                roll /= 2;
            } else if( entry.type == "offal" ) {
                roll /= 5;
            } else {
                continue;
            }
        }
        // RIP AND TEAR
        if( action == DISMEMBER ) {
            if( entry.type == "flesh" ) {
                roll /= 6;
            } else {
                continue;
            }
        }
        // field dressing ignores everything outside below list
        if( action == F_DRESS ) {
            if( entry.type == "bone" ) {
                roll = rng( 0, roll / 2 );
            }
            if( entry.type == "flesh" ) {
                continue;
            }
            if( entry.type == "skin" ) {
                continue;
            }
        }

        // you only get the skin from skinning
        if( action == SKIN ) {
            if( entry.type != "skin" ) {
                continue;
            }
            if( corpse_item->has_flag( "FIELD_DRESS_FAILED" ) ) {
                roll = rng( 0, roll );
            }
        }

        // field dressing removed innards and bones from meatless limbs
        if( ( action == BUTCHER_FULL || action == BUTCHER ) && corpse_item->has_flag( "FIELD_DRESS" ) ) {
            if( entry.type == "offal" ) {
                continue;
            }
            if( entry.type == "bone" ) {
                roll = ( roll / 2 ) + rng( roll / 2, roll );
            }
        }
        // unskillfull field dressing may damage the skin, meat, and other parts
        if( ( action == BUTCHER_FULL || action == BUTCHER ) &&
            corpse_item->has_flag( "FIELD_DRESS_FAILED" ) ) {
            if( entry.type == "offal" ) {
                continue;
            }
            if( entry.type == "bone" ) {
                roll = ( roll / 2 ) + rng( roll / 2, roll );
            }
            if( entry.type == "flesh" || entry.type == "skin" ) {
                roll = rng( 0, roll );
            }
        }
        // quartering ruins skin
        if( corpse_item->has_flag( "QUARTERED" ) ) {
            if( entry.type == "skin" ) {
                roll = 0; //not continue to show fail effect
            } else {
                roll /= 4;
            }
        }

        if( action != DISSECT && entry.type != "bionic_group" ) {
            // divide total dropped weight by drop's weight to get amount
            if( entry.mass_ratio != 0.00f ) {
                // apply skill before converting to items, but only if mass_ratio is defined
                roll *= roll_drops();
                monster_weight_remaining -= roll;
                roll = ceil( static_cast<double>( roll ) /
                             to_gram( item::find_type( entry.drop )->weight ) );
            } else {
                monster_weight_remaining -= roll * to_gram( ( item::find_type( entry.drop ) )->weight );
            }

            if( roll <= 0 ) {
                p.add_msg_if_player( m_bad, _( "You fail to harvest: %s" ), drop->nname( 1 ) );
                continue;
            }
            if( drop->phase == LIQUID ) {
                item obj( drop, calendar::turn, roll );
                if( obj.has_temperature() ) {
                    obj.set_item_temperature( 0.00001 * corpse_item->temperature );
                }
                if( obj.goes_bad() ) {
                    obj.set_rot( corpse_item->get_rot() );
                }
                for( const std::string &flg : entry.flags ) {
                    obj.set_flag( flg );
                }
                for( const fault_id &flt : entry.faults ) {
                    obj.faults.emplace( flt );
                }
                // TODO : smarter NPC liquid handling
                if( p.is_npc() ) {
                    drop_on_map( p, item_drop_reason::deliberate, { obj }, p.pos() );
                } else {
                    liquid_handler::handle_all_liquid( obj, 1 );
                }
            } else if( drop->count_by_charges() ) {
                item obj( drop, calendar::turn, roll );
                if( obj.has_temperature() ) {
                    obj.set_item_temperature( 0.00001 * corpse_item->temperature );
                }
                if( obj.goes_bad() ) {
                    obj.set_rot( corpse_item->get_rot() );
                }
                for( const std::string &flg : entry.flags ) {
                    obj.set_flag( flg );
                }
                for( const fault_id &flt : entry.faults ) {
                    obj.faults.emplace( flt );
                }
                if( !p.backlog.empty() && p.backlog.front().id() == activity_id( "ACT_MULTIPLE_BUTCHER" ) ) {
                    obj.set_var( "activity_var", p.name );
                }
                g->m.add_item_or_charges( p.pos(), obj );
            } else {
                item obj( drop, calendar::turn );
                obj.set_mtype( &mt );
                if( obj.has_temperature() ) {
                    obj.set_item_temperature( 0.00001 * corpse_item->temperature );
                }
                if( obj.goes_bad() ) {
                    obj.set_rot( corpse_item->get_rot() );
                }
                for( const std::string &flg : entry.flags ) {
                    obj.set_flag( flg );
                }
                for( const fault_id &flt : entry.faults ) {
                    obj.faults.emplace( flt );
                }
                if( !p.backlog.empty() && p.backlog.front().id() == activity_id( "ACT_MULTIPLE_BUTCHER" ) ) {
                    obj.set_var( "activity_var", p.name );
                }
                for( int i = 0; i != roll; ++i ) {
                    g->m.add_item_or_charges( p.pos(), obj );
                }
            }
            p.add_msg_if_player( m_good, _( "You harvest: %s" ), drop->nname( roll ) );
        }
        practice++;
    }
    // 20% of the original corpse weight is not an item, but liquid gore
    monster_weight_remaining -= monster_weight / 5;
    // add the remaining unusable weight as rotting garbage
    if( monster_weight_remaining > 0 ) {
        if( action == F_DRESS ) {
            // 25% of the corpse weight is what's removed during field dressing
            monster_weight_remaining -= monster_weight * 3 / 4;
        } else if( action == SKIN ) {
            monster_weight_remaining -= monster_weight * 0.85;
        } else {
            // a carcass is 75% of the weight of the unmodified creature's weight
            if( ( corpse_item->has_flag( "FIELD_DRESS" ) || corpse_item->has_flag( "FIELD_DRESS_FAILED" ) ) &&
                !corpse_item->has_flag( "QUARTERED" ) ) {
                monster_weight_remaining -= monster_weight / 4;
            } else if( corpse_item->has_flag( "QUARTERED" ) ) {
                monster_weight_remaining -= ( monster_weight - ( monster_weight * 3 / 4 / 4 ) );
            }
            if( corpse_item->has_flag( "SKINNED" ) ) {
                monster_weight_remaining -= monster_weight * 0.15;
            }
        }
        const int item_charges = monster_weight_remaining / to_gram( (
                                     item::find_type( "ruined_chunks" ) )->weight );
        if( item_charges > 0 ) {
            item ruined_parts( "ruined_chunks", calendar::turn, item_charges );
            ruined_parts.set_mtype( &mt );
            ruined_parts.set_item_temperature( 0.00001 * corpse_item->temperature );
            ruined_parts.set_rot( corpse_item->get_rot() );
            if( !p.backlog.empty() && p.backlog.front().id() == activity_id( "ACT_MULTIPLE_BUTCHER" ) ) {
                ruined_parts.set_var( "activity_var", p.name );
            }
            g->m.add_item_or_charges( p.pos(), ruined_parts );
        }
    }

    if( action == DISSECT ) {
        p.practice( skill_firstaid, std::max( 0, practice ), std::max( mt.size - MS_MEDIUM, 0 ) + 4 );
    } else {
        p.practice( skill_survival, std::max( 0, practice ), std::max( mt.size - MS_MEDIUM, 0 ) + 4 );
    }
}

static void butchery_quarter( item *corpse_item, player &p )
{
    corpse_item->set_flag( "QUARTERED" );
    p.add_msg_if_player( m_good,
                         _( "You roughly slice the corpse of %s into four parts and set them aside." ),
                         corpse_item->get_mtype()->nname() );
    for( int i = 1; i <= 3; i++ ) { // 4 quarters (one exists, add 3, flag does the rest)
        g->m.add_item_or_charges( p.pos(), *corpse_item, true );
    }
}

void activity_handlers::butcher_finish( player_activity *act, player *p )
{
    // No targets means we are done
    if( act->targets.empty() ) {
        act->set_to_null();
        resume_for_multi_activities( *p );
        return;
    }

    item_location &target = act->targets.back();

    // Corpses can disappear (rezzing!), so check for that
    if( !target || !target->is_corpse() ) {
        p->add_msg_if_player( m_info, _( "There's no corpse to butcher!" ) );
        act->set_to_null();
        return;
    }

    butcher_type action = BUTCHER;
    if( act->id() == activity_id( "ACT_BUTCHER" ) ) {
        action = BUTCHER;
    } else if( act->id() == activity_id( "ACT_BUTCHER_FULL" ) ) {
        action = BUTCHER_FULL;
    } else if( act->id() == activity_id( "ACT_FIELD_DRESS" ) ) {
        action = F_DRESS;
    } else if( act->id() == activity_id( "ACT_QUARTER" ) ) {
        action = QUARTER;
    } else if( act->id() == activity_id( "ACT_DISSECT" ) ) {
        action = DISSECT;
    } else if( act->id() == activity_id( "ACT_SKIN" ) ) {
        action = SKIN;
    } else if( act->id() == activity_id( "ACT_DISMEMBER" ) ) {
        action = DISMEMBER;
    }

    // index is a bool that determines if we are ready to start the next target
    if( act->index ) {
        set_up_butchery( *act, *p, action );
        return;
    }

    item &corpse_item = *target;
    std::list<item> contents = corpse_item.contents;
    const mtype *corpse = corpse_item.get_mtype();
    const field_type_id type_blood = corpse->bloodType();
    const field_type_id type_gib = corpse->gibType();

    if( action == QUARTER ) {
        butchery_quarter( &corpse_item, *p );
        act->index = true;
        return;
    }

    int skill_level = p->get_skill_level( skill_survival );
    int factor = p->max_quality( action == DISSECT ? quality_id( "CUT_FINE" ) :
                                 quality_id( "BUTCHER" ) );

    // DISSECT has special case factor calculation and results.
    if( action == DISSECT ) {
        skill_level = p->get_skill_level( skill_firstaid );
        skill_level += p->max_quality( quality_id( "CUT_FINE" ) );
        skill_level += p->get_skill_level( skill_electronics ) / 2;
        add_msg( m_debug, _( "Skill: %s" ), skill_level );
    }

    const auto roll_butchery = [&]() {
        double skill_shift = 0.0;
        ///\EFFECT_SURVIVAL randomly increases butcher rolls
        skill_shift += rng_float( 0, skill_level - 3 );
        ///\EFFECT_DEX >8 randomly increases butcher rolls, slightly, <8 decreases
        skill_shift += rng_float( 0, p->dex_cur - 8 ) / 4.0;

        if( factor < 0 ) {
            skill_shift -= rng_float( 0, -factor / 5.0 );
        }

        return static_cast<int>( round( skill_shift ) );
    };

    if( action == DISMEMBER ) {
        g->m.add_splatter( type_gib, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
    }

    //all BUTCHERY types - FATAL FAILURE
    if( action != DISSECT && roll_butchery() <= ( -15 ) && one_in( 2 ) ) {
        switch( rng( 1, 3 ) ) {
            case 1:
                p->add_msg_if_player( m_warning,
                                      _( "You hack up the corpse so unskillfully, that there is nothing left to salvage from this bloody mess." ) );
                break;
            case 2:
                p->add_msg_if_player( m_warning,
                                      _( "You wanted to cut the corpse, but instead you hacked the meat, spilled the guts all over it, and made a bloody mess." ) );
                break;
            case 3:
                p->add_msg_if_player( m_warning,
                                      _( "You made so many mistakes during the process that you doubt even vultures will be interested in what's left of it." ) );
                break;
        }

        // Remove the target from the map
        target.remove_item();
        act->targets.pop_back();

        g->m.add_splatter( type_gib, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
        g->m.add_splatter( type_blood, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
        for( int i = 1; i <= corpse->size; i++ ) {
            g->m.add_splatter_trail( type_gib, p->pos(), random_entry( g->m.points_in_radius( p->pos(),
                                     corpse->size + 1 ) ) );
            g->m.add_splatter_trail( type_blood, p->pos(), random_entry( g->m.points_in_radius( p->pos(),
                                     corpse->size + 1 ) ) );
        }

        // Ready to move on to the next item, if there is one
        act->index = true;
        return;
    }
    // function just for drop yields
    const auto roll_drops = [&]() {
        factor = std::max( factor, -50 );
        return 0.5 * skill_level / 10 + 0.3 * ( factor + 50 ) / 100 + 0.2 * p->dex_cur / 20;
    };
    // all action types - yields
    butchery_drops_harvest( &corpse_item, *corpse, *p, roll_butchery, action, roll_drops );

    // reveal hidden items / hidden content
    if( action != F_DRESS && action != SKIN ) {
        for( auto &content : contents ) {
            if( ( roll_butchery() + 10 ) * 5 > rng( 0, 100 ) ) {
                //~ %1$s - item name, %2$s - monster name
                p->add_msg_if_player( m_good, _( "You discover a %1$s in the %2$s!" ), content.tname(),
                                      corpse->nname() );
                g->m.add_item_or_charges( p->pos(), content );
            } else if( content.is_bionic() ) {
                g->m.spawn_item( p->pos(), "burnt_out_bionic", 1, 0, calendar::turn );
            }
        }
    }

    //end messages and effects
    switch( action ) {
        case QUARTER:
            break;
        case BUTCHER:
            p->add_msg_if_player( m_good,
                                  _( "You apply few quick cuts to the %s and leave what's left of it for scavengers." ),
                                  corpse_item.tname() );

            // Remove the target from the map
            target.remove_item();
            act->targets.pop_back();
            break;
        case BUTCHER_FULL:
            p->add_msg_if_player( m_good, _( "You finish butchering the %s." ), corpse_item.tname() );

            // Remove the target from the map
            target.remove_item();
            act->targets.pop_back();
            break;
        case F_DRESS:
            if( roll_butchery() < 0 ) {  // partial failure
                switch( rng( 1, 3 ) ) {
                    case 1:
                        p->add_msg_if_player( m_warning,
                                              _( "You unskillfully hack up the corpse and chop off some excess body parts.  You're left wondering how you did so poorly." ) );
                        break;
                    case 2:
                        p->add_msg_if_player( m_warning,
                                              _( "Your unskilled hands slip and damage the corpse.  You still hope it's not a total waste though." ) );
                        break;
                    case 3:
                        p->add_msg_if_player( m_warning,
                                              _( "You did something wrong and hacked the corpse badly.  Maybe it's still recoverable." ) );
                        break;
                }
                corpse_item.set_flag( "FIELD_DRESS_FAILED" );

                g->m.add_splatter( type_gib, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
                g->m.add_splatter( type_blood, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
                for( int i = 1; i <= corpse->size; i++ ) {
                    g->m.add_splatter_trail( type_gib, p->pos(), random_entry( g->m.points_in_radius( p->pos(),
                                             corpse->size + 1 ) ) );
                    g->m.add_splatter_trail( type_blood, p->pos(), random_entry( g->m.points_in_radius( p->pos(),
                                             corpse->size + 1 ) ) );
                }

            } else { // success

                switch( rng( 1, 3 ) ) {
                    case 1:
                        p->add_msg_if_player( m_good, _( "You field dress the %s." ), corpse->nname() );
                        break;
                    case 2:
                        p->add_msg_if_player( m_good,
                                              _( "You slice the corpse's belly and remove intestines and organs, until you're confident that it will not rot from inside." ) );
                        break;
                    case 3:
                        p->add_msg_if_player( m_good,
                                              _( "You remove guts and excess parts, preparing the corpse for later use." ) );
                        break;
                }
                corpse_item.set_flag( "FIELD_DRESS" );

                g->m.add_splatter( type_gib, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
                g->m.add_splatter( type_blood, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
                for( int i = 1; i <= corpse->size; i++ ) {
                    g->m.add_splatter_trail( type_gib, p->pos(), random_entry( g->m.points_in_radius( p->pos(),
                                             corpse->size + 1 ) ) );
                    g->m.add_splatter_trail( type_blood, p->pos(), random_entry( g->m.points_in_radius( p->pos(),
                                             corpse->size + 1 ) ) );
                }

            }
            act->targets.pop_back();
            break;
        case SKIN:
            switch( rng( 1, 4 ) ) {
                case 1:
                    p->add_msg_if_player( m_good, _( "You skin the %s." ), corpse->nname() );
                    break;
                case 2:
                    p->add_msg_if_player( m_good, _( "You carefully remove the hide from the %s" ),
                                          corpse->nname() );
                    break;
                case 3:
                    p->add_msg_if_player( m_good,
                                          _( "The %s is challenging to skin, but you get a good hide from it." ),
                                          corpse->nname() );
                    break;
                case 4:
                    p->add_msg_if_player( m_good, _( "With a few deft slices you take the skin from the %s" ),
                                          corpse->nname() );
                    break;
            }
            corpse_item.set_flag( "SKINNED" );
            act->targets.pop_back();
            break;
        case DISMEMBER:
            switch( rng( 1, 3 ) ) {
                case 1:
                    p->add_msg_if_player( m_good, _( "You hack the %s apart." ), corpse_item.tname() );
                    break;
                case 2:
                    p->add_msg_if_player( m_good, _( "You lop the limbs off the %s." ), corpse_item.tname() );
                    break;
                case 3:
                    p->add_msg_if_player( m_good, _( "You cleave the %s into pieces." ), corpse_item.tname() );
            }

            // Remove the target from the map
            target.remove_item();
            act->targets.pop_back();
            break;
        case DISSECT:
            p->add_msg_if_player( m_good, _( "You finish dissecting the %s." ), corpse_item.tname() );

            // Remove the target from the map
            target.remove_item();
            act->targets.pop_back();
            break;
    }

    // Ready to move on to the next item, if there is one (for example if multibutchering)
    act->index = true;
    // if its mutli-tile butchering,then restart the backlog.
    resume_for_multi_activities( *p );
}

void activity_handlers::fill_liquid_do_turn( player_activity *act, player *p )
{
    player_activity &act_ref = *act;
    try {
        // 1. Gather the source item.
        vehicle *source_veh = nullptr;
        const tripoint source_pos = act_ref.coords.at( 0 );
        map_stack source_stack = g->m.i_at( source_pos );
        map_stack::iterator on_ground;
        monster *source_mon = nullptr;
        item liquid;
        const auto source_type = static_cast<liquid_source_type>( act_ref.values.at( 0 ) );
        int part_num = -1;
        int veh_charges = 0;
        switch( source_type ) {
            case LST_VEHICLE:
                source_veh = veh_pointer_or_null( g->m.veh_at( source_pos ) );
                if( source_veh == nullptr ) {
                    throw std::runtime_error( "could not find source vehicle for liquid transfer" );
                }
                deserialize( liquid, act_ref.str_values.at( 0 ) );
                part_num = static_cast<int>( act_ref.values.at( 1 ) );
                veh_charges = liquid.charges;
                break;
            case LST_INFINITE_MAP:
                deserialize( liquid, act_ref.str_values.at( 0 ) );
                liquid.charges = item::INFINITE_CHARGES;
                break;
            case LST_MAP_ITEM:
                if( static_cast<size_t>( act_ref.values.at( 1 ) ) >= source_stack.size() ) {
                    throw std::runtime_error( "could not find source item on ground for liquid transfer" );
                }
                on_ground = source_stack.begin();
                std::advance( on_ground, act_ref.values.at( 1 ) );
                liquid = *on_ground;
                break;
            case LST_MONSTER:
                Creature *c = g->critter_at( source_pos );
                source_mon = dynamic_cast<monster *>( c );
                if( source_mon == nullptr ) {
                    debugmsg( "could not find source creature for liquid transfer" );
                    act_ref.set_to_null();
                }
                deserialize( liquid, act_ref.str_values.at( 0 ) );
                liquid.charges = 1;
                break;
        }

        static const units::volume volume_per_second = units::from_liter( 4.0F / 6.0F );
        const int charges_per_second = std::max( 1, liquid.charges_per_volume( volume_per_second ) );
        liquid.charges = std::min( charges_per_second, liquid.charges );
        const int original_charges = liquid.charges;
        if( liquid.has_temperature() && liquid.specific_energy < 0 ) {
            liquid.set_item_temperature( std::max( temp_to_kelvin( g->weather.get_temperature( p->pos() ) ),
                                                   277.15 ) );
        }

        // 2. Transfer charges.
        switch( static_cast<liquid_target_type>( act_ref.values.at( 2 ) ) ) {
            case LTT_VEHICLE:
                if( const optional_vpart_position vp = g->m.veh_at( act_ref.coords.at( 1 ) ) ) {
                    p->pour_into( vp->vehicle(), liquid );
                } else {
                    throw std::runtime_error( "could not find target vehicle for liquid transfer" );
                }
                break;
            case LTT_CONTAINER:
                p->pour_into( p->i_at( act_ref.values.at( 3 ) ), liquid );
                break;
            case LTT_MAP:
                if( iexamine::has_keg( act_ref.coords.at( 1 ) ) ) {
                    iexamine::pour_into_keg( act_ref.coords.at( 1 ), liquid );
                } else {
                    g->m.add_item_or_charges( act_ref.coords.at( 1 ), liquid );
                    p->add_msg_if_player( _( "You pour %1$s onto the ground." ), liquid.tname() );
                    liquid.charges = 0;
                }
                break;
            case LTT_MONSTER:
                liquid.charges = 0;
                break;
        }

        const int removed_charges = original_charges - liquid.charges;
        if( removed_charges == 0 ) {
            // Nothing has been transferred, target must be full.
            act_ref.set_to_null();
            return;
        }

        // 3. Remove charges from source.
        switch( source_type ) {
            case LST_VEHICLE:
                if( part_num != -1 ) {
                    source_veh->drain( part_num, removed_charges );
                    liquid.charges = veh_charges - removed_charges;
                    // If there's no liquid left in this tank we're done, otherwise
                    // we need to update our liquid serialization to reflect how
                    // many charges are actually left for the next time we come
                    // around this loop.
                    if( !liquid.charges ) {
                        act_ref.set_to_null();
                    } else {
                        if( act_ref.str_values.empty() ) {
                            act_ref.str_values.push_back( std::string() );
                        }
                        act_ref.str_values.at( 0 ) = serialize( liquid );
                    }
                } else {
                    source_veh->drain( liquid.typeId(), removed_charges );
                }
                if( source_veh->fuel_left( liquid.typeId() ) <= 0 ) {
                    act_ref.set_to_null();
                }
                break;
            case LST_MAP_ITEM:
                on_ground->charges -= removed_charges;
                if( on_ground->charges <= 0 ) {
                    source_stack.erase( on_ground );
                    if( g->m.ter( source_pos ).obj().examine == &iexamine::gaspump ) {
                        add_msg( _( "With a clang and a shudder, the %s pump goes silent." ),
                                 liquid.type_name( 1 ) );
                    } else if( g->m.furn( source_pos ).obj().examine == &iexamine::fvat_full ) {
                        add_msg( _( "You squeeze the last drops of %s from the vat." ),
                                 liquid.type_name( 1 ) );
                        map_stack items_here = g->m.i_at( source_pos );
                        if( items_here.empty() ) {
                            g->m.furn_set( source_pos, f_fvat_empty );
                        }
                    }
                    act_ref.set_to_null();
                }
                break;
            case LST_INFINITE_MAP:
                // nothing, the liquid source is infinite
                break;
            case LST_MONSTER:
                // liquid source charges handled in monexamine::milk_source
                if( liquid.charges == 0 ) {
                    act_ref.set_to_null();
                }
                break;
        }

        if( removed_charges < original_charges ) {
            // Transferred less than the available charges -> target must be full
            act_ref.set_to_null();
        }

    } catch( const std::runtime_error &err ) {
        debugmsg( "error in activity data: \"%s\"", err.what() );
        act_ref.set_to_null();
        return;
    }
}

void activity_handlers::firstaid_finish( player_activity *act, player *p )
{
    static const std::string iuse_name_string( "heal" );

    item &it = p->i_at( act->position );
    item *used_tool = it.get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for healing" );
        act->set_to_null();
        return;
    }

    const auto use_fun = used_tool->get_use( iuse_name_string );
    const auto *actor = dynamic_cast<const heal_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    // TODO: Store the patient somehow, retrieve here
    player &patient = *p;
    const hp_part healed = static_cast<hp_part>( act->values[0] );
    const int charges_consumed = actor->finish_using( *p, patient, *used_tool, healed );
    p->consume_charges( it, charges_consumed );

    // Erase activity and values.
    act->set_to_null();
    act->values.clear();
}

void activity_handlers::forage_finish( player_activity *act, player *p )
{
    const int veggy_chance = rng( 1, 100 );
    bool found_something = false;

    items_location loc;
    ter_str_id next_ter;

    switch( season_of_year( calendar::turn ) ) {
        case SPRING:
            loc = "forage_spring";
            next_ter = ter_str_id( "t_underbrush_harvested_spring" );
            break;
        case SUMMER:
            loc = "forage_summer";
            next_ter = ter_str_id( "t_underbrush_harvested_summer" );
            break;
        case AUTUMN:
            loc = "forage_autumn";
            next_ter = ter_str_id( "t_underbrush_harvested_autumn" );
            break;
        case WINTER:
            loc = "forage_winter";
            next_ter = ter_str_id( "t_underbrush_harvested_winter" );
            break;
        default:
            debugmsg( "Invalid season" );
    }

    g->m.ter_set( act->placement, next_ter );

    // Survival gives a bigger boost, and Perception is leveled a bit.
    // Both survival and perception affect time to forage

    ///\EFFECT_PER slightly increases forage success chance
    ///\EFFECT_SURVIVAL increases forage success chance
    if( veggy_chance < p->get_skill_level( skill_survival ) * 3 + p->per_cur - 2 ) {
        const auto dropped = g->m.put_items_from_loc( loc, p->pos(), calendar::turn );
        for( const auto &it : dropped ) {
            add_msg( m_good, _( "You found: %s!" ), it->tname() );
            found_something = true;
            if( it->has_flag( "FORAGE_POISON" ) && one_in( 10 ) ) {
                it->set_flag( "HIDDEN_POISON" );
                it->poison = rng( 2, 7 );
            }
            if( it->has_flag( "FORAGE_HALLU" ) && !it->has_flag( "HIDDEN_POISON" ) && one_in( 10 ) ) {
                it->set_flag( "HIDDEN_HALLU" );
            }
        }
    }
    // 10% to drop a item/items from this group.
    if( one_in( 10 ) ) {
        const auto dropped = g->m.put_items_from_loc( "trash_forest", p->pos(), calendar::turn );
        for( const auto &it : dropped ) {
            add_msg( m_good, _( "You found: %s!" ), it->tname() );
            found_something = true;
        }
    }

    if( !found_something ) {
        add_msg( _( "You didn't find anything." ) );
    }

    iexamine::practice_survival_while_foraging( p );

    act->set_to_null();
}

void activity_handlers::game_do_turn( player_activity *act, player *p )
{
    //Gaming takes time, not speed
    act->moves_left -= 100;

    item &game_item = p->i_at( act->position );

    //Deduct 1 battery charge for every minute spent playing
    if( calendar::once_every( 1_minutes ) ) {
        game_item.ammo_consume( 1, p->pos() );
        p->add_morale( MORALE_GAME, 1, 100 ); //1 points/min, almost 2 hours to fill
    }
    if( game_item.ammo_remaining() == 0 ) {
        act->moves_left = 0;
        add_msg( m_info, _( "The %s runs out of batteries." ), game_item.tname() );
    }
}

void activity_handlers::hotwire_finish( player_activity *act, player *p )
{
    //Grab this now, in case the vehicle gets shifted
    if( const optional_vpart_position vp = g->m.veh_at( g->m.getlocal( tripoint( act->values[0],
                                           act->values[1],
                                           p->posz() ) ) ) ) {
        vehicle *const veh = &vp->vehicle();
        const int mech_skill = act->values[2];
        if( mech_skill > static_cast<int>( rng( 1, 6 ) ) ) {
            //success
            veh->is_locked = false;
            add_msg( _( "This wire will start the engine." ) );
        } else if( mech_skill > static_cast<int>( rng( 0, 4 ) ) ) {
            //soft fail
            veh->is_locked = false;
            veh->is_alarm_on = veh->has_security_working();
            add_msg( _( "This wire will probably start the engine." ) );
        } else if( veh->is_alarm_on ) {
            veh->is_locked = false;
            add_msg( _( "By process of elimination, this wire will start the engine." ) );
        } else {
            //hard fail
            veh->is_alarm_on = veh->has_security_working();
            add_msg( _( "The red wire always starts the engine, doesn't it?" ) );
        }
    } else {
        dbg( D_ERROR ) << "game:process_activity: ACT_HOTWIRE_CAR: vehicle not found";
        debugmsg( "process_activity ACT_HOTWIRE_CAR: vehicle not found" );
    }
    act->set_to_null();
}

void activity_handlers::longsalvage_finish( player_activity *act, player *p )
{
    static const std::string salvage_string = "salvage";
    item &main_tool = p->i_at( act->index );
    auto items = g->m.i_at( p->pos() );
    item *salvage_tool = main_tool.get_usable_item( salvage_string );
    if( salvage_tool == nullptr ) {
        debugmsg( "Lost tool used for long salvage" );
        act->set_to_null();
        return;
    }

    const auto use_fun = salvage_tool->get_use( salvage_string );
    const auto actor = dynamic_cast<const salvage_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    for( auto &item : items ) {
        if( actor->valid_to_cut_up( item ) ) {
            item_location item_loc( map_cursor( p->pos() ), &item );
            actor->cut_up( *p, *salvage_tool, item_loc );
            return;
        }
    }

    add_msg( _( "You finish salvaging." ) );
    act->set_to_null();
}

void activity_handlers::make_zlave_finish( player_activity *act, player *p )
{
    act->set_to_null();
    auto items = g->m.i_at( p->pos() );
    const std::string corpse_name = act->str_values[0];
    item *body = nullptr;

    for( auto &item : items ) {
        if( item.display_name() == corpse_name ) {
            body = &item;
        }
    }

    if( body == nullptr ) {
        add_msg( m_info, _( "There's no corpse to make into a zombie slave!" ) );
        return;
    }

    int success = act->values[0];

    if( success > 0 ) {

        p->practice( skill_firstaid, rng( 2, 5 ) );
        p->practice( skill_survival, rng( 2, 5 ) );

        p->add_msg_if_player( m_good,
                              _( "You slice muscles and tendons, and remove body parts until you're confident the zombie won't be able to attack you when it reanimates." ) );

        body->set_var( "zlave", "zlave" );
        //take into account the chance that the body yet can regenerate not as we need.
        if( one_in( 10 ) ) {
            body->set_var( "zlave", "mutilated" );
        }

    } else if( success > -20 ) {

        p->practice( skill_firstaid, rng( 3, 6 ) );
        p->practice( skill_survival, rng( 3, 6 ) );

        p->add_msg_if_player( m_warning,
                              _( "You hack into the corpse and chop off some body parts.  You think the zombie won't be able to attack when it reanimates." ) );

        success += rng( 1, 20 );

        if( success > 0 && !one_in( 5 ) ) {
            body->set_var( "zlave", "zlave" );
        } else {
            body->set_var( "zlave", "mutilated" );
        }

    } else {

        p->practice( skill_firstaid, rng( 1, 8 ) );
        p->practice( skill_survival, rng( 1, 8 ) );

        body->mod_damage( rng( 0, body->max_damage() - body->damage() ), DT_STAB );
        if( body->damage() == body->max_damage() ) {
            body->active = false;
            p->add_msg_if_player( m_warning, _( "You cut up the corpse too much, it is thoroughly pulped." ) );
        } else {
            p->add_msg_if_player( m_warning,
                                  _( "You cut into the corpse trying to make it unable to attack, but you don't think you have it right." ) );
        }
    }
}

void activity_handlers::pickaxe_do_turn( player_activity *act, player * )
{
    const tripoint &pos = act->placement;
    sfx::play_activity_sound( "tool", "pickaxe", sfx::get_heard_volume( pos ) );
    if( calendar::once_every( 1_minutes ) ) { // each turn is too much
        //~ Sound of a Pickaxe at work!
        sounds::sound( pos, 30, sounds::sound_t::destructive_activity, _( "CHNK! CHNK! CHNK!" ) );
    }
}

void activity_handlers::pickaxe_finish( player_activity *act, player *p )
{
    const tripoint pos( act->placement );
    item &it = p->i_at( act->position );
    act->set_to_null(); // Invalidate the activity early to prevent a query from mod_pain()
    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    if( g->m.is_bashable( pos ) && g->m.has_flag( "SUPPORTS_ROOF", pos ) &&
        g->m.ter( pos ) != t_tree ) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Betcha wish you'd opted for the J-Hammer ;P
        p->mod_stored_nutr( 15 - ( helpersize * 3 ) );
        p->mod_thirst( 15 - ( helpersize * 3 ) );
        if( p->has_trait( trait_id( "STOCKY_TROGLO" ) ) ) {
            p->mod_fatigue( 20 - ( helpersize  * 3 ) ); // Yep, dwarves can dig longer before tiring
        } else {
            p->mod_fatigue( 30 - ( helpersize  * 3 ) );
        }
        p->mod_pain( std::max( 0, ( 2 * static_cast<int>( rng( 1, 3 ) ) ) - helpersize ) );
    } else if( g->m.move_cost( pos ) == 2 && g->get_levz() == 0 &&
               g->m.ter( pos ) != t_dirt && g->m.ter( pos ) != t_grass ) {
        //Breaking up concrete on the surface? not nearly as bad
        p->mod_stored_nutr( 5 - ( helpersize ) );
        p->mod_thirst( 5 - ( helpersize ) );
        p->mod_fatigue( 10 - ( helpersize  * 2 ) );
    }
    p->add_msg_if_player( m_good, _( "You finish digging." ) );
    g->m.destroy( pos, true );
    it.charges = std::max( 0, it.charges - it.type->charges_to_use() );
    if( it.charges == 0 && it.destroyed_at_zero_charges() ) {
        p->i_rem( &it );
    }
}

void activity_handlers::pulp_do_turn( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;

    // Stabbing weapons are a lot less effective at pulping
    const int cut_power = std::max( p->weapon.damage_melee( DT_CUT ),
                                    p->weapon.damage_melee( DT_STAB ) / 2 );

    ///\EFFECT_STR increases pulping power, with diminishing returns
    float pulp_power = sqrt( ( p->str_cur + p->weapon.damage_melee( DT_BASH ) ) *
                             ( cut_power + 1.0f ) );
    float pulp_effort = p->str_cur + p->weapon.damage_melee( DT_BASH );
    // Multiplier to get the chance right + some bonus for survival skill
    pulp_power *= 40 + p->get_skill_level( skill_survival ) * 5;

    const int mess_radius = p->weapon.has_flag( "MESSY" ) ? 2 : 1;

    int moves = 0;
    int &num_corpses = act->index; // use this to collect how many corpse are pulped
    auto corpse_pile = g->m.i_at( pos );
    for( auto &corpse : corpse_pile ) {
        const mtype *corpse_mtype = corpse.get_mtype();
        if( !corpse.is_corpse() || !corpse_mtype->has_flag( MF_REVIVES ) ||
            ( std::find( act->str_values.begin(), act->str_values.end(), "auto_pulp_no_acid" ) !=
              act->str_values.end() && corpse_mtype->bloodType().obj().has_acid ) ) {
            // Don't smash non-rezing corpses //don't smash acid zombies when auto pulping
            continue;
        }

        while( corpse.damage() < corpse.max_damage() ) {
            // Increase damage as we keep smashing ensuring we eventually smash the target.
            if( x_in_y( pulp_power, corpse.volume() / units::legacy_volume_factor ) ) {
                corpse.inc_damage( DT_BASH );
                if( corpse.damage() == corpse.max_damage() ) {
                    num_corpses++;
                }
            }

            if( x_in_y( pulp_power, corpse.volume() /
                        units::legacy_volume_factor ) ) { // Splatter some blood around
                // Splatter a bit more randomly, so that it looks cooler
                const int radius = mess_radius + x_in_y( pulp_power, 500 ) + x_in_y( pulp_power, 1000 );
                const tripoint dest( pos + point( rng( -radius, radius ), rng( -radius, radius ) ) );
                const field_type_id type_blood = ( mess_radius > 1 && x_in_y( pulp_power, 10000 ) ) ?
                                                 corpse.get_mtype()->gibType() :
                                                 corpse.get_mtype()->bloodType();
                g->m.add_splatter_trail( type_blood, pos, dest );
            }

            p->mod_stat( "stamina", -pulp_effort );

            if( one_in( 4 ) ) {
                // Smashing may not be butchery, but it involves some zombie anatomy
                p->practice( skill_survival, 2, 2 );
            }

            float stamina_ratio = static_cast<float>( p->stamina ) / p->get_stamina_max();
            moves += 100 / std::max( 0.25f, stamina_ratio );
            if( stamina_ratio < 0.33 || p->is_npc() ) {
                p->moves = std::min( 0, p->moves - moves );
                return;
            }
            if( moves >= p->moves ) {
                // Enough for this turn;
                p->moves -= moves;
                return;
            }
        }
        corpse.set_flag( "PULPED" );
    }
    // If we reach this, all corpses have been pulped, finish the activity
    act->moves_left = 0;
    if( num_corpses == 0 ) {
        p->add_msg_if_player( m_bad, _( "The corpse moved before you could finish smashing it!" ) );
        return;
    }
    // TODO: Factor in how long it took to do the smashing.
    p->add_msg_player_or_npc( ngettext( "The corpse is thoroughly pulped.",
                                        "The corpses are thoroughly pulped.", num_corpses ),
                              ngettext( "<npcname> finished pulping the corpse.",
                                        "<npcname> finished pulping the corpses.", num_corpses ) );
}

void activity_handlers::pulp_finish( player_activity *act, player *p )
{
    if( p->is_npc() ) {
        npc *guy = dynamic_cast<npc *>( p );
        guy->revert_after_activity();
    } else {
        act->set_to_null();
    }
}

void activity_handlers::reload_finish( player_activity *act, player *p )
{
    act->set_to_null();

    if( act->targets.size() != 2 || act->index <= 0 ) {
        debugmsg( "invalid arguments to ACT_RELOAD" );
        return;
    }

    if( !act->targets[0] ) {
        debugmsg( "reload target is null, failed to reload" );
        return;
    }

    if( !act->targets[1] ) {
        debugmsg( "ammo target is null, failed to reload" );
        return;
    }

    item &reloadable = *act->targets[ 0 ];
    item &ammo = *act->targets[1];
    const int qty = act->index;
    const bool is_speedloader = ammo.has_flag( "SPEEDLOADER" );
    const bool is_bolt = ammo.ammo_type() == ammotype( "bolt" );

    if( !reloadable.reload( *p, std::move( act->targets[ 1 ] ), qty ) ) {
        add_msg( m_info, _( "Can't reload the %s." ), reloadable.tname() );
        return;
    }

    std::string msg = _( "You reload the %s." );

    if( reloadable.is_gun() ) {
        p->recoil = MAX_RECOIL;

        if( reloadable.has_flag( "RELOAD_ONE" ) && !is_speedloader ) {
            for( int i = 0; i != qty; ++i ) {
                if( is_bolt ) {
                    msg = _( "You insert a bolt into the %s." );
                } else {
                    msg = _( "You insert a cartridge into the %s." );
                }
            }
        }
        if( reloadable.type->gun->reload_noise_volume > 0 ) {
            sfx::play_variant_sound( "reload", reloadable.typeId(), sfx::get_heard_volume( p->pos() ) );
            sounds::ambient_sound( p->pos(), reloadable.type->gun->reload_noise_volume,
                                   sounds::sound_t::activity, reloadable.type->gun->reload_noise );
        }
    } else if( reloadable.is_watertight_container() ) {
        msg = _( "You refill the %s." );
    }
    add_msg( m_neutral, msg, reloadable.tname() );
}

void activity_handlers::start_fire_finish( player_activity *act, player *p )
{
    static const std::string iuse_name_string( "firestarter" );

    item &it = p->i_at( act->position );
    item *used_tool = it.get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for starting fire" );
        act->set_to_null();
        return;
    }

    const auto use_fun = used_tool->get_use( iuse_name_string );
    const auto *actor = dynamic_cast<const firestarter_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    p->consume_charges( it, it.type->charges_to_use() );
    p->practice( skill_survival, act->index, 5 );

    firestarter_actor::resolve_firestarter_use( *p, act->placement );
    act->set_to_null();
}

void activity_handlers::start_fire_do_turn( player_activity *act, player *p )
{
    if( !g->m.is_flammable( act->placement ) ) {
        try_fuel_fire( *act, *p, true );
        if( !g->m.is_flammable( act->placement ) ) {
            p->add_msg_if_player( m_info, _( "There's nothing to light there." ) );
            p->cancel_activity();
            return;
        }
    }

    item &firestarter = p->i_at( act->position );
    if( firestarter.has_flag( "REQUIRES_TINDER" ) ) {
        if( !g->m.tinder_at( act->placement ) ) {
            p->add_msg_if_player( m_info, _( "This item requires tinder to light." ) );
            p->cancel_activity();
            return;
        }
    }

    const auto usef = firestarter.type->get_use( "firestarter" );
    if( usef == nullptr || usef->get_actor_ptr() == nullptr ) {
        add_msg( m_bad, _( "You have lost the item you were using to start the fire." ) );
        p->cancel_activity();
        return;
    }

    p->mod_moves( -p->moves );
    const auto actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
    const float light = actor->light_mod( p->pos() );
    act->moves_left -= light * 100;
    if( light < 0.1 ) {
        add_msg( m_bad, _( "There is not enough sunlight to start a fire now.  You stop trying." ) );
        p->cancel_activity();
    }
}

void activity_handlers::train_finish( player_activity *act, player *p )
{
    const skill_id sk( act->name );
    if( sk.is_valid() ) {
        const Skill &skill = sk.obj();
        std::string skill_name = skill.name();
        int old_skill_level = p->get_skill_level( sk );
        p->practice( sk, 100, old_skill_level + 2 );
        int new_skill_level = p->get_skill_level( sk );
        if( old_skill_level != new_skill_level ) {
            add_msg( m_good, _( "You finish training %s to level %d." ),
                     skill_name, new_skill_level );
            g->events().send<event_type::gains_skill_level>( p->getID(), sk, new_skill_level );
        } else {
            add_msg( m_good, _( "You get some training in %s." ), skill_name );
        }
        act->set_to_null();
        return;
    }

    const auto &ma_id = matype_id( act->name );
    if( ma_id.is_valid() ) {
        const auto &mastyle = ma_id.obj();
        // Trained martial arts,
        add_msg( m_good, _( "You learn %s." ), mastyle.name );
        g->events().send<event_type::learns_martial_art>( p->getID(), ma_id );
        p->add_martialart( mastyle.id );
    } else {
        debugmsg( "train_finish without a valid skill or style name" );
    }

    act->set_to_null();
    return;
}

void activity_handlers::vehicle_finish( player_activity *act, player *p )
{
    //Grab this now, in case the vehicle gets shifted
    const optional_vpart_position vp = g->m.veh_at( g->m.getlocal( tripoint( act->values[0],
                                       act->values[1],
                                       p->posz() ) ) );
    veh_interact::complete_vehicle( *p );
    // complete_vehicle set activity type to NULL if the vehicle
    // was completely dismantled, otherwise the vehicle still exist and
    // is to be examined again.
    if( act->is_null() ) {
        return;
    }
    act->set_to_null();
    if( !p->is_npc() ) {
        if( act->values.size() < 7 ) {
            dbg( D_ERROR ) << "game:process_activity: invalid ACT_VEHICLE values: "
                           << act->values.size();
            debugmsg( "process_activity invalid ACT_VEHICLE values:%d",
                      act->values.size() );
        } else {
            if( vp ) {
                g->m.invalidate_map_cache( g->get_levz() );
                g->refresh_all();
                // TODO: Z (and also where the activity is queued)
                // Or not, because the vehicle coordinates are dropped anyway
                if( resume_for_multi_activities( *p ) ) {
                    return;
                }
                g->exam_vehicle( vp->vehicle(), point( act->values[ 2 ], act->values[ 3 ] ) );
                return;
            } else {
                dbg( D_ERROR ) << "game:process_activity: ACT_VEHICLE: vehicle not found";
                debugmsg( "process_activity ACT_VEHICLE: vehicle not found" );
            }
        }
    }
}

void activity_handlers::hand_crank_do_turn( player_activity *act, player *p )
{
    // Hand-crank chargers seem to range from 2 watt (very common easily verified)
    // to 10 watt (suspicious claims from some manufacturers) sustained output.
    // It takes 2.4 minutes to produce 1kj at just slightly under 7 watts (25 kj per hour)
    // time-based instead of speed based because it's a sustained activity
    act->moves_left -= 100;
    item &hand_crank_item = p ->i_at( act->position );

    if( calendar::once_every( 144_seconds ) ) {
        p->mod_fatigue( 1 );
        if( hand_crank_item.ammo_capacity() > hand_crank_item.ammo_remaining() ) {
            hand_crank_item.ammo_set( "battery", hand_crank_item.ammo_remaining() + 1 );
        } else {
            act->moves_left = 0;
            add_msg( m_info, _( "You've charged the battery completely." ) );
        }
    }
    if( p->get_fatigue() >= DEAD_TIRED ) {
        act->moves_left = 0;
        add_msg( m_info, _( "You're too exhausted to keep cranking." ) );
    }

}

void activity_handlers::vibe_do_turn( player_activity *act, player *p )
{
    //Using a vibrator takes time (10 minutes), not speed
    //Linear increase in morale during action with a small boost at end
    //Deduct 1 battery charge for every minute in use, or vibrator is much less effective
    act->moves_left -= 100;

    item &vibrator_item = p->i_at( act->position );

    if( ( p->is_wearing( "rebreather" ) ) || ( p->is_wearing( "rebreather_xl" ) ) ||
        ( p->is_wearing( "mask_h20survivor" ) ) ) {
        act->moves_left = 0;
        add_msg( m_bad, _( "You have trouble breathing, and stop." ) );
    }

    if( calendar::once_every( 1_minutes ) ) {
        p->mod_fatigue( 1 );
        if( vibrator_item.ammo_remaining() > 0 ) {
            vibrator_item.ammo_consume( 1, p->pos() );
            p->add_morale( MORALE_FEELING_GOOD, 3, 40 );
            if( vibrator_item.ammo_remaining() == 0 ) {
                add_msg( m_info, _( "The %s runs out of batteries." ), vibrator_item.tname() );
            }
        } else {
            p->add_morale( MORALE_FEELING_GOOD, 1, 40 ); //twenty minutes to fill
        }
    }
    if( p->get_fatigue() >= DEAD_TIRED ) { // Dead Tired: different kind of relaxation needed
        act->moves_left = 0;
        add_msg( m_info, _( "You're too tired to continue." ) );
    }

    // Vibrator requires that you be able to move around, stretch, etc, so doesn't play
    // well with roots.  Sorry.  :-(

    p->pause();
}

void activity_handlers::start_engines_finish( player_activity *act, player *p )
{
    act->set_to_null();
    // Find the vehicle by looking for a remote vehicle first, then by player relative coordinates
    vehicle *veh = g->remoteveh();
    if( !veh ) {
        const tripoint pos = act->placement + g->u.pos();
        veh = veh_pointer_or_null( g->m.veh_at( pos ) );
        if( !veh ) {
            return;
        }
    }

    int attempted = 0;
    int non_muscle_attempted = 0;
    int started = 0;
    int non_muscle_started = 0;
    int non_combustion_started = 0;
    const bool take_control = act->values[0];

    for( size_t e = 0; e < veh->engines.size(); ++e ) {
        if( veh->is_engine_on( e ) ) {
            attempted++;
            if( !veh->is_engine_type( e, "muscle" ) && !veh->is_engine_type( e, "animal" ) ) {
                non_muscle_attempted++;
            }
            if( veh->start_engine( e ) ) {
                started++;
                if( !veh->is_engine_type( e, "muscle" ) && !veh->is_engine_type( e, "animal" ) ) {
                    non_muscle_started++;
                } else {
                    non_combustion_started++;
                }
            }
        }
    }

    //Did any engines start?
    veh->engine_on = started;
    //init working engine noise
    sfx::do_vehicle_engine_sfx();

    if( attempted == 0 ) {
        add_msg( m_info, _( "The %s doesn't have an engine!" ), veh->name );
    } else if( non_muscle_attempted > 0 ) {
        //Some non-muscle engines tried to start
        if( non_muscle_attempted == non_muscle_started ) {
            //All of the non-muscle engines started
            add_msg( ngettext( "The %s's engine starts up.",
                               "The %s's engines start up.", non_muscle_started ), veh->name );
        } else if( non_muscle_started > 0 ) {
            //Only some of the non-muscle engines started
            add_msg( ngettext( "One of the %s's engines start up.",
                               "Some of the %s's engines start up.", non_muscle_started ), veh->name );
        } else if( non_combustion_started > 0 ) {
            //Non-combustions "engines" started
            add_msg( _( "The %s is ready for movement." ), veh->name );
        } else {
            //All of the non-muscle engines failed
            add_msg( m_bad, ngettext( "The %s's engine fails to start.",
                                      "The %s's engines fail to start.", non_muscle_attempted ), veh->name );
        }
    }

    if( take_control && !veh->engine_on && !veh->velocity ) {
        p->controlling_vehicle = false;
        add_msg( _( "You let go of the controls." ) );
    }
}

void activity_handlers::oxytorch_do_turn( player_activity *act, player *p )
{
    if( act->values[0] <= 0 ) {
        return;
    }

    item &it = p->i_at( act->position );
    // act->values[0] is the number of charges yet to be consumed
    const int charges_used = std::min( act->values[0], it.ammo_required() );

    it.ammo_consume( charges_used, p->pos() );
    act->values[0] -= static_cast<int>( charges_used );

    sfx::play_activity_sound( "tool", "oxytorch", sfx::get_heard_volume( act->placement ) );
    if( calendar::once_every( 2_turns ) ) {
        sounds::sound( act->placement, 10, sounds::sound_t::destructive_activity, _( "hissssssssss!" ) );
    }
}

void activity_handlers::oxytorch_finish( player_activity *act, player *p )
{
    act->set_to_null();
    const tripoint &pos = act->placement;
    const ter_id ter = g->m.ter( pos );

    // fast players might still have some charges left to be consumed
    p->i_at( act->position ).ammo_consume( act->values[0], p->pos() );

    if( g->m.furn( pos ) == f_rack ) {
        g->m.furn_set( pos, f_null );
        g->m.spawn_item( p->pos(), "steel_chunk", rng( 2, 6 ) );
    } else if( ter == t_chainfence || ter == t_chaingate_c || ter == t_chaingate_l ) {
        g->m.ter_set( pos, t_dirt );
        g->m.spawn_item( pos, "pipe", rng( 1, 4 ) );
        g->m.spawn_item( pos, "wire", rng( 4, 16 ) );
    } else if( ter == t_chainfence_posts ) {
        g->m.ter_set( pos, t_dirt );
        g->m.spawn_item( pos, "pipe", rng( 1, 4 ) );
    } else if( ter == t_door_metal_locked || ter == t_door_metal_c || ter == t_door_bar_c ||
               ter == t_door_bar_locked || ter == t_door_metal_pickable ) {
        g->m.ter_set( pos, t_mdoor_frame );
        g->m.spawn_item( pos, "steel_plate", rng( 0, 1 ) );
        g->m.spawn_item( pos, "steel_chunk", rng( 3, 8 ) );
    } else if( ter == t_window_enhanced || ter == t_window_enhanced_noglass ) {
        g->m.ter_set( pos, t_window_empty );
        g->m.spawn_item( pos, "steel_plate", rng( 0, 1 ) );
        g->m.spawn_item( pos, "sheet_metal", rng( 1, 3 ) );
    } else if( ter == t_reb_cage ) {
        g->m.ter_set( pos, t_pit );
        g->m.spawn_item( pos, "spike", rng( 1, 19 ) );
        g->m.spawn_item( pos, "scrap", rng( 1, 8 ) );
    } else if( ter == t_bars ) {
        if( g->m.ter( pos + point_east ) == t_sewage || g->m.ter( pos + point_south ) ==
            t_sewage ||
            g->m.ter( pos + point_west ) == t_sewage || g->m.ter( pos + point_north ) ==
            t_sewage ) {
            g->m.ter_set( pos, t_sewage );
            g->m.spawn_item( p->pos(), "pipe", rng( 1, 2 ) );
        } else {
            g->m.ter_set( pos, t_floor );
            g->m.spawn_item( p->pos(), "pipe", rng( 1, 2 ) );
        }
    } else if( ter == t_window_bars_alarm ) {
        g->m.ter_set( pos, t_window_alarm );
        g->m.spawn_item( p->pos(), "pipe", rng( 1, 2 ) );
    } else if( ter == t_window_bars ) {
        g->m.ter_set( pos, t_window_empty );
        g->m.spawn_item( p->pos(), "pipe", rng( 1, 2 ) );
    }
}

void activity_handlers::cracking_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( m_good, _( "With a satisfying click, the lock on the safe opens!" ) );
    g->m.furn_set( act->placement, f_safe_c );
    act->set_to_null();
}

void activity_handlers::open_gate_finish( player_activity *act, player * )
{
    const tripoint pos = act->placement; // Don't use reference and don't inline, because act can change
    gates::open_gate( pos );
    act->set_to_null();
}

enum repeat_type : int {
    // REPEAT_INIT should be zero. In some scenarios (veh welder), activity value default to zero.
    REPEAT_INIT = 0,    // Haven't found repeat value yet.
    REPEAT_ONCE,        // Repeat just once
    REPEAT_FOREVER,     // Repeat for as long as possible
    REPEAT_FULL,        // Repeat until damage==0
    REPEAT_EVENT,       // Repeat until something interesting happens
    REPEAT_CANCEL,      // Stop repeating
};

static repeat_type repeat_menu( const std::string &title, repeat_type last_selection )
{
    uilist rmenu;
    rmenu.text = title;

    rmenu.addentry( REPEAT_ONCE, true, '1', _( "Repeat once" ) );
    rmenu.addentry( REPEAT_FOREVER, true, '2', _( "Repeat until reinforced" ) );
    rmenu.addentry( REPEAT_FULL, true, '3', _( "Repeat until fully repaired, but don't reinforce" ) );
    rmenu.addentry( REPEAT_EVENT, true, '4', _( "Repeat until success/failure/level up" ) );
    rmenu.addentry( REPEAT_INIT, true, '5', _( "Back to item selection" ) );

    rmenu.selected = last_selection - REPEAT_ONCE;
    rmenu.query();

    if( rmenu.ret >= REPEAT_INIT && rmenu.ret <= REPEAT_EVENT ) {
        return static_cast<repeat_type>( rmenu.ret );
    }

    return REPEAT_CANCEL;
}

// This is a part of a hack to provide pseudo items for long repair activity
// Note: similar hack could be used to implement all sorts of vehicle pseudo-items
//  and possibly CBM pseudo-items too.
struct weldrig_hack {
    vehicle *veh;
    int part;
    item pseudo;

    weldrig_hack()
        : veh( nullptr )
        , part( -1 )
        , pseudo( "welder", calendar::turn )
    { }

    bool init( const player_activity &act ) {
        if( act.coords.empty() || act.values.size() < 2 ) {
            return false;
        }

        part = act.values[1];
        veh = veh_pointer_or_null( g->m.veh_at( act.coords[0] ) );
        if( veh == nullptr || veh->parts.size() <= static_cast<size_t>( part ) ) {
            part = -1;
            return false;
        }

        part = veh->part_with_feature( part, "WELDRIG", true );
        return part >= 0;
    }

    item &get_item() {
        if( veh != nullptr && part >= 0 ) {
            pseudo.charges = veh->drain( "battery", 1000 - pseudo.charges );
            return pseudo;
        }

        // null item should be handled just fine
        return null_item_reference();
    }

    void clean_up() {
        // Return unused charges
        if( veh == nullptr || part < 0 ) {
            return;
        }

        veh->charge_battery( pseudo.charges );
        pseudo.charges = 0;
    }

    ~weldrig_hack() {
        clean_up();
    }
};

void activity_handlers::repair_item_finish( player_activity *act, player *p )
{
    const std::string iuse_name_string = act->get_str_value( 0, "repair_item" );
    repeat_type repeat = static_cast<repeat_type>( act->get_value( 0, REPEAT_INIT ) );
    weldrig_hack w_hack;
    item_location *ploc = nullptr;

    if( !act->targets.empty() ) {
        ploc = &act->targets[0];
    }

    item &main_tool = !w_hack.init( *act ) ?
                      ploc ?
                      **ploc : p->i_at( act->index ) : w_hack.get_item();

    item *used_tool = main_tool.get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for long repair" );
        act->set_to_null();
        return;
    }

    const auto use_fun = used_tool->get_use( iuse_name_string );
    // TODO: De-uglify this block. Something like get_use<iuse_actor_type>() maybe?
    const auto *actor = dynamic_cast<const repair_item_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    // Valid Repeat choice and target, attempt repair.
    if( repeat != REPEAT_INIT && act->targets.size() >= 2 ) {
        item_location &fix_location = act->targets[1];

        // Remember our level: we want to stop retrying on level up
        const int old_level = p->get_skill_level( actor->used_skill );
        const auto attempt = actor->repair( *p, *used_tool, fix_location );
        if( attempt != repair_item_actor::AS_CANT ) {
            if( ploc && ploc->where() == item_location::type::map ) {
                used_tool->ammo_consume( used_tool->ammo_required(), ploc->position() );
            } else {
                p->consume_charges( *used_tool, used_tool->ammo_required() );
            }
        }

        // TODO: Allow setting this in the actor
        // TODO: Don't use charges_to_use: welder has 50 charges per use, soldering iron has 1
        if( !used_tool->units_sufficient( *p ) ) {
            p->add_msg_if_player( _( "Your %s ran out of charges" ), used_tool->tname() );
            act->set_to_null();
            return;
        }

        // Print message explaining why we stopped
        // But only if we didn't destroy the item (because then it's obvious)
        const bool destroyed = attempt == repair_item_actor::AS_DESTROYED;
        const bool cannot_continue_repair = attempt == repair_item_actor::AS_CANT ||
                                            destroyed || !actor->can_repair_target( *p, *fix_location, !destroyed );
        if( cannot_continue_repair ) {
            // Cannot continue to repair target, select another target.
            // **Warning**: as soon as the item is popped back, it is destroyed and can't be used anymore!
            act->targets.pop_back();
        }

        const bool event_happened = attempt == repair_item_actor::AS_FAILURE ||
                                    attempt == repair_item_actor::AS_SUCCESS ||
                                    old_level != p->get_skill_level( actor->used_skill );

        const bool need_input =
            ( repeat == REPEAT_ONCE ) ||
            ( repeat == REPEAT_EVENT && event_happened ) ||
            ( repeat == REPEAT_FULL && ( cannot_continue_repair || fix_location->damage() <= 0 ) );
        if( need_input ) {
            repeat = REPEAT_INIT;
        }
    }
    // Check tool is valid before we query target and Repeat choice.
    if( !actor->can_use_tool( *p, *used_tool, true ) ) {
        act->set_to_null();
        return;
    }

    // target selection and validation.
    while( act->targets.size() < 2 ) {
        auto item_loc = game_menus::inv::repair( *p, actor, &main_tool );

        if( item_loc == item_location::nowhere ) {
            p->add_msg_if_player( m_info, _( "Never mind." ) );
            act->set_to_null();
            return;
        }
        if( actor->can_repair_target( *p, *item_loc, true ) ) {
            act->targets.emplace_back( item_loc );
            repeat = REPEAT_INIT;
        }
    }

    const item &fix = *act->targets[1];

    if( repeat == REPEAT_INIT ) {
        g->draw();
        const int level = p->get_skill_level( actor->used_skill );
        auto action_type = actor->default_action( fix, level );
        if( action_type == repair_item_actor::RT_NOTHING ) {
            p->add_msg_if_player( _( "You won't learn anything more by doing that." ) );
        }

        const auto chance = actor->repair_chance( *p, fix, action_type );
        if( chance.first <= 0.0f ) {
            action_type = repair_item_actor::RT_PRACTICE;
        }

        std::string title = string_format( _( "%s %s\n" ),
                                           repair_item_actor::action_description( action_type ),
                                           fix.tname() );
        title += string_format( _( "Charges: <color_light_blue>%s/%s</color> %s (%s per use)\n" ),
                                used_tool->ammo_remaining(), used_tool->ammo_capacity(),
                                item::nname( used_tool->ammo_current() ),
                                used_tool->ammo_required() );
        title += string_format( _( "Skill used: <color_light_blue>%s (%s)</color>\n" ),
                                actor->used_skill.obj().name(), level );
        title += string_format( _( "Success chance: <color_light_blue>%.1f</color>%%\n" ),
                                100.0f * chance.first );
        title += string_format( _( "Damage chance: <color_light_blue>%.1f</color>%%" ),
                                100.0f * chance.second );

        if( act->values.empty() ) {
            act->values.resize( 1 );
        }
        do {
            repeat = repeat_menu( title, repeat );

            if( repeat == REPEAT_CANCEL ) {
                act->set_to_null();
                return;
            }
            act->values[0] = static_cast<int>( repeat );
            if( repeat == REPEAT_INIT ) {       // BACK selected, redo target selection next.
                p->activity.targets.pop_back();
                return;
            }
            if( repeat == REPEAT_FULL && fix.damage() <= 0 ) {
                p->add_msg_if_player( m_info, _( "Your %s is already fully repaired." ), fix.tname() );
                repeat = REPEAT_INIT;
            }
        } while( repeat == REPEAT_INIT );
    }

    // Otherwise keep retrying
    act->moves_left = actor->move_cost;
}

void activity_handlers::mend_item_finish( player_activity *act, player *p )
{
    act->set_to_null();
    if( act->targets.size() != 1 ) {
        debugmsg( "invalid arguments to ACT_MEND_ITEM" );
        return;
    }

    item_location &target = act->targets[ 0 ];

    const auto f = target->faults.find( fault_id( act->name ) );
    if( f == target->faults.end() ) {
        debugmsg( "item %s does not have fault %s", target->tname(), act->name );
        return;
    }

    const inventory inv = p->crafting_inventory();
    const requirement_data &reqs = f->obj().requirements();
    if( !reqs.can_make_with_inventory( inv, is_crafting_component ) ) {
        add_msg( m_info, _( "You are currently unable to mend the %s." ), target->tname() );
    }
    for( const auto &e : reqs.get_components() ) {
        p->consume_items( e );
    }
    for( const auto &e : reqs.get_tools() ) {
        p->consume_tools( e );
    }
    p->invalidate_crafting_inventory();

    target->faults.erase( *f );
    if( act->name == "fault_gun_blackpowder" || act->name == "fault_gun_dirt" ) {
        target->set_var( "dirt", 0 );
    }
    add_msg( m_good, _( "You successfully mended the %s." ), target->tname() );
}

void activity_handlers::gunmod_add_finish( player_activity *act, player *p )
{
    act->set_to_null();
    // first unpack all of our arguments
    if( act->values.size() != 4 ) {
        debugmsg( "Insufficient arguments to ACT_GUNMOD_ADD" );
        return;
    }

    item &gun = p->i_at( act->position );
    item &mod = p->i_at( act->values[0] );

    const int roll = act->values[1]; // chance of success (%)
    const int risk = act->values[2]; // chance of damage (%)

    // any tool charges used during installation
    const std::string tool = act->name;
    const int qty = act->values[3];

    if( !gun.is_gunmod_compatible( mod ).success() ) {
        debugmsg( "Invalid arguments in ACT_GUNMOD_ADD" );
        return;
    }

    if( !tool.empty() && qty > 0 ) {
        p->use_charges( tool, qty );
    }

    if( rng( 0, 100 ) <= roll ) {
        add_msg( m_good, _( "You successfully attached the %1$s to your %2$s." ), mod.tname(),
                 gun.tname() );
        gun.contents.push_back( p->i_rem( &mod ) );

    } else if( rng( 0, 100 ) <= risk ) {
        if( gun.inc_damage() ) {
            // Remove irremovable mods prior to destroying the gun
            for( auto mod : gun.gunmods() ) {
                if( mod->is_irremovable() ) {
                    p->remove_item( *mod );
                }
            }
            add_msg( m_bad, _( "You failed at installing the %s and destroyed your %s!" ), mod.tname(),
                     gun.tname() );
            p->i_rem( &gun );
        } else {
            add_msg( m_bad, _( "You failed at installing the %s and damaged your %s!" ), mod.tname(),
                     gun.tname() );
        }

    } else {
        add_msg( m_info, _( "You failed at installing the %s." ), mod.tname() );
    }
}

void activity_handlers::toolmod_add_finish( player_activity *act, player *p )
{
    act->set_to_null();
    if( act->targets.size() != 2 || !act->targets[0] || !act->targets[1] ) {
        debugmsg( "Incompatible arguments to ACT_TOOLMOD_ADD" );
        return;
    }
    item &tool = *act->targets[0];
    item &mod = *act->targets[1];
    p->add_msg_if_player( m_good, _( "You successfully attached the %1$s to your %2$s." ),
                          mod.tname(), tool.tname() );
    mod.item_tags.insert( "IRREMOVABLE" );
    tool.contents.push_back( mod );
    act->targets[1].remove_item();
}

void activity_handlers::clear_rubble_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    p->add_msg_if_player( m_info, _( "You clear up the %s." ),
                          g->m.furnname( pos ) );
    g->m.furn_set( pos, f_null );

    act->set_to_null();
}

void activity_handlers::meditate_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( m_good, _( "You pause to engage in spiritual contemplation." ) );
    p->add_morale( MORALE_FEELING_GOOD, 5, 10 );
    act->set_to_null();
}

void activity_handlers::aim_do_turn( player_activity *act, player * )
{
    if( act->index == 0 ) {
        g->m.invalidate_map_cache( g->get_levz() );
        g->m.build_map_cache( g->get_levz() );
        avatar_action::fire( g->u, g->m );
    }
}

void activity_handlers::pickup_do_turn( player_activity *, player * )
{
    activity_on_turn_pickup();
}

void activity_handlers::wear_do_turn( player_activity *act, player *p )
{
    activity_on_turn_wear( *act, *p );
}

// This activity opens the menu (it's not meant to queue consumption of items)
void activity_handlers::eat_menu_do_turn( player_activity *, player * )
{
    g->eat();
}

void activity_handlers::consume_food_menu_do_turn( player_activity *, player * )
{
    g->eat( game_menus::inv::consume_food );
}

void activity_handlers::consume_drink_menu_do_turn( player_activity *, player * )
{
    g->eat( game_menus::inv::consume_drink );
}

void activity_handlers::consume_meds_menu_do_turn( player_activity *, player * )
{
    g->eat( game_menus::inv::consume_meds );
}

void activity_handlers::move_items_do_turn( player_activity *act, player *p )
{
    activity_on_turn_move_items( *act, *p );
}

void activity_handlers::move_loot_do_turn( player_activity *act, player *p )
{
    activity_on_turn_move_loot( *act, *p );
}

void activity_handlers::adv_inventory_do_turn( player_activity *, player *p )
{
    p->cancel_activity();
    advanced_inv();
}

void activity_handlers::drive_do_turn( player_activity *act, player *p )
{
    vehicle *player_veh = veh_pointer_or_null( g->m.veh_at( p->pos() ) );
    if( !player_veh ) {
        act->set_to_null();
        p->cancel_activity();
        return;
    }
    if( p->in_vehicle && p->controlling_vehicle && player_veh->is_autodriving &&
        !g->u.omt_path.empty() && !player_veh->omt_path.empty() ) {
        player_veh->do_autodrive();
        if( g->u.global_omt_location() == g->u.omt_path.back() ) {
            g->u.omt_path.pop_back();
        }
        p->moves = 0;
    } else {
        p->add_msg_if_player( m_info, _( "Auto-drive cancelled." ) );
        if( !player_veh->omt_path.empty() ) {
            player_veh->omt_path.clear();
        }
        act->set_to_null();
        p->cancel_activity();
        return;
    }
    if( player_veh->omt_path.empty() ) {
        act->set_to_null();
        p->add_msg_if_player( m_info, _( "You have reached your destination." ) );
        p->cancel_activity();
    }
}

void activity_handlers::travel_do_turn( player_activity *act, player *p )
{
    if( !p->omt_path.empty() ) {
        p->omt_path.pop_back();
        if( p->omt_path.empty() ) {
            p->add_msg_if_player( m_info, _( "You have reached your destination." ) );
            g->draw();
            act->set_to_null();
            return;
        }
        tripoint sm_tri = g->m.getlocal( sm_to_ms_copy( omt_to_sm_copy( p->omt_path.back() ) ) );
        tripoint centre_sub = sm_tri + point( SEEX, SEEY );
        if( !g->m.passable( centre_sub ) ) {
            auto candidates = g->m.points_in_radius( centre_sub, 2 );
            for( const auto &elem : candidates ) {
                if( g->m.passable( elem ) ) {
                    centre_sub = elem;
                    break;
                }
            }
        }
        const auto route_to = g->m.route( p->pos(), centre_sub, p->get_pathfinding_settings(),
                                          p->get_path_avoid() );
        if( !route_to.empty() ) {
            const activity_id act_travel = activity_id( "ACT_TRAVELLING" );
            p->set_destination( route_to, player_activity( act_travel ) );
        } else {
            p->add_msg_if_player( _( "You cannot reach that destination" ) );
        }
    } else {
        p->add_msg_if_player( m_info, _( "You have reached your destination." ) );
    }
    g->draw();
    act->set_to_null();
}

void activity_handlers::armor_layers_do_turn( player_activity *, player *p )
{
    p->cancel_activity();
    p->sort_armor();
}

void activity_handlers::atm_do_turn( player_activity *, player *p )
{
    iexamine::atm( *p, p->pos() );
}

// fish-with-rod fish catching function.
static void rod_fish( player *p, std::vector<monster *> &fishables )
{
    //if the vector is empty (no fish around) the player is still given a small chance to get a (let us say it was hidden) fish
    if( fishables.empty() ) {
        const std::vector<mtype_id> fish_group = MonsterGroupManager::GetMonstersFromGroup(
                    mongroup_id( "GROUP_FISH" ) );
        const mtype_id fish_mon = random_entry_ref( fish_group );
        g->m.add_item_or_charges( p->pos(), item::make_corpse( fish_mon, calendar::turn + rng( 0_turns,
                                  3_hours ) ) );
        p->add_msg_if_player( m_good, _( "You caught a %s." ), fish_mon.obj().nname() );
    } else {
        monster *chosen_fish = random_entry( fishables );
        chosen_fish->fish_population -= 1;
        if( chosen_fish->fish_population <= 0 ) {
            g->catch_a_monster( chosen_fish, p->pos(), p, 50_hours );
        } else {
            g->m.add_item_or_charges( p->pos(), item::make_corpse( chosen_fish->type->id,
                                      calendar::turn + rng( 0_turns,
                                              3_hours ) ) );
            p->add_msg_if_player( m_good, _( "You caught a %s." ), chosen_fish->type->nname() );
        }
    }
    for( auto &elem : g->m.i_at( p->pos() ) ) {
        if( elem.is_corpse() && !elem.has_var( "activity_var" ) ) {
            elem.set_var( "activity_var", p->name );
        }
    }
}

void activity_handlers::fish_do_turn( player_activity *act, player *p )
{
    item &it = p->i_at( act->position );
    int fish_chance = 1;
    int survival_skill = p->get_skill_level( skill_survival );
    if( it.has_flag( "FISH_POOR" ) ) {
        survival_skill += dice( 1, 6 );
    } else if( it.has_flag( "FISH_GOOD" ) ) {
        // Much better chances with a good fishing implement.
        survival_skill += dice( 4, 9 );
        survival_skill *= 2;
    }
    std::vector<monster *> fishables = g->get_fishable_monsters( act->coord_set );
    // Fish are always there, even if it dosnt seem like they are visible!
    if( fishables.empty() ) {
        fish_chance += survival_skill / 2;
    } else {
        // if they are visible however, it implies a larger population
        for( auto elem : fishables ) {
            fish_chance += elem->fish_population;
        }
        fish_chance += survival_skill;
    }
    // no matter the population of fish, your skill and tool limits the ease of catching.
    fish_chance = std::min( survival_skill * 10, fish_chance );
    if( x_in_y( fish_chance, 600000 ) ) {
        p->add_msg_if_player( m_good, _( "You feel a tug on your line!" ) );
        rod_fish( p, fishables );
    }
    if( calendar::once_every( 60_minutes ) ) {
        p->practice( skill_survival, rng( 1, 3 ) );
    }

}

void activity_handlers::fish_finish( player_activity *act, player *p )
{
    ( void )p;
    act->set_to_null();
    p->add_msg_if_player( m_info, _( "You finish fishing" ) );
    if( !p->backlog.empty() && p->backlog.front().id() == activity_id( "ACT_MULTIPLE_FISH" ) ) {
        p->backlog.clear();
        p->assign_activity( activity_id( "ACT_TIDY_UP" ) );
    }
}

void activity_handlers::cracking_do_turn( player_activity *act, player *p )
{
    auto cracking_tool = p->crafting_inventory().items_with( []( const item & it ) -> bool {
        item temporary_item( it.type );
        return temporary_item.has_flag( "SAFECRACK" );
    } );
    if( cracking_tool.empty() && !p->has_bionic( bionic_id( "bio_ears" ) ) ) {
        // We lost our cracking tool somehow, bail out.
        act->set_to_null();
        return;
    }
}

void activity_handlers::repair_item_do_turn( player_activity *act, player *p )
{
    // Moves are decremented based on a combination of speed and good vision (not in the dark, farsighted, etc)
    const int effective_moves = p->moves / p->fine_detail_vision_mod();
    if( effective_moves <= act->moves_left ) {
        act->moves_left -= effective_moves;
        p->moves = 0;
    } else {
        p->moves -= act->moves_left * p->fine_detail_vision_mod();
        act->moves_left = 0;
    }
}

void activity_handlers::butcher_do_turn( player_activity * /*act*/, player *p )
{
    p->mod_stat( "stamina", -20 );
}

void activity_handlers::read_do_turn( player_activity *act, player *p )
{
    if( p->is_player() ) {
        if( !act->str_values.empty() && act->str_values[0] == "martial_art" && one_in( 3 ) ) {
            if( act->values.empty() ) {
                act->values.push_back( p->stamina );
            }
            p->stamina = act->values[0] - 1;
            act->values[0] = p->stamina;
        }
        if( p->stamina < p->get_stamina_max() / 10 ) {
            p->add_msg_if_player( m_info, _( "This training is exhausting.  Time to rest." ) );
            act->set_to_null();
        }
    } else {
        p->moves = 0;
    }
}

void activity_handlers::read_finish( player_activity *act, player *p )
{
    if( p->is_npc() ) {
        npc *guy = dynamic_cast<npc *>( p );
        guy->finish_read( * act->targets.front().get_item() );
    } else {
        if( avatar *u = dynamic_cast<avatar *>( p ) ) {
            u->do_read( *act->targets.front().get_item() );
        } else {
            act->set_to_null();
        }
        if( !act ) {
            p->add_msg_if_player( m_info, _( "You finish reading." ) );
        }
    }
}

void activity_handlers::wait_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "You finish waiting." ) );
    act->set_to_null();
}

void activity_handlers::wait_weather_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "You finish waiting." ) );
    act->set_to_null();
}

void activity_handlers::find_mount_do_turn( player_activity *act, player *p )
{
    //npc only activity
    if( p->is_player() ) {
        act->set_to_null();
        return;
    }
    npc &guy = dynamic_cast<npc &>( *p );
    monster *mon = guy.chosen_mount.lock().get();
    if( !mon ) {
        act->set_to_null();
        guy.revert_after_activity();
        return;
    }
    if( rl_dist( guy.pos(), mon->pos() ) <= 1 ) {
        if( mon->has_effect( effect_controlled ) ) {
            mon->remove_effect( effect_controlled );
        }
        if( p->can_mount( *mon ) ) {
            act->set_to_null();
            guy.revert_after_activity();
            guy.chosen_mount = std::weak_ptr<monster>();
            p->mount_creature( *mon );
        } else {
            act->set_to_null();
            guy.revert_after_activity();
            return;
        }
    } else {
        const std::vector<tripoint> route = route_adjacent( *p, guy.chosen_mount.lock().get()->pos() );
        if( route.empty() ) {
            act->set_to_null();
            guy.revert_after_activity();
            mon->remove_effect( effect_controlled );
            return;
        } else {
            p->activity = player_activity();
            mon->add_effect( effect_controlled, 40_turns );
            p->set_destination( route, player_activity( activity_id( "ACT_FIND_MOUNT" ) ) );
        }
    }
}

void activity_handlers::wait_npc_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "%s finishes with you..." ), act->str_values[0] );
    act->set_to_null();
}

void activity_handlers::wait_stamina_do_turn( player_activity *act, player *p )
{
    int stamina_threshold = p->get_stamina_max();
    if( !act->values.empty() ) {
        stamina_threshold = act->values[0];
        // remember initial stamina, only for waiting-with-threshold
        if( act->values.size() == 1 ) {
            act->values.push_back( p->stamina );
        }
    }
    if( p->stamina >= stamina_threshold ) {
        wait_stamina_finish( act, p );
    }
}

void activity_handlers::wait_stamina_finish( player_activity *act, player *p )
{
    if( !act->values.empty() ) {
        const int stamina_threshold = act->values[0];
        const int stamina_initial = ( act->values.size() > 1 ) ? act->values[1] : p->stamina;
        if( p->stamina < stamina_threshold && p->stamina <= stamina_initial ) {
            debugmsg( "Failed to wait until stamina threshold %d reached, only at %d. You may not be regaining stamina.",
                      act->values.front(), p->stamina );
        }
    } else if( p->stamina < p->get_stamina_max() ) {
        p->add_msg_if_player( _( "You are bored of waiting, so you stop." ) );
    } else {
        p->add_msg_if_player( _( "You finish waiting and feel refreshed." ) );
    }
    act->set_to_null();
}

void activity_handlers::socialize_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "%s finishes chatting with you." ), act->str_values[0] );
    act->set_to_null();
}

void activity_handlers::try_sleep_do_turn( player_activity *act, player *p )
{
    if( !p->has_effect( effect_sleep ) ) {
        if( p->can_sleep() ) {
            act->set_to_null();
            p->fall_asleep();
            p->remove_value( "sleep_query" );
        } else if( one_in( 1000 ) ) {
            p->add_msg_if_player( _( "You toss and turn..." ) );
        }
        if( calendar::once_every( 30_minutes ) ) {
            try_sleep_query( act, p );
        }
    }
}

void activity_handlers::try_sleep_query( player_activity *act, player *p )
{
    if( p->get_value( "sleep_query" ) == "false" ) {
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
        case 1:
            act->set_to_null();
            break;
        case 3:
            p->set_value( "sleep_query", "false" );
            break;
        case 2:
        default:
            break;
    }
}

void activity_handlers::operation_do_turn( player_activity *act, player *p )
{
    /**
    - values[0]: Difficulty
    - values[1]: success
    - values[2]: max_power_level
    - values[3]: pl_skill
    - values[4] and above: occupied body_parts
    - str_values[0]: install/uninstall
    - str_values[1]: deprecated
    - str_values[2]: bionic_id
    - str_values[3]: deprecated
    - str_values[4]: upgraded cbm bionic_id
    - str_values[5]: installer name
    - str_values[6]: bool autodoc
    - str_values[7] and above: traits removed by the cbm
    */
    enum operation_values_ids {
        operation_type = 0,
        cbm_name = 1,
        cbm_id = 2,
        upgraded_cbm_name = 3,
        upgraded_cbm_id = 4,
        installer_name = 5,
        is_autodoc = 6,
        trait_first = 7
    };
    const bionic_id bid( act->str_values[cbm_id] );
    const bionic_id upbid( act->str_values[upgraded_cbm_id] );
    const bool autodoc = act->str_values[is_autodoc] == "true";
    const bool u_see = p->is_player() ? true : g->u.sees( p->pos() ) &&
                       !g->u.has_effect( effect_narcosis );

    const int difficulty = act->values.front();

    const time_duration half_op_duration = difficulty * 10_minutes;
    time_duration time_left = time_duration::from_turns( act->moves_left / 100 ) ;

    if( autodoc && g->m.inbounds( p->pos() ) ) {
        const std::list<tripoint> autodocs = g->m.find_furnitures_in_radius( p->pos(), 1,
                                             furn_str_id( "f_autodoc" ) );

        if( g->m.furn( p->pos() ) != furn_str_id( "f_autodoc_couch" ) || autodocs.empty() ) {
            p->remove_effect( effect_under_op );
            act->set_to_null();

            if( u_see ) {
                add_msg( m_bad, _( "The autodoc suffers a catastrophic failure." ) );

                p->add_msg_player_or_npc( m_bad,
                                          _( "The Autodoc's failure damages you greatly." ),
                                          _( "The Autodoc's failure damages <npcname> greatly." ) );
            }
            if( act->values.size() > 4 ) {
                for( size_t i = 4; i < act->values.size(); i++ ) {
                    p->make_bleed( body_part( act->values[i] ), 1_turns, difficulty, true );
                    p->apply_damage( nullptr, body_part( act->values[i] ), 20 * difficulty );

                    if( u_see ) {
                        p->add_msg_player_or_npc( m_bad, _( "Your %s is ripped open." ),
                                                  _( "<npcname>'s %s is ripped open." ),
                                                  body_part_name_accusative( body_part( act->values[i] ) ) );
                    }

                    if( body_part( act->values[i] ) == bp_eyes ) {
                        p->add_effect( effect_blind, 1_hours, num_bp );
                    }
                }
            } else {
                p->make_bleed( num_bp, 1_turns, difficulty, true );
                p->apply_damage( nullptr, num_bp, 20 * difficulty );
            }
        }
    }

    if( time_left > half_op_duration ) {
        if( act->values.size() > 4 ) {
            for( size_t i = 4; i < act->values.size(); i++ ) {
                if( calendar::once_every( 5_minutes ) && u_see && autodoc ) {
                    p->add_msg_player_or_npc( m_info,
                                              _( "The Autodoc is meticulously cutting your %s open." ),
                                              _( "The Autodoc is meticulously cutting <npcname>'s %s open." ),
                                              body_part_name_accusative( body_part( act->values[i] ) ) );
                }
            }
        } else {
            if( calendar::once_every( 5_minutes ) && u_see ) {
                p->add_msg_player_or_npc( m_info,
                                          _( "The Autodoc is meticulously cutting you open." ),
                                          _( "The Autodoc is meticulously cutting <npcname> open." ) );
            }
        }
    } else if( time_left == half_op_duration ) {
        if( act->str_values[operation_type] == "uninstall" ) {
            if( u_see && autodoc ) {
                add_msg( m_info, _( "The Autodoc attempts to carefully extract the bionic." ) );
            }

            if( p->has_bionic( bid ) ) {
                p->perform_uninstall( bid, act->values[0], act->values[1],
                                      act->values[2], act->values[3] );
            } else {
                debugmsg( _( "Tried to uninstall %s, but you don't have this bionic installed." ),
                          act->str_values[cbm_id] );
                p->remove_effect( effect_under_op );
                act->set_to_null();
            }
        } else {
            if( u_see && autodoc ) {
                add_msg( m_info, _( "The Autodoc attempts to carefully insert the bionic." ) );
            }

            if( bid.is_valid() ) {
                std::vector<trait_id> trait_to_rem;
                if( act->str_values.size() > trait_first + 1 ) {
                    for( size_t i = trait_first; i < act->str_values.size(); i++ ) {
                        trait_to_rem.emplace_back( trait_id( act->str_values[i] ) );
                    }
                }
                p->perform_install( bid, upbid, act->values[0], act->values[1], act->values[3],
                                    act->str_values[installer_name], trait_to_rem, p->pos() );
            } else {
                debugmsg( _( "%s is no a valid bionic_id" ), act->str_values[cbm_id] );
                p->remove_effect( effect_under_op );
                act->set_to_null();
            }
        }
    } else if( act->values[1] > 0 ) {
        if( act->values.size() > 4 ) {
            for( size_t i = 4; i < act->values.size(); i++ ) {
                if( calendar::once_every( 5_minutes ) && u_see && autodoc ) {
                    p->add_msg_player_or_npc( m_info,
                                              _( "The Autodoc is stitching your %s back up." ),
                                              _( "The Autodoc is stitching <npcname>'s %s back up." ),
                                              body_part_name_accusative( body_part( act->values[i] ) ) );
                }
            }
        } else {
            if( calendar::once_every( 5_minutes ) && u_see && autodoc ) {
                p->add_msg_player_or_npc( m_info,
                                          _( "The Autodoc is stitching you back up." ),
                                          _( "The Autodoc is stitching <npcname> back up." ) );
            }
        }
    } else {
        if( calendar::once_every( 5_minutes ) && u_see && autodoc ) {
            p->add_msg_player_or_npc( m_bad,
                                      _( "The Autodoc is moving erratically through the rest of its program, not actually stitching your wounds." ),
                                      _( "The Autodoc is moving erratically through the rest of its program, not actually stitching <npcname>'s wounds." ) );
        }
    }

    // Makes sure NPC is still under anesthesia
    if( p->has_effect( effect_narcosis ) ) {
        const time_duration remaining_time = p->get_effect_dur( effect_narcosis );
        if( remaining_time <= time_left ) {
            const time_duration top_off_time = time_left - remaining_time;
            p->add_effect( effect_narcosis, top_off_time );
            p->add_effect( effect_sleep, top_off_time );
        }
    } else {
        p->add_effect( effect_narcosis, time_left );
        p->add_effect( effect_sleep, time_left );
    }
    p->set_moves( 0 );
}

void activity_handlers::try_sleep_finish( player_activity *act, player *p )
{
    if( !p->has_effect( effect_sleep ) ) {
        p->add_msg_if_player( _( "You try to sleep, but can't..." ) );
    }
    act->set_to_null();
}

void activity_handlers::operation_finish( player_activity *act, player *p )
{
    if( act->str_values[6] == "true" ) {
        if( act->values[1] > 0 ) {
            add_msg( m_good,
                     _( "The Autodoc returns to its resting position after successfully performing the operation." ) );
            const std::list<tripoint> autodocs = g->m.find_furnitures_in_radius( p->pos(), 1,
                                                 furn_str_id( "f_autodoc" ) );
            sounds::sound( autodocs.front(), 10, sounds::sound_t::music,
                           _( "a short upbeat jingle: \"Operation successful\"" ), true,
                           "Autodoc",
                           "success" );
        } else {
            add_msg( m_bad,
                     _( "The Autodoc jerks back to its resting position after failing the operation." ) );
            const std::list<tripoint> autodocs = g->m.find_furnitures_in_radius( p->pos(), 1,
                                                 furn_str_id( "f_autodoc" ) );
            sounds::sound( autodocs.front(), 10, sounds::sound_t::music,
                           _( "a sad beeping noise: \"Operation failed\"" ), true,
                           "Autodoc",
                           "failure" );
        }
    } else {
        if( act->values[1] > 0 ) {
            add_msg( m_good,
                     _( "The operation is a success." ) );
        } else {
            add_msg( m_bad,
                     _( "The operation is a failure." ) );
        }
    }
    p->remove_effect( effect_under_op );
    act->set_to_null();
}

void activity_handlers::churn_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "You finish churning up the earth here." ) );
    g->m.ter_set( g->m.getlocal( act->placement ), t_dirtmound );
    // Go back to what we were doing before
    // could be player zone activity, or could be NPC multi-farming
    act->set_to_null();
    resume_for_multi_activities( *p );
}

void activity_handlers::churn_do_turn( player_activity *act, player *p )
{
    ( void )act;
    p->set_moves( 0 );
}

void activity_handlers::build_do_turn( player_activity *act, player *p )
{
    const std::vector<construction> &list_constructions = get_constructions();
    partial_con *pc = g->m.partial_con_at( g->m.getlocal( act->placement ) );
    // Maybe the player and the NPC are working on the same construction at the same time
    if( !pc ) {
        if( p->is_npc() ) {
            // if player completes the work while NPC still in activity loop
            p->activity = player_activity();
            p->set_moves( 0 );
        } else {
            p->cancel_activity();
        }
        add_msg( m_info, _( "%s did not find an unfinished construction at the activity spot." ),
                 p->disp_name() );
        return;
    }
    // if you ( or NPC ) are finishing someone elses started construction...
    const construction &built = list_constructions[pc->id];
    if( !p->meets_skill_requirements( built ) ) {
        add_msg( m_info, _( "%s can't work on this construction anymore." ), p->disp_name() );
        p->cancel_activity();
        if( p->is_npc() ) {
            p->activity = player_activity();
            p->set_moves( 0 );
        }
        return;
    }
    // item_counter represents the percent progress relative to the base batch time
    // stored precise to 5 decimal places ( e.g. 67.32 percent would be stored as 6732000 )
    const int old_counter = pc->counter;

    // Base moves for construction with no speed modifier or assistants
    // Must ensure >= 1 so we don't divide by 0;
    const double base_total_moves = std::max( 1, built.time );
    // Current expected total moves, includes construction speed modifiers and assistants
    const double cur_total_moves = std::max( 1, built.adjusted_time() );
    // Delta progress in moves adjusted for current crafting speed
    const double delta_progress = p->get_moves() * base_total_moves / cur_total_moves;
    // Current progress in moves
    const double current_progress = old_counter * base_total_moves / 10000000.0 +
                                    delta_progress;
    // Current progress as a percent of base_total_moves to 2 decimal places
    pc->counter = round( current_progress / base_total_moves * 10000000.0 );

    p->set_moves( 0 );

    pc->counter = std::min( pc->counter, 10000000 );
    // If construction_progress has reached 100% or more
    if( pc->counter >= 10000000 ) {
        // Activity is cancelled in complete_construction()
        complete_construction( p );
    }
}

void activity_handlers::tidy_up_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::multiple_fish_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::multiple_construction_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::multiple_chop_planks_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::multiple_butcher_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::vehicle_deconstruction_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::vehicle_repair_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::chop_trees_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::multiple_farm_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::fetch_do_turn( player_activity *act, player *p )
{
    generic_multi_activity_handler( *act, *p );
}

void activity_handlers::craft_do_turn( player_activity *act, player *p )
{
    item *craft = act->targets.front().get_item();

    // item_location::get_item() will return nullptr if the item is lost
    if( !craft ) {
        p->add_msg_player_or_npc(
            _( "You no longer have the in progress craft in your possession.  "
               "You stop crafting.  "
               "Reactivate the in progress craft to continue crafting." ),
            _( "<npcname> no longer has the in progress craft in their possession.  "
               "<npcname> stops crafting." ) );
        p->cancel_activity();
        return;
    }

    if( !craft->is_craft() ) {
        debugmsg( "ACT_CRAFT target '%s' is not a craft.  Aborting ACT_CRAFT.", craft->tname() );
        p->cancel_activity();
        return;
    }

    if( !p->can_continue_craft( *craft ) ) {
        p->cancel_activity();
        return;
    }

    const recipe &rec = craft->get_making();
    const tripoint loc = act->targets.front().where() == item_location::type::character ?
                         tripoint_zero : act->targets.front().position();
    const float crafting_speed = p->crafting_speed_multiplier( *craft, loc );
    const int assistants = p->available_assistant_count( craft->get_making() );
    const bool is_long = act->values[0];

    if( crafting_speed <= 0.0f ) {
        p->cancel_activity();
        return;
    }

    // item_counter represents the percent progress relative to the base batch time
    // stored precise to 5 decimal places ( e.g. 67.32 percent would be stored as 6'732'000 )
    const int old_counter = craft->item_counter;

    // Base moves for batch size with no speed modifier or assistants
    // Must ensure >= 1 so we don't divide by 0;
    const double base_total_moves = std::max( 1, rec.batch_time( craft->charges, 1.0f, 0 ) );
    // Current expected total moves, includes crafting speed modifiers and assistants
    const double cur_total_moves = std::max( 1, rec.batch_time( craft->charges, crafting_speed,
                                   assistants ) );
    // Delta progress in moves adjusted for current crafting speed
    const double delta_progress = p->get_moves() * base_total_moves / cur_total_moves;
    // Current progress in moves
    const double current_progress = craft->item_counter * base_total_moves / 10'000'000.0 +
                                    delta_progress;
    // Current progress as a percent of base_total_moves to 2 decimal places
    craft->item_counter = round( current_progress / base_total_moves * 10'000'000.0 );
    p->set_moves( 0 );

    // This is to ensure we don't over count skill steps
    craft->item_counter = std::min( craft->item_counter, 10'000'000 );

    // Skill and tools are gained/consumed after every 5% progress
    int five_percent_steps = craft->item_counter / 500'000 - old_counter / 500'000;
    if( five_percent_steps > 0 ) {
        p->craft_skill_gain( *craft, five_percent_steps );
    }

    // Unlike skill, tools are consumed once at the start and should not be consumed at the end
    if( craft->item_counter >= 10'000'000 ) {
        --five_percent_steps;
    }

    if( five_percent_steps > 0 ) {
        if( !p->craft_consume_tools( *craft, five_percent_steps, false ) ) {
            // So we don't skip over any tool comsuption
            craft->item_counter -= craft->item_counter % 500'000 + 1;
            p->cancel_activity();
            return;
        }
    }

    // if item_counter has reached 100% or more
    if( craft->item_counter >= 10'000'000 ) {
        item craft_copy = *craft;
        act->targets.front().remove_item();
        p->cancel_activity();
        p->complete_craft( craft_copy, loc );
        if( is_long ) {
            if( p->making_would_work( p->lastrecipe, craft_copy.charges ) ) {
                p->last_craft->execute( loc );
            }
        }
    } else if( craft->item_counter >= craft->get_next_failure_point() ) {
        craft->handle_craft_failure( *p );
    }
}

void activity_handlers::disassemble_finish( player_activity *, player *p )
{
    p->complete_disassemble();
}

void activity_handlers::vibe_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( m_good, _( "You feel much better." ) );
    p->add_morale( MORALE_FEELING_GOOD, 10, 40 );
    act->set_to_null();
}

void activity_handlers::atm_finish( player_activity *act, player * )
{
    // ATM sets index to 0 to indicate it's finished.
    if( !act->index ) {
        act->set_to_null();
    }
}

void activity_handlers::aim_finish( player_activity *, player * )
{
    // Aim bails itself by resetting itself every turn,
    // you only re-enter if it gets set again.
    return;
}
void activity_handlers::eat_menu_finish( player_activity *, player * )
{
    // Only exists to keep the eat activity alive between turns
    return;
}

void activity_handlers::hacksaw_do_turn( player_activity *act, player * )
{
    sfx::play_activity_sound( "tool", "hacksaw", sfx::get_heard_volume( act->placement ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a metal sawing tool at work!
        sounds::sound( act->placement, 15, sounds::sound_t::destructive_activity, _( "grnd grnd grnd" ) );
    }
}

void activity_handlers::hacksaw_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    const ter_id ter = g->m.ter( pos );

    if( g->m.furn( pos ) == f_rack ) {
        g->m.furn_set( pos, f_null );
        g->m.spawn_item( p->pos(), "pipe", rng( 1, 3 ) );
        g->m.spawn_item( p->pos(), "steel_chunk" );
    } else if( ter == t_chainfence || ter == t_chaingate_c || ter == t_chaingate_l ) {
        g->m.ter_set( pos, t_dirt );
        g->m.spawn_item( p->pos(), "pipe", 6 );
        g->m.spawn_item( p->pos(), "wire", 20 );
    } else if( ter == t_chainfence_posts ) {
        g->m.ter_set( pos, t_dirt );
        g->m.spawn_item( p->pos(), "pipe", 6 );
    } else if( ter == t_window_bars_alarm ) {
        g->m.ter_set( pos, t_window_alarm );
        g->m.spawn_item( p->pos(), "pipe", 6 );
    } else if( ter == t_window_bars ) {
        g->m.ter_set( pos, t_window_empty );
        g->m.spawn_item( p->pos(), "pipe", 6 );
    } else if( ter == t_window_enhanced ) {
        g->m.ter_set( pos, t_window_reinforced );
        g->m.spawn_item( p->pos(), "spike", rng( 1, 4 ) );
    } else if( ter == t_window_enhanced_noglass ) {
        g->m.ter_set( pos, t_window_reinforced_noglass );
        g->m.spawn_item( p->pos(), "spike", rng( 1, 4 ) );
    } else if( ter == t_reb_cage ) {
        g->m.ter_set( pos, t_pit );
        g->m.spawn_item( p->pos(), "spike", 19 );
        g->m.spawn_item( p->pos(), "scrap", 8 );
    } else if( ter == t_bars ) {
        if( g->m.ter( pos + point_east ) == t_sewage || g->m.ter( pos + point_south )
            == t_sewage ||
            g->m.ter( pos + point_west ) == t_sewage || g->m.ter( pos + point_north ) ==
            t_sewage ) {
            g->m.ter_set( pos, t_sewage );
            g->m.spawn_item( p->pos(), "pipe", 3 );
        } else {
            g->m.ter_set( pos, t_floor );
            g->m.spawn_item( p->pos(), "pipe", 3 );
        }
    } else if( ter == t_door_bar_c || ter == t_door_bar_locked ) {
        g->m.ter_set( pos, t_mdoor_frame );
        g->m.spawn_item( p->pos(), "pipe", 12 );
    }

    p->mod_stored_nutr( 5 );
    p->mod_thirst( 5 );
    p->mod_fatigue( 10 );
    p->add_msg_if_player( m_good, _( "You finish cutting the metal." ) );

    act->set_to_null();
}

void activity_handlers::chop_tree_do_turn( player_activity *act, player * )
{
    sfx::play_activity_sound( "tool", "axe", sfx::get_heard_volume( g->m.getlocal( act->placement ) ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a wood chopping tool at work!
        sounds::sound( g->m.getlocal( act->placement ), 15, sounds::sound_t::activity, _( "CHK!" ) );
    }
}

void activity_handlers::chop_tree_finish( player_activity *act, player *p )
{
    const tripoint &pos = g->m.getlocal( act->placement );

    tripoint direction;
    if( !p->is_npc() ) {
        if( p->backlog.empty() || ( !p->backlog.empty() &&
                                    p->backlog.front().id() != activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ) ) {
            while( true ) {
                if( const cata::optional<tripoint> dir = choose_direction(
                            _( "Select a direction for the tree to fall in." ) ) ) {
                    direction = *dir;
                    break;
                }
                // try again
            }
        }
    } else {
        for( const auto elem : g->m.points_in_radius( pos, 1 ) ) {
            bool cantuse = false;
            tripoint direc = elem - pos;
            tripoint proposed_to = pos + point( 3 * direction.x, 3 * direction.y );
            std::vector<tripoint> rough_tree_line = line_to( pos, proposed_to );
            for( const auto elem : rough_tree_line ) {
                if( g->critter_at( elem ) ) {
                    cantuse = true;
                    break;
                }
            }
            if( !cantuse ) {
                direction = direc;
            }
        }
    }

    const tripoint to = pos + 3 * direction.xy() + point( rng( -1, 1 ), rng( -1, 1 ) );
    std::vector<tripoint> tree = line_to( pos, to, rng( 1, 8 ) );
    for( auto &elem : tree ) {
        g->m.destroy( elem );
        g->m.ter_set( elem, t_trunk );
    }

    g->m.ter_set( pos, t_stump );
    const int helpersize = p->get_num_crafting_helpers( 3 );
    p->mod_stored_nutr( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish chopping down a tree." ) );
    // sound of falling tree
    sfx::play_variant_sound( "misc", "timber",
                             sfx::get_heard_volume( g->m.getlocal( act->placement ) ) );
    act->set_to_null();
    resume_for_multi_activities( *p );
}

void activity_handlers::chop_logs_finish( player_activity *act, player *p )
{
    const tripoint &pos = g->m.getlocal( act->placement );
    int log_quan;
    int stick_quan;
    int splint_quan;
    if( g->m.ter( pos ) == t_trunk ) {
        log_quan = rng( 2, 3 );
        stick_quan = rng( 0, 1 );
        splint_quan = 0;
    } else if( g->m.ter( pos ) == t_stump ) {
        log_quan = rng( 0, 2 );
        stick_quan = 0;
        splint_quan = rng( 5, 15 );
    } else {
        log_quan = 0;
        stick_quan = 0;
        splint_quan = 0;
    }
    for( int i = 0; i != log_quan; ++i ) {
        item obj( "log", calendar::turn );
        obj.set_var( "activity_var", p->name );
        g->m.add_item_or_charges( pos, obj );
    }
    for( int i = 0; i != stick_quan; ++i ) {
        item obj( "stick_long", calendar::turn );
        obj.set_var( "activity_var", p->name );
        g->m.add_item_or_charges( pos, obj );
    }
    for( int i = 0; i != splint_quan; ++i ) {
        item obj( "splinter", calendar::turn );
        obj.set_var( "activity_var", p->name );
        g->m.add_item_or_charges( pos, obj );
    }
    g->m.ter_set( pos, t_dirt );
    const int helpersize = p->get_num_crafting_helpers( 3 );
    p->mod_stored_nutr( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish chopping wood." ) );

    act->set_to_null();
    resume_for_multi_activities( *p );
}

void activity_handlers::chop_planks_finish( player_activity *act, player *p )
{
    const int max_planks = 10;
    /** @EFFECT_FABRICATION increases number of planks cut from a log */
    int planks = normal_roll( 2 + p->get_skill_level( skill_id( "fabrication" ) ), 1 );
    int wasted_planks = max_planks - planks;
    int scraps = rng( wasted_planks, wasted_planks * 3 ) ;
    planks = std::min( planks, max_planks );

    if( planks > 0 ) {
        g->m.spawn_item( g->m.getlocal( act->placement ), "2x4", planks, 0, calendar::turn );
        p->add_msg_if_player( m_good, _( "You produce %d planks." ), planks );
    }
    if( scraps > 0 ) {
        g->m.spawn_item( g->m.getlocal( act->placement ), "splinter", scraps, 0, calendar::turn );
        p->add_msg_if_player( m_good, _( "You produce %d splinters." ), scraps );
    }
    if( planks < max_planks / 2 ) {
        p->add_msg_if_player( m_bad, _( "You waste a lot of the wood." ) );
    }
    act->set_to_null();
    resume_for_multi_activities( *p );
}

void activity_handlers::jackhammer_do_turn( player_activity *act, player * )
{
    sfx::play_activity_sound( "tool", "jackhammer", sfx::get_heard_volume( act->placement ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a jackhammer at work!
        sounds::sound( act->placement, 15, sounds::sound_t::destructive_activity, _( "TATATATATATATAT!" ) );
    }
}

void activity_handlers::jackhammer_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;

    g->m.destroy( pos, true );

    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    p->mod_stored_nutr( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish drilling." ) );

    act->set_to_null();
}

void activity_handlers::dig_do_turn( player_activity *act, player * )
{
    sfx::play_activity_sound( "tool", "shovel", sfx::get_heard_volume( act->placement ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel digging a pit at work!
        sounds::sound( act->placement, 10, sounds::sound_t::activity, _( "hsh!" ) );
    }
}

void activity_handlers::dig_channel_do_turn( player_activity *act, player * )
{
    sfx::play_activity_sound( "tool", "shovel", sfx::get_heard_volume( act->placement ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel digging a pit at work!
        sounds::sound( act->placement, 10, sounds::sound_t::activity, _( "hsh!" ) );
    }
}

void activity_handlers::dig_finish( player_activity *act, player *p )
{
    const ter_id result_terrain( act->str_values[1] );
    const std::string byproducts_item_group = act->str_values[0];
    const int byproducts_count = act->values[0];
    const tripoint dump_loc = act->coords[0];
    const tripoint &pos = act->placement;
    const bool grave = g->m.ter( pos ) == t_grave;

    if( grave ) {
        if( one_in( 10 ) ) {
            static const std::array<mtype_id, 5> monids = { {
                    mtype_id( "mon_zombie" ), mtype_id( "mon_zombie_fat" ),
                    mtype_id( "mon_zombie_rot" ), mtype_id( "mon_skeleton" ),
                    mtype_id( "mon_zombie_crawler" )
                }
            };

            g->place_critter_at( random_entry( monids ), dump_loc );
            g->m.furn_set( pos, f_coffin_o );
            p->add_msg_if_player( m_warning, _( "Something crawls out of the coffin!" ) );
        } else {
            g->m.spawn_item( pos, "bone_human", rng( 5, 15 ) );
            g->m.furn_set( pos, f_coffin_c );
        }
        std::vector<item *> dropped = g->m.place_items( "allclothes", 50, pos, pos, false, calendar::turn );
        g->m.place_items( "grave", 25, pos, pos, false, calendar::turn );
        g->m.place_items( "jewelry_front", 20, pos, pos, false, calendar::turn );
        for( const auto &it : dropped ) {
            if( it->is_armor() ) {
                it->item_tags.insert( "FILTHY" );
                it->set_damage( rng( 1, it->max_damage() - 1 ) );
            }
        }
        g->events().send<event_type::exhumes_grave>( p->getID() );
    }

    g->m.ter_set( pos, result_terrain );

    for( int i = 0; i < byproducts_count; i++ ) {
        g->m.spawn_items( dump_loc, item_group::items_from( byproducts_item_group, calendar::turn ) );
    }

    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    p->mod_stored_nutr( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    if( grave ) {
        p->add_msg_if_player( m_good, _( "You finish exhuming a grave." ) );
    } else {
        p->add_msg_if_player( m_good, _( "You finish digging the %s." ),
                              g->m.ter( act->placement ).obj().name() );
    }

    act->set_to_null();
}

void activity_handlers::dig_channel_finish( player_activity *act, player *p )
{
    const ter_id result_terrain( act->str_values[1] );
    const std::string byproducts_item_group = act->str_values[0];
    const int byproducts_count = act->values[0];
    const tripoint dump_loc = act->coords[0];

    g->m.ter_set( act->placement, result_terrain );

    for( int i = 0; i < byproducts_count; i++ ) {
        g->m.spawn_items( dump_loc, item_group::items_from( byproducts_item_group, calendar::turn ) );
    }

    p->mod_hunger( 5 );
    p->mod_thirst( 5 );
    p->mod_fatigue( 10 );
    p->add_msg_if_player( m_good, _( "You finish digging up %s." ),
                          g->m.ter( act->placement ).obj().name() );

    act->set_to_null();
}

void activity_handlers::fill_pit_do_turn( player_activity *act, player * )
{
    sfx::play_activity_sound( "tool", "shovel", 100 );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel filling a pit or mound at work!
        sounds::sound( act->placement, 10, sounds::sound_t::activity, _( "hsh!" ) );
    }
}

void activity_handlers::fill_pit_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    const ter_id ter = g->m.ter( pos );
    const ter_id old_ter = ter;

    if( ter == t_pit || ter == t_pit_spiked || ter == t_pit_glass ||
        ter == t_pit_corpsed ) {
        g->m.ter_set( pos, t_pit_shallow );
    } else {
        g->m.ter_set( pos, t_dirt );
    }
    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    p->mod_stored_nutr( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish filling up %s." ), old_ter.obj().name() );

    act->set_to_null();
}

void activity_handlers::play_with_pet_finish( player_activity *act, player *p )
{
    p->add_morale( MORALE_PLAY_WITH_PET, rng( 3, 10 ), 10, 5_hours, 25_minutes );
    p->add_msg_if_player( m_good, _( "Playing with your %s has lifted your spirits a bit." ),
                          act->str_values[0] );
    act->set_to_null();
}

void activity_handlers::shaving_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "You open up your kit and shave." ) );
    p->add_morale( MORALE_SHAVE, 8, 8, 240_minutes, 3_minutes );
    act->set_to_null();
}

void activity_handlers::haircut_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "You give your hair a trim." ) );
    p->add_morale( MORALE_HAIRCUT, 3, 3, 480_minutes, 3_minutes );
    act->set_to_null();
}

void activity_handlers::unload_mag_finish( player_activity *act, player *p )
{
    int qty = 0;
    item &it = *act->targets[ 0 ];

    // remove the ammo leads in the belt
    it.contents.erase( std::remove_if( it.contents.begin(),
    it.contents.end(), [&]( item & e ) {
        if( !p->add_or_drop_with_msg( e, true ) ) {
            return false;
        }
        qty += e.charges;
        return true;
    } ), it.contents.end() );

    // remove the belt linkage
    if( it.is_ammo_belt() ) {
        if( it.type->magazine->linkage ) {
            item link( *it.type->magazine->linkage, calendar::turn, qty );
            p->add_or_drop_with_msg( link, true );
        }
        add_msg( _( "You disassemble your %s." ), it.tname() );
    } else {
        add_msg( _( "You unload your %s." ), it.tname() );
    }

    if( it.has_flag( "MAG_DESTROY" ) && it.ammo_remaining() == 0 ) {
        act->targets[ 0 ].remove_item();
    }

    act->set_to_null();
}

std::vector<tripoint> get_sorted_tiles_by_distance( const tripoint &abspos,
        const std::unordered_set<tripoint> &tiles )
{
    const auto cmp = [abspos]( tripoint a, tripoint b ) {
        const int da = rl_dist( abspos, a );
        const int db = rl_dist( abspos, b );

        return da < db;
    };

    std::vector<tripoint> sorted( tiles.begin(), tiles.end() );
    std::sort( sorted.begin(), sorted.end(), cmp );

    return sorted;
}

template<typename fn>
static void cleanup_tiles( std::unordered_set<tripoint> &tiles, fn &cleanup )
{
    auto it = tiles.begin();
    while( it != tiles.end() ) {
        auto current = it++;

        const auto &tile_loc = g->m.getlocal( *current );

        if( cleanup( tile_loc ) ) {
            tiles.erase( current );
        }
    }
}

static void perform_zone_activity_turn( player *p,
                                        const zone_type_id &ztype,
                                        const std::function<bool( const tripoint & )> &tile_filter,
                                        const std::function<void ( player &p, const tripoint & )> &tile_action,
                                        const std::string &finished_msg )
{
    const auto &mgr = zone_manager::get_manager();
    const auto abspos = g->m.getabs( p->pos() );
    auto unsorted_tiles = mgr.get_near( ztype, abspos );

    cleanup_tiles( unsorted_tiles, tile_filter );

    // sort remaining tiles by distance
    const auto &tiles = get_sorted_tiles_by_distance( abspos, unsorted_tiles );

    for( auto &tile : tiles ) {
        const auto &tile_loc = g->m.getlocal( tile );

        auto route = g->m.route( p->pos(), tile_loc, p->get_pathfinding_settings(), p->get_path_avoid() );
        if( route.size() > 1 ) {
            route.pop_back();

            p->set_destination( route, p->activity );
            p->activity.set_to_null();
            return;
        } else { // we are at destination already
            /* Perform action */
            tile_action( *p, tile_loc );
            if( p->moves <= 0 ) {
                return;
            }
        }
    }
    add_msg( m_info, finished_msg );
    p->activity.set_to_null();
}

void activity_handlers::harvest_plot_do_turn( player_activity *, player *p )
{
    const auto reject_tile = []( const tripoint & tile ) {
        return !g->m.has_flag_furn( "GROWTH_HARVEST", tile );
    };
    const auto harvest = [&]( player & p, const tripoint & tile ) {
        iexamine::harvest_plant( p, tile, false );
    };
    perform_zone_activity_turn( p,
                                zone_type_id( "FARM_PLOT" ),
                                reject_tile,
                                harvest,
                                _( "You harvested all the plots you could." ) );

}

void activity_handlers::till_plot_do_turn( player_activity *, player *p )
{
    const auto reject_tile = []( const tripoint & tile ) {
        return !g->m.has_flag( "PLOWABLE", tile ) || g->m.has_furn( tile );
    };

    const auto dig = []( player & p, const tripoint & tile_loc ) {
        p.assign_activity( activity_id( "ACT_CHURN" ), 18000, -1 );
        p.activity.placement = g->m.getabs( tile_loc );
    };
    perform_zone_activity_turn( p,
                                zone_type_id( "FARM_PLOT" ),
                                reject_tile,
                                dig,
                                _( "You tilled every tile you could." ) );
}

void activity_handlers::fertilize_plot_do_turn( player_activity *act, player *p )
{
    itype_id fertilizer;
    auto check_fertilizer = [&]( bool ask_user = true ) -> void {
        if( act->str_values.empty() )
        {
            act->str_values.push_back( "" );
        }
        fertilizer = act->str_values[0];

        /* If unspecified, or if we're out of what we used before, ask */
        if( ask_user && ( fertilizer.empty() || !p->has_charges( fertilizer, 1 ) ) )
        {
            fertilizer = iexamine::choose_fertilizer( *p, "plant",
                    false /* Don't confirm action with player */ );
            act->str_values[0] = fertilizer;
        }
    };

    auto have_fertilizer = [&]() {
        return !fertilizer.empty() && p->has_charges( fertilizer, 1 );
    };

    const auto reject_tile = [&]( const tripoint & tile ) {
        check_fertilizer();
        ret_val<bool> can_fert = iexamine::can_fertilize( *p, tile, fertilizer );
        return !can_fert.success();
    };

    const auto fertilize = [&]( player & p, const tripoint & tile ) {
        check_fertilizer();
        if( have_fertilizer() ) {
            iexamine::fertilize_plant( p, tile, fertilizer );
            if( !have_fertilizer() ) {
                add_msg( m_info, _( "You have run out of %s" ), fertilizer );
            }
        }
    };

    check_fertilizer();
    if( !have_fertilizer() ) {
        act->set_to_null();
        return;
    }

    perform_zone_activity_turn( p,
                                zone_type_id( "FARM_PLOT" ),
                                reject_tile,
                                fertilize,
                                _( "You fertilized every plot you could." ) );
}

void activity_handlers::plant_plot_do_turn( player_activity *, player *p )
{
    const auto &mgr = zone_manager::get_manager();
    std::vector<item *> seed_inv = p->items_with( []( const item & itm ) {
        return itm.is_seed();
    } );

    // get seeds requested by zones on the tile (local coords)
    auto get_seeds = [&]( const tripoint & tile ) {
        auto seeds = std::vector<std::string>();
        const auto &zones = mgr.get_zones( zone_type_id( "FARM_PLOT" ), g->m.getabs( tile ) );
        for( const auto &zone : zones ) {
            const auto options = dynamic_cast<const plot_options &>( zone.get_options() );
            const auto seed = options.get_seed();

            if( !seed.empty() && std::find( seeds.begin(), seeds.end(), seed ) == seeds.end() ) {
                seeds.emplace_back( seed );
            }
        }

        return seeds;
    };

    // cleanup unwanted tiles (local coords)
    const auto reject_tiles = [&]( const tripoint & tile ) {
        if( !g->m.has_flag_ter_or_furn( "PLANTABLE", tile ) ||
            g->m.has_items( tile ) ) {
            return true;
        }

        const auto seeds = get_seeds( tile );

        return std::all_of( seeds.begin(), seeds.end(), [&]( const std::string & seed ) {
            return std::all_of( seed_inv.begin(), seed_inv.end(), [seed]( item * it ) {
                return it->typeId() != itype_id( seed );
            } );
        } );
    };

    const auto plant_appropriate_seed = [&]( player & p, const tripoint & tile_loc ) {
        const auto seeds = get_seeds( tile_loc );
        std::vector<item *> seed_inv = p.items_with( [seeds]( const item & itm ) {
            return itm.is_seed() && std::any_of( seeds.begin(), seeds.end(), [itm]( const std::string & seed ) {
                return itm.typeId() == itype_id( seed );
            } );
        } );
        if( !seed_inv.empty() ) {
            const auto it = seed_inv.front();
            iexamine::plant_seed( p, tile_loc, it->typeId() );
        }
    };

    perform_zone_activity_turn( p,
                                zone_type_id( "FARM_PLOT" ),
                                reject_tiles,
                                plant_appropriate_seed,
                                _( "You planted all seeds you could." ) );
}

void activity_handlers::robot_control_do_turn( player_activity *act, player *p )
{
    if( act->monsters.empty() ) {
        debugmsg( "No monster assigned in ACT_ROBOT_CONTROL" );
        act->set_to_null();
        return;
    }
    const std::shared_ptr<monster> z = act->monsters[0].lock();

    if( !z || !iuse::robotcontrol_can_target( p, *z ) ) {
        p->add_msg_if_player( _( "Target lost. IFF override failed." ) );
        act->set_to_null();
        return;
    }

    // TODO: Add some kind of chance of getting the target's attention

    // Allow time to pass
    p->pause();
}

void activity_handlers::robot_control_finish( player_activity *act, player *p )
{
    act->set_to_null();

    if( act->monsters.empty() ) {
        debugmsg( "No monster assigned in ACT_ROBOT_CONTROL" );
        return;
    }

    std::shared_ptr<monster> z = act->monsters[0].lock();
    act->monsters.clear();

    if( !z || !iuse::robotcontrol_can_target( p, *z ) ) {
        p->add_msg_if_player( _( "Target lost. IFF override failed." ) );
        return;
    }

    p->add_msg_if_player( _( "You unleash your override attack on the %s." ), z->name() );

    /** @EFFECT_INT increases chance of successful robot reprogramming, vs difficulty */
    /** @EFFECT_COMPUTER increases chance of successful robot reprogramming, vs difficulty */
    const int computer_skill = p->get_skill_level( skill_id( "computer" ) );
    const float randomized_skill = rng( 2, p->int_cur ) + computer_skill;
    float success = computer_skill - 3 * z->type->difficulty / randomized_skill;
    if( z->has_flag( MF_RIDEABLE_MECH ) ) {
        success = randomized_skill - rng( 1, 11 );
    }
    // rideable mechs are not hostile, they have no AI, they do not resist control as much.
    if( success >= 0 ) {
        p->add_msg_if_player( _( "You successfully override the %s's IFF protocols!" ),
                              z->name() );
        z->friendly = -1;
        if( z->has_flag( MF_RIDEABLE_MECH ) ) {
            z->add_effect( effect_pet, 1_turns, num_bp, true );
        }
    } else if( success >= -2 ) { //A near success
        p->add_msg_if_player( _( "The %s short circuits as you attempt to reprogram it!" ),
                              z->name() );
        z->apply_damage( p, bp_torso, rng( 1, 10 ) ); //damage it a little
        if( z->is_dead() ) {
            p->practice( skill_id( "computer" ), 10 );
            return; // Do not do the other effects if the robot died
        }
        if( one_in( 3 ) ) {
            p->add_msg_if_player( _( "...and turns friendly!" ) );
            if( one_in( 3 ) ) { //did the robot became friendly permanently?
                z->friendly = -1; //it did
            } else {
                z->friendly = rng( 5, 40 ); // it didn't
            }
        }
    } else {
        p->add_msg_if_player( _( "...but the robot refuses to acknowledge you as an ally!" ) );
    }
    p->practice( skill_id( "computer" ), 10 );
}

void activity_handlers::tree_communion_do_turn( player_activity *act, player *p )
{
    // There's an initial rooting process.
    if( act->values.front() > 0 ) {
        act->values.front() -= 1;
        if( act->values.front() == 0 ) {
            if( p->has_trait( trait_id( "SPIRITUAL" ) ) ) {
                p->add_msg_if_player( m_good, _( "The ancient tree spirits answer your call." ) );
            } else {
                p->add_msg_if_player( m_good, _( "Your communion with the trees has begun." ) );
            }
        }
        return;
    }
    // Information is received every minute.
    if( !calendar::once_every( 1_minutes ) ) {
        return;
    }
    // Breadth-first search forest tiles until one reveals new overmap tiles.
    std::queue<tripoint> q;
    std::unordered_set<tripoint> seen;
    tripoint loc = p->global_omt_location();
    q.push( loc );
    seen.insert( loc );
    const std::function<bool( const oter_id & )> filter = []( const oter_id & ter ) {
        return ter.obj().is_wooded() || ter.obj().get_name() == "field";
    };
    while( !q.empty() ) {
        tripoint tpt = q.front();
        if( overmap_buffer.reveal( tpt, 3, filter ) ) {
            if( p->has_trait( trait_id( "SPIRITUAL" ) ) ) {
                p->add_morale( MORALE_TREE_COMMUNION, 2, 30, 8_hours, 6_hours );
            } else {
                p->add_morale( MORALE_TREE_COMMUNION, 1, 15, 2_hours, 1_hours );
            }
            if( one_in( 128 ) ) {
                p->add_msg_if_player( SNIPPET.random_from_category( "tree_communion" ) );
            }
            return;
        }
        for( const tripoint &neighbor : points_in_radius( tpt, 1 ) ) {
            if( seen.find( neighbor ) != seen.end() ) {
                continue;
            }
            seen.insert( neighbor );
            if( !overmap_buffer.ter( neighbor ).obj().is_wooded() ) {
                continue;
            }
            q.push( neighbor );
        }
        q.pop();
    }
    p->add_msg_if_player( m_info, _( "The trees have shown you what they will." ) );
    act->set_to_null();
}

void activity_handlers::hack_door_finish( player_activity *act, player *p )
{
    hack_result result = iexamine::hack_attempt( *p );
    if( result == HACK_SUCCESS ) {
        add_msg( _( "You activate the panel!" ) );
        add_msg( m_good, _( "The nearby doors slide into the floor." ) );
        g->m.ter_set( act->placement, t_card_reader_broken );
        for( const tripoint &tmp : g->m.points_in_radius( ( act->placement ), 3 ) ) {
            if( g->m.ter( tmp ) == t_door_metal_locked ) {
                g->m.ter_set( tmp, t_floor );
            }
        }
    }

    act->set_to_null();
}

void activity_handlers::hack_safe_finish( player_activity *act, player *p )
{
    hack_result result = iexamine::hack_attempt( *p );
    if( result == HACK_FAIL ) {
        g->events().send<event_type::triggers_alarm>( p->getID() );
        sounds::sound( p->pos(), 60, sounds::sound_t::music, _( "an alarm sound!" ), true,
                       "environment", "alarm" );
        if( act->placement.z > 0 && !g->timed_events.queued( TIMED_EVENT_WANTED ) ) {
            g->timed_events.add( TIMED_EVENT_WANTED, calendar::turn + 30_minutes, 0,
                                 p->global_sm_location() );
        }
    } else if( result == HACK_SUCCESS ) {
        add_msg( m_good, _( "The door on the safe swings open." ) );
        g->m.furn_set( act->placement, furn_str_id( "f_safe_o" ) );
    }

    act->set_to_null();
}

static void blood_magic( player *p, int cost )
{
    static std::array<body_part, 6> part = { {
            bp_head, bp_torso, bp_arm_l, bp_arm_r, bp_leg_l, bp_leg_r
        }
    };
    int max_hp_part = 0;
    std::vector<uilist_entry> uile;
    for( int i = 0; i < num_hp_parts; i++ ) {
        uilist_entry entry( i, p->hp_cur[i] > cost, i + 49, body_part_hp_bar_ui_text( part[i] ) );
        if( p->hp_cur[max_hp_part] < p->hp_cur[i] ) {
            max_hp_part = i;
        }
        const auto &hp = get_hp_bar( p->hp_cur[i], p->hp_max[i] );
        entry.ctxt = colorize( hp.first, hp.second );
        uile.emplace_back( entry );
    }
    int action = -1;
    while( action < 0 ) {
        action = uilist( _( "Choose part\nto draw blood from." ), uile );
    }
    p->hp_cur[action] -= cost;
    p->mod_pain( std::max( 1, cost / 3 ) );
}

static spell casting;

static spell &player_or_item_spell( player *p, const spell_id &sp, int level )
{
    if( level == -1 ) {
        return p->magic.get_spell( sp );
    }
    casting = spell( sp );
    while( casting.get_level() < level && !casting.is_max_level() ) {
        casting.gain_level();
    }
    return casting;
}

void activity_handlers::spellcasting_finish( player_activity *act, player *p )
{
    act->set_to_null();
    const int level_override = act->get_value( 0 );
    spell &casting = player_or_item_spell( p, spell_id( act->name ), level_override );
    const bool no_fail = act->get_value( 1 ) == 1;
    const bool no_mana = act->get_value( 2 ) == 0;

    // choose target for spell (if the spell has a range > 0)

    target_handler th;
    tripoint target = p->pos();
    bool target_is_valid = false;
    if( casting.range() > 0 && !casting.is_valid_target( target_none ) &&
        !casting.has_flag( RANDOM_TARGET ) ) {
        do {
            std::vector<tripoint> trajectory = th.target_ui( casting, no_fail, no_mana );
            if( !trajectory.empty() ) {
                target = trajectory.back();
                target_is_valid = casting.is_valid_target( *p, target );
                if( !( casting.is_valid_target( target_ground ) || p->sees( target ) ) ) {
                    target_is_valid = false;
                }
            } else {
                target_is_valid = false;
            }
            if( !target_is_valid ) {
                if( query_yn( _( "Stop casting spell?  Time spent will be lost." ) ) ) {
                    return;
                }
            }
        } while( !target_is_valid );
    } else if( casting.has_flag( RANDOM_TARGET ) ) {
        target = casting.random_valid_target( *p, p->pos() );
    }

    // no turning back now. it's all said and done.
    bool success = no_fail || rng_float( 0.0f, 1.0f ) >= casting.spell_fail( *p );
    int exp_gained = casting.casting_exp( *p );
    if( !success ) {
        p->add_msg_if_player( m_bad, "You lose your concentration!" );
        if( !casting.is_max_level() && level_override == -1 ) {
            // still get some experience for trying
            casting.gain_exp( exp_gained / 5 );
            p->add_msg_if_player( m_good, _( "You gain %i experience.  New total %i." ), exp_gained / 5,
                                  casting.xp() );
        }
        return;
    }

    if( casting.has_flag( spell_flag::VERBAL ) ) {
        sounds::sound( p->pos(), p->get_shout_volume() / 2, sounds::sound_t::speech, _( "cast a spell" ),
                       false );
    }

    p->add_msg_if_player( casting.message(), casting.name() );

    casting.cast_all_effects( *p, target );

    if( !no_mana ) {
        // pay the cost
        int cost = casting.energy_cost( *p );
        switch( casting.energy_source() ) {
            case mana_energy:
                p->magic.mod_mana( *p, -cost );
                break;
            case stamina_energy:
                p->mod_stat( "stamina", -cost );
                break;
            case bionic_energy:
                p->mod_power_level( -units::from_kilojoule( cost ) );
                break;
            case hp_energy:
                blood_magic( p, cost );
                break;
            case fatigue_energy:
                p->mod_fatigue( cost );
                break;
            case none_energy:
            default:
                break;
        }
    }
    if( level_override == -1 ) {
        if( !casting.is_max_level() ) {
            // reap the reward
            int old_level = casting.get_level();
            if( old_level == 0 ) {
                casting.gain_level();
                p->add_msg_if_player( m_good,
                                      _( "Something about how this spell works just clicked!  You gained a level!" ) );
            } else {
                casting.gain_exp( exp_gained );
                p->add_msg_if_player( m_good, _( "You gain %i experience.  New total %i." ), exp_gained,
                                      casting.xp() );
            }
            if( casting.get_level() != old_level ) {
                g->events().send<event_type::player_levels_spell>( casting.id(), casting.get_level() );
            }
        }
    }
}

void activity_handlers::study_spell_do_turn( player_activity *act, player *p )
{
    if( p->fine_detail_vision_mod() > 4 ) {
        act->values[2] = -1;
        act->moves_left = 0;
        return;
    }
    if( act->get_str_value( 1 ) == "study" ) {
        spell &studying = p->magic.get_spell( spell_id( act->name ) );
        if( act->get_str_value( 0 ) == "gain_level" ) {
            if( studying.get_level() < act->get_value( 1 ) ) {
                act->moves_left = 1000000;
            } else {
                act->moves_left = 0;
            }
        }
        const int xp = roll_remainder( studying.exp_modifier( *p ) / to_turns<float>( 6_seconds ) );
        act->values[0] += xp;
        studying.gain_exp( xp );
    }
}

void activity_handlers::study_spell_finish( player_activity *act, player *p )
{
    act->set_to_null();
    const int total_exp_gained = act->get_value( 0 );

    if( act->get_str_value( 1 ) == "study" ) {
        p->add_msg_if_player( m_good, _( "You gained %i experience from your study session." ),
                              total_exp_gained );
        p->practice( skill_id( "spellcraft" ), total_exp_gained,
                     p->magic.get_spell( spell_id( act->name ) ).get_difficulty() );
    } else if( act->get_str_value( 1 ) == "learn" && act->values[2] == 0 ) {
        p->magic.learn_spell( act->name, *p );
    }
    if( act->values[2] == -1 ) {
        p->add_msg_if_player( m_bad, _( "It's too dark to read." ) );
    }
}

//This is just used for robofac_intercom_mission_2
void activity_handlers::mind_splicer_finish( player_activity *act, player *p )
{
    act->set_to_null();

    if( act->targets.size() != 1 || !act->targets[0] ) {
        debugmsg( "Incompatible arguments to: activity_handlers::mind_splicer_finish" );
        return;
    }
    item &data_card = *act->targets[0];
    p->add_msg_if_player( m_info, _( "...you finally find the memory banks." ) );
    p->add_msg_if_player( m_info, _( "The kit makes a copy of the data inside the bionic." ) );
    data_card.contents.clear();
    data_card.put_in( item( "mind_scan_robofac" ) );
}
