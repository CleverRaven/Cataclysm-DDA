#pragma once
#ifndef CATA_SRC_LOADOUT_H
#define CATA_SRC_LOADOUT_H

#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "translations.h"
#include "type_id.h"

class JsonObject;
class addiction;
class avatar;
class item;
class Character;
template<typename T>
class generic_factory;

class loadout
{
public:
    using StartingSkill = std::pair<skill_id, int>;
    using StartingSkillList = std::vector<StartingSkill>;
    struct itypedec {
        itype_id type_id;
        /** Snippet id, @see snippet_library. */
        snippet_id snip_id;
        // compatible with when this was just a std::string
        explicit itypedec(const std::string& t) :
            type_id(t), snip_id(snippet_id::NULL_ID()) {
        }
        itypedec(const std::string& t, const snippet_id& d) : type_id(t), snip_id(d) {
        }
    };
    using itypedecvec = std::vector<itypedec>;
    friend class string_id<loadout>;
    friend class generic_factory<loadout>;
    friend struct mod_tracker;

private:
    string_id<loadout> id;
    std::vector<std::pair<string_id<loadout>, mod_id>> src;
    bool was_loaded = false;

    signed int _point_cost = 0;
    translation _description;
    translation _name;

    // TODO: In professions.json, replace lists of itypes (legacy) with item groups
    itypedecvec legacy_starting_items;
    itypedecvec legacy_starting_items_male;
    itypedecvec legacy_starting_items_female;
    item_group_id _starting_items = item_group_id("EMPTY_GROUP");
    item_group_id _starting_items_male = item_group_id("EMPTY_GROUP");
    item_group_id _starting_items_female = item_group_id("EMPTY_GROUP");
    itype_id no_bonus; // See loadout::items and class json_item_substitution in loadout.cpp

    std::vector<addiction> _starting_addictions;
    std::vector<bionic_id> _starting_CBMs;
    std::vector<mtype_id> _starting_pets;
    vproto_id _starting_vehicle = vproto_id::NULL_ID();
    bool _allow_hobbies;
    bool _replace_initial;
    // the int is what level the spell starts at
    std::map<spell_id, int> _starting_spells;
    std::set<std::string> flags; // flags for some special properties of the profession
    StartingSkillList  _starting_skills;
    std::vector<mission_type_id> _missions; // starting missions for profession

    std::string _subtype;

    void check_item_definitions(const itypedecvec& items) const;

    void load(const JsonObject& jo, const std::string& src);

public:
    loadout();

    static void load_loadout(const JsonObject& jo, const std::string& src);
    static void load_item_substitutions(const JsonObject& jo);
    
    // these should be the only ways used to get at loadouts
    static const loadout* generic(); // points to the generic, default profession
    static const std::vector<loadout>& get_all();
    static std::vector<string_id<loadout>> get_all_hobbies();

    static bool has_initialized();
    // clear profession map, every profession pointer becomes invalid!
    static void reset();
    /** calls @ref check_definition for each profession */
    static void check_definitions();
    /** Check that item/CBM/addiction/skill definitions are valid. */
    void check_definition() const;

    const string_id<loadout>& ident() const;
    std::string name() const;
    std::string description() const;
    signed int point_cost() const;
    std::list<item> items(bool male, const std::vector<trait_id>& traits) const;
    std::vector<addiction> addictions() const;
    vproto_id vehicle() const;
    std::vector<mtype_id> pets() const;
    std::vector<bionic_id> CBMs() const;
    std::map<spell_id, int> spells() const;
    void learn_spells(avatar& you) const;

    bool has_flag(const std::string& flag) const;
    
    bool can_pick(const Character& you, int points) const;
    bool allow_hobbies() const;
    bool replace_initial() const;
    bool is_locked_loadout(const trait_id& trait) const;
    bool is_forbidden_loadout(const trait_id& trait) const;
    std::vector<trait_id> get_locked_loadouts() const;
    std::set<trait_id> get_forbidden_loadouts() const;
    
    bool is_hobby() const;

    const std::vector<mission_type_id>& missions() const;

};

#endif // CATA_SRC_LOADOUT_H
