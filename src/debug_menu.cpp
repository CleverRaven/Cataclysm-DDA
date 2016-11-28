#include "debug_menu.h"

#include "action.h"
#include "coordinate_conversions.h"
#include "game.h"
#include "messages.h"
#include "overmap.h"
#include "player.h"
#include "ui.h"
#include "npc.h"
#include "npc_class.h"
#include "overmapbuffer.h"
#include "vitamin.h"
#include "mission.h"

void debug_menu::teleport_short()
{
    const tripoint where( g->look_around() );
    if( where == tripoint_min || where == g->u.pos() ) {
        return;
    }
    g->place_player( where );
    const tripoint new_pos( g->u.pos() );
    add_msg( _( "You teleport to point (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}

void debug_menu::teleport_long()
{
    const tripoint where( overmap::draw_overmap() );
    if( where == overmap::invalid_tripoint ) {
        return;
    }
    g->place_player_overmap( where );
    add_msg( _( "You teleport to submap (%d,%d,%d)." ), where.x, where.y, where.z );
}

void debug_menu::teleport_overmap()
{
    tripoint dir;

    if( !choose_direction( _( "Where is the desired overmap?" ), dir ) ) {
        return;
    }

    const tripoint offset( OMAPX * dir.x, OMAPY * dir.y, dir.z );
    const tripoint where( g->u.global_omt_location() + offset );

    g->place_player_overmap( where );

    const tripoint new_pos( omt_to_om_copy( g->u.global_omt_location() ) );
    add_msg( _( "You teleport to overmap (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}

void debug_menu::npc_edit_menu()
{
    std::vector< tripoint > locations;
    uimenu charmenu;
    charmenu.return_invalid = true;
    int charnum = -1;
    charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, "%s", _( "You" ) );
    locations.emplace_back( g->u.pos() );
    for( auto *npc_p : g->active_npc ) {
        charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, "%s", npc_p->name.c_str() );
        locations.emplace_back( npc_p->pos() );
    }

    pointmenu_cb callback( locations );
    charmenu.callback = &callback;
    charmenu.w_y = 0;
    charmenu.query();
    int npcdex = charmenu.ret;
    if( npcdex < -1 || npcdex > charnum ) {
        return;
    }
    player &p = npcdex != -1 ? *g->active_npc[npcdex] : g->u;
    // The NPC is also required for "Add mission", so has to be in this scope
    npc *np = npcdex != -1 ? g->active_npc[npcdex] : nullptr;
    uimenu nmenu;
    nmenu.return_invalid = true;

    if( np != nullptr ) {
        std::stringstream data;
        data << np->name << " " << ( np->male ? _( "Male" ) : _( "Female" ) ) << std::endl;
        data << np->myclass.obj().get_name() << "; " <<
             npc_attitude_name( np->attitude ) << std::endl;
        if( np->has_destination() ) {
            data << string_format( _( "Destination: %d:%d:%d (%s)" ),
                                   np->goal.x, np->goal.y, np->goal.z,
                                   overmap_buffer.ter( np->goal )->name.c_str() ) << std::endl;
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

        nmenu.text = data.str();
    } else {
        nmenu.text = _( "Player" );
    }

    enum { D_SKILLS, D_STATS, D_ITEMS, D_DELETE_ITEMS, D_ITEM_WORN,
           D_HP, D_PAIN, D_NEEDS, D_HEALTHY, D_STATUS, D_MISSION_ADD,
           D_TELE, D_MUTATE, D_CLASS
         };
    nmenu.addentry( D_SKILLS, true, 's', "%s", _( "Edit [s]kills" ) );
    nmenu.addentry( D_STATS, true, 't', "%s", _( "Edit s[t]ats" ) );
    nmenu.addentry( D_ITEMS, true, 'i', "%s", _( "Grant [i]tems" ) );
    nmenu.addentry( D_DELETE_ITEMS, true, 'd', "%s", _( "[d]elete (all) items" ) );
    nmenu.addentry( D_ITEM_WORN, true, 'w', "%s",
                    _( "[w]ear/[w]ield an item from player's inventory" ) );
    nmenu.addentry( D_HP, true, 'h', "%s", _( "Set [h]it points" ) );
    nmenu.addentry( D_PAIN, true, 'p', "%s", _( "Cause [p]ain" ) );
    nmenu.addentry( D_HEALTHY, true, 'a', "%s", _( "Set he[a]lth" ) );
    nmenu.addentry( D_NEEDS, true, 'n', "%s", _( "Set [n]eeds" ) );
    nmenu.addentry( D_MUTATE, true, 'u', "%s", _( "M[u]tate" ) );
    nmenu.addentry( D_STATUS, true, '@', "%s", _( "Status Window [@]" ) );
    nmenu.addentry( D_TELE, true, 'e', "%s", _( "t[e]leport" ) );
    if( p.is_npc() ) {
        nmenu.addentry( D_MISSION_ADD, true, 'm', "%s", _( "Add [m]ission" ) );
        nmenu.addentry( D_CLASS, true, 'c', "%s", _( "Randomize with [c]lass" ) );
    }
    nmenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
    nmenu.selected = 0;
    nmenu.query();
    switch( nmenu.ret ) {
        case D_SKILLS:
            wishskill( &p );
            break;
        case D_STATS: {
            uimenu smenu;
            smenu.return_invalid = true;
            smenu.addentry( 0, true, 'S', "%s: %d", _( "Maximum strength" ), p.str_max );
            smenu.addentry( 1, true, 'D', "%s: %d", _( "Maximum dexterity" ), p.dex_max );
            smenu.addentry( 2, true, 'I', "%s: %d", _( "Maximum intelligence" ), p.int_max );
            smenu.addentry( 3, true, 'P', "%s: %d", _( "Maximum perception" ), p.per_max );
            smenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
            smenu.selected = 0;
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
                if( query_int( value, "Set the stat to? Currently: %d", *bp_ptr ) && value >= 0 ) {
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
            if( !query_yn( "Delete all items from the target?" ) ) {
                break;
            }
            for( auto &it : p.worn ) {
                it.on_takeoff( p );
            }
            p.worn.clear();
            p.inv.clear();
            p.weapon = p.ret_null;
            break;
        case D_ITEM_WORN: {
            int item_pos = g->inv_for_all( "Make target equip" );
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
            uimenu smenu;
            smenu.return_invalid = true;
            smenu.addentry( 0, true, 'q', "%s: %d", _( "Torso" ), p.hp_cur[hp_torso] );
            smenu.addentry( 1, true, 'w', "%s: %d", _( "Head" ), p.hp_cur[hp_head] );
            smenu.addentry( 2, true, 'a', "%s: %d", _( "Left arm" ), p.hp_cur[hp_arm_l] );
            smenu.addentry( 3, true, 's', "%s: %d", _( "Right arm" ), p.hp_cur[hp_arm_r] );
            smenu.addentry( 4, true, 'z', "%s: %d", _( "Left leg" ), p.hp_cur[hp_leg_l] );
            smenu.addentry( 5, true, 'x', "%s: %d", _( "Right leg" ), p.hp_cur[hp_leg_r] );
            smenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
            smenu.selected = 0;
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
                if( query_int( value, "Set the hitpoints to? Currently: %d", *bp_ptr ) && value >= 0 ) {
                    *bp_ptr = value;
                    p.reset_stats();
                }
            }
        }
        break;
        case D_PAIN: {
            int value;
            if( query_int( value, "Cause how much pain? pain: %d", p.get_pain() ) ) {
                p.mod_pain( value );
            }
        }
        break;
        case D_NEEDS: {
            uimenu smenu;
            smenu.return_invalid = true;
            smenu.addentry( 0, true, 'h', "%s: %d", _( "Hunger" ), p.get_hunger() );
            smenu.addentry( 1, true, 't', "%s: %d", _( "Thirst" ), p.get_thirst() );
            smenu.addentry( 2, true, 'f', "%s: %d", _( "Fatigue" ), p.get_fatigue() );

            const auto& vits = vitamin::all();
            for( const auto& v : vits ) {
                smenu.addentry( -1, true, 0, "%s: %d", v.second.name().c_str(), p.vitamin_get( v.first ) );
            }

            smenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
            smenu.selected = 0;
            smenu.query();

            switch( smenu.ret ) {
                int value;
                case 0:
                    if( query_int( value, "Set hunger to? Currently: %d", p.get_hunger() ) ) {
                        p.set_hunger( value );
                    }
                    break;

                case 1:
                    if( query_int( value, "Set thirst to? Currently: %d", p.get_thirst() ) ) {
                        p.set_thirst( value );
                    }
                    break;

                case 2:
                    if( query_int( value, "Set fatigue to? Currently: %d", p.get_fatigue() ) ) {
                        p.set_fatigue( value );
                    }
                    break;

                default:
                    if( smenu.ret > 2 && smenu.ret < static_cast<int>( vits.size() + 3 ) ) {
                        auto iter = std::next( vits.begin(), smenu.ret - 3 );
                        if( query_int( value, "Set %s to? Currently: %d",
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
            uimenu smenu;
            smenu.return_invalid = true;
            smenu.addentry( 0, true, 'h', "%s: %d", _( "Health" ), p.get_healthy() );
            smenu.addentry( 1, true, 'm', "%s: %d", _( "Health modifier" ), p.get_healthy_mod() );
            smenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
            smenu.selected = 0;
            smenu.query();
            switch( smenu.ret ) {
                int value;
                case 0:
                    if( query_int( value, "Set the value to? Currently: %d", p.get_healthy() ) ) {
                        p.set_healthy( value );
                    }
                    break;
                case 1:
                    if( query_int( value, "Set the value to? Currently: %d", p.get_healthy_mod() ) ) {
                        p.set_healthy_mod( value );
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
            uimenu types;
            types.return_invalid = true;
            types.text = _( "Choose mission type" );
            const auto all_missions = mission_type::get_all();
            std::vector<const mission_type *> mts;
            for( size_t i = 0; i < all_missions.size(); i++ ) {
                types.addentry( i, true, -1, all_missions[ i ].name );
                mts.push_back( &all_missions[ i ] );
            }

            types.addentry( INT_MAX, true, -1, _( "Cancel" ) );
            types.query();
            if( types.ret >= 0 && types.ret < (int)mts.size() ) {
                np->add_new_mission( mission::reserve_new( mts[ types.ret ]->id, np->getID() ) );
            }
        }
        break;
        case D_TELE: {
            tripoint newpos = g->look_around();
            if( newpos != tripoint_min ) {
                p.setpos( newpos );
                if( p.is_player() ) {
                    g->update_map( g->u );
                }
            }
        }
        break;
        case D_CLASS: {
            uimenu classes;
            classes.return_invalid = true;
            classes.text = _( "Choose new class" );
            std::vector<npc_class_id> ids;
            size_t i = 0;
            for( auto &cl : npc_class::get_all() ) {
                ids.push_back( cl.id );
                classes.addentry( i, true, -1, cl.get_name() );
                i++;
            }

            classes.addentry( INT_MAX, true, -1, _( "Cancel" ) );
            classes.query();
            if( classes.ret < (int)ids.size() && classes.ret >= 0 ) {
                np->randomize( ids[ classes.ret ] );
            }
        }
    }
}
