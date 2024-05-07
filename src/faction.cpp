#include "faction.h"

#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "avatar.h"
#include "basecamp.h"
#include "calendar.h"
#include "catacharset.h"
#include "character.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "debug.h"
#include "display.h"
#include "faction_camp.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "game_constants.h"
#include "input_context.h"
#include "json_error.h"
#include "line.h"
#include "localized_comparator.h"
#include "mission_companion.h"
#include "mtype.h"
#include "npc.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "point.h"
#include "skill.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"

static const faction_id faction_no_faction( "no_faction" );
static const faction_id faction_your_followers( "your_followers" );

static const flag_id json_flag_TWO_WAY_RADIO( "TWO_WAY_RADIO" );

namespace npc_factions
{
static std::vector<faction_template> all_templates;
} // namespace npc_factions

faction_template::faction_template()
{
    likes_u = 0;
    respects_u = 0;
    trusts_u = 0;
    known_by_u = true;
    food_supply.calories = 0;
    wealth = 0;
    size = 0;
    power = 0;
    lone_wolf_faction = false;
    currency = itype_id::NULL_ID();
}

faction::faction( const faction_template &templ )
{
    id = templ.id;
    // first init *all* members, than copy those from the template
    static_cast<faction_template &>( *this ) = templ;
}

void faction_template::load( const JsonObject &jsobj )
{
    faction_template fac( jsobj );
    npc_factions::all_templates.emplace_back( fac );
}

void faction_template::check_consistency()
{
    for( const faction_template &fac : npc_factions::all_templates ) {
        for( const auto &epi : fac.epilogue_data ) {
            if( !std::get<2>( epi ).is_valid() ) {
                debugmsg( "There's no snippet with id %s", std::get<2>( epi ).str() );
            }
        }
    }
}

void faction_template::reset()
{
    npc_factions::all_templates.clear();
}

void faction_template::load_relations( const JsonObject &jsobj )
{
    for( const JsonMember fac : jsobj.get_object( "relations" ) ) {
        JsonObject rel_jo = fac.get_object();
        std::bitset<npc_factions::rel_types> fac_relation( 0 );
        for( const auto &rel_flag : npc_factions::relation_strs ) {
            fac_relation.set( rel_flag.second, rel_jo.get_bool( rel_flag.first, false ) );
        }
        relations[fac.name()] = fac_relation;
    }
}
faction_price_rule faction_price_rules_reader::get_next( JsonValue &jv )
{
    JsonObject jo = jv.get_object();
    faction_price_rule ret( icg_entry_reader::_part_get_next( jo ) );
    optional( jo, false, "markup", ret.markup, 1.0 );
    optional( jo, false, "premium", ret.premium, 1.0 );
    optional( jo, false, "fixed_adj", ret.fixed_adj, std::nullopt );
    optional( jo, false, "price", ret.price, std::nullopt );
    return ret;
}

faction_template::faction_template( const JsonObject &jsobj )
    : name( jsobj.get_string( "name" ) )
    , likes_u( jsobj.get_int( "likes_u" ) )
    , respects_u( jsobj.get_int( "respects_u" ) )
    , trusts_u( jsobj.get_int( "trusts_u", 0 ) )
    , known_by_u( jsobj.get_bool( "known_by_u" ) )
    , id( faction_id( jsobj.get_string( "id" ) ) )
    , size( jsobj.get_int( "size" ) )
    , power( jsobj.get_int( "power" ) )
    , food_supply()
    , wealth( jsobj.get_int( "wealth" ) )
{
    jsobj.get_member( "description" ).read( desc );
    optional( jsobj, false, "price_rules", price_rules, faction_price_rules_reader {} );
    jsobj.read( "fac_food_supply", food_supply, true );
    if( jsobj.has_string( "currency" ) ) {
        jsobj.read( "currency", currency, true );
        price_rules.emplace_back( currency, 1, 0 );
    } else {
        currency = itype_id::NULL_ID();
    }
    lone_wolf_faction = jsobj.get_bool( "lone_wolf_faction", false );
    load_relations( jsobj );
    mon_faction = mfaction_str_id( jsobj.get_string( "mon_faction", "human" ) );
    for( const JsonObject jao : jsobj.get_array( "epilogues" ) ) {
        epilogue_data.emplace( jao.get_int( "power_min", std::numeric_limits<int>::min() ),
                               jao.get_int( "power_max", std::numeric_limits<int>::max() ),
                               snippet_id( jao.get_string( "id", "epilogue_faction_default" ) ) );
    }
}

std::string faction::describe() const
{
    std::string ret = desc.translated();
    return ret;
}

std::vector<std::string> faction::epilogue() const
{
    std::vector<std::string> ret;
    for( const std::tuple<int, int, snippet_id> &epilogue_entry : epilogue_data ) {
        if( power >= std::get<0>( epilogue_entry ) && power < std::get<1>( epilogue_entry ) ) {
            ret.emplace_back( std::get<2>( epilogue_entry )->translated() );
        }
    }
    return ret;
}

void faction::add_to_membership( const character_id &guy_id, const std::string &guy_name,
                                 const bool known )
{
    members[guy_id] = std::make_pair( guy_name, known );
}

void faction::remove_member( const character_id &guy_id )
{
    for( auto it = members.cbegin(), next_it = it; it != members.cend(); it = next_it ) {
        ++next_it;
        if( guy_id == it->first ) {
            members.erase( it );
            break;
        }
    }
    if( members.empty() ) {
        for( const faction_template &elem : npc_factions::all_templates ) {
            // This is a templated base faction - don't delete it, just leave it as zero members for now.
            // Only want to delete dynamically created factions.
            if( elem.id == id ) {
                return;
            }
        }
        g->faction_manager_ptr->remove_faction( id );
    }
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
        return pgettext( "Faction respect", "Legendary" );
    }
    if( val >= 80 ) {
        return pgettext( "Faction respect", "Unchallenged" );
    }
    if( val >= 60 ) {
        return pgettext( "Faction respect", "Mighty" );
    }
    if( val >= 40 ) {
        return pgettext( "Faction respect", "Famous" );
    }
    if( val >= 20 ) {
        return pgettext( "Faction respect", "Well-Known" );
    }
    if( val >= 10 ) {
        return pgettext( "Faction respect", "Spoken Of" );
    }

    // Disrespected, laughed at, etc.
    if( val <= -100 ) {
        return pgettext( "Faction respect", "Worthless Scum" );
    }
    if( val <= -80 ) {
        return pgettext( "Faction respect", "Vermin" );
    }
    if( val <= -60 ) {
        return pgettext( "Faction respect", "Despicable" );
    }
    if( val <= -40 ) {
        return pgettext( "Faction respect", "Parasite" );
    }
    if( val <= -20 ) {
        return pgettext( "Faction respect", "Leech" );
    }
    if( val <= -10 ) {
        return pgettext( "Faction respect", "Laughingstock" );
    }

    return pgettext( "Faction respect", "Neutral" );
}

std::string fac_wealth_text( int val, int size )
{
    //Wealth per person
    val = val / size;
    if( val >= 1000000 ) {
        return pgettext( "Faction wealth", "Filthy rich" );
    }
    if( val >= 750000 ) {
        return pgettext( "Faction wealth", "Affluent" );
    }
    if( val >= 500000 ) {
        return pgettext( "Faction wealth", "Prosperous" );
    }
    if( val >= 250000 ) {
        return pgettext( "Faction wealth", "Well-Off" );
    }
    if( val >= 100000 ) {
        return pgettext( "Faction wealth", "Comfortable" );
    }
    if( val >= 85000 ) {
        return pgettext( "Faction wealth", "Wanting" );
    }
    if( val >= 70000 ) {
        return pgettext( "Faction wealth", "Failing" );
    }
    if( val >= 50000 ) {
        return pgettext( "Faction wealth", "Impoverished" );
    }
    return pgettext( "Faction wealth", "Destitute" );
}

std::string faction::food_supply_text()
{
    //Convert to how many days you can support the population
    int val = food_supply.kcal() / ( size * 288 );
    if( val >= 30 ) {
        return pgettext( "Faction food", "Overflowing" );
    }
    if( val >= 14 ) {
        return pgettext( "Faction food", "Well-Stocked" );
    }
    if( val >= 6 ) {
        return pgettext( "Faction food", "Scrapping By" );
    }
    if( val >= 3 ) {
        return pgettext( "Faction food", "Malnourished" );
    }
    return pgettext( "Faction food", "Starving" );
}

nc_color faction::food_supply_color()
{
    int val = food_supply.kcal() / ( size * 288 );
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

std::pair<nc_color, std::string> faction::vitamin_stores( vitamin_type vit_type )
{
    bool is_toxin = vit_type == vitamin_type::TOXIN;
    const double days_of_food = food_supply.kcal() / 3000.0;
    std::map<vitamin_id, int> stored_vits = food_supply.vitamins();
    // First, pare down our search to only the relevant type
    for( auto it = stored_vits.cbegin(); it != stored_vits.cend(); ) {
        if( it->first->type() != vit_type ) {
            it = stored_vits.erase( it );
        } else {
            ++it;
        }
    }
    if( stored_vits.empty() ) {
        return std::pair<nc_color, std::string>( !is_toxin ? c_red : c_green, _( "None present (NONE)" ) );
    }
    std::vector<std::pair<vitamin_id, double>> vitamins;
    // Iterate the map's content into a sortable container...
    for( auto &vit : stored_vits ) {
        int units_per_day = vit.first.obj().units_absorption_per_day();
        double relative_intake = static_cast<double>( vit.second ) / static_cast<double>
                                 ( units_per_day ) / days_of_food;
        // We use the inverse value for toxins, since they are bad.
        if( is_toxin ) {
            relative_intake = 1 / relative_intake;
        }
        vitamins.emplace_back( vit.first, relative_intake );
    }
    // Sort to find the worst-case scenario, lowest relative_intake is first
    std::sort( vitamins.begin(), vitamins.end(), []( const auto & x, const auto & y ) {
        return x.second > y.second;
    } );
    const double worst_intake = vitamins.at( 0 ).second;
    std::string vit_name = vitamins.at( 0 ).first.obj().name();
    std::string msg = is_toxin ? _( "(TRACE)" ) : _( "(PLENTY)" );
    if( worst_intake <= 0.3 ) {
        msg = is_toxin ? _( "(POISON)" ) : _( "(LACK)" );
        return std::pair<nc_color, std::string>( c_red, string_format( _( "%1$s %2$s" ), vit_name,
                msg ) );
    }
    if( worst_intake <= 1.0 ) {
        msg = is_toxin ? _( "(DANGER)" ) : _( "(MEAGER)" );
        return std::pair<nc_color, std::string>( c_yellow, string_format( _( "%1$s %2$s" ), vit_name,
                msg ) );
    }
    return std::pair<nc_color, std::string>( c_green, string_format( _( "%1$s %2$s" ), vit_name,
            msg ) );
}

faction_price_rule const *faction::get_price_rules( item const &it, npc const &guy ) const
{
    auto const el = std::find_if(
    price_rules.crbegin(), price_rules.crend(), [&it, &guy]( faction_price_rule const & fc ) {
        return fc.matches( it, guy );
    } );
    if( el != price_rules.crend() ) {
        return &*el;
    }
    return nullptr;
}

bool faction::has_relationship( const faction_id &guy_id, npc_factions::relationship flag ) const
{
    for( const auto &rel_data : relations ) {
        if( rel_data.first == guy_id.c_str() ) {
            return rel_data.second.test( flag );
        }
    }
    return false;
}

std::string fac_combat_ability_text( int val )
{
    if( val >= 150 ) {
        return pgettext( "Faction combat lvl", "Legendary" );
    }
    if( val >= 130 ) {
        return pgettext( "Faction combat lvl", "Expert" );
    }
    if( val >= 115 ) {
        return pgettext( "Faction combat lvl", "Veteran" );
    }
    if( val >= 105 ) {
        return pgettext( "Faction combat lvl", "Skilled" );
    }
    if( val >= 95 ) {
        return pgettext( "Faction combat lvl", "Competent" );
    }
    if( val >= 85 ) {
        return pgettext( "Faction combat lvl", "Untrained" );
    }
    if( val >= 75 ) {
        return pgettext( "Faction combat lvl", "Crippled" );
    }
    if( val >= 50 ) {
        return pgettext( "Faction combat lvl", "Feeble" );
    }
    return pgettext( "Faction combat lvl", "Worthless" );
}

void npc_factions::finalize()
{
    g->faction_manager_ptr->create_if_needed();
}

void faction_manager::clear()
{
    factions.clear();
}

void faction_manager::remove_faction( const faction_id &id )
{
    if( id.str().empty() || id == faction_no_faction ) {
        return;
    }
    for( auto it = factions.cbegin(), next_it = it; it != factions.cend(); it = next_it ) {
        ++next_it;
        if( id == it->first ) {
            factions.erase( it );
            break;
        }
    }
}

void faction_manager::create_if_needed()
{
    if( !factions.empty() ) {
        return;
    }
    for( const faction_template &fac_temp : npc_factions::all_templates ) {
        factions[fac_temp.id] = faction( fac_temp );
    }
}

faction *faction_manager::add_new_faction( const std::string &name_new, const faction_id &id_new,
        const faction_id &template_id )
{
    for( const faction_template &fac_temp : npc_factions::all_templates ) {
        if( template_id == fac_temp.id ) {
            faction fac( fac_temp );
            fac.name = name_new;
            fac.id = id_new;
            factions[fac.id] = fac;
            return &factions[fac.id];
        }
    }
    return nullptr;
}

faction *faction_manager::get( const faction_id &id, const bool complain )
{
    if( id.is_null() ) {
        return get( faction_no_faction );
    }
    for( auto &elem : factions ) {
        if( elem.first == id ) {
            if( !elem.second.validated ) {
                for( const faction_template &fac_temp : npc_factions::all_templates ) {
                    if( fac_temp.id == id ) {
                        elem.second.currency = fac_temp.currency;
                        elem.second.price_rules = fac_temp.price_rules;
                        elem.second.lone_wolf_faction = fac_temp.lone_wolf_faction;
                        elem.second.name = fac_temp.name;
                        elem.second.desc = fac_temp.desc;
                        elem.second.mon_faction = fac_temp.mon_faction;
                        elem.second.epilogue_data = fac_temp.epilogue_data;
                        for( const auto &rel_data : fac_temp.relations ) {
                            if( elem.second.relations.find( rel_data.first ) == elem.second.relations.end() ) {
                                elem.second.relations[rel_data.first] = rel_data.second;
                            }
                        }
                        break;
                    }
                }
                elem.second.validated = true;
            }
            return &elem.second;
        }
    }
    for( const faction_template &elem : npc_factions::all_templates ) {
        // id isn't already in factions map, so load in the template.
        if( elem.id == id ) {
            factions[elem.id] = faction( elem );
            if( !factions.empty() ) {
                factions[elem.id].validated = true;
            }
            return &factions[elem.id];
        }
    }
    // Sometimes we add new IDs to the map, sometimes we want to check if its already there.
    if( complain ) {
        debugmsg( "Requested non-existing faction '%s'", id.str() );
    }
    return nullptr;
}

template<>
bool string_id<faction>::is_valid() const
{
    return g->faction_manager_ptr->get( *this, false ) != nullptr;
}

void basecamp::faction_display( const catacurses::window &fac_w, const int width ) const
{
    int y = 2;
    const nc_color col = c_white;
    Character &player_character = get_player_character();
    const tripoint_abs_omt player_abspos = player_character.global_omt_location();
    tripoint_abs_omt camp_pos = camp_omt_pos();
    std::string direction = direction_name( direction_from( player_abspos, camp_pos ) );
    mvwprintz( fac_w, point( width, ++y ), c_light_gray, _( "Press enter to rename this camp" ) );
    if( direction != "center" ) {
        mvwprintz( fac_w, point( width, ++y ), c_light_gray, _( "Direction: to the " ) + direction );
    }
    mvwprintz( fac_w, point( width, ++y ), col, _( "Location: %s" ), camp_pos.to_string() );
    faction *yours = player_character.get_faction();
    std::string food_text = string_format( _( "Food Supply: %s %d kilocalories" ),
                                           yours->food_supply_text(), yours->food_supply.kcal() );
    nc_color food_col = yours->food_supply_color();
    mvwprintz( fac_w, point( width, ++y ), food_col, food_text );
    std::pair<nc_color, std::string> vitamins = yours->vitamin_stores( vitamin_type::VITAMIN );
    mvwprintz( fac_w, point( width, ++y ), vitamins.first, _( "Worst vitamin:" ) + vitamins.second );
    std::pair<nc_color, std::string> toxins = yours->vitamin_stores( vitamin_type::TOXIN );
    mvwprintz( fac_w, point( width, ++y ), toxins.first, _( "Worst toxin:" ) + toxins.second );
    std::string bldg = next_upgrade( base_camps::base_dir, 1 );
    std::string bldg_full = _( "Next Upgrade: " ) + bldg;
    mvwprintz( fac_w, point( width, ++y ), col, bldg_full );
}

void faction::faction_display( const catacurses::window &fac_w, const int width ) const
{
    int y = 2;
    mvwprintz( fac_w, point( width, ++y ), c_light_gray, _( "Attitude to you:           %s" ),
               fac_ranking_text( likes_u ) );
    fold_and_print( fac_w, point( width, ++y ), getmaxx( fac_w ) - width - 2, c_light_gray,
                    "%s", desc );
}

std::string npc::get_current_status() const
{
    if( current_target() != nullptr ) {
        return _( "In Combat!" );
    } else if( in_sleep_state() ) {
        return _( "Sleeping" );
    } else if( is_following() ) {
        return _( "Following" );
    } else if( is_leader() ) {
        return _( "Leading" );
    } else if( is_patrolling() ) {
        return _( "Patrolling" );
    } else if( is_guarding() ) {
        return _( "Guarding" );
    } else {
        return get_current_activity();
    }
}

int npc::faction_display( const catacurses::window &fac_w, const int width ) const
{
    int retval = 0;
    int y = 2;
    const nc_color col = c_white;
    Character &player_character = get_player_character();
    const tripoint_abs_omt player_abspos = player_character.global_omt_location();

    //get NPC followers, status, direction, location, needs, weapon, etc.
    mvwprintz( fac_w, point( width, ++y ), c_light_gray, _( "Press enter to talk to this follower " ) );
    std::string mission_string;
    if( has_companion_mission() ) {
        std::string dest_string;
        std::optional<tripoint_abs_omt> dest = get_mission_destination();
        if( dest ) {
            basecamp *dest_camp;
            std::optional<basecamp *> temp_camp = overmap_buffer.find_camp( dest->xy() );
            if( temp_camp ) {
                dest_camp = *temp_camp;
                dest_string = _( "traveling to: " ) + dest_camp->camp_name();
            } else {
                dest_string = string_format( _( "traveling to: %s" ), dest->to_string() );
            }
            mission_string = _( "Current Mission: " ) + dest_string;
        } else {
            npc_companion_mission c_mission = get_companion_mission();
            mission_string = _( "Current Mission: " ) + action_of( c_mission.miss_id.id );
        }
        fold_and_print( fac_w, point( width, ++y ), getmaxx( fac_w ) - width - 2, col, mission_string );

        // Determine remaining time in mission, and display it
        std::string mission_eta;
        if( companion_mission_time_ret < calendar::turn ) {
            mission_eta = _( "JOB COMPLETED" );
        } else {
            mission_eta = _( "ETA: " ) + to_string( companion_mission_time_ret - calendar::turn );
        }
        fold_and_print( fac_w, point( width, ++y ), getmaxx( fac_w ) - width - 2, col, mission_eta );
    }

    tripoint_abs_omt guy_abspos = global_omt_location();
    basecamp *temp_camp = nullptr;
    if( assigned_camp ) {
        std::optional<basecamp *> bcp = overmap_buffer.find_camp( ( *assigned_camp ).xy() );
        if( bcp ) {
            temp_camp = *bcp;
        }
    }
    const bool is_stationed = assigned_camp && temp_camp;
    std::string direction = direction_name( direction_from( player_abspos, guy_abspos ) );
    if( direction != "center" ) {
        mvwprintz( fac_w, point( width, ++y ), col, _( "Direction: to the " ) + direction );
    } else {
        mvwprintz( fac_w, point( width, ++y ), col, _( "Direction: Nearby" ) );
    }
    if( is_stationed ) {
        mvwprintz( fac_w, point( width, ++y ), col, _( "Location: %s, at camp: %s" ),
                   guy_abspos.to_string(), temp_camp->camp_name() );
    } else {
        mvwprintz( fac_w, point( width, ++y ), col, _( "Location: %s" ), guy_abspos.to_string() );
    }
    std::string can_see;
    nc_color see_color;

    bool u_has_radio = player_character.cache_has_item_with_flag( json_flag_TWO_WAY_RADIO, true );
    bool guy_has_radio = cache_has_item_with_flag( json_flag_TWO_WAY_RADIO, true );
    // is the NPC even in the same area as the player?
    if( rl_dist( player_abspos, global_omt_location() ) > 3 ||
        ( rl_dist( player_character.pos(), pos() ) > SEEX * 2 || !player_character.sees( pos() ) ) ) {
        if( u_has_radio && guy_has_radio ) {
            if( !( player_character.pos().z >= 0 && pos().z >= 0 ) &&
                !( player_character.pos().z == pos().z ) ) {
                //Early exit
                can_see = _( "Not within radio range" );
                see_color = c_light_red;
            } else {
                // TODO: better range calculation than just elevation.
                const int base_range = 200;
                float send_elev_boost = ( 1 + ( player_character.pos().z * 0.1 ) );
                float recv_elev_boost = ( 1 + ( pos().z * 0.1 ) );
                if( ( square_dist( player_character.global_sm_location(),
                                   global_sm_location() ) <= base_range * send_elev_boost * recv_elev_boost ) ) {
                    //Direct radio contact, both of their elevation are in effect
                    retval = 2;
                    can_see = _( "Within radio range" );
                    see_color = c_light_green;
                } else {
                    //contact via camp radio tower
                    int recv_range = base_range * recv_elev_boost;
                    int send_range = base_range * send_elev_boost;
                    const int radio_tower_boost = 5;
                    // find camps that are near player or npc
                    const std::vector<camp_reference> &camps_near_player = overmap_buffer.get_camps_near(
                                player_character.global_sm_location(), send_range * radio_tower_boost );
                    const std::vector<camp_reference> &camps_near_npc = overmap_buffer.get_camps_near(
                                global_sm_location(), recv_range * radio_tower_boost );
                    bool camp_to_npc = false;
                    bool camp_to_camp = false;
                    for( const camp_reference &i : camps_near_player ) {
                        if( !i.camp->has_provides( "radio" ) ) {
                            continue;
                        }
                        if( camp_to_camp ||
                            square_dist( i.abs_sm_pos, global_sm_location() ) <= recv_range * radio_tower_boost ) {
                            //one radio tower relay
                            camp_to_npc = true;
                            break;
                        }
                        for( const camp_reference &j : camps_near_npc ) {
                            //two radio tower relays
                            if( ( j.camp )->has_provides( "radio" ) &&
                                ( square_dist( i.abs_sm_pos, j.abs_sm_pos ) <= base_range * radio_tower_boost *
                                  radio_tower_boost ) ) {
                                camp_to_camp = true;
                                break;
                            }
                        }
                    }
                    if( camp_to_npc || camp_to_camp ) {
                        retval = 2;
                        can_see = _( "Within radio range" );
                        see_color = c_light_green;
                    } else {
                        can_see = _( "Not within radio range" );
                        see_color = c_light_red;
                    }
                }
            }
        } else if( guy_has_radio && !u_has_radio ) {
            can_see = _( "You do not have a radio" );
            see_color = c_light_red;
        } else if( !guy_has_radio && u_has_radio ) {
            can_see = _( "Follower does not have a radio" );
            see_color = c_light_red;
        } else {
            can_see = _( "Both you and follower need a radio" );
            see_color = c_light_red;
        }
    } else {
        retval = 1;
        can_see = _( "Within interaction range" );
        see_color = c_light_green;
    }
    // TODO: NPCS on mission contactable same as traveling
    if( has_companion_mission() ) {
        can_see = _( "Press enter to recall from their mission." );
        see_color = c_light_red;
    }
    mvwprintz( fac_w, point( width, ++y ), see_color, "%s", can_see );
    nc_color status_col = col;
    if( current_target() != nullptr ) {
        status_col = c_light_red;
    }
    mvwprintz( fac_w, point( width, ++y ), status_col, _( "Status: " ) + get_current_status() );
    if( is_stationed && has_job() ) {
        mvwprintz( fac_w, point( width, ++y ), col, _( "Working at camp" ) );
    } else if( is_stationed ) {
        mvwprintz( fac_w, point( width, ++y ), col, _( "Idling at camp" ) );
    }

    const std::pair <std::string, nc_color> condition = hp_description();
    mvwprintz( fac_w, point( width, ++y ), condition.second, _( "Condition: " ) + condition.first );
    const std::pair <std::string, nc_color> hunger_pair = display::hunger_text_color( *this );
    const std::pair <std::string, nc_color> thirst_pair = display::thirst_text_color( *this );
    const std::pair <std::string, nc_color> sleepiness_pair = display::sleepiness_text_color( *this );
    const std::string nominal = pgettext( "needs", "Nominal" );
    mvwprintz( fac_w, point( width, ++y ), hunger_pair.second,
               _( "Hunger: " ) + ( hunger_pair.first.empty() ? nominal : hunger_pair.first ) );
    mvwprintz( fac_w, point( width, ++y ), thirst_pair.second,
               _( "Thirst: " ) + ( thirst_pair.first.empty() ? nominal : thirst_pair.first ) );
    mvwprintz( fac_w, point( width, ++y ), sleepiness_pair.second,
               _( "Sleepiness: " ) + ( sleepiness_pair.first.empty() ? nominal : sleepiness_pair.first ) );
    int lines = fold_and_print( fac_w, point( width, ++y ), getmaxx( fac_w ) - width - 2, c_white,
                                _( "Wielding: " ) + weapname_simple() );
    y += lines;

    const auto skillslist = Skill::get_skills_sorted_by( [&]( const Skill & a, const Skill & b ) {
        const int level_a = get_skill_level( a.ident() );
        const int level_b = get_skill_level( b.ident() );
        return localized_compare( std::make_pair( -level_a, a.name() ),
                                  std::make_pair( -level_b, b.name() ) );
    } );
    size_t count = 0;
    std::vector<std::string> skill_strs;
    for( size_t i = 0; i < skillslist.size() && count < 3; i++ ) {
        if( !skillslist[ i ]->is_combat_skill() ) {
            std::string skill_str = string_format( "%s: %d", skillslist[i]->name(),
                                                   static_cast<int>( get_skill_level( skillslist[i]->ident() ) ) );
            skill_strs.push_back( skill_str );
            count += 1;
        }
    }
    std::string best_three_noncombat = _( "Best other skills: " );
    std::string best_skill_text = string_format( _( "Best combat skill: %s: %d" ),
                                  best_combat_skill( combat_skills::NO_GENERAL ).first.obj().name(),
                                  best_combat_skill( combat_skills::NO_GENERAL ).second );
    mvwprintz( fac_w, point( width, ++y ), col, best_skill_text );
    mvwprintz( fac_w, point( width, ++y ), col, best_three_noncombat + skill_strs[0] );
    mvwprintz( fac_w, point( width + utf8_width( best_three_noncombat ), ++y ), col, skill_strs[1] );
    mvwprintz( fac_w, point( width + utf8_width( best_three_noncombat ), ++y ), col, skill_strs[2] );
    return retval;
}

void faction_manager::display() const
{
    catacurses::window w_missions;
    int entries_per_page = 0;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const point term( TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                          TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );

        w_missions = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                         point( term.y, term.x ) );

        entries_per_page = FULL_SCREEN_HEIGHT - 4;

        ui.position_from_window( w_missions );
    } );
    ui.mark_resize();

    enum class tab_mode : int {
        TAB_MYFACTION = 0,
        TAB_FOLLOWERS,
        TAB_OTHERFACTIONS,
        TAB_LORE,
        TAB_CREATURES,
        NUM_TABS,
        FIRST_TAB = 0,
        LAST_TAB = NUM_TABS - 1
    };
    g->validate_camps();
    g->validate_npc_followers();
    tab_mode tab = tab_mode::FIRST_TAB;
    size_t selection = 0;
    input_context ctxt( "FACTION_MANAGER" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    std::vector<npc *> followers;
    std::vector<const faction *> valfac; // Factions that we know of.
    npc *guy = nullptr;
    const faction *cur_fac = nullptr;
    bool interactable = false;
    bool radio_interactable = false;
    basecamp *camp = nullptr;
    std::vector<basecamp *> camps;
    size_t active_vec_size = 0;
    std::vector<std::pair<snippet_id, std::string>> lore; // Lore we have seen
    std::pair<snippet_id, std::string> *snippet = nullptr;
    std::vector<mtype_id> creatures; // Creatures we've recorded
    mtype_id cur_creature = mtype_id::NULL_ID();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_missions );

        for( int i = 3; i < FULL_SCREEN_HEIGHT - 1; i++ ) {
            mvwputch( w_missions, point( 30, i ), BORDER_COLOR, LINE_XOXO );
        }

        const std::vector<std::pair<tab_mode, std::string>> tabs = {
            { tab_mode::TAB_MYFACTION, _( "YOUR FACTION" ) },
            { tab_mode::TAB_FOLLOWERS, _( "YOUR FOLLOWERS" ) },
            { tab_mode::TAB_OTHERFACTIONS, _( "OTHER FACTIONS" ) },
            { tab_mode::TAB_LORE, _( "LORE" ) },
            { tab_mode::TAB_CREATURES, _( "CREATURES" ) },
        };
        draw_tabs( w_missions, tabs, tab );
        draw_border_below_tabs( w_missions );

        mvwputch( w_missions, point( 30, 2 ), BORDER_COLOR,
                  tab == tab_mode::TAB_FOLLOWERS ? ' ' : LINE_OXXX ); // ^|^
        mvwputch( w_missions, point( 30, FULL_SCREEN_HEIGHT - 1 ), BORDER_COLOR, LINE_XXOX ); // _|_
        const nc_color col = c_white;

        // entries_per_page * page number
        const size_t top_of_page = entries_per_page * ( selection / entries_per_page );

        switch( tab ) {
            case tab_mode::TAB_MYFACTION: {
                const std::string no_camp = _( "You have no camps" );
                if( active_vec_size > 0 ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size,
                                    point( 0, 3 ) );
                    for( size_t i = top_of_page; i < active_vec_size && i < top_of_page + entries_per_page; i++ ) {
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, point( 1, y ), 28, selection == i ? hilite( col ) : col,
                                        camps[i]->camp_name() );
                    }
                    if( camp ) {
                        camp->faction_display( w_missions, 31 );
                    } else {
                        mvwprintz( w_missions, point( 31, 4 ), c_light_red, no_camp );
                    }
                    break;
                } else {
                    mvwprintz( w_missions, point( 31, 4 ), c_light_red, no_camp );
                }
            }
            break;
            case tab_mode::TAB_FOLLOWERS: {
                const std::string no_ally = _( "You have no followers" );
                if( !followers.empty() ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size,
                                    point( 0, 3 ) );
                    for( size_t i = top_of_page; i < active_vec_size && i < top_of_page + entries_per_page; i++ ) {
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, point( 1, y ), 28, selection == i ? hilite( col ) : col,
                                        followers[i]->disp_name() );
                    }
                    if( guy ) {
                        int retval = guy->faction_display( w_missions, 31 );
                        if( retval == 2 ) {
                            radio_interactable = true;
                        } else if( retval == 1 ) {
                            interactable = true;
                        }
                    } else {
                        mvwprintz( w_missions, point( 31, 4 ), c_light_red, no_ally );
                    }
                    break;
                } else {
                    mvwprintz( w_missions, point( 31, 4 ), c_light_red, no_ally );
                }
            }
            break;
            case tab_mode::TAB_OTHERFACTIONS: {
                const std::string no_fac = _( "You don't know of any factions." );
                if( active_vec_size > 0 ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size,
                                    point( 0, 3 ) );
                    for( size_t i = top_of_page; i < active_vec_size && i < top_of_page + entries_per_page; i++ ) {
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, point( 1, y ), 28, selection == i ? hilite( col ) : col,
                                        _( valfac[i]->name ) );
                    }
                    if( cur_fac ) {
                        cur_fac->faction_display( w_missions, 31 );
                    } else {
                        mvwprintz( w_missions, point( 31, 4 ), c_light_red, no_fac );
                    }
                    break;
                } else {
                    mvwprintz( w_missions, point( 31, 4 ), c_light_red, no_fac );
                }
            }
            break;
            case tab_mode::TAB_LORE: {
                const std::string no_lore = _( "You haven't learned anything about the world." );
                if( active_vec_size > 0 ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size,
                                    point( 0, 3 ) );
                    for( size_t i = top_of_page; i < active_vec_size && i < top_of_page + entries_per_page; i++ ) {
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, point( 1, y ), 28, selection == i ? hilite( col ) : col,
                                        _( lore[i].second ) );
                    }
                    if( snippet != nullptr ) {
                        int y = 2;
                        fold_and_print( w_missions, point( 31, ++y ), getmaxx( w_missions ) - 31 - 2, c_light_gray,
                                        SNIPPET.get_snippet_by_id( snippet->first ).value().translated() );
                    } else {
                        mvwprintz( w_missions, point( 31, 4 ), c_light_red, no_lore );
                    }
                    break;
                } else {
                    mvwprintz( w_missions, point( 31, 4 ), c_light_red, no_lore );
                }
            }
            break;
            case tab_mode::TAB_CREATURES: {
                const std::string no_creatures =
                    _( "You haven't recorded sightings of any creatures.  Taking photos can be a good way to keep track of them." );
                const int w = getmaxx( w_missions ) - 31 - 2;
                if( active_vec_size > 0 ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size,
                                    point( 0, 3 ) );
                    for( size_t i = top_of_page; i < active_vec_size && i < top_of_page + entries_per_page; i++ ) {
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, point( 1, y ), 28, selection == i ? hilite( col ) : col,
                                        string_format( "%s  %s", colorize( creatures[i]->sym,
                                                       selection == i ? hilite( creatures[i]->color ) : creatures[i]->color ),
                                                       creatures[i]->nname() ) );
                    }
                    if( !cur_creature.is_null() ) {
                        cur_creature->faction_display( w_missions, point( 31, 3 ), w );
                    } else {
                        fold_and_print( w_missions, point( 31, 4 ), w, c_light_red, no_creatures );
                    }
                    break;
                } else {
                    fold_and_print( w_missions, point( 31, 4 ), w, c_light_red, no_creatures );
                }
            }
            default:
                break;
        }
        wnoutrefresh( w_missions );
    } );

    avatar &player_character = get_avatar();
    while( true ) {
        // create a list of NPCs, visible and the ones on overmapbuffer
        followers.clear();
        for( const character_id &elem : g->get_follower_list() ) {
            shared_ptr_fast<npc> npc_to_get = overmap_buffer.find_npc( elem );
            if( !npc_to_get ) {
                continue;
            }
            npc *npc_to_add = npc_to_get.get();
            followers.push_back( npc_to_add );
        }
        valfac.clear();
        for( const auto &elem : g->faction_manager_ptr->all() ) {
            if( elem.second.known_by_u && elem.second.id != faction_your_followers ) {
                valfac.push_back( &elem.second );
            }
        }
        guy = nullptr;
        cur_fac = nullptr;
        snippet = nullptr;
        cur_creature = mtype_id::NULL_ID();
        interactable = false;
        radio_interactable = false;
        camp = nullptr;
        // create a list of faction camps
        camps.clear();
        for( tripoint_abs_omt elem : player_character.camps ) {
            std::optional<basecamp *> p = overmap_buffer.find_camp( elem.xy() );
            if( !p ) {
                continue;
            }
            basecamp *temp_camp = *p;
            camps.push_back( temp_camp );
        }
        lore.clear();
        for( const auto &elem : player_character.get_snippets() ) {
            std::optional<translation> name = SNIPPET.get_name_by_id( elem );
            if( name.has_value() ) {
                if( !name->empty() ) {
                    lore.emplace_back( elem, name->translated() );
                } else {
                    lore.emplace_back( elem, elem.str() );
                }
            }
        }
        auto compare_second =
            []( const std::pair<snippet_id, std::string> &a,
        const std::pair<snippet_id, std::string> &b ) {
            return localized_compare( a.second, b.second );
        };
        std::sort( lore.begin(), lore.end(), compare_second );
        if( tab < tab_mode::FIRST_TAB || tab >= tab_mode::NUM_TABS ) {
            debugmsg( "The sanity check failed because tab=%d", static_cast<int>( tab ) );
            tab = tab_mode::FIRST_TAB;
        }
        creatures.clear();
        creatures.reserve( player_character.get_known_monsters().size() );
        creatures.insert( creatures.end(), player_character.get_known_monsters().begin(),
                          player_character.get_known_monsters().end() );
        std::sort( creatures.begin(), creatures.end(), []( const mtype_id & a, const mtype_id & b ) {
            return localized_compare( a->nname(), b->nname() );
        } );
        active_vec_size = camps.size();
        if( tab == tab_mode::TAB_FOLLOWERS ) {
            if( selection < followers.size() ) {
                guy = followers[selection];
            }
            active_vec_size = followers.size();
        } else if( tab == tab_mode::TAB_MYFACTION ) {
            if( selection < camps.size() ) {
                camp = camps[selection];
            }
            active_vec_size = camps.size();
        } else if( tab == tab_mode::TAB_OTHERFACTIONS ) {
            if( selection < valfac.size() ) {
                cur_fac = valfac[selection];
            }
            active_vec_size = valfac.size();
        } else if( tab == tab_mode::TAB_LORE ) {
            if( selection < lore.size() ) {
                snippet = &lore[selection];
            }
            active_vec_size = lore.size();
        } else if( tab == tab_mode::TAB_CREATURES ) {
            if( selection < creatures.size() ) {
                cur_creature = creatures[selection];
            }
            active_vec_size = creatures.size();
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "LEFT" || action == "PREV_TAB" || action == "RIGHT" || action == "NEXT_TAB" ) {
            // necessary to use inc_clamp_wrap
            static_assert( static_cast<int>( tab_mode::FIRST_TAB ) == 0 );
            tab = inc_clamp_wrap( tab, action == "RIGHT" || action == "NEXT_TAB", tab_mode::NUM_TABS );
            selection = 0;
        } else if( navigate_ui_list( action, selection, 10, active_vec_size, true ) ) {
        } else if( action == "CONFIRM" ) {
            if( tab == tab_mode::TAB_FOLLOWERS && guy ) {
                if( guy->has_companion_mission() ) {

                    talk_function::basecamp_mission( *guy );

                } else if( interactable || radio_interactable ) {
                    player_character.talk_to( get_talker_for( *guy ), radio_interactable );
                }
            } else if( tab == tab_mode::TAB_MYFACTION && camp ) {
                camp->query_new_name();
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }
}
