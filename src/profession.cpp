#include <iostream>
#include <fstream>
#include <sstream>

#include "profession.h"

#include "debug.h"
#include "json.h"
#include "player.h"
#include "bionics.h"
#include "mutation.h"
#include "text_snippets.h"
#include "rng.h"
#include "translations.h"
#include "skill.h"
#include "addiction.h"
#include "pldata.h"
#include "itype.h"

template<>
const profession &string_id<profession>::obj() const
{
    const auto prof = profession::_all_profs.find( *this );
    if( prof != profession::_all_profs.end() ) {
        return prof->second;
    } else {
        debugmsg( "Tried to get invalid profession: %s", c_str() );
        static const profession dummy{};
        return dummy;
    }
}

template<>
bool string_id<profession>::is_valid() const
{
    return profession::_all_profs.count( *this ) > 0;
}

const string_id<profession> generic_profession_id( "unemployed" );

profession::profession()
    : _ident(""), _name_male("null"), _name_female("null"),
      _description_male("null"), _description_female("null"), _point_cost(0)
{
}

profmap profession::_all_profs;

void profession::load_profession(JsonObject &jsobj)
{
    profession prof;
    JsonArray jsarr;

    prof._ident = string_id<profession>( jsobj.get_string( "ident" ) );
    //If the "name" is an object then we have to deal with gender-specific titles,
    if(jsobj.has_object("name")) {
        JsonObject name_obj = jsobj.get_object("name");
        prof._name_male = pgettext("profession_male", name_obj.get_string("male").c_str());
        prof._name_female = pgettext("profession_female", name_obj.get_string("female").c_str());
    } else {
        // Same profession names for male and female in English.
        // Still need to different names in other languages.
        const std::string name = jsobj.get_string("name");
        prof._name_female = pgettext("profession_female", name.c_str());
        prof._name_male = pgettext("profession_male", name.c_str());
    }

    const std::string desc = jsobj.get_string("description").c_str();
    prof._description_male = pgettext("prof_desc_male", desc.c_str());
    prof._description_female = pgettext("prof_desc_female", desc.c_str());

    prof._point_cost = jsobj.get_int("points");

    JsonObject items_obj = jsobj.get_object("items");
    prof.add_items_from_jsonarray(items_obj.get_array("both"), "both");
    prof.add_items_from_jsonarray(items_obj.get_array("male"), "male");
    prof.add_items_from_jsonarray(items_obj.get_array("female"), "female");

    jsarr = jsobj.get_array("skills");
    while (jsarr.has_more()) {
        JsonObject jo = jsarr.next_object();
        prof.add_skill(skill_id( jo.get_string("name") ),
                       jo.get_int("level"));
    }
    jsarr = jsobj.get_array("addictions");
    while (jsarr.has_more()) {
        JsonObject jo = jsarr.next_object();
        prof.add_addiction(addiction_type(jo.get_string("type")),
                           jo.get_int("intensity"));
    }
    jsarr = jsobj.get_array("CBMs");
    while (jsarr.has_more()) {
        prof.add_CBM(jsarr.next_string());
    }
    jsarr = jsobj.get_array("traits");
    while (jsarr.has_more()) {
        prof.add_trait(jsarr.next_string());
    }
    jsarr = jsobj.get_array("flags");
    while (jsarr.has_more()) {
        prof.flags.insert(jsarr.next_string());
    }

    _all_profs[prof._ident] = prof;
    DebugLog( D_INFO, DC_ALL ) << "Loaded profession: " << prof._ident.str();
}

const profession *profession::generic()
{
    return &generic_profession_id.obj();
}

// Strategy: a third of the time, return the generic profession.  Otherwise, return a profession,
// weighting 0 cost professions more likely--the weight of a profession with cost n is 2/(|n|+2),
// e.g., cost 1 is 2/3rds as likely, cost -2 is 1/2 as likely.
const profession *profession::weighted_random()
{
    if (one_in(3)) {
        return generic();
    } else {
        const profession *retval = 0;
        while(retval == 0) {
            profmap::iterator iter = _all_profs.begin();
            for (int i = rng(0, _all_profs.size() - 1); i > 0; --i) {
                ++iter;
            }
            if (x_in_y(2, abs(iter->second.point_cost()) + 2) && !(iter->second.has_flag("SCEN_ONLY"))) {
                retval = &(iter->second);
            }  // else reroll in the while loop.
        }
        return retval;
    }
}

profmap::const_iterator profession::begin()
{
    return _all_profs.begin();
}

profmap::const_iterator profession::end()
{
    return _all_profs.end();
}

int profession::count()
{
    return _all_profs.size();
}

void profession::reset()
{
    _all_profs.clear();
}

void profession::check_definitions()
{
    for (profmap::const_iterator a = _all_profs.begin(); a != _all_profs.end(); ++a) {
        a->second.check_definition();
    }
}

void profession::check_item_definitions( const itypedecvec &items ) const
{
    for( auto & itd : items ) {
        if( !item::type_is_defined( itd.type_id ) ) {
            debugmsg( "profession %s: item %s does not exist", _ident.c_str() , itd.type_id.c_str() );
        } else if( !itd.snippet_id.empty() ) {
            const itype *type = item::find_type( itd.type_id );
            if( type->snippet_category.empty() ) {
                debugmsg( "profession %s: item %s has no snippet category - no description can be set",
                          _ident.c_str(), itd.type_id.c_str() );
            } else {
                const int hash = SNIPPET.get_snippet_by_id( itd.snippet_id );
                if( SNIPPET.get( hash ).empty() ) {
                    debugmsg( "profession %s: snippet id %s for item %s is not contained in snippet category %s",
                              _ident.c_str(), itd.snippet_id.c_str(), itd.type_id.c_str(), type->snippet_category.c_str() );
                }
            }
        }
    }
}

void profession::check_definition() const
{
    check_item_definitions( _starting_items );
    check_item_definitions( _starting_items_female );
    check_item_definitions( _starting_items_male );
    for (auto const &a : _starting_CBMs) {
        if (!is_valid_bionic(a)) {
            debugmsg("bionic %s for profession %s does not exist", a.c_str(), _ident.c_str());
        }
    }

    for( auto &t : _starting_traits ) {
        if( !mutation_branch::has( t ) ) {
            debugmsg( "trait %s for profession %s does not exist", t.c_str(), _ident.c_str() );
        }
    }

    for( const auto &elem : _starting_skills ) {
        if( !elem.first.is_valid() ) {
            debugmsg( "skill %s for profession %s does not exist", elem.first.c_str(), _ident.c_str() );
        }
    }
}

bool profession::has_initialized()
{
    return generic_profession_id.is_valid();
}

void profession::add_items_from_jsonarray(JsonArray jsarr, std::string gender)
{
    while (jsarr.has_more()) {
        // either a plain item type id string, or an array with item type id
        // and as second entry the item description.
        if( jsarr.test_array() ) {
            auto arr = jsarr.next_array();
            const itypedec entry( arr.get_string( 0 ),
                                  _( arr.get_string( 1 ).c_str() ) );
            add_item( entry, gender );
        } else {
            add_item( itypedec( jsarr.next_string(), "" ), gender );
        }
    }
}

void profession::add_item(const itypedec &entry, const std::string &gender)
{
    if(gender == "male") {
        _starting_items_male.push_back( entry );
    } else if(gender == "female") {
        _starting_items_female.push_back( entry );
    } else {
        _starting_items.push_back( entry );
    }
}

void profession::add_CBM(std::string CBM)
{
    _starting_CBMs.push_back(CBM);
}

void profession::add_trait(std::string trait)
{
    _starting_traits.push_back(trait);
}

void profession::add_addiction(add_type type, int intensity)
{
    _starting_addictions.push_back(addiction(type, intensity));
}
void profession::add_skill(const skill_id &skill_name, const int level)
{
    _starting_skills.push_back(StartingSkill(skill_name, level));
}

const string_id<profession> &profession::ident() const
{
    return _ident;
}

std::string profession::gender_appropriate_name(bool male) const
{
    if(male) {
        return _name_male;
    } else {
        return _name_female;
    }
}

std::string profession::description(bool male) const
{
    if(male) {
        return _description_male;
    } else {
        return _description_female;
    }
}

signed int profession::point_cost() const
{
    return _point_cost;
}

profession::itypedecvec profession::items(bool male) const
{
    auto result = _starting_items;
    const auto &gender_items = male ? _starting_items_male : _starting_items_female;
    result.insert( result.begin(), gender_items.begin(), gender_items.end() );
    return result;
}

std::vector<addiction> profession::addictions() const
{
    return _starting_addictions;
}

std::vector<std::string> profession::CBMs() const
{
    return _starting_CBMs;
}

std::vector<std::string> profession::traits() const
{
    return _starting_traits;
}

const profession::StartingSkillList profession::skills() const
{
    return _starting_skills;
}

bool profession::has_flag(std::string flag) const
{
    return flags.count(flag) != 0;
}

bool profession::can_pick(player *u, int points) const
{
    if (point_cost() - u->prof->point_cost() > points) {
        return false;
    }

    return true;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:fdl=0:
