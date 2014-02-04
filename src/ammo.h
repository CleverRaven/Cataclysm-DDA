#ifndef _AMMO_H_
#define _AMMO_H_

#include <string>
#include <map>

#include "json.h"

class ammunition_type;

typedef std::map<std::string, ammunition_type> ammunition_type_map;

class ammunition_type
{
private:
    std::string _ident;
    std::string _name;
    std::string _default_ammotype;

    static ammunition_type_map _all_ammunition_type;

public:
    ammunition_type();
    ammunition_type(std::string ident, std::string name, std::string default_ammotype);
    static void load_ammunition_type(JsonObject &jsobj);

    // functions
    static ammunition_type* find_ammunition_type(std::string ident);
    // clear ammotype map, every ammotype pointer becames invalid!
    static void reset();

    std::string name() const;
    std::string default_ammotype() const;
};

#endif // _AMMO_H_
