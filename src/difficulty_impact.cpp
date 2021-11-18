#include <map>
#include <string>

#include "difficulty_impact.h"
#include "generic_factory.h"

const std::string difficulty_impact::get_diff_desc( const difficulty_option &diff ) const
{
    static const std::map<difficulty_option, std::string> diff_map {
        //~ Describes game difficulty
        { DIFF_VERY_EASY, _( "Very Easy" ) },
        //~ Describes game difficulty
        { DIFF_EASY, _( "Easy" ) },
        //~ Describes game difficulty
        { DIFF_MEDIUM, _( "Medium" ) },
        //~ Describes game difficulty
        { DIFF_HARD, _( "Hard" ) },
        //~ Describes game difficulty
        { DIFF_VERY_HARD, _( "Very Hard" ) }
    };
    auto d = diff_map.find( diff );
    return d == diff_map.end() ? _( "None" ) : d->second;
}

difficulty_impact::difficulty_option difficulty_impact::get_opt_from_str( const std::string &diff_str ) const
{
    static const std::map<std::string, difficulty_option> diff_map {
        { "very_easy", DIFF_VERY_EASY },
        { "easy", DIFF_EASY },
        { "medium", DIFF_MEDIUM },
        { "hard", DIFF_HARD },
        { "very_hard", DIFF_VERY_HARD }
    };
    auto d = diff_map.find( diff_str );
    return d == diff_map.end() ? DIFF_NONE : d->second;
}

void difficulty_impact::load( const JsonObject &jo )
{
    std::string readr_intl;
    std::string readr_us;
    optional( jo, false, "offence", readr_intl, std::string() );
    optional( jo, false, "offense", readr_us, readr_intl );
    offence = get_opt_from_str( readr_us );
    optional( jo, false, "defence", readr_intl, std::string() );
    optional( jo, false, "defense", readr_us, readr_intl );
    defence = get_opt_from_str( readr_us );
    optional( jo, false, "crafting", readr_intl, std::string() );
    crafting = get_opt_from_str( readr_intl );
    optional( jo, false, "wilderness", readr_intl, std::string() );
    wilderness = get_opt_from_str( readr_intl );
    optional( jo, false, "social", readr_intl, std::string() );
    social = get_opt_from_str( readr_intl );
}