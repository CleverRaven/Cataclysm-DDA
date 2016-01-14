#ifndef SCENARIO_H
#define SCENARIO_H

#include "string_id.h"

#include <string>
#include <vector>
#include <map>
#include <set>

class scenario;
class profession;
class player;
class JsonArray;
class JsonObject;
class addiction;
enum add_type : int;

typedef std::map<std::string, scenario> scenmap;

class scenario
{

private:
    std::string _ident;
    std::string _name_male;
    std::string _name_female;
    std::string _description_male;
    std::string _description_female;
    std::string _gender_req;
    std::string _start_name;
    std::string _default_loc;
    string_id<profession> _profession;
    std::set<string_id<profession>> _allowed_professions;
    std::set<std::string> _allowed_traits;
    std::set<std::string> _forced_traits;
    std::set<std::string> _forbidden_traits;
    std::set<std::string> _allowed_locs;
    int _mission;
    std::vector<std::string> traits;
    int _point_cost;
//Gender variations left in for translation purposes
    std::vector<std::string> _starting_items;
    std::vector<std::string> _starting_items_male;
    std::vector<std::string> _starting_items_female;
    std::set<std::string> flags; // flags for some special properties of the scenario
    std::string _map_special;

    void add_items_from_jsonarray(JsonArray jsarr, std::string gender);
    void add_item(std::string item, std::string gender);


    static scenmap _all_scens;
public:
    //these three aren't meant for external use, but had to be made public regardless
    scenario();
    static void load_scenario(JsonObject &jsobj);

    // these should be the only ways used to get at scenario
    static const scenario* scen(std::string ident);
    static const scenario* generic(); // points to the generic, default profession
    // return a random scenario, weighted for use w/ random character creation
    static const scenario* weighted_random();
    static bool exists(std::string ident);
    static scenmap::const_iterator begin();
    static scenmap::const_iterator end();
    static int count();

    static bool has_initialized();
    // clear scenario map, every scenario pointer becames invalid!
    static void reset();
    /** calls @ref check_definition for each scenario */
    static void check_definitions();
    /** Check that item definitions are valid */
    void check_definition() const;

    std::string ident() const;
    std::string gender_appropriate_name(bool male) const;
    std::string description(bool male) const;
    std::string gender_req() const;
    std::string start_location() const;
    std::string random_start_location() const;
    std::string start_name() const;
    const profession* get_profession() const;
    const profession* random_profession() const;
    bool profquery(const profession* proff) const;
    bool traitquery(std::string trait) const;
    bool locked_traits(std::string trait) const;
    bool forbidden_traits(std::string trait) const;
    bool allowed_start(std::string loc) const;
    int profsize() const;
    int mission() const;
    signed int point_cost() const;
    std::vector<std::string> items() const;
    std::vector<std::string> items_male() const;
    std::vector<std::string> items_female() const;
    bool has_map_special() const;
    const std::string& get_map_special() const;


    /**
     * Check if this type of scenario has a certain flag set.
     *
     * Current flags: none
     */
    bool has_flag(std::string flag) const;

    /**
     *
     */
    bool can_pick(int points) const;

};

#endif
