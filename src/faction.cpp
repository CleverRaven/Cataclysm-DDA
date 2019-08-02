#include "faction.h"

#include <assert.h>
#include <cstdlib>
#include <bitset>
#include <map>
#include <string>
#include <memory>
#include <set>
#include <utility>

#include "avatar.h"
#include "basecamp.h"
#include "cursesdef.h"
#include "debug.h"
#include "faction_camp.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "json.h"
#include "line.h"
#include "npc.h"
#include "output.h"
#include "overmapbuffer.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "item.h"
#include "optional.h"
#include "pimpl.h"
#include "type_id.h"
#include "point.h"

namespace npc_factions
{
std::vector<faction_template> all_templates;
} // namespace npc_factions

const faction_id your_faction = faction_id( "your_followers" );

faction_template::faction_template()
{
    likes_u = 0;
    respects_u = 0;
    known_by_u = true;
    food_supply = 0;
    wealth = 0;
    size = 0;
    power = 0;
    currency = "null";
}

faction::faction( const faction_template &templ )
{
    id = templ.id;
    // first init *all* members, than copy those from the template
    static_cast<faction_template &>( *this ) = templ;
}

void faction_template::load( JsonObject &jsobj )
{
    faction_template fac( jsobj );
    npc_factions::all_templates.emplace_back( fac );
}

void faction_template::reset()
{
    npc_factions::all_templates.clear();
}

void faction_template::load_relations( JsonObject &jsobj )
{
    JsonObject jo = jsobj.get_object( "relations" );
    for( const std::string &fac_id : jo.get_member_names() ) {
        JsonObject rel_jo = jo.get_object( fac_id );
        std::bitset<npc_factions::rel_types> fac_relation( 0 );
        for( const auto &rel_flag : npc_factions::relation_strs ) {
            fac_relation.set( rel_flag.second, rel_jo.get_bool( rel_flag.first, false ) );
        }
        relations[fac_id] = fac_relation;
    }
}

faction_template::faction_template( JsonObject &jsobj )
    : name( jsobj.get_string( "name" ) )
    , likes_u( jsobj.get_int( "likes_u" ) )
    , respects_u( jsobj.get_int( "respects_u" ) )
    , known_by_u( jsobj.get_bool( "known_by_u" ) )
    , id( faction_id( jsobj.get_string( "id" ) ) )
    , desc( jsobj.get_string( "description" ) )
    , size( jsobj.get_int( "size" ) )
    , power( jsobj.get_int( "power" ) )
    , food_supply( jsobj.get_int( "food_supply" ) )
    , wealth( jsobj.get_int( "wealth" ) )
{
    if( jsobj.has_string( "currency" ) ) {
        currency = jsobj.get_string( "currency" );
    } else {
        currency = "null";
    }
    load_relations( jsobj );
    mon_faction = jsobj.get_string( "mon_faction", "human" );
}

std::string faction::describe() const
{
    std::string ret = _( desc );
    return ret;
}

// Used in game.cpp
std::string fac_ranking_text( int val )
{
    if( val <= -100 ) {
        return _( "Archenemy" );
    }
    if( val <= -80 ) {
        return _( "Wanted Dead" );
    }
    if( val <= -60 ) {
        return _( "Enemy of the People" );
    }
    if( val <= -40 ) {
        return _( "Wanted Criminal" );
    }
    if( val <= -20 ) {
        return _( "Not Welcome" );
    }
    if( val <= -10 ) {
        return _( "Pariah" );
    }
    if( val <= -5 ) {
        return _( "Disliked" );
    }
    if( val >= 100 ) {
        return _( "Hero" );
    }
    if( val >= 80 ) {
        return _( "Idol" );
    }
    if( val >= 60 ) {
        return _( "Beloved" );
    }
    if( val >= 40 ) {
        return _( "Highly Valued" );
    }
    if( val >= 20 ) {
        return _( "Valued" );
    }
    if( val >= 10 ) {
        return _( "Well-Liked" );
    }
    if( val >= 5 ) {
        return _( "Liked" );
    }

    return _( "Neutral" );
}

// Used in game.cpp
std::string fac_respect_text( int val )
{
    // Respected, feared, etc.
    if( val >= 100 ) {
        return _( "Legendary" );
    }
    if( val >= 80 ) {
        return _( "Unchallenged" );
    }
    if( val >= 60 ) {
        return _( "Mighty" );
    }
    if( val >= 40 ) {
        return _( "Famous" );
    }
    if( val >= 20 ) {
        return _( "Well-Known" );
    }
    if( val >= 10 ) {
        return _( "Spoken Of" );
    }

    // Disrespected, laughed at, etc.
    if( val <= -100 ) {
        return _( "Worthless Scum" );
    }
    if( val <= -80 ) {
        return _( "Vermin" );
    }
    if( val <= -60 ) {
        return _( "Despicable" );
    }
    if( val <= -40 ) {
        return _( "Parasite" );
    }
    if( val <= -20 ) {
        return _( "Leech" );
    }
    if( val <= -10 ) {
        return _( "Laughingstock" );
    }

    return _( "Neutral" );
}

std::string fac_wealth_text( int val, int size )
{
    //Wealth per person
    val = val / size;
    if( val >= 1000000 ) {
        return _( "Filthy rich" );
    }
    if( val >= 750000 ) {
        return _( "Affluent" );
    }
    if( val >= 500000 ) {
        return _( "Prosperous" );
    }
    if( val >= 250000 ) {
        return _( "Well-Off" );
    }
    if( val >= 100000 ) {
        return _( "Comfortable" );
    }
    if( val >= 85000 ) {
        return _( "Wanting" );
    }
    if( val >= 70000 ) {
        return _( "Failing" );
    }
    if( val >= 50000 ) {
        return _( "Impoverished" );
    }
    return _( "Destitute" );
}

std::string faction::food_supply_text()
{
    //Convert to how many days you can support the population
    int val = food_supply / ( size * 288 );
    if( val >= 30 ) {
        return _( "Overflowing" );
    }
    if( val >= 14 ) {
        return _( "Well-Stocked" );
    }
    if( val >= 6 ) {
        return _( "Scrapping By" );
    }
    if( val >= 3 ) {
        return _( "Malnourished" );
    }
    return _( "Starving" );
}

nc_color faction::food_supply_color()
{
    int val = food_supply / ( size * 288 );
    if( val >= 30 ) {
        return c_green;
    } else if( val >= 14 ) {
        return c_light_green;
    } else if( val >= 6 ) {
        return c_yellow;
    } else if( val >= 3 ) {
        return c_light_red;
    } else {
        return c_red;
    }
}

bool faction::has_relationship( const faction_id &guy_id, npc_factions::relationship flag ) const
{
    for( const auto rel_data : relations ) {
        if( rel_data.first == guy_id.c_str() ) {
            return rel_data.second.test( flag );
        }
    }
    return false;
}

std::string fac_combat_ability_text( int val )
{
    if( val >= 150 ) {
        return _( "Legendary" );
    }
    if( val >= 130 ) {
        return _( "Expert" );
    }
    if( val >= 115 ) {
        return _( "Veteran" );
    }
    if( val >= 105 ) {
        return _( "Skilled" );
    }
    if( val >= 95 ) {
        return _( "Competent" );
    }
    if( val >= 85 ) {
        return _( "Untrained" );
    }
    if( val >= 75 ) {
        return _( "Crippled" );
    }
    if( val >= 50 ) {
        return _( "Feeble" );
    }
    return _( "Worthless" );
}

void npc_factions::finalize()
{
    g->faction_manager_ptr->create_if_needed();
}

void faction_manager::clear()
{
    factions.clear();
}

void faction_manager::create_if_needed()
{
    if( !factions.empty() ) {
        return;
    }
    for( const auto &fac_temp : npc_factions::all_templates ) {
        factions.emplace_back( fac_temp );
    }
}

faction *faction_manager::get( const faction_id &id )
{
    for( faction &elem : factions ) {
        if( elem.id == id ) {
            if( !elem.validated ) {
                for( const faction_template &fac_temp : npc_factions::all_templates ) {
                    if( fac_temp.id == id ) {
                        elem.currency = fac_temp.currency;
                        elem.name = fac_temp.name;
                        elem.desc = fac_temp.desc;
                        elem.mon_faction = fac_temp.mon_faction;
                        for( const auto &rel_data : fac_temp.relations ) {
                            if( elem.relations.find( rel_data.first ) == elem.relations.end() ) {
                                elem.relations[rel_data.first] = rel_data.second;
                            }
                        }
                        break;
                    }
                }
                elem.validated = true;
            }
            return &elem;
        }
    }
    for( const faction_template &elem : npc_factions::all_templates ) {
        if( elem.id == id ) {
            factions.emplace_back( elem );
            factions.back().validated = true;
            return &factions.back();
        }
    }

    debugmsg( "Requested non-existing faction '%s'", id.str() );
    return nullptr;
}

void basecamp::faction_display( const catacurses::window &fac_w, const int width ) const
{
    int y = 2;
    const nc_color col = c_white;
    const tripoint player_abspos = g->u.global_omt_location();
    tripoint camp_pos = camp_omt_pos();
    std::string direction = direction_name( direction_from( player_abspos, camp_pos ) );
    mvwprintz( fac_w, ++y, width, c_light_gray, _( "Press enter to rename this camp" ) );
    if( direction != "center" ) {
        mvwprintz( fac_w, ++y, width, c_light_gray, _( "Direction : to the " ) + direction );
    }
    mvwprintz( fac_w, ++y, width, col, _( "Location : (%d, %d)" ), camp_pos.x, camp_pos.y );
    faction *yours = g->faction_manager_ptr->get( your_faction );
    std::string food_text = string_format( _( "Food Supply : %s %d calories" ),
                                           yours->food_supply_text(), yours->food_supply );
    nc_color food_col = yours->food_supply_color();
    mvwprintz( fac_w, ++y, width, food_col, food_text );
    const std::string base_dir = "[B]";
    std::string bldg = next_upgrade( base_dir, 1 );
    std::string bldg_full = _( "Next Upgrade : " ) + bldg;
    mvwprintz( fac_w, ++y, width, col, bldg_full );
    std::string requirements = om_upgrade_description( bldg, true );
    fold_and_print( fac_w, ++y, width, getmaxx( fac_w ) - width - 2, col, requirements );
}

int npc::faction_display( const catacurses::window &fac_w, const int width ) const
{
    int retval = 0;
    int y = 2;
    const nc_color col = c_white;
    const tripoint player_abspos = g->u.global_omt_location();

    //get NPC followers, status, direction, location, needs, weapon, etc.
    mvwprintz( fac_w, ++y, width, c_light_gray, _( "Press enter to talk to this follower " ) );
    std::string mission_string;
    if( has_companion_mission() ) {
        std::string dest_string;
        cata::optional<tripoint> dest = get_mission_destination();
        if( dest ) {
            basecamp *dest_camp;
            cata::optional<basecamp *> temp_camp = overmap_buffer.find_camp( dest->xy() );
            if( temp_camp ) {
                dest_camp = *temp_camp;
                dest_string = _( "travelling to : " ) + dest_camp->camp_name();
            } else {
                dest_string = string_format( _( "travelling to : (%d, %d)" ), dest->x, dest->y );
            }
            mission_string = _( "Current Mission : " ) + dest_string;
        } else {
            npc_companion_mission c_mission = get_companion_mission();
            mission_string = _( "Current Mission : " ) +
                             get_mission_action_string( c_mission.mission_id );
        }
    }
    fold_and_print( fac_w, ++y, width, getmaxx( fac_w ) - width - 2, col, mission_string );
    tripoint guy_abspos = global_omt_location();
    basecamp *stationed_at;
    bool is_stationed = false;
    cata::optional<basecamp *> p = overmap_buffer.find_camp( guy_abspos.xy() );
    if( p ) {
        is_stationed = true;
        stationed_at = *p;
    } else {
        stationed_at = nullptr;
    }
    std::string direction = direction_name( direction_from( player_abspos, guy_abspos ) );
    if( direction != "center" ) {
        mvwprintz( fac_w, ++y, width, col, _( "Direction : to the " ) + direction );
    } else {
        mvwprintz( fac_w, ++y, width, col, _( "Direction : Nearby" ) );
    }
    if( is_stationed ) {
        mvwprintz( fac_w, ++y, width, col, _( "Location : (%d, %d), at camp: %s" ), guy_abspos.x,
                   guy_abspos.y, stationed_at->camp_name() );
    } else {
        mvwprintz( fac_w, ++y, width, col, _( "Location : (%d, %d)" ), guy_abspos.x,
                   guy_abspos.y );
    }
    std::string can_see;
    nc_color see_color;
    bool u_has_radio = g->u.has_item_with_flag( "TWO_WAY_RADIO", true );
    bool guy_has_radio = has_item_with_flag( "TWO_WAY_RADIO", true );
    // TODO NPCS on mission contactable same as travelling
    if( has_companion_mission() && mission != NPC_MISSION_TRAVELLING ) {
        can_see = "Not interactable while on a mission";
        see_color = c_light_red;
        // is the NPC even in the same area as the player?
    } else if( rl_dist( player_abspos, global_omt_location() ) > 3 ||
               ( rl_dist( g->u.pos(), pos() ) > SEEX * 2 || !g->u.sees( pos() ) ) ) {
        if( u_has_radio && guy_has_radio ) {
            // TODO: better range calculation than just elevation.
            int max_range = 200;
            max_range *= ( 1 + ( g->u.pos().z * 0.1 ) );
            max_range *= ( 1 + ( pos().z * 0.1 ) );
            if( is_stationed ) {
                // if camp that NPC is at, has a radio tower
                if( stationed_at->has_provides( "radio_tower" ) ) {
                    max_range *= 5;
                }
            }
            // if camp that player is at, has a radio tower
            cata::optional<basecamp *> player_camp =
                overmap_buffer.find_camp( g->u.global_omt_location().xy() );
            if( const cata::optional<basecamp *> player_camp = overmap_buffer.find_camp(
                        g->u.global_omt_location().xy() ) ) {
                if( ( *player_camp )->has_provides( "radio_tower" ) ) {
                    max_range *= 5;
                }
            }
            if( ( ( g->u.pos().z >= 0 && pos().z >= 0 ) || ( g->u.pos().z == pos().z ) ) &&
                square_dist( g->u.global_sm_location(), global_sm_location() ) <= max_range ) {
                retval = 2;
                can_see = "Within radio range";
                see_color = c_light_green;
            } else {
                can_see = "Not within radio range";
                see_color = c_light_red;
            }
        } else if( guy_has_radio && !u_has_radio ) {
            can_see = "You do not have a radio";
            see_color = c_light_red;
        } else if( !guy_has_radio && u_has_radio ) {
            can_see = "Follower does not have a radio";
            see_color = c_light_red;
        } else {
            can_see = "Both you and follower need a radio";
            see_color = c_light_red;
        }
    } else {
        retval = 1;
        can_see = "Within interaction range";
        see_color = c_light_green;
    }
    mvwprintz( fac_w, ++y, width, see_color, can_see );
    nc_color status_col = col;
    std::string current_status = _( "Status : " );
    if( current_target() != nullptr ) {
        current_status += _( "In Combat!" );
        status_col = c_light_red;
    } else if( in_sleep_state() ) {
        current_status += _( "Sleeping" );
    } else if( is_following() ) {
        current_status += _( "Following" );
    } else if( is_leader() ) {
        current_status += _( "Leading" );
    } else if( is_patrolling() ) {
        current_status += _( "Patrolling" );
    } else if( is_guarding() ) {
        current_status += _( "Guarding" );
    }
    mvwprintz( fac_w, ++y, width, status_col, current_status );

    const std::pair <std::string, nc_color> condition = hp_description();
    mvwprintz( fac_w, ++y, width, condition.second, _( "Condition : " ) + condition.first );
    const std::pair <std::string, nc_color> hunger_pair = get_hunger_description();
    const std::pair <std::string, nc_color> thirst_pair = get_thirst_description();
    const std::pair <std::string, nc_color> fatigue_pair = get_fatigue_description();
    mvwprintz( fac_w, ++y, width, hunger_pair.second,
               _( "Hunger : " ) + ( hunger_pair.first.empty() ? "Nominal" : hunger_pair.first ) );
    mvwprintz( fac_w, ++y, width, thirst_pair.second,
               _( "Thirst : " ) + ( thirst_pair.first.empty() ? "Nominal" : thirst_pair.first ) );
    mvwprintz( fac_w, ++y, width, fatigue_pair.second,
               _( "Fatigue : " ) + ( fatigue_pair.first.empty() ?
                                     "Nominal" : fatigue_pair.first ) );
    int lines = fold_and_print( fac_w, ++y, width, getmaxx( fac_w ) - width - 2, c_white,
                                _( "Wielding : " ) + weapon.tname() );
    y += lines;

    const auto skillslist = Skill::get_skills_sorted_by( [&]( const Skill & a, const Skill & b ) {
        const int level_a = get_skill_level( a.ident() );
        const int level_b = get_skill_level( b.ident() );
        return level_a > level_b || ( level_a == level_b && a.name() < b.name() );
    } );
    size_t count = 0;
    std::vector<std::string> skill_strs;
    for( size_t i = 0; i < skillslist.size() && count < 3; i++ ) {
        if( !skillslist[ i ]->is_combat_skill() ) {
            std::string skill_str = string_format( "%s : %d", skillslist[i]->name(),
                                                   get_skill_level( skillslist[i]->ident() ) );
            skill_strs.push_back( skill_str );
            count += 1;
        }
    }
    std::string best_three_noncombat = _( "Best other skills : " );
    std::string best_skill_text = string_format( _( "Best combat skill : %s : %d" ),
                                  best_skill().obj().name(), best_skill_level() );
    mvwprintz( fac_w, ++y, width, col, best_skill_text );
    mvwprintz( fac_w, ++y, width, col, best_three_noncombat + skill_strs[0] );
    mvwprintz( fac_w, ++y, width + 20, col, skill_strs[1] );
    mvwprintz( fac_w, ++y, width + 20, col, skill_strs[2] );
    return retval;
}

void new_faction_manager::display() const
{
    int term_x = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;
    int term_y = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;

    catacurses::window w_missions = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                    term_x, term_y );

    enum class tab_mode : int {
        TAB_MYFACTION = 0,
        TAB_FOLLOWERS,
        TAB_OTHERFACTIONS,
        NUM_TABS,
        FIRST_TAB = 0,
        LAST_TAB = NUM_TABS - 1
    };
    g->validate_camps();
    g->validate_npc_followers();
    tab_mode tab = tab_mode::FIRST_TAB;
    const int entries_per_page = FULL_SCREEN_HEIGHT - 4;
    size_t selection = 0;
    input_context ctxt( "FACTION MANAGER" );
    ctxt.register_cardinal();
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    while( true ) {
        werase( w_missions );
        // create a list of NPCs, visible and the ones on overmapbuffer
        std::vector<npc *> followers;
        for( auto &elem : g->get_follower_list() ) {
            std::shared_ptr<npc> npc_to_get = overmap_buffer.find_npc( elem );
            if( !npc_to_get ) {
                continue;
            }
            npc *npc_to_add = npc_to_get.get();
            followers.push_back( npc_to_add );
        }
        npc *guy = nullptr;
        bool interactable = false;
        bool radio_interactable = false;
        basecamp *camp = nullptr;
        // create a list of faction camps
        std::vector<basecamp *> camps;
        for( auto elem : g->u.camps ) {
            cata::optional<basecamp *> p = overmap_buffer.find_camp( elem.xy() );
            if( !p ) {
                continue;
            }
            basecamp *temp_camp = *p;
            camps.push_back( temp_camp );
        }
        if( tab < tab_mode::FIRST_TAB || tab >= tab_mode::NUM_TABS ) {
            debugmsg( "The sanity check failed because tab=%d", static_cast<int>( tab ) );
            tab = tab_mode::FIRST_TAB;
        }
        size_t active_vec_size = camps.size();
        // entries_per_page * page number
        const size_t top_of_page = entries_per_page * ( selection / entries_per_page );
        if( tab == tab_mode::TAB_FOLLOWERS ) {
            if( !followers.empty() ) {
                guy = followers[selection];
            }
            active_vec_size = followers.size();
        } else if( tab == tab_mode::TAB_MYFACTION ) {
            if( !camps.empty() ) {
                camp = camps[selection];
            }
        }
        for( int i = 1; i < FULL_SCREEN_WIDTH - 1; i++ ) {
            mvwputch( w_missions, 2, i, BORDER_COLOR, LINE_OXOX );
            mvwputch( w_missions, FULL_SCREEN_HEIGHT - 1, i, BORDER_COLOR, LINE_OXOX );

            if( i > 2 && i < FULL_SCREEN_HEIGHT - 1 ) {
                mvwputch( w_missions, i, 30, BORDER_COLOR, LINE_XOXO );
                mvwputch( w_missions, i, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOXO );
            }
        }
        draw_tab( w_missions, 7, _( "YOUR FACTION" ), tab == tab_mode::TAB_MYFACTION );
        draw_tab( w_missions, 30, _( "YOUR FOLLOWERS" ), tab == tab_mode::TAB_FOLLOWERS );
        draw_tab( w_missions, 56, _( "OTHER FACTIONS" ), tab == tab_mode::TAB_OTHERFACTIONS );

        mvwputch( w_missions, 2, 0, BORDER_COLOR, LINE_OXXO ); // |^
        mvwputch( w_missions, 2, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_OOXX ); // ^|

        mvwputch( w_missions, FULL_SCREEN_HEIGHT - 1, 0, BORDER_COLOR, LINE_XXOO ); // |
        mvwputch( w_missions, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, BORDER_COLOR,
                  LINE_XOOX ); // _|
        mvwputch( w_missions, 2, 30, BORDER_COLOR,
                  tab == tab_mode::TAB_FOLLOWERS ? LINE_XOXX : LINE_XXXX ); // + || -|
        mvwputch( w_missions, FULL_SCREEN_HEIGHT - 1, 30, BORDER_COLOR, LINE_XXOX ); // _|_
        const nc_color col = c_white;
        static const std::string no_camp = _( "You have no camps" );
        static const std::string no_ally = _( "You have no followers" );

        switch( tab ) {
            case tab_mode::TAB_MYFACTION:
                if( active_vec_size > 0 ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size,
                                    3, 0 );
                    for( size_t i = top_of_page; i < active_vec_size; i++ ) {
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, y, 1, 28, selection == i ? hilite( col ) : col,
                                        camps[i]->camp_name() );
                    }
                    if( selection < camps.size() ) {
                        assert( camp ); // To appease static analysis
                        camp->faction_display( w_missions, 31 );
                    } else {
                        mvwprintz( w_missions, 4, 31, c_light_red, no_camp );
                    }
                    break;
                } else {
                    mvwprintz( w_missions, 4, 31, c_light_red, no_camp );
                }
                break;
            case tab_mode::TAB_FOLLOWERS:
                if( !followers.empty() ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size,
                                    3, 0 );
                    for( size_t i = top_of_page; i < active_vec_size; i++ ) {
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, y, 1, 28, selection == i ? hilite( col ) : col,
                                        followers[i]->disp_name() );
                    }
                    if( selection < followers.size() ) {
                        assert( guy ); // To appease static analysis
                        int retval = guy->faction_display( w_missions, 31 );
                        if( retval == 2 ) {
                            radio_interactable = true;
                        } else if( retval == 1 ) {
                            interactable = true;
                        }
                    } else {
                        mvwprintz( w_missions, 4, 31, c_light_red, no_ally );
                    }
                    break;
                } else {
                    mvwprintz( w_missions, 4, 31, c_light_red, no_ally );
                }
                break;
            case tab_mode::TAB_OTHERFACTIONS:
                // Currently the info on factions is incomplete.
                break;
            default:
                break;
        }
        wrefresh( w_missions );
        const std::string action = ctxt.handle_input();
        if( action == "NEXT_TAB" || action == "RIGHT" ) {
            tab = static_cast<tab_mode>( static_cast<int>( tab ) + 1 );
            if( tab >= tab_mode::NUM_TABS ) {
                tab = tab_mode::FIRST_TAB;
            }
            selection = 0;
        } else if( action == "PREV_TAB" || action == "LEFT" ) {
            tab = static_cast<tab_mode>( static_cast<int>( tab ) - 1 );
            if( tab < tab_mode::FIRST_TAB ) {
                tab = tab_mode::LAST_TAB;
            }
            selection = 0;
        } else if( action == "DOWN" ) {
            selection++;
            if( selection >= active_vec_size ) {
                selection = 0;
            }
        } else if( action == "UP" ) {
            if( selection == 0 ) {
                selection = active_vec_size == 0 ? 0 : active_vec_size - 1;
            } else {
                selection--;
            }
        } else if( action == "CONFIRM" ) {
            if( tab == tab_mode::TAB_FOLLOWERS && guy && ( interactable || radio_interactable ) ) {
                guy->talk_to_u( false, radio_interactable );
            } else if( tab == tab_mode::TAB_MYFACTION && camp ) {
                camp->query_new_name();
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }

    g->refresh_all();
}
