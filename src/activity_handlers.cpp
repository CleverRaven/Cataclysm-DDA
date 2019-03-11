#include "activity_handlers.h"

#include <algorithm>
#include <cmath>

#include "action.h"
#include "advanced_inv.h"
#include "catalua.h"
#include "clzones.h"
#include "construction.h"
#include "craft_command.h"
#include "debug.h"
#include "fault.h"
#include "field.h"
#include "game.h"
#include "gates.h"
#include "harvest.h"
#include "iexamine.h"
#include "itype.h"
#include "iuse_actor.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "messages.h"
#include "mongroup.h"
#include "morale_types.h"
#include "mtype.h"
#include "output.h"
#include "player.h"
#include "requirements.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "veh_interact.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "map_selector.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

const skill_id skill_survival( "survival" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_electronics( "electronics" );

const species_id HUMAN( "HUMAN" );
const species_id ZOMBIE( "ZOMBIE" );

const efftype_id effect_milked( "milked" );
const efftype_id effect_sleep( "sleep" );

using namespace activity_handlers;

const std::map< activity_id, std::function<void( player_activity *, player * )> >
activity_handlers::do_turn_functions = {
    { activity_id( "ACT_BURROW" ), burrow_do_turn },
    { activity_id( "ACT_CRAFT" ), craft_do_turn },
    { activity_id( "ACT_LONGCRAFT" ), craft_do_turn },
    { activity_id( "ACT_FILL_LIQUID" ), fill_liquid_do_turn },
    { activity_id( "ACT_PICKAXE" ), pickaxe_do_turn },
    { activity_id( "ACT_DROP" ), drop_do_turn },
    { activity_id( "ACT_STASH" ), stash_do_turn },
    { activity_id( "ACT_PULP" ), pulp_do_turn },
    { activity_id( "ACT_GAME" ), game_do_turn },
    { activity_id( "ACT_START_FIRE" ), start_fire_do_turn },
    { activity_id( "ACT_VIBE" ), vibe_do_turn },
    { activity_id( "ACT_OXYTORCH" ), oxytorch_do_turn },
    { activity_id( "ACT_AIM" ), aim_do_turn },
    { activity_id( "ACT_PICKUP" ), pickup_do_turn },
    { activity_id( "ACT_WEAR" ), wear_do_turn },
    { activity_id( "ACT_MOVE_ITEMS" ), move_items_do_turn },
    { activity_id( "ACT_MOVE_LOOT" ), move_loot_do_turn },
    { activity_id( "ACT_ADV_INVENTORY" ), adv_inventory_do_turn },
    { activity_id( "ACT_ARMOR_LAYERS" ), armor_layers_do_turn },
    { activity_id( "ACT_ATM" ), atm_do_turn },
    { activity_id( "ACT_CRACKING" ), cracking_do_turn },
    { activity_id( "ACT_REPAIR_ITEM" ), repair_item_do_turn },
    { activity_id( "ACT_BUTCHER" ), butcher_do_turn },
    { activity_id( "ACT_BUTCHER_FULL" ), butcher_do_turn },
    { activity_id( "ACT_FIELD_DRESS" ), butcher_do_turn },
    { activity_id( "ACT_SKIN" ), butcher_do_turn },
    { activity_id( "ACT_QUARTER" ), butcher_do_turn },
    { activity_id( "ACT_DISMEMBER" ), butcher_do_turn },
    { activity_id( "ACT_DISSECT" ), butcher_do_turn },
    { activity_id( "ACT_HACKSAW" ), hacksaw_do_turn },
    { activity_id( "ACT_CHOP_TREE" ), chop_tree_do_turn },
    { activity_id( "ACT_CHOP_LOGS" ), chop_tree_do_turn },
    { activity_id( "ACT_JACKHAMMER" ), jackhammer_do_turn },
    { activity_id( "ACT_DIG" ), dig_do_turn },
    { activity_id( "ACT_FILL_PIT" ), fill_pit_do_turn },
    { activity_id( "ACT_TILL_PLOT" ), till_plot_do_turn },
    { activity_id( "ACT_HARVEST_PLOT" ), harvest_plot_do_turn },
    { activity_id( "ACT_PLANT_PLOT" ), plant_plot_do_turn },
    { activity_id( "ACT_FERTILIZE_PLOT" ), fertilize_plot_do_turn },
    { activity_id( "ACT_TRY_SLEEP" ), try_sleep_do_turn }
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
    { activity_id( "ACT_VEHICLE" ), vehicle_finish },
    { activity_id( "ACT_START_ENGINES" ), start_engines_finish },
    { activity_id( "ACT_OXYTORCH" ), oxytorch_finish },
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
    { activity_id( "ACT_SOCIALIZE" ), socialize_finish },
    { activity_id( "ACT_TRY_SLEEP" ), try_sleep_finish },
    { activity_id( "ACT_CRAFT" ), craft_finish },
    { activity_id( "ACT_LONGCRAFT" ), longcraft_finish },
    { activity_id( "ACT_DISASSEMBLE" ), disassemble_finish },
    { activity_id( "ACT_BUILD" ), build_finish },
    { activity_id( "ACT_VIBE" ), vibe_finish },
    { activity_id( "ACT_ATM" ), atm_finish },
    { activity_id( "ACT_AIM" ), aim_finish },
    { activity_id( "ACT_WASH" ), washing_finish },
    { activity_id( "ACT_HACKSAW" ), hacksaw_finish },
    { activity_id( "ACT_CHOP_TREE" ), chop_tree_finish },
    { activity_id( "ACT_CHOP_LOGS" ), chop_logs_finish },
    { activity_id( "ACT_JACKHAMMER" ), jackhammer_finish },
    { activity_id( "ACT_DIG" ), dig_finish },
    { activity_id( "ACT_FILL_PIT" ), fill_pit_finish },
    { activity_id( "ACT_PLAY_WITH_PET" ), play_with_pet_finish },
    { activity_id( "ACT_SHAVE" ), shaving_finish },
    { activity_id( "ACT_HAIRCUT" ), haircut_finish },
    { activity_id( "ACT_UNLOAD_MAG" ), unload_mag_finish },
    { activity_id( "ACT_ROBOT_CONTROL" ), robot_control_finish }
};

void messages_in_process( const player_activity &act, const player &p )
{
    if( act.moves_left <= 91000 && act.moves_left > 89000 ) {
        p.add_msg_if_player( m_info, _( "You figure it'll take about an hour and a half at this rate." ) );
        return;
    }
    if( act.moves_left <= 61000 && act.moves_left > 59000 ) {
        p.add_msg_if_player( m_info, _( "About an hour left to go." ) );
        return;
    }
    if( act.moves_left <= 31000 && act.moves_left > 29000 ) {
        p.add_msg_if_player( m_info, _( "Shouldn't be more than half an hour or so now!" ) );
        return;
    }
    if( act.moves_left <= 11000 && act.moves_left > 9000 ) {
        p.add_msg_if_player( m_info, _( "Almost there! Ten more minutes of work and you'll be through." ) );
        return;
    }
}

void activity_handlers::burrow_do_turn( player_activity *act, player *p )
{
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a Rat mutant burrowing!
        sounds::sound( act->placement, 10, sounds::sound_t::movement,
                       _( "ScratchCrunchScrabbleScurry." ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::burrow_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    if( g->m.is_bashable( pos ) && g->m.has_flag( "SUPPORTS_ROOF", pos ) &&
        g->m.ter( pos ) != t_tree ) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Not quite as bad as the pickaxe, though
        p->mod_hunger( 10 );
        p->mod_thirst( 10 );
        p->mod_fatigue( 15 );
        p->mod_pain( 3 * rng( 1, 3 ) );
    } else if( g->m.move_cost( pos ) == 2 && g->get_levz() == 0 &&
               g->m.ter( pos ) != t_dirt && g->m.ter( pos ) != t_grass ) {
        //Breaking up concrete on the surface? not nearly as bad
        p->mod_hunger( 5 );
        p->mod_thirst( 5 );
        p->mod_fatigue( 10 );
    }
    p->add_msg_if_player( m_good, _( "You finish burrowing." ) );
    g->m.destroy( pos, true );

    act->set_to_null();
}

bool check_butcher_cbm( const int roll )
{
    // 2/3 chance of failure with a roll of 0, 2/6 with a roll of 1, 2/9 etc.
    // The roll is usually b/t 0 and first_aid-3, so first_aid 4 will succeed
    // 50%, first_aid 5 will succeed 61%, first_aid 6 will succeed 67%, etc.
    bool failed = x_in_y( 2, 3 + roll * 3 );
    return !failed;
}

void butcher_cbm_item( const std::string &what, const tripoint &pos,
                       const time_point &age, const int roll )
{
    if( roll < 0 ) {
        return;
    }

    item cbm( check_butcher_cbm( roll ) ? what : "burnt_out_bionic", age );
    add_msg( m_good, _( "You discover a %s!" ), cbm.tname().c_str() );
    g->m.add_item( pos, cbm );
}

void butcher_cbm_group( const std::string &group, const tripoint &pos,
                        const time_point &age, const int roll )
{
    if( roll < 0 ) {
        return;
    }

    //To see if it spawns a random additional CBM
    if( check_butcher_cbm( roll ) ) {
        //The CBM works
        const auto spawned = g->m.put_items_from_loc( group, pos, age );
        for( const auto &it : spawned ) {
            add_msg( m_good, _( "You discover a %s!" ), it->tname().c_str() );
        }
    } else {
        //There is a burnt out CBM
        item cbm( "burnt_out_bionic", age );
        add_msg( m_good, _( "You discover a %s!" ), cbm.tname().c_str() );
        g->m.add_item( pos, cbm );
    }
}

void set_up_butchery( player_activity &act, player &u, butcher_type action )
{
    if( !act.values.empty() ) {
        act.index = act.values.back();
        act.values.pop_back();
    } else {
        debugmsg( "Invalid butchery item index %d", act.index );
        act.set_to_null();
        return;
    }

    int factor = u.max_quality( action == DISSECT ? quality_id( "CUT_FINE" ) :
                                quality_id( "BUTCHER" ) );
    auto items = g->m.i_at( u.pos() );
    if( static_cast<size_t>( act.index ) >= items.size() ) {
        // Let it print a msg for lack of corpses
        act.index = INT_MAX;
        return;
    }

    item corpse_item = items[act.index];
    const mtype *corpse_ptr = corpse_item.get_mtype();
    if( corpse_ptr == nullptr ) {
        debugmsg( "Tried to butcher a non-corpse item, %s",
                  corpse_item.tname( corpse_item.count() ) );
        act.set_to_null();
        return;
    }
    const mtype &corpse = *corpse_ptr;

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
            ( g->m.veh_at( pt ) && g->m.veh_at( pt )->vehicle().has_part( "KITCHEN" ) ) ) {
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
        bool has_rope = u.has_amount( "rope_30", 1 ) || u.has_amount( "rope_makeshift_30", 1 ) ||
                        u.has_amount( "vine_30", 1 ) || u.has_amount( "grapnel", 1 );
        bool big_corpse = corpse.size >= MS_MEDIUM;

        if( big_corpse && has_rope && !has_tree_nearby && !b_rack_present ) {
            u.add_msg_if_player( m_info,
                                 _( "You need to suspend this corpse to butcher it, you have a rope to lift the corpse but there is no tree nearby." ) );
            act.index = -1;
            return;
        } else if( big_corpse && !has_rope && !b_rack_present ) {
            u.add_msg_if_player( m_info,
                                 _( "For a corpse this big you need a rope and a nearby tree or a butchering rack to perform a full butchery." ) );
            act.index = -1;
            return;
        }
        if( big_corpse && !has_table_nearby ) {
            u.add_msg_if_player( m_info,
                                 _( "For a corpse this big you need a table nearby or something else with a flat surface to perform a full butchery." ) );
            act.index = -1;
            return;
        }
        if( !u.has_quality( quality_id( "CUT" ) ) ) {
            u.add_msg_if_player( m_info, _( "You need a cutting tool to perform a full butchery." ) );
            act.index = -1;
            return;
        }
        if( big_corpse && !( u.has_quality( quality_id( "SAW_W" ) ) ||
                             u.has_quality( quality_id( "SAW_M" ) ) ) ) {
            u.add_msg_if_player( m_info,
                                 _( "For a corpse this big you need a saw to perform a full butchery." ) );
            act.index = -1;
            return;
        }
    }

    if( action == DISSECT && ( corpse_item.has_flag( "QUARTERED" ) ||
                               corpse_item.has_flag( "FIELD_DRESS_FAILED" ) ) ) {
        u.add_msg_if_player( m_info,
                             _( "It would be futile to search for implants inside this badly damaged corpse." ) );
        act.index = -1;
        return;
    }

    if( action == F_DRESS && ( corpse_item.has_flag( "FIELD_DRESS" ) ||
                               corpse_item.has_flag( "FIELD_DRESS_FAILED" ) ) ) {
        u.add_msg_if_player( m_info, _( "This corpse is already field dressed." ) );
        act.index = -1;
        return;
    }

    if( action == SKIN && corpse_item.has_flag( "SKINNED" ) ) {
        u.add_msg_if_player( m_info, _( "This corpse is already skinned." ) );
        act.index = -1;
        return;
    }

    if( action == QUARTER ) {
        if( corpse.size == MS_TINY ) {
            u.add_msg_if_player( m_bad, _( "This corpse is too small to quarter without damaging." ),
                                 corpse.nname().c_str() );
            act.index = -1;
            return;
        }
        if( corpse_item.has_flag( "QUARTERED" ) ) {
            u.add_msg_if_player( m_bad, _( "This is already quartered." ), corpse.nname().c_str() );
            act.index = -1;
            return;
        }
        if( !( corpse_item.has_flag( "FIELD_DRESS" ) || corpse_item.has_flag( "FIELD_DRESS_FAILED" ) ) ) {
            u.add_msg_if_player( m_bad, _( "You need to perform field dressing before quartering." ),
                                 corpse.nname().c_str() );
            act.index = -1;
            return;
        }
    }

    // applies to all butchery actions
    bool is_human = corpse.id == mtype_id::NULL_ID() || ( corpse.in_species( HUMAN ) &&
                    !corpse.in_species( ZOMBIE ) );
    if( is_human && !( u.has_trait_flag( "CANNIBAL" ) || u.has_trait_flag( "PSYCHOPATH" ) ||
                       u.has_trait_flag( "SAPIOVORE" ) ) ) {

        if( query_yn( _( "Would you dare desecrate the mortal remains of a fellow human being?" ) ) ) {
            g->u.add_morale( MORALE_BUTCHER, -50, 0, 2_days, 3_hours );
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
        } else {
            u.add_msg_if_player( m_good, _( "It needs a coffin, not a knife." ) );
            act.index = -1;
            return;
        }
    }

    act.moves_left = butcher_time_to_cut( u, corpse_item, action );
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
            time_to_cut = 25;
            break;
        case MS_SMALL:
            time_to_cut = 50;
            break;
        case MS_MEDIUM:
            time_to_cut = 75;
            break;
        case MS_LARGE:
            time_to_cut = 100;
            break;
        case MS_HUGE:
            time_to_cut = 300;
            break;
    }

    // At factor 0, 10 time_to_cut is 10 turns. At factor 50, it's 5 turns, at 75 it's 2.5
    time_to_cut *= std::max( 25, 100 - factor );
    if( time_to_cut < 500 ) {
        time_to_cut = 500;
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
            time_to_cut *= 2;
            break;
        case SKIN:
            time_to_cut *= 2;
            break;
        case QUARTER:
            time_to_cut /= 4;
            if( time_to_cut < 200 ) {
                time_to_cut = 200;
            }
            break;
        case DISMEMBER:
            time_to_cut /= 10;
            if( time_to_cut < 100 ) {
                time_to_cut = 100;
            }
            break;
        case DISSECT:
            time_to_cut *= 6;
            break;
    }

    if( corpse_item.has_flag( "QUARTERED" ) ) {
        time_to_cut /= 4;
    }
    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    time_to_cut = time_to_cut * ( 1 - ( g->u.get_num_crafting_helpers( 3 ) / 10 ) );
    return time_to_cut;
}
// The below function exists to allow mods to migrate their content fully to the new harvest system. This function should be removed eventually.
harvest_id butchery_flags_deprecate( const mtype &mt )
{
    std::string harvest_id_name = "null";
    if( mt.has_flag( MF_CBM_CIV ) ) {
        harvest_id_name = "CBM_CIV";
    } else if( mt.has_flag( MF_CBM_SCI ) ) {
        harvest_id_name = "CBM_SCI";
    } else if( mt.has_flag( MF_CBM_TECH ) ) {
        harvest_id_name = "CBM_TECH";
    } else if( mt.has_flag( MF_CBM_SUBS ) ) {
        harvest_id_name = "CBM_SUBS";
    } else if( mt.has_flag( MF_CBM_OP ) ) {
        harvest_id_name = "CBM_OP";
    } else if( mt.has_flag( MF_POISON ) ) { // POISON tag means tainted meat
        if( mt.made_of( material_id( "veggy" ) ) ) {
            harvest_id_name = "fungaloid";
        } else if( mt.made_of( material_id( "bone" ) ) ) {
            harvest_id_name = "mr_bones";
        } else if( mt.has_flag( MF_CHITIN ) ) { // only arachnids drop chitin
            harvest_id_name = "arachnid_tainted";
            // acid ants have ACIDPROOF and do not have the CHITIN flag
        } else if( mt.has_flag( MF_ACIDPROOF ) ) {
            harvest_id_name = "arachnid_acid";
        } else if( mt.has_flag( MF_LEATHER ) ) {
            harvest_id_name = "zombie_leather";
        } else if( mt.has_flag( MF_FUR ) ) {
            harvest_id_name = "zombie_fur";
        } else if( mt.has_flag( MF_BONES ) ) {
            harvest_id_name = "zombie";
        } else {
            harvest_id_name = "zombie_meatslug";
        }
    } else { // drops regular edible meat
        if( mt.made_of( material_id( "veggy" ) ) ) {
            harvest_id_name = "triffid_small";
        } else if( mt.size == MS_TINY ) {
            if( mt.has_flag( MF_FEATHER ) ) {
                harvest_id_name = "bird_tiny";
            } else if( mt.has_flag( MF_AQUATIC ) ) {
                harvest_id_name = "fish_small";
            } else {
                harvest_id_name = "mammal_tiny";
            }
        } else if( mt.has_flag( MF_HUMAN ) ) {
            harvest_id_name = "human";
        } else if( mt.size >= MS_SMALL && mt.size <= MS_MEDIUM ) {
            if( mt.has_flag( MF_LEATHER ) ) {
                harvest_id_name = "mammal_leather";
            } else if( mt.has_flag( MF_FUR ) ) {
                harvest_id_name = "mammal_fur";
            } else if( mt.has_flag( MF_WOOL ) ) {
                harvest_id_name = "mammal_wool";
            } else if( mt.has_flag( MF_FEATHER ) ) {
                harvest_id_name = "bird_small";
            } else if( mt.has_flag( MF_AQUATIC ) ) {
                harvest_id_name = "fish_large";
            } else if( mt.has_flag( MF_CHITIN ) ) {
                harvest_id_name = "arachnid";
            } else if( mt.has_flag( MF_BONES ) ) {
                harvest_id_name = "animal_noskin";
            } else {
                harvest_id_name = "meatslug";
            }
        } else if( mt.size >= MS_LARGE ) {
            if( mt.has_flag( MF_LEATHER ) ) {
                harvest_id_name = "mammal_large_leather";
            } else if( mt.has_flag( MF_FUR ) ) {
                harvest_id_name = "mammal_large_fur";
            } else if( mt.has_flag( MF_WOOL ) ) {
                harvest_id_name = "mammal_large_wool";
            } else if( mt.has_flag( MF_FEATHER ) ) {
                harvest_id_name = "bird_large";
            } else if( mt.has_flag( MF_AQUATIC ) ) {
                harvest_id_name = "fish_large";
            } else if( mt.has_flag( MF_CHITIN ) ) {
                harvest_id_name = "arachnid";
            } else if( mt.has_flag( MF_BONES ) ) {
                harvest_id_name = "animal_large_noskin";
            } else {
                harvest_id_name = "meatslug";
            }
        }
    }

    return harvest_id( harvest_id_name );
}

// this function modifies the input weight by its damage level, depending on the bodypart
int corpse_damage_effect( int weight, const std::string &entry_type, int damage_level )
{
    float slight_damage = 0.9;
    float damage = 0.75;
    float high_damage = 0.5;
    int destroyed = 0;

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

void butchery_drops_harvest( item *corpse_item, const mtype &mt, player &p, const time_point &bday,
                             const std::function<int()> &roll_butchery, butcher_type action,
                             const std::function<double()> &roll_drops )
{
    p.add_msg_if_player( m_neutral, _( mt.harvest->message().c_str() ) );
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

    harvest_id hid = mt.harvest.is_null() ? butchery_flags_deprecate( mt ) : mt.harvest;
    const harvest_list &harvest = *hid;

    for( const auto &entry : harvest ) {
        int butchery = roll_butchery();
        float min_num = entry.base_num.first + butchery * entry.scale_num.first;
        float max_num = entry.base_num.second + butchery * entry.scale_num.second;
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
                p.add_msg_if_player( m_bad,
                                     _( "You suspect there might be bionics implanted in this corpse, that careful dissection might reveal." ) );
                continue;
            }
            if( action == BUTCHER || action == BUTCHER_FULL || action == DISMEMBER ) {
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
            if( entry.type == "bionic" ) {
                butcher_cbm_item( entry.drop, p.pos(), bday, roll_butchery() );
            } else if( entry.type == "bionic_group" ) {
                butcher_cbm_group( entry.drop, p.pos(), bday, roll_butchery() );
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
                roll = 0;
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
                roll = 0;
                continue;
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
                roll = ceil( roll / to_gram( ( item::find_type( entry.drop ) )->weight ) );
            } else {
                monster_weight_remaining -= roll * to_gram( ( item::find_type( entry.drop ) )->weight );
            }

            if( roll <= 0 ) {
                p.add_msg_if_player( m_bad, _( "You fail to harvest: %s" ), drop->nname( 1 ) );
                continue;
            }

            if( drop->phase == LIQUID ) {
                g->handle_all_liquid( item( drop, bday, roll ), 1 );

            } else if( drop->stackable ) {
                g->m.add_item_or_charges( p.pos(), item( drop, bday, roll ) );

            } else {
                item obj( drop, bday );
                obj.set_mtype( &mt );
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
            item ruined_parts( "ruined_chunks", bday, item_charges );
            ruined_parts.set_mtype( &mt );
            g->m.add_item_or_charges( p.pos(), ruined_parts );
        }
    }

    if( action == DISSECT ) {
        p.practice( skill_firstaid, std::max( 0, practice ), std::max( mt.size - MS_MEDIUM, 0 ) + 4 );
    } else {
        p.practice( skill_survival, std::max( 0, practice ), std::max( mt.size - MS_MEDIUM, 0 ) + 4 );
    }
}

void butchery_quarter( item *corpse_item, player &p )
{
    corpse_item->set_flag( "QUARTERED" );
    p.add_msg_if_player( m_good,
                         _( "You roughly slice the corpse of %s into four parts and set them aside." ),
                         corpse_item->get_mtype()->nname().c_str() );
    for( int i = 1; i <= 3; i++ ) { // 4 quarters (one exists, add 3, flag does the rest)
        g->m.add_item_or_charges( p.pos(), *corpse_item, true );
    }
}

void activity_handlers::butcher_finish( player_activity *act, player *p )
{
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

    //Negative index means try to start next item
    if( act->index < 0 ) {
        //No values means no items left to try
        if( act->values.empty() ) {
            act->set_to_null();
            return;
        }
        set_up_butchery( *act, *p, action );
        return;
    }
    // Corpses can disappear (rezzing!), so check for that
    auto items_here = g->m.i_at( p->pos() );
    if( static_cast<int>( items_here.size() ) <= act->index ||
        !( items_here[act->index].is_corpse() ) ) {
        p->add_msg_if_player( m_info, _( "There's no corpse to butcher!" ) );
        act->set_to_null();
        return;
    }

    item &corpse_item = items_here[act->index];
    auto contents = corpse_item.contents;
    const mtype *corpse = corpse_item.get_mtype();
    time_point bday = corpse_item.birthday();
    const field_id type_blood = corpse->bloodType();
    const field_id type_gib = corpse->gibType();

    // corpse decays at 75% factor, but meat shares birthday and not relative_rot so this takes care of it
    // no FIELD_DRESS_FAILED here as it gets no benefit
    if( corpse_item.has_flag( "FIELD_DRESS" ) && !corpse_item.is_going_bad() ) {
        bday += corpse_item.age() * 3 / 4;
    }

    if( action == QUARTER ) {
        butchery_quarter( &corpse_item, *p );
        act->index = -1;
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
    }

    auto roll_butchery = [&]() {
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
        g->m.i_rem( p->pos(), act->index );
        g->m.add_splatter( type_gib, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
        g->m.add_splatter( type_blood, p->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
        for( int i = 1; i <= corpse->size; i++ ) {
            g->m.add_splatter_trail( type_gib, p->pos(), random_entry( g->m.points_in_radius( p->pos(),
                                     corpse->size + 1 ) ) );
            g->m.add_splatter_trail( type_blood, p->pos(), random_entry( g->m.points_in_radius( p->pos(),
                                     corpse->size + 1 ) ) );
        }
        act->index = -1;
        return;
    }
    // function just for drop yields
    auto roll_drops = [&]() {
        factor = std::max( factor, -50 );
        return 0.5 * skill_level / 10 + 0.3 * ( factor + 50 ) / 100 + 0.2 * p->dex_cur / 20;
    };
    // all action types - yields
    butchery_drops_harvest( &corpse_item, *corpse, *p, bday, roll_butchery, action, roll_drops );

    // reveal hidden items / hidden content
    if( action != F_DRESS && action != SKIN ) {
        for( auto &content : contents ) {
            if( ( roll_butchery() + 10 ) * 5 > rng( 0, 100 ) ) {
                //~ %1$s - item name, %2$s - monster name
                p->add_msg_if_player( m_good, _( "You discover a %1$s in the %2$s!" ), content.tname().c_str(),
                                      corpse->nname().c_str() );
                g->m.add_item_or_charges( p->pos(), content );
            } else if( content.is_bionic() ) {
                g->m.spawn_item( p->pos(), "burnt_out_bionic", 1, 0, bday );
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
                                  corpse_item.tname().c_str() );
            g->m.i_rem( p->pos(), act->index );
            break; //no set_to_null here, for multibutchering
        case BUTCHER_FULL:
            p->add_msg_if_player( m_good, _( "You finish butchering the %s." ), corpse_item.tname().c_str() );
            g->m.i_rem( p->pos(), act->index );
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
                        p->add_msg_if_player( m_good, _( "You field dress the %s." ), corpse->nname().c_str() );
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
            break;
        case SKIN:
            switch( rng( 1, 4 ) ) {
                case 1:
                    p->add_msg_if_player( m_good, _( "You skin the %s." ), corpse->nname().c_str() );
                    break;
                case 2:
                    p->add_msg_if_player( m_good, _( "You carefully remove the hide from the %s" ),
                                          corpse->nname().c_str() );
                    break;
                case 3:
                    p->add_msg_if_player( m_good,
                                          _( "The %s is challenging to skin, but you get a good hide from it." ),
                                          corpse->nname().c_str() );
                    break;
                case 4:
                    p->add_msg_if_player( m_good, _( "With a few deft slices you take the skin from the %s" ),
                                          corpse->nname().c_str() );
                    break;
            }
            corpse_item.set_flag( "SKINNED" );
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
            g->m.i_rem( p->pos(), act->index );
            break;
        case DISSECT:
            p->add_msg_if_player( m_good, _( "You finish dissecting the %s." ), corpse_item.tname().c_str() );
            g->m.i_rem( p->pos(), act->index );
            break;
    }
    // multibutchering
    act->index = -1;
}

enum liquid_source_type { LST_INFINITE_MAP = 1, LST_MAP_ITEM = 2, LST_VEHICLE = 3, LST_MONSTER = 4};

// All serialize_liquid_source functions should add the same number of elements to the vectors of
// the activity. This makes it easier to distinguish the values of the source and the values of the target.
void serialize_liquid_source( player_activity &act, const vehicle &veh, const int part_num,
                              const item &liquid )
{
    act.values.push_back( LST_VEHICLE );
    act.values.push_back( part_num );
    act.coords.push_back( veh.global_pos3() );
    act.str_values.push_back( serialize( liquid ) );
}

void serialize_liquid_source( player_activity &act, const monster &mon, const item &liquid )
{
    act.values.push_back( LST_MONSTER );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( mon.pos() );
    act.str_values.push_back( serialize( liquid ) );
}

void serialize_liquid_source( player_activity &act, const tripoint &pos, const item &liquid )
{
    const auto stack = g->m.i_at( pos );
    // Need to store the *index* of the item on the ground, but it may be a virtual item from
    // an infinite liquid source.
    const auto iter = std::find_if( stack.begin(), stack.end(), [&]( const item & i ) {
        return &i == &liquid;
    } );
    if( iter == stack.end() ) {
        act.values.push_back( LST_INFINITE_MAP );
        act.values.push_back( 0 ); // dummy
    } else {
        act.values.push_back( LST_MAP_ITEM );
        act.values.push_back( std::distance( stack.begin(), iter ) );
    }
    act.coords.push_back( pos );
    act.str_values.push_back( serialize( liquid ) );
}

enum liquid_target_type { LTT_CONTAINER = 1, LTT_VEHICLE = 2, LTT_MAP = 3, LTT_MONSTER = 4 };

void serialize_liquid_target( player_activity &act, const vehicle &veh )
{
    act.values.push_back( LTT_VEHICLE );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( veh.global_pos3() );
}

void serialize_liquid_target( player_activity &act, int container_item_pos )
{
    act.values.push_back( LTT_CONTAINER );
    act.values.push_back( container_item_pos );
    act.coords.push_back( tripoint() ); // dummy
}

void serialize_liquid_target( player_activity &act, const tripoint &pos )
{
    act.values.push_back( LTT_MAP );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( pos );
}

void serialize_liquid_target( player_activity &act, const monster &mon )
{
    act.values.push_back( LTT_MAP );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( mon.pos() );
}

void activity_handlers::fill_liquid_do_turn( player_activity *act, player *p )
{
    player_activity &act_ref = *act;
    try {
        // 1. Gather the source item.
        vehicle *source_veh = nullptr;
        const tripoint source_pos = act_ref.coords.at( 0 );
        map_stack source_stack = g->m.i_at( source_pos );
        std::list<item>::iterator on_ground;
        monster *source_mon = nullptr;
        item liquid;
        const auto source_type = static_cast<liquid_source_type>( act_ref.values.at( 0 ) );
        int part_num = -1;
        long veh_charges = 0;
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

        static const auto volume_per_turn = units::from_liter( 4 );
        const long charges_per_turn = std::max( 1l, liquid.charges_per_volume( volume_per_turn ) );
        liquid.charges = std::min( charges_per_turn, liquid.charges );
        const long original_charges = liquid.charges;

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
                    p->add_msg_if_player( _( "You pour %1$s onto the ground." ), liquid.tname().c_str() );
                    liquid.charges = 0;
                }
                break;
            case LTT_MONSTER:
                liquid.charges = 0;
                break;
        }

        const long removed_charges = original_charges - liquid.charges;
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
                                 liquid.type_name( 1 ).c_str() );
                    } else if( g->m.furn( source_pos ).obj().examine == &iexamine::fvat_full ) {
                        g->m.furn_set( source_pos, f_fvat_empty );
                        add_msg( _( "You squeeze the last drops of %s from the vat." ),
                                 liquid.type_name( 1 ).c_str() );
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
    hp_part healed = static_cast<hp_part>( act->values[0] );
    long charges_consumed = actor->finish_using( *p, patient, *used_tool, healed );
    p->consume_charges( it, charges_consumed );

    // Erase activity and values.
    act->set_to_null();
    act->values.clear();
}

// fish-with-rod fish catching function.
static void rod_fish( player *p, int sSkillLevel, int fishChance, const tripoint &fish_point )
{
    if( sSkillLevel > fishChance ) {
        std::vector<monster *> fishables = g->get_fishable( 60, fish_point ); //get the nearby fish list.
        //if the vector is empty (no fish around) the player is still given a small chance to get a (let us say it was hidden) fish
        if( fishables.empty() ) {
            if( one_in( 20 ) ) {
                item fish;
                const std::vector<mtype_id> fish_group = MonsterGroupManager::GetMonstersFromGroup(
                            mongroup_id( "GROUP_FISH" ) );
                const mtype_id &fish_mon = random_entry_ref( fish_group );
                g->m.add_item_or_charges( p->pos(), item::make_corpse( fish_mon ) );
                p->add_msg_if_player( m_good, _( "You caught a %s." ), fish_mon.obj().nname().c_str() );
            } else {
                p->add_msg_if_player( _( "You didn't catch anything." ) );
            }
        } else {
            g->catch_a_monster( fishables, p->pos(), p, 50_hours );
        }

    } else {
        p->add_msg_if_player( _( "You didn't catch anything." ) );
    }
}

void activity_handlers::fish_finish( player_activity *act, player *p )
{
    item &it = p->i_at( act->position );
    int sSkillLevel = 0;
    int fishChance = 20;
    if( it.has_flag( "FISH_POOR" ) ) {
        sSkillLevel = p->get_skill_level( skill_survival ) + dice( 1, 6 );
        fishChance = dice( 1, 20 );
    } else if( it.has_flag( "FISH_GOOD" ) ) {
        // Much better chances with a good fishing implement.
        sSkillLevel = p->get_skill_level( skill_survival ) * 1.5 + dice( 1, 6 ) + 3;
        fishChance = dice( 1, 20 );
    }
    const tripoint fish_pos = act->placement;
    ///\EFFECT_SURVIVAL increases chance of fishing success
    rod_fish( p, sSkillLevel, fishChance, fish_pos );
    p->practice( skill_survival, rng( 5, 15 ) );
    act->set_to_null();
}

void activity_handlers::forage_finish( player_activity *act, player *p )
{
    int veggy_chance = rng( 1, 100 );
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
    }

    g->m.ter_set( act->placement, next_ter );

    // Survival gives a bigger boost, and Perception is leveled a bit.
    // Both survival and perception affect time to forage
    ///\EFFECT_SURVIVAL increases forage success chance

    ///\EFFECT_PER slightly increases forage success chance
    if( veggy_chance < p->get_skill_level( skill_survival ) * 3 + p->per_cur - 2 ) {
        const auto dropped = g->m.put_items_from_loc( loc, p->pos(), calendar::turn );
        for( const auto &it : dropped ) {
            add_msg( m_good, _( "You found: %s!" ), it->tname().c_str() );
            found_something = true;
        }
    }
    // 10% to drop a item/items from this group.
    if( one_in( 10 ) ) {
        const auto dropped = g->m.put_items_from_loc( "trash_forest", p->pos(), calendar::turn );
        for( const auto &it : dropped ) {
            add_msg( m_good, _( "You found: %s!" ), it->tname().c_str() );
            found_something = true;
            if( it->typeId() == "mushroom" ) {
                if( one_in( 10 ) ) {
                    it->item_tags.insert( "HIDDEN_POISON" );
                    it->poison = rng( 2, 7 );
                    break;
                } else if( one_in( 10 ) ) {
                    it->item_tags.insert( "HIDDEN_HALLU" );
                    break;
                }
            }
        }
    }

    if( !found_something ) {
        add_msg( _( "You didn't find anything." ) );
    }

    ///\EFFECT_INT Intelligence caps survival skill gains from foraging
    const int max_forage_skill = p->int_cur / 3 + 1;
    ///\EFFECT_SURVIVAL decreases survival skill gain from foraging (NEGATIVE)
    const int max_exp = 2 * ( max_forage_skill - p->get_skill_level( skill_survival ) );
    // Award experience for foraging attempt regardless of success
    p->practice( skill_survival, rng( 1, max_exp ), max_forage_skill );

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
        add_msg( m_info, _( "The %s runs out of batteries." ), game_item.tname().c_str() );
    }
}

void activity_handlers::hotwire_finish( player_activity *act, player *p )
{
    //Grab this now, in case the vehicle gets shifted
    if( const optional_vpart_position vp = g->m.veh_at( tripoint( act->values[0], act->values[1],
                                           p->posz() ) ) ) {
        vehicle *const veh = &vp->vehicle();
        int mech_skill = act->values[2];
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
    std::string corpse_name = act->str_values[0];
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

    } else {

        if( success > -20 ) {

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
}

void activity_handlers::pickaxe_do_turn( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    if( calendar::once_every( 1_minutes ) ) { // each turn is too much
        //~ Sound of a Pickaxe at work!
        sounds::sound( pos, 30, sounds::sound_t::combat, _( "CHNK! CHNK! CHNK!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::pickaxe_finish( player_activity *act, player *p )
{
    const tripoint pos( act->placement );
    item &it = p->i_at( act->position );
    act->set_to_null(); // Invalidate the activity early to prevent a query from mod_pain()
    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    if( g->m.is_bashable( pos ) && g->m.has_flag( "SUPPORTS_ROOF", pos ) &&
        g->m.ter( pos ) != t_tree ) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Betcha wish you'd opted for the J-Hammer ;P
        p->mod_hunger( 15 - ( helpersize  * 3 ) );
        p->mod_thirst( 15 - ( helpersize  * 3 ) );
        if( p->has_trait( trait_id( "STOCKY_TROGLO" ) ) ) {
            p->mod_fatigue( 20 - ( helpersize  * 3 ) ); // Yep, dwarves can dig longer before tiring
        } else {
            p->mod_fatigue( 30 - ( helpersize  * 3 ) );
        }
        p->mod_pain( std::max( 0, ( 2 * static_cast<int>( rng( 1, 3 ) ) ) - helpersize ) );
    } else if( g->m.move_cost( pos ) == 2 && g->get_levz() == 0 &&
               g->m.ter( pos ) != t_dirt && g->m.ter( pos ) != t_grass ) {
        //Breaking up concrete on the surface? not nearly as bad
        p->mod_hunger( 5 - ( helpersize ) );
        p->mod_thirst( 5 - ( helpersize ) );
        p->mod_fatigue( 10 - ( helpersize  * 2 ) );
    }
    p->add_msg_if_player( m_good, _( "You finish digging." ) );
    g->m.destroy( pos, true );
    it.charges = std::max( long( 0 ), it.charges - it.type->charges_to_use() );
    if( it.charges == 0 && it.destroyed_at_zero_charges() ) {
        p->i_rem( &it );
    }
}

void activity_handlers::pulp_do_turn( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;

    // Stabbing weapons are a lot less effective at pulping
    int cut_power = std::max( p->weapon.damage_melee( DT_CUT ), p->weapon.damage_melee( DT_STAB ) / 2 );

    ///\EFFECT_STR increases pulping power, with diminishing returns
    float pulp_power = sqrt( ( p->str_cur + p->weapon.damage_melee( DT_BASH ) ) *
                             ( cut_power + 1.0f ) );
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
              act->str_values.end() && corpse_mtype->bloodType() == fd_acid ) ) {
            // Don't smash non-rezing corpses //don't smash acid zombies when auto pulping
            continue;
        }

        if( corpse.damage() >= corpse.max_damage() ) {
            // Deactivate already-pulped corpses that weren't properly deactivated
            corpse.active = false;
            continue;
        }

        while( corpse.damage() < corpse.max_damage() ) {
            // Increase damage as we keep smashing ensuring we eventually smash the target.
            if( x_in_y( pulp_power, corpse.volume() / units::legacy_volume_factor ) ) {
                corpse.inc_damage( DT_BASH );
                if( corpse.damage() == corpse.max_damage() ) {
                    corpse.active = false;
                    num_corpses++;
                }
            }

            if( x_in_y( pulp_power, corpse.volume() /
                        units::legacy_volume_factor ) ) { // Splatter some blood around
                // Splatter a bit more randomly, so that it looks cooler
                const int radius = mess_radius + x_in_y( pulp_power, 500 ) + x_in_y( pulp_power, 1000 );
                const tripoint dest( pos.x + rng( -radius, radius ), pos.y + rng( -radius, radius ), pos.z );
                const field_id type_blood = ( mess_radius > 1 && x_in_y( pulp_power, 10000 ) ) ?
                                            corpse.get_mtype()->gibType() :
                                            corpse.get_mtype()->bloodType();
                g->m.add_splatter_trail( type_blood, pos, dest );
            }

            float stamina_ratio = static_cast<float>( p->stamina ) / p->get_stamina_max();
            p->mod_stat( "stamina", stamina_ratio * -40 );

            moves += 100 / std::max( 0.25f, stamina_ratio );
            if( one_in( 4 ) ) {
                // Smashing may not be butchery, but it involves some zombie anatomy
                p->practice( skill_survival, 2, 2 );
            }

            if( moves >= p->moves ) {
                // Enough for this turn;
                p->moves -= moves;
                return;
            }
        }
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
    int qty = act->index;
    bool is_speedloader = act->targets[ 1 ]->has_flag( "SPEEDLOADER" );

    if( !reloadable.reload( *p, std::move( act->targets[ 1 ] ), qty ) ) {
        add_msg( m_info, _( "Can't reload the %s." ), reloadable.tname().c_str() );
        return;
    }

    std::string msg = _( "You reload the %s." );

    if( reloadable.is_gun() ) {
        p->recoil = MAX_RECOIL;

        if( reloadable.has_flag( "RELOAD_ONE" ) && !is_speedloader ) {
            for( int i = 0; i != qty; ++i ) {
                if( reloadable.ammo_type() == ammotype( "bolt" ) ) {
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
    add_msg( msg.c_str(), reloadable.tname().c_str() );
}

void activity_handlers::start_fire_finish( player_activity *act, player *p )
{
    firestarter_actor::resolve_firestarter_use( *p, act->placement );
    act->set_to_null();
}

void activity_handlers::start_fire_do_turn( player_activity *act, player *p )
{
    item &lens_item = p->i_at( act->position );
    const auto usef = lens_item.type->get_use( "firestarter" );
    if( usef == nullptr || usef->get_actor_ptr() == nullptr ) {
        add_msg( m_bad, _( "You have lost the item you were using to start the fire." ) );
        p->cancel_activity();
        return;
    }

    p->mod_moves( -p->moves );
    const auto actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
    float light = actor->light_mod( p->pos() );
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
        int new_skill_level = p->get_skill_level( sk ) + 1;
        p->set_skill_level( sk, new_skill_level );
        add_msg( m_good, _( "You finish training %s to level %d." ), skill_name, new_skill_level );
        if( new_skill_level % 4 == 0 ) {
            //~ %d is skill level %s is skill name
            p->add_memorial_log( pgettext( "memorial_male", "Reached skill level %1$d in %2$s." ),
                                 pgettext( "memorial_female", "Reached skill level %1$d in %2$s." ),
                                 new_skill_level, skill_name );
        }
        const std::string skill_increase_source = "training";
        CallbackArgumentContainer lua_callback_args_info;
        lua_callback_args_info.emplace_back( p->getID() );
        lua_callback_args_info.emplace_back( skill_increase_source );
        lua_callback_args_info.emplace_back( sk.str() );
        lua_callback_args_info.emplace_back( new_skill_level );
        lua_callback( "on_player_skill_increased", lua_callback_args_info );
        lua_callback( "on_skill_increased" ); // Legacy callback
        act->set_to_null();
        return;
    }

    const auto &ma_id = matype_id( act->name );
    if( ma_id.is_valid() ) {
        const auto &mastyle = ma_id.obj();
        // Trained martial arts,
        add_msg( m_good, _( "You learn %s." ), mastyle.name.c_str() );
        //~ %s is martial art
        p->add_memorial_log( pgettext( "memorial_male", "Learned %s." ),
                             pgettext( "memorial_female", "Learned %s." ),
                             mastyle.name.c_str() );
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
    const optional_vpart_position vp = g->m.veh_at( tripoint( act->values[0], act->values[1],
                                       p->posz() ) );
    veh_interact::complete_vehicle();
    // complete_vehicle set activity type to NULL if the vehicle
    // was completely dismantled, otherwise the vehicle still exist and
    // is to be examined again.
    if( act->is_null() ) {
        return;
    }
    act->set_to_null();
    if( act->values.size() < 7 ) {
        dbg( D_ERROR ) << "game:process_activity: invalid ACT_VEHICLE values: "
                       << act->values.size();
        debugmsg( "process_activity invalid ACT_VEHICLE values:%d",
                  act->values.size() );
    } else {
        if( vp ) {
            g->refresh_all();
            // TODO: Z (and also where the activity is queued)
            // Or not, because the vehicle coordinates are dropped anyway
            g->exam_vehicle( vp->vehicle(), act->values[ 2 ], act->values[ 3 ] );
            return;
        } else {
            dbg( D_ERROR ) << "game:process_activity: ACT_VEHICLE: vehicle not found";
            debugmsg( "process_activity ACT_VEHICLE: vehicle not found" );
        }
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
                add_msg( m_info, _( "The %s runs out of batteries." ), vibrator_item.tname().c_str() );
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
    const bool take_control = act->values[0];

    for( size_t e = 0; e < veh->engines.size(); ++e ) {
        if( veh->is_engine_on( e ) ) {
            attempted++;
            if( !veh->is_engine_type( e, "muscle" ) ) {
                non_muscle_attempted++;
            }
            if( veh->start_engine( e ) ) {
                started++;
                if( !veh->is_engine_type( e, "muscle" ) ) {
                    non_muscle_started++;
                }
            }
        }
    }

    //Did any engines start?
    veh->engine_on = started;

    if( attempted == 0 ) {
        add_msg( m_info, _( "The %s doesn't have an engine!" ), veh->name.c_str() );
    } else if( non_muscle_attempted > 0 ) {
        //Some non-muscle engines tried to start
        if( non_muscle_attempted == non_muscle_started ) {
            //All of the non-muscle engines started
            add_msg( ngettext( "The %s's engine starts up.",
                               "The %s's engines start up.", non_muscle_started ), veh->name.c_str() );
        } else if( non_muscle_started > 0 ) {
            //Only some of the non-muscle engines started
            add_msg( ngettext( "One of the %s's engines start up.",
                               "Some of the %s's engines start up.", non_muscle_started ), veh->name.c_str() );
        } else {
            //All of the non-muscle engines failed
            add_msg( m_bad, ngettext( "The %s's engine fails to start.",
                                      "The %s's engines fail to start.", non_muscle_attempted ), veh->name.c_str() );
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
    const long charges_used = std::min( long( act->values[0] ), it.ammo_required() );

    it.ammo_consume( charges_used, p->pos() );
    act->values[0] -= int( charges_used );

    if( calendar::once_every( 2_turns ) ) {
        sounds::sound( act->placement, 10, sounds::sound_t::combat, _( "hissssssssss!" ) );
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
        if( g->m.ter( {pos.x + 1, pos.y, pos.z} ) == t_sewage || g->m.ter( {pos.x, pos.y + 1, pos.z} ) ==
            t_sewage ||
            g->m.ter( {pos.x - 1, pos.y, pos.z} ) == t_sewage || g->m.ter( {pos.x, pos.y - 1, pos.z} ) ==
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

repeat_type repeat_menu( const std::string &title, repeat_type last_selection )
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
    if( repeat != REPEAT_INIT && act->position != INT_MIN ) {
        item &fix = p->i_at( act->position );

        // Remember our level: we want to stop retrying on level up
        const int old_level = p->get_skill_level( actor->used_skill );
        const auto attempt = actor->repair( *p, *used_tool, fix );
        if( attempt != repair_item_actor::AS_CANT ) {
            if( ploc && ploc->where() == item_location::type::map ) {
                used_tool->ammo_consume( used_tool->ammo_required(), ploc->position() );
            } else {
                p->consume_charges( *used_tool, used_tool->ammo_required() );
            }
        }

        // TODO: Allow setting this in the actor
        // TODO: Don't use charges_to_use: welder has 50 charges per use, soldering iron has 1
        if( !used_tool->ammo_sufficient() ) {
            p->add_msg_if_player( _( "Your %s ran out of charges" ), used_tool->tname() );
            act->set_to_null();
            return;
        }

        // Print message explaining why we stopped
        // But only if we didn't destroy the item (because then it's obvious)
        const bool destroyed = attempt == repair_item_actor::AS_DESTROYED;
        if( attempt == repair_item_actor::AS_CANT ||
            destroyed ||
            !actor->can_repair_target( *p, fix, !destroyed ) ) {
            // Cannot continue to repair target, select another target.
            act->position = INT_MIN;
        }

        bool event_happened =
            attempt == repair_item_actor::AS_FAILURE ||
            attempt == repair_item_actor::AS_SUCCESS ||
            old_level != p->get_skill_level( actor->used_skill );

        const bool need_input =
            repeat == REPEAT_ONCE ||
            ( repeat == REPEAT_EVENT && event_happened ) ||
            ( repeat == REPEAT_FULL && fix.damage() <= 0 );
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
    while( act->position == INT_MIN ) {
        g->draw_sidebar_messages();     // Refresh messages to show feedback.
        const int pos = g->inv_for_filter( _( "Repair what?" ), [&actor, &main_tool]( const item & itm ) {
            return itm.made_of_any( actor->materials ) && !itm.count_by_charges() && !itm.is_firearm() &&
                   &itm != &main_tool;
        }, string_format( _( "You have no items that could be repaired with a %s." ),
                          main_tool.type_name( 1 ) ) );

        if( pos == INT_MIN ) {
            p->add_msg_if_player( m_info, _( "Never mind." ) );
            act->set_to_null();
            return;
        }
        if( actor->can_repair_target( *p, p->i_at( pos ), true ) ) {
            act->position = pos;
            repeat = REPEAT_INIT;
        }
    }

    const item &fix = p->i_at( act->position );

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
            g->draw_sidebar_messages();
            repeat = repeat_menu( title, repeat );

            if( repeat == REPEAT_CANCEL ) {
                act->set_to_null();
                return;
            }
            act->values[0] = static_cast<int>( repeat );
            if( repeat == REPEAT_INIT ) {       // BACK selected, redo target selection next.
                p->activity.position = INT_MIN;
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

    auto f = target->faults.find( fault_id( act->name ) );
    if( f == target->faults.end() ) {
        debugmsg( "item %s does not have fault %s", target->tname().c_str(), act->name.c_str() );
        return;
    }

    auto inv = p->crafting_inventory();
    const auto &reqs = f->obj().requirements();
    if( !reqs.can_make_with_inventory( inv ) ) {
        add_msg( m_info, _( "You are currently unable to mend the %s." ), target->tname().c_str() );
    }
    for( const auto &e : reqs.get_components() ) {
        p->consume_items( e );
    }
    for( const auto &e : reqs.get_tools() ) {
        p->consume_tools( e );
    }
    p->invalidate_crafting_inventory();

    target->faults.erase( *f );
    add_msg( m_good, _( "You successfully mended the %s." ), target->tname().c_str() );
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

    int roll = act->values[1]; // chance of success (%)
    int risk = act->values[2]; // chance of damage (%)

    // any tool charges used during installation
    std::string tool = act->name;
    int qty = act->values[3];

    if( !gun.is_gunmod_compatible( mod ).success() ) {
        debugmsg( "Invalid arguments in ACT_GUNMOD_ADD" );
        return;
    }

    if( !tool.empty() && qty > 0 ) {
        p->use_charges( tool, qty );
    }

    if( rng( 0, 100 ) <= roll ) {
        add_msg( m_good, _( "You successfully attached the %1$s to your %2$s." ), mod.tname().c_str(),
                 gun.tname().c_str() );
        gun.contents.push_back( p->i_rem( &mod ) );

    } else if( rng( 0, 100 ) <= risk ) {
        if( gun.inc_damage() ) {
            // Remove irremovable mods prior to destroying the gun
            for( auto mod : gun.gunmods() ) {
                if( mod->is_irremovable() ) {
                    p->remove_item( *mod );
                }
            }
            add_msg( m_bad, _( "You failed at installing the %s and destroyed your %s!" ), mod.tname().c_str(),
                     gun.tname().c_str() );
            p->i_rem( &gun );
        } else {
            add_msg( m_bad, _( "You failed at installing the %s and damaged your %s!" ), mod.tname().c_str(),
                     gun.tname().c_str() );
        }

    } else {
        add_msg( m_info, _( "You failed at installing the %s." ), mod.tname().c_str() );
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
                          mod.tname().c_str(),
                          tool.tname().c_str() );
    mod.item_tags.insert( "IRREMOVABLE" );
    tool.contents.push_back( mod );
    act->targets[1].remove_item();
}

void activity_handlers::clear_rubble_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    p->add_msg_if_player( m_info, _( "You clear up the %s." ),
                          g->m.furnname( pos ).c_str() );
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
        g->m.build_map_cache( g->get_levz() );
        g->plfire();
    }
}

void activity_handlers::pickup_do_turn( player_activity *, player * )
{
    activity_on_turn_pickup();
}

void activity_handlers::wear_do_turn( player_activity *, player * )
{
    activity_on_turn_wear();
}

void activity_handlers::move_items_do_turn( player_activity *, player * )
{
    activity_on_turn_move_items();
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

void activity_handlers::armor_layers_do_turn( player_activity *, player *p )
{
    p->cancel_activity();
    p->sort_armor();
}

void activity_handlers::atm_do_turn( player_activity *, player *p )
{
    iexamine::atm( *p, p->pos() );
}

void activity_handlers::cracking_do_turn( player_activity *act, player *p )
{
    if( !( p->has_amount( "stethoscope", 1 ) || p->has_bionic( bionic_id( "bio_ears" ) ) ) ) {
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

void activity_handlers::butcher_do_turn( player_activity *, player *p )
{
    p->mod_stat( "stamina", -20.0f * p->stamina / p->get_stamina_max() );
}

void activity_handlers::read_finish( player_activity *act, player *p )
{
    p->do_read( *act->targets.front().get_item() );
    if( !act ) {
        p->add_msg_if_player( m_info, _( "You finish reading." ) );
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

void activity_handlers::wait_npc_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "%s finishes with you..." ), act->str_values[0].c_str() );
    act->set_to_null();
}

void activity_handlers::socialize_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( _( "%s finishes chatting with you." ), act->str_values[0].c_str() );
    act->set_to_null();
}

void activity_handlers::try_sleep_do_turn( player_activity *act, player *p )
{
    if( !p->has_effect( effect_sleep ) ) {
        if( p->can_sleep() ) {
            act->set_to_null();
            p->fall_asleep();
        } else if( one_in( 1000 ) ) {
            p->add_msg_if_player( _( "You toss and turn..." ) );
        }
    }
}

void activity_handlers::try_sleep_finish( player_activity *act, player *p )
{
    if( !p->has_effect( effect_sleep ) ) {
        p->add_msg_if_player( _( "You try to sleep, but can't..." ) );
    }
    act->set_to_null();
}

void activity_handlers::craft_do_turn( player_activity *act, player *p )
{
    const recipe &rec = recipe_id( act->name ).obj();
    float crafting_speed = p->crafting_speed_multiplier( rec, true );
    if( crafting_speed <= 0.0f ) {
        if( p->lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
            p->add_msg_if_player( m_bad, _( "You can no longer see well enough to keep crafting." ) );
        } else {
            p->add_msg_if_player( m_bad, _( "You are too frustrated to continue and just give up." ) );
        }
        p->cancel_activity();
        return;
    }
    act->moves_left -= crafting_speed * p->get_moves();
    p->set_moves( 0 );
    if( calendar::once_every( 1_hours ) && crafting_speed < 0.75f ) {
        // @todo Describe the causes of slowdown
        p->add_msg_if_player( m_bad, _( "You can't focus and are working slowly." ) );
    }
}

void activity_handlers::craft_finish( player_activity *act, player *p )
{
    p->complete_craft();
    act->set_to_null();
}

void activity_handlers::longcraft_finish( player_activity *act, player *p )
{
    int batch_size = act->values.front();
    p->complete_craft();
    act->set_to_null();
    // Workaround for a bug where longcraft can be unset in complete_craft().
    if( p->making_would_work( p->lastrecipe, batch_size ) ) {
        p->last_craft->execute();
    }
}

void activity_handlers::disassemble_finish( player_activity *, player *p )
{
    p->complete_disassemble();
}

void activity_handlers::build_finish( player_activity *, player * )
{
    complete_construction();
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

void activity_handlers::hacksaw_do_turn( player_activity *act, player *p )
{
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a metal sawing tool at work!
        sounds::sound( act->placement, 15, sounds::sound_t::combat, _( "grnd grnd grnd" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::hacksaw_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    const ter_id ter = g->m.ter( pos );

    if( g->m.furn( pos ) == f_rack ) {
        g->m.furn_set( pos, f_null );
        g->m.spawn_item( pos, "pipe", rng( 1, 3 ) );
        g->m.spawn_item( pos, "steel_chunk" );
    } else if( ter == t_chainfence || ter == t_chaingate_c || ter == t_chaingate_l ) {
        g->m.ter_set( pos, t_dirt );
        g->m.spawn_item( pos, "pipe", 6 );
        g->m.spawn_item( pos, "wire", 20 );
    } else if( ter == t_chainfence_posts ) {
        g->m.ter_set( pos, t_dirt );
        g->m.spawn_item( pos, "pipe", 6 );
    } else if( ter == t_window_bars_alarm ) {
        g->m.ter_set( pos, t_window_alarm );
        g->m.spawn_item( pos, "pipe", 6 );
    } else if( ter == t_window_bars ) {
        g->m.ter_set( pos, t_window_empty );
        g->m.spawn_item( pos, "pipe", 6 );
    } else if( ter == t_window_enhanced ) {
        g->m.ter_set( pos, t_window_reinforced );
        g->m.spawn_item( pos, "spike", rng( 1, 4 ) );
    } else if( ter == t_window_enhanced_noglass ) {
        g->m.ter_set( pos, t_window_reinforced_noglass );
        g->m.spawn_item( pos, "spike", rng( 1, 4 ) );
    } else if( ter == t_reb_cage ) {
        g->m.ter_set( pos, t_pit );
        g->m.spawn_item( pos, "spike", 19 );
        g->m.spawn_item( pos, "scrap", 8 );
    } else if( ter == t_bars ) {
        if( g->m.ter( { pos.x + 1, pos.y, pos.z } ) == t_sewage || g->m.ter( { pos.x, pos.y + 1, pos.z } )
            == t_sewage ||
            g->m.ter( { pos.x - 1, pos.y, pos.z } ) == t_sewage || g->m.ter( { pos.x, pos.y - 1, pos.z } ) ==
            t_sewage ) {
            g->m.ter_set( pos, t_sewage );
            g->m.spawn_item( pos, "pipe", 3 );
        } else {
            g->m.ter_set( pos, t_floor );
            g->m.spawn_item( pos, "pipe", 3 );
        }
    } else if( ter == t_door_bar_c || ter == t_door_bar_locked ) {
        g->m.ter_set( pos, t_mdoor_frame );
        g->m.spawn_item( pos, "pipe", 12 );
    }

    p->mod_hunger( 5 );
    p->mod_thirst( 5 );
    p->mod_fatigue( 10 );
    p->add_msg_if_player( m_good, _( "You finish cutting the metal." ) );

    act->set_to_null();
}

void activity_handlers::chop_tree_do_turn( player_activity *act, player *p )
{
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a wood chopping tool at work!
        sounds::sound( act->placement, 15, sounds::sound_t::combat, _( "CHK!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::chop_tree_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;

    tripoint direction;
    while( true ) {
        if( const cata::optional<tripoint> dir = choose_direction(
                    _( "Select a direction for the tree to fall in." ) ) ) {
            direction = *dir;
            break;
        }
        // try again
    }

    tripoint to = pos + point( 3 * direction.x + rng( -1, 1 ), 3 * direction.y + rng( -1, 1 ) );
    std::vector<tripoint> tree = line_to( pos, to, rng( 1, 8 ) );
    for( auto &elem : tree ) {
        g->m.destroy( elem );
        g->m.ter_set( elem, t_trunk );
    }

    g->m.ter_set( pos, t_stump );
    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    p->mod_hunger( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish chopping down a tree." ) );

    act->set_to_null();
}

void activity_handlers::chop_logs_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;

    if( g->m.ter( pos ) == t_trunk ) {
        g->m.spawn_item( pos.x, pos.y, "log", rng( 2, 3 ), 0, calendar::turn );
        g->m.spawn_item( pos.x, pos.y, "stick_long", rng( 0, 1 ), 0, calendar::turn );
    } else if( g->m.ter( pos ) == t_stump ) {
        g->m.spawn_item( pos.x, pos.y, "log", rng( 0, 2 ), 0, calendar::turn );
        g->m.spawn_item( pos.x, pos.y, "splinter", rng( 5, 15 ), 0, calendar::turn );
    }

    g->m.ter_set( pos, t_dirt );
    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    p->mod_hunger( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish chopping wood." ) );

    act->set_to_null();
}

void activity_handlers::jackhammer_do_turn( player_activity *act, player *p )
{
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a jackhammer at work!
        sounds::sound( act->placement, 15, sounds::sound_t::combat, _( "TATATATATATATAT!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::jackhammer_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;

    g->m.destroy( pos, true );

    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    p->mod_hunger( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish drilling." ) );

    act->set_to_null();
}

void activity_handlers::dig_do_turn( player_activity *act, player *p )
{
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel digging a pit at work!
        sounds::sound( act->placement, 10, sounds::sound_t::combat, _( "hsh!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::dig_finish( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;

    if( g->m.ter( pos ) == t_pit_shallow ) {
        g->m.ter_set( pos, t_pit );
    } else {
        g->m.ter_set( pos, t_pit_shallow );
    }

    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    p->mod_hunger( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish digging up %s." ), g->m.ter( pos ).obj().name() );

    act->set_to_null();
}

void activity_handlers::fill_pit_do_turn( player_activity *act, player *p )
{
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel filling a pit or mound at work!
        sounds::sound( act->placement, 10, sounds::sound_t::combat, _( "hsh!" ) );
        messages_in_process( *act, *p );
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
    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    const int helpersize = g->u.get_num_crafting_helpers( 3 );
    p->mod_hunger( 5 - helpersize );
    p->mod_thirst( 5 - helpersize );
    p->mod_fatigue( 10 - ( helpersize * 2 ) );
    p->add_msg_if_player( m_good, _( "You finish filling up %s." ), old_ter.obj().name() );

    act->set_to_null();
}

void activity_handlers::play_with_pet_finish( player_activity *act, player *p )
{
    p->add_morale( MORALE_PLAY_WITH_PET, rng( 3, 10 ), 10, 5_hours, 25_minutes );
    p->add_msg_if_player( m_good, _( "Playing with your %s has lifted your spirits a bit." ),
                          act->str_values[0].c_str() );
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
    long qty = 0;
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
        add_msg( _( "You disassemble your %s." ), it.tname().c_str() );
    } else {
        add_msg( _( "You unload your %s." ), it.tname().c_str() );
    }

    if( it.has_flag( "MAG_DESTROY" ) && it.ammo_remaining() == 0 ) {
        act->targets[ 0 ].remove_item();
    }

    act->set_to_null();
}

std::vector<tripoint> get_sorted_tiles_by_distance( const tripoint &abspos,
        const std::unordered_set<tripoint> &tiles )
{
    auto cmp = [abspos]( tripoint a, tripoint b ) {
        int da = rl_dist( abspos, a );
        int db = rl_dist( abspos, b );

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
    auto reject_tile = [p]( const tripoint & tile ) {
        return !p->sees( tile ) || g->m.furn( tile ) != f_plant_harvest;
    };
    perform_zone_activity_turn( p,
                                zone_type_id( "FARM_PLOT" ),
                                reject_tile,
                                iexamine::harvest_plant,
                                _( "You harvested all the plots you could." ) );

}

void activity_handlers::till_plot_do_turn( player_activity *, player *p )
{
    auto reject_tile = [p]( const tripoint & tile ) {
        return !p->sees( tile ) || !g->m.has_flag( "PLOWABLE", tile ) || g->m.has_flag( "PLANT", tile ) ||
               g->m.ter( tile ) == t_dirtmound;
    };

    auto dig = []( player & p, const tripoint & tile_loc ) {
        p.add_msg_if_player( _( "You churn up the earth here." ) );
        p.mod_moves( -300 );
        g->m.ter_set( tile_loc, t_dirtmound );
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


    auto reject_tile = [&]( const tripoint & tile ) {
        check_fertilizer();
        std::string failure = iexamine::fertilize_failure_reason( *p, tile, fertilizer );
        return !p->sees( tile ) || !failure.empty();
    };

    auto fertilize = [&]( player & p, const tripoint & tile ) {
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
    auto reject_tiles = [&]( const tripoint & tile ) {
        if( !p->sees( tile ) || g->m.ter( tile ) != t_dirtmound || !g->m.i_at( tile ).empty() ) {
            return true;
        }

        const auto seeds = get_seeds( tile );

        return std::all_of( seeds.begin(), seeds.end(), [&]( std::string seed ) {
            return std::all_of( seed_inv.begin(), seed_inv.end(), [seed]( item * it ) {
                return it->typeId() != itype_id( seed );
            } );
        } );
    };

    auto plant_appropriate_seed = [&]( player & p, const tripoint & tile_loc ) {
        const auto seeds = get_seeds( tile_loc );
        std::vector<item *> seed_inv = p.items_with( [seeds]( const item & itm ) {
            return itm.is_seed() && std::any_of( seeds.begin(), seeds.end(), [itm]( std::string seed ) {
                return itm.typeId() == itype_id( seed );
            } );
        } );
        if( !seed_inv.empty() ) {
            auto it = seed_inv.front();
            iexamine::plant_seed( p, tile_loc, it->typeId() );
        }
    };


    perform_zone_activity_turn( p,
                                zone_type_id( "FARM_PLOT" ),
                                reject_tiles,
                                plant_appropriate_seed,
                                _( "You planted all seeds you could." ) );
}

void activity_handlers::robot_control_finish( player_activity *act, player *p )
{
    uilist pick_robot;
    pick_robot.text = _( "Override ready, choose an endpoint to hack." );
    // Build a list of all unfriendly robots in range.
    std::vector< monster * > mons; // @todo: change into vector<Creature*>
    std::vector< tripoint > locations;
    int entry_num = 0;
    for( monster &candidate : g->all_monsters() ) {
        if( candidate.type->in_species( species_id( "ROBOT" ) ) && candidate.friendly == 0 &&
            rl_dist( p->pos(), candidate.pos() ) <= 10 ) {
            mons.push_back( &candidate );
            pick_robot.addentry( entry_num++, true, MENU_AUTOASSIGN, candidate.name() );
            tripoint seen_loc;
            // Show locations of seen robots, center on player if robot is not seen
            if( p->sees( candidate ) ) {
                seen_loc = candidate.pos();
            } else {
                seen_loc = p->pos();
            }
            locations.push_back( seen_loc );
        }
    }
    if( mons.empty() ) {
        p->add_msg_if_player( m_info, _( "No enemy robots in range." ) );
        act->set_to_null();
        return;
    }
    pointmenu_cb callback( locations );
    pick_robot.callback = &callback;
    pick_robot.query();
    if( pick_robot.ret < 0 || static_cast<size_t>( pick_robot.ret ) >= mons.size() ) {
        p->add_msg_if_player( m_info, _( "Never mind" ) );
        act->set_to_null();
        return;
    }
    const size_t mondex = pick_robot.ret;
    monster *z = mons[mondex];
    p->add_msg_if_player( _( "You unleash your override attack on the %s." ), z->name().c_str() );

    /** @EFFECT_INT increases chance of successful robot reprogramming, vs difficulty */
    /** @EFFECT_COMPUTER increases chance of successful robot reprogramming, vs difficulty */
    float success = p->get_skill_level( skill_id( "computer" ) ) - 1.5 * ( z->type->difficulty ) /
                    ( ( rng( 2, p->int_cur ) / 2 ) + ( p->get_skill_level( skill_id( "computer" ) ) / 2 ) );
    if( success >= 0 ) {
        p->add_msg_if_player( _( "You successfully override the %s's IFF protocols!" ),
                              z->name().c_str() );
        z->friendly = -1;
    } else if( success >= -2 ) { //A near success
        p->add_msg_if_player( _( "The %s short circuits as you attempt to reprogram it!" ),
                              z->name().c_str() );
        z->apply_damage( p, bp_torso, rng( 1, 10 ) ); //damage it a little
        if( z->is_dead() ) {
            p->practice( skill_id( "computer" ), 10 );
            act->set_to_null();
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
    act->set_to_null();
}
