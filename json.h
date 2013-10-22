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
    int verify_position(const std::string &name,
                        const bool throw_exception=true);

public:
    JsonObject(JsonIn *jsin);
    JsonObject(const JsonObject &jsobj);

    void finish(); // moves the stream to the end of the object

    bool has_member(const std::string &name); // true iff named member exists

    // variants with no fallback throw an error if the name is not found.
    // variants with a fallback return the fallback value in stead.
    bool get_bool(const std::string &name);
    bool get_bool(const std::string &name, const bool fallback);
    int get_int(const std::string &name);
    int get_int(const std::string &name, const int fallback);
    double get_float(const std::string &name);
    double get_float(const std::string &name, const double fallback);
    std::string get_string(const std::string &name);
    std::string get_string(const std::string &name, const std::string &fallback);

    // note: returns empty array if not found
    JsonArray get_array(const std::string &name);
    // note: throws exception if not found (can change behaviour if desirable)
    JsonObject get_object(const std::string &name);
    // note: returns empty set if not found
    std::set<std::string> get_tags(const std::string &name);
    // TODO: some sort of get_map(), maybe

    // type checking
    bool has_null(const std::string &name);
    bool has_bool(const std::string &name);
    bool has_int(const std::string &name);
    bool has_float(const std::string &name);
    bool has_number(const std::string &name) { return has_float(name); };
    bool has_string(const std::string &name);
    bool has_array(const std::string &name);
    bool has_object(const std::string &name);

    // useful debug info
    std::string line_number(); // for occasional use only

    // sets and gets validation mode // todo constructor arg
    bool strict;

    // dump substring of input, for error messages and copying
    std::string dump_input();
};

class JsonArray {
private:
    std::vector<int> positions;
    int start;
    int index;
    int end;
    JsonIn *jsin;
    void verify_index(int i);

public:
    JsonArray(JsonIn *jsin);
    JsonArray(const JsonArray &jsarr);
    JsonArray() : positions(), start(0), index(0), jsin(NULL) {};

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

    // iterative type checking
    bool test_null();
    bool test_bool();
    bool test_int();
    bool test_float();
    bool test_number() { return test_float(); };
    bool test_string();
    bool test_array();
    bool test_object();

    // random-access type checking
    bool has_null(int index);
    bool has_bool(int index);
    bool has_int(int index);
    bool has_float(int index);
    bool has_number(int index) { return has_float(index); };
    bool has_string(int index);
    bool has_array(int index);
    bool has_object(int index);

    // sets and gets validation mode // todo constructor arg
    bool strict;

    // dump substring of input, for error messages and copying
    std::string dump_input();
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
    bool skip_member();
    bool skip_pair_separator();
    bool skip_string();
    bool skip_value();
    bool skip_object();
    bool skip_array();
    bool skip_true();
    bool skip_false();
    bool skip_null();
    bool skip_number();
    bool skip_separator();

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

    // sets and gets validation mode // todo constructor arg
    bool strict;

    // raw read of string, for dump_input
    void read(char * str, int len);
};

#endif // _JSON_H_
