#include "ammo.h"
#include "debug.h"
#include "json.h"
#include "item.h"

#include <unordered_map>

namespace {
using ammo_map_t = std::unordered_map<std::string, ammunition_type>;

ammo_map_t& all_ammunition_types() {
    static ammo_map_t the_map;
    return the_map;
}
} //namespace

ammunition_type::ammunition_type()
  : name_("null")
{
}

ammunition_type::ammunition_type(std::string name, std::string default_ammotype)
  : name_(std::move(name)), default_ammotype_(std::move(default_ammotype))
{
}

void ammunition_type::load_ammunition_type(JsonObject &jsobj)
{
    auto const result = all_ammunition_types().insert(std::make_pair(
        jsobj.get_string("id"),
        ammunition_type(jsobj.get_string("name"), jsobj.get_string("default"))));

    if (!result.second) {
        debugmsg("duplicate ammo id: %s", result.first->first.c_str());
    }
}

ammunition_type const& ammunition_type::find_ammunition_type(std::string const &ident)
{
    auto const& the_map = all_ammunition_types();

    auto const it = the_map.find(ident);
    if (it != the_map.end()) {
        return it->second;
    }

    debugmsg("Tried to get invalid ammunition: %s", ident.c_str());
    static ammunition_type const null_ammunition;
    return null_ammunition;
}

void ammunition_type::reset()
{
    all_ammunition_types().clear();
}

void ammunition_type::check_consistency()
{
    for (const auto &ammo : all_ammunition_types()) {
        auto const &id = ammo.first;
        auto const &at = ammo.second.default_ammotype_;

        // TODO: these ammo types should probably not have default ammo at all.
        if (at == "UPS" || at == "components" || at == "thrown") {
            continue;
        }

        if (!at.empty() && !item::type_is_defined(at)) {
            debugmsg("ammo type %s has invalid default ammo %s", id.c_str(), at.c_str());
        }
    }
}
