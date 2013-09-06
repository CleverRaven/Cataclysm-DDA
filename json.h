#ifndef _JSON_H_
#define _JSON_H_

#include <string>
#include <vector>
#include <istream>

bool is_whitespace(char ch);

class Jsin {
private:
    std::istream *stream;
    int saved_pos;

public:
    Jsin(std::istream *stream);

    void save_pos(); // store stream position
    void load_pos(); // load stored stream position
    char peek(); // what's the next char gonna be?
    bool good(); // whether stream is ok

    void eat_whitespace();

    std::string find_type(); // find an object's "type".

    void skip_member();
    void skip_pair_separator();
    void skip_string();
    void skip_value();
    void skip_object();
    void skip_array();
    void skip_true();
    void skip_false();
    void skip_null();
    void skip_number();
    void skip_separator();

    std::string get_string(); // get the next value as a string
    int get_int(); // get the next value as an int
    bool get_bool(); // get the next value as a bool
    double get_float(); // get the next value as a double
    std::string get_member_name(); // also strips the ':'

    bool start_array(); // verify array start
    bool end_array(); // returns false if it's not the end
    bool start_object();
    bool end_object(); // returns false if it's not the end
};

#endif // _JSON_H_
