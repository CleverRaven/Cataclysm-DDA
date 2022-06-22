#include "loadout.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <memory>

#include "game.h"
#include "addiction.h"
#include "avatar.h"
#include "calendar.h"
#include "debug.h"
#include "flag.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "json.h"
#include "magic.h"
#include "mission.h"
#include "options.h"
#include "pimpl.h"
#include "profession.h"
#include "translations.h"
#include "type_id.h"
#include "visitable.h"

namespace
{
    generic_factory<loadout> all_loadouts("loadout");
} //namespace
static class json_item_substitution
{
public:
    void reset();
    void load(const JsonObject& jo);
    void check_consistency();

private:
    struct trait_requirements {
        explicit trait_requirements(const JsonObject& obj);
        trait_requirements() = default;
        std::vector<trait_id> present;
        std::vector<trait_id> absent;
        bool meets_condition(const std::vector<trait_id>& traits) const;
    };
    struct substitution {
        trait_requirements trait_reqs;
        struct info {
            explicit info(const JsonValue& value);
            info() = default;
            itype_id new_item;
            double ratio = 1.0; // new charges / old charges
        };
        std::vector<info> infos;
    };
    std::map<itype_id, std::vector<substitution>> substitutions;
    std::vector<std::pair<item_group_id, trait_requirements>> bonuses;
public:
    std::vector<item> get_bonus_items(const std::vector<trait_id>& traits) const;
    std::vector<item> get_substitution(const item& it, const std::vector<trait_id>& traits) const;
} item_substitutions;

/** @relates string_id */
template<>
const loadout& string_id<loadout>::obj() const
{
    return all_loadouts.obj(*this);
}

/** @relates string_id */
template<>
bool string_id<loadout>::is_valid() const
{
    return all_loadouts.is_valid(*this);
}

loadout::loadout()
    : _allow_hobbies(),
    _replace_initial()
{
}

void loadout::load_loadout(const JsonObject& jo, const std::string& src)
{
    all_loadouts.load(jo, src);
}

class addiction_reader : public generic_typed_reader<addiction_reader>
{
public:
    addiction get_next(JsonValue& jv) const {
        JsonObject jo = jv.get_object();
        return addiction(addiction_id(jo.get_string("type")), jo.get_int("intensity"));
    }
    template<typename C>
    void erase_next(std::string&& type_str, C& container) const {
        const addiction_id type(type_str);
        reader_detail::handler<C>().erase_if(container, [&type](const addiction& e) {
            return e.type == type;
            });
    }
};

class item_reader : public generic_typed_reader<item_reader>
{
public:
    loadout::itypedec get_next(JsonValue& jv) const {
        // either a plain item type id string, or an array with item type id
        // and as second entry the item description.
        if (jv.test_string()) {
            return loadout::itypedec(jv.get_string());
        }
        JsonArray jarr = jv.get_array();
        const auto id = jarr.get_string(0);
        const snippet_id snippet(jarr.get_string(1));
        return loadout::itypedec(id, snippet);
    }
    template<typename C>
    void erase_next(std::string&& id, C& container) const {
        reader_detail::handler<C>().erase_if(container, [&id](const loadout::itypedec& e) {
            return e.type_id.str() == id;
            });
    }
};

void loadout::load(const JsonObject& jo, const std::string&)
{
    //If the "name" is an object then we have to deal with gender-specific titles,
    if (jo.has_object("name")) {
        JsonObject name_obj = jo.get_object("name");
    }
    else if (jo.has_string("name")) {
        // Same profession names for male and female in English.
        // Still need to different names in other languages.
        const std::string name = jo.get_string("name");
    }
    else if (!was_loaded) {
        jo.throw_error("missing mandatory member \"name\"");
    }

    if (!was_loaded || jo.has_member("description")) {
        std::string desc;
        std::string desc_male;
        std::string desc_female;

        bool use_default_description = true;
        if (jo.has_object("description")) {
            JsonObject desc_obj = jo.get_object("description");
            desc_obj.allow_omitted_members();

            if (desc_obj.has_member("male") && desc_obj.has_member("female")) {
                use_default_description = false;
                mandatory(desc_obj, false, "male", desc_male, text_style_check_reader());
                mandatory(desc_obj, false, "female", desc_female, text_style_check_reader());
            }
        }

        if (use_default_description) {
            mandatory(jo, false, "description", desc, text_style_check_reader());
            desc_male = desc;
            desc_female = desc;
        }
    }
    if (jo.has_string("allow_hobbies")) {
        bool allow_hobbies;
        if (jo.get_string("allow_hobbies") == "false") {
            allow_hobbies = false;
        }
        else
        {
            allow_hobbies = true;
        }
    }
    if (jo.has_string("replace_initial")) {
        bool replace_initial;
        if (jo.get_string("replace_initial") == "false") {
            replace_initial = false;
        }
        else
        {
            replace_initial = true;
        }
    }

    if (jo.has_string("vehicle")) {
        _starting_vehicle = vproto_id(jo.get_string("vehicle"));
    }
    if (jo.has_array("pets")) {
        for (JsonObject subobj : jo.get_array("pets")) {
            int count = subobj.get_int("amount");
            mtype_id mon = mtype_id(subobj.get_string("name"));
            for (int start = 0; start < count; ++start) {
                _starting_pets.push_back(mon);
            }
        }
    }

    if (jo.has_array("spells")) {
        for (JsonObject subobj : jo.get_array("spells")) {
            int level = subobj.get_int("level");
            spell_id sp = spell_id(subobj.get_string("id"));
            _starting_spells.emplace(sp, level);
        }
    }

    mandatory(jo, was_loaded, "points", _point_cost);

    if (!was_loaded || jo.has_member("items")) {
        std::string c = "items for loadout " + id.str();
        JsonObject items_obj = jo.get_object("items");

        if (items_obj.has_array("both")) {
            optional(items_obj, was_loaded, "both", legacy_starting_items, item_reader{});
        }
        if (items_obj.has_object("both")) {
            _starting_items = item_group::load_item_group(
                items_obj.get_member("both"), "collection", c);
        }
        if (items_obj.has_array("male")) {
            optional(items_obj, was_loaded, "male", legacy_starting_items_male, item_reader{});
        }
        if (items_obj.has_object("male")) {
            _starting_items_male = item_group::load_item_group(
                items_obj.get_member("male"), "collection", c);
        }
        if (items_obj.has_array("female")) {
            optional(items_obj, was_loaded, "female", legacy_starting_items_female, item_reader{});
        }
        if (items_obj.has_object("female")) {
            _starting_items_female = item_group::load_item_group(items_obj.get_member("female"),
                "collection", c);
        }
    }
    optional(jo, was_loaded, "no_bonus", no_bonus);

    optional(jo, was_loaded, "addictions", _starting_addictions, addiction_reader{});
    // TODO: use string_id<bionic_type> or so
    optional(jo, was_loaded, "CBMs", _starting_CBMs, string_id_reader<::bionic_data> {});
    optional(jo, was_loaded, "flags", flags, auto_flags_reader<> {});

    // Flag which denotes if a profession is a hobby
    optional(jo, was_loaded, "subtype", _subtype, "");
    optional(jo, was_loaded, "missions", _missions, string_id_reader<::mission_type> {});
}

const loadout* loadout::generic()
{
    const string_id<loadout> generic_loadout_id(
        get_option<std::string>("GENERIC_LOADOUT_ID"));
    return &generic_loadout_id.obj();
}

const std::vector<loadout>& loadout::get_all()
{
    return all_loadouts.get_all();
}

std::vector<string_id<loadout>> loadout::get_all_hobbies()
{
    std::vector<loadout> all = loadout::get_all();
    std::vector<loadout_id> ret;

    // remove all non-hobbies from list of professions
    const auto new_end = std::remove_if(all.begin(),
        all.end(), [&](const loadout& arg) {
            return !arg.is_hobby();
        });
    all.erase(new_end, all.end());

    // convert to string_id's then return
    for (const loadout& p : all) {
        ret.emplace(ret.end(), p.ident());
    }
    return ret;
}

void loadout::reset()
{
    all_loadouts.reset();
    item_substitutions.reset();
}

void loadout::check_definitions()
{
    item_substitutions.check_consistency();
    for (const auto& loadouts : all_loadouts.get_all()) {
        loadouts.check_definition();
    }
}

void loadout::check_item_definitions(const itypedecvec& items) const
{
    for (const auto& itd : items) {
        if (!item::type_is_defined(itd.type_id)) {
            debugmsg("loadout %s: item %s does not exist", id.str(), itd.type_id.str());
        }
        else if (!itd.snip_id.is_null()) {
            const itype* type = item::find_type(itd.type_id);
            if (type->snippet_category.empty()) {
                debugmsg("loadout %s: item %s has no snippet category - no description can "
                    "be set", id.str(), itd.type_id.str());
            }
            else {
                if (!itd.snip_id.is_valid()) {
                    debugmsg("loadout %s: there's no snippet with id %s",
                        id.str(), itd.snip_id.str());
                }
            }
        }
    }
}

void loadout::check_definition() const
{
    check_item_definitions(legacy_starting_items);
    check_item_definitions(legacy_starting_items_female);
    check_item_definitions(legacy_starting_items_male);
    if (!no_bonus.is_empty() && !item::type_is_defined(no_bonus)) {
        debugmsg("no_bonus item '%s' is not an itype_id", no_bonus.c_str());
    }

    if (!item_group::group_is_defined(_starting_items)) {
        debugmsg("_starting_items group is undefined");
    }
    if (!item_group::group_is_defined(_starting_items_male)) {
        debugmsg("_starting_items_male group is undefined");
    }
    if (!item_group::group_is_defined(_starting_items_female)) {
        debugmsg("_starting_items_female group is undefined");
    }
    if (_starting_vehicle && !_starting_vehicle.is_valid()) {
        debugmsg("vehicle prototype %s for loadout %s does not exist", _starting_vehicle.c_str(),
            id.c_str());
    }
    for (const auto& a : _starting_CBMs) {
        if (!a.is_valid()) {
            debugmsg("bionic %s for loadout %s does not exist", a.c_str(), id.c_str());
        }
    }
    for (const auto& elem : _starting_pets) {
        if (!elem.is_valid()) {
            debugmsg("starting pet %s for loadout %s does not exist", elem.c_str(), id.c_str());
        }
    }
    for (const auto& elem : _starting_skills) {
        if (!elem.first.is_valid()) {
            debugmsg("skill %s for loadout %s does not exist", elem.first.c_str(), id.c_str());
        }
    }

    for (const auto& m : _missions) {
        if (!m.is_valid()) {
            debugmsg("starting mission %s for loadout %s does not exist", m.c_str(), id.c_str());
        }

        if (std::find(m->origins.begin(), m->origins.end(), ORIGIN_GAME_START) == m->origins.end()) {
            debugmsg("starting mission %s for loadout %s must include an origin of ORIGIN_GAME_START",
                m.c_str(), id.c_str());
        }
    }
}

bool loadout::has_initialized()
{
    if (!g || g->new_game) {
        return false;
    }
    const string_id<loadout> generic_loadout_id(
        get_option<std::string>("GENERIC_LOADOUT_ID"));
    return generic_loadout_id.is_valid();
}

const string_id<loadout>& loadout::ident() const
{
    return id;
}

std::string loadout::name() const
{
    return _name.translated();
}

std::string loadout::description() const
{
    return _description.translated();
}

static time_point advanced_spawn_time()
{
    return calendar::start_of_game;
}

signed int loadout::point_cost() const
{
    return _point_cost;
}

static void clear_faults(item& it)
{
    if (it.get_var("dirt", 0) > 0) {
        it.set_var("dirt", 0);
    }
    if (it.is_faulty()) {
        it.faults.clear();
    }
}

std::list<item> loadout::items(bool male, const std::vector<trait_id>& traits) const
{
    std::list<item> result;
    auto add_legacy_items = [&result](const itypedecvec& vec) {
        for (const itypedec& elem : vec) {
            item it(elem.type_id, advanced_spawn_time(), item::default_charges_tag{});
            if (!elem.snip_id.is_null()) {
                it.set_snippet(elem.snip_id);
            }
            it = it.in_its_container();
            result.push_back(it);
        }
    };

    add_legacy_items(legacy_starting_items);
    add_legacy_items(male ? legacy_starting_items_male : legacy_starting_items_female);

    std::vector<item> group_both = item_group::items_from(_starting_items,
        advanced_spawn_time());
    std::vector<item> group_gender = item_group::items_from(male ? _starting_items_male :
        _starting_items_female, advanced_spawn_time());
    result.insert(result.begin(), std::make_move_iterator(group_both.begin()),
        std::make_move_iterator(group_both.end()));
    result.insert(result.begin(), std::make_move_iterator(group_gender.begin()),
        std::make_move_iterator(group_gender.end()));

    if (!has_flag("NO_BONUS_ITEMS")) {
        const std::vector<item>& items = item_substitutions.get_bonus_items(traits);
        for (const item& it : items) {
            if (it.typeId() != no_bonus) {
                result.push_back(it);
            }
        }
    }
    for (auto iter = result.begin(); iter != result.end(); ) {
        const auto sub = item_substitutions.get_substitution(*iter, traits);
        if (!sub.empty()) {
            result.insert(result.begin(), sub.begin(), sub.end());
            iter = result.erase(iter);
        }
        else {
            ++iter;
        }
    }
    for (item& it : result) {
        it.visit_items([](item* it, item*) {
            clear_faults(*it);
            return VisitResponse::NEXT;
            });
        if (it.has_flag(flag_VARSIZE)) {
            it.set_flag(flag_FIT);
        }
    }

    if (result.empty()) {
        // No need to do the below stuff. Plus it would cause said below stuff to crash
        return result;
    }

    // Merge charges for items that stack with each other
    for (auto outer = result.begin(); outer != result.end(); ++outer) {
        if (!outer->count_by_charges()) {
            continue;
        }
        for (auto inner = std::next(outer); inner != result.end(); ) {
            if (outer->stacks_with(*inner)) {
                outer->merge_charges(*inner);
                inner = result.erase(inner);
            }
            else {
                ++inner;
            }
        }
    }

    result.sort([](const item& first, const item& second) {
        return first.get_layer() < second.get_layer();
        });
    return result;
}

vproto_id loadout::vehicle() const
{
    return _starting_vehicle;
}

std::vector<mtype_id> loadout::pets() const
{
    return _starting_pets;
}

std::vector<addiction> loadout::addictions() const
{
    return _starting_addictions;
}

std::vector<bionic_id> loadout::CBMs() const
{
    return _starting_CBMs;
}

bool loadout::has_flag(const std::string& flag) const
{
    return flags.count(flag) != 0;
}

bool loadout::can_pick(const Character& you, const int points) const
{
    return point_cost() - you.prof->point_cost() <= points;
}

bool loadout::allow_hobbies() const
{
    return _allow_hobbies;
}

bool loadout::replace_initial() const
{
    return _replace_initial;
}

std::map<spell_id, int> loadout::spells() const
{
    return _starting_spells;
}

void loadout::learn_spells(avatar& you) const
{
    for (const std::pair<spell_id, int> spell_pair : spells()) {
        you.magic->learn_spell(spell_pair.first, you, true);
        spell& sp = you.magic->get_spell(spell_pair.first);
        while (sp.get_level() < spell_pair.second && !sp.is_max_level()) {
            sp.gain_level();
        }
    }
}

// item_substitution stuff:

void loadout::load_item_substitutions(const JsonObject& jo)
{
    item_substitutions.load(jo);
}

bool loadout::is_hobby() const
{
    return _subtype == "hobby";
}

const std::vector<mission_type_id>& loadout::missions() const
{
    return _missions;
}
