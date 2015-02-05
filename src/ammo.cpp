#include "ammo.h"

#include "debug.h"
#include "json.h"
#include "translations.h"
#include "debug.h"
#include "item.h"

#include <string>

ammunition_type::ammunition_type()
{
    _ident = "";
    _name = "null";
    _default_ammotype = "";
}

ammunition_type::ammunition_type(std::string ident, std::string name, std::string default_ammotype)
{
    _ident = ident;
    _name = name;
    _default_ammotype = default_ammotype;
}

ammunition_type_map ammunition_type::_all_ammunition_type;

void ammunition_type::load_ammunition_type(JsonObject &jsobj)
{
    ammunition_type amtype;

    amtype._ident = jsobj.get_string("id");
    amtype._name = _(jsobj.get_string("name").c_str());
    amtype._default_ammotype = jsobj.get_string("default");
    _all_ammunition_type[amtype._ident] = amtype;
}

ammunition_type *ammunition_type::find_ammunition_type(std::string ident)
{
    ammunition_type_map::iterator found = _all_ammunition_type.find(ident);
    if(found != _all_ammunition_type.end()) {
        return &(found->second);
    } else {
        debugmsg("Tried to get invalid ammunition: %s", ident.c_str());
        static ammunition_type null_ammunition;
        return &null_ammunition;
    }
}

void ammunition_type::reset()
{
    _all_ammunition_type.clear();
}

std::string ammunition_type::name() const
{
    return _name;
}

std::string ammunition_type::default_ammotype() const
{
    return _default_ammotype;
}

void ammunition_type::check_consistency()
{
    for( const auto &ammo : _all_ammunition_type ) {
        // TODO: these ammo types should probably not have default ammo at all.
        if( ammo.second._default_ammotype == "UPS" ||
            ammo.second._default_ammotype == "components" ||
            ammo.second._default_ammotype == "thrown" ) {
            continue;
        }
        if( !ammo.second._default_ammotype.empty() &&
            !item::type_is_defined( ammo.second._default_ammotype ) ) {
            debugmsg( "ammo type %s has invalid default ammo %s", ammo.second._ident.c_str(),
                      ammo.second._default_ammotype.c_str() );
        }
    }
}
