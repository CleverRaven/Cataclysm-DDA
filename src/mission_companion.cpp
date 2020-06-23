#include "mission_companion.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "avatar.h"
#include "basecamp.h"
#include "calendar.h"
#include "catacharset.h"
#include "colony.h"
#include "color.h"
#include "compatibility.h" // needed for the workaround for the std::to_string bug in some compilers
#include "coordinate_conversions.h"
#include "creature.h"
#include "cursesdef.h"
#include "debug.h"
#include "enums.h"
#include "faction.h"
#include "faction_camp.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_stack.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "material.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "optional.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pldata.h"
#include "point.h"
#include "rng.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "value_ptr.h"
#include "weather.h"
#include "weighted_list.h"

static const efftype_id effect_riding( "riding" );

static const itype_id itype_fungal_seeds( "fungal_seeds" );
static const itype_id itype_marloss_seed( "marloss_seed" );

static const skill_id skill_bashing( "bashing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_unarmed( "unarmed" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_NPC_MISSION_LEV_1( "NPC_MISSION_LEV_1" );
static const trait_id trait_NPC_CONSTRUCTION_LEV_1( "NPC_CONSTRUCTION_LEV_1" );
static const trait_id trait_NPC_CONSTRUCTION_LEV_2( "NPC_CONSTRUCTION_LEV_2" );

struct comp_rank {
    int industry;
    int combat;
    int survival;
};

mission_data::mission_data()
{
    for( int tab_num = base_camps::TAB_MAIN; tab_num != base_camps::TAB_NW + 3; tab_num++ ) {
        std::vector<mission_entry> k;
        entries.push_back( k );
    }
}

namespace talk_function
{
void scavenger_patrol( mission_data &mission_key, npc &p );
void scavenger_raid( mission_data &mission_key, npc &p );
void commune_menial( mission_data &mission_key, npc &p );
void commune_carpentry( mission_data &mission_key, npc &p );
void commune_farmfield( mission_data &mission_key, npc &p );
void commune_forage( mission_data &mission_key, npc &p );
void commune_refuge_caravan( mission_data &mission_key, npc &p );
bool handle_outpost_mission( const mission_entry &cur_key, npc &p );
} // namespace talk_function

void talk_function::companion_mission( npc &p )
{
    mission_data mission_key;

    std::string role_id = p.companion_mission_role_id;
    const tripoint omt_pos = p.global_omt_location();
    std::string title = _( "Outpost Missions" );
    if( role_id == "SCAVENGER" ) {
        title = _( "Junk Shop Missions" );
        scavenger_patrol( mission_key, p );
        if( p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
            scavenger_raid( mission_key, p );
        }
    } else if( role_id == "COMMUNE CROPS" ) {
        title = _( "Agricultural Missions" );
        commune_farmfield( mission_key, p );
        commune_forage( mission_key, p );
        commune_refuge_caravan( mission_key, p );
    } else if( role_id == "FOREMAN" ) {
        title = _( "Construction Missions" );
        commune_menial( mission_key, p );
        if( p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
            commune_carpentry( mission_key, p );
        }
    } else if( role_id == "REFUGEE MERCHANT" ) {
        title = _( "Free Merchant Missions" );
        commune_refuge_caravan( mission_key, p );
    } else {
        return;
    }
    if( display_and_choose_opts( mission_key, omt_pos, role_id, title ) ) {
        handle_outpost_mission( mission_key.cur_key, p );
    }
}

void talk_function::scavenger_patrol( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: $25-$500\nDanger: Low\nTime: 10 hour missions\n\n"
                           "Assigning one of your allies to patrol the surrounding wilderness "
                           "and isolated buildings presents the opportunity to build survival "
                           "skills while engaging in relatively safe combat against isolated "
                           "creatures." );
    mission_key.add( "Assign Scavenging Patrol", _( "Assign Scavenging Patrol" ), entry );
    std::vector<npc_ptr> npc_list = companion_list( p, "_scavenging_patrol" );
    if( !npc_list.empty() ) {
        entry = _( "Profit: $25-$500\nDanger: Low\nTime: 10 hour missions\n\nPatrol Roster:\n" );
        for( auto &elem : npc_list ) {
            entry = entry + "  " + elem->name + " [" + to_string( to_hours<int>( calendar::turn -
                    elem->companion_mission_time ) ) + _( " hours]\n" );
        }
        entry = entry + _( "\n\nDo you wish to bring your allies back into your party?" );
        mission_key.add( "Retrieve Scavenging Patrol", _( "Retrieve Scavenging Patrol" ), entry );
    }
}

void talk_function::scavenger_raid( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: $200-$1000\nDanger: Medium\nTime: 10 hour missions\n\n"
                           "Scavenging raids target formerly populated areas to loot as many "
                           "valuable items as possible before being surrounded by the undead.  "
                           "Combat is to be expected and assistance from the rest of the party "
                           "can't be guaranteed.  The rewards are greater and there is a chance "
                           "of the companion bringing back items." );
    mission_key.add( "Assign Scavenging Raid", _( "Assign Scavenging Raid" ), entry );
    std::vector<npc_ptr> npc_list = companion_list( p, "_scavenging_raid" );
    if( !npc_list.empty() ) {
        entry = _( "Profit: $200-$1000\nDanger: Medium\nTime: 10 hour missions\n\n"
                   "Raid Roster:\n" );
        for( auto &elem : npc_list ) {
            entry = entry + "  " + elem->name + " [" + to_string( to_hours<int>( calendar::turn -
                    elem->companion_mission_time ) ) + _( " hours]\n" );
        }
        entry = entry + _( "\n\nDo you wish to bring your allies back into your party?" );
        mission_key.add( "Retrieve Scavenging Raid", _( "Retrieve Scavenging Raid" ), entry );
    }
}

void talk_function::commune_menial( mission_data &mission_key, npc &p )
{
    mission_key.add( "Assign Ally to Menial Labor", _( "Assign Ally to Menial Labor" ) );
    std::vector<npc_ptr> npc_list = companion_list( p, "_labor" );
    if( !npc_list.empty() ) {
        std::string entry = _( "Profit: $8/hour\nDanger: Minimal\nTime: 1 hour minimum\n\n"
                               "Assigning one of your allies to menial labor is a safe way to teach "
                               "them basic skills and build reputation with the outpost.  Don't expect "
                               "much of a reward though.\n\nLabor Roster:\n" );
        for( auto &elem : npc_list ) {
            entry = entry + "  " + elem->name + " [" + to_string( to_hours<int>( calendar::turn -
                    elem->companion_mission_time ) ) + _( " hours]\n" );
        }
        entry = entry + _( "\n\nDo you wish to bring your allies back into your party?" );
        mission_key.add( "Recover Ally from Menial Labor", _( "Recover Ally from Menial Labor" ),
                         entry );
    }
}

void talk_function::commune_carpentry( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: $12/hour\nDanger: Minimal\nTime: 1 hour minimum\n\n"
                           "Carpentry work requires more skill than menial labor while offering "
                           "modestly improved pay.  It is unlikely that your companions will face "
                           "combat but there are hazards working on makeshift buildings." );
    mission_key.add( "Assign Ally to Carpentry Work", _( "Assign Ally to Carpentry Work" ), entry );
    std::vector<npc_ptr>  npc_list = companion_list( p, "_carpenter" );
    if( !npc_list.empty() ) {
        entry = _( "Profit: $12/hour\nDanger: Minimal\nTime: 1 hour minimum\n\nLabor Roster:\n" );
        for( auto &elem : npc_list ) {
            entry = entry + "  " + elem->name + " [" + to_string( to_hours<int>( calendar::turn -
                    elem->companion_mission_time ) ) + _( " hours]\n" );
        }
        entry = entry + _( "\n\nDo you wish to bring your allies back into your party?" );
        mission_key.add( "Recover Ally from Carpentry Work",
                         _( "Recover Ally from Carpentry Work" ), entry );
    }
}

void talk_function::commune_farmfield( mission_data &mission_key, npc &p )
{
    if( !p.has_trait( trait_NPC_CONSTRUCTION_LEV_1 ) ) {
        std::string entry = _( "Cost: $1000\n\n\n"
                               "                .........\n" // NOLINT(cata-text-style)
                               "                .........\n" // NOLINT(cata-text-style)
                               "                .........\n" // NOLINT(cata-text-style)
                               "                .........\n" // NOLINT(cata-text-style)
                               "                .........\n" // NOLINT(cata-text-style)
                               "                .........\n" // NOLINT(cata-text-style)
                               "                ..#....**\n" // NOLINT(cata-text-style)
                               "                ..#Ov..**\n" // NOLINT(cata-text-style)
                               "                ...O|....\n\n" // NOLINT(cata-text-style)
                               "We're willing to let you purchase a field at a substantial "
                               "discount to use for your own agricultural enterprises.  We'll "
                               "plow it for you so you know exactly what is yours… after you "
                               "have a field you can hire workers to plant or harvest crops for "
                               "you.  If the crop is something we have a demand for, we'll be "
                               "willing to liquidate it." );
        mission_key.add( "Purchase East Field", _( "Purchase East Field" ), entry );
    }
    if( p.has_trait( trait_NPC_CONSTRUCTION_LEV_1 ) && !p.has_trait( trait_NPC_CONSTRUCTION_LEV_2 ) ) {
        std::string entry = _( "Cost: $5500\n\n"
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ..#....**" // NOLINT(cata-text-style)
                               "\n              ..#Ov..**" // NOLINT(cata-text-style)
                               "\n              ...O|....\n\n" // NOLINT(cata-text-style)
                               "Protecting your field with a sturdy picket fence will keep most "
                               "wildlife from nibbling your crops apart.  You can expect yields to "
                               "increase." );
        mission_key.add( "Upgrade East Field I", _( "Upgrade East Field I" ), entry );
    }

    if( p.has_trait( trait_NPC_CONSTRUCTION_LEV_1 ) ) {
        std::string entry = _( "Cost: $3.00/plot\n\n"
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ........." // NOLINT(cata-text-style)
                               "\n              ..#....**" // NOLINT(cata-text-style)
                               "\n              ..#Ov..**" // NOLINT(cata-text-style)
                               "\n              ...O|....\n\n" // NOLINT(cata-text-style)
                               "We'll plant the field with your choice of crop if you are willing "
                               "to finance it.  When the crop is ready to harvest you can have us "
                               "liquidate it or harvest it for you." );
        mission_key.add( "Plant East Field", _( "Plant East Field" ), entry );
        entry = _( "Cost: $2.00/plot\n\n"
                   "\n              ........." // NOLINT(cata-text-style)
                   "\n              ........." // NOLINT(cata-text-style)
                   "\n              ........." // NOLINT(cata-text-style)
                   "\n              ........." // NOLINT(cata-text-style)
                   "\n              ........." // NOLINT(cata-text-style)
                   "\n              ........." // NOLINT(cata-text-style)
                   "\n              ..#....**" // NOLINT(cata-text-style)
                   "\n              ..#Ov..**" // NOLINT(cata-text-style)
                   "\n              ...O|....\n\n" // NOLINT(cata-text-style)
                   "You can either have us liquidate the crop and give you the cash or pay us to "
                   "harvest it for you." );
        mission_key.add( "Harvest East Field", _( "Harvest East Field" ), entry );
    }
}

void talk_function::commune_forage( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: $10/hour\nDanger: Low\nTime: 4 hour minimum\n\n"
                           "Foraging for food involves dispatching a companion to search the "
                           "surrounding wilderness for wild edibles.  Combat will be avoided but "
                           "encounters with wild animals are to be expected.  The low pay is "
                           "supplemented with the odd item as a reward for particularly large "
                           "hauls." );
    mission_key.add( "Assign Ally to Forage for Food", _( "Assign Ally to Forage for Food" ),
                     entry );
    std::vector<npc_ptr> npc_list = companion_list( p, "_forage" );
    if( !npc_list.empty() ) {
        entry = _( "Profit: $10/hour\nDanger: Low\nTime: 4 hour minimum\n\nLabor Roster:\n" );
        for( auto &elem : npc_list ) {
            entry = entry + "  " + elem->name + " [" + to_string( to_hours<int>( calendar::turn -
                    elem->companion_mission_time ) ) + _( " hours]\n" );
        }
        entry = entry + _( "\n\nDo you wish to bring your allies back into your party?" );
        mission_key.add( "Recover Ally from Foraging", _( "Recover Ally from Foraging" ), entry );
    }
}

void talk_function::commune_refuge_caravan( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: $18/hour\nDanger: High\nTime: UNKNOWN\n\n"
                           "Adding companions to the caravan team increases the likelihood of "
                           "success.  By nature, caravans are extremely tempting targets for "
                           "raiders or hostile groups so only a strong party is recommended.  The "
                           "rewards are significant for those participating but are even more "
                           "important for the factions that profit.\n\n"
                           "The commune is sending food to the Free Merchants in the Refugee "
                           "Center as part of a tax and in exchange for skilled labor." );
    mission_key.add( "Caravan Commune-Refugee Center", _( "Caravan Commune-Refugee Center" ),
                     entry );
    std::vector<npc_ptr> npc_list = companion_list( p, "_commune_refugee_caravan" );
    std::vector<npc_ptr> npc_list_aux;
    if( !npc_list.empty() ) {
        entry = _( "Profit: $18/hour\nDanger: High\nTime: UNKNOWN\n\n"
                   "\nRoster:\n" );
        for( auto &elem : npc_list ) {
            if( elem->companion_mission_time == calendar::before_time_starts ) {
                entry = entry + "  " + elem->name + _( " [READY]\n" );
                npc_list_aux.push_back( elem );
            } else if( calendar::turn >= elem->companion_mission_time ) {
                entry = entry + "  " + elem->name + _( " [COMPLETE]\n" );
            } else {
                entry = entry + "  " + elem->name + " [" + to_string( std::abs( to_hours<int>
                        ( calendar::turn - elem->companion_mission_time ) ) ) + _( " Hours]\n" );
            }
        }
        if( !npc_list_aux.empty() ) {
            std::string entry_aux = _( "Profit: $18/hour\nDanger: High\nTime: UNKNOWN\n\n"
                                       "\nRoster:\n" );
            for( auto &elem : npc_list_aux ) {
                if( elem->companion_mission_time == calendar::before_time_starts ) {
                    entry_aux = entry_aux + "  " + elem->name + _( " [READY]\n" );
                }
            }
            entry_aux = entry_aux + _( "\n\n"
                                       "The caravan will contain two or three additional members "
                                       "from the commune, are you ready to depart?" );
            mission_key.add( "Begin Commune-Refugee Center Run",
                             _( "Begin Commune-Refugee Center Run" ), entry );
        }
        entry = entry + _( "\n\nDo you wish to bring your allies back into your party?" );
        mission_key.add( "Recover Commune-Refugee Center", _( "Recover Commune-Refugee Center" ),
                         entry );
    }
}

bool talk_function::display_and_choose_opts( mission_data &mission_key, const tripoint &omt_pos,
        const std::string &role_id, const std::string &title )
{
    if( mission_key.entries.empty() ) {
        popup( _( "There are no missions at this colony.  Press Spacebar…" ) );
        return false;
    }

    int TITLE_TAB_HEIGHT = 0;
    if( role_id == "FACTION_CAMP" ) {
        TITLE_TAB_HEIGHT = 1;
    }

    base_camps::tab_mode tab_mode = base_camps::TAB_MAIN;

    size_t sel = 0;

    // The following are for managing the right pane scrollbar.
    size_t info_offset = 0;
    nc_color col = c_white;
    std::vector<std::string> name_text;
    std::vector<std::string> mission_text;

    input_context ctxt( "FACTIONS" );
    ctxt.register_action( "UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    std::vector<mission_entry> cur_key_list;

    auto reset_cur_key_list = [&]() {
        cur_key_list = mission_key.entries[0];
        for( const auto &k : mission_key.entries[1] ) {
            bool has = false;
            for( const auto &keys : cur_key_list ) {
                if( k.id == keys.id ) {
                    has = true;
                    break;
                }
            }
            if( !has ) {
                cur_key_list.push_back( k );
            }
        }
    };

    reset_cur_key_list();

    if( cur_key_list.empty() ) {
        popup( _( "There are no missions at this colony.  Press Spacebar…" ) );
        return false;
    }

    size_t part_y = 0;
    size_t part_x = 0;
    size_t maxy = 0;
    size_t maxx = 0;
    size_t info_height = 0;
    size_t info_width = 0;

    catacurses::window w_list;
    catacurses::window w_tabs;
    catacurses::window w_info;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        part_y = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 4 : 0;
        part_x = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 4 : 0;
        maxy = part_y ? TERMY - 2 * part_y : FULL_SCREEN_HEIGHT;
        maxx = part_x ? TERMX - 2 * part_x : FULL_SCREEN_WIDTH;
        info_height = maxy - 3;
        info_width = maxx - 1 - MAX_FAC_NAME_SIZE;

        w_list = catacurses::newwin( maxy, maxx,
                                     point( part_x, part_y + TITLE_TAB_HEIGHT ) );
        w_info = catacurses::newwin( info_height, info_width,
                                     point( part_x + MAX_FAC_NAME_SIZE, part_y + TITLE_TAB_HEIGHT + 1 ) );

        if( role_id == "FACTION_CAMP" ) {
            w_tabs = catacurses::newwin( TITLE_TAB_HEIGHT, maxx, point( part_x, part_y ) );
            ui.position( point( part_x, part_y ), point( maxx, maxy + TITLE_TAB_HEIGHT ) );
        } else {
            ui.position( point( part_x, part_y + TITLE_TAB_HEIGHT ), point( maxx, maxy ) );
        }
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_list );
        draw_border( w_list );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_list, point( 1, 1 ), c_white, name_mission_tabs( omt_pos, role_id, title,
                   tab_mode ) );

        std::vector<std::vector<std::string>> folded_names;
        size_t folded_names_lines = 0;
        for( const auto &cur_key_entry : cur_key_list ) {
            std::vector<std::string> f_name = foldstring( cur_key_entry.name_display, MAX_FAC_NAME_SIZE - 5,
                                              ' ' );
            folded_names_lines += f_name.size();
            folded_names.emplace_back( f_name );
        }

        int name_offset = 0;
        calcStartPos( name_offset, sel, info_height, folded_names_lines );

        size_t list_line = 2;
        for( size_t current = name_offset; list_line < info_height &&
             current < cur_key_list.size(); current++ ) {
            nc_color col = ( current == sel ? h_white : c_white );
            //highlight important missions
            for( const auto &k : mission_key.entries[0] ) {
                if( cur_key_list[current].id == k.id ) {
                    col = ( current == sel ? h_white : c_yellow );
                    break;
                }
            }
            //dull uncraftable items
            for( const auto &k : mission_key.entries[10] ) {
                if( cur_key_list[current].id == k.id ) {
                    col = ( current == sel ? h_white : c_dark_gray );
                    break;
                }
            }
            std::vector<std::string> &name_text = folded_names[current];
            for( size_t name_line = 0; name_line < name_text.size(); name_line++ ) {
                print_colored_text( w_list, point( name_line ? 5 : 1, list_line ),
                                    col, col, name_text[name_line] );
                list_line += 1;
            }
        }

        if( cur_key_list.size() > info_height + 1 ) {
            scrollbar()
            .offset_x( 0 )
            .offset_y( 1 )
            .content_size( folded_names_lines )
            .viewport_pos( sel )
            .viewport_size( info_height + 1 )
            .apply( w_list );
        }
        wnoutrefresh( w_list );
        werase( w_info );

        // Fold mission text, store it for scrolling
        mission_text = foldstring( mission_key.cur_key.text, info_width - 2, ' ' );
        if( info_height >= mission_text.size() ) {
            info_offset = 0;
        } else if( info_offset + info_height > mission_text.size() ) {
            info_offset = mission_text.size() - info_height;
        }
        if( mission_text.size() > info_height ) {
            scrollbar()
            .offset_x( info_width - 1 )
            .offset_y( 0 )
            .content_size( mission_text.size() )
            .viewport_pos( info_offset )
            .viewport_size( info_height )
            .apply( w_info );
        }
        const size_t end_line = std::min( info_height, mission_text.size() - info_offset );

        // Display the current subset of the mission text.
        for( size_t start_line = 0; start_line < end_line; start_line++ ) {
            print_colored_text( w_info, point( 0, start_line ), col, col,
                                mission_text[start_line + info_offset] );
        }

        wnoutrefresh( w_info );

        if( role_id == "FACTION_CAMP" ) {
            werase( w_tabs );
            draw_camp_tabs( w_tabs, tab_mode, mission_key.entries );
            wnoutrefresh( w_tabs );
        }
    } );

    while( true ) {
        mission_key.cur_key = cur_key_list[sel];
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            if( sel == cur_key_list.size() - 1 ) {
                sel = 0;    // Wrap around
            } else {
                sel++;
            }
            info_offset = 0;
        } else if( action == "UP" ) {
            if( sel == 0 ) {
                sel = cur_key_list.size() - 1;    // Wrap around
            } else {
                sel--;
            }
            info_offset = 0;
        } else if( action == "PAGE_UP" ) {
            if( info_offset > 0 ) {
                info_offset--;
            }
        } else if( action == "PAGE_DOWN" ) {
            info_offset++;
        } else if( action == "NEXT_TAB" && role_id == "FACTION_CAMP" ) {
            sel = 0;
            info_offset = 0;

            do {
                if( tab_mode == base_camps::TAB_NW ) {
                    tab_mode = base_camps::TAB_MAIN;
                    reset_cur_key_list();
                } else {
                    tab_mode = static_cast<base_camps::tab_mode>( tab_mode + 1 );
                    cur_key_list = mission_key.entries[tab_mode + 1];
                }
            } while( cur_key_list.empty() );
        } else if( action == "PREV_TAB" && role_id == "FACTION_CAMP" ) {
            sel = 0;
            info_offset = 0;

            do {
                if( tab_mode == base_camps::TAB_MAIN ) {
                    tab_mode = base_camps::TAB_NW;
                } else {
                    tab_mode = static_cast<base_camps::tab_mode>( tab_mode - 1 );
                }

                if( tab_mode == base_camps::TAB_MAIN ) {
                    reset_cur_key_list();
                } else {
                    cur_key_list = mission_key.entries[tab_mode + 1];
                }
            } while( cur_key_list.empty() );
        } else if( action == "QUIT" ) {
            mission_entry dud;
            dud.id = "NONE";
            dud.name_display = "NONE";
            mission_key.cur_key = dud;
            break;
        } else if( action == "CONFIRM" ) {
            if( mission_key.cur_key.possible ) {
                break;
            } else {
                continue;
            }
        }
    }
    return true;
}

bool talk_function::handle_outpost_mission( const mission_entry &cur_key, npc &p )
{
    if( cur_key.id == "Caravan Commune-Refugee Center" ) {
        individual_mission( p, _( "joins the caravan team…" ), "_commune_refugee_caravan", true );
    }
    if( cur_key.id == "Begin Commune-Refugee Center Run" ) {
        caravan_depart( p, "evac_center_18", "_commune_refugee_caravan" );
    }
    if( cur_key.id == "Recover Commune-Refugee Center" ) {
        caravan_return( p, "evac_center_18", "_commune_refugee_caravan" );
    }
    if( cur_key.id == "Purchase East Field" ) {
        field_build_1( p );
    }
    if( cur_key.id == "Upgrade East Field I" ) {
        field_build_2( p );
    }
    if( cur_key.id == "Plant East Field" ) {
        field_plant( p, "ranch_camp_63" );
    }
    if( cur_key.id == "Harvest East Field" ) {
        field_harvest( p, "ranch_camp_63" );
    }
    if( cur_key.id == "Assign Scavenging Patrol" ) {
        individual_mission( p, _( "departs on the scavenging patrol…" ), "_scavenging_patrol" );
    }
    if( cur_key.id == "Retrieve Scavenging Patrol" ) {
        scavenging_patrol_return( p );
    }
    if( cur_key.id == "Assign Scavenging Raid" ) {
        individual_mission( p, _( "departs on the scavenging raid…" ), "_scavenging_raid" );
    }
    if( cur_key.id == "Retrieve Scavenging Raid" ) {
        scavenging_raid_return( p );
    }
    if( cur_key.id == "Assign Ally to Menial Labor" ) {
        individual_mission( p, _( "departs to work as a laborer…" ), "_labor" );
    }
    if( cur_key.id == "Recover Ally from Menial Labor" ) {
        labor_return( p );
    }

    if( cur_key.id == "Assign Ally to Carpentry Work" ) {
        individual_mission( p, _( "departs to work as a carpenter…" ), "_carpenter" );
    }
    if( cur_key.id == "Recover Ally from Carpentry Work" ) {
        carpenter_return( p );
    }
    if( cur_key.id == "Assign Ally to Forage for Food" ) {
        individual_mission( p, _( "departs to forage for food…" ), "_forage" );
    }
    if( cur_key.id == "Recover Ally from Foraging" ) {
        forage_return( p );
    }

    return true;
}

npc_ptr talk_function::individual_mission( npc &p, const std::string &desc,
        const std::string &miss_id, bool group, const std::vector<item *> &equipment,
        const std::map<skill_id, int> &required_skills )
{
    const tripoint omt_pos = p.global_omt_location();
    return individual_mission( omt_pos, p.companion_mission_role_id, desc, miss_id, group,
                               equipment, required_skills );
}
npc_ptr talk_function::individual_mission( const tripoint &omt_pos,
        const std::string &role_id, const std::string &desc,
        const std::string &miss_id, bool group, const std::vector<item *> &equipment,
        const std::map<skill_id, int> &required_skills )
{
    npc_ptr comp = companion_choose( required_skills );
    if( comp == nullptr ) {
        return comp;
    }
    // make sure, for now, that NPCs dismount their horse before going on a mission.
    if( comp->has_effect( effect_riding ) ) {
        comp->npc_dismount();
    }
    //Ensure we have someone to give equipment to before we lose it
    for( auto i : equipment ) {
        comp->companion_mission_inv.add_item( *i );
        //comp->i_add(*i);
        if( item::count_by_charges( i->typeId() ) ) {
            g->u.use_charges( i->typeId(), i->charges );
        } else {
            g->u.use_amount( i->typeId(), 1 );
        }
    }
    if( comp->in_vehicle ) {
        g->m.unboard_vehicle( comp->pos() );
    }
    popup( "%s %s", comp->name, desc );
    comp->set_companion_mission( omt_pos, role_id, miss_id );
    if( group ) {
        comp->companion_mission_time = calendar::before_time_starts;
    } else {
        comp->companion_mission_time = calendar::turn;
    }
    g->reload_npcs();
    g->validate_npc_followers();
    assert( !comp->is_active() );
    return comp;
}

void talk_function::caravan_depart( npc &p, const std::string &dest, const std::string &id )
{
    std::vector<npc_ptr> npc_list = companion_list( p, id );
    int distance = caravan_dist( dest );
    time_duration time = 20_minutes + distance * 10_minutes;
    popup( _( "The caravan departs with an estimated total travel time of %d hours…" ),
           to_hours<int>( time ) );

    for( auto &elem : npc_list ) {
        if( elem->companion_mission_time == calendar::before_time_starts ) {
            //Adds a 10% error in estimated travel time
            elem->companion_mission_time = calendar::turn + time * rng_float( -1.1, 1.1 );
        }
    }
}

//Could be expanded to actually path to the site, just returns the distance
int talk_function::caravan_dist( const std::string &dest )
{
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), dest, 0, false );
    int distance = rl_dist( g->u.pos(), site );
    return distance;
}

void talk_function::caravan_return( npc &p, const std::string &dest, const std::string &id )
{
    npc_ptr comp = companion_choose_return( p, id, calendar::turn );
    if( comp == nullptr ) {
        return;
    }
    if( comp->companion_mission_time == calendar::before_time_starts ) {
        popup( _( "%s returns to your party." ), comp->name );
        companion_return( *comp );
        return;
    }
    //So we have chosen to return an individual or party who went on the mission
    //Everyone who was on the mission will have the same companion_mission_time
    //and will simulate the mission and return together
    std::vector<npc_ptr> caravan_party;
    std::vector<npc_ptr> bandit_party;
    std::vector<npc_ptr> npc_list = companion_list( p, id );
    const int rand_caravan_size = rng( 1, 3 );
    caravan_party.reserve( npc_list.size() + rand_caravan_size );
    for( int i = 0; i < rand_caravan_size; i++ ) {
        caravan_party.push_back( temp_npc( string_id<npc_template>( "commune_guard" ) ) );
    }
    for( auto &elem : npc_list ) {
        if( elem->companion_mission_time == comp->companion_mission_time ) {
            caravan_party.push_back( elem );
        }
    }

    int distance = caravan_dist( dest );
    int time = 200 + distance * 100;
    int experience = rng( 10, time / 300 );

    const int rand_bandit_size = rng( 1, 3 );
    bandit_party.reserve( rand_bandit_size * 2 );
    for( int i = 0; i < rand_bandit_size * 2; i++ ) {
        bandit_party.push_back( temp_npc( string_id<npc_template>( "bandit" ) ) );
        bandit_party.push_back( temp_npc( string_id<npc_template>( "thug" ) ) );
    }

    if( one_in( 3 ) ) {
        if( one_in( 2 ) ) {
            popup( _( "A bandit party approaches the caravan in the open!" ) );
            force_on_force( caravan_party, "caravan", bandit_party, "band", 1 );
        } else if( one_in( 3 ) ) {
            popup( _( "A bandit party attacks the caravan while it it's camped!" ) );
            force_on_force( caravan_party, "caravan", bandit_party, "band", 2 );
        } else {
            popup( _( "The caravan walks into a bandit ambush!" ) );
            force_on_force( caravan_party, "caravan", bandit_party, "band", -1 );
        }
    }

    int money = 0;
    for( const auto &elem : caravan_party ) {
        //Scrub temporary party members and the dead
        if( elem->hp_cur[hp_torso] == 0 && elem->has_companion_mission() ) {
            overmap_buffer.remove_npc( comp->getID() );
            money += ( time / 600 ) * 9;
        } else if( elem->has_companion_mission() ) {
            money += ( time / 600 ) * 18;
            companion_skill_trainer( *elem, "combat", experience * 10_minutes, 10 );
            companion_return( *elem );
        }
    }

    if( money != 0 ) {
        g->u.cash += ( 100 * money );
        popup( _( "The caravan party has returned.  Your share of the profits are $%d!" ), money );
    } else {
        popup( _( "The caravan was a disaster and your companions never made it home…" ) );
    }
}

//A random NPC on one team attacks a random monster on the opposite
void talk_function::attack_random( const std::vector<npc_ptr> &attacker,
                                   const std::vector< monster * > &group )
{
    if( attacker.empty() || group.empty() ) {
        return;
    }
    const auto att = random_entry( attacker );
    monster *def = random_entry( group );
    att->melee_attack( *def, false );
    if( def->get_hp() <= 0 ) {
        popup( _( "%s is wasted by %s!" ), def->type->nname(), att->name );
    }
}

//A random monster on one side attacks a random NPC on the other
void talk_function::attack_random( const std::vector< monster * > &group,
                                   const std::vector<npc_ptr> &defender )
{
    if( defender.empty() || group.empty() ) {
        return;
    }
    monster *att = random_entry( group );
    const auto def = random_entry( defender );
    att->melee_attack( *def );
    //monster mon;
    if( def->hp_cur[hp_torso] <= 0 || def->is_dead() ) {
        popup( _( "%s is wasted by %s!" ), def->name, att->type->nname() );
    }
}

//A random NPC on one team attacks a random NPC on the opposite
void talk_function::attack_random( const std::vector<npc_ptr> &attacker,
                                   const std::vector<npc_ptr> &defender )
{
    if( attacker.empty() || defender.empty() ) {
        return;
    }
    const auto att = random_entry( attacker );
    const auto def = random_entry( defender );
    const skill_id best = att->best_skill();
    int best_score = 1;
    if( best ) {
        best_score = att->get_skill_level( best );
    }
    ///\EFFECT_DODGE_NPC increases avoidance of random attacks
    if( rng( -1, best_score ) >= rng( 0, def->get_skill_level( skill_dodge ) ) ) {
        def->hp_cur[hp_torso] = 0;
        popup( _( "%s is wasted by %s!" ), def->name, att->name );
    } else {
        popup( _( "%s dodges %s's attack!" ), def->name, att->name );
    }
}

//Used to determine when to retreat, might want to add in a random factor so that engagements aren't
//drawn out wars of attrition
int talk_function::combat_score( const std::vector<npc_ptr> &group )
{
    int score = 0;
    for( const auto &elem : group ) {
        if( elem->hp_cur[hp_torso] != 0 ) {
            const skill_id best = elem->best_skill();
            if( best ) {
                score += elem->get_skill_level( best );
            } else {
                score += 1;
            }
        }
    }
    return score;
}

int talk_function::combat_score( const std::vector< monster * > &group )
{
    int score = 0;
    for( const auto &elem : group ) {
        if( elem->get_hp() > 0 ) {
            score += elem->type->difficulty;
        }
    }
    return score;
}

npc_ptr talk_function::temp_npc( const string_id<npc_template> &type )
{
    npc_ptr temp = make_shared_fast<npc>();
    temp->normalize();
    temp->load_npc_template( type );
    return temp;
}

//The field is designed as more of a convenience than a profit opportunity.
void talk_function::field_build_1( npc &p )
{
    if( g->u.cash < 100000 ) {
        popup( _( "I'm sorry, you don't have enough money." ) );
        return;
    }
    p.set_mutation( trait_NPC_CONSTRUCTION_LEV_1 );
    g->u.cash += -100000;
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), "ranch_camp_63", 20,
                          false );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_ter( t_dirt, point( 5, 4 ), point( 15, 14 ) );
    bay.draw_square_ter( t_dirtmound, point( 6, 5 ), point( 6, 13 ) );
    bay.draw_square_ter( t_dirtmound, point( 8, 5 ), point( 8, 13 ) );
    bay.draw_square_ter( t_dirtmound, point( 10, 5 ), point( 10, 13 ) );
    bay.draw_square_ter( t_dirtmound, point( 12, 5 ), point( 12, 13 ) );
    bay.draw_square_ter( t_dirtmound, point( 14, 5 ), point( 14, 13 ) );
    bay.save();
    popup( _( "%s jots your name down on a ledger and yells out to nearby laborers to begin "
              "plowing your new field." ), p.name );
}

//Really expensive, but that is so you can't tear down the fence and sell the wood for a profit!
void talk_function::field_build_2( npc &p )
{
    if( g->u.cash < 550000 ) {
        popup( _( "I'm sorry, you don't have enough money." ) );
        return;
    }
    p.set_mutation( trait_NPC_CONSTRUCTION_LEV_2 );
    g->u.cash += -550000;
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), "ranch_camp_63",
                          20, false );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_ter( t_fence, point( 4, 3 ), point( 16, 3 ) );
    bay.draw_square_ter( t_fence, point( 4, 15 ), point( 16, 15 ) );
    bay.draw_square_ter( t_fence, point( 4, 3 ), point( 4, 15 ) );
    bay.draw_square_ter( t_fence, point( 16, 3 ), point( 16, 15 ) );
    bay.draw_square_ter( t_fencegate_c, point( 10, 3 ), point( 10, 3 ) );
    bay.draw_square_ter( t_fencegate_c, point( 10, 15 ), point( 10, 15 ) );
    bay.draw_square_ter( t_fencegate_c, point( 4, 9 ), point( 4, 9 ) );
    bay.save();
    popup( _( "After counting your money %s directs a nearby laborer to begin constructing a "
              "fence around your plot…" ), p.name );
}

void talk_function::field_plant( npc &p, const std::string &place )
{
    if( !warm_enough_to_plant( g->u.pos() ) ) {
        popup( _( "It is too cold to plant anything now." ) );
        return;
    }
    std::vector<item *> seed_inv = g->u.items_with( []( const item & itm ) {
        return itm.is_seed() && itm.typeId() != itype_marloss_seed &&
               itm.typeId() != itype_fungal_seeds;
    } );
    if( seed_inv.empty() ) {
        popup( _( "You have no seeds to plant!" ) );
        return;
    }

    int empty_plots = 0;
    int free_seeds = 0;

    std::vector<itype_id> seed_types;
    std::vector<std::string> seed_names;
    for( auto &seed : seed_inv ) {
        if( std::find( seed_types.begin(), seed_types.end(), seed->typeId() ) == seed_types.end() ) {
            seed_types.push_back( seed->typeId() );
            seed_names.push_back( seed->tname() );
        }
    }
    // Choose seed if applicable
    const int seed_index = uilist( _( "Which seeds do you wish to have planted?" ), seed_names );
    // Did we cancel?
    if( seed_index < 0 || static_cast<size_t>( seed_index ) >= seed_types.size() ) {
        popup( _( "You saved your seeds for later." ) );
        return;
    }

    const auto &seed_id = seed_types[seed_index];
    if( item::count_by_charges( seed_id ) ) {
        free_seeds = g->u.charges_of( seed_id );
    } else {
        free_seeds = g->u.amount_of( seed_id );
    }

    //Now we need to find how many free plots we have to plant in...
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), place, 20,
                          false );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    for( const tripoint &plot : bay.points_on_zlevel() ) {
        if( bay.ter( plot ) == t_dirtmound ) {
            empty_plots++;
        }
    }

    if( empty_plots == 0 ) {
        popup( _( "You have no room to plant seeds…" ) );
        return;
    }

    int limiting_number = free_seeds;
    if( free_seeds > empty_plots ) {
        limiting_number = empty_plots;
    }

    signed int a = limiting_number * 300;
    if( a > g->u.cash ) {
        popup( _( "I'm sorry, you don't have enough money to plant those seeds…" ) );
        return;
    }
    if( !query_yn( _( "Do you wish to have %d %s planted here for $%d?" ), limiting_number,
                   seed_names[seed_index], limiting_number * 3 ) ) {
        return;
    }

    //Plant the actual seeds
    for( const tripoint &plot : bay.points_on_zlevel() ) {
        if( bay.ter( plot ) == t_dirtmound && limiting_number > 0 ) {
            std::list<item> used_seed;
            if( item::count_by_charges( seed_id ) ) {
                used_seed = g->u.use_charges( seed_id, 1 );
            } else {
                used_seed = g->u.use_amount( seed_id, 1 );
            }
            used_seed.front().set_age( 0_turns );
            bay.add_item_or_charges( plot, used_seed.front() );
            bay.set( plot, t_dirt, f_plant_seed );
            limiting_number--;
        }
    }
    bay.draw_square_ter( t_fence, point( 4, 3 ), point( 16, 3 ) );
    bay.save();
    popup( _( "After counting your money and collecting your seeds, %s calls forth a labor party "
              "to plant your field." ), p.name );
}

void talk_function::field_harvest( npc &p, const std::string &place )
{
    //First we need a list of plants that can be harvested...
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), place, 20,
                          false );
    tinymap bay;
    item tmp;
    std::vector<itype_id> seed_types;
    std::vector<itype_id> plant_types;
    std::vector<std::string> plant_names;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    for( const tripoint &plot : bay.points_on_zlevel() ) {
        map_stack items = bay.i_at( plot );
        if( bay.furn( plot ) == furn_str_id( "f_plant_harvest" ) && !items.empty() ) {
            // Can't use item_stack::only_item() since there might be fertilizer
            map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
                return it.is_seed();
            } );

            if( seed != items.end() ) {
                const islot_seed &seed_data = *seed->type->seed;
                tmp = item( seed_data.fruit_id, calendar::turn );
                bool check = false;
                for( const std::string &elem : plant_names ) {
                    if( elem == tmp.type_name( 3 ) ) {
                        check = true;
                    }
                }
                if( !check ) {
                    plant_types.push_back( tmp.typeId() );
                    plant_names.push_back( tmp.type_name( 3 ) );
                    seed_types.push_back( seed->typeId() );
                }
            }
        }
    }
    if( plant_names.empty() ) {
        popup( _( "There aren't any plants that are ready to harvest…" ) );
        return;
    }
    // Choose the crop to harvest
    const int plant_index = uilist( _( "Which plants do you want to have harvested?" ),
                                    plant_names );
    // Did we cancel?
    if( plant_index < 0 || static_cast<size_t>( plant_index ) >= plant_types.size() ) {
        popup( _( "You decided to hold off for now…" ) );
        return;
    }

    int number_plots = 0;
    int number_plants = 0;
    int number_seeds = 0;
    int skillLevel = 2;
    if( p.has_trait( trait_NPC_CONSTRUCTION_LEV_2 ) ) {
        skillLevel += 2;
    }

    for( const tripoint &plot : bay.points_on_zlevel() ) {
        if( bay.furn( plot ) == furn_str_id( "f_plant_harvest" ) ) {
            // Can't use item_stack::only_item() since there might be fertilizer
            map_stack items = bay.i_at( plot );
            map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
                return it.is_seed();
            } );

            if( seed != items.end() ) {
                const islot_seed &seed_data = *seed->type->seed;
                tmp = item( seed_data.fruit_id, calendar::turn );
                if( tmp.typeId() == plant_types[plant_index] ) {
                    number_plots++;
                    bay.i_clear( plot );
                    bay.furn_set( plot, f_null );
                    bay.ter_set( plot, t_dirtmound );
                    int plantCount = rng( skillLevel / 2, skillLevel );
                    if( plantCount >= 9 ) {
                        plantCount = 9;
                    } else if( plantCount <= 0 ) {
                        plantCount = 1;
                    }
                    number_plants += plantCount;
                    number_seeds += std::max( 1, rng( plantCount / 4, plantCount / 2 ) );
                }
            }
        }
    }
    bay.save();
    tmp = item( plant_types[plant_index], calendar::turn );
    int money = ( number_plants * tmp.price( true ) - number_plots * 2 ) / 100;
    bool liquidate = false;

    signed int a = number_plots * 2;
    if( a > g->u.cash ) {
        liquidate = true;
        popup( _( "You don't have enough to pay the workers to harvest the crop so you are forced "
                  "to sell…" ) );
    } else {
        liquidate = query_yn( _( "Do you wish to sell the crop of %d %s for a profit of $%d?" ),
                              number_plants, plant_names[plant_index], money );
    }

    //Add fruit
    if( liquidate ) {
        add_msg( _( "The %s are sold for $%d…" ), plant_names[plant_index], money );
        g->u.cash += ( number_plants * tmp.price( true ) - number_plots * 2 ) / 100;
    } else {
        if( tmp.count_by_charges() ) {
            tmp.charges = 1;
        }
        for( int i = 0; i < number_plants; ++i ) {
            //Should be dropped at your feet once greedy companions can be controlled
            g->u.i_add( tmp );
        }
        add_msg( _( "You receive %d %s…" ), number_plants, plant_names[plant_index] );
    }
    tmp = item( seed_types[plant_index], calendar::turn );
    const islot_seed &seed_data = *tmp.type->seed;
    if( seed_data.spawn_seeds ) {
        if( tmp.count_by_charges() ) {
            tmp.charges = 1;
        }
        for( int i = 0; i < number_seeds; ++i ) {
            g->u.i_add( tmp );
        }
        add_msg( _( "You receive %d %s…" ), number_seeds, tmp.type_name( 3 ) );
    }

}

static int scavenging_combat_skill( npc &p, int bonus, bool guns )
{
    // the following doxygen aliases do not yet exist. this is marked for future reference
    ///\EFFECT_MELEE_NPC affects scavenging_patrol results
    ///\EFFECT_SURVIVAL_NPC affects scavenging_patrol results
    ///\EFFECT_BASHING_NPC affects scavenging_patrol results
    ///\EFFECT_CUTTING_NPC affects scavenging_patrol results
    ///\EFFECT_GUN_NPC affects scavenging_patrol results
    ///\EFFECT_STABBING_NPC affects scavenging_patrol results
    ///\EFFECT_UNARMED_NPC affects scavenging_patrol results
    ///\EFFECT_DODGE_NPC affects scavenging_patrol results
    return bonus + p.get_skill_level( skill_melee ) + .5 * p.get_skill_level( skill_survival ) +
           p.get_skill_level( skill_bashing ) + p.get_skill_level( skill_cutting ) +
           ( guns ? p.get_skill_level( skill_gun ) : 0 ) + p.get_skill_level( skill_stabbing ) +
           p.get_skill_level( skill_unarmed ) + p.get_skill_level( skill_dodge );
}

bool talk_function::scavenging_patrol_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, "_scavenging_patrol",
                                            calendar::turn - 10_hours );
    if( comp == nullptr ) {
        return false;
    }
    int experience = rng( 5, 20 );
    if( one_in( 4 ) ) {
        popup( _( "While scavenging, %s's party suddenly found itself set upon by a large mob of "
                  "undead…" ), comp->name );
        int skill = scavenging_combat_skill( *comp, 4, true );
        if( one_in( 6 ) ) {
            popup( _( "Through quick thinking the group was able to evade combat!" ) );
        } else {
            popup( _( "Combat took place in close quarters, focusing on melee skills…" ) );
            int monsters = rng( 8, 30 );
            if( skill * rng_float( .60, 1.4 ) > .35 * monsters * rng_float( .6, 1.4 ) ) {
                popup( _( "Through brute force the party smashed through the group of %d"
                          " undead!" ), monsters );
                experience += rng( 2, 10 );
            } else {
                popup( _( "Unfortunately they were overpowered by the undead…  I'm sorry." ) );
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }

    int money = rng( 25, 450 );
    g->u.cash += money * 100;

    companion_skill_trainer( *comp, "combat", experience * 10_minutes, 10 );
    popup( _( "%s returns from patrol having earned $%d and a fair bit of experience…" ),
           comp->name, money );
    if( one_in( 10 ) ) {
        popup( _( "%s was impressed with %s's performance and gave you a small bonus ( $100 )" ),
               p.name, comp->name );
        g->u.cash += 10000;
    }
    if( one_in( 10 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
        p.set_mutation( trait_NPC_MISSION_LEV_1 );
        popup( _( "%s feels more confident in your abilities and is willing to let you "
                  "participate in daring raids." ), p.name );
    }
    companion_return( *comp );
    return true;
}

bool talk_function::scavenging_raid_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, "_scavenging_raid",
                                            calendar::turn - 10_hours );
    if( comp == nullptr ) {
        return false;
    }
    int experience = rng( 10, 20 );
    if( one_in( 2 ) ) {
        popup( _( "While scavenging, %s's party suddenly found itself set upon by a large mob of "
                  "undead…" ), comp->name );
        int skill = scavenging_combat_skill( *comp, 4, true );
        if( one_in( 6 ) ) {
            popup( _( "Through quick thinking the group was able to evade combat!" ) );
        } else {
            popup( _( "Combat took place in close quarters, focusing on melee skills…" ) );
            int monsters = rng( 8, 30 );
            if( skill * rng_float( .60, 1.4 ) > ( .35 * monsters * rng_float( .6, 1.4 ) ) ) {
                popup( _( "Through brute force the party smashed through the group of %d "
                          "undead!" ), monsters );
                experience += rng( 2, 10 );
            } else {
                popup( _( "Unfortunately they were overpowered by the undead…  I'm sorry." ) );
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }
    //The loot value needs to be added to the faction - what the player is payed
    tripoint loot_location = g->u.global_omt_location();
    // Only check at the ground floor.
    loot_location.z = 0;
    for( int i = 0; i < rng( 2, 3 ); i++ ) {
        const tripoint site = overmap_buffer.find_closest( loot_location, "house", 0, false,
                              ot_match_type::prefix );
        overmap_buffer.reveal( site, 2 );
        loot_building( site );
    }

    int money = rng( 200, 900 );
    g->u.cash += money * 100;

    companion_skill_trainer( *comp, "combat", experience * 10_minutes, 10 );
    popup( _( "%s returns from the raid having earned $%d and a fair bit of experience…" ),
           comp->name, money );
    if( one_in( 20 ) ) {
        popup( _( "%s was impressed with %s's performance and gave you a small bonus ( $100 )" ),
               p.name, comp->name );
        g->u.cash += 10000;
    }
    if( one_in( 2 ) ) {
        std::string itemlist = "npc_misc";
        if( one_in( 8 ) ) {
            itemlist = "npc_weapon_random";
        }
        auto result = item_group::item_from( itemlist );
        if( !result.is_null() ) {
            popup( _( "%s returned with a %s for you!" ), comp->name, result.tname() );
            g->u.i_add( result );
        }
    }
    companion_return( *comp );
    return true;
}

bool talk_function::labor_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, "_labor", calendar::turn - 1_hours );
    if( comp == nullptr ) {
        return false;
    }

    float hours = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int money = 8 * hours;
    g->u.cash += money * 100;

    companion_skill_trainer( *comp, "menial", calendar::turn - comp->companion_mission_time, 1 );

    popup( _( "%s returns from working as a laborer having earned $%d and a bit of experience…" ),
           comp->name, money );
    companion_return( *comp );
    if( hours >= 8 && one_in( 8 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
        p.set_mutation( trait_NPC_MISSION_LEV_1 );
        popup( _( "%s feels more confident in your companions and is willing to let them "
                  "participate in advanced tasks." ), p.name );
    }

    return true;
}

bool talk_function::carpenter_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, "_carpenter",
                                            calendar::turn - 1_hours );
    if( comp == nullptr ) {
        return false;
    }

    if( one_in( 20 ) ) {
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_FABRICATION_NPC affects carpenter mission results

        ///\EFFECT_DODGE_NPC affects carpenter mission results

        ///\EFFECT_SURVIVAL_NPC affects carpenter mission results
        int skill_1 = comp->get_skill_level( skill_fabrication );
        int skill_2 = comp->get_skill_level( skill_dodge );
        int skill_3 = comp->get_skill_level( skill_survival );
        popup( _( "While %s was framing a building one of the walls began to collapse…" ),
               comp->name );
        if( skill_1 > rng( 1, 8 ) ) {
            popup( _( "In the blink of an eye, %s threw a brace up and averted a disaster." ),
                   comp->name );
        } else if( skill_2 > rng( 1, 8 ) ) {
            popup( _( "Darting out a window, %s escaped the collapse." ), comp->name );
        } else if( skill_3 > rng( 1, 8 ) ) {
            popup( _( "%s didn't make it out in time…" ), comp->name );
            popup( _( "but %s was rescued from the debris with only minor injuries!" ),
                   comp->name );
        } else {
            popup( _( "%s didn't make it out in time…" ), comp->name );
            popup( _( "Everyone who was trapped under the collapsing roof died…" ) );
            popup( _( "I'm sorry, there is nothing we could do." ) );
            overmap_buffer.remove_npc( comp->getID() );
            return false;
        }
    }

    float hours = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int money = 12 * hours;
    g->u.cash += money * 100;

    companion_skill_trainer( *comp, "construction", calendar::turn -
                             comp->companion_mission_time, 2 );

    popup( _( "%s returns from working as a carpenter having earned $%d and a bit of "
              "experience…" ), comp->name, money );
    companion_return( *comp );
    return true;
}

bool talk_function::forage_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, "_forage", calendar::turn - 4_hours );
    if( comp == nullptr ) {
        return false;
    }

    if( one_in( 10 ) ) {
        popup( _( "While foraging, a beast began to stalk %s…" ), comp->name );
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_SURVIVAL_NPC affects forage mission results

        ///\EFFECT_DODGE_NPC affects forage mission results
        int skill_1 = comp->get_skill_level( skill_survival );
        int skill_2 = comp->get_skill_level( skill_dodge );
        if( skill_1 > rng( -2, 8 ) ) {
            popup( _( "Alerted by a rustle, %s fled to the safety of the outpost!" ), comp->name );
        } else if( skill_2 > rng( -2, 8 ) ) {
            popup( _( "As soon as the cougar sprang %s darted to the safety of the outpost!" ),
                   comp->name );
        } else {
            popup( _( "%s was caught unaware and was forced to fight the creature at close "
                      "range!" ), comp->name );
            int skill = scavenging_combat_skill( *comp, 0, false );
            int monsters = rng( 0, 10 );
            if( skill * rng_float( .80, 1.2 ) > monsters * rng_float( .8, 1.2 ) ) {
                if( one_in( 2 ) ) {
                    popup( _( "%s was able to scare off the bear after delivering a nasty "
                              "blow!" ), comp->name );
                } else {
                    popup( _( "%s beat the cougar into a bloody pulp!" ), comp->name );
                }
            } else {
                if( one_in( 2 ) ) {
                    popup( _( "%s was able to hold off the first wolf but the others that were "
                              "skulking in the tree line caught up…" ), comp->name );
                    popup( _( "I'm sorry, there wasn't anything we could do…" ) );
                } else {
                    popup( _( "We… we don't know what exactly happened but we found %s's gear "
                              "ripped and bloody…" ), comp->name );
                    popup( _( "I fear your companion won't be returning." ) );
                }
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }

    float hours = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int money = 10 * hours;
    g->u.cash += money * 100;

    companion_skill_trainer( *comp, "gathering", calendar::turn -
                             comp->companion_mission_time, 2 );

    popup( _( "%s returns from working as a forager having earned $%d and a bit of "
              "experience…" ), comp->name, money );
    // the following doxygen aliases do not yet exist. this is marked for future reference

    ///\EFFECT_SURVIVAL_NPC affects forage mission results
    int skill = comp->get_skill_level( skill_survival );
    if( skill > rng_float( -.5, 8 ) ) {
        std::string itemlist = "farming_seeds";
        if( one_in( 2 ) ) {
            switch( season_of_year( calendar::turn ) ) {
                case SPRING:
                    itemlist = "forage_spring";
                    break;
                case SUMMER:
                    itemlist = "forage_summer";
                    break;
                case AUTUMN:
                    itemlist = "forage_autumn";
                    break;
                case WINTER:
                    itemlist = "forage_winter";
                    break;
                default:
                    debugmsg( "Invalid season" );
            }
        }
        auto result = item_group::item_from( itemlist );
        if( !result.is_null() ) {
            popup( _( "%s returned with a %s for you!" ), comp->name, result.tname() );
            g->u.i_add( result );
        }
        if( one_in( 6 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
            p.set_mutation( trait_NPC_MISSION_LEV_1 );
            popup( _( "%s feels more confident in your companions and is willing to let them "
                      "participate in advanced tasks." ), p.name );
        }
    }
    companion_return( *comp );
    return true;
}

bool talk_function::companion_om_combat_check( const std::vector<npc_ptr> &group,
        const tripoint &om_tgt, bool try_engage )
{
    if( overmap_buffer.is_safe( om_tgt ) ) {
        //Should work but is_safe is always returning true regardless of what is there.
        //return true;
    }

    //If the map isn't generated we need to do that...
    if( MAPBUFFER.lookup_submap( om_to_sm_copy( om_tgt ) ) == nullptr ) {
        //This doesn't gen monsters...
        //tinymap tmpmap;
        //tmpmap.generate( om_tgt.x * 2, om_tgt.y * 2, om_tgt.z, calendar::turn );
        //tmpmap.save();
    }

    tinymap target_bay;
    target_bay.load( tripoint( om_tgt.x * 2, om_tgt.y * 2, om_tgt.z ), false );
    std::vector< monster * > monsters_around;
    for( int x = 0; x < 2; x++ ) {
        for( int y = 0; y < 2; y++ ) {
            point sm( ( om_tgt.x * 2 ) + x, ( om_tgt.x * 2 ) + y );
            const point omp = sm_to_om_remain( sm );
            overmap &omi = overmap_buffer.get( omp );

            const tripoint current_submap_loc( tripoint( 2 * om_tgt.x, 2 * om_tgt.y, om_tgt.z ) + point( x,
                                               y ) );
            auto monster_bucket = omi.monster_map.equal_range( current_submap_loc );
            std::for_each( monster_bucket.first,
            monster_bucket.second, [&]( std::pair<const tripoint, monster> &monster_entry ) {
                monster &this_monster = monster_entry.second;
                monsters_around.push_back( &this_monster );
            } );
        }
    }
    float avg_survival = 0;
    for( auto &guy : group ) {
        avg_survival += guy->get_skill_level( skill_survival );
    }
    avg_survival = avg_survival / group.size();

    monster mon;

    std::vector< monster * > monsters_fighting;
    for( auto mons : monsters_around ) {
        if( mons->get_hp() <= 0 ) {
            continue;
        }
        int d_modifier = avg_survival - mons->type->difficulty;
        int roll = rng( 1, 20 ) + d_modifier;
        if( roll > 10 ) {
            if( try_engage ) {
                mons->death_drops = false;
                monsters_fighting.push_back( mons );
            }
        } else {
            mons->death_drops = false;
            monsters_fighting.push_back( mons );
        }
    }

    if( !monsters_fighting.empty() ) {
        bool outcome = force_on_force( group, "Patrol", monsters_fighting, "attacking monsters",
                                       rng( -1, 2 ) );
        for( auto mons : monsters_fighting ) {
            mons->death_drops = true;
        }
        return outcome;
    }
    return true;
}

bool talk_function::force_on_force( const std::vector<npc_ptr> &defender,
                                    const std::string &def_desc,
                                    const std::vector< monster * > &monsters_fighting,
                                    const std::string &att_desc, int advantage )
{
    std::string adv;
    if( advantage < 0 ) {
        adv = ", attacker advantage";
    } else if( advantage > 0 ) {
        adv = ", defender advantage";
    }
    faction *yours = g->u.get_faction();
    //Find out why your followers don't have your faction...
    popup( _( "Engagement between %d members of %s %s and %d %s%s!" ), defender.size(),
           yours->name, def_desc, monsters_fighting.size(), att_desc, adv );
    int defense = 0;
    int attack = 0;
    int att_init = 0;
    int def_init = 0;
    while( true ) {
        std::vector< monster * > remaining_mon;
        for( const auto &elem : monsters_fighting ) {
            if( elem->get_hp() > 0 ) {
                remaining_mon.push_back( elem );
            }
        }
        std::vector<npc_ptr> remaining_def;
        for( const auto &elem : defender ) {
            if( !elem->is_dead() && elem->hp_cur[hp_torso] >= 0 && elem->hp_cur[hp_head] >= 0 ) {
                remaining_def.push_back( elem );
            }
        }

        defense = combat_score( remaining_def );
        attack = combat_score( remaining_mon );
        if( attack > defense * 3 ) {
            attack_random( remaining_mon, remaining_def );
            if( defense == 0 || ( remaining_def.size() == 1 && remaining_def[0]->is_dead() ) ) {
                //Here too...
                popup( _( "%s forces are destroyed!" ), yours->name );
            } else {
                //Again, no faction for your followers
                popup( _( "%s forces retreat from combat!" ), yours->name );
            }
            return false;
        } else if( attack * 3 < defense ) {
            attack_random( remaining_def, remaining_mon );
            if( attack == 0 || ( remaining_mon.size() == 1 && remaining_mon[0]->get_hp() == 0 ) ) {
                popup( _( "The monsters are destroyed!" ) );
            } else {
                popup( _( "The monsters disengage!" ) );
            }
            return true;
        } else {
            def_init = rng( 1, 6 ) + advantage;
            att_init = rng( 1, 6 );
            if( def_init >= att_init ) {
                attack_random( remaining_mon, remaining_def );
            }
            if( def_init <= att_init ) {
                attack_random( remaining_def, remaining_mon );
            }
        }
    }
}

void talk_function::force_on_force( const std::vector<npc_ptr> &defender,
                                    const std::string &def_desc,
                                    const std::vector<npc_ptr> &attacker,
                                    const std::string &att_desc, int advantage )
{
    std::string adv;
    if( advantage < 0 ) {
        adv = ", attacker advantage";
    } else if( advantage > 0 ) {
        adv = ", defender advantage";
    }
    popup( _( "Engagement between %d members of %s %s and %d members of %s %s%s!" ),
           defender.size(), defender[0]->get_faction()->name, def_desc, attacker.size(),
           attacker[0]->get_faction()->name, att_desc, adv );
    int defense = 0;
    int attack = 0;
    int att_init = 0;
    int def_init = 0;
    while( true ) {
        std::vector<npc_ptr> remaining_att;
        for( const auto &elem : attacker ) {
            if( elem->hp_cur[hp_torso] != 0 ) {
                remaining_att.push_back( elem );
            }
        }
        std::vector<npc_ptr> remaining_def;
        for( const auto &elem : defender ) {
            if( elem->hp_cur[hp_torso] != 0 ) {
                remaining_def.push_back( elem );
            }
        }

        defense = combat_score( remaining_def );
        attack = combat_score( remaining_att );
        if( attack > defense * 3 ) {
            attack_random( remaining_att, remaining_def );
            if( defense == 0 || ( remaining_def.size() == 1 &&
                                  remaining_def[0]->hp_cur[hp_torso] == 0 ) ) {
                popup( _( "%s forces are destroyed!" ), defender[0]->get_faction()->name );
            } else {
                popup( _( "%s forces retreat from combat!" ), defender[0]->get_faction()->name );
            }
            return;
        } else if( attack * 3 < defense ) {
            attack_random( remaining_def, remaining_att );
            if( attack == 0 || ( remaining_att.size() == 1 &&
                                 remaining_att[0]->hp_cur[hp_torso] == 0 ) ) {
                popup( _( "%s forces are destroyed!" ), attacker[0]->get_faction()->name );
            } else {
                popup( _( "%s forces retreat from combat!" ), attacker[0]->get_faction()->name );
            }
            return;
        } else {
            def_init = rng( 1, 6 ) + advantage;
            att_init = rng( 1, 6 );
            if( def_init >= att_init ) {
                attack_random( remaining_att, remaining_def );
            }
            if( def_init <= att_init ) {
                attack_random( remaining_def, remaining_att );
            }
        }
    }
}

// skill training support functions
void talk_function::companion_skill_trainer( npc &comp, const std::string &skill_tested,
        time_duration time_worked, int difficulty )
{
    difficulty = std::max( 1, difficulty );
    int checks = 1 + to_minutes<int>( time_worked ) / 10;

    weighted_int_list<skill_id> skill_practice;
    if( skill_tested == "combat" ) {
        const skill_id best_skill = comp.best_skill();
        if( best_skill ) {
            skill_practice.add( best_skill, 30 );
        }
    }
    for( Skill &sk : Skill::skills ) {
        skill_practice.add( sk.ident(), sk.get_companion_skill_practice( skill_tested ) );
    }
    if( skill_practice.empty() ) {
        comp.practice( skill_id( skill_tested ), difficulty * to_minutes<int>( time_worked ) / 10 );
    } else {
        for( int i = 0; i < checks; i++ ) {
            skill_id *ident = skill_practice.pick();
            if( ident ) {
                comp.practice( *ident, difficulty );
            }
        }
    }
}

void talk_function::companion_skill_trainer( npc &comp, const skill_id &skill_tested,
        time_duration time_worked, int difficulty )
{
    difficulty = std::max( 1, difficulty );
    comp.practice( skill_tested, difficulty * to_minutes<int>( time_worked ) / 10 );
}

void talk_function::companion_return( npc &comp )
{
    assert( !comp.is_active() );
    comp.reset_companion_mission();
    comp.companion_mission_time = calendar::before_time_starts;
    comp.companion_mission_time_ret = calendar::before_time_starts;
    for( size_t i = 0; i < comp.companion_mission_inv.size(); i++ ) {
        for( const auto &it : comp.companion_mission_inv.const_stack( i ) ) {
            if( !it.count_by_charges() || it.charges > 0 ) {
                g->m.add_item_or_charges( g->u.pos(), it );
            }
        }
    }
    comp.companion_mission_inv.clear();
    comp.companion_mission_points.clear();
    // npc *may* be active, or not if outside the reality bubble
    g->reload_npcs();
}

std::vector<npc_ptr> talk_function::companion_list( const npc &p, const std::string &mission_id,
        bool contains )
{
    std::vector<npc_ptr> available;
    const tripoint omt_pos = p.global_omt_location();
    for( const auto &elem : overmap_buffer.get_companion_mission_npcs() ) {
        npc_companion_mission c_mission = elem->get_companion_mission();
        if( c_mission.position == omt_pos && c_mission.mission_id == mission_id &&
            c_mission.role_id == p.companion_mission_role_id ) {
            available.push_back( elem );
        } else if( contains && c_mission.mission_id.find( mission_id ) != std::string::npos ) {
            available.push_back( elem );
        }
    }
    return available;
}

static int companion_combat_rank( const npc &p )
{
    int combat = 2 * p.get_dex() + 3 * p.get_str() + 2 * p.get_per() + p.get_int();
    for( const Skill &sk : Skill::skills ) {
        combat += p.get_skill_level( sk.ident() ) * sk.companion_combat_rank_factor();
    }
    return combat * std::min( p.get_dex(), 32 ) * std::min( p.get_str(), 32 ) / 64;
}

static int companion_survival_rank( const npc &p )
{
    int survival = 2 * p.get_dex() + p.get_str() + 2 * p.get_per() + 1.5 * p.get_int();
    for( const Skill &sk : Skill::skills ) {
        survival += p.get_skill_level( sk.ident() ) * sk.companion_survival_rank_factor();
    }
    return survival * std::min( p.get_dex(), 32 ) * std::min( p.get_per(), 32 ) / 64;
}

static int companion_industry_rank( const npc &p )
{
    int industry = p.get_dex() + p.get_str() + p.get_per() + 3 * p.get_int();
    for( const Skill &sk : Skill::skills ) {
        industry += p.get_skill_level( sk.ident() ) * sk.companion_industry_rank_factor();
    }
    return industry * std::min( p.get_int(), 32 ) / 8;
}

static bool companion_sort_compare( const npc_ptr &first, const npc_ptr &second )
{
    return companion_combat_rank( *first ) > companion_combat_rank( *second );
}

comp_list talk_function::companion_sort( comp_list available,
        const std::map<skill_id, int> &required_skills )
{
    if( required_skills.empty() ) {
        std::sort( available.begin(), available.end(), companion_sort_compare );
        return available;
    }
    skill_id hardest_skill;
    int hardest_diff = -1;
    for( const std::pair<const skill_id, int> &req_skill : required_skills ) {
        if( req_skill.second > hardest_diff ) {
            hardest_diff = req_skill.second;
            hardest_skill = req_skill.first;
        }
    }

    struct companion_sort_skill {
        companion_sort_skill( const skill_id  &skill_tested ) {
            req_skill = skill_tested;
        }

        bool operator()( const npc_ptr &first, const npc_ptr &second ) {
            return first->get_skill_level( req_skill ) > second->get_skill_level( req_skill );
        }

        skill_id req_skill;
    };
    std::sort( available.begin(), available.end(), companion_sort_skill( hardest_skill ) );

    return available;
}

std::vector<comp_rank> talk_function::companion_rank( const std::vector<npc_ptr> &available,
        bool adj )
{
    std::vector<comp_rank> raw;
    int max_combat = 0;
    int max_survival = 0;
    int max_industry = 0;
    for( const auto &e : available ) {
        comp_rank r;
        r.combat = companion_combat_rank( *e );
        r.survival = companion_survival_rank( *e );
        r.industry = companion_industry_rank( *e );
        raw.push_back( r );
        if( r.combat > max_combat ) {
            max_combat = r.combat;
        }
        if( r.survival > max_survival ) {
            max_survival = r.survival;
        }
        if( r.industry > max_industry ) {
            max_industry = r.industry;
        }
    }

    if( !adj ) {
        return raw;
    }

    std::vector<comp_rank> adjusted;
    for( const auto &entry : raw ) {
        comp_rank r;
        r.combat = max_combat ? 100 * entry.combat / max_combat : 0;
        r.survival = max_survival ? 100 * entry.survival / max_survival : 0;
        r.industry = max_industry ? 100 * entry.industry / max_industry : 0;
        adjusted.push_back( r );
    }
    return adjusted;
}

npc_ptr talk_function::companion_choose( const std::map<skill_id, int> &required_skills )
{
    std::vector<npc_ptr> available;
    cata::optional<basecamp *> bcp = overmap_buffer.find_camp( g->u.global_omt_location().xy() );

    for( auto &elem : g->get_follower_list() ) {
        npc_ptr guy = overmap_buffer.find_npc( elem );
        if( !guy ) {
            continue;
        }
        npc_companion_mission c_mission = guy->get_companion_mission();
        // get non-assigned visible followers
        if( g->u.posz() == guy->posz() && !guy->has_companion_mission() &&
            !guy->is_travelling() &&
            ( rl_dist( g->u.pos(), guy->pos() ) <= SEEX * 2 ) && g->u.sees( guy->pos() ) ) {
            available.push_back( guy );
        } else if( bcp ) {
            basecamp *player_camp = *bcp;
            std::vector<npc_ptr> camp_npcs = player_camp->get_npcs_assigned();
            if( std::any_of( camp_npcs.begin(), camp_npcs.end(),
            [guy]( const npc_ptr & i ) {
            return i == guy;
        } ) ) {
                available.push_back( guy );
            }
        } else {
            const tripoint &guy_omt_pos = guy->global_omt_location();
            cata::optional<basecamp *> guy_camp = overmap_buffer.find_camp( guy_omt_pos.xy() );
            if( guy_camp ) {
                // get NPCs assigned to guard a remote base
                basecamp *temp_camp = *guy_camp;
                std::vector<npc_ptr> assigned_npcs = temp_camp->get_npcs_assigned();
                if( std::any_of( assigned_npcs.begin(), assigned_npcs.end(),
                [guy]( const npc_ptr & i ) {
                return i == guy;
            } ) ) {
                    available.push_back( guy );
                }
            }
        }
    }
    if( available.empty() ) {
        popup( _( "You don't have any companions to send out…" ) );
        return nullptr;
    }
    std::vector<uilist_entry> npc_menu;
    available = companion_sort( available, required_skills );
    std::vector<comp_rank> rankings = companion_rank( available );

    int x = 0;
    std::string menu_header = left_justify( _( "Who do you want to send?" ), 51 );
    if( required_skills.empty() ) {
        menu_header += _( "[ COMBAT : SURVIVAL : INDUSTRY ]" );
    }
    for( const npc_ptr &e : available ) {
        std::string npc_desc;
        bool can_do = true;
        if( e->mission == NPC_MISSION_GUARD_ALLY ) {
            //~ %1$s: npc name
            npc_desc = string_format( pgettext( "companion", "%1$s (Guarding)" ), e->name );
        } else {
            npc_desc = e->name;
        }
        if( required_skills.empty() ) {
            npc_desc = string_format( pgettext( "companion ranking", "%s [ %4d : %4d : %4d ]" ),
                                      left_justify( npc_desc, 51 ), rankings[x].combat,
                                      rankings[x].survival, rankings[x].industry );
        } else {
            npc_desc = left_justify( npc_desc, 51 );
            bool first = true;
            for( const std::pair<const skill_id, int> &skill_tested : required_skills ) {
                if( first ) {
                    first = false;
                } else {
                    npc_desc += ", ";
                }
                skill_id skill_tested_id = skill_tested.first;
                int skill_level = skill_tested.second;
                if( skill_level == 0 ) {
                    //~ %1$s: skill name, %2$d: companion skill level
                    npc_desc += string_format( pgettext( "companion skill", "%1$s %2$d" ),
                                               skill_tested_id.obj().name(),
                                               e->get_skill_level( skill_tested_id ) );
                } else {
                    //~ %1$s: skill name, %2$d: companion skill level, %3$d: skill requirement
                    npc_desc += string_format( pgettext( "companion skill", "%1$s %2$d/%3$d" ),
                                               skill_tested_id.obj().name(),
                                               e->get_skill_level( skill_tested_id ),
                                               skill_level );
                    can_do &= e->get_skill_level( skill_tested_id ) >= skill_level;
                }
            }
        }
        uilist_entry npc_entry = uilist_entry( x, can_do, x, npc_desc );
        npc_menu.push_back( npc_entry );
        x++;
    }
    const size_t npc_choice = uilist( menu_header, npc_menu );
    if( npc_choice >= available.size() ) {
        popup( _( "You choose to send no one…" ), npc_choice );
        return nullptr;
    }

    return available[npc_choice];
}

npc_ptr talk_function::companion_choose_return( const npc &p, const std::string &mission_id,
        const time_point &deadline )
{
    const tripoint omt_pos = p.global_omt_location();
    const std::string &role_id = p.companion_mission_role_id;
    return companion_choose_return( omt_pos, role_id, mission_id, deadline );
}
npc_ptr talk_function::companion_choose_return( const tripoint &omt_pos,
        const std::string &role_id,
        const std::string &mission_id,
        const time_point &deadline,
        const bool by_mission )
{
    std::vector<npc_ptr> available;
    for( npc_ptr &guy : overmap_buffer.get_companion_mission_npcs() ) {
        npc_companion_mission c_mission = guy->get_companion_mission();
        if( c_mission.position != omt_pos ||
            ( by_mission && c_mission.mission_id != mission_id ) || c_mission.role_id != role_id ) {
            continue;
        }
        if( g->u.has_trait( trait_DEBUG_HS ) ) {
            available.push_back( guy );
        } else if( deadline == calendar::before_time_starts ) {
            if( guy->companion_mission_time_ret <= calendar::turn ) {
                available.push_back( guy );
            }
        } else if( guy->companion_mission_time <= deadline ) {
            available.push_back( guy );
        }
    }

    if( available.empty() ) {
        popup( _( "You don't have any companions ready to return…" ) );
        return nullptr;
    }

    if( available.size() == 1 ) {
        return available[0];
    }

    std::vector<std::string> npcs;
    for( auto &elem : available ) {
        npcs.push_back( ( elem )->name );
    }
    const size_t npc_choice = uilist( _( "Who should return?" ), npcs );
    if( npc_choice < available.size() ) {
        return available[npc_choice];
    }
    popup( _( "No one returns to your party…" ) );
    return nullptr;
}

//Smash stuff, steal valuables, and change map maker
void talk_function::loot_building( const tripoint &site )
{
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    for( const tripoint &p : bay.points_on_zlevel() ) {
        const ter_id t = bay.ter( p );
        //Open all the doors, doesn't need to be exhaustive
        if( t == t_door_c || t == t_door_c_peep || t == t_door_b
            || t == t_door_boarded || t == t_door_boarded_damaged
            || t == t_rdoor_boarded || t == t_rdoor_boarded_damaged
            || t == t_door_boarded_peep || t == t_door_boarded_damaged_peep ) {
            bay.ter_set( p, t_door_o );
        } else if( t == t_door_locked || t == t_door_locked_peep || t == t_door_locked_alarm ) {
            const map_bash_info &bash = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash.ter_set );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
        } else if( t == t_door_metal_c || t == t_door_metal_locked || t == t_door_metal_pickable ) {
            bay.ter_set( p, t_door_metal_o );
        } else if( t == t_door_glass_c ) {
            bay.ter_set( p, t_door_glass_o );
        } else if( t == t_wall && one_in( 25 ) ) {
            const map_bash_info &bash = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash.ter_set );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
            bay.collapse_at( p, false );
        }
        //Smash easily breakable stuff
        else if( ( t == t_window || t == t_window_taped || t == t_window_domestic ||
                   t == t_window_boarded_noglass || t == t_window_domestic_taped ||
                   t == t_window_alarm_taped || t == t_window_boarded ||
                   t == t_curtains || t == t_window_alarm ||
                   t == t_window_no_curtains || t == t_window_no_curtains_taped )
                 && one_in( 4 ) ) {
            const map_bash_info &bash = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash.ter_set );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
        } else if( ( t == t_wall_glass || t == t_wall_glass_alarm ) && one_in( 3 ) ) {
            const map_bash_info &bash = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash.ter_set );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
        } else if( bay.has_furn( p ) && bay.furn( p ).obj().bash.str_max != -1 && one_in( 10 ) ) {
            const map_bash_info &bash = bay.furn( p ).obj().bash;
            bay.furn_set( p, bash.furn_set );
            bay.delete_signage( p );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
        }
        //Kill zombies!  Only works against pre-spawned enemies at the moment...
        Creature *critter = g->critter_at( p );
        if( critter != nullptr ) {
            critter->die( nullptr );
        }
        //Hoover up tasty items!
        map_stack items = bay.i_at( p );
        for( map_stack::iterator it = items.begin(); it != items.end(); ) {
            if( ( ( it->is_food() || it->is_food_container() ) && !one_in( 8 ) ) ||
                ( it->made_of( phase_id::LIQUID ) && !one_in( 8 ) ) ||
                ( it->price( true ) > 1000 && !one_in( 4 ) ) || one_in( 5 ) ) {
                it = items.erase( it );
            } else {
                ++it;
            }
        }
    }
    bay.save();
    overmap_buffer.ter_set( site, oter_id( "looted_building" ) );
}

void mission_data::add( const std::string &id, const std::string &name_display,
                        const std::string &text )
{
    add( id, name_display, cata::nullopt, text, false, true );
}
void mission_data::add_return( const std::string &id, const std::string &name_display,
                               const cata::optional<point> &dir, const std::string &text, bool possible )
{
    add( id, name_display, dir, text, true, possible );
}
void mission_data::add_start( const std::string &id, const std::string &name_display,
                              const cata::optional<point> &dir, const std::string &text, bool possible )
{
    add( id, name_display, dir, text, false, possible );
}
void mission_data::add( const std::string &id, const std::string &name_display,
                        const cata::optional<point> &dir, const std::string &text,
                        bool priority, bool possible )
{
    mission_entry miss;
    miss.id = id;
    if( name_display.empty() ) {
        miss.name_display = id;
    } else {
        miss.name_display = name_display;
    }
    miss.dir = dir;
    miss.text = text;
    miss.priority = priority;
    miss.possible = possible;

    if( priority ) {
        entries[0].push_back( miss );
    }
    if( !possible ) {
        entries[10].push_back( miss );
    }
    const point direction = dir ? *dir : base_camps::base_dir;
    const int tab_order = base_camps::all_directions.at( direction ).tab_order;
    entries[tab_order + 1].emplace_back( miss );
}
