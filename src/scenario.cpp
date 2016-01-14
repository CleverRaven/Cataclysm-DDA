#include <iostream>
#include <fstream>
#include <sstream>

#include "scenario.h"

#include "debug.h"
#include "json.h"
#include "player.h"
#include "bionics.h"
#include "start_location.h"
#include "game.h"
#include "map.h"
#include "translations.h"
#include "pldata.h"
#include "addiction.h"
#include "skill.h"
#include "profession.h"
#include "mutation.h"
#include "mapgen.h"

scenario::scenario()
   : _ident(""), _name_male("null"), _name_female("null"),
     _description_male("null"), _description_female("null")
{
}

scenmap scenario::_all_scens;

void scenario::load_scenario(JsonObject &jsobj)
{
    scenario scen;
    JsonArray jsarr;

    scen._ident = jsobj.get_string("ident");
    //If the "name" is an object then we have to deal with gender-specific titles,
    if(jsobj.has_object("name")) {
        JsonObject name_obj=jsobj.get_object("name");
        scen._name_male = pgettext("scenario_male", name_obj.get_string("male").c_str());
        scen._name_female = pgettext("scenario_female", name_obj.get_string("female").c_str());
    }
    else {
        // Same scenario names for male and female in English.
        // Still need to different names in other languages.
        const std::string name = jsobj.get_string("name");
        scen._name_female = pgettext("scenario_female", name.c_str());
        scen._name_male = pgettext("scenario_male", name.c_str());
    }

    const std::string desc = jsobj.get_string("description").c_str();
    scen._description_male = pgettext("scen_desc_male", desc.c_str());
    scen._description_female = pgettext("scen_desc_female", desc.c_str());

    const std::string stame = jsobj.get_string("start_name").c_str();
    scen._start_name = pgettext("start_name", stame.c_str());


    scen._point_cost = jsobj.get_int("points");

    JsonObject items_obj=jsobj.get_object("items");
    scen.add_items_from_jsonarray(items_obj.get_array("both"), "both");
    scen.add_items_from_jsonarray(items_obj.get_array("male"), "male");
    scen.add_items_from_jsonarray(items_obj.get_array("female"), "female");

    bool first = false;
    jsarr = jsobj.get_array("professions");
    while (jsarr.has_more()) {
        if (first == true){
            scen._allowed_professions.insert(jsarr.next_string());
        }
        else{
            scen._profession = jsarr.next_string();
            scen._allowed_professions.insert(scen._profession);
            first = true;
        }
    }
    if (scen._allowed_professions.size() < 1){
        scen._profession = profession::generic()->ident();
    }
    jsarr = jsobj.get_array("traits");
    while (jsarr.has_more()) {
        scen._allowed_traits.insert(jsarr.next_string());
    }
    jsarr = jsobj.get_array("forced_traits");
    while (jsarr.has_more()) {
        scen._forced_traits.insert(jsarr.next_string());
    }
    jsarr = jsobj.get_array("forbidden_traits");
    while (jsarr.has_more()) {
        scen._forbidden_traits.insert(jsarr.next_string());
    }
    jsarr = jsobj.get_array("allowed_locs");
    while (jsarr.has_more()) {
        if (scen._default_loc.size() < 1){
            scen._default_loc = jsarr.next_string();
            scen._allowed_locs.insert(scen._default_loc);
        }
        else{
            scen._allowed_locs.insert(jsarr.next_string());
        }
    }
    jsarr = jsobj.get_array("flags");
    while (jsarr.has_more()) {
        scen.flags.insert(jsarr.next_string());
    }
    scen._map_special = jsobj.get_string( "map_special", "mx_null" );

    _all_scens[scen._ident] = scen;
    DebugLog( D_INFO, DC_ALL ) << "Loaded scenario: " << scen._ident;
}

const scenario *scenario::scen(std::string ident)
{
    scenmap::iterator scen = _all_scens.find(ident);
    if (scen != _all_scens.end())
    {
        return &(scen->second);
    }
    else
    {
        debugmsg("Tried to get invalid scenario: %s", ident.c_str());
        return NULL;
    }
}

const scenario *scenario::generic()
{
    return scenario::scen("evacuee");
}

// Strategy: a third of the time, return the generic scenario.  Otherwise, return a scenario,
// weighting 0 cost scenario more likely--the weight of a scenario with cost n is 2/(|n|+2),
// e.g., cost 1 is 2/3rds as likely, cost -2 is 1/2 as likely.
const scenario *scenario::weighted_random()
{
    if (one_in(3)) {
        return generic();
    } else {
        const scenario* retval = 0;
        while(retval == 0) {
            scenmap::iterator iter = _all_scens.begin();
            for (int i = rng(0, _all_scens.size() - 1); i > 0; --i) {
                ++iter;
            }
            if (x_in_y(2, abs(iter->second.point_cost()) + 2)) {
                retval = &(iter->second);
            }  // else reroll in the while loop.
        }
        return retval;
    }
}

bool scenario::exists(std::string ident)
{
    return _all_scens.find(ident) != _all_scens.end();
}

scenmap::const_iterator scenario::begin()
{
    return _all_scens.begin();
}

scenmap::const_iterator scenario::end()
{
    return _all_scens.end();
}

int scenario::count()
{
    return _all_scens.size();
}

void scenario::reset()
{
    _all_scens.clear();
}

void scenario::check_definitions()
{
    for (scenmap::const_iterator a = _all_scens.begin(); a != _all_scens.end(); ++a) {
        a->second.check_definition();
    }
}

void check_traits( const std::set<std::string> &traits, const std::string &ident )
{
    for( auto &t : traits ) {
        if( !mutation_branch::has( t ) ) {
            debugmsg( "trait %s for scenario %s does not exist", t.c_str(), ident.c_str() );
        }
    }
}

void scenario::check_definition() const
{
    for (std::vector<std::string>::const_iterator a = _starting_items.begin(); a != _starting_items.end(); ++a) {
        if( !item::type_is_defined( *a ) ) {
            debugmsg("item %s for scenario %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    for (std::vector<std::string>::const_iterator a = _starting_items_female.begin(); a != _starting_items_female.end(); ++a) {
        if( !item::type_is_defined( *a ) ) {
            debugmsg("item %s for scenario %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    for (std::vector<std::string>::const_iterator a = _starting_items_male.begin(); a != _starting_items_male.end(); ++a) {
        if( !item::type_is_defined( *a ) ) {
            debugmsg("item %s for scenario %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    check_traits( _allowed_traits, _ident );
    check_traits( _forced_traits, _ident );
    check_traits( _forbidden_traits, _ident );
    MapExtras::get_function( _map_special ); // triggers a debug message upon invalid input
}

bool scenario::has_initialized()
{
    return exists("evacuee");
}

void scenario::add_items_from_jsonarray(JsonArray jsarr, std::string gender)
{
    while (jsarr.has_more()) {
        add_item(jsarr.next_string(), gender);
    }
}

void scenario::add_item(std::string item, std::string gender)
{
    if(gender=="male") {
        _starting_items_male.push_back(item);
    }
    else if(gender=="female") {
        _starting_items_female.push_back(item);
    }
    else {
        _starting_items.push_back(item);
    }
}


std::string scenario::ident() const
{
    return _ident;
}

std::string scenario::gender_appropriate_name(bool male) const
{
    if(male) {
        return _name_male;
    }
    else {
        return _name_female;
    }
}

std::string scenario::description(bool male) const
{
    if(male) {
        return _description_male;
    }
    else {
        return _description_female;
    }
}

signed int scenario::point_cost() const
{
    return _point_cost;
}

std::string scenario::start_location() const
{
    return _default_loc;
}
std::string scenario::random_start_location() const
{
   return random_entry( _allowed_locs, start_location() );
}
const profession* scenario::get_profession() const
{
    return profession::prof(_profession);;
}
const profession* scenario::random_profession() const
{
    std::vector<std::string> allowed_professions(_allowed_professions.begin(), _allowed_professions.end());
    if (allowed_professions.size() == 0) {
        return profession::generic();
    }
    return profession::prof(allowed_professions[rng(0, allowed_professions.size()-1)]);
}
std::string scenario::start_name() const
{
    return _start_name;
}
int scenario::mission() const
{
    return _mission;
}
std::vector<std::string> scenario::items() const
{
    return _starting_items;
}

std::vector<std::string> scenario::items_male() const
{
    return _starting_items_male;
}

std::vector<std::string> scenario::items_female() const
{
    return _starting_items_female;
}
bool scenario::profquery(const profession* proff) const
{
    return _allowed_professions.count(proff->ident()) != 0;
}
bool scenario::traitquery(std::string trait) const
{
    return _allowed_traits.count(trait) != 0;
}
bool scenario::locked_traits(std::string trait) const
{
    return _forced_traits.count(trait) != 0;
}
bool scenario::forbidden_traits(std::string trait) const
{
    return _forbidden_traits.count(trait) != 0;
}
int scenario::profsize() const
{
    return _allowed_professions.size();
}
bool scenario::has_flag(std::string flag) const
{
    return flags.count(flag) != 0;
}
bool scenario::allowed_start(std::string loc) const
{
    return _allowed_locs.count(loc) != 0;
}
bool scenario::can_pick(int points) const
{
    if (point_cost() - g->scen->point_cost() > points) {
        return false;
    }

    return true;
}
bool scenario::has_map_special() const
{
    return _map_special != "mx_null";
}
const std::string& scenario::get_map_special() const
{
    return _map_special;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:fdl=0:
