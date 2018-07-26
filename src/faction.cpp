#include "faction.h"
#include <sstream>

#include "rng.h"
#include "math.h"
#include "string_formatter.h"
#include "output.h"
#include "debug.h"
#include "input.h"
#include "cursesdef.h"
#include "enums.h"
#include "game_constants.h"
#include "catacharset.h"

#include "json.h"
#include "translations.h"

#include <map>
#include <string>
#include <cstdlib>

static std::map<faction_id, faction_template> _all_faction_templates;

std::string invent_name();
std::string invent_adj();

constexpr inline unsigned long mfb( const int v )
{
    return 1 << v;
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
    static_cast<faction_template&>( *this ) = templ;
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

//TODO move them to json

static const std::array<std::string, 15> faction_adj_pos = { {
    translate_marker("Shining"), translate_marker("Sacred"), translate_marker("Golden"), translate_marker("Holy"), translate_marker("Righteous"), translate_marker("Devoted"),
    translate_marker("Virtuous"), translate_marker("Splendid"), translate_marker("Divine"), translate_marker("Radiant"), translate_marker("Noble"), translate_marker("Venerable"),
    translate_marker("Immaculate"), translate_marker("Heroic"), translate_marker("Bright")
} };

static const std::array<std::string, 15> faction_adj_neu = {{
    translate_marker("Original"), translate_marker("Crystal"), translate_marker("Metal"), translate_marker("Mighty"), translate_marker("Powerful"), translate_marker("Solid"),
    translate_marker("Stone"), translate_marker("Firey"), translate_marker("Colossal"), translate_marker("Famous"), translate_marker("Supreme"), translate_marker("Invincible"),
    translate_marker("Unlimited"), translate_marker("Great"), translate_marker("Electric")
}};

static const std::array<std::string, 15> faction_adj_bad = {{
    translate_marker("Poisonous"), translate_marker("Deadly"), translate_marker("Foul"), translate_marker("Nefarious"), translate_marker("Wicked"), translate_marker("Vile"),
    translate_marker("Ruinous"), translate_marker("Horror"), translate_marker("Devastating"), translate_marker("Vicious"), translate_marker("Sinister"), translate_marker("Baleful"),
    translate_marker("Pestilent"), translate_marker("Pernicious"), translate_marker("Dread")
}};

static const std::array<std::string, 15> faction_noun_strong = {{
    translate_marker("Fists"), translate_marker("Slayers"), translate_marker("Furies"), translate_marker("Dervishes"), translate_marker("Tigers"), translate_marker("Destroyers"),
    translate_marker("Berserkers"), translate_marker("Samurai"), translate_marker("Valkyries"), translate_marker("Army"), translate_marker("Killers"), translate_marker("Paladins"),
    translate_marker("Knights"), translate_marker("Warriors"), translate_marker("Huntsmen")
}};

static const std::array<std::string, 15> faction_noun_sneak = {{
    translate_marker("Snakes"), translate_marker("Rats"), translate_marker("Assassins"), translate_marker("Ninja"), translate_marker("Agents"), translate_marker("Shadows"),
    translate_marker("Guerillas"), translate_marker("Eliminators"), translate_marker("Snipers"), translate_marker("Smoke"), translate_marker("Arachnids"), translate_marker("Creepers"),
    translate_marker("Shade"), translate_marker("Stalkers"), translate_marker("Eels")
}};

static const std::array<std::string, 15> faction_noun_crime = {{
    translate_marker("Bandits"), translate_marker("Punks"), translate_marker("Family"), translate_marker("Mafia"), translate_marker("Mob"), translate_marker("Gang"), translate_marker("Vandals"),
    translate_marker("Sharks"), translate_marker("Muggers"), translate_marker("Cutthroats"), translate_marker("Guild"), translate_marker("Faction"), translate_marker("Thugs"),
    translate_marker("Racket"), translate_marker("Crooks")
}};

static const std::array<std::string, 15> faction_noun_cult = {{
    translate_marker("Brotherhood"), translate_marker("Church"), translate_marker("Ones"), translate_marker("Crucible"), translate_marker("Sect"), translate_marker("Creed"),
    translate_marker("Doctrine"), translate_marker("Priests"), translate_marker("Tenet"), translate_marker("Monks"), translate_marker("Clerics"), translate_marker("Pastors"),
    translate_marker("Gnostics"), translate_marker("Elders"), translate_marker("Inquisitors")
}};

static const std::array<std::string, 15> faction_noun_none = {{
    translate_marker("Settlers"), translate_marker("People"), translate_marker("Men"), translate_marker("Faction"), translate_marker("Tribe"), translate_marker("Clan"), translate_marker("Society"),
    translate_marker("Folk"), translate_marker("Nation"), translate_marker("Republic"), translate_marker("Colony"), translate_marker("State"), translate_marker("Kingdom"), translate_marker("Party"),
    translate_marker("Company")
}};

static const std::array<faction_value_datum, NUM_FACGOALS> facgoal_data = {{
    // "Their ultimate goal is <name>"
    //Name                               Good Str Sneak Crime Cult
    {"Null",                             0,   0,  0,    0,    0},
    {translate_marker("basic survival"),                0,   0,  0,    0,    0},
    {translate_marker("financial wealth"),              0,  -1,  0,    2,   -1},
    {translate_marker("dominance of the region"),      -1,   1, -1,    1,   -1},
    {translate_marker("the extermination of monsters"), 1,   3, -1,   -1,   -1},
    {translate_marker("contact with unseen powers"),   -1,   0,  1,    0,    4},
    {translate_marker("bringing the apocalypse"),      -5,   1,  2,    0,    7},
    {translate_marker("general chaos and anarchy"),    -3,   2, -3,    2,   -1},
    {translate_marker("the cultivation of knowledge"),  2,  -3,  2,   -1,    0},
    {translate_marker("harmony with nature"),           2,  -2,  0,   -1,    2},
    {translate_marker("rebuilding civilization"),       2,   1, -2,   -2,   -4},
    {translate_marker("spreading the fungus"),         -2,   1,  1,    0,    4}
}};
// TOTAL:                               -5    3  -2     0     7

static const std::array<faction_value_datum, NUM_FACJOBS> facjob_data = {{
    // "They earn money via <name>"
    //Name                              Good Str Sneak Crime Cult
    {"Null",                            0,   0,  0,    0,    0},
    {translate_marker("protection rackets"),          -3,   2, -1,    4,    0},
    {translate_marker("the sale of information"),     -1,  -1,  4,    1,    0},
    {translate_marker("their bustling trade centers"), 1,  -1, -2,   -4,   -4},
    {translate_marker("trade caravans"),               2,  -1, -1,   -3,   -2},
    {translate_marker("scavenging supplies"),          0,  -1,  0,   -1,   -1},
    {translate_marker("mercenary work"),               0,   3, -1,    1,   -1},
    {translate_marker("assassinations"),              -1,   2,  2,    1,    1},
    {translate_marker("raiding settlements"),         -4,   4, -3,    3,   -2},
    {translate_marker("the theft of property"),       -3,  -1,  4,    4,    1},
    {translate_marker("gambling parlors"),            -1,  -2, -1,    1,   -1},
    {translate_marker("medical aid"),                  4,  -3, -2,   -3,    0},
    {translate_marker("farming & selling food"),       3,  -4, -2,   -4,    1},
    {translate_marker("drug dealing"),                -2,   0, -1,    2,    0},
    {translate_marker("selling manufactured goods"),   1,   0, -1,   -2,    0}
}};
// TOTAL:                              -5   -3  -5     0    -6

static const std::array<faction_value_datum, NUM_FACVALS> facval_data = {{
    // "They are known for <name>"
    //Name                            Good Str Sneak Crime Cult
    {"Null",                          0,   0,  0,    0,    0},
    {translate_marker("their charitable nature"),    5,  -1, -1,   -2,   -2},
    {translate_marker("their isolationism"),         0,  -2,  1,    0,    2},
    {translate_marker("exploring extensively"),      1,   0,  0,   -1,   -1},
    {translate_marker("collecting rare artifacts"),  0,   1,  1,    0,    3},
    {translate_marker("their knowledge of bionics"), 1,   2,  0,    0,    0},
    {translate_marker("their libraries"),            1,  -3,  0,   -2,    1},
    {translate_marker("their elite training"),       0,   4,  2,    0,    2},
    {translate_marker("their robotics factories"),   0,   3, -1,    0,   -2},
    {translate_marker("treachery"),                 -3,   0,  1,    3,    0},
    {translate_marker("the avoidance of drugs"),     1,   0,  0,   -1,    1},
    {translate_marker("their adherence to the law"), 2,  -1, -1,   -4,   -1},
    {translate_marker("their cruelty"),             -3,   1, -1,    4,    1}
}};
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
    mapx = rng(OMAPX / 10, OMAPX - OMAPX / 10);
    mapy = rng(OMAPY / 10, OMAPY - OMAPY / 10);
    // Pick an overall goal.
    goal = faction_goal(rng(1, NUM_FACGOALS - 1));
    if (one_in(4)) {
        goal = FACGOAL_NONE;    // Slightly more likely to not have a real goal
    }
    good     = facgoal_data[goal].good;
    strength = facgoal_data[goal].strength;
    sneak    = facgoal_data[goal].sneak;
    crime    = facgoal_data[goal].crime;
    cult     = facgoal_data[goal].cult;
    job1 = faction_job(rng(1, NUM_FACJOBS - 1));
    do {
        job2 = faction_job(rng(0, NUM_FACJOBS - 1));
    } while (job2 == job1);
    good     += facjob_data[job1].good     + facjob_data[job2].good;
    strength += facjob_data[job1].strength + facjob_data[job2].strength;
    sneak    += facjob_data[job1].sneak    + facjob_data[job2].sneak;
    crime    += facjob_data[job1].crime    + facjob_data[job2].crime;
    cult     += facjob_data[job1].cult     + facjob_data[job2].cult;

    int num_values = 0;
    int tries = 0;
    values = 0;
    do {
        int v = rng(1, NUM_FACVALS - 1);
        if (!has_value(faction_value(v)) && matches_us(faction_value(v))) {
            values |= mfb(v);
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
    if (sneak > best) {
        sel = 2;
        best = sneak;
    }
    if (crime > best) {
        sel = 3;
        best = crime;
    }
    if (cult > best) {
        sel = 4;
    }
    if (strength <= 0 && sneak <= 0 && crime <= 0 && cult <= 0) {
        sel = 0;
    }

    switch (sel) {
    case 1:
        noun  = _( random_entry_ref( faction_noun_strong ).c_str() );
        power = dice(5, 20);
        size  = dice(5, 6);
        break;
    case 2:
        noun  = _( random_entry_ref( faction_noun_sneak ).c_str() );
        power = dice(5, 8);
        size  = dice(5, 8);
        break;
    case 3:
        noun  = _( random_entry_ref( faction_noun_crime ).c_str() );
        power = dice(5, 16);
        size  = dice(5, 8);
        break;
    case 4:
        noun  = _( random_entry_ref( faction_noun_cult ).c_str() );
        power = dice(8, 8);
        size  = dice(4, 6);
        break;
    default:
        noun  = _( random_entry_ref( faction_noun_none ).c_str() );
        power = dice(6, 8);
        size  = dice(6, 6);
    }

    if (one_in(4)) {
        do {
            name = string_format(_("The %1$s of %2$s"), noun.c_str(), invent_name().c_str());
        } while (utf8_width(name) > MAX_FAC_NAME_SIZE);
    } else if (one_in(2)) {
        do {
            name = string_format(_("The %1$s %2$s"), invent_adj().c_str(), noun.c_str());
        } while (utf8_width(name) > MAX_FAC_NAME_SIZE);
    } else {
        do {
            std::string adj;
            if (good >= 3) {
                adj = _( random_entry_ref( faction_adj_pos ).c_str() );
            } else if  (good <= -3) {
                adj = _( random_entry_ref( faction_adj_bad ).c_str() );
            } else {
                adj = _( random_entry_ref( faction_adj_neu ).c_str() );
            }
            name = string_format(_("The %1$s %2$s"), adj.c_str(), noun.c_str());
            if (one_in(4)) {
                name = string_format(_("%1$s of %2$s"), name.c_str(), invent_name().c_str());
            }
        } while (utf8_width(name) > MAX_FAC_NAME_SIZE);
    }
}

bool faction::has_job(faction_job j) const
{
    return (job1 == j || job2 == j);
}

bool faction::has_value(faction_value v) const
{
    return values & mfb(v);
}

bool faction::matches_us(faction_value v) const
{
    int numvals = 2;
    if (job2 != FACJOB_NULL) {
        numvals++;
    }
    for (int i = 0; i < NUM_FACVALS; i++) {
        if (has_value(faction_value(i))) {
            numvals++;
        }
    }
    if (has_job(FACJOB_DRUGS) && v == FACVAL_STRAIGHTEDGE) { // Mutually exclusive
        return false;
    }
    int avggood = (good / numvals + good) / 2;
    int avgstrength = (strength / numvals + strength) / 2;
    int avgsneak = (sneak / numvals + sneak / 2);
    int avgcrime = (crime / numvals + crime / 2);
    int avgcult = (cult / numvals + cult / 2);
    /*
     debugmsg("AVG: GOO %d STR %d SNK %d CRM %d CLT %d\n\
           VAL: GOO %d STR %d SNK %d CRM %d CLT %d (%s)", avggood, avgstrength,
    avgsneak, avgcrime, avgcult, facval_data[v].good, facval_data[v].strength,
    facval_data[v].sneak, facval_data[v].crime, facval_data[v].cult,
    facval_data[v].name.c_str());
    */
    if ((abs(facval_data[v].good     - avggood)  <= 3 ||
         (avggood >=  5 && facval_data[v].good >=  1) ||
         (avggood <= -5 && facval_data[v].good <=  0))  &&
        (abs(facval_data[v].strength - avgstrength)   <= 5 ||
         (avgstrength >=  5 && facval_data[v].strength >=  3) ||
         (avgstrength <= -5 && facval_data[v].strength <= -1))  &&
        (abs(facval_data[v].sneak    - avgsneak) <= 4 ||
         (avgsneak >=  5 && facval_data[v].sneak >=  1) ||
         (avgsneak <= -5 && facval_data[v].sneak <= -1))  &&
        (abs(facval_data[v].crime    - avgcrime) <= 4 ||
         (avgcrime >=  5 && facval_data[v].crime >=  0) ||
         (avgcrime <= -5 && facval_data[v].crime <= -1))  &&
        (abs(facval_data[v].cult     - avgcult)  <= 3 ||
         (avgcult >=  5 && facval_data[v].cult >=  1) ||
         (avgcult <= -5 && facval_data[v].cult <= -1))) {
        return true;
    }
    return false;
}

std::string faction::describe() const
{
    std::string ret = _( desc.c_str() );
    ret = ret + "\n\n" + string_format( _( "%1$s have the ultimate goal of %2$s." ), _( name.c_str() ),
                                        _( facgoal_data[goal].name.c_str() ) );
    if (job2 == FACJOB_NULL) {
        ret += string_format( _(" Their primary concern is %s."), _( facjob_data[job1].name.c_str()));
    } else {
        ret += string_format( _(" Their primary concern is %1$s, but they are also involved in %2$s."),
                              _( facjob_data[job1].name.c_str()),
                              _( facjob_data[job2].name.c_str()));
    }
    if( values == 0 ) {
        return ret;
    }
    std::vector<faction_value> vals;
    vals.reserve( NUM_FACVALS );
    for( int i = 0; i < NUM_FACVALS; i++ ) {
        vals.push_back( faction_value( i ) );
    }
    const std::string known_vals = enumerate_as_string( vals.begin(), vals.end(), [ this ]( const faction_value val ) {
        return has_value( val ) ? _( facval_data[val].name.c_str() ) : "";
    } );
    if( !known_vals.empty() ) {
        ret += _( " They are known for " ) + known_vals + ".";
    }
    return ret;
}

int faction::response_time( const tripoint &abs_sm_pos ) const
{
    int base = abs( mapx - abs_sm_pos.x );
    if (abs( mapy - abs_sm_pos.y ) > base) {
        base = abs( mapy - abs_sm_pos.y );
    }
    if (base > size) { // Out of our sphere of influence
        base *= 2.5;
    }
    base *= 24; // 24 turns to move one overmap square
    int maxdiv = 10;
    if (goal == FACGOAL_DOMINANCE) {
        maxdiv += 2;
    }
    if (has_job(FACJOB_CARAVANS)) {
        maxdiv += 2;
    }
    if (has_job(FACJOB_SCAVENGE)) {
        maxdiv++;
    }
    if (has_job(FACJOB_MERCENARIES)) {
        maxdiv += 2;
    }
    if (has_job(FACJOB_FARMERS)) {
        maxdiv -= 2;
    }
    if (has_value(FACVAL_EXPLORATION)) {
        maxdiv += 2;
    }
    if (has_value(FACVAL_LONERS)) {
        maxdiv -= 3;
    }
    if (has_value(FACVAL_TREACHERY)) {
        maxdiv -= rng(0, 3);
    }
    int mindiv = (maxdiv > 9 ? maxdiv - 9 : 1);
    base /= rng(mindiv, maxdiv);// We might be in the field
    base -= likes_u; // We'll hurry, if we like you
    if (base < 100) {
        base = 100;
    }
    return base;
}


// END faction:: MEMBER FUNCTIONS


std::string invent_name()
{
    std::string ret = "";
    std::string tmp;
    int syllables = rng(2, 3);
    for (int i = 0; i < syllables; i++) {
        switch (rng(0, 25)) {
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

    return capitalize_letter(ret);
}

std::string invent_adj()
{
    int syllables = dice(2, 2) - 1;
    std::string ret;
    std::string tmp;
    switch (rng(0, 25)) {
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
    for (int i = 0; i < syllables - 2; i++) {
        switch (rng(0, 17)) {
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
    switch (rng(0, 24)) {
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
std::string fac_ranking_text(int val)
{
    if (val <= -100) {
        return _("Archenemy");
    }
    if (val <= -80) {
        return _("Wanted Dead");
    }
    if (val <= -60) {
        return _("Enemy of the People");
    }
    if (val <= -40) {
        return _("Wanted Criminal");
    }
    if (val <= -20) {
        return _("Not Welcome");
    }
    if (val <= -10) {
        return _("Pariah");
    }
    if (val <=  -5) {
        return _("Disliked");
    }
    if (val >= 100) {
        return _("Hero");
    }
    if (val >= 80) {
        return _("Idol");
    }
    if (val >= 60) {
        return _("Beloved");
    }
    if (val >= 40) {
        return _("Highly Valued");
    }
    if (val >= 20) {
        return _("Valued");
    }
    if (val >= 10) {
        return _("Well-Liked");
    }
    if (val >= 5) {
        return _("Liked");
    }

    return _("Neutral");
}

// Used in game.cpp
std::string fac_respect_text(int val)
{
    // Respected, feared, etc.
    if (val >= 100) {
        return _("Legendary");
    }
    if (val >= 80) {
        return _("Unchallenged");
    }
    if (val >= 60) {
        return _("Mighty");
    }
    if (val >= 40) {
        return _("Famous");
    }
    if (val >= 20) {
        return _("Well-Known");
    }
    if (val >= 10) {
        return _("Spoken Of");
    }

    // Disrespected, laughed at, etc.
    if (val <= -100) {
        return _("Worthless Scum");
    }
    if (val <= -80) {
        return _("Vermin");
    }
    if (val <= -60) {
        return _("Despicable");
    }
    if (val <= -40) {
        return _("Parasite");
    }
    if (val <= -20) {
        return _("Leech");
    }
    if (val <= -10) {
        return _("Laughingstock");
    }

    return _("Neutral");
}

std::string fac_wealth_text(int val, int size)
{
    //Wealth per person
    val = val/size;
    if (val >= 1000000) {
        return _("Filthy rich");
    }
    if (val >= 750000) {
        return _("Affluent");
    }
    if (val >= 500000) {
        return _("Prosperous");
    }
    if (val >= 250000) {
        return _("Well-Off");
    }
    if (val >= 100000) {
        return _("Comfortable");
    }
    if (val >= 85000) {
        return _("Wanting");
    }
    if (val >= 70000) {
        return _("Failing");
    }
    if (val >= 50000) {
        return _("Impoverished");
    }
    return _("Destitute");
}

std::string fac_food_supply_text(int val, int size)
{
    //Convert to how many days you can support the population
    val = val/(size*288);
    if (val >= 30) {
        return _("Overflowing");
    }
    if (val >= 14) {
        return _("Well-Stocked");
    }
    if (val >= 6) {
        return _("Scrapping By");
    }
    if (val >= 3) {
        return _("Malnourished");
    }
    return _("Starving");
}

std::string fac_combat_ability_text(int val)
{
    if (val >= 150) {
        return _("Legendary");
    }
    if (val >= 130) {
        return _("Expert");
    }
    if (val >= 115) {
        return _("Veteran");
    }
    if (val >= 105) {
        return _("Skilled");
    }
    if (val >= 95) {
        return _("Competent");
    }
    if (val >= 85) {
        return _("Untrained");
    }
    if (val >= 75) {
        return _("Crippled");
    }
    if (val >= 50) {
        return _("Feeble");
    }
    return _("Worthless");
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
