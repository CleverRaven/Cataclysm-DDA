#include "debug_menu.h"

#include <stddef.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <array>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "action.h"
#include "coordinate_conversions.h"
#include "game.h"
#include "messages.h"
#include "mission.h"
#include "morale_types.h"
#include "npc.h"
#include "npc_class.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "player.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "ui.h"
#include "vitamin.h"
#include "color.h"
#include "debug.h"
#include "enums.h"
#include "faction.h"
#include "game_constants.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "omdata.h"
#include "optional.h"
#include "pldata.h"
#include "translations.h"

#include "map.h"
#include "veh_type.h"
#include "enums.h"
#include "weather.h"
#include "recipe_dictionary.h"
#include "martialarts.h"
#include "item.h"
#include "sounds.h"
#include "trait_group.h"
#include "map_extras.h"
#include "artifact.h"
#include "vpart_position.h"
#include "rng.h"

namespace debug_menu
{

class mission_debug
{
    private:
        // Doesn't actually "destroy" the mission, just removes assignments
        static void remove_mission( mission &m );
    public:
        static void edit_mission( mission &m );
        static void edit( player &who );
        static void edit_player();
        static void edit_npc( npc &who );
        static std::string describe( const mission &m );
};

void teleport_short()
{
    const cata::optional<tripoint> where = g->look_around();
    if( !where || *where == g->u.pos() ) {
        return;
    }
    g->place_player( *where );
    const tripoint new_pos( g->u.pos() );
    add_msg( _( "You teleport to point (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}

void teleport_long()
{
    const tripoint where( ui::omap::choose_point() );
    if( where == overmap::invalid_tripoint ) {
        return;
    }
    g->place_player_overmap( where );
    add_msg( _( "You teleport to submap (%d,%d,%d)." ), where.x, where.y, where.z );
}

void teleport_overmap()
{
    const cata::optional<tripoint> dir_ = choose_direction( _( "Where is the desired overmap?" ) );
    if( !dir_ ) {
        return;
    }

    const tripoint offset( OMAPX * dir_->x, OMAPY * dir_->y, dir_->z );
    const tripoint where( g->u.global_omt_location() + offset );

    g->place_player_overmap( where );

    const tripoint new_pos( omt_to_om_copy( g->u.global_omt_location() ) );
    add_msg( _( "You teleport to overmap (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}

void character_edit_menu()
{
    std::vector< tripoint > locations;
    uilist charmenu;
    int charnum = 0;
    charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, "%s", _( "You" ) );
    locations.emplace_back( g->u.pos() );
    for( const npc &guy : g->all_npcs() ) {
        charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, guy.name );
        locations.emplace_back( guy.pos() );
    }

    pointmenu_cb callback( locations );
    charmenu.callback = &callback;
    charmenu.w_y = 0;
    charmenu.query();
    if( charmenu.ret < 0 || static_cast<size_t>( charmenu.ret ) >= locations.size() ) {
        return;
    }
    const size_t index = charmenu.ret;
    // The NPC is also required for "Add mission", so has to be in this scope
    npc *np = g->critter_at<npc>( locations[index] );
    player &p = np ? *np : g->u;
    uilist nmenu;

    if( np != nullptr ) {
        std::stringstream data;
        data << np->name << " " << ( np->male ? _( "Male" ) : _( "Female" ) ) << std::endl;
        data << np->myclass.obj().get_name() << "; " <<
             npc_attitude_name( np->get_attitude() ) << "; " <<
             ( np->my_fac ? np->my_fac->name : _( "no faction" ) ) << "; " <<
             "api: " << np->get_faction_ver() << std::endl;
        if( np->has_destination() ) {
            data << string_format( _( "Destination: %d:%d:%d (%s)" ),
                                   np->goal.x, np->goal.y, np->goal.z,
                                   overmap_buffer.ter( np->goal )->get_name() ) << std::endl;
        } else {
            data << _( "No destination." ) << std::endl;
        }
        data << string_format( _( "Trust: %d" ), np->op_of_u.trust ) << " "
             << string_format( _( "Fear: %d" ), np->op_of_u.fear ) << " "
             << string_format( _( "Value: %d" ), np->op_of_u.value ) << " "
             << string_format( _( "Anger: %d" ), np->op_of_u.anger ) << " "
             << string_format( _( "Owed: %d" ), np->op_of_u.owed ) << std::endl;

        data << string_format( _( "Aggression: %d" ),
                               static_cast<int>( np->personality.aggression ) ) << " "
             << string_format( _( "Bravery: %d" ), static_cast<int>( np->personality.bravery ) ) << " "
             << string_format( _( "Collector: %d" ), static_cast<int>( np->personality.collector ) ) << " "
             << string_format( _( "Altruism: %d" ), static_cast<int>( np->personality.altruism ) ) << std::endl;

        data << _( "Needs:" ) << std::endl;
        for( const auto &need : np->needs ) {
            data << need << std::endl;
        }
        data << string_format( _( "Total morale: %d" ), np->get_morale_level() ) << std::endl;

        nmenu.text = data.str();
    } else {
        nmenu.text = _( "Player" );
    }

    enum { D_NAME, D_SKILLS, D_STATS, D_ITEMS, D_DELETE_ITEMS, D_ITEM_WORN,
           D_HP, D_MORALE, D_PAIN, D_NEEDS, D_HEALTHY, D_STATUS, D_MISSION_ADD, D_MISSION_EDIT,
           D_TELE, D_MUTATE, D_CLASS, D_ATTITUDE
         };
    nmenu.addentry( D_NAME, true, 'N', "%s", _( "Edit [N]ame" ) );
    nmenu.addentry( D_SKILLS, true, 's', "%s", _( "Edit [s]kills" ) );
    nmenu.addentry( D_STATS, true, 't', "%s", _( "Edit s[t]ats" ) );
    nmenu.addentry( D_ITEMS, true, 'i', "%s", _( "Grant [i]tems" ) );
    nmenu.addentry( D_DELETE_ITEMS, true, 'd', "%s", _( "[d]elete (all) items" ) );
    nmenu.addentry( D_ITEM_WORN, true, 'w', "%s",
                    _( "[w]ear/[w]ield an item from player's inventory" ) );
    nmenu.addentry( D_HP, true, 'h', "%s", _( "Set [h]it points" ) );
    nmenu.addentry( D_MORALE, true, 'o', "%s", _( "Set m[o]rale" ) );
    nmenu.addentry( D_PAIN, true, 'p', "%s", _( "Cause [p]ain" ) );
    nmenu.addentry( D_HEALTHY, true, 'a', "%s", _( "Set he[a]lth" ) );
    nmenu.addentry( D_NEEDS, true, 'n', "%s", _( "Set [n]eeds" ) );
    nmenu.addentry( D_MUTATE, true, 'u', "%s", _( "M[u]tate" ) );
    nmenu.addentry( D_STATUS, true, '@', "%s", _( "Status Window [@]" ) );
    nmenu.addentry( D_TELE, true, 'e', "%s", _( "t[e]leport" ) );
    nmenu.addentry( D_MISSION_EDIT, true, 'M', "%s", _( "Edit [M]issions (WARNING: Unstable!)" ) );
    if( p.is_npc() ) {
        nmenu.addentry( D_MISSION_ADD, true, 'm', "%s", _( "Add [m]ission" ) );
        nmenu.addentry( D_CLASS, true, 'c', "%s", _( "Randomize with [c]lass" ) );
        nmenu.addentry( D_ATTITUDE, true, 'A', "%s", _( "Set [A]ttitude" ) );
    }
    nmenu.query();
    switch( nmenu.ret ) {
        case D_SKILLS:
            wishskill( &p );
            break;
        case D_STATS: {
            uilist smenu;
            smenu.addentry( 0, true, 'S', "%s: %d", _( "Maximum strength" ), p.str_max );
            smenu.addentry( 1, true, 'D', "%s: %d", _( "Maximum dexterity" ), p.dex_max );
            smenu.addentry( 2, true, 'I', "%s: %d", _( "Maximum intelligence" ), p.int_max );
            smenu.addentry( 3, true, 'P', "%s: %d", _( "Maximum perception" ), p.per_max );
            smenu.query();
            int *bp_ptr = nullptr;
            switch( smenu.ret ) {
                case 0:
                    bp_ptr = &p.str_max;
                    break;
                case 1:
                    bp_ptr = &p.dex_max;
                    break;
                case 2:
                    bp_ptr = &p.int_max;
                    break;
                case 3:
                    bp_ptr = &p.per_max;
                    break;
                default:
                    break;
            }

            if( bp_ptr != nullptr ) {
                int value;
                if( query_int( value, _( "Set the stat to? Currently: %d" ), *bp_ptr ) && value >= 0 ) {
                    *bp_ptr = value;
                    p.reset_stats();
                }
            }
        }
        break;
        case D_ITEMS:
            wishitem( &p );
            break;
        case D_DELETE_ITEMS:
            if( !query_yn( _( "Delete all items from the target?" ) ) ) {
                break;
            }
            for( auto &it : p.worn ) {
                it.on_takeoff( p );
            }
            p.worn.clear();
            p.inv.clear();
            p.weapon = item();
            break;
        case D_ITEM_WORN: {
            int item_pos = g->inv_for_all( _( "Make target equip" ) );
            item &to_wear = g->u.i_at( item_pos );
            if( to_wear.is_armor() ) {
                p.on_item_wear( to_wear );
                p.worn.push_back( to_wear );
            } else if( !to_wear.is_null() ) {
                p.weapon = to_wear;
            }
        }
        break;
        case D_HP: {
            uilist smenu;
            smenu.addentry( 0, true, 'q', "%s: %d", _( "Torso" ), p.hp_cur[hp_torso] );
            smenu.addentry( 1, true, 'w', "%s: %d", _( "Head" ), p.hp_cur[hp_head] );
            smenu.addentry( 2, true, 'a', "%s: %d", _( "Left arm" ), p.hp_cur[hp_arm_l] );
            smenu.addentry( 3, true, 's', "%s: %d", _( "Right arm" ), p.hp_cur[hp_arm_r] );
            smenu.addentry( 4, true, 'z', "%s: %d", _( "Left leg" ), p.hp_cur[hp_leg_l] );
            smenu.addentry( 5, true, 'x', "%s: %d", _( "Right leg" ), p.hp_cur[hp_leg_r] );
            smenu.query();
            int *bp_ptr = nullptr;
            switch( smenu.ret ) {
                case 0:
                    bp_ptr = &p.hp_cur[hp_torso];
                    break;
                case 1:
                    bp_ptr = &p.hp_cur[hp_head];
                    break;
                case 2:
                    bp_ptr = &p.hp_cur[hp_arm_l];
                    break;
                case 3:
                    bp_ptr = &p.hp_cur[hp_arm_r];
                    break;
                case 4:
                    bp_ptr = &p.hp_cur[hp_leg_l];
                    break;
                case 5:
                    bp_ptr = &p.hp_cur[hp_leg_r];
                    break;
                default:
                    break;
            }

            if( bp_ptr != nullptr ) {
                int value;
                if( query_int( value, _( "Set the hitpoints to? Currently: %d" ), *bp_ptr ) && value >= 0 ) {
                    *bp_ptr = value;
                    p.reset_stats();
                }
            }
        }
        break;
        case D_MORALE: {
            int current_morale_level = p.get_morale_level();
            int value;
            if( query_int( value, _( "Set the morale to? Currently: %d" ), current_morale_level ) ) {
                int morale_level_delta = value - current_morale_level;
                p.add_morale( MORALE_PERM_DEBUG, morale_level_delta );
                p.apply_persistent_morale();
            }
        }
        break;
        case D_NAME: {
            std::string filterstring = p.name;
            string_input_popup popup;
            popup
            .title( _( "Rename:" ) )
            .width( 85 )
            .description( string_format( _( "NPC: \n%s\n" ), p.name ) )
            .edit( filterstring );
            if( popup.confirmed() ) {
                p.name = filterstring;
            }
        }
        break;
        case D_PAIN: {
            int value;
            if( query_int( value, _( "Cause how much pain? pain: %d" ), p.get_pain() ) ) {
                p.mod_pain( value );
            }
        }
        break;
        case D_NEEDS: {
            uilist smenu;
            smenu.addentry( 0, true, 'h', "%s: %d", _( "Hunger" ), p.get_hunger() );
            smenu.addentry( 1, true, 's', "%s: %d", _( "Stored kCal" ), p.get_stored_kcal() );
            smenu.addentry( 2, true, 't', "%s: %d", _( "Thirst" ), p.get_thirst() );
            smenu.addentry( 3, true, 'f', "%s: %d", _( "Fatigue" ), p.get_fatigue() );
            smenu.addentry( 4, true, 'd', "%s: %d", _( "Sleep Deprivation" ), p.get_sleep_deprivation() );
            smenu.addentry( 5, true, 'a', _( "Reset all basic needs" ) );

            const auto &vits = vitamin::all();
            for( const auto &v : vits ) {
                smenu.addentry( -1, true, 0, "%s: %d", v.second.name(), p.vitamin_get( v.first ) );
            }

            smenu.query();
            int value;
            switch( smenu.ret ) {
                case 0:
                    if( query_int( value, _( "Set hunger to? Currently: %d" ), p.get_hunger() ) ) {
                        p.set_hunger( value );
                    }
                    break;

                case 1:
                    if( query_int( value, _( "Set stored kCal to? Currently: %d" ), p.get_stored_kcal() ) ) {
                        p.set_stored_kcal( value );
                    }
                    break;

                case 2:
                    if( query_int( value, _( "Set thirst to? Currently: %d" ), p.get_thirst() ) ) {
                        p.set_thirst( value );
                    }
                    break;

                case 3:
                    if( query_int( value, _( "Set fatigue to? Currently: %d" ), p.get_fatigue() ) ) {
                        p.set_fatigue( value );
                    }
                    break;

                case 4:
                    if( query_int( value, _( "Set sleep deprivation to? Currently: %d" ),
                                   p.get_sleep_deprivation() ) ) {
                        p.set_sleep_deprivation( value );
                    }
                    break;
                case 5:
                    p.initialize_stomach_contents();
                    p.set_hunger( 0 );
                    p.set_thirst( 0 );
                    p.set_fatigue( 0 );
                    p.set_sleep_deprivation( 0 );
                    p.set_stored_kcal( p.get_healthy_kcal() );
                    break;
                default:
                    if( smenu.ret >= 6 && smenu.ret < static_cast<int>( vits.size() + 6 ) ) {
                        auto iter = std::next( vits.begin(), smenu.ret - 6 );
                        if( query_int( value, _( "Set %s to? Currently: %d" ),
                                       iter->second.name(), p.vitamin_get( iter->first ) ) ) {
                            p.vitamin_set( iter->first, value );
                        }
                    }
            }

        }
        break;
        case D_MUTATE:
            wishmutate( &p );
            break;
        case D_HEALTHY: {
            uilist smenu;
            smenu.addentry( 0, true, 'h', "%s: %d", _( "Health" ), p.get_healthy() );
            smenu.addentry( 1, true, 'm', "%s: %d", _( "Health modifier" ), p.get_healthy_mod() );
            smenu.addentry( 2, true, 'r', "%s: %d", _( "Radiation" ), p.radiation );
            smenu.query();
            int value;
            switch( smenu.ret ) {
                case 0:
                    if( query_int( value, _( "Set the value to? Currently: %d" ), p.get_healthy() ) ) {
                        p.set_healthy( value );
                    }
                    break;
                case 1:
                    if( query_int( value, _( "Set the value to? Currently: %d" ), p.get_healthy_mod() ) ) {
                        p.set_healthy_mod( value );
                    }
                    break;
                case 2:
                    if( query_int( value, _( "Set the value to? Currently: %d" ), p.radiation ) ) {
                        p.radiation = value;
                    }
                    break;
                default:
                    break;
            }
        }
        break;
        case D_STATUS:
            p.disp_info();
            break;
        case D_MISSION_ADD: {
            uilist types;
            types.text = _( "Choose mission type" );
            const auto all_missions = mission_type::get_all();
            std::vector<const mission_type *> mts;
            for( size_t i = 0; i < all_missions.size(); i++ ) {
                types.addentry( i, true, -1, all_missions[ i ].name );
                mts.push_back( &all_missions[ i ] );
            }

            types.query();
            if( types.ret >= 0 && types.ret < static_cast<int>( mts.size() ) ) {
                np->add_new_mission( mission::reserve_new( mts[ types.ret ]->id, np->getID() ) );
            }
        }
        break;
        case D_MISSION_EDIT:
            mission_debug::edit( p );
            break;
        case D_TELE: {
            if( const cata::optional<tripoint> newpos = g->look_around() ) {
                p.setpos( *newpos );
                if( p.is_player() ) {
                    g->update_map( g->u );
                }
            }
        }
        break;
        case D_CLASS: {
            uilist classes;
            classes.text = _( "Choose new class" );
            std::vector<npc_class_id> ids;
            size_t i = 0;
            for( auto &cl : npc_class::get_all() ) {
                ids.push_back( cl.id );
                classes.addentry( i, true, -1, cl.get_name() );
                i++;
            }

            classes.query();
            if( classes.ret < static_cast<int>( ids.size() ) && classes.ret >= 0 ) {
                np->randomize( ids[ classes.ret ] );
            }
        }
        break;
        case D_ATTITUDE: {
            uilist attitudes_ui;
            attitudes_ui.text = _( "Choose new attitude" );
            std::vector<npc_attitude> attitudes;
            for( int i = NPCATT_NULL; i < NPCATT_END; i++ ) {
                npc_attitude att_id = static_cast<npc_attitude>( i );
                std::string att_name = npc_attitude_name( att_id );
                attitudes.push_back( att_id );
                if( att_name == _( "Unknown attitude" ) ) {
                    continue;
                }

                attitudes_ui.addentry( i, true, -1, att_name );
            }

            attitudes_ui.query();
            if( attitudes_ui.ret < static_cast<int>( attitudes.size() ) && attitudes_ui.ret >= 0 ) {
                np->set_attitude( attitudes[attitudes_ui.ret] );
            }
        }
    }
}

const std::string &mission_status_string( mission::mission_status status )
{
    static const std::map<mission::mission_status, std::string> desc {{
            { mission::mission_status::yet_to_start, _( "Yet to start" ) },
            { mission::mission_status::in_progress, _( "In progress" ) },
            { mission::mission_status::success, _( "Success" ) },
            { mission::mission_status::failure, _( "Failure" ) }
        }
    };

    const auto &iter = desc.find( status );
    if( iter != desc.end() ) {
        return iter->second;
    }

    static const std::string errmsg = _( "Bugged" );
    return errmsg;
}

std::string mission_debug::describe( const mission &m )
{
    std::stringstream data;
    data << _( "Type:" ) << m.type->id.str();
    data << _( " Status:" ) << mission_status_string( m.status );
    data << _( " ID:" ) << m.uid;
    data << _( " NPC ID:" ) << m.npc_id;
    data << _( " Target:" ) << m.target.x << "," << m.target.y << "," << m.target.z;
    data << _( "Player ID:" ) << m.player_id;

    return data.str();
}

void add_header( uilist &mmenu, const std::string &str )
{
    if( !mmenu.entries.empty() ) {
        mmenu.addentry( -1, false, -1, "" );
    }
    uilist_entry header( -1, false, -1, str, c_yellow, c_yellow );
    header.force_color = true;
    mmenu.entries.push_back( header );
}

void mission_debug::edit( player &who )
{
    if( who.is_player() ) {
        edit_player();
    } else if( who.is_npc() ) {
        edit_npc( dynamic_cast<npc &>( who ) );
    }
}

void mission_debug::edit_npc( npc &who )
{
    npc_chatbin &bin = who.chatbin;
    std::vector<mission *> all_missions;

    uilist mmenu;
    mmenu.text = _( "Select mission to edit" );

    add_header( mmenu, _( "Currently assigned missions:" ) );
    for( mission *m : bin.missions_assigned ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    add_header( mmenu, _( "Not assigned missions:" ) );
    for( mission *m : bin.missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    mmenu.query();
    if( mmenu.ret < 0 || mmenu.ret >= static_cast<int>( all_missions.size() ) ) {
        return;
    }

    edit_mission( *all_missions[ mmenu.ret ] );
}

void mission_debug::edit_player()
{
    std::vector<mission *> all_missions;

    uilist mmenu;
    mmenu.text = _( "Select mission to edit" );

    add_header( mmenu, _( "Active missions:" ) );
    for( mission *m : g->u.active_missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    add_header( mmenu, _( "Completed missions:" ) );
    for( mission *m : g->u.completed_missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    add_header( mmenu, _( "Failed missions:" ) );
    for( mission *m : g->u.failed_missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    mmenu.query();
    if( mmenu.ret < 0 || mmenu.ret >= static_cast<int>( all_missions.size() ) ) {
        return;
    }

    edit_mission( *all_missions[ mmenu.ret ] );
}

bool remove_from_vec( std::vector<mission *> &vec, mission *m )
{
    auto iter = std::remove( vec.begin(), vec.end(), m );
    bool ret = iter != vec.end();
    vec.erase( iter, vec.end() );
    return ret;
}

void mission_debug::remove_mission( mission &m )
{
    if( remove_from_vec( g->u.active_missions, &m ) ) {
        add_msg( _( "Removing from active_missions" ) );
    }
    if( remove_from_vec( g->u.completed_missions, &m ) ) {
        add_msg( _( "Removing from completed_missions" ) );
    }
    if( remove_from_vec( g->u.failed_missions, &m ) ) {
        add_msg( _( "Removing from failed_missions" ) );
    }

    if( g->u.active_mission == &m ) {
        g->u.active_mission = nullptr;
        add_msg( _( "Unsetting active mission" ) );
    }

    const auto giver = g->find_npc( m.npc_id );
    if( giver != nullptr ) {
        if( remove_from_vec( giver->chatbin.missions_assigned, &m ) ) {
            add_msg( _( "Removing from %s missions_assigned" ), giver->name );
        }
        if( remove_from_vec( giver->chatbin.missions, &m ) ) {
            add_msg( _( "Removing from %s missions" ), giver->name );
        }
    }
}

void mission_debug::edit_mission( mission &m )
{
    uilist mmenu;
    mmenu.text = describe( m );

    enum { M_FAIL, M_SUCCEED, M_REMOVE
         };

    mmenu.addentry( M_FAIL, true, 'f', "%s", _( "Fail mission" ) );
    mmenu.addentry( M_SUCCEED, true, 'c', "%s", _( "Mark as complete" ) );
    mmenu.addentry( M_REMOVE, true, 'r', "%s", _( "Remove mission without proper cleanup" ) );

    mmenu.query();
    switch( mmenu.ret ) {
        case M_FAIL:
            m.fail();
            break;
        case M_SUCCEED:
            m.status = mission::mission_status::success;
            break;
        case M_REMOVE:
            remove_mission( m );
            break;
    }
}

void draw_benchmark( const int max_difference )
{
    // call the draw procedure as many times as possible in max_difference milliseconds
    auto start_tick = std::chrono::steady_clock::now();
    auto end_tick = std::chrono::steady_clock::now();
    long difference = 0;
    int draw_counter = 0;
    while( true ) {
        end_tick = std::chrono::steady_clock::now();
        difference = std::chrono::duration_cast<std::chrono::milliseconds>( end_tick - start_tick ).count();
        if( difference >= max_difference ) {
            break;
        }
        g->draw();
        draw_counter++;
    }

    DebugLog( D_INFO, DC_ALL ) << "Draw benchmark:\n" <<
                               "\n| USE_TILES |  RENDERER | FRAMEBUFFER_ACCEL | USE_COLOR_MODULATED_TEXTURES | FPS |" <<
                               "\n|:---:|:---:|:---:|:---:|:---:|\n| " <<
                               get_option<bool>( "USE_TILES" ) << " | " <<
#if !defined(__ANDROID__)
                               get_option<std::string>( "RENDERER" ) << " | " <<
#else
                               get_option<bool>( "SOFTWARE_RENDERING" ) << " | " <<
#endif
                               get_option<bool>( "FRAMEBUFFER_ACCEL" ) << " | " <<
                               get_option<bool>( "USE_COLOR_MODULATED_TEXTURES" ) << " | " <<
                               static_cast<int>( 1000.0 * draw_counter / static_cast<double>( difference ) ) << " |\n";

    add_msg( m_info, _( "Drew %d times in %.3f seconds. (%.3f fps average)" ), draw_counter,
             difference / 1000.0, 1000.0 * draw_counter / static_cast<double>( difference ) );
}

void debug()
{
    int action = uilist(
    _( "Debug Functions - Using these will cheat not only the game, but yourself.  You won't grow. You won't improve.\nTaking this shortcut will gain you nothing. Your victory will be hollow.\nNothing will be risked and nothing will be gained." ), {
        _( "Wish for an item" ),                // 0
        _( "Teleport - Short Range" ),          // 1
        _( "Teleport - Long Range" ),           // 2
        _( "Reveal map" ),                      // 3
        _( "Spawn NPC" ),                       // 4
        _( "Spawn Monster" ),                   // 5
        _( "Check game state..." ),             // 6
        _( "Kill NPCs" ),                       // 7
        _( "Mutate" ),                          // 8
        _( "Spawn a vehicle" ),                 // 9
        _( "Change all skills" ),               // 10
        _( "Learn all melee styles" ),          // 11
        _( "Unlock all recipes" ),              // 12
        _( "Edit player/NPC" ),                 // 13
        _( "Spawn Artifact" ),                  // 14
        _( "Spawn Clairvoyance Artifact" ),     // 15
        _( "Map editor" ),                      // 16
        _( "Change weather" ),                  // 17
        _( "Change wind direction" ),           // 18
        _( "Change wind speed" ),               // 19
        _( "Kill all monsters" ),               // 20
        _( "Display hordes" ),                  // 21
        _( "Test Item Group" ),                 // 22
        _( "Damage Self" ),                     // 23
        _( "Show Sound Clustering" ),           // 24
        _( "Display weather" ),                 // 25
        _( "Display overmap scents" ),          // 26
        _( "Change time" ),                     // 27
        _( "Set automove route" ),              // 28
        _( "Show mutation category levels" ),   // 29
        _( "Overmap editor" ),                  // 30
        _( "Draw benchmark (X seconds)" ),      // 31
        _( "Teleport - Adjacent overmap" ),     // 32
        _( "Test trait group" ),                // 33
        _( "Show debug message" ),              // 34
        _( "Crash game (test crash handling)" ),// 35
        _( "Spawn Map Extra" ),                 // 36
        _( "Toggle NPC pathfinding on map" ),   // 37
        _( "Quit to Main Menu" ),               // 38
    } );
    g->refresh_all();
    player &u = g->u;
    map &m = g->m;
    switch( action ) {
        case 0:
            debug_menu::wishitem( &u );
            break;

        case 1:
            debug_menu::teleport_short();
            break;

        case 2:
            debug_menu::teleport_long();
            break;

        case 3: {
            auto &cur_om = g->get_cur_om();
            for( int i = 0; i < OMAPX; i++ ) {
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                        cur_om.seen( i, j, k ) = true;
                    }
                }
            }
            add_msg( m_good, _( "Current overmap revealed." ) );
        }
        break;

        case 4: {
            std::shared_ptr<npc> temp = std::make_shared<npc>();
            temp->normalize();
            temp->randomize();
            temp->spawn_at_precise( { g->get_levx(), g->get_levy() }, u.pos() + point( -4, -4 ) );
            overmap_buffer.insert_npc( temp );
            temp->form_opinion( u );
            temp->mission = NPC_MISSION_NULL;
            temp->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, temp->global_omt_location(),
                                   temp->getID() ) );
            g->load_npcs();
        }
        break;

        case 5:
            debug_menu::wishmonster( cata::nullopt );
            break;

        case 6: {
            std::string s = _( "Location %d:%d in %d:%d, %s\n" );
            s += _( "Current turn: %d.\n%s\n" );
            s += ngettext( "%d creature exists.\n", "%d creatures exist.\n", g->num_creatures() );
            popup_top(
                s.c_str(),
                u.posx(), g->u.posy(), g->get_levx(), g->get_levy(),
                overmap_buffer.ter( g->u.global_omt_location() )->get_name(),
                static_cast<int>( calendar::turn ),
                ( get_option<bool>( "RANDOM_NPC" ) ? _( "NPCs are going to spawn." ) :
                  _( "NPCs are NOT going to spawn." ) ),
                g->num_creatures() );
            for( const npc &guy : g->all_npcs() ) {
                tripoint t = guy.global_sm_location();
                add_msg( m_info, _( "%s: map ( %d:%d ) pos ( %d:%d )" ), guy.name, t.x,
                         t.y, guy.posx(), guy.posy() );
            }

            add_msg( m_info, _( "(you: %d:%d)" ), u.posx(), u.posy() );
            std::string stom =
                _( "Stomach Contents: %d ml / %d ml kCal: %d, Water: %d ml" );
            add_msg( m_info, stom.c_str(), units::to_milliliter( u.stomach.contains() ),
                     units::to_milliliter( u.stomach.capacity() ), u.stomach.get_calories(),
                     units::to_milliliter( u.stomach.get_water() ), u.get_hunger() );
            stom = _( "Guts Contents: %d ml / %d ml kCal: %d, Water: %d ml\nHunger: %d, Thirst: %d, kCal: %d / %d" );
            add_msg( m_info, stom.c_str(), units::to_milliliter( u.guts.contains() ),
                     units::to_milliliter( u.guts.capacity() ), u.guts.get_calories(),
                     units::to_milliliter( u.guts.get_water() ), u.get_hunger(), u.get_thirst(), u.get_stored_kcal(),
                     u.get_healthy_kcal() );
            g->disp_NPCs();
            break;
        }
        case 7:
            for( npc &guy : g->all_npcs() ) {
                add_msg( _( "%s's head implodes!" ), guy.name );
                guy.hp_cur[bp_head] = 0;
            }
            break;

        case 8:
            debug_menu::wishmutate( &u );
            break;

        case 9:
            if( m.veh_at( u.pos() ) ) {
                dbg( D_ERROR ) << "game:load: There's already vehicle here";
                debugmsg( "There's already vehicle here" );
            } else {
                std::vector<vproto_id> veh_strings;
                uilist veh_menu;
                veh_menu.text = _( "Choose vehicle to spawn" );
                int menu_ind = 0;
                for( auto &elem : vehicle_prototype::get_all() ) {
                    if( elem != vproto_id( "custom" ) ) {
                        const vehicle_prototype &proto = elem.obj();
                        veh_strings.push_back( elem );
                        //~ Menu entry in vehicle wish menu: 1st string: displayed name, 2nd string: internal name of vehicle
                        veh_menu.addentry( menu_ind, true, MENU_AUTOASSIGN, _( "%1$s (%2$s)" ), _( proto.name ),
                                           elem.c_str() );
                        ++menu_ind;
                    }
                }
                veh_menu.query();
                if( veh_menu.ret >= 0 && veh_menu.ret < static_cast<int>( veh_strings.size() ) ) {
                    //Didn't cancel
                    const vproto_id &selected_opt = veh_strings[veh_menu.ret];
                    tripoint dest = u.pos(); // TODO: Allow picking this when add_vehicle has 3d argument
                    vehicle *veh = m.add_vehicle( selected_opt, dest.x, dest.y, -90, 100, 0 );
                    if( veh != nullptr ) {
                        m.board_vehicle( u.pos(), &u );
                    }
                }
            }
            break;

        case 10: {
            debug_menu::wishskill( &u );
        }
        break;

        case 11:
            add_msg( m_info, _( "Martial arts debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( auto &style : all_martialart_types() ) {
                if( style != matype_id( "style_none" ) ) {
                    u.add_martialart( style );
                }
            }
            add_msg( m_good, _( "You now know a lot more than just 10 styles of kung fu." ) );
            break;

        case 12: {
            add_msg( m_info, _( "Recipe debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( const auto &e : recipe_dict ) {
                u.learn_recipe( &e.second );
            }
            add_msg( m_good, _( "You know how to craft that now." ) );
        }
        break;

        case 13:
            debug_menu::character_edit_menu();
            break;

        case 14:
            if( const cata::optional<tripoint> center = g->look_around() ) {
                artifact_natural_property prop = artifact_natural_property( rng( ARTPROP_NULL + 1,
                                                 ARTPROP_MAX - 1 ) );
                m.create_anomaly( *center, prop );
                m.spawn_natural_artifact( *center, prop );
            }
            break;

        case 15:
            u.i_add( item( architects_cube(), calendar::turn ) );
            break;

        case 16: {
            g->look_debug();
        }
        break;

        case 17: {
            uilist weather_menu;
            weather_menu.text = _( "Select new weather pattern:" );
            weather_menu.addentry( 0, true, MENU_AUTOASSIGN, g->weather_override == WEATHER_NULL ?
                                   _( "Keep normal weather patterns" ) : _( "Disable weather forcing" ) );
            for( int weather_id = 1; weather_id < NUM_WEATHER_TYPES; weather_id++ ) {
                weather_menu.addentry( weather_id, true, MENU_AUTOASSIGN,
                                       weather_data( static_cast<weather_type>( weather_id ) ).name );
            }

            weather_menu.query();

            if( weather_menu.ret >= 0 && weather_menu.ret < NUM_WEATHER_TYPES ) {
                weather_type selected_weather = static_cast<weather_type>( weather_menu.ret );
                g->weather_override = selected_weather;
                g->set_nextweather( calendar::turn );
            }
        }
        break;

        case 18: {
            uilist wind_direction_menu;
            wind_direction_menu.text = _( "Select new wind direction:" );
            wind_direction_menu.addentry( 0, true, MENU_AUTOASSIGN, g->wind_direction_override ?
                                          _( "Disable direction forcing" ) : _( "Keep normal wind direction" ) );
            int count = 1;
            for( int angle = 0; angle <= 315; angle += 45 ) {
                wind_direction_menu.addentry( count, true, MENU_AUTOASSIGN, get_wind_arrow( angle ) );
                count += 1;
            }
            wind_direction_menu.query();
            if( wind_direction_menu.ret == 0 ) {
                g->wind_direction_override = cata::nullopt;
            } else if( wind_direction_menu.ret >= 0 && wind_direction_menu.ret < 9 ) {
                g->wind_direction_override = ( wind_direction_menu.ret - 1 ) * 45;
                g->set_nextweather( calendar::turn );
            }
        }
        break;

        case 19: {
            uilist wind_speed_menu;
            wind_speed_menu.text = _( "Select new wind speed:" );
            wind_speed_menu.addentry( 0, true, MENU_AUTOASSIGN, g->wind_direction_override ?
                                      _( "Disable speed forcing" ) : _( "Keep normal wind speed" ) );
            int count = 1;
            for( int speed = 0; speed <= 100; speed += 10 ) {
                std::string speedstring = std::to_string( speed ) + " " + velocity_units( VU_WIND );
                wind_speed_menu.addentry( count, true, MENU_AUTOASSIGN, speedstring );
                count += 1;
            }
            wind_speed_menu.query();
            if( wind_speed_menu.ret == 0 ) {
                g->windspeed_override = cata::nullopt;
            } else if( wind_speed_menu.ret >= 0 && wind_speed_menu.ret < 12 ) {
                int selected_wind_speed = ( wind_speed_menu.ret - 1 ) * 10;
                g->windspeed_override = selected_wind_speed;
                g->set_nextweather( calendar::turn );
            }
        }
        break;

        case 20: {
            for( monster &critter : g->all_monsters() ) {
                // Use the normal death functions, useful for testing death
                // and for getting a corpse.
                critter.die( nullptr );
            }
            g->cleanup_dead();
        }
        break;
        case 21:
            ui::omap::display_hordes();
            break;
        case 22: {
            item_group::debug_spawn();
        }
        break;

        // Damage Self
        case 23: {
            int dbg_damage;
            if( query_int( dbg_damage, _( "Damage self for how much? hp: %d" ), u.hp_cur[hp_torso] ) ) {
                u.hp_cur[hp_torso] -= dbg_damage;
                u.die( nullptr );
            }
        }
        break;

        case 24: {
#if defined(TILES)
        // *INDENT-OFF*
        const point offset{
            POSX - u.posx() + u.view_offset.x,
            POSY - u.posy() + u.view_offset.y
        }; // *INDENT-ON*
        g->draw_ter();
        auto sounds_to_draw = sounds::get_monster_sounds();
        for( const auto &sound : sounds_to_draw.first ) {
            mvwputch( g->w_terrain, offset.y + sound.y, offset.x + sound.x, c_yellow, '?' );
        }
        for( const auto &sound : sounds_to_draw.second ) {
            mvwputch( g->w_terrain, offset.y + sound.y, offset.x + sound.x, c_red, '?' );
        }
        wrefresh( g->w_terrain );
        g->draw_panels();
        inp_mngr.wait_for_any_key();
#else
                popup( _( "This binary was not compiled with tiles support." ) );
#endif
    }
             break;

    case 25:
        ui::omap::display_weather();
        break;
    case 26:
        ui::omap::display_scents();
        break;
    case 27: {
        auto set_turn = [&]( const int initial, const int factor, const char *const msg ) {
            const auto text = string_input_popup()
                .title( msg )
                .width( 20 )
                .text( to_string( initial ) )
                .only_digits( true )
                .query_string();
            if( text.empty() ) {
                return;
            }
            const int new_value = (std::atoi( text.c_str() ) - initial) * factor;
            calendar::turn += std::max( std::min( INT_MAX / 2 - calendar::turn, new_value ),
                -calendar::turn );
        };

        uilist smenu;
        do {
            const int iSel = smenu.ret;
            smenu.reset();
            smenu.addentry( 0, true, 'y', "%s: %d", _( "year" ), calendar::turn.years() );
            smenu.addentry( 1, !calendar::eternal_season(), 's', "%s: %d",
                _( "season" ), static_cast<int>(season_of_year( calendar::turn )) );
            smenu.addentry( 2, true, 'd', "%s: %d", _( "day" ), day_of_season<int>( calendar::turn ) );
            smenu.addentry( 3, true, 'h', "%s: %d", _( "hour" ), hour_of_day<int>( calendar::turn ) );
            smenu.addentry( 4, true, 'm', "%s: %d", _( "minute" ), minute_of_hour<int>( calendar::turn ) );
            smenu.addentry( 5, true, 't', "%s: %d", _( "turn" ), static_cast<int>(calendar::turn) );
            smenu.selected = iSel;
            smenu.query();

            switch( smenu.ret ) {
            case 0:
                set_turn( calendar::turn.years(), to_turns<int>( calendar::year_length() ), _( "Set year to?" ) );
                break;
            case 1:
                set_turn( static_cast<int>(season_of_year( calendar::turn )),
                    to_turns<int>( calendar::turn.season_length() ),
                    _( "Set season to? (0 = spring)" ) );
                break;
            case 2:
                set_turn( day_of_season<int>( calendar::turn ), DAYS( 1 ), _( "Set days to?" ) );
                break;
            case 3:
                set_turn( hour_of_day<int>( calendar::turn ), HOURS( 1 ), _( "Set hour to?" ) );
                break;
            case 4:
                set_turn( minute_of_hour<int>( calendar::turn ), MINUTES( 1 ), _( "Set minute to?" ) );
                break;
            case 5:
                set_turn( calendar::turn, 1,
                    string_format( _( "Set turn to? (One day is %i turns)" ), static_cast<int>(DAYS( 1 )) ).c_str() );
                break;
            default:
                break;
            }
        } while( smenu.ret != UILIST_CANCEL );
    }
             break;
    case 28: {
        const cata::optional<tripoint> dest = g->look_around();
        if( !dest || *dest == u.pos() ) {
            break;
        }

        auto rt = m.route( u.pos(), *dest, u.get_pathfinding_settings(), u.get_path_avoid() );
        if( !rt.empty() ) {
            u.set_destination( rt );
        }
        else {
            popup( "Couldn't find path" );
        }
    }
             break;
    case 29:
        for( const auto &elem : u.mutation_category_level ) {
            add_msg( "%s: %d", elem.first.c_str(), elem.second );
        }
        break;

    case 30:
        ui::omap::display_editor();
        break;

    case 31: {
        const int ms = string_input_popup()
            .title( _( "Enter benchmark length (in milliseconds):" ) )
            .width( 20 )
            .text( "5000" )
            .query_int();
        debug_menu::draw_benchmark( ms );
    }
             break;

    case 32:
        debug_menu::teleport_overmap();
        break;
    case 33:
        trait_group::debug_spawn();
        break;
    case 34:
        debugmsg( "Test debugmsg" );
        break;
    case 35:
        std::raise( SIGSEGV );
        break;
    case 36: {
        oter_id terrain_type = overmap_buffer.ter( g->u.global_omt_location() );

        map_extras ex = region_settings_map["default"].region_extras[terrain_type->get_extras()];
        uilist mx_menu;
        std::vector<std::string> mx_str;
        for( auto &extra : ex.values ) {
            mx_menu.addentry( -1, true, -1, extra.obj );
            mx_str.push_back( extra.obj );
        }
        mx_menu.query( false );
        int mx_choice = mx_menu.ret;
        if( mx_choice >= 0 && mx_choice < static_cast<int>(mx_str.size()) ) {
            auto func = MapExtras::get_function( mx_str[mx_choice] );
            if( func != nullptr ) {
                const tripoint where( ui::omap::choose_point() );
                if( where != overmap::invalid_tripoint ) {
                    tinymap mx_map;
                    mx_map.load( where.x * 2, where.y * 2, where.z, false );
                    func( mx_map, where );
                }
            }
        }
        break;
    }
    case 37:
        g->debug_pathfinding = !g->debug_pathfinding;
        break;
    case 38:
        if( query_yn(
            _( "Quit without saving? This may cause issues such as duplicated or missing items and vehicles!" ) ) ) {
            u.moves = 0;
            g->uquit = QUIT_NOSAVED;
        }
        break;
    }
    catacurses::erase();
    g->refresh_all();
}

}
