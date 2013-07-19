#ifndef _CATAJSON_H_
#define _CATAJSON_H_

#include "picojson.h"
#include <set>

class catajson
{
public:
    catajson();
    catajson(std::string path, bool is_static = false);
    catajson(picojson::value val, std::string path_msg);

    // accessors for picojson values containing simple types
    char as_char() const;
    std::string as_string() const;
    int as_int() const;
    bool as_bool() const;
    double as_double() const;
    // accessor for not-so-simple type
    std::set<std::string> as_tags();
    // accessors in the case of array or object values
    catajson get(std::string key) const;
    catajson get(int index) const;

    //these check whether a given key/index is valid
    bool has(std::string key) const;
    bool has(int index) const;

    bool is_char() const;
    bool is_string() const;
    bool is_number() const; // double OR int
    bool is_bool() const;
    bool is_object() const; // string-value pairs
    bool is_array() const;

    // these are intended for use in iteration
    // STL-style iterators are acceptable too, if anyone wants to add them
    void set_begin(); // start iterating from the beginning
    void set_end(); // start iterating from the end
    void next(); // caller's responsibility to init first
    void prev(); // caller's responsibility to init first
    bool has_curr() const; // would curr() be valid?
    catajson curr() const; // get the current element

private:
    picojson::value val;
    picojson::array::const_iterator ar_iter;
    picojson::object::const_iterator obj_iter;
    std::string path_msg; // used for error messages
};

#define json_good() picojson::get_last_error().empty()

#endif
