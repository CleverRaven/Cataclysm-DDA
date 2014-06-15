#include "start_location.h"
#include "output.h"

static location_map _locations;

start_location::start_location() {
    _ident = "";
    _name = "null";
    _target = "shelter";
}

start_location::start_location( std::string ident, std::string name,
                                std::string target ) {
    _ident = ident;
    _name = name;
    _target = target;
}

std::string start_location::ident() const {
    return _ident;
}

std::string start_location::name() const {
    return _name;
}

std::string start_location::target() const {
    return _target;
}

location_map::iterator start_location::begin() {
    return _locations.begin();
}

location_map::iterator start_location::end() {
    return _locations.end();
}

start_location *start_location::find( const std::string ident ) {
    location_map::iterator found = _locations.find( ident );
    if(found != _locations.end()){
        return &(found->second);
    } else {
        debugmsg("Tried to get invalid location: %s", ident.c_str());
        static start_location null_location;
        return &null_location;
    }
}

void start_location::load_location( JsonObject &jsonobj ) {
    start_location new_location;

    new_location._ident = jsonobj.get_string("ident");
    new_location._name = jsonobj.get_string("name");
    new_location._target = jsonobj.get_string("target");

    _locations[new_location._ident] = new_location;
}
