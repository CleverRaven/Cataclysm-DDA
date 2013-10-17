#ifndef _JSON_H_
#define _JSON_H_

#include <string>
#include <vector>
#include <istream>
#include <map>
#include <set>

class JsonIn;
class JsonObject;
class JsonArray;

bool is_whitespace(char ch);

class JsonObject {
private:
    std::map<std::string, int> positions;
    int start;
    int end;
    JsonIn *jsin;

    int verify_position(std::string &name, bool throw_exception=true);

public:
    JsonObject(JsonIn *jsin);

    void finish(); // moves the stream to the end of the object

    bool has_member(std::string name); // true iff named member exists

    // variants with no fallback throw an error if the name is not found.
    // variants with a fallback return the fallback value in stead.
    bool get_bool(std::string name);
    bool get_bool(std::string name, bool fallback);
    int get_int(std::string name);
    int get_int(std::string name, int fallback);
    double get_float(std::string name);
    double get_float(std::string name, double fallback);
    std::string get_string(std::string name);
    std::string get_string(std::string name, std::string fallback);

    JsonArray get_array(std::string name); // returns empty array if not found
    JsonObject get_object(std::string name);
    std::set<std::string> get_tags(std::string name); // returns empty set if not found

    // type checking
    bool has_null(std::string name);
    bool has_bool(std::string name);
    bool has_int(std::string name);
    bool has_float(std::string name);
    bool has_number(std::string name) { return has_float(name); };
    bool has_string(std::string name);
    bool has_array(std::string name);
    bool has_object(std::string name);

    // useful debug info
    std::string line_number(); // for occasional use only
};

class JsonArray {
private:
    std::vector<int> positions;
    int start;
    int index;
    JsonIn *jsin;

    void verify_index(int i);

public:
    JsonArray(JsonIn *jsin);
    JsonArray() {};

    bool has_more(); // true iff more elements may be retrieved with next_*
    int size();

    // iterative access
    bool next_bool();
    int next_int();
    double next_float();
    std::string next_string();
    JsonArray next_array();
    JsonObject next_object();

    // static access
    bool get_bool(int index);
    int get_int(int index);
    double get_float(int index);
    std::string get_string(int index);
    JsonArray get_array(int index);
    JsonObject get_object(int index);
};

class JsonIn {
private:
    std::istream *stream;

public:
    JsonIn(std::istream *stream);

    int tell(); // get current stream position
    void seek(int pos); // seek to specified stream position
    char peek(); // what's the next char gonna be?
    bool good(); // whether stream is ok

    // advance seek head to the next non-whitespace character
    void eat_whitespace();

    // quick skipping for when values don't have to be parsed
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

    // data parsing
    std::string get_string(); // get the next value as a string
    int get_int(); // get the next value as an int
    bool get_bool(); // get the next value as a bool
    double get_float(); // get the next value as a double
    std::string get_member_name(); // also strips the ':'
    JsonObject get_object() { return JsonObject(this); };
    JsonArray get_array() { return JsonArray(this); };

    // container control and iteration
    void start_array(); // verify array start
    bool end_array(); // returns false if it's not the end
    void start_object();
    bool end_object(); // returns false if it's not the end

    // type testing
    bool test_null();
    bool test_bool();
    bool test_int();
    bool test_float();
    bool test_number() { return test_float(); };
    bool test_string();
    bool test_array();
    bool test_object();

    // useful debug info
    std::string line_number(int offset_modifier=0); // for occasional use only
};

#endif // _JSON_H_
