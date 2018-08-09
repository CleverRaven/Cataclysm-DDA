#include "activity_handlers.h"

#include "game.h"
#include "map.h"
#include "player.h"
#include "action.h"
#include "veh_interact.h"
#include "debug.h"
#include "translations.h"
#include "sounds.h"
#include "iuse_actor.h"
#include "skill.h"
#include "craft_command.h"
#include "rng.h"
#include "requirements.h"
#include "mongroup.h"
#include "morale_types.h"
#include "string_formatter.h"
#include "output.h"
#include "vpart_position.h"
#include "messages.h"
#include "martialarts.h"
#include "itype.h"
#include "vehicle.h"
#include "mapdata.h"
#include "iexamine.h"
#include "mtype.h"
#include "field.h"
#include "weather.h"
#include "ui.h"
#include "map_iterator.h"
#include "gates.h"
#include "catalua.h"
#include "fault.h"
#include "construction.h"
#include "harvest.h"

#include <math.h>
#include <sstream>
#include <algorithm>

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

const skill_id skill_survival( "survival" );
const skill_id skill_firstaid( "firstaid" );

const efftype_id effect_milked( "milked" );

using namespace activity_handlers;

const std::map< activity_id, std::function<void( player_activity *, player *)> > activity_handlers::do_turn_functions =
{
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
    { activity_id( "ACT_MOVE_ITEMS" ), move_items_do_turn },
    { activity_id( "ACT_ADV_INVENTORY" ), adv_inventory_do_turn },
    { activity_id( "ACT_ARMOR_LAYERS" ), armor_layers_do_turn },
    { activity_id( "ACT_ATM" ), atm_do_turn },
    { activity_id( "ACT_CRACKING" ), cracking_do_turn },
    { activity_id( "ACT_REPAIR_ITEM" ), repair_item_do_turn },
    { activity_id( "ACT_BUTCHER" ), butcher_do_turn },
    { activity_id( "ACT_HACKSAW" ), hacksaw_do_turn },
    { activity_id( "ACT_CHOP_TREE" ), chop_tree_do_turn },
    { activity_id( "ACT_CHOP_LOGS" ), chop_tree_do_turn },
    { activity_id( "ACT_JACKHAMMER" ), jackhammer_do_turn },
    { activity_id( "ACT_DIG" ), dig_do_turn },
    { activity_id( "ACT_FILL_PIT" ), fill_pit_do_turn }
};

const std::map< activity_id, std::function<void( player_activity *, player *)> > activity_handlers::finish_functions =
{
    { activity_id( "ACT_BURROW" ), burrow_finish },
    { activity_id( "ACT_BUTCHER" ), butcher_finish },
    { activity_id( "ACT_FIRSTAID" ), firstaid_finish },
    { activity_id( "ACT_FISH" ), fish_finish },
    { activity_id( "ACT_FORAGE" ), forage_finish },
    { activity_id( "ACT_HOTWIRE_CAR" ), hotwire_finish },
    { activity_id( "ACT_LONGSALVAGE" ), longsalvage_finish },
    { activity_id( "ACT_MAKE_ZLAVE" ), make_zlave_finish },
    { activity_id( "ACT_PICKAXE" ), pickaxe_finish },
    { activity_id( "ACT_PICKUP" ), pickup_finish },
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
    { activity_id( "ACT_CRAFT" ), craft_finish },
    { activity_id( "ACT_LONGCRAFT" ), longcraft_finish },
    { activity_id( "ACT_DISASSEMBLE" ), disassemble_finish },
    { activity_id( "ACT_BUILD" ), build_finish },
    { activity_id( "ACT_VIBE" ), vibe_finish },
    { activity_id( "ACT_MOVE_ITEMS" ), move_items_finish },
    { activity_id( "ACT_ATM" ), atm_finish },
    { activity_id( "ACT_AIM" ), aim_finish },
    { activity_id( "ACT_WASH" ), washing_finish },
    { activity_id( "ACT_HACKSAW" ), hacksaw_finish },
    { activity_id( "ACT_CHOP_TREE" ), chop_tree_finish },
    { activity_id( "ACT_CHOP_LOGS" ), chop_logs_finish },
    { activity_id( "ACT_JACKHAMMER" ), jackhammer_finish },
    { activity_id( "ACT_DIG" ), dig_finish },
    { activity_id( "ACT_FILL_PIT" ), fill_pit_finish },
    { activity_id( "ACT_SHAVE" ), shaving_finish },
    { activity_id( "ACT_HAIRCUT" ), haircut_finish }
};

void messages_in_process( const player_activity &act, const player &p ) {
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
        sounds::sound( act->placement, 10, _( "ScratchCrunchScrabbleScurry." ) );
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
    // The roll is usually b/t 0 and survival-3, so survival 4 will succeed
    // 50%, survival 5 will succeed 61%, survival 6 will succeed 67%, etc.
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

void set_up_butchery( player_activity &act, player &u )
{
    if( !act.values.empty() ) {
        act.index = act.values.back();
        act.values.pop_back();
    } else {
        debugmsg( "Invalid butchery item index %d", act.index );
        act.set_to_null();
        return;
    }

    const int factor = u.max_quality( quality_id( "BUTCHER" ) );
    auto items = g->m.i_at( u.pos() );
    if( ( size_t )act.index >= items.size() || factor == INT_MIN ) {
        // Let it print a msg for lack of corpses
        act.index = INT_MAX;
        return;
    }

    const mtype *corpse = items[act.index].get_mtype();
    int time_to_cut = 0;
    switch( corpse->size ) {
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

    act.moves_left = time_to_cut;
}

void butchery_drops_hardcoded( const mtype *corpse, player *p, const time_point &age, const std::function<int()> &roll_butchery )
{
    itype_id meat = corpse->get_meat_itype();
    if( corpse->made_of( material_id( "bone" ) ) ) {
        //For butchering yield purposes, we treat it as bones, not meat
        meat = "null";
    }

    int pieces = corpse->get_meat_chunks_count();
    int skins = 0;
    int bones = 0;
    int fats = 0;
    int sinews = 0;
    int feathers = 0;
    int wool = 0;
    bool stomach = false;

    int max_practice = 4;
    switch( corpse->size ) {
        case MS_TINY:
            skins = 1;
            bones = 1;
            fats = 1;
            sinews = 1;
            feathers = 2;
            wool = 1;
            break;
        case MS_SMALL:
            skins = 2;
            bones = 4;
            fats = 2;
            sinews = 4;
            feathers = 6;
            wool = 2;
            break;
        case MS_MEDIUM:
            skins = 4;
            bones = 9;
            fats = 4;
            sinews = 9;
            feathers = 11;
            wool = 4;
            break;
        case MS_LARGE:
            skins = 8;
            bones = 14;
            fats = 8;
            sinews = 14;
            feathers = 17;
            wool = 8;
            max_practice = 5;
            break;
        case MS_HUGE:
            skins = 16;
            bones = 21;
            fats = 16;
            sinews = 21;
            feathers = 24;
            wool = 16;
            max_practice = 6;
            break;
    }

    // Lose some meat, skins, etc if the rolls are low
    pieces +=   std::min( 0, roll_butchery() );
    skins +=    std::min( 0, roll_butchery() - 4 );
    bones +=    std::min( 0, roll_butchery() - 2 );
    fats +=     std::min( 0, roll_butchery() - 4 );
    sinews +=   std::min( 0, roll_butchery() - 8 );
    feathers += std::min( 0, roll_butchery() - 1 );
    wool +=     std::min( 0, roll_butchery() );
    stomach = roll_butchery() >= 0;

    int practice = std::max( 0, 4 + pieces + roll_butchery() );

    p->practice( skill_survival, practice, max_practice );

    if( bones > 0 ) {
        if( corpse->made_of( material_id( "veggy" ) ) ) {
            g->m.spawn_item( p->pos(), "plant_sac", bones, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some fluid bladders!" ) );
        } else if( corpse->has_flag( MF_BONES ) && corpse->has_flag( MF_POISON ) ) {
            g->m.spawn_item( p->pos(), "bone_tainted", bones / 2, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some salvageable bones!" ) );
        } else if( corpse->has_flag( MF_BONES ) && corpse->has_flag( MF_HUMAN ) ) {
            g->m.spawn_item( p->pos(), "bone_human", bones, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some salvageable bones!" ) );
        } else if( corpse->has_flag( MF_BONES ) ) {
            g->m.spawn_item( p->pos(), "bone", bones, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some usable bones!" ) );
        }
    } else if( meat == "null" && corpse->has_flag( MF_BONES ) ) {
        //print a failure message only if the corpse doesn't have meat and has bones
        p->add_msg_if_player( m_bad, _( "Your clumsy butchering destroys the bones!" ) );
    }

    if( sinews > 0 ) {
        if( corpse->has_flag( MF_BONES ) && !corpse->has_flag( MF_POISON ) ) {
            g->m.spawn_item( p->pos(), "sinew", sinews, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some usable sinews!" ) );
        } else if( corpse->made_of( material_id( "veggy" ) ) ) {
            g->m.spawn_item( p->pos(), "plant_fibre", sinews, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some plant fibers!" ) );
        }
    }

    if( stomach ) {
        const itype_id meat = corpse->get_meat_itype();
        if( meat == "meat" ) {
            if( corpse->size == MS_SMALL || corpse->size == MS_MEDIUM ) {
                g->m.spawn_item( p->pos(), "stomach", 1, 0, age );
                p->add_msg_if_player( m_good, _( "You harvest the stomach!" ) );
            } else if( corpse->size == MS_LARGE || corpse->size == MS_HUGE ) {
                g->m.spawn_item( p->pos(), "stomach_large", 1, 0, age );
                p->add_msg_if_player( m_good, _( "You harvest the stomach!" ) );
            }
        } else if( meat == "human_flesh" ) {
            if( corpse->size == MS_SMALL || corpse->size == MS_MEDIUM ) {
                g->m.spawn_item( p->pos(), "hstomach", 1, 0, age );
                p->add_msg_if_player( m_good, _( "You harvest the stomach!" ) );
            } else if( corpse->size == MS_LARGE || corpse->size == MS_HUGE ) {
                g->m.spawn_item( p->pos(), "hstomach_large", 1, 0, age );
                p->add_msg_if_player( m_good, _( "You harvest the stomach!" ) );
            }
        }
    }

    if( ( corpse->has_flag( MF_FUR ) || corpse->has_flag( MF_LEATHER ) ||
          corpse->has_flag( MF_CHITIN ) ) && skins > 0 ) {
        p->add_msg_if_player( m_good, _( "You manage to skin the %s!" ), corpse->nname().c_str() );
        int fur = 0;
        int tainted_fur = 0;
        int leather = 0;
        int tainted_leather = 0;
        int human_leather = 0;
        int chitin = 0;

        while( skins > 0 ) {
            if( corpse->has_flag( MF_CHITIN ) ) {
                chitin = rng( 0, skins );
                skins -= chitin;
                skins = std::max( skins, 0 );
            }
            if( corpse->has_flag( MF_FUR ) ) {
                if( corpse->has_flag( MF_POISON ) ) {
                    tainted_fur = rng( 0, skins );
                    skins -= tainted_fur;
                } else {
                    fur = rng( 0, skins );
                    skins -= fur;
                }
                skins = std::max( skins, 0 );
            }
            if( corpse->has_flag( MF_LEATHER ) ) {
                if( corpse->has_flag( MF_POISON ) ) {
                    tainted_leather = rng( 0, skins );
                    skins -= tainted_leather;
                } else if( corpse->has_flag( MF_HUMAN ) ) {
                    human_leather = rng( 0, skins );
                    skins -= human_leather;
                } else {
                    leather = rng( 0, skins );
                    skins -= leather;
                }
                skins = std::max( skins, 0 );
            }
        }

        if( chitin > 0 ) {
            g->m.spawn_item( p->pos(), "chitin_piece", chitin, 0, age );
        }
        if( fur > 0 ) {
            g->m.spawn_item( p->pos(), "raw_fur", fur, 0, age );
        }
        if( tainted_fur > 0 ) {
            g->m.spawn_item( p->pos(), "raw_tainted_fur", fur, 0, age );
        }
        if( leather > 0 ) {
            g->m.spawn_item( p->pos(), "raw_leather", leather, 0, age );
        }
        if( human_leather > 0 ) {
            g->m.spawn_item( p->pos(), "raw_hleather", leather, 0, age );
        }
        if( tainted_leather > 0 ) {
            g->m.spawn_item( p->pos(), "raw_tainted_leather", leather, 0, age );
        }
    }

    if( feathers > 0 ) {
        if( corpse->has_flag( MF_FEATHER ) ) {
            g->m.spawn_item( p->pos(), "feather", feathers, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some feathers!" ) );
        }
    }

    if( wool > 0 ) {
        if( corpse->has_flag( MF_WOOL ) ) {
            g->m.spawn_item( p->pos(), "wool_staple", wool, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some wool staples!" ) );
        }
    }

    if( fats > 0 ) {
        if( corpse->has_flag( MF_FAT ) && corpse->has_flag( MF_POISON ) ) {
            g->m.spawn_item( p->pos(), "fat_tainted", fats, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some gooey fat!" ) );
        } else if( corpse->has_flag( MF_FAT ) ) {
            g->m.spawn_item( p->pos(), "fat", fats, 0, age );
            p->add_msg_if_player( m_good, _( "You harvest some fat!" ) );
        }
    }

    //Add a chance of CBM recovery. For shocker and cyborg corpses.
    //As long as the factor is above -4 (the sinew cutoff), you will be able to extract CBMs
    if( corpse->has_flag( MF_CBM_CIV ) ) {
        butcher_cbm_item( "bio_power_storage", p->pos(), age, roll_butchery() );
        butcher_cbm_group( "bionics_common", p->pos(), age, roll_butchery() );
    }

    // Zombie scientist bionics
    if( corpse->has_flag( MF_CBM_SCI ) ) {
        butcher_cbm_item( "bio_power_storage", p->pos(), age, roll_butchery() );
        butcher_cbm_group( "bionics_sci", p->pos(), age, roll_butchery() );
    }

    // Zombie technician bionics
    if( corpse->has_flag( MF_CBM_TECH ) ) {
        butcher_cbm_item( "bio_power_storage", p->pos(), age, roll_butchery() );
        butcher_cbm_group( "bionics_tech", p->pos(), age, roll_butchery() );
    }

    // Substation mini-boss bionics
    if( corpse->has_flag( MF_CBM_SUBS ) ) {
        butcher_cbm_item( "bio_power_storage", p->pos(), age, roll_butchery() );
        butcher_cbm_group( "bionics_subs", p->pos(), age, roll_butchery() );
        butcher_cbm_group( "bionics_subs", p->pos(), age, roll_butchery() );
    }

    // Payoff for butchering the zombie bio-op
    if( corpse->has_flag( MF_CBM_OP ) ) {
        butcher_cbm_item( "bio_power_storage_mkII", p->pos(), age, roll_butchery() );
        butcher_cbm_group( "bionics_op", p->pos(), age, roll_butchery() );
    }

    //Add a chance of CBM power storage recovery.
    if( corpse->has_flag( MF_CBM_POWER ) ) {
        butcher_cbm_item( "bio_power_storage", p->pos(), age, roll_butchery() );
    }

    //now handle the meat, if there is any
    if( meat!= "null" ) {
        if( pieces <= 0 ) {
            p->add_msg_if_player( m_bad, _( "Your clumsy butchering destroys the flesh!" ) );
        } else {
            p->add_msg_if_player( m_good, _( "You harvest some flesh." ) );

            item chunk( meat, age, 1 );
            chunk.set_mtype( corpse );

            item parts( "offal", age, 0 );
            parts.set_mtype( corpse );

            // for now don't drop tainted or cannibal. parts overhaul of taint system to not require excessive item duplication
            bool make_offal = !chunk.is_tainted() && !chunk.has_flag( "CANNIBALISM" ) &&
                              !chunk.made_of ( material_id ( "veggy" ) );

            for ( int i = 1; i < pieces; ++i ) {
                if ( make_offal && one_in( 3 ) ) {
                    parts.charges++;
                } else {
                    chunk.charges++;
                }
            }

            g->m.add_item_or_charges( p->pos(), chunk );
            if ( parts.charges > 0 ) {
                g->m.add_item_or_charges( p->pos(), parts );
            }
        }
    }
}

void butchery_drops_harvest( const mtype &mt, player &p, const time_point &age, const std::function<int()> &roll_butchery )
{
    p.add_msg_if_player( m_neutral, _( mt.harvest->message().c_str() ) );

    int practice = 4 + roll_butchery();
    for( const auto &entry : *mt.harvest ) {
        int butchery = roll_butchery();
        float min_num = entry.base_num.first + butchery * entry.scale_num.first;
        float max_num = entry.base_num.second + butchery * entry.scale_num.second;
        int roll = std::min<int>( entry.max, round( rng_float( min_num, max_num ) ) );

        const itype *drop = item::find_type( entry.drop );

        if( roll <= 0 ) {
            p.add_msg_if_player( m_bad, _( "You fail to harvest: %s" ), drop->nname( 1 ).c_str() );
            continue;
        }

        if( drop->phase == LIQUID ) {
            g->handle_all_liquid( item( drop, age, roll ), 1 );

        } else if( drop->stackable ) {
            g->m.add_item_or_charges( p.pos(), item( drop, age, roll ) );

        } else {
            item obj( drop, age );
            obj.set_mtype( &mt );
            for( int i = 0; i != roll; ++i ) {
                g->m.add_item_or_charges( p.pos(), obj );
            }
        }

        p.add_msg_if_player( m_good, _( "You harvest: %s" ), drop->nname( roll ).c_str() );
        practice++;
    }

    p.practice( skill_survival, std::max( 0, practice ), std::max( mt.size - MS_MEDIUM, 0 ) + 4 );
}

void activity_handlers::butcher_finish( player_activity *act, player *p )
{
    if( act->index < 0 ) {
        set_up_butchery( *act, *p );
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
    const time_point &age = corpse_item.birthday();
    g->m.i_rem( p->pos(), act->index );

    const int skill_level = p->get_skill_level( skill_survival );
    const int factor = p->max_quality( quality_id( "BUTCHER" ) );

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

    if( corpse->harvest.is_null() ) {
        butchery_drops_hardcoded( corpse, p, age, roll_butchery );
    } else {
        butchery_drops_harvest( *corpse, *p, age, roll_butchery );
    }

    // Recover hidden items
    for( auto &content : contents  ) {
        if( ( roll_butchery() + 10 ) * 5 > rng( 0, 100 ) ) {
            //~ %1$s - item name, %2$s - monster name
            p->add_msg_if_player( m_good, _( "You discover a %1$s in the %2$s!" ), content.tname().c_str(),
                     corpse->nname().c_str() );
            g->m.add_item_or_charges( p->pos(), content );
        } else if( content.is_bionic()  ) {
            g->m.spawn_item(p->pos(), "burnt_out_bionic", 1, 0, age);
        }
    }

    p->add_msg_if_player( m_good, _("You finish butchering the %s."), corpse->nname().c_str() );

    if( act->values.empty() ) {
        act->set_to_null();
    } else {
        set_up_butchery( *act, *p );
    }
}

enum liquid_source_type { LST_INFINITE_MAP = 1, LST_MAP_ITEM = 2, LST_VEHICLE = 3, LST_MONSTER = 4};

// All serialize_liquid_source functions should add the same number of elements to the vectors of
// the activity. This makes it easier to distinguish the values of the source and the values of the target.
void serialize_liquid_source( player_activity &act, const vehicle &veh, const itype_id &ftype )
{
    act.values.push_back( LST_VEHICLE );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( veh.global_pos3() );
    act.str_values.push_back( ftype );
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
    const auto iter = std::find_if( stack.begin(), stack.end(), [&]( const item &i ) { return &i == &liquid; } );
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

void activity_handlers::fill_liquid_do_turn( player_activity *act_, player *p )
{
    player_activity &act = *act_;
    try {
        // 1. Gather the source item.
        vehicle *source_veh = nullptr;
        const tripoint source_pos = act.coords.at( 0 );
        map_stack source_stack = g->m.i_at( source_pos );
        std::list<item>::iterator on_ground;
        monster *source_mon = nullptr;
        item liquid;
        const auto source_type = static_cast<liquid_source_type>( act.values.at( 0 ) );
        switch( source_type ) {
        case LST_VEHICLE:
            source_veh = veh_pointer_or_null( g->m.veh_at( source_pos ) );
            if( source_veh == nullptr ) {
                throw std::runtime_error( "could not find source vehicle for liquid transfer" );
            }
            liquid = item( act.str_values.at( 0 ), calendar::turn, source_veh->fuel_left( act.str_values.at( 0 ) ) );
            break;
        case LST_INFINITE_MAP:
            deserialize( liquid, act.str_values.at( 0 ) );
            liquid.charges = item::INFINITE_CHARGES;
            break;
        case LST_MAP_ITEM:
            if( static_cast<size_t>( act.values.at( 1 ) ) >= source_stack.size() ) {
                throw std::runtime_error( "could not find source item on ground for liquid transfer" );
            }
            on_ground = source_stack.begin();
            std::advance( on_ground, act.values.at( 1 ) );
            liquid = *on_ground;
            break;
        case LST_MONSTER:
            Creature *c = g->critter_at( source_pos );
            source_mon = dynamic_cast<monster *>( c );
            if( source_mon == nullptr ) {
                debugmsg( "could not find source creature for liquid transfer" );
                act.set_to_null();
            }
            deserialize( liquid, act.str_values.at( 0 ) );
            liquid.charges = 1;
            break;
        }

        static const auto volume_per_turn = units::from_liter( 4 );
        const long charges_per_turn = std::max( 1l, liquid.charges_per_volume( volume_per_turn ) );
        liquid.charges = std::min( charges_per_turn, liquid.charges );
        const long original_charges = liquid.charges;

        // 2. Transfer charges.
        switch( static_cast<liquid_target_type>( act.values.at( 2 ) ) ) {
        case LTT_VEHICLE:
            if( const optional_vpart_position vp = g->m.veh_at( act.coords.at( 1 ) ) ) {
                p->pour_into( vp->vehicle(), liquid );
            } else {
                throw std::runtime_error( "could not find target vehicle for liquid transfer" );
            }
            break;
        case LTT_CONTAINER:
            p->pour_into( p->i_at( act.values.at( 3 ) ), liquid );
            break;
        case LTT_MAP:
            if( iexamine::has_keg( act.coords.at( 1 ) ) ) {
                iexamine::pour_into_keg( act.coords.at( 1 ), liquid );
            } else {
                g->m.add_item_or_charges( act.coords.at( 1 ), liquid );
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
            act.set_to_null();
            return;
        }

        // 3. Remove charges from source.
        switch( source_type ) {
        case LST_VEHICLE:
            source_veh->drain( liquid.typeId(), removed_charges );
            if( source_veh->fuel_left( liquid.typeId() ) <= 0 ) {
                act.set_to_null();
            }
            break;
        case LST_MAP_ITEM:
            on_ground->charges -= removed_charges;
            if( on_ground->charges <= 0 ) {
                source_stack.erase( on_ground );
                if( g->m.ter( source_pos ).obj().examine == &iexamine::gaspump ) {
                    add_msg( _( "With a clang and a shudder, the %s pump goes silent."),
                             liquid.type_name( 1 ).c_str() );
                } else if( g->m.furn( source_pos ).obj().examine == &iexamine::fvat_full ) {
                    g->m.furn_set( source_pos, f_fvat_empty );
                    add_msg( _( "You squeeze the last drops of %s from the vat." ),
                             liquid.type_name( 1 ).c_str() );
                }
                act.set_to_null();
            }
            break;
        case LST_INFINITE_MAP:
            // nothing, the liquid source is infinite
            break;
        case LST_MONSTER:
            // liquid source charges handled in monexamine::milk_source
            if( liquid.charges == 0 ) {
                act.set_to_null();
            }
            break;
        }

        if( removed_charges < original_charges ) {
            // Transferred less than the available charges -> target must be full
            act.set_to_null();
        }

    } catch( const std::runtime_error &err ) {
        debugmsg( "error in activity data: \"%s\"", err.what() );
        act.set_to_null();
        return;
    }
}

// handles equipping an item on ACT_PICKUP, if requested
void activity_handlers::pickup_finish(player_activity *act, player *p)
{
    // loop through all the str_values, and if we find equip, do so.
    // if no str_values present, carry on
    for(auto &elem : act->str_values) {
        if(elem == "equip") {
            item &it = p->i_at(act->position);
            p->wear_item(it);
        }
    }
}

void activity_handlers::move_items_finish( player_activity *act, player *p )
{
    pickup_finish( act, p );
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
    hp_part healed = (hp_part)act->values[0];
    long charges_consumed = actor->finish_using( *p, patient, *used_tool, healed );
    p->consume_charges( it, charges_consumed );

    // Erase activity and values.
    act->set_to_null();
    act->values.clear();
}

// fish-with-rod fish catching function.
static void rod_fish( player *p, int sSkillLevel, int fishChance )
{
   if( sSkillLevel > fishChance ) {
        std::vector<monster *> fishables = g->get_fishable(60); //get the nearby fish list.
        //if the vector is empty (no fish around) the player is still given a small chance to get a (let us say it was hidden) fish
        if( fishables.empty() ) {
            if( one_in(20) ) {
                item fish;
                const std::vector<mtype_id> fish_group = MonsterGroupManager::GetMonstersFromGroup( mongroup_id( "GROUP_FISH" ) );
                const mtype_id &fish_mon = random_entry_ref( fish_group );
                g->m.add_item_or_charges(p->pos(), item::make_corpse( fish_mon ) );
                p->add_msg_if_player(m_good, _("You caught a %s."), fish_mon.obj().nname().c_str());
            } else {
                p->add_msg_if_player(_("You didn't catch anything."));
            }
        } else {
            g->catch_a_monster(fishables, p->pos(), p, 30000);
        }

    } else {
        p->add_msg_if_player(_("You didn't catch anything."));
    }
}

void activity_handlers::fish_finish( player_activity *act, player *p )
{
    item &it = p->i_at(act->position);
    int sSkillLevel = 0;
    int fishChance = 20;
    if( it.has_flag("FISH_POOR") ) {
        sSkillLevel = p->get_skill_level( skill_survival ) + dice(1, 6);
        fishChance = dice(1, 20);
    } else if( it.has_flag("FISH_GOOD") ) {
        // Much better chances with a good fishing implement.
        sSkillLevel = p->get_skill_level( skill_survival ) * 1.5 + dice(1, 6) + 3;
        fishChance = dice(1, 20);
    }
    ///\EFFECT_SURVIVAL increases chance of fishing success
    rod_fish( p, sSkillLevel, fishChance );
    p->practice( skill_survival, rng(5, 15) );
    act->set_to_null();
}

void activity_handlers::forage_finish( player_activity *act, player *p )
{
    int veggy_chance = rng(1, 100);
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
    if( one_in(10) ) {
        const auto dropped = g->m.put_items_from_loc( "trash_forest", p->pos(), calendar::turn );
        for( const auto &it : dropped ) {
            add_msg( m_good, _( "You found: %s!" ), it->tname().c_str() );
            found_something = true;
        }
    }

    if( !found_something ) {
        add_msg(_("You didn't find anything."));
    }

    ///\EFFECT_INT Intelligence caps survival skill gains from foraging
    const int max_forage_skill = p->int_cur / 3 + 1;
    ///\EFFECT_SURVIVAL decreases survival skill gain from foraging (NEGATIVE)
    const int max_exp = 2 * ( max_forage_skill - p->get_skill_level( skill_survival ) );
    // Award experience for foraging attempt regardless of success
    p->practice( skill_survival, rng(1, max_exp), max_forage_skill );

    act->set_to_null();
}


void activity_handlers::game_do_turn( player_activity *act, player *p )
{
    //Gaming takes time, not speed
    act->moves_left -= 100;

    item &game_item = p->i_at(act->position);

    //Deduct 1 battery charge for every minute spent playing
    if( calendar::once_every( 1_minutes ) ) {
        game_item.ammo_consume( 1, p->pos() );
        p->add_morale(MORALE_GAME, 1, 100); //1 points/min, almost 2 hours to fill
    }
    if( game_item.ammo_remaining() == 0 ) {
        act->moves_left = 0;
        add_msg(m_info, _("The %s runs out of batteries."), game_item.tname().c_str());
    }
}


void activity_handlers::hotwire_finish( player_activity *act, player *pl )
{
    //Grab this now, in case the vehicle gets shifted
    if( const optional_vpart_position vp = g->m.veh_at( tripoint( act->values[0], act->values[1], pl->posz() ) ) ) {
        vehicle *const veh = &vp->vehicle();
        int mech_skill = act->values[2];
        if( mech_skill > (int)rng(1, 6) ) {
            //success
            veh->is_locked = false;
            add_msg(_("This wire will start the engine."));
        } else if( mech_skill > (int)rng(0, 4) ) {
            //soft fail
            veh->is_locked = false;
            veh->is_alarm_on = veh->has_security_working();
            add_msg(_("This wire will probably start the engine."));
        } else if( veh->is_alarm_on ) {
            veh->is_locked = false;
            add_msg(_("By process of elimination, this wire will start the engine."));
        } else {
            //hard fail
            veh->is_alarm_on = veh->has_security_working();
            add_msg(_("The red wire always starts the engine, doesn't it?"));
        }
    } else {
        dbg(D_ERROR) << "game:process_activity: ACT_HOTWIRE_CAR: vehicle not found";
        debugmsg("process_activity ACT_HOTWIRE_CAR: vehicle not found");
    }
    act->set_to_null();
}


void activity_handlers::longsalvage_finish( player_activity *act, player *p )
{
    const static std::string salvage_string = "salvage";
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

    for( auto it = items.begin(); it != items.end(); ++it ) {
        if( actor->valid_to_cut_up( *it ) ) {
            actor->cut_up( *p, *salvage_tool, *it );
            return;
        }
    }

    add_msg( _( "You finish salvaging." ) );
    act->set_to_null();
}


void activity_handlers::make_zlave_finish( player_activity *act, player *p )
{
    act->set_to_null();
    auto items = g->m.i_at(p->pos());
    std::string corpse_name = act->str_values[0];
    item *body = NULL;

    for( auto it = items.begin(); it != items.end(); ++it ) {
        if( it->display_name() == corpse_name ) {
            body = &*it;
        }
    }

    if( body == NULL ) {
        add_msg(m_info, _("There's no corpse to make into a zombie slave!"));
        return;
    }

    int success = act->values[0];

    if( success > 0 ) {

        p->practice( skill_firstaid, rng(2, 5) );
        p->practice( skill_survival, rng(2, 5) );

        p->add_msg_if_player(m_good,
                             _("You slice muscles and tendons, and remove body parts until you're confident the zombie won't be able to attack you when it reanimates."));

        body->set_var( "zlave", "zlave" );
        //take into account the chance that the body yet can regenerate not as we need.
        if( one_in(10) ) {
            body->set_var( "zlave", "mutilated" );
        }

    } else {

        if( success > -20 ) {

            p->practice( skill_firstaid, rng(3, 6) );
            p->practice( skill_survival, rng(3, 6) );

            p->add_msg_if_player(m_warning,
                                 _("You hack into the corpse and chop off some body parts.  You think the zombie won't be able to attack when it reanimates."));

            success += rng(1, 20);

            if( success > 0 && !one_in(5) ) {
                body->set_var( "zlave", "zlave" );
            } else {
                body->set_var( "zlave", "mutilated" );
            }

        } else {

            p->practice( skill_firstaid, rng(1, 8) );
            p->practice( skill_survival, rng(1, 8) );

            body->mod_damage( rng( 0, body->max_damage() - body->damage() ), DT_STAB );
            if( body->damage() == body->max_damage() ) {
                body->active = false;
                p->add_msg_if_player(m_warning, _("You cut up the corpse too much, it is thoroughly pulped."));
            } else {
                p->add_msg_if_player(m_warning,
                                     _("You cut into the corpse trying to make it unable to attack, but you don't think you have it right."));
            }
        }
    }
}

void activity_handlers::pickaxe_do_turn( player_activity *act, player *p )
{
    const tripoint &pos = act->placement;
    if( calendar::once_every( 1_minutes ) ) { // each turn is too much
        //~ Sound of a Pickaxe at work!
        sounds::sound( pos, 30, _( "CHNK! CHNK! CHNK!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::pickaxe_finish( player_activity *act, player *p )
{
    const tripoint pos( act->placement );
    item &it = p->i_at( act->position );

    act->set_to_null(); // Invalidate the activity early to prevent a query from mod_pain()

    if( g->m.is_bashable( pos ) && g->m.has_flag( "SUPPORTS_ROOF", pos ) &&
        g->m.ter(pos) != t_tree ) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Betcha wish you'd opted for the J-Hammer ;P
        p->mod_hunger( 15 );
        p->mod_thirst( 15 );
        if( p->has_trait( trait_id( "STOCKY_TROGLO" ) ) ) {
            p->mod_fatigue( 20 ); // Yep, dwarves can dig longer before tiring
        } else {
            p->mod_fatigue( 30 );
        }
        p->mod_pain( 2 * rng( 1, 3 ) );
    } else if( g->m.move_cost(pos) == 2 && g->get_levz() == 0 &&
               g->m.ter(pos) != t_dirt && g->m.ter(pos) != t_grass ) {
        //Breaking up concrete on the surface? not nearly as bad
        p->mod_hunger( 5 );
        p->mod_thirst( 5 );
        p->mod_fatigue( 10 );
    }
    p->add_msg_if_player( m_good, _( "You finish digging." ) );
    g->m.destroy( pos, true );
    it.charges = std::max(long(0), it.charges - it.type->charges_to_use());
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
    float pulp_power = sqrt( ( p->str_cur + p->weapon.damage_melee( DT_BASH ) ) * ( cut_power + 1.0f ) );
    // Multiplier to get the chance right + some bonus for survival skill
    pulp_power *= 40 + p->get_skill_level( skill_survival ) * 5;

    const int mess_radius = p->weapon.has_flag("MESSY") ? 2 : 1;

    int moves = 0;
    int &num_corpses = act->index; // use this to collect how many corpse are pulped
    auto corpse_pile = g->m.i_at( pos );
    for( auto &corpse : corpse_pile ) {
        if( !corpse.is_corpse() || !corpse.get_mtype()->has_flag( MF_REVIVES ) ||
            ( std::find( act->str_values.begin(), act->str_values.end(), "auto_pulp_no_acid" ) !=
              act->str_values.end() && corpse.get_mtype()->bloodType() == fd_acid ) ) {
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

            if( x_in_y( pulp_power, corpse.volume() / units::legacy_volume_factor ) ) { // Splatter some blood around
                // Splatter a bit more randomly, so that it looks cooler
                const int radius = mess_radius + x_in_y( pulp_power, 500 ) + x_in_y( pulp_power, 1000 );
                const tripoint dest( pos.x + rng( -radius, radius ), pos.y + rng( -radius, radius ), pos.z );
                const field_id type_blood = ( mess_radius > 1 && x_in_y( pulp_power, 10000 ) ) ?
                                            corpse.get_mtype()->gibType() :
                                            corpse.get_mtype()->bloodType();
                g->m.add_splatter_trail( type_blood, pos, dest );
            }

            float stamina_ratio = (float)p->stamina / p->get_stamina_max();
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
        p->add_msg_if_player(m_bad, _("The corpse moved before you could finish smashing it!"));
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
            sounds::ambient_sound( p->pos(), reloadable.type->gun->reload_noise_volume, reloadable.type->gun->reload_noise );
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
    item &lens_item = p->i_at(act->position);
    const auto usef = lens_item.type->get_use( "firestarter" );
    if( usef == nullptr || usef->get_actor_ptr() == nullptr ) {
        add_msg( m_bad, _( "You have lost the item you were using to start the fire." ) );
        p->cancel_activity();
        return;
    }

    p->mod_moves( -p->moves );
    const auto actor = dynamic_cast<const firestarter_actor*>( usef->get_actor_ptr() );
    float light = actor->light_mod( p->pos() );
    act->moves_left -= light * 100;
    if( light < 0.1 ) {
        add_msg( m_bad, _("There is not enough sunlight to start a fire now. You stop trying.") );
        p->cancel_activity();
    }
}

void activity_handlers::train_finish( player_activity *act, player *p )
{
    const skill_id sk( act->name );
    if( sk.is_valid() ) {
        const Skill &skill = sk.obj();
        int new_skill_level = p->get_skill_level( sk ) + 1;
        p->set_skill_level( sk, new_skill_level );
        add_msg(m_good, _("You finish training %s to level %d."),
                skill.name().c_str(),
                new_skill_level);
        if( new_skill_level % 4 == 0 ) {
            //~ %d is skill level %s is skill name
            p->add_memorial_log(pgettext("memorial_male", "Reached skill level %1$d in %2$s."),
                                pgettext("memorial_female", "Reached skill level %1$d in %2$s."),
                                new_skill_level, skill.name().c_str());
        }

        lua_callback("on_skill_increased");
        act->set_to_null();
        return;
    }

    const auto &ma_id = matype_id( act->name );
    if( ma_id.is_valid() ) {
        const auto &mastyle = ma_id.obj();
        // Trained martial arts,
        add_msg(m_good, _("You learn %s."), mastyle.name.c_str());
        //~ %s is martial art
        p->add_memorial_log(pgettext("memorial_male", "Learned %s."),
                            pgettext("memorial_female", "Learned %s."),
                            mastyle.name.c_str());
        p->add_martialart( mastyle.id );
    } else {
        debugmsg( "train_finish without a valid skill or style name" );
    }

    act->set_to_null();
    return;
}

void activity_handlers::vehicle_finish( player_activity *act, player *pl )
{
    //Grab this now, in case the vehicle gets shifted
    const optional_vpart_position vp = g->m.veh_at( tripoint( act->values[0], act->values[1], pl->posz() ) );
    veh_interact::complete_vehicle();
    // complete_vehicle set activity type to NULL if the vehicle
    // was completely dismantled, otherwise the vehicle still exist and
    // is to be examined again.
    if( act->is_null() ) {
        return;
    }
    act->set_to_null();
    if( act->values.size() < 7 ) {
        dbg(D_ERROR) << "game:process_activity: invalid ACT_VEHICLE values: "
                     << act->values.size();
        debugmsg("process_activity invalid ACT_VEHICLE values:%d",
                 act->values.size());
    } else {
        if( vp ) {
            g->refresh_all();
            // TODO: Z (and also where the activity is queued)
            // Or not, because the vehicle coordinates are dropped anyway
            g->exam_vehicle( vp->vehicle(), act->values[ 2 ], act->values[ 3 ] );
            return;
        } else {
            dbg(D_ERROR) << "game:process_activity: ACT_VEHICLE: vehicle not found";
            debugmsg("process_activity ACT_VEHICLE: vehicle not found");
        }
    }
}

void activity_handlers::vibe_do_turn( player_activity *act, player *p )
{
    //Using a vibrator takes time (10 minutes), not speed
    //Linear increase in morale during action with a small boost at end
    //Deduct 1 battery charge for every minute in use, or vibrator is much less effective
    act->moves_left -= 100;

    item &vibrator_item = p->i_at(act->position);

    if( (p->is_wearing("rebreather")) || (p->is_wearing("rebreather_xl")) ||
        (p->is_wearing("mask_h20survivor")) ) {
        act->moves_left = 0;
        add_msg(m_bad, _("You have trouble breathing, and stop."));
    }

    if( calendar::once_every( 1_minutes ) ) {
        p->mod_fatigue(1);
        if( vibrator_item.ammo_remaining() > 0 ) {
            vibrator_item.ammo_consume( 1, p->pos() );
            p->add_morale(MORALE_FEELING_GOOD, 3, 40);
            if( vibrator_item.ammo_remaining() == 0 ) {
                add_msg(m_info, _("The %s runs out of batteries."), vibrator_item.tname().c_str());
            }
        }
        else {
            p->add_morale(MORALE_FEELING_GOOD, 1, 40); //twenty minutes to fill
        }
    }
    if( p->get_fatigue() >= DEAD_TIRED ) { // Dead Tired: different kind of relaxation needed
        act->moves_left = 0;
        add_msg(m_info, _("You're too tired to continue."));
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
        if( !veh ) { return; }
    }

    int attempted = 0;
    int started = 0;
    int not_muscle = 0;
    const bool take_control = act->values[0];

    for( size_t e = 0; e < veh->engines.size(); ++e ) {
        if( veh->is_engine_on( e ) ) {
            attempted++;
            if( veh->start_engine( e ) ) { started++; }
            if( !veh->is_engine_type( e, "muscle" ) ) { not_muscle++; }
        }
    }

    veh->engine_on = attempted > 0 && started == attempted;

    if( attempted == 0 ) {
        add_msg( m_info, _("The %s doesn't have an engine!"), veh->name.c_str() );
    } else if( not_muscle > 0 ) {
        if( started == attempted ) {
            add_msg( ngettext("The %s's engine starts up.",
                "The %s's engines start up.", not_muscle), veh->name.c_str() );
        } else {
            add_msg( m_bad, ngettext("The %s's engine fails to start.",
                "The %s's engines fail to start.", not_muscle), veh->name.c_str() );
        }
    }

    if( take_control && !veh->engine_on && !veh->velocity ) {
        p->controlling_vehicle = false;
        add_msg(_("You let go of the controls."));
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
        sounds::sound( act->placement, 10, _("hissssssssss!") );
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
        g->m.spawn_item( p->pos(), "steel_chunk", rng(2, 6) );
    } else if( ter == t_chainfence || ter == t_chaingate_c || ter == t_chaingate_l ) {
        g->m.ter_set( pos, t_dirt );
        g->m.spawn_item( pos, "pipe", rng(1, 4) );
        g->m.spawn_item( pos, "wire", rng(4, 16) );
    } else if( ter == t_chainfence_posts ) {
        g->m.ter_set( pos, t_dirt );
        g->m.spawn_item( pos, "pipe", rng(1, 4) );
    } else if( ter == t_door_metal_locked || ter == t_door_metal_c || ter == t_door_bar_c ||
               ter == t_door_bar_locked || ter == t_door_metal_pickable ) {
        g->m.ter_set( pos, t_mdoor_frame );
        g->m.spawn_item( pos, "steel_plate", rng(0, 1) );
        g->m.spawn_item( pos, "steel_chunk", rng(3, 8) );
    } else if( ter == t_window_enhanced || ter == t_window_enhanced_noglass ) {
        g->m.ter_set( pos, t_window_empty );
        g->m.spawn_item( pos, "steel_plate", rng(0, 1) );
        g->m.spawn_item( pos, "sheet_metal", rng(1, 3) );
    } else if( ter == t_reb_cage ) {
        g->m.ter_set( pos, t_pit );
        g->m.spawn_item( pos, "spike", rng(1, 19) );
        g->m.spawn_item( pos, "scrap", rng(1, 8) );
    } else if( ter == t_bars ) {
        if (g->m.ter( {pos.x + 1, pos.y, pos.z} ) == t_sewage || g->m.ter( {pos.x, pos.y + 1, pos.z} ) == t_sewage ||
            g->m.ter( {pos.x - 1, pos.y, pos.z} ) == t_sewage || g->m.ter( {pos.x, pos.y - 1, pos.z} ) == t_sewage) {
            g->m.ter_set( pos, t_sewage );
            g->m.spawn_item( p->pos(), "pipe", rng(1, 2) );
        } else {
            g->m.ter_set( pos, t_floor );
            g->m.spawn_item( p->pos(), "pipe", rng(1, 2) );
        }
    } else if( ter == t_window_bars_alarm ) {
        g->m.ter_set( pos, t_window_alarm );
        g->m.spawn_item( p->pos(), "pipe", rng(1, 2) );
    } else if( ter == t_window_bars ) {
        g->m.ter_set( pos, t_window_empty );
        g->m.spawn_item( p->pos(), "pipe", rng(1, 2) );
    }
}

void activity_handlers::cracking_finish( player_activity *act, player *p )
{
    p->add_msg_if_player( m_good, _( "The safe opens!" ) );
    g->m.furn_set( act->placement, f_safe_o );
    act->set_to_null();
}

void activity_handlers::open_gate_finish( player_activity *act, player * )
{
    const tripoint pos = act->placement; // Don't use reference and don't inline, because act can change
    gates::open_gate( pos );
    act->set_to_null();
}

enum repeat_type : int {
    REPEAT_ONCE = 0,    // Repeat just once
    REPEAT_FOREVER,     // Repeat for as long as possible
    REPEAT_FULL,        // Repeat until damage==0
    REPEAT_EVENT,       // Repeat until something interesting happens
    REPEAT_CANCEL,      // Stop repeating
    REPEAT_INIT         // Haven't found repeat value yet.
};

repeat_type repeat_menu( const std::string &title, repeat_type last_selection )
{
    uimenu rmenu;
    rmenu.text = title;
    rmenu.return_invalid = true;

    rmenu.addentry( REPEAT_ONCE, true, '1', _("Repeat once") );
    rmenu.addentry( REPEAT_FOREVER, true, '2', _("Repeat as long as you can") );
    rmenu.addentry( REPEAT_FULL, true, '3', _("Repeat until fully repaired, but don't reinforce") );
    rmenu.addentry( REPEAT_EVENT, true, '4', _("Repeat until success/failure/level up") );

    rmenu.selected = last_selection;

    rmenu.query();
    if( rmenu.ret >= REPEAT_ONCE && rmenu.ret <= REPEAT_EVENT ) {
        return ( repeat_type )rmenu.ret;
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

    bool init( const player_activity &act )
    {
        if( act.coords.empty() || act.values.size() < 2 ) {
            return false;
        }

        part = act.values[1];
        veh = veh_pointer_or_null( g->m.veh_at( act.coords[0] ) );
        if( veh == nullptr || veh->parts.size() <= ( size_t )part ) {
            part = -1;
            return false;
        }

        part = veh->part_with_feature( part, "WELDRIG" );
        return part >= 0;
    }

    item &get_item()
    {
        if( veh != nullptr && part >= 0 ) {
            pseudo.charges = veh->drain( "battery", 1000 - pseudo.charges );
            return pseudo;
        }

        // null item should be handled just fine
        return null_item_reference();
    }

    void clean_up()
    {
        // Return unused charges
        if( veh == nullptr || part < 0 ) {
            return;
        }

        veh->charge_battery( pseudo.charges );
        pseudo.charges = 0;
    }
};

void activity_handlers::repair_item_finish( player_activity *act, player *p )
{
    const std::string iuse_name_string = act->get_str_value( 0, "repair_item" );
    repeat_type repeat = ( repeat_type )act->get_value( 0, REPEAT_INIT );
    weldrig_hack w_hack;
    item &main_tool = !w_hack.init( *act ) ?
                      p->i_at( act->index ) : w_hack.get_item();

    item *used_tool = main_tool.get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for long repair" );
        act->set_to_null();
        return;
    }
    bool event_happened = false;

    const auto use_fun = used_tool->get_use( iuse_name_string );
    // TODO: De-uglify this block. Something like get_use<iuse_actor_type>() maybe?
    const auto *actor = dynamic_cast<const repair_item_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    // TODO: Allow setting this in the actor
    // TODO: Don't use charges_to_use: welder has 50 charges per use, soldering iron has 1
    if( !used_tool->ammo_sufficient() ) {
        p->add_msg_if_player( _( "Your %s ran out of charges" ), used_tool->tname().c_str() );
        act->set_to_null();
        return;
    }

    item &fix = p->i_at( act->position );

    // The first time through we just find out how many times the player wants to repeat the action.
    if( repeat != REPEAT_INIT ) {
        // Remember our level: we want to stop retrying on level up
        const int old_level = p->get_skill_level( actor->used_skill );
        const auto attempt = actor->repair( *p, *used_tool, fix );
        if( attempt != repair_item_actor::AS_CANT ) {
            p->consume_charges( *used_tool, used_tool->ammo_required() );
        }

        // Print message explaining why we stopped
        // But only if we didn't destroy the item (because then it's obvious)
        const bool destroyed = attempt == repair_item_actor::AS_DESTROYED;
        if( attempt == repair_item_actor::AS_CANT ||
            destroyed ||
            !actor->can_repair( *p, *used_tool, fix, !destroyed ) ) {
            // Can't repeat any more
            act->set_to_null();
            w_hack.clean_up();
            return;
        }

        event_happened =
            attempt == repair_item_actor::AS_FAILURE ||
            attempt == repair_item_actor::AS_SUCCESS ||
            old_level != p->get_skill_level( actor->used_skill );
    } else {
        repeat = REPEAT_ONCE;
    }

    w_hack.clean_up();
    const bool need_input =
        repeat == REPEAT_ONCE ||
        ( repeat == REPEAT_EVENT && event_happened ) ||
        ( repeat == REPEAT_FULL && fix.damage() <= 0 );

    if( need_input ) {
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

        const std::string title = string_format(
                                      _( "%s\nSuccess chance: %.1f%%\nDamage chance: %.1f%%" ),
                                      repair_item_actor::action_description( action_type ).c_str(),
                                      100.0f * chance.first, 100.0f * chance.second );
        repeat_type answer = repeat_menu( title, repeat );
        if( answer == REPEAT_CANCEL ) {
            act->set_to_null();
            return;
        }

        if( act->values.empty() ) {
            act->values.resize( 1 );
        }

        act->values[0] = ( int )answer;
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
    const auto& reqs = f->obj().requirements();
    if( !reqs.can_make_with_inventory( inv ) ) {
        add_msg( m_info, _( "You are currently unable to mend the %s." ), target->tname().c_str() );
    }
    for( const auto& e : reqs.get_components() ) {
        p->consume_items( e );
    }
    for( const auto& e : reqs.get_tools() ) {
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
    p->add_msg_if_player( m_good, _( "You successfully attached the %1$s to your %2$s." ), mod.tname().c_str(),
                tool.tname().c_str() );
    tool.contents.push_back( mod );
    act->targets[1].remove_item();
}

void activity_handlers::clear_rubble_finish( player_activity *act, player *p )
{
    const tripoint &target = act->coords[0];
    if( target == p->pos() ) {
        p->add_msg_if_player( m_info, _( "You clear up the %s at your feet." ),
                              g->m.furnname( target ).c_str() );
    } else {
        const std::string direction = direction_name( direction_from( p->pos(), target ) );
        p->add_msg_if_player( m_info, _( "You clear up the %s to your %s." ),
                              g->m.furnname( target ).c_str(), direction.c_str() );
    }
    g->m.furn_set( target, f_null );

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

void activity_handlers::move_items_do_turn( player_activity *, player * )
{
    activity_on_turn_move_items();
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
    p->add_morale(MORALE_FEELING_GOOD, 10, 40);
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

void activity_handlers::hacksaw_do_turn( player_activity *act, player *p ) {
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a metal sawing tool at work!
        sounds::sound( act->placement, 15, _( "grnd grnd grnd" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::hacksaw_finish( player_activity *act, player *p ) {
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
        g->m.spawn_item( pos, "pipe", 6);
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
        g->m.spawn_item( pos, "spike", 19);
        g->m.spawn_item( pos, "scrap", 8);
    } else if( ter == t_bars ) {
        if( g->m.ter( { pos.x + 1, pos.y, pos.z } ) == t_sewage || g->m.ter( { pos.x, pos.y + 1, pos.z } ) == t_sewage ||
            g->m.ter( { pos.x - 1, pos.y, pos.z } ) == t_sewage || g->m.ter( { pos.x, pos.y - 1, pos.z } ) == t_sewage ) {
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

void activity_handlers::chop_tree_do_turn( player_activity *act, player *p ) {
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a wood chopping tool at work!
        sounds::sound( act->placement, 15, _( "CHK!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::chop_tree_finish( player_activity *act, player *p ) {
    const tripoint &pos = act->placement;

    tripoint direction;
    while( !choose_direction( _( "Select a direction for the tree to fall in." ), direction ) ) {
        // try again
    }

    tripoint to = pos + point( 3 * direction.x + rng( -1, 1 ), 3 * direction.y + rng( -1, 1 ) );
    std::vector<tripoint> tree = line_to( pos, to, rng( 1, 8 ) );
    for( auto &elem : tree ) {
        g->m.destroy( elem );
        g->m.ter_set( elem, t_trunk );
    }

    g->m.ter_set( pos, t_dirt );

    p->mod_hunger( 5 );
    p->mod_thirst( 5 );
    p->mod_fatigue( 10 );
    p->add_msg_if_player( m_good, _( "You finish chopping down a tree." ) );

    act->set_to_null();
}

void activity_handlers::chop_logs_finish( player_activity *act, player *p ) {
    const tripoint &pos = act->placement;

    g->m.ter_set( pos, t_dirt );
    g->m.spawn_item( pos.x, pos.y, "log", rng( 2, 3 ), 0, calendar::turn );
    g->m.spawn_item( pos.x, pos.y, "stick_long", rng( 0, 1 ), 0, calendar::turn );

    p->mod_hunger( 5 );
    p->mod_thirst( 5 );
    p->mod_fatigue( 10 );
    p->add_msg_if_player( m_good, _( "You finish chopping the logs." ) );

    act->set_to_null();
}

void activity_handlers::jackhammer_do_turn( player_activity *act, player *p ) {
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a jackhammer at work!
        sounds::sound( act->placement, 15, _( "TATATATATATATAT!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::jackhammer_finish( player_activity *act, player *p ) {
    const tripoint &pos = act->placement;

    g->m.destroy( pos, true );

    p->mod_hunger( 5 );
    p->mod_thirst( 5 );
    p->mod_fatigue( 10 );
    p->add_msg_if_player( m_good, _( "You finish drilling." ) );

    act->set_to_null();
}

void activity_handlers::dig_do_turn( player_activity *act, player *p ) {
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel digging a pit at work!
        sounds::sound( act->placement, 10, _( "hsh!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::dig_finish( player_activity *act, player *p ) {
    const tripoint &pos = act->placement;

    if( g->m.ter( pos ) == t_pit_shallow ) {
        g->m.ter_set( pos, t_pit );
    } else {
        g->m.ter_set( pos, t_pit_shallow );
    }

    p->mod_hunger( 5 );
    p->mod_thirst( 5 );
    p->mod_fatigue( 10 );
    p->add_msg_if_player( m_good, _( "You finish digging up %s." ), g->m.ter( pos ).obj().name() );

    act->set_to_null();
}

void activity_handlers::fill_pit_do_turn( player_activity *act, player *p ) {
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel filling a pit or mound at work!
        sounds::sound( act->placement, 10, _( "hsh!" ) );
        messages_in_process( *act, *p );
    }
}

void activity_handlers::fill_pit_finish( player_activity *act, player *p ) {
    const tripoint &pos = act->placement;
    const ter_id ter = g->m.ter( pos );
    const ter_id old_ter = ter;

    if( ter == t_pit || ter == t_pit_spiked || ter == t_pit_glass ||
        ter == t_pit_corpsed ) {
        g->m.ter_set( pos, t_pit_shallow );
    } else {
        g->m.ter_set( pos, t_dirt );
    }

    p->mod_hunger( 5 );
    p->mod_thirst( 5 );
    p->mod_fatigue( 10 );
    p->add_msg_if_player( m_good, _( "You finish filling up %s."), old_ter.obj().name() );

    act->set_to_null();
}

void activity_handlers::shaving_finish( player_activity *act, player *p ) {
    p->add_msg_if_player( _( "You open up your kit and shave." ) );
    p->add_morale( MORALE_SHAVE, 8, 8, 240_minutes, 3_minutes );
    act->set_to_null();
}

void activity_handlers::haircut_finish( player_activity *act, player *p ) {
    p->add_msg_if_player( _( "You give your hair a trim." ) );
    p->add_morale( MORALE_HAIRCUT, 3, 3, 480_minutes, 3_minutes );
    act->set_to_null();
}
