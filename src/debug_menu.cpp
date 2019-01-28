#include "debug_menu.h"

#include <algorithm>
#include <chrono>
#include <vector>

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
             npc_attitude_name( np->get_attitude() ) << std::endl;
        if( np->has_destination() ) {
            data << string_format( _( "Destination: %d:%d:%d (%s)" ),
                                   np->goal.x, np->goal.y, np->goal.z,
                                   overmap_buffer.ter( np->goal )->get_name().c_str() ) << std::endl;
        } else {
            data << _( "No destination." ) << std::endl;
        }
        data << string_format( _( "Trust: %d" ), np->op_of_u.trust ) << " "
             << string_format( _( "Fear: %d" ), np->op_of_u.fear ) << " "
             << string_format( _( "Value: %d" ), np->op_of_u.value ) << " "
             << string_format( _( "Anger: %d" ), np->op_of_u.anger ) << " "
             << string_format( _( "Owed: %d" ), np->op_of_u.owed ) << std::endl;

        data << string_format( _( "Aggression: %d" ), int( np->personality.aggression ) ) << " "
             << string_format( _( "Bravery: %d" ), int( np->personality.bravery ) ) << " "
             << string_format( _( "Collector: %d" ), int( np->personality.collector ) ) << " "
             << string_format( _( "Altruism: %d" ), int( np->personality.altruism ) ) << std::endl;

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
           D_TELE, D_MUTATE, D_CLASS
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
            smenu.addentry( 1, true, 's', "%s: %d", _( "Starvation" ), p.get_starvation() );
            smenu.addentry( 2, true, 't', "%s: %d", _( "Thirst" ), p.get_thirst() );
            smenu.addentry( 3, true, 'f', "%s: %d", _( "Fatigue" ), p.get_fatigue() );
            smenu.addentry( 4, true, 'd', "%s: %d", _( "Sleep Deprivation" ), p.get_sleep_deprivation() );

            const auto &vits = vitamin::all();
            for( const auto &v : vits ) {
                smenu.addentry( -1, true, 0, "%s: %d", v.second.name().c_str(), p.vitamin_get( v.first ) );
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
                    if( query_int( value, _( "Set starvation to? Currently: %d" ), p.get_starvation() ) ) {
                        p.set_starvation( value );
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

                default:
                    if( smenu.ret > 3 && smenu.ret < static_cast<int>( vits.size() + 4 ) ) {
                        auto iter = std::next( vits.begin(), smenu.ret - 4 );
                        if( query_int( value, _( "Set %s to? Currently: %d" ),
                                       iter->second.name().c_str(), p.vitamin_get( iter->first ) ) ) {
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
            add_msg( _( "Removing from %s missions_assigned" ), giver->name.c_str() );
        }
        if( remove_from_vec( giver->chatbin.missions, &m ) ) {
            add_msg( _( "Removing from %s missions" ), giver->name.c_str() );
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
#ifndef __ANDROID__
                               get_option<std::string>( "RENDERER" ) << " | " <<
#else
                               get_option<bool>( "SOFTWARE_RENDERING" ) << " | " <<
#endif
                               get_option<bool>( "FRAMEBUFFER_ACCEL" ) << " | " <<
                               get_option<bool>( "USE_COLOR_MODULATED_TEXTURES" ) << " | " <<
                               int( 1000.0 * draw_counter / ( double )difference ) << " |\n";

    add_msg( m_info, _( "Drew %d times in %.3f seconds. (%.3f fps average)" ), draw_counter,
             difference / 1000.0, 1000.0 * draw_counter / ( double )difference );
}

}
