#include <sstream>

#include "faction.h"
#include "rng.h"
#include "math.h"
#include "output.h"
#include "omdata.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "catacharset.h"

#include "json.h"
#include "translations.h"
#include <string>
#include <cstdlib>

std::string invent_name();
std::string invent_adj();

faction::faction()
{
    // debugmsg("Warning: Faction created without UID!");
    name = "null";
    values = 0;
    likes_u = 0;
    respects_u = 0;
    known_by_u = true;
    goal = FACGOAL_NULL;
    job1 = FACJOB_NULL;
    job2 = FACJOB_NULL;
    strength = 0;
    combat_ability = 0;
    food_supply = 0;
    wealth = 0;
    sneak = 0;
    crime = 0;
    cult = 0;
    good = 0;
    mapx = 0;
    mapy = 0;
    size = 0;
    power = 0;
    id = "";
    desc = "";
}

faction::faction(std::string uid)
{
    name = "";
    values = 0;
    likes_u = 0;
    respects_u = 0;
    known_by_u = true;
    goal = FACGOAL_NULL;
    job1 = FACJOB_NULL;
    job2 = FACJOB_NULL;
    strength = 0;
    sneak = 0;
    crime = 0;
    cult = 0;
    good = 0;
    mapx = 0;
    mapy = 0;
    size = 0;
    power = 0;
    combat_ability = 0;
    food_supply = 0;
    wealth = 0;
    id = uid;
    desc = "";
}

faction_map faction::_all_faction;

void faction::load_faction(JsonObject &jsobj)
{
    faction fac;
    fac.id = jsobj.get_string("id");
    fac.name = jsobj.get_string("name");
    fac.likes_u = jsobj.get_int("likes_u");
    fac.respects_u = jsobj.get_int("respects_u");
    fac.known_by_u = jsobj.get_bool("known_by_u");
    fac.size = jsobj.get_int("size");
    fac.power = jsobj.get_int("power");
    fac.combat_ability = jsobj.get_int("combat_ability");
    fac.food_supply = jsobj.get_int("food_supply");
    fac.wealth = jsobj.get_int("wealth");
    fac.good = jsobj.get_int("good");
    fac.strength = jsobj.get_int("strength");
    fac.sneak = jsobj.get_int("sneak");
    fac.crime = jsobj.get_int("crime");
    fac.cult = jsobj.get_int("cult");
    fac.desc = jsobj.get_string("description");
    _all_faction[jsobj.get_string("id")] = fac;
}

faction *faction::find_faction(std::string ident)
{
    faction_map::iterator found = _all_faction.find(ident);
    if (found != _all_faction.end()) {
        return &(found->second);
    } else {
        debugmsg("Tried to get invalid faction: %s", ident.c_str());
        static faction null_faction;
        return &null_faction;
    }
}

void faction::load_faction_template(std::string ident)
{
    faction_map::iterator found = _all_faction.find(ident);
    if (found != _all_faction.end()) {
        id = found->second.id;
        name = found->second.name;
        likes_u = found->second.likes_u;
        respects_u = found->second.respects_u;
        known_by_u = found->second.known_by_u;
        size = found->second.size;
        power = found->second.power;
        combat_ability = found->second.combat_ability;
        food_supply = found->second.food_supply;
        wealth = found->second.wealth;
        good = found->second.good;
        strength = found->second.strength;
        sneak = found->second.sneak;
        crime = found->second.crime;
        cult = found->second.cult;
        desc = found->second.desc;

        return;
    } else {
        debugmsg("Tried to get invalid faction: %s", ident.c_str());
        return;
    }
}

std::vector<std::string> faction::all_json_factions()
{
    std::vector<std::string> v;
    for(std::map<std::string, faction>::const_iterator it = _all_faction.begin();
        it != _all_faction.end(); it++) {
        v.push_back(it -> first.c_str());
    }
    return v;
}

faction::~faction()
{
}

std::string faction::faction_adj_pos[15];
std::string faction::faction_adj_neu[15];
std::string faction::faction_adj_bad[15];
std::string faction::faction_noun_strong[15];
std::string faction::faction_noun_sneak[15];
std::string faction::faction_noun_crime[15];
std::string faction::faction_noun_cult[15];
std::string faction::faction_noun_none[15];
faction_value_datum faction::facgoal_data[NUM_FACGOALS];
faction_value_datum faction::facjob_data[NUM_FACJOBS];
faction_value_datum faction::facval_data[NUM_FACVALS];

//TODO move them to json
void game::init_faction_data()
{
    std::string tmp_pos[] = {
        _("Shining"), _("Sacred"), _("Golden"), _("Holy"), _("Righteous"), _("Devoted"),
        _("Virtuous"), _("Splendid"), _("Divine"), _("Radiant"), _("Noble"), _("Venerable"),
        _("Immaculate"), _("Heroic"), _("Bright")
    };
    for(int i = 0; i < 15; i++) {
        faction::faction_adj_pos[i] = tmp_pos[i];
    }

    std::string tmp_neu[] = {
        _("Original"), _("Crystal"), _("Metal"), _("Mighty"), _("Powerful"), _("Solid"),
        _("Stone"), _("Firey"), _("Colossal"), _("Famous"), _("Supreme"), _("Invincible"),
        _("Unlimited"), _("Great"), _("Electric")
    };
    for(int i = 0; i < 15; i++) {
        faction::faction_adj_neu[i] = tmp_neu[i];
    }

    std::string tmp_bad[] = {
        _("Poisonous"), _("Deadly"), _("Foul"), _("Nefarious"), _("Wicked"), _("Vile"),
        _("Ruinous"), _("Horror"), _("Devastating"), _("Vicious"), _("Sinister"), _("Baleful"),
        _("Pestilent"), _("Pernicious"), _("Dread")
    };
    for(int i = 0; i < 15; i++) {
        faction::faction_adj_bad[i] = tmp_bad[i];
    }

    std::string tmp_strong[] = {
        _("Fists"), _("Slayers"), _("Furies"), _("Dervishes"), _("Tigers"), _("Destroyers"),
        _("Berserkers"), _("Samurai"), _("Valkyries"), _("Army"), _("Killers"), _("Paladins"),
        _("Knights"), _("Warriors"), _("Huntsmen")
    };
    for(int i = 0; i < 15; i++) {
        faction::faction_noun_strong[i] = tmp_strong[i];
    }

    std::string tmp_sneak[] = {
        _("Snakes"), _("Rats"), _("Assassins"), _("Ninja"), _("Agents"), _("Shadows"),
        _("Guerillas"), _("Eliminators"), _("Snipers"), _("Smoke"), _("Arachnids"), _("Creepers"),
        _("Shade"), _("Stalkers"), _("Eels")
    };
    for(int i = 0; i < 15; i++) {
        faction::faction_noun_sneak[i] = tmp_sneak[i];
    }

    std::string tmp_crime[] = {
        _("Bandits"), _("Punks"), _("Family"), _("Mafia"), _("Mob"), _("Gang"), _("Vandals"),
        _("Sharks"), _("Muggers"), _("Cutthroats"), _("Guild"), _("Faction"), _("Thugs"),
        _("Racket"), _("Crooks")
    };
    for(int i = 0; i < 15; i++) {
        faction::faction_noun_crime[i] = tmp_crime[i];
    }

    std::string tmp_cult[] = {
        _("Brotherhood"), _("Church"), _("Ones"), _("Crucible"), _("Sect"), _("Creed"),
        _("Doctrine"), _("Priests"), _("Tenet"), _("Monks"), _("Clerics"), _("Pastors"),
        _("Gnostics"), _("Elders"), _("Inquisitors")
    };
    for(int i = 0; i < 15; i++) {
        faction::faction_noun_cult[i] = tmp_cult[i];
    }

    std::string tmp_none[] = {
        _("Settlers"), _("People"), _("Men"), _("Faction"), _("Tribe"), _("Clan"), _("Society"),
        _("Folk"), _("Nation"), _("Republic"), _("Colony"), _("State"), _("Kingdom"), _("Party"),
        _("Company")
    };
    for(int i = 0; i < 15; i++) {
        faction::faction_noun_none[i] = tmp_none[i];
    }


    faction_value_datum tmp_goal[] = {
        // "Their ultimate goal is <name>"
        //Name                               Good Str Sneak Crime Cult
        {"Null",                             0,   0,  0,    0,    0},
        {_("basic survival"),                0,   0,  0,    0,    0},
        {_("financial wealth"),              0,  -1,  0,    2,   -1},
        {_("dominance of the region"),      -1,   1, -1,    1,   -1},
        {_("the extermination of monsters"), 1,   3, -1,   -1,   -1},
        {_("contact with unseen powers"),   -1,   0,  1,    0,    4},
        {_("bringing the apocalypse"),      -5,   1,  2,    0,    7},
        {_("general chaos and anarchy"),    -3,   2, -3,    2,   -1},
        {_("the cultivation of knowledge"),  2,  -3,  2,   -1,    0},
        {_("harmony with nature"),           2,  -2,  0,   -1,    2},
        {_("rebuilding civilization"),       2,   1, -2,   -2,   -4},
        {_("spreading the fungus"),         -2,   1,  1,    0,    4}
    };
    // TOTAL:                               -5    3  -2     0     7
    for(int i = 0; i < NUM_FACGOALS; i++) {
        faction::facgoal_data[i] = tmp_goal[i];
    }

    faction_value_datum tmp_job[] = {
        // "They earn money via <name>"
        //Name                              Good Str Sneak Crime Cult
        {"Null",                            0,   0,  0,    0,    0},
        {_("protection rackets"),          -3,   2, -1,    4,    0},
        {_("the sale of information"),     -1,  -1,  4,    1,    0},
        {_("their bustling trade centers"), 1,  -1, -2,   -4,   -4},
        {_("trade caravans"),               2,  -1, -1,   -3,   -2},
        {_("scavenging supplies"),          0,  -1,  0,   -1,   -1},
        {_("mercenary work"),               0,   3, -1,    1,   -1},
        {_("assassinations"),              -1,   2,  2,    1,    1},
        {_("raiding settlements"),         -4,   4, -3,    3,   -2},
        {_("the theft of property"),       -3,  -1,  4,    4,    1},
        {_("gambling parlors"),            -1,  -2, -1,    1,   -1},
        {_("medical aid"),                  4,  -3, -2,   -3,    0},
        {_("farming & selling food"),       3,  -4, -2,   -4,    1},
        {_("drug dealing"),                -2,   0, -1,    2,    0},
        {_("selling manufactured goods"),   1,   0, -1,   -2,    0}
    };
    // TOTAL:                              -5   -3  -5     0    -6
    for(int i = 0; i < NUM_FACJOBS; i++) {
        faction::facjob_data[i] = tmp_job[i];
    }

    faction_value_datum tmp_val[] = {
        // "They are known for <name>"
        //Name                            Good Str Sneak Crime Cult
        {"Null",                          0,   0,  0,    0,    0},
        {_("their charitable nature"),    5,  -1, -1,   -2,   -2},
        {_("their isolationism"),         0,  -2,  1,    0,    2},
        {_("exploring extensively"),      1,   0,  0,   -1,   -1},
        {_("collecting rare artifacts"),  0,   1,  1,    0,    3},
        {_("their knowledge of bionics"), 1,   2,  0,    0,    0},
        {_("their libraries"),            1,  -3,  0,   -2,    1},
        {_("their elite training"),       0,   4,  2,    0,    2},
        {_("their robotics factories"),   0,   3, -1,    0,   -2},
        {_("treachery"),                 -3,   0,  1,    3,    0},
        {_("the avoidance of drugs"),     1,   0,  0,   -1,    1},
        {_("their adherance to the law"), 2,  -1, -1,   -4,   -1},
        {_("their cruelty"),             -3,   1, -1,    4,    1}
    };
    // TOTALS:                            5    4   1    -3     4
    for(int i = 0; i < NUM_FACVALS; i++) {
        faction::facval_data[i] = tmp_val[i];
    }
    /* Note: It's nice to keep the totals around 0 for Good, and about even for the
     * other four.  It's okay if Good is slightly negative (after all, in a post-
     * apocalyptic world people might be a LITTLE less virtuous), and to keep
     * strength valued a bit higher than the others.
     */
}

void faction::load_info(std::string data)
{
    std::stringstream dump;
    int valuetmp, goaltmp, jobtmp1, jobtmp2;
    int omx, omy;
    dump << data;
    dump >> id >> valuetmp >> goaltmp >> jobtmp1 >> jobtmp2 >> likes_u >>
         respects_u >> known_by_u >> strength >> sneak >> crime >> cult >>
         good >> omx >> omy >> mapx >> mapy >> size >> power >> combat_ability >>
         food_supply >> wealth;
    // Make mapx/mapy global coordinate
    mapx += omx * OMAPX * 2;
    mapy += omy * OMAPY * 2;
    values = valuetmp;
    goal = faction_goal(goaltmp);
    job1 = faction_job(jobtmp1);
    job2 = faction_job(jobtmp2);
    int tmpsize, tmpop;
    dump >> tmpsize;
    for (int i = 0; i < tmpsize; i++) {
        dump >> tmpop;
        opinion_of.push_back(tmpop);
    }
    std::string subdesc;
    while (dump >> subdesc) {
        desc += " " + subdesc;
    }

    std::string subname;
    while (dump >> subname) {
        name += " " + subname;
    }
}

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
    } while((one_in(num_values) || one_in(num_values)) && tries < 15);

    std::string noun;
    int sel = 1, best = strength;
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
        noun  = faction_noun_strong[rng(0, 14)];
        power = dice(5, 20);
        size  = dice(5, 6);
        break;
    case 2:
        noun  = faction_noun_sneak [rng(0, 14)];
        power = dice(5, 8);
        size  = dice(5, 8);
        break;
    case 3:
        noun  = faction_noun_crime [rng(0, 14)];
        power = dice(5, 16);
        size  = dice(5, 8);
        break;
    case 4:
        noun  = faction_noun_cult  [rng(0, 14)];
        power = dice(8, 8);
        size  = dice(4, 6);
        break;
    default:
        noun  = faction_noun_none  [rng(0, 14)];
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
                adj = faction_adj_pos[rng(0, 14)];
            } else if  (good <= -3) {
                adj = faction_adj_bad[rng(0, 14)];
            } else {
                adj = faction_adj_neu[rng(0, 14)];
            }
            name = string_format(_("The %1$s %2$s"), adj.c_str(), noun.c_str());
            if (one_in(4)) {
                name = string_format(_("%1$s of %2$s"), name.c_str(), invent_name().c_str());
            }
        } while (utf8_width(name) > MAX_FAC_NAME_SIZE);
    }
}

void faction::make_army()
{
    name = _("The army");
    mapx = OMAPX / 2;
    mapy = OMAPY / 2;
    size = OMAPX * 2;
    power = OMAPX;
    goal = FACGOAL_DOMINANCE;
    job1 = FACJOB_MERCENARIES;
    job2 = FACJOB_NULL;
    if (one_in(4)) {
        values |= mfb(FACVAL_CHARITABLE);
    }
    if (!one_in(4)) {
        values |= mfb(FACVAL_EXPLORATION);
    }
    if (one_in(3)) {
        values |= mfb(FACVAL_BIONICS);
    }
    if (one_in(3)) {
        values |= mfb(FACVAL_ROBOTS);
    }
    if (one_in(4)) {
        values |= mfb(FACVAL_TREACHERY);
    }
    if (one_in(4)) {
        values |= mfb(FACVAL_STRAIGHTEDGE);
    }
    if (!one_in(3)) {
        values |= mfb(FACVAL_LAWFUL);
    }
    if (one_in(8)) {
        values |= mfb(FACVAL_CRUELTY);
    }
    id = "army";
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
    std::string ret;
    ret = desc + "\n \n" + string_format( _("%1$s have the ultimate goal of %2$s."), name.c_str(),
                                          facgoal_data[goal].name.c_str());
    if (job2 == FACJOB_NULL) {
        ret += string_format( _(" Their primary concern is %s."), facjob_data[job1].name.c_str());
    } else {
        ret += string_format( _(" Their primary concern is %1$s, but they are also involved in %2$s."),
                              facjob_data[job1].name.c_str(),
                              facjob_data[job2].name.c_str());
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
        return has_value( val ) ? facval_data[val].name : "";
    } );
    if( !known_vals.empty() ) {
        ret += _( " They are known for " ) + known_vals + ".";
    }
    return ret;
}

int faction::response_time() const
{
    int base = abs(mapx - g->get_levx());
    if (abs(mapy - g->get_levy()) > base) {
        base = abs(mapy - g->get_levy());
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
    std::string ret,  tmp;
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
        tmp = "";
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

    // Disrepected, laughed at, etc.
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
    return _("Destitue");
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
