#include "faction_camp.h"

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "debug.h"
#include "display.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "skill.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"

static std::map<risk_diff_level, translation> risk_diff_to_risk_name( {
    { risk_diff_level::NONE,                 to_translation( "risk level", "Risk: None" ) },
    { risk_diff_level::VERY_LOW,             to_translation( "risk level", "Risk: Very Low" ) },
    { risk_diff_level::LOW,                  to_translation( "risk level", "Risk: Low" ) },
    { risk_diff_level::MEDIUM,               to_translation( "risk level", "Risk: Medium" ) },
    { risk_diff_level::HIGH,                 to_translation( "risk level", "Risk: High" ) },
    { risk_diff_level::VERY_HIGH,            to_translation( "risk level", "Risk: Very High " ) }
} );

static std::map<risk_diff_level, translation> risk_diff_to_diff_name( {
    { risk_diff_level::NONE,                 to_translation( "difficulty level", "Difficulty: None" ) },
    { risk_diff_level::VERY_LOW,             to_translation( "difficulty level", "Difficulty: Very Easy" ) },
    { risk_diff_level::LOW,                  to_translation( "difficulty level", "Difficulty: Easy" ) },
    { risk_diff_level::MEDIUM,               to_translation( "difficulty level", "Difficulty: Moderate" ) },
    { risk_diff_level::HIGH,                 to_translation( "difficulty level", "Difficulty: Hard" ) },
    { risk_diff_level::VERY_HIGH,            to_translation( "difficulty level", "Difficulty: Very Hard" ) }
} );

class JsonObject;

void faction_mission::load( const JsonObject &jo, const std::string_view & )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "desc", description );
    optional( jo, was_loaded, "skill", skill_used, skill_id::NULL_ID() );
    optional( jo, was_loaded, "difficulty", difficulty, risk_diff_level::NUM_RISK_DIFF_LEVELS );
    optional( jo, was_loaded, "risk", risk, risk_diff_level::NUM_RISK_DIFF_LEVELS );
    std::string activity_level_str;
    optional( jo, was_loaded, "activity", activity_level_str );
    if( !activity_level_str.empty() ) {
        const auto activity_level_it = activity_levels_map.find( activity_level_str );
        if( activity_level_it == activity_levels_map.end() ) {
            debugmsg( "Invalid faction mission activity level: %s", activity_level_str );
        } else {
            activity_level = activity_level_it->second;
        }
    }
    optional( jo, was_loaded, "time", time );
    optional( jo, was_loaded, "positions", positions );
    optional( jo, was_loaded, "items_label", items_label );
    optional( jo, was_loaded, "items_possibilities", items_possibilities );
    optional( jo, was_loaded, "effects", effects );
    optional( jo, was_loaded, "footer", footer );
}

namespace
{

generic_factory<faction_mission> faction_mission_factory( "faction_mission" );

} // namespace

/** @relates string_id */
template<>
const faction_mission &string_id<faction_mission>::obj() const
{
    return faction_mission_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<faction_mission>::is_valid() const
{
    return faction_mission_factory.is_valid( *this );
}

namespace io
{

template<>
std::string enum_to_string<risk_diff_level>( risk_diff_level data )
{
    switch( data ) {
        // *INDENT-OFF*
        case risk_diff_level::NONE: return "NONE";
        case risk_diff_level::VERY_LOW: return "VERY_LOW";
        case risk_diff_level::LOW: return "LOW";
        case risk_diff_level::MEDIUM: return "MEDIUM";
        case risk_diff_level::HIGH: return "HIGH";
        case risk_diff_level::VERY_HIGH: return "VERY_HIGH";
        // *INDENT-ON*
        case risk_diff_level::NUM_RISK_DIFF_LEVELS:
            break;
    }
    cata_fatal( "Invalid risk_diff_level" );
}

} // namespace io


void faction_mission::load_faction_missions( const JsonObject &jo, const std::string &src )
{
    faction_mission_factory.load( jo, src );
}

void faction_mission::reset()
{
    faction_mission_factory.reset();
}

///Format the string as a list item, e.g. "> foo"
static std::string desc_list_item( const std::string &item )
{
    return string_format( _( "> %s" ), item );
}

std::string faction_mission::describe( int npc_count,
                                       const std::function<std::vector<std::string>()> &get_items_possibilities,
                                       const std::function<std::vector<std::string>()> &get_description ) const
{
    std::vector<std::string> lines;
    lines.emplace_back( name.translated() );
    lines.emplace_back( description.translated() );
    if( get_description ) {
        for( const std::string &line : get_description() ) {
            lines.emplace_back( line );
        }
    }
    lines.emplace_back( "" );
    if( !skill_used.is_null() ) {
        lines.emplace_back( string_format( _( "Skill used: %s" ), skill_used->name() ) );
    }
    if( difficulty != risk_diff_level::NUM_RISK_DIFF_LEVELS ) {
        lines.emplace_back( risk_diff_to_diff_name[difficulty].translated() );
    }
    if( risk != risk_diff_level::NUM_RISK_DIFF_LEVELS ) {
        lines.emplace_back( risk_diff_to_risk_name[risk].translated() );
    }
    if( activity_level > 0 ) {
        lines.emplace_back( string_format( _( "Activity level: %s" ),
                                           display::activity_level_str( activity_level ) ) );
    }
    if( !time.empty() ) {
        lines.emplace_back( string_format( _( "Time: %s" ), time.translated() ) );
    }
    if( !items_label.empty() ) {
        lines.emplace_back( string_format( _( "%s possibilities:" ), items_label.translated() ) );
        if( get_items_possibilities ) {
            for( const std::string &poss : get_items_possibilities() ) {
                lines.emplace_back( desc_list_item( poss ) );
            }
        } else {
            for( const translation &poss : items_possibilities ) {
                lines.emplace_back( desc_list_item( poss.translated() ) );
            }
        }
    }
    if( !effects.empty() ) {
        lines.emplace_back( _( "Effects:" ) );
        for( const translation &eff : effects ) {
            lines.emplace_back( desc_list_item( eff.translated() ) );
        }
    }
    if( !footer.empty() ) {
        lines.emplace_back( footer.translated() );
    }
    if( positions != 0 ) {
        lines.emplace_back( string_format( _( "Positions: %d/%d" ), npc_count, positions ) );
    }
    return string_join( lines, "\n" );
}

void faction_mission::check_consistency()
{
    faction_mission_factory.check();
}

void faction_mission::check() const
{
    if( !skill_used.is_valid() && !skill_used.is_null() ) {
        debugmsg( "Invalid skill in faction mission: %s, %s", name, skill_used.str() );
    }
}
