#include "faction.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>

#include "basecamp.h"
#include "catacharset.h"
#include "coordinate_conversions.h"
#include "cursesdef.h"
#include "debug.h"
#include "enums.h"
#include "faction_camp.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "mission.h"
#include "npc.h"
#include "npctalk.h"
#include "omdata.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player.h"
#include "rng.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"

static std::map<faction_id, faction_template> _all_faction_templates;

std::string invent_name();
std::string invent_adj();

constexpr inline unsigned long mfb( const int v )
{
    return 1ul << v;
}

faction_template::faction_template()
{
    likes_u = 0;
    respects_u = 0;
    known_by_u = true;
    strength = 0;
    combat_ability = 0;
    food_supply = 0;
    wealth = 0;
    sneak = 0;
    crime = 0;
    cult = 0;
    good = 0;
    size = 0;
    power = 0;
}

faction::faction( const faction_template &templ )
{
    values = 0;
    goal = FACGOAL_NULL;
    job1 = FACJOB_NULL;
    job2 = FACJOB_NULL;
    mapx = 0;
    mapy = 0;
    id = templ.id;
    randomize();
    // first init *all* members, than copy those from the template
    static_cast<faction_template &>( *this ) = templ;
}

void faction_template::load( JsonObject &jsobj )
{
    faction_template fac( jsobj );
    _all_faction_templates.emplace( fac.id, fac );
}

void faction_template::reset()
{
    _all_faction_templates.clear();
}

faction_template::faction_template( JsonObject &jsobj )
    : name( jsobj.get_string( "name" ) )
    , likes_u( jsobj.get_int( "likes_u" ) )
    , respects_u( jsobj.get_int( "respects_u" ) )
    , known_by_u( jsobj.get_bool( "known_by_u" ) )
    , id( faction_id( jsobj.get_string( "id" ) ) )
    , desc( jsobj.get_string( "description" ) )
    , strength( jsobj.get_int( "strength" ) )
    , sneak( jsobj.get_int( "sneak" ) )
    , crime( jsobj.get_int( "crime" ) )
    , cult( jsobj.get_int( "cult" ) )
    , good( jsobj.get_int( "good" ) )
    , size( jsobj.get_int( "size" ) )
    , power( jsobj.get_int( "power" ) )
    , combat_ability( jsobj.get_int( "combat_ability" ) )
    , food_supply( jsobj.get_int( "food_supply" ) )
    , wealth( jsobj.get_int( "wealth" ) )
{
}

// TODO: move them to json

static const std::array<std::string, 15> faction_adj_pos = { {
        translate_marker_context( "faction_adj", "Shining" ), translate_marker_context( "faction_adj", "Sacred" ), translate_marker_context( "faction_adj", "Golden" ), translate_marker_context( "faction_adj", "Holy" ), translate_marker_context( "faction_adj", "Righteous" ), translate_marker_context( "faction_adj", "Devoted" ),
        translate_marker_context( "faction_adj", "Virtuous" ), translate_marker_context( "faction_adj", "Splendid" ), translate_marker_context( "faction_adj", "Divine" ), translate_marker_context( "faction_adj", "Radiant" ), translate_marker_context( "faction_adj", "Noble" ), translate_marker_context( "faction_adj", "Venerable" ),
        translate_marker_context( "faction_adj", "Immaculate" ), translate_marker_context( "faction_adj", "Heroic" ), translate_marker_context( "faction_adj", "Bright" )
    }
};

static const std::array<std::string, 15> faction_adj_neu = {{
        translate_marker_context( "faction_adj", "Original" ), translate_marker_context( "faction_adj", "Crystal" ), translate_marker_context( "faction_adj", "Metal" ), translate_marker_context( "faction_adj", "Mighty" ), translate_marker_context( "faction_adj", "Powerful" ), translate_marker_context( "faction_adj", "Solid" ),
        translate_marker_context( "faction_adj", "Stone" ), translate_marker_context( "faction_adj", "Firey" ), translate_marker_context( "faction_adj", "Colossal" ), translate_marker_context( "faction_adj", "Famous" ), translate_marker_context( "faction_adj", "Supreme" ), translate_marker_context( "faction_adj", "Invincible" ),
        translate_marker_context( "faction_adj", "Unlimited" ), translate_marker_context( "faction_adj", "Great" ), translate_marker_context( "faction_adj", "Electric" )
    }
};

static const std::array<std::string, 15> faction_adj_bad = {{
        translate_marker_context( "faction_adj", "Poisonous" ), translate_marker_context( "faction_adj", "Deadly" ), translate_marker_context( "faction_adj", "Foul" ), translate_marker_context( "faction_adj", "Nefarious" ), translate_marker_context( "faction_adj", "Wicked" ), translate_marker_context( "faction_adj", "Vile" ),
        translate_marker_context( "faction_adj", "Ruinous" ), translate_marker_context( "faction_adj", "Horror" ), translate_marker_context( "faction_adj", "Devastating" ), translate_marker_context( "faction_adj", "Vicious" ), translate_marker_context( "faction_adj", "Sinister" ), translate_marker_context( "faction_adj", "Baleful" ),
        translate_marker_context( "faction_adj", "Pestilent" ), translate_marker_context( "faction_adj", "Pernicious" ), translate_marker_context( "faction_adj", "Dread" )
    }
};

static const std::array<std::string, 15> faction_noun_strong = {{
        translate_marker_context( "faction_adj", "Fists" ), translate_marker_context( "faction_adj", "Slayers" ), translate_marker_context( "faction_adj", "Furies" ), translate_marker_context( "faction_adj", "Dervishes" ), translate_marker_context( "faction_adj", "Tigers" ), translate_marker_context( "faction_adj", "Destroyers" ),
        translate_marker_context( "faction_adj", "Berserkers" ), translate_marker_context( "faction_adj", "Samurai" ), translate_marker_context( "faction_adj", "Valkyries" ), translate_marker_context( "faction_adj", "Army" ), translate_marker_context( "faction_adj", "Killers" ), translate_marker_context( "faction_adj", "Paladins" ),
        translate_marker_context( "faction_adj", "Knights" ), translate_marker_context( "faction_adj", "Warriors" ), translate_marker_context( "faction_adj", "Huntsmen" )
    }
};

static const std::array<std::string, 15> faction_noun_sneak = {{
        translate_marker_context( "faction_adj", "Snakes" ), translate_marker_context( "faction_adj", "Rats" ), translate_marker_context( "faction_adj", "Assassins" ), translate_marker_context( "faction_adj", "Ninja" ), translate_marker_context( "faction_adj", "Agents" ), translate_marker_context( "faction_adj", "Shadows" ),
        translate_marker_context( "faction_adj", "Guerillas" ), translate_marker_context( "faction_adj", "Eliminators" ), translate_marker_context( "faction_adj", "Snipers" ), translate_marker_context( "faction_adj", "Smoke" ), translate_marker_context( "faction_adj", "Arachnids" ), translate_marker_context( "faction_adj", "Creepers" ),
        translate_marker_context( "faction_adj", "Shade" ), translate_marker_context( "faction_adj", "Stalkers" ), translate_marker_context( "faction_adj", "Eels" )
    }
};

static const std::array<std::string, 15> faction_noun_crime = {{
        translate_marker_context( "faction_adj", "Bandits" ), translate_marker_context( "faction_adj", "Punks" ), translate_marker_context( "faction_adj", "Family" ), translate_marker_context( "faction_adj", "Mafia" ), translate_marker_context( "faction_adj", "Mob" ), translate_marker_context( "faction_adj", "Gang" ), translate_marker_context( "faction_adj", "Vandals" ),
        translate_marker_context( "faction_adj", "Sharks" ), translate_marker_context( "faction_adj", "Muggers" ), translate_marker_context( "faction_adj", "Cutthroats" ), translate_marker_context( "faction_adj", "Guild" ), translate_marker_context( "faction_adj", "Faction" ), translate_marker_context( "faction_adj", "Thugs" ),
        translate_marker_context( "faction_adj", "Racket" ), translate_marker_context( "faction_adj", "Crooks" )
    }
};

static const std::array<std::string, 15> faction_noun_cult = {{
        translate_marker_context( "faction_adj", "Brotherhood" ), translate_marker_context( "faction_adj", "Church" ), translate_marker_context( "faction_adj", "Ones" ), translate_marker_context( "faction_adj", "Crucible" ), translate_marker_context( "faction_adj", "Sect" ), translate_marker_context( "faction_adj", "Creed" ),
        translate_marker_context( "faction_adj", "Doctrine" ), translate_marker_context( "faction_adj", "Priests" ), translate_marker_context( "faction_adj", "Tenet" ), translate_marker_context( "faction_adj", "Monks" ), translate_marker_context( "faction_adj", "Clerics" ), translate_marker_context( "faction_adj", "Pastors" ),
        translate_marker_context( "faction_adj", "Gnostics" ), translate_marker_context( "faction_adj", "Elders" ), translate_marker_context( "faction_adj", "Inquisitors" )
    }
};

static const std::array<std::string, 15> faction_noun_none = {{
        translate_marker_context( "faction_adj", "Settlers" ), translate_marker_context( "faction_adj", "People" ), translate_marker_context( "faction_adj", "Men" ), translate_marker_context( "faction_adj", "Faction" ), translate_marker_context( "faction_adj", "Tribe" ), translate_marker_context( "faction_adj", "Clan" ), translate_marker_context( "faction_adj", "Society" ),
        translate_marker_context( "faction_adj", "Folk" ), translate_marker_context( "faction_adj", "Nation" ), translate_marker_context( "faction_adj", "Republic" ), translate_marker_context( "faction_adj", "Colony" ), translate_marker_context( "faction_adj", "State" ), translate_marker_context( "faction_adj", "Kingdom" ), translate_marker_context( "faction_adj", "Party" ),
        translate_marker_context( "faction_adj", "Company" )
    }
};

static const std::array<faction_value_datum, NUM_FACGOALS> facgoal_data = {{
        // "Their ultimate goal is <name>"
        //Name                               Good Str Sneak Crime Cult
        {"Null",                             0,   0,  0,    0,    0},
        {translate_marker_context( "faction_goal", "basic survival" ),                0,   0,  0,    0,    0},
        {translate_marker_context( "faction_goal", "financial wealth" ),              0,  -1,  0,    2,   -1},
        {translate_marker_context( "faction_goal", "dominance of the region" ),      -1,   1, -1,    1,   -1},
        {translate_marker_context( "faction_goal", "the extermination of monsters" ), 1,   3, -1,   -1,   -1},
        {translate_marker_context( "faction_goal", "contact with unseen powers" ),   -1,   0,  1,    0,    4},
        {translate_marker_context( "faction_goal", "bringing the apocalypse" ),      -5,   1,  2,    0,    7},
        {translate_marker_context( "faction_goal", "general chaos and anarchy" ),    -3,   2, -3,    2,   -1},
        {translate_marker_context( "faction_goal", "the cultivation of knowledge" ),  2,  -3,  2,   -1,    0},
        {translate_marker_context( "faction_goal", "harmony with nature" ),           2,  -2,  0,   -1,    2},
        {translate_marker_context( "faction_goal", "rebuilding civilization" ),       2,   1, -2,   -2,   -4},
        {translate_marker_context( "faction_goal", "spreading the fungus" ),         -2,   1,  1,    0,    4}
    }
};
// TOTAL:                               -5    3  -2     0     7

static const std::array<faction_value_datum, NUM_FACJOBS> facjob_data = {{
        // "They earn money via <name>"
        //Name                              Good Str Sneak Crime Cult
        {"Null",                            0,   0,  0,    0,    0},
        {translate_marker_context( "faction_job", "protection rackets" ),          -3,   2, -1,    4,    0},
        {translate_marker_context( "faction_job", "the sale of information" ),     -1,  -1,  4,    1,    0},
        {translate_marker_context( "faction_job", "their bustling trade centers" ), 1,  -1, -2,   -4,   -4},
        {translate_marker_context( "faction_job", "trade caravans" ),               2,  -1, -1,   -3,   -2},
        {translate_marker_context( "faction_job", "scavenging supplies" ),          0,  -1,  0,   -1,   -1},
        {translate_marker_context( "faction_job", "mercenary work" ),               0,   3, -1,    1,   -1},
        {translate_marker_context( "faction_job", "assassinations" ),              -1,   2,  2,    1,    1},
        {translate_marker_context( "faction_job", "raiding settlements" ),         -4,   4, -3,    3,   -2},
        {translate_marker_context( "faction_job", "the theft of property" ),       -3,  -1,  4,    4,    1},
        {translate_marker_context( "faction_job", "gambling parlors" ),            -1,  -2, -1,    1,   -1},
        {translate_marker_context( "faction_job", "medical aid" ),                  4,  -3, -2,   -3,    0},
        {translate_marker_context( "faction_job", "farming & selling food" ),       3,  -4, -2,   -4,    1},
        {translate_marker_context( "faction_job", "drug dealing" ),                -2,   0, -1,    2,    0},
        {translate_marker_context( "faction_job", "selling manufactured goods" ),   1,   0, -1,   -2,    0}
    }
};
// TOTAL:                              -5   -3  -5     0    -6

static const std::array<faction_value_datum, NUM_FACVALS> facval_data = {{
        // "They are known for <name>"
        //Name                            Good Str Sneak Crime Cult
        {"Null",                          0,   0,  0,    0,    0},
        {translate_marker_context( "faction_value", "their charitable nature" ),    5,  -1, -1,   -2,   -2},
        {translate_marker_context( "faction_value", "their isolationism" ),         0,  -2,  1,    0,    2},
        {translate_marker_context( "faction_value", "exploring extensively" ),      1,   0,  0,   -1,   -1},
        {translate_marker_context( "faction_value", "collecting rare artifacts" ),  0,   1,  1,    0,    3},
        {translate_marker_context( "faction_value", "their knowledge of bionics" ), 1,   2,  0,    0,    0},
        {translate_marker_context( "faction_value", "their libraries" ),            1,  -3,  0,   -2,    1},
        {translate_marker_context( "faction_value", "their elite training" ),       0,   4,  2,    0,    2},
        {translate_marker_context( "faction_value", "their robotics factories" ),   0,   3, -1,    0,   -2},
        {translate_marker_context( "faction_value", "treachery" ),                 -3,   0,  1,    3,    0},
        {translate_marker_context( "faction_value", "the avoidance of drugs" ),     1,   0,  0,   -1,    1},
        {translate_marker_context( "faction_value", "their adherence to the law" ), 2,  -1, -1,   -4,   -1},
        {translate_marker_context( "faction_value", "their cruelty" ),             -3,   1, -1,    4,    1}
    }
};
// TOTALS:                            5    4   1    -3     4
/* Note: It's nice to keep the totals around 0 for Good, and about even for the
 * other four.  It's okay if Good is slightly negative (after all, in a post-
 * apocalyptic world people might be a LITTLE less virtuous), and to keep
 * strength valued a bit higher than the others.
 */

void faction::randomize()
{
    // Set up values
    // TODO: Not always in overmap 0,0
    mapx = rng( OMAPX / 10, OMAPX - OMAPX / 10 );
    mapy = rng( OMAPY / 10, OMAPY - OMAPY / 10 );
    // Pick an overall goal.
    goal = faction_goal( rng( 1, NUM_FACGOALS - 1 ) );
    if( one_in( 4 ) ) {
        goal = FACGOAL_NONE;    // Slightly more likely to not have a real goal
    }
    good     = facgoal_data[goal].good;
    strength = facgoal_data[goal].strength;
    sneak    = facgoal_data[goal].sneak;
    crime    = facgoal_data[goal].crime;
    cult     = facgoal_data[goal].cult;
    job1 = faction_job( rng( 1, NUM_FACJOBS - 1 ) );
    do {
        job2 = faction_job( rng( 0, NUM_FACJOBS - 1 ) );
    } while( job2 == job1 );
    good     += facjob_data[job1].good     + facjob_data[job2].good;
    strength += facjob_data[job1].strength + facjob_data[job2].strength;
    sneak    += facjob_data[job1].sneak    + facjob_data[job2].sneak;
    crime    += facjob_data[job1].crime    + facjob_data[job2].crime;
    cult     += facjob_data[job1].cult     + facjob_data[job2].cult;

    int num_values = 0;
    int tries = 0;
    values = 0;
    do {
        int v = rng( 1, NUM_FACVALS - 1 );
        if( !has_value( faction_value( v ) ) && matches_us( faction_value( v ) ) ) {
            values |= mfb( v );
            tries = 0;
            num_values++;
            good     += facval_data[v].good;
            strength += facval_data[v].strength;
            sneak    += facval_data[v].sneak;
            crime    += facval_data[v].crime;
            cult     += facval_data[v].cult;
        } else {
            tries++;
        }
    } while( ( one_in( num_values ) || one_in( num_values ) ) && tries < 15 );

    std::string noun;
    int sel = 1;
    int best = strength;
    if( sneak > best ) {
        sel = 2;
        best = sneak;
    }
    if( crime > best ) {
        sel = 3;
        best = crime;
    }
    if( cult > best ) {
        sel = 4;
    }
    if( strength <= 0 && sneak <= 0 && crime <= 0 && cult <= 0 ) {
        sel = 0;
    }

    switch( sel ) {
        case 1:
            noun  = pgettext( "faction_adj", random_entry_ref( faction_noun_strong ).c_str() );
            power = dice( 5, 20 );
            size  = dice( 5, 6 );
            break;
        case 2:
            noun  = pgettext( "faction_adj", random_entry_ref( faction_noun_sneak ).c_str() );
            power = dice( 5, 8 );
            size  = dice( 5, 8 );
            break;
        case 3:
            noun  = pgettext( "faction_adj", random_entry_ref( faction_noun_crime ).c_str() );
            power = dice( 5, 16 );
            size  = dice( 5, 8 );
            break;
        case 4:
            noun  = pgettext( "faction_adj", random_entry_ref( faction_noun_cult ).c_str() );
            power = dice( 8, 8 );
            size  = dice( 4, 6 );
            break;
        default:
            noun  = pgettext( "faction_adj", random_entry_ref( faction_noun_none ).c_str() );
            power = dice( 6, 8 );
            size  = dice( 6, 6 );
    }

    if( one_in( 4 ) ) {
        do {
            name = string_format( _( "The %1$s of %2$s" ), noun.c_str(), invent_name().c_str() );
        } while( utf8_width( name ) > MAX_FAC_NAME_SIZE );
    } else if( one_in( 2 ) ) {
        do {
            name = string_format( _( "The %1$s %2$s" ), invent_adj().c_str(), noun.c_str() );
        } while( utf8_width( name ) > MAX_FAC_NAME_SIZE );
    } else {
        do {
            std::string adj;
            if( good >= 3 ) {
                adj = _( random_entry_ref( faction_adj_pos ).c_str() );
            } else if( good <= -3 ) {
                adj = _( random_entry_ref( faction_adj_bad ).c_str() );
            } else {
                adj = _( random_entry_ref( faction_adj_neu ).c_str() );
            }
            name = string_format( _( "The %1$s %2$s" ), adj.c_str(), noun.c_str() );
            if( one_in( 4 ) ) {
                name = string_format( _( "%1$s of %2$s" ), name.c_str(), invent_name().c_str() );
            }
        } while( utf8_width( name ) > MAX_FAC_NAME_SIZE );
    }
}

bool faction::has_job( faction_job j ) const
{
    return ( job1 == j || job2 == j );
}

bool faction::has_value( faction_value v ) const
{
    return values & mfb( v );
}

bool faction::matches_us( faction_value v ) const
{
    int numvals = 2;
    if( job2 != FACJOB_NULL ) {
        numvals++;
    }
    for( int i = 0; i < NUM_FACVALS; i++ ) {
        if( has_value( faction_value( i ) ) ) {
            numvals++;
        }
    }
    if( has_job( FACJOB_DRUGS ) && v == FACVAL_STRAIGHTEDGE ) { // Mutually exclusive
        return false;
    }
    int avggood = ( good / numvals + good ) / 2;
    int avgstrength = ( strength / numvals + strength ) / 2;
    int avgsneak = ( sneak / numvals + sneak / 2 );
    int avgcrime = ( crime / numvals + crime / 2 );
    int avgcult = ( cult / numvals + cult / 2 );
    /*
     debugmsg("AVG: GOO %d STR %d SNK %d CRM %d CLT %d\n\
           VAL: GOO %d STR %d SNK %d CRM %d CLT %d (%s)", avggood, avgstrength,
    avgsneak, avgcrime, avgcult, facval_data[v].good, facval_data[v].strength,
    facval_data[v].sneak, facval_data[v].crime, facval_data[v].cult,
    facval_data[v].name.c_str());
    */
    if( ( abs( facval_data[v].good     - avggood )  <= 3 ||
          ( avggood >=  5 && facval_data[v].good >=  1 ) ||
          ( avggood <= -5 && facval_data[v].good <=  0 ) )  &&
        ( abs( facval_data[v].strength - avgstrength )   <= 5 ||
          ( avgstrength >=  5 && facval_data[v].strength >=  3 ) ||
          ( avgstrength <= -5 && facval_data[v].strength <= -1 ) )  &&
        ( abs( facval_data[v].sneak    - avgsneak ) <= 4 ||
          ( avgsneak >=  5 && facval_data[v].sneak >=  1 ) ||
          ( avgsneak <= -5 && facval_data[v].sneak <= -1 ) )  &&
        ( abs( facval_data[v].crime    - avgcrime ) <= 4 ||
          ( avgcrime >=  5 && facval_data[v].crime >=  0 ) ||
          ( avgcrime <= -5 && facval_data[v].crime <= -1 ) )  &&
        ( abs( facval_data[v].cult     - avgcult )  <= 3 ||
          ( avgcult >=  5 && facval_data[v].cult >=  1 ) ||
          ( avgcult <= -5 && facval_data[v].cult <= -1 ) ) ) {
        return true;
    }
    return false;
}

std::string faction::describe() const
{
    std::string ret = _( desc.c_str() );
    ret = ret + "\n\n" + string_format( _( "%1$s have the ultimate goal of %2$s." ), _( name.c_str() ),
                                        pgettext( "faction_goal", facgoal_data[goal].name.c_str() ) );
    if( job2 == FACJOB_NULL ) {
        ret += string_format( _( " Their primary concern is %s." ),
                              pgettext( "faction_job", facjob_data[job1].name.c_str() ) );
    } else {
        ret += string_format( _( " Their primary concern is %1$s, but they are also involved in %2$s." ),
                              pgettext( "faction_job", facjob_data[job1].name.c_str() ),
                              pgettext( "faction_job", facjob_data[job2].name.c_str() ) );
    }
    if( values == 0 ) {
        return ret;
    }
    std::vector<faction_value> vals;
    vals.reserve( NUM_FACVALS );
    for( int i = 0; i < NUM_FACVALS; i++ ) {
        vals.push_back( faction_value( i ) );
    }
    const std::string known_vals = enumerate_as_string( vals.begin(),
    vals.end(), [ this ]( const faction_value val ) {
        return has_value( val ) ? pgettext( "faction_value", facval_data[val].name.c_str() ) : "";
    } );
    if( !known_vals.empty() ) {
        ret += _( " They are known for " ) + known_vals + ".";
    }
    return ret;
}

int faction::response_time( const tripoint &abs_sm_pos ) const
{
    int base = abs( mapx - abs_sm_pos.x );
    if( abs( mapy - abs_sm_pos.y ) > base ) {
        base = abs( mapy - abs_sm_pos.y );
    }
    if( base > size ) { // Out of our sphere of influence
        base *= 2.5;
    }
    base *= 24; // 24 turns to move one overmap square
    int maxdiv = 10;
    if( goal == FACGOAL_DOMINANCE ) {
        maxdiv += 2;
    }
    if( has_job( FACJOB_CARAVANS ) ) {
        maxdiv += 2;
    }
    if( has_job( FACJOB_SCAVENGE ) ) {
        maxdiv++;
    }
    if( has_job( FACJOB_MERCENARIES ) ) {
        maxdiv += 2;
    }
    if( has_job( FACJOB_FARMERS ) ) {
        maxdiv -= 2;
    }
    if( has_value( FACVAL_EXPLORATION ) ) {
        maxdiv += 2;
    }
    if( has_value( FACVAL_LONERS ) ) {
        maxdiv -= 3;
    }
    if( has_value( FACVAL_TREACHERY ) ) {
        maxdiv -= rng( 0, 3 );
    }
    int mindiv = ( maxdiv > 9 ? maxdiv - 9 : 1 );
    base /= rng( mindiv, maxdiv ); // We might be in the field
    base -= likes_u; // We'll hurry, if we like you
    if( base < 100 ) {
        base = 100;
    }
    return base;
}

// END faction:: MEMBER FUNCTIONS

std::string invent_name()
{
    std::string ret;
    std::string tmp;
    int syllables = rng( 2, 3 );
    for( int i = 0; i < syllables; i++ ) {
        switch( rng( 0, 25 ) ) {
            case  0:
                tmp = pgettext( "faction name", "ab" );
                break;
            case  1:
                tmp = pgettext( "faction name", "bon" );
                break;
            case  2:
                tmp = pgettext( "faction name", "cor" );
                break;
            case  3:
                tmp = pgettext( "faction name", "den" );
                break;
            case  4:
                tmp = pgettext( "faction name", "el" );
                break;
            case  5:
                tmp = pgettext( "faction name", "fes" );
                break;
            case  6:
                tmp = pgettext( "faction name", "gun" );
                break;
            case  7:
                tmp = pgettext( "faction name", "hit" );
                break;
            case  8:
                tmp = pgettext( "faction name", "id" );
                break;
            case  9:
                tmp = pgettext( "faction name", "jan" );
                break;
            case 10:
                tmp = pgettext( "faction name", "kal" );
                break;
            case 11:
                tmp = pgettext( "faction name", "ler" );
                break;
            case 12:
                tmp = pgettext( "faction name", "mal" );
                break;
            case 13:
                tmp = pgettext( "faction name", "nor" );
                break;
            case 14:
                tmp = pgettext( "faction name", "or" );
                break;
            case 15:
                tmp = pgettext( "faction name", "pan" );
                break;
            case 16:
                tmp = pgettext( "faction name", "qua" );
                break;
            case 17:
                tmp = pgettext( "faction name", "ros" );
                break;
            case 18:
                tmp = pgettext( "faction name", "sin" );
                break;
            case 19:
                tmp = pgettext( "faction name", "tor" );
                break;
            case 20:
                tmp = pgettext( "faction name", "urr" );
                break;
            case 21:
                tmp = pgettext( "faction name", "ven" );
                break;
            case 22:
                tmp = pgettext( "faction name", "wel" );
                break;
            case 23:
                tmp = pgettext( "faction name", "oxo" );
                break;
            case 24:
                tmp = pgettext( "faction name", "yen" );
                break;
            case 25:
                tmp = pgettext( "faction name", "zu" );
                break;
        }
        ret += tmp;
    }

    return capitalize_letter( ret );
}

std::string invent_adj()
{
    int syllables = dice( 2, 2 ) - 1;
    std::string ret;
    std::string tmp;
    switch( rng( 0, 25 ) ) {
        case  0:
            ret = pgettext( "faction adjective", "Ald" );
            break;
        case  1:
            ret = pgettext( "faction adjective", "Brogg" );
            break;
        case  2:
            ret = pgettext( "faction adjective", "Cald" );
            break;
        case  3:
            ret = pgettext( "faction adjective", "Dredd" );
            break;
        case  4:
            ret = pgettext( "faction adjective", "Eld" );
            break;
        case  5:
            ret = pgettext( "faction adjective", "Forr" );
            break;
        case  6:
            ret = pgettext( "faction adjective", "Gugg" );
            break;
        case  7:
            ret = pgettext( "faction adjective", "Horr" );
            break;
        case  8:
            ret = pgettext( "faction adjective", "Ill" );
            break;
        case  9:
            ret = pgettext( "faction adjective", "Jov" );
            break;
        case 10:
            ret = pgettext( "faction adjective", "Kok" );
            break;
        case 11:
            ret = pgettext( "faction adjective", "Lill" );
            break;
        case 12:
            ret = pgettext( "faction adjective", "Moom" );
            break;
        case 13:
            ret = pgettext( "faction adjective", "Nov" );
            break;
        case 14:
            ret = pgettext( "faction adjective", "Orb" );
            break;
        case 15:
            ret = pgettext( "faction adjective", "Perv" );
            break;
        case 16:
            ret = pgettext( "faction adjective", "Quot" );
            break;
        case 17:
            ret = pgettext( "faction adjective", "Rar" );
            break;
        case 18:
            ret = pgettext( "faction adjective", "Suss" );
            break;
        case 19:
            ret = pgettext( "faction adjective", "Torr" );
            break;
        case 20:
            ret = pgettext( "faction adjective", "Umbr" );
            break;
        case 21:
            ret = pgettext( "faction adjective", "Viv" );
            break;
        case 22:
            ret = pgettext( "faction adjective", "Warr" );
            break;
        case 23:
            ret = pgettext( "faction adjective", "Xen" );
            break;
        case 24:
            ret = pgettext( "faction adjective", "Yend" );
            break;
        case 25:
            ret = pgettext( "faction adjective", "Zor" );
            break;
    }
    for( int i = 0; i < syllables - 2; i++ ) {
        switch( rng( 0, 17 ) ) {
            case  0:
                tmp = pgettext( "faction adjective", "al" );
                break;
            case  1:
                tmp = pgettext( "faction adjective", "arn" );
                break;
            case  2:
                tmp = pgettext( "faction adjective", "astr" );
                break;
            case  3:
                tmp = pgettext( "faction adjective", "antr" );
                break;
            case  4:
                tmp = pgettext( "faction adjective", "ent" );
                break;
            case  5:
                tmp = pgettext( "faction adjective", "ell" );
                break;
            case  6:
                tmp = pgettext( "faction adjective", "ev" );
                break;
            case  7:
                tmp = pgettext( "faction adjective", "emm" );
                break;
            case  8:
                tmp = pgettext( "faction adjective", "empr" );
                break;
            case  9:
                tmp = pgettext( "faction adjective", "ill" );
                break;
            case 10:
                tmp = pgettext( "faction adjective", "ial" );
                break;
            case 11:
                tmp = pgettext( "faction adjective", "ior" );
                break;
            case 12:
                tmp = pgettext( "faction adjective", "ordr" );
                break;
            case 13:
                tmp = pgettext( "faction adjective", "oth" );
                break;
            case 14:
                tmp = pgettext( "faction adjective", "omn" );
                break;
            case 15:
                tmp = pgettext( "faction adjective", "uv" );
                break;
            case 16:
                tmp = pgettext( "faction adjective", "ulv" );
                break;
            case 17:
                tmp = pgettext( "faction adjective", "urn" );
                break;
        }
        ret += tmp;
    }
    switch( rng( 0, 24 ) ) {
        case  0:
            tmp.clear();
            break;
        case  1:
            tmp = pgettext( "faction adjective", "al" );
            break;
        case  2:
            tmp = pgettext( "faction adjective", "an" );
            break;
        case  3:
            tmp = pgettext( "faction adjective", "ard" );
            break;
        case  4:
            tmp = pgettext( "faction adjective", "ate" );
            break;
        case  5:
            tmp = pgettext( "faction adjective", "e" );
            break;
        case  6:
            tmp = pgettext( "faction adjective", "ed" );
            break;
        case  7:
            tmp = pgettext( "faction adjective", "en" );
            break;
        case  8:
            tmp = pgettext( "faction adjective", "er" );
            break;
        case  9:
            tmp = pgettext( "faction adjective", "ial" );
            break;
        case 10:
            tmp = pgettext( "faction adjective", "ian" );
            break;
        case 11:
            tmp = pgettext( "faction adjective", "iated" );
            break;
        case 12:
            tmp = pgettext( "faction adjective", "ier" );
            break;
        case 13:
            tmp = pgettext( "faction adjective", "ious" );
            break;
        case 14:
            tmp = pgettext( "faction adjective", "ish" );
            break;
        case 15:
            tmp = pgettext( "faction adjective", "ive" );
            break;
        case 16:
            tmp = pgettext( "faction adjective", "oo" );
            break;
        case 17:
            tmp = pgettext( "faction adjective", "or" );
            break;
        case 18:
            tmp = pgettext( "faction adjective", "oth" );
            break;
        case 19:
            tmp = pgettext( "faction adjective", "old" );
            break;
        case 20:
            tmp = pgettext( "faction adjective", "ous" );
            break;
        case 21:
            tmp = pgettext( "faction adjective", "ul" );
            break;
        case 22:
            tmp = pgettext( "faction adjective", "un" );
            break;
        case 23:
            tmp = pgettext( "faction adjective", "ule" );
            break;
        case 24:
            tmp = pgettext( "faction adjective", "y" );
            break;
    }
    ret += tmp;
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
    if( val <=  -5 ) {
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

std::string fac_food_supply_text( int val, int size )
{
    //Convert to how many days you can support the population
    val = val / ( size * 288 );
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

nc_color get_food_supply_color( int val, int size )
{
    nc_color col;
    val = val / ( size * 288 );
    if( val >= 30 ) {
        col = c_green;
    } else if( val >= 14 ) {
        col = c_light_green;
    } else if( val >= 6 ) {
        col = c_yellow;
    } else if( val >= 3 ) {
        col = c_light_red;
    } else {
        col = c_red;
    }
    return col;
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

void faction_manager::clear()
{
    factions.clear();
}

void faction_manager::create_if_needed()
{
    if( !factions.empty() ) {
        return;
    }
    for( const auto &elem : _all_faction_templates ) {
        factions.emplace_back( elem.second );
    }
}

faction *faction_manager::get( const faction_id &id )
{
    for( faction &elem : factions ) {
        if( elem.id == id ) {
            return &elem;
        }
    }
    debugmsg( "Requested non-existing faction '%s'", id.str() );
    return nullptr;
}
// this is legacy and un-used, but will be incorporated into proper factions
void faction_manager::display() const
{
    std::vector<const faction *> valfac; // Factions that we know of.
    for( const faction &elem : factions ) {
        if( elem.known_by_u ) {
            valfac.push_back( &elem );
        }
    }
    if( valfac.empty() ) { // We don't know of any factions!
        popup( _( "You don't know of any factions.  Press Spacebar..." ) );
        return;
    }

    catacurses::window w_list = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                ( ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ),
                                ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );
    catacurses::window w_info = catacurses::newwin( FULL_SCREEN_HEIGHT - 2,
                                FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE,
                                1 + ( ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ),
                                MAX_FAC_NAME_SIZE + ( ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ) );

    const int maxlength = FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE;
    size_t sel = 0;
    bool redraw = true;

    input_context ctxt( "FACTIONS" );
    ctxt.register_action( "UP", _( "Move cursor up" ) );
    ctxt.register_action( "DOWN", _( "Move cursor down" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    while( true ) {
        const faction &cur_frac = *valfac[sel];
        if( redraw ) {
            werase( w_list );
            draw_border( w_list );
            mvwprintz( w_list, 1, 1, c_white, _( "FACTIONS:" ) );
            for( size_t i = 0; i < valfac.size(); i++ ) {
                nc_color col = ( i == sel ? h_white : c_white );
                mvwprintz( w_list, i + 2, 1, col, _( valfac[i]->name.c_str() ) );
            }
            wrefresh( w_list );
            werase( w_info );
            mvwprintz( w_info, 0, 0, c_white, _( "Ranking:           %s" ),
                       fac_ranking_text( cur_frac.likes_u ) );
            mvwprintz( w_info, 1, 0, c_white, _( "Respect:           %s" ),
                       fac_respect_text( cur_frac.respects_u ) );
            mvwprintz( w_info, 2, 0, c_white, _( "Wealth:            %s" ), fac_wealth_text( cur_frac.wealth,
                       cur_frac.size ) );
            mvwprintz( w_info, 3, 0, c_white, _( "Food Supply:       %s" ),
                       fac_food_supply_text( cur_frac.food_supply, cur_frac.size ) );
            mvwprintz( w_info, 4, 0, c_white, _( "Combat Ability:    %s" ),
                       fac_combat_ability_text( cur_frac.combat_ability ) );
            fold_and_print( w_info, 6, 0, maxlength, c_white, cur_frac.describe() );
            wrefresh( w_info );
            redraw = false;
        }
        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            mvwprintz( w_list, sel + 2, 1, c_white, cur_frac.name );
            sel = ( sel + 1 ) % valfac.size();
            redraw = true;
        } else if( action == "UP" ) {
            mvwprintz( w_list, sel + 2, 1, c_white, cur_frac.name );
            sel = ( sel + valfac.size() - 1 ) % valfac.size();
            redraw = true;
        } else if( action == "HELP_KEYBINDINGS" ) {
            redraw = true;
        } else if( action == "QUIT" ) {
            break;
        } else if( action == "CONFIRM" ) {
            break;
        }
    }
}
void new_faction_manager::display() const
{
    catacurses::window w_missions = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                    ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                                    ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );

    enum class tab_mode : int {
        TAB_MYFACTION = 0,
        TAB_FOLLOWERS,
        TAB_OTHERFACTIONS,
        NUM_TABS,
        FIRST_TAB = 0,
        LAST_TAB = NUM_TABS - 1
    };
    g->validate_camps();
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
    const tripoint player_abspos =  g->u.global_omt_location();
    while( true ) {
        werase( w_missions );
        // create a list of NPCs, visible and the ones on overmapbuffer
        g->validate_npc_followers();
        std::vector<npc *> followers;
        for( auto &elem : g->get_follower_list() ) {
            std::shared_ptr<npc> npc_to_get = overmap_buffer.find_npc( elem );
            npc *npc_to_add = npc_to_get.get();
            followers.push_back( npc_to_add );
        }
        npc *guy = nullptr;
        bool interactable = false;
        basecamp *camp = nullptr;
        // create a list of faction camps
        std::vector<basecamp *> camps;
        for( auto elem : g->u.camps ) {
            cata::optional<basecamp *> p = overmap_buffer.find_camp( elem.x, elem.y );
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
        int active_vec_size;
        // entries_per_page * page number
        const int top_of_page = entries_per_page * ( selection / entries_per_page );
        if( tab == tab_mode::TAB_FOLLOWERS ) {
            if( !followers.empty() ) {
                guy = followers[selection];
            }
            active_vec_size = followers.size();
        } else if( tab == tab_mode::TAB_MYFACTION ) {
            if( !camps.empty() ) {
                camp = camps[selection];
            }
            active_vec_size = camps.size();
        } else {
            active_vec_size = camps.size();
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
        switch( tab ) {
            case tab_mode::TAB_MYFACTION:
                if( active_vec_size > 0 ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size, 3, 0 );
                    for( int i = top_of_page; i <= ( active_vec_size - 1 ); i++ ) {
                        const auto camp = camps[i];
                        const std::string &camp_name = camp->camp_name();
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, y, 1, 28, static_cast<int>( selection ) == i ? hilite( col ) : col,
                                        camp_name );
                    }
                    if( selection < camps.size() ) {
                        int y = 2;
                        tripoint camp_pos = camp->camp_omt_pos();
                        std::string direction = direction_name( direction_from(
                                player_abspos, camp_pos ) );
                        mvwprintz( w_missions, ++y, 31, c_light_gray, _( "Press enter to rename this camp" ) );
                        std::string centerstring = "center";
                        if( ( !direction.compare( centerstring ) ) == 0 ) {
                            mvwprintz( w_missions, ++y, 31, c_light_gray,
                                       _( "Direction : to the " ) + direction );
                        }
                        mvwprintz( w_missions, ++y, 31, col, _( "Location : (%d, %d)" ), camp_pos.x, camp_pos.y );
                        faction *yours = g->faction_manager_ptr->get( faction_id( "your_followers" ) );
                        std::string calorie_value = ( std::to_string( yours->food_supply ) ) + _( " calories" );
                        std::string food_supply = fac_food_supply_text( yours->food_supply, yours->size );
                        std::string food_supply_text = _( "Food Supply : " ) + food_supply;
                        std::string food_supply_full = food_supply_text + " " + calorie_value;
                        nc_color food_col = get_food_supply_color( yours->food_supply, yours->size );
                        mvwprintz( w_missions, ++y, 31, food_col, food_supply_full );
                        const std::string base_dir = "[B]";
                        std::string bldg = camp->next_upgrade( base_dir, 1 );
                        std::string bldg_full = _( "Next Upgrade : " ) + bldg;
                        mvwprintz( w_missions, ++y, 31, col, bldg_full );
                        std::string requirements = camp->om_upgrade_description( bldg, true );
                        fold_and_print( w_missions, ++y, 31, getmaxx( w_missions ) - 33, col,
                                        ( requirements ) );
                    } else {
                        static const std::string nope = _( "You have no camps" );
                        mvwprintz( w_missions, 4, 31, c_light_red, nope );
                    }
                    break;
                } else {
                    static const std::string nope = _( "You have no camps" );
                    mvwprintz( w_missions, 4, 31, c_light_red, nope );
                }
                break;
            case tab_mode::TAB_FOLLOWERS:
                if( !followers.empty() ) {
                    draw_scrollbar( w_missions, selection, entries_per_page, active_vec_size, 3, 0 );
                    for( int i = top_of_page; i <= ( active_vec_size - 1 ); i++ ) {
                        const auto guy = followers[i];
                        const int y = i - top_of_page + 3;
                        trim_and_print( w_missions, y, 1, 28, static_cast<int>( selection ) == i ? hilite( col ) : col,
                                        guy->disp_name() );
                    }
                    if( selection < followers.size() ) {
                        int y = 2;
                        //get NPC followers, status, direction, location, needs, weapon, etc.
                        mvwprintz( w_missions, ++y, 31, c_light_gray, _( "Press enter to talk to this follower " ) );
                        std::string mission_string;
                        if( guy->has_companion_mission() ) {
                            npc_companion_mission c_mission = guy->get_companion_mission();
                            mission_string = _( "Current Mision : " ) + get_mission_action_string( c_mission.mission_id );
                        }
                        fold_and_print( w_missions, ++y, 31, getmaxx( w_missions ) - 33, col,
                                        mission_string );
                        tripoint guy_abspos = guy->global_omt_location();
                        std::string direction = direction_name( direction_from(
                                player_abspos, guy_abspos ) );
                        std::string centerstring = "center";
                        if( ( !direction.compare( centerstring ) ) == 0 ) {
                            mvwprintz( w_missions, ++y, 31, col,
                                       _( "Direction : to the " ) + direction );
                        } else {
                            mvwprintz( w_missions, ++y, 31, col,
                                       _( "Direction : Nearby" ) );
                        }
                        mvwprintz( w_missions, ++y, 31, col, _( "Location : (%d, %d)" ), guy_abspos.x, guy_abspos.y );
                        std::string can_see;
                        nc_color see_color;
                        const std::vector<npc *> interactable_followers = g->get_npcs_if( [&]( const npc & guy ) {
                            return ( ( guy.is_friend() && guy.is_following() ) || guy.mission == NPC_MISSION_GUARD_ALLY ) &&
                                   g->u.posz() == guy.posz() &&
                                   g->u.sees( guy.pos() ) && rl_dist( g->u.pos(), guy.pos() ) <= SEEX * 2;
                        } );
                        if( std::find( interactable_followers.begin(), interactable_followers.end(),
                                       guy ) != interactable_followers.end() ) {
                            interactable = true;
                            can_see = "Within interaction range";
                            see_color = c_light_green;
                        } else {
                            interactable = false;
                            can_see = "Not within interaction range";
                            see_color = c_light_red;
                        }
                        mvwprintz( w_missions, ++y, 31, see_color, can_see );
                        nc_color status_col = col;
                        std::string current_status = _( "Status : " );
                        if( guy->current_target() != nullptr ) {
                            current_status += _( "In Combat!" );
                            status_col = c_light_red;
                        } else if( guy->in_sleep_state() ) {
                            current_status += _( "Sleeping" );
                        } else if( guy->is_following() ) {
                            current_status += _( "Following" );
                        } else if( guy->is_leader() ) {
                            current_status += _( "Leading" );
                        } else if( guy->is_guarding() ) {
                            current_status += _( "Guarding" );
                        }
                        mvwprintz( w_missions, ++y, 31, status_col, current_status );
                        const std::pair <std::string, nc_color> condition = guy->hp_description();
                        const std::string condition_string = _( "Condition : " ) + condition.first;
                        mvwprintz( w_missions, ++y, 31, condition.second, condition_string );
                        const std::pair <std::string, nc_color> hunger_pair = guy->get_hunger_description();
                        const std::pair <std::string, nc_color> thirst_pair = guy->get_thirst_description();
                        const std::pair <std::string, nc_color> fatigue_pair = guy->get_fatigue_description();
                        mvwprintz( w_missions, ++y, 31, hunger_pair.second,
                                   _( "Hunger : " ) + ( hunger_pair.first.empty() ? "Nominal" : hunger_pair.first ) );
                        mvwprintz( w_missions, ++y, 31, thirst_pair.second,
                                   _( "Thirst : " ) + ( thirst_pair.first.empty() ? "Nominal" : thirst_pair.first ) );
                        mvwprintz( w_missions, ++y, 31, fatigue_pair.second,
                                   _( "Fatigue : " ) + ( fatigue_pair.first.empty() ? "Nominal" : fatigue_pair.first ) );
                        const std::string wield_str = _( "Wielding : " ) + guy->weapon.tname();
                        int lines = fold_and_print( w_missions, ++y, 31, getmaxx( w_missions ) - 33, c_white,
                                                    ( wield_str ) );
                        y += lines;
                        const auto skillslist = Skill::get_skills_sorted_by( [&]( const Skill & a, const Skill & b ) {
                            const int level_a = guy->get_skill_level( a.ident() );
                            const int level_b = guy->get_skill_level( b.ident() );
                            return level_a > level_b || ( level_a == level_b && a.name() < b.name() );
                        } );
                        size_t count = 0;
                        std::vector<std::string> skill_strs;
                        for( int i = 0; i < static_cast<int>( skillslist.size() ) && count < 3; i++ ) {
                            if( !skillslist[ i ]->is_combat_skill() ) {
                                std::string skill_str = skillslist[i]->name() + " : " + std::to_string( guy->get_skill_level(
                                                            skillslist[i]->ident() ) );
                                skill_strs.push_back( skill_str );
                                count += 1;
                            }
                        }
                        std::string best_three_noncombat = _( "Best other skills : " );
                        std::string best_skill = _( "Best combat skill : " ) + guy->best_skill().obj().name() + " : " +
                                                 std::to_string( guy->best_skill_level() );
                        mvwprintz( w_missions, ++y, 31, col, best_skill );
                        mvwprintz( w_missions, ++y, 31, col, best_three_noncombat + skill_strs[0] );
                        mvwprintz( w_missions, ++y, 51, col, skill_strs[1] );
                        mvwprintz( w_missions, ++y, 51, col, skill_strs[2] );
                    } else {
                        static const std::string nope = _( "You have no followers" );
                        mvwprintz( w_missions, 4, 31, c_light_red, nope );
                    }
                    break;
                } else {
                    static const std::string nope = _( "You have no followers" );
                    mvwprintz( w_missions, 4, 31, c_light_red, nope );
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
            if( selection >= static_cast<size_t>( active_vec_size ) ) {
                selection = 0;
            }
        } else if( action == "UP" ) {
            if( selection == 0 ) {
                selection = active_vec_size == 0 ? 0 : active_vec_size - 1;
            } else {
                selection--;
            }
        } else if( action == "CONFIRM" ) {
            if( tab == tab_mode::TAB_FOLLOWERS && interactable && guy ) {
                guy->talk_to_u();
            } else if( tab == tab_mode::TAB_MYFACTION && camp ) {
                camp->query_new_name();
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }

    g->refresh_all();
}
