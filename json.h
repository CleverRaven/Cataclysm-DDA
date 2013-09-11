#ifndef _JSON_H_
#define _JSON_H_

#include <string>
#include <vector>
#include <istream>
#include <map>

class Jsin;
class Jsobj;
class Jsarr;

bool is_whitespace(char ch);

class Jsobj {
private:
    std::map<std::string, int> positions;
    int start;
    int end;
    Jsin *jsin;
public:
    Jsobj(Jsin *jsin);

    void finish();

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

    Jsarr get_array(std::string name); // returns empty array if not found
    //Jsobj get_object(std::string name);
};

class Jsarr {
private:
    std::vector<int> positions;
    int start;
    int pos;
    Jsin *jsin;
public:
    Jsarr(Jsin *jsin);
    Jsarr() {};

    // iterative access
    bool has_more(); // true iff more elements may be retrieved with next_*
    bool next_bool();
    int next_int();
    double next_float();
    std::string next_string();
    Jsarr next_array();
    Jsobj next_object();

    // static access
    //int size();
    //bool get_bool(int pos);
    //int get_int(int pos);
    //double get_float(int pos);
    //std::string get_string(int pos);
    //Jsarr get_array(int pos);
    //Jsobj get_object(int pos);
};

class Jsin {
private:
    std::istream *stream;

public:
    Jsin(std::istream *stream);

    int tell(); // get current stream position
    void seek(int pos); // seek to specified stream position
    char peek(); // what's the next char gonna be?
    bool good(); // whether stream is ok

    void eat_whitespace();

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
    Jsobj get_object() { return Jsobj(this); };
    Jsarr get_array() { return Jsarr(this); };

    bool start_array(); // verify array start
    bool end_array(); // returns false if it's not the end
    bool start_object();
    bool end_object(); // returns false if it's not the end
};

#endif // _JSON_H_
