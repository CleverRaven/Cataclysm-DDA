#include "faction.h"

#include <algorithm>
#include <bitset>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "calendar.h"
#include "character.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "pimpl.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "vitamin.h"

static const faction_id faction_no_faction( "no_faction" );

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
    _food_supply.clear();
    wealth = 0;
    size = 0;
    power = 0;
    lone_wolf_faction = false;
    limited_area_claim = false;
    currency = itype_id::NULL_ID();
    steal_persist = std::nullopt; // Default ask
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
        for( const faction_epilogue_data &epi : fac.epilogue_data ) {
            if( !epi.epilogue.is_valid() ) {
                debugmsg( "There's no snippet with id %s", epi.epilogue.str() );
            }
        }
    }
}

void faction_template::reset()
{
    npc_factions::all_templates.clear();
}

std::string faction_template::get_name() const
{
    return _( name );
}

void faction_template::set_name( std::string new_name )
{
    name = std::move( new_name );
}

void faction_template::load_relations( const JsonObject &jsobj )
{
    for( const JsonMember fac : jsobj.get_object( "relations" ) ) {
        JsonObject rel_jo = fac.get_object();
        std::bitset<static_cast<size_t>( npc_factions::relationship::rel_types )> fac_relation( 0 );
        for( const auto &rel_flag : npc_factions::relation_strs ) {
            fac_relation.set( static_cast<size_t>( rel_flag.second ),
                              rel_jo.get_bool( rel_flag.first, false ) );
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
    , wealth( jsobj.get_int( "wealth" ) )
{
    jsobj.get_member( "description" ).read( desc );
    optional( jsobj, false, "consumes_food", consumes_food, false );
    optional( jsobj, false, "price_rules", price_rules, faction_price_rules_reader {} );
    jsobj.read( "fac_food_supply", _food_supply, true );
    if( jsobj.has_string( "currency" ) ) {
        jsobj.read( "currency", currency, true );
        price_rules.emplace_back( currency, 1, 0 );
    } else {
        currency = itype_id::NULL_ID();
    }
    lone_wolf_faction = jsobj.get_bool( "lone_wolf_faction", false );
    limited_area_claim = jsobj.get_bool( "limited_area_claim", false );
    load_relations( jsobj );
    mon_faction = mfaction_str_id( jsobj.get_string( "mon_faction", "human" ) );
    optional( jsobj, false, "epilogues", epilogue_data );
}

std::string faction::describe() const
{
    std::string ret = desc.translated();
    return ret;
}

void faction_power_spec::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "faction", faction );
    optional( jo, false, "power_min", power_min );
    optional( jo, false, "power_max", power_max );

    if( !power_min.has_value() && !power_max.has_value() ) {
        jo.throw_error( "Must have either a power_min or a power_max" );
    }
}

void faction_epilogue_data::deserialize( const JsonObject &jo )
{
    optional( jo, false, "power_min", power_min );
    optional( jo, false, "power_max", power_max );
    optional( jo, false, "dynamic", dynamic_conditions );
    mandatory( jo, false, "id", epilogue );
}


bool faction::check_relations( const std::vector<faction_power_spec> &faction_power_specs ) const
{
    if( faction_power_specs.empty() ) {
        return true;
    }
    for( const faction_power_spec &spec : faction_power_specs ) {
        if( spec.power_min.has_value() ) {
            if( spec.faction->power < spec.power_min.value() ) {
                return false;
            }
        }
        if( spec.power_max.has_value() ) {
            if( spec.faction->power >= spec.power_max.value() ) {
                return false;
            }
        }
    }
    return true;
}


std::vector<std::string> faction::epilogue() const
{
    std::vector<std::string> ret;
    for( const faction_epilogue_data &epi : epilogue_data ) {
        if( ( !epi.power_min.has_value() || power >= epi.power_min ) && ( !epi.power_max.has_value() ||
                power < epi.power_max ) ) {
            if( check_relations( epi.dynamic_conditions ) ) {
                ret.emplace_back( epi.epilogue->translated() );
            }
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

std::string faction::food_supply_text() const
{
    //Convert to how many days you can support the population
    int val = food_supply().kcal() / ( size * 288 );
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

nc_color faction::food_supply_color() const
{
    int val = food_supply().kcal() / ( size * 288 );
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
    const double days_of_food = food_supply().kcal() / 3000.0;
    std::map<vitamin_id, int> stored_vits = food_supply().vitamins();
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
            return rel_data.second.test( static_cast<size_t>( flag ) );
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
            fac.set_name( name_new );
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
                        elem.second.limited_area_claim = fac_temp.limited_area_claim;
                        elem.second.set_name( fac_temp.get_name() );
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
