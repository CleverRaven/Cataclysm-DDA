#ifndef _JSON_H_
#define _JSON_H_

#include <string>
#include <vector>
#include <istream>
#include <map>
#include <set>

/* Cataclysm-DDA homegrown JSON tools
 * copyright CC-BY-SA-3.0 2013 CleverRaven
 * 
 * Consists of four main JSON manipulation tools:
 * JsonIn - for low-level parsing of an input JSON stream
 * JsonObject - convenience-wrapper for reading JSON objects from a JsonIn
 * JsonArray - convenience-wrapper for reading JSON arrays from a JsonIn
 * JsonOut - for outputting JSON
 * 
 * Generally usage in code will be based around a JsonObject or JsonArray.
 * 
 * 
 * JsonObject and JsonArray
 * ========================
 * 
 * Typical usage for a JsonObject will be something like the following:
 * 
 *     JsonObject jo(&jsin);
 *     std::string id = jo.get_string("id");
 *     std::string name = _(jo.get_string("name").c_str());
 *     std::string description = _(jo.get_string("description").c_str());
 *     int points = jo.get_int("points", 0);
 *     std::set<std::string> tags = jo.get_tags("flags");
 *     my_object_type myobject(id, name, description, points, tags);
 * 
 * Here the "id", "name" and "description" members are required.
 * JsonObject will throw a std::string if they are not found,
 * identifying the problem and the current position in the input stream.
 * 
 * Note that "name" and "description" are passed to gettext for translating.
 * Any members with string information that is displayed directly to the user
 * will need similar treatment, and may also need to be explicitly handled
 * in lang/extract_json_strings.py to be translatable.
 * 
 * The "points" member here is not required.
 * If it is not found in the incoming JSON object,
 * it will be initialized with the default value given as the second parameter,
 * in this case, 0.
 * 
 * get_tags() always returns an empty set if the member is not found,
 * and so does not require a default value to be specified.
 * 
 * 
 * Arrays can be iterated over, or accessed directly by position.
 * Typical iterative usage is as follows:
 * 
 *     JsonArray ja = jo.get_array("some_array_member");
 *     std::vector<int> myarray;
 *     while (ja.has_more()) {
 *         myarray.push_back(ja.next_int());
 *     }
 * 
 * If the array member is not found, get_array() returns an empty array,
 * and has_more() is always false, so myarray will remain empty.
 * 
 * next_int() and the other next_* methods advance the array iterator,
 * so after the final element has been read,
 * has_more() will return false and the loop will terminate.
 * 
 * If the next element is not an integer,
 * JsonArray will throw a std::string indicating the problem,
 * and the position in the input stream.
 * 
 * To handle arrays with elements of indeterminate type,
 * the test_* methods can be used, calling next_* according to the result.
 * 
 * 
 * Positional array access uses get_* methods as follows:
 * 
 *     JsonArray ja = jo.get_array("xydata");
 *     point xydata(ja.get_int(0), ja.get_int(1));
 * 
 * Arrays also provide has_int(index) etc., for positional type testing.
 * 
 * 
 * JsonObject can also test for member type with has_int(name) etc.,
 * and for member existence with has_member(name).
 * 
 * 
 * These two classes, JsonObject and JsonArray,
 * provide the most convenient access method for incoming JSON.
 * 
 * Usage of the lower-level JsonIn requires more care,
 * and so should be avoided unless performance is a proven concern.
 * 
 * 
 * JsonIn
 * ======
 * 
 * The JsonIn class is intended to provide a wrapper around a std::istream,
 * with methods for reading JSON directly from the stream.
 * 
 * The JsonObject and JsonArray classes are wrappers around a JsonIn.
 * JsonArray stores only the byte offset of each element,
 * and iterates or accesses according to these offsets.
 * JsonObject maps member names to the byte offset of the paired value.
 * 
 * Both of them provide data by seeking the stream to the relevant position,
 * and calling the correct JsonIn method to read the value from the stream.
 * 
 * As such, when processing items using a JsonIn,
 * care must be taken to move the stream offset to the end of the item
 * after usage has finished.
 * This can be done by calling the .finish() method,
 * after the item has been dealt with, and before the stream continues.
 * 
 * Typical usage might be something like the following:
 * 
 *     JsonIn jsin(&myistream);
 *     // expecting an array of objects
 *     jsin.start_array(); // throws std::string if array not found
 *     while (!jsin.end_array()) { // end_array returns false if not the end
 *         JsonObject jo = jsin.get_object();
 *         ... // load object as above
 *         jo.finish();
 *     }
 * 
 * The array could have been loaded into a JsonArray for convenience.
 * Not doing so saves one full pass of the data,
 * as the element positions don't have to be read,
 * but requires any JsonObject and JsonArrays that might have been accessed
 * to be .finish()ed before the next iteration,
 * so as to move the stream cursor to the correct position.
 * 
 * If a JsonObject is used inside a JsonIn and not .finish()ed,
 * it /will/ cause bugs.
 * 
 * 
 * A JsonIn can also be used for single-pass loading,
 * by passing off members as they arrive according to their names.
 * 
 * Typical usage might be:
 * 
 *     JsonIn jsin(&myistream);
 *     // expecting an array of objects, to be mapped by id
 *     std::map<std::string,my_data_type> myobjects;
 *     jsin.start_array();
 *     while (!jsin.end_array()) {
 *         my_data_type myobject;
 *         jsin.start_object();
 *         while (!jsin.end_object()) {
 *             std::string name = jsin.get_member_name();
 *             if (name == "id") {
 *                 myobject.id = jsin.get_string();
 *             } else if (name == "name") {
 *                 myobject.name = _(jsin.get_string().c_str());
 *             } else if (name == "description") {
 *                 myobject.description = _(jsin.get_string().c_str());
 *             } else if (name == "points") {
 *                 myobject.points = jsin.get_int();
 *             } else if (name == "flags") {
 *                 if (jsin.test_string()) { // load single string tag
 *                     myobject.tags.insert(jsin.get_string());
 *                 } else { // load tag array
 *                     jsin.start_array();
 *                     while (!jsin.end_array()) {
 *                         myobject.tags.insert(jsin.get_string());
 *                     }
 *                 }
 *             } else {
 *                 jsin.skip_value();
 *             }
 *         }
 *         myobjects[myobject.id] = myobject;
 *     }
 * 
 * The get_* methods automatically verify and skip separators and whitespace.
 * 
 * If the JSON structure is not as expected,
 * verbose error messages are provided, indicating the problem,
 * and the exact line number and byte offset within the istream.
 * 
 * Using this method, unrecognized members are silently ignored,
 * allowing for things like "comment" members.
 * 
 * As every item must be handled somehow,
 * unrecognized member values must always be explicitly skipped.
 * 
 * If an if;else if;... is missing the "else", it /will/ cause bugs.
 * 
 * 
 * JsonOut
 * =======
 * 
 * Design in progress.
 * 
 */

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
