#include "json.h"

#include "catacharset.h" // utf32_to_utf8

#include <cstdlib> // strtoul
#include <cstring> // strcmp
#include <cmath> // pow
#include <istream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <locale> // ensure user's locale doesn't interfere with output

/* JSON parsing and serialization tools for Cataclysm-DDA
 * ~
 * These tools are intended to support the following use cases:
 * (1) loading game data from .json files,
 * (2) serializing data to JSON for game saving,
 * (3) loading JSON data from game save files.
 * ~
 * For (1), files are assumed to be either a single object,
 * or an array of objects. Each object must have a "type" member,
 * indicating the internal data type it is intended to represent.
 * Object members should be named in accord with the names used
 * in the equivalent C++ struct or class.
 * ~
 * An example object might look something like the following:
 * {
 *     "type" : "tool",
 *     "id" : "mop",
 *     "name" : "mop",
 *     "description": "An unwieldy mop. Good for cleaning up spills."
 * }
 * ~
 * A central dispatcher (init.cpp) will load each .json file,
 * construct a JsonObject instance to represent each object in the file,
 * then send the JsonObject to the appropriate data constructor,
 * according to its "type" member.
 * ~
 * Object constructors can use the JsonObject class to construct from JSON.
 * The type of each member must be inferrable from the object "type",
 * and each should be parsed expecting the correct datatype.
 * If it fails, it's an error.
 * ~
 * Members not present in JSON should be left with their default values.
 * ~
 * Members named "name", "description", "sound", "text" and "message"
 * will be automatically made available for translation.
 * ~
 * Other members that need translating must be special-cased in
 * lang/extract_json_strings.py.
 * ~
 * For (2), objects will be serialized to a compatible format,
 * for use within player save files.
 * ~
 * For (3), objects will be loaded in a similar fashion to (1).
 * ~
 * For (2) and (3), a similar parser/dispatcher can be used,
 * but the container format needn't be pure JSON.
 */

bool is_whitespace(char ch)
{
    // These are all the valid whitespace characters allowed by RFC 4627.
    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r') {
        return true;
    } else {
        return false;
    }
}


/* class JsonObject
 * represents a JSON object,
 * providing access to the underlying data.
 */
JsonObject::JsonObject(JsonIn *j) : positions()
{
    jsin = j;
    start = jsin->tell();
    // cache the position of the value for each member
    jsin->start_object();
    while (!jsin->end_object()) {
        std::string n = jsin->get_member_name();
        int p = jsin->tell();
        positions[n] = p;
        jsin->skip_value();
    }
    end = jsin->tell();
    final_separator = jsin->get_ate_separator();
}

JsonObject::JsonObject(const JsonObject &jo)
{
    jsin = jo.jsin;
    start = jo.start;
    positions = jo.positions;
    end = jo.end;
    final_separator = jo.final_separator;
}

void JsonObject::finish()
{
    if (jsin && jsin->good()) {
        jsin->seek(end);
        jsin->set_ate_separator(final_separator);
    }
}

size_t JsonObject::size() { return positions.size(); }
bool JsonObject::empty() { return positions.empty(); }

int JsonObject::verify_position(const std::string &name,
                                const bool throw_exception)
{
    int pos = positions[name]; // initialized to 0 if it doesn't exist
    if (pos > start) {
        return pos;
    } else if (throw_exception && !jsin) {
        throw (std::string)"member lookup on empty object: " + name;
    } else if (throw_exception) {
        jsin->seek(start);
        jsin->error("member not found: " + name);
    }
    // 0 is always the opening brace,
    // so it will never indicate a valid member position
    return 0;
}

bool JsonObject::has_member(const std::string &name)
{
    return (bool)verify_position(name, false);
}

std::set<std::string> JsonObject::get_member_names()
{
    std::set<std::string> ret;
    for (std::map<std::string,int>::iterator it = positions.begin();
         it != positions.end(); ++it) {
        ret.insert(it->first);
    }
    return ret;
}

std::string JsonObject::line_number()
{
    jsin->seek(start);
    return jsin->line_number();
}

std::string JsonObject::str()
{
    if (jsin) {
        return jsin->substr(start,end-start);
    } else {
        return "{}";
    }
}

JsonIn* JsonObject::get_raw(const std::string &name)
{
    int pos = verify_position(name);
    jsin->seek(pos);
    return jsin;
}


/* returning values by name */

bool JsonObject::get_bool(const std::string &name)
{
    int pos = verify_position(name);
    jsin->seek(pos);
    return jsin->get_bool();
}

bool JsonObject::get_bool(const std::string &name, const bool fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_bool();
}

int JsonObject::get_int(const std::string &name)
{
    int pos = verify_position(name);
    jsin->seek(pos);
    return jsin->get_int();
}

int JsonObject::get_int(const std::string &name, const int fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_int();
}

double JsonObject::get_float(const std::string &name)
{
    int pos = verify_position(name);
    jsin->seek(pos);
    return jsin->get_float();
}

double JsonObject::get_float(const std::string &name, const double fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_float();
}

std::string JsonObject::get_string(const std::string &name)
{
    int pos = verify_position(name);
    jsin->seek(pos);
    return jsin->get_string();
}

std::string JsonObject::get_string(const std::string &name, const std::string &fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_string();
}

/* returning containers by name */

JsonArray JsonObject::get_array(const std::string &name)
{
    int pos = positions[name];
    if (pos <= start) {
        return JsonArray(); // empty array
    }
    jsin->seek(pos);
    return JsonArray(jsin);
}

std::vector<int> JsonObject::get_int_array(const std::string &name)
{
    JsonArray ja = get_array(name);
    std::vector<int> ret;
    while (ja.has_more()) {
        ret.push_back(ja.next_int());
    }
    return ret;
}

std::vector<std::string> JsonObject::get_string_array(const std::string &name)
{
    JsonArray ja = get_array(name);
    std::vector<std::string> ret;
    while (ja.has_more()) {
        ret.push_back(ja.next_string());
    }
    return ret;
}

JsonObject JsonObject::get_object(const std::string &name)
{
    int pos = positions[name];
    if (pos <= start) {
        return JsonObject(); // empty object
    }
    jsin->seek(pos);
    return jsin->get_object();
}

std::set<std::string> JsonObject::get_tags(const std::string &name)
{
    std::set<std::string> ret;
    int pos = positions[name];
    if (pos <= start) {
        return ret; // empty set
    }
    jsin->seek(pos);
    // allow single string as tag
    if (jsin->test_string()) {
        ret.insert(jsin->get_string());
        return ret;
    }
    // otherwise assume it's an array and error if it isn't.
    JsonArray jsarr = jsin->get_array();
    while (jsarr.has_more()) {
        ret.insert(jsarr.next_string());
    }
    return ret;
}

/* non-fatal member existence and type testing */

bool JsonObject::has_null(const std::string &name)
{
    int pos = verify_position(name, false);
    if (!pos) {
        return false;
    }
    jsin->seek(pos);
    if (jsin->test_null()) {
        return true;
    }
    return false;
}

bool JsonObject::has_bool(const std::string &name)
{
    int pos = verify_position(name, false);
    if (!pos) {
        return false;
    }
    jsin->seek(pos);
    if (jsin->test_bool()) {
        return true;
    }
    return false;
}

bool JsonObject::has_int(const std::string &name)
{
    int pos = verify_position(name, false);
    if (!pos) {
        return false;
    }
    jsin->seek(pos);
    if (jsin->test_int()) {
        return true;
    }
    return false;
}

bool JsonObject::has_float(const std::string &name)
{
    int pos = verify_position(name, false);
    if (!pos) {
        return false;
    }
    jsin->seek(pos);
    if (jsin->test_float()) {
        return true;
    }
    return false;
}

bool JsonObject::has_string(const std::string &name)
{
    int pos = verify_position(name, false);
    if (!pos) {
        return false;
    }
    jsin->seek(pos);
    if (jsin->test_string()) {
        return true;
    }
    return false;
}

bool JsonObject::has_array(const std::string &name)
{
    int pos = verify_position(name, false);
    if (!pos) {
        return false;
    }
    jsin->seek(pos);
    if (jsin->test_array()) {
        return true;
    }
    return false;
}

bool JsonObject::has_object(const std::string &name)
{
    int pos = verify_position(name, false);
    if (!pos) {
        return false;
    }
    jsin->seek(pos);
    if (jsin->test_object()) {
        return true;
    }
    return false;
}

/* non-fatal value setting by reference */

bool JsonObject::read_into(const std::string &name, bool &b)
{
    if (!has_bool(name)) {
        return false;
    }
    b = get_bool(name);
    return true;
}

bool JsonObject::read_into(const std::string &name, int &i)
{
    if (!has_number(name)) {
        return false;
    }
    i = get_int(name);
    return true;
}

bool JsonObject::read_into(const std::string &name, unsigned int &u)
{
    if (!has_number(name)) {
        return false;
    }
    u = get_int(name);
    return true;
}

bool JsonObject::read_into(const std::string &name, float &f)
{
    if (!has_number(name)) {
        return false;
    }
    f = get_float(name);
    return true;
}

bool JsonObject::read_into(const std::string &name, double &d)
{
    if (!has_number(name)) {
        return false;
    }
    d = get_float(name);
    return true;
}

bool JsonObject::read_into(const std::string &name, std::string &s)
{
    if (!has_string(name)) {
        return false;
    }
    s = get_string(name);
    return true;
}

bool JsonObject::read_into(const std::string &name, JsonDeserializer &j)
{
    // can't know what type of json object it will deserialize from,
    // so just try to deserialize, catching any error.
    // TODO: non-verbose flag for JsonIn errors so try/catch is faster here
    int pos = verify_position(name, false);
    if (!pos) {
        return false;
    }
    try {
        jsin->seek(pos);
        j.deserialize(*jsin);
        return true;
    } catch (std::string e) {
        return false;
    }
}


/* class JsonArray
 * represents a JSON array,
 * providing access to the underlying data.
 */
JsonArray::JsonArray(JsonIn *j) : positions()
{
    jsin = j;
    start = jsin->tell();
    index = 0;
    // cache the position of each element
    jsin->start_array();
    while (!jsin->end_array()) {
        positions.push_back(jsin->tell());
        jsin->skip_value();
    }
    end = jsin->tell();
    final_separator = jsin->get_ate_separator();
}

JsonArray::JsonArray(const JsonArray &ja)
{
    jsin = ja.jsin;
    start = ja.start;
    index = 0;
    positions = ja.positions;
    end = ja.end;
    final_separator = ja.final_separator;
}

void JsonArray::finish()
{
    if (jsin && jsin->good()) {
        jsin->seek(end);
        jsin->set_ate_separator(final_separator);
    }
}

bool JsonArray::has_more() { return (index >= 0 && index < positions.size()); }
int JsonArray::size() { return positions.size(); }
bool JsonArray::empty() { return positions.empty(); }

std::string JsonArray::str()
{
    if (jsin) {
        return jsin->substr(start,end-start);
    } else {
        return "[]";
    }
}

void JsonArray::verify_index(int i)
{
    if (!jsin) {
        throw (std::string)"tried to access empty array.";
    } else if (i < 0 || i >= positions.size()) {
        jsin->seek(start);
        std::stringstream err;
        err << "bad index value: " << i;
        jsin->error(err.str());
    }
}


/* iterative access */

bool JsonArray::next_bool()
{
    verify_index(index);
    jsin->seek(positions[index++]);
    return jsin->get_bool();
}

int JsonArray::next_int()
{
    verify_index(index);
    jsin->seek(positions[index++]);
    return jsin->get_int();
}

double JsonArray::next_float()
{
    verify_index(index);
    jsin->seek(positions[index++]);
    return jsin->get_float();
}

std::string JsonArray::next_string()
{
    verify_index(index);
    jsin->seek(positions[index++]);
    return jsin->get_string();
}

JsonArray JsonArray::next_array()
{
    verify_index(index);
    jsin->seek(positions[index++]);
    return jsin->get_array();
}

JsonObject JsonArray::next_object()
{
    verify_index(index);
    jsin->seek(positions[index++]);
    return jsin->get_object();
}

void JsonArray::skip_value()
{
    verify_index(index);
    ++index;
}

/* static access */

bool JsonArray::get_bool(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->get_bool();
}

int JsonArray::get_int(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->get_int();
}

double JsonArray::get_float(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->get_float();
}

std::string JsonArray::get_string(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->get_string();
}

JsonArray JsonArray::get_array(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->get_array();
}

JsonObject JsonArray::get_object(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->get_object();
}

/* iterative type checking */

bool JsonArray::test_null()
{
    if (!has_more()) {
        return false;
    }
    jsin->seek(positions[index]);
    return jsin->test_null();
}

bool JsonArray::test_bool()
{
    if (!has_more()) {
        return false;
    }
    jsin->seek(positions[index]);
    return jsin->test_bool();
}

bool JsonArray::test_int()
{
    if (!has_more()) {
        return false;
    }
    jsin->seek(positions[index]);
    return jsin->test_int();
}

bool JsonArray::test_float()
{
    if (!has_more()) {
        return false;
    }
    jsin->seek(positions[index]);
    return jsin->test_float();
}

bool JsonArray::test_string()
{
    if (!has_more()) {
        return false;
    }
    jsin->seek(positions[index]);
    return jsin->test_string();
}

bool JsonArray::test_array()
{
    if (!has_more()) {
        return false;
    }
    jsin->seek(positions[index]);
    return jsin->test_array();
}

bool JsonArray::test_object()
{
    if (!has_more()) {
        return false;
    }
    jsin->seek(positions[index]);
    return jsin->test_object();
}

/* random-access type checking */

bool JsonArray::has_null(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->test_null();
}

bool JsonArray::has_bool(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->test_bool();
}

bool JsonArray::has_int(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->test_int();
}

bool JsonArray::has_float(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->test_float();
}

bool JsonArray::has_string(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->test_string();
}

bool JsonArray::has_array(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->test_array();
}

bool JsonArray::has_object(int i)
{
    verify_index(i);
    jsin->seek(positions[i]);
    return jsin->test_object();
}

/* iterative value setting by reference */

bool JsonArray::read_into(bool &b)
{
    if (!test_bool()) {
        skip_value();
        return false;
    }
    b = next_bool();
    return true;
}

bool JsonArray::read_into(int &i)
{
    if (!test_number()) {
        skip_value();
        return false;
    }
    i = next_int();
    return true;
}

bool JsonArray::read_into(unsigned &u)
{
    if (!test_number()) {
        skip_value();
        return false;
    }
    u = next_int();
    return true;
}

bool JsonArray::read_into(float &f)
{
    if (!test_number()) {
        skip_value();
        return false;
    }
    f = next_float();
    return true;
}

bool JsonArray::read_into(double &d)
{
    if (!test_number()) {
        skip_value();
        return false;
    }
    d = next_float();
    return true;
}

bool JsonArray::read_into(std::string &s)
{
    if (!test_string()) {
        skip_value();
        return false;
    }
    s = next_string();
    return true;
}

bool JsonArray::read_into(JsonDeserializer &j)
{
    try {
        verify_index(index);
        jsin->seek(positions[index++]);
        j.deserialize(*jsin);
        return true;
    } catch (std::string e) {
        return false;
    }
}


/* class JsonIn
 * represents an istream of JSON data,
 * allowing easy extraction into c++ datatypes.
 */
JsonIn::JsonIn(std::istream *s, bool strict) :
    stream(s), strict(strict), ate_separator(false)
{
}

int JsonIn::tell() { return stream->tellg(); }
char JsonIn::peek() { return (char)stream->peek(); }
bool JsonIn::good() { return stream->good(); }

void JsonIn::seek(int pos)
{
    stream->clear();
    stream->seekg(pos);
    ate_separator = false;
}

void JsonIn::eat_whitespace()
{
    while (is_whitespace(peek())) {
        stream->get();
    }
}

void JsonIn::uneat_whitespace()
{
    while (tell() > 0) {
        stream->seekg(-1, std::istream::cur);
        if (!is_whitespace(peek())) {
            break;
        }
    }
}

void JsonIn::end_value()
{
    ate_separator = false;
    skip_separator();
}

void JsonIn::skip_member()
{
    skip_string();
    skip_pair_separator();
    skip_value();
}

void JsonIn::skip_separator()
{
    char ch;
    eat_whitespace();
    ch = peek();
    if (ch == ',') {
        if (strict && ate_separator) {
            error("duplicate separator");
        }
        stream->get();
        ate_separator = true;
    } else if (ch == ']' || ch == '}' || ch == ':') {
        // okay
        if (strict && ate_separator) {
            std::stringstream err;
            err << "separator should not be found before '" << ch << "'";
            uneat_whitespace();
            error(err.str());
        }
        ate_separator = false;
    } else if (ch == EOF) {
        // that's okay too... probably
        if (strict && ate_separator) {
            uneat_whitespace();
            error("separator at end of file not strictly allowed");
        }
        ate_separator = false;
    } else if (strict) {
        // not okay >:(
        uneat_whitespace();
        error("missing separator", 1);
    }
}

void JsonIn::skip_pair_separator()
{
    char ch;
    eat_whitespace();
    stream->get(ch);
    if (ch != ':') {
        std::stringstream err;
        err << "expected pair separator ':', not '" << ch << "'";
        error(err.str(), -1);
    } else if (strict && ate_separator) {
        error("duplicate separator not strictly allowed", -1);
    }
    ate_separator = true;
}

void JsonIn::skip_string()
{
    char ch;
    eat_whitespace();
    stream->get(ch);
    if (ch != '"') {
        std::stringstream err;
        err << "expecting string but found '" << ch << "'";
        error(err.str(), -1);
    }
    while (stream->good()) {
        stream->get(ch);
        if (ch == '\\') {
            stream->get(ch);
            continue;
        } else if (ch == '"') {
            break;
        } else if (strict && (ch == '\r' || ch == '\n')) {
            error("string not closed before end of line", -1);
        }
    }
    end_value();
}

void JsonIn::skip_value()
{
    char ch;
    eat_whitespace();
    ch = peek();
    // it's either a string '"'
    if (ch == '"') {
        skip_string();
    // or an object '{'
    } else if (ch == '{') {
        skip_object();
    // or an array '['
    } else if (ch == '[') {
        skip_array();
    // or a number (-0123456789)
    } else if (ch == '-' || (ch >= '0' && ch <= '9')) {
        skip_number();
    // or "true", "false" or "null"
    } else if (ch == 't') {
        skip_true();
    } else if (ch == 'f') {
        skip_false();
    } else if (ch == 'n') {
        skip_null();
    // or an error.
    } else {
        std::stringstream err;
        err << "expected JSON value but got '" << ch << "'";
        error(err.str());
    }
    // skip_* end value automatically
}

void JsonIn::skip_object()
{
    start_object();
    while (!end_object()) {
        skip_member();
    }
    // end_value called by end_object
}

void JsonIn::skip_array()
{
    start_array();
    while (!end_array()) {
        skip_value();
    }
    // end_value called by end_array
}

void JsonIn::skip_true()
{
    char text[5];
    eat_whitespace();
    stream->get(text, 5);
    if (strcmp(text, "true") != 0) {
        std::stringstream err;
        err << "expected \"true\", but found \"" << text << "\"";
        error(err.str(), -4);
    }
    end_value();
}

void JsonIn::skip_false()
{
    char text[6];
    eat_whitespace();
    stream->get(text, 6);
    if (strcmp(text, "false") != 0) {
        std::stringstream err;
        err << "expected \"false\", but found \"" << text << "\"";
        error(err.str(), -5);
    }
    end_value();
}

void JsonIn::skip_null()
{
    char text[5];
    eat_whitespace();
    stream->get(text, 5);
    if (strcmp(text, "null") != 0) {
        std::stringstream err;
        err << "expected \"null\", but found \"" << text << "\"";
        error(err.str(), -4);
    }
    end_value();
}

void JsonIn::skip_number()
{
    char ch;
    eat_whitespace();
    // skip all of (+-0123456789.eE)
    while (stream->good()) {
        stream->get(ch);
        if (ch != '+' && ch != '-' && (ch < '0' || ch > '9') &&
                ch != 'e' && ch != 'E' && ch != '.') {
            stream->unget();
            break;
        }
    }
    end_value();
}

std::string JsonIn::get_member_name()
{
    std::string s = get_string();
    skip_pair_separator();
    return s;
}

std::string JsonIn::get_string()
{
    std::string s = "";
    char ch;
    bool backslash = false;
    char unihex[5] = "0000";
    eat_whitespace();
    int startpos = tell();
    // the first character had better be a '"'
    stream->get(ch);
    if (ch != '"') {
        std::stringstream err;
        err << "expecting string but got '" << ch << "'";
        error(err.str(), -1);
    }
    // add chars to the string, one at a time, converting:
    // \", \\, \/, \b, \f, \n, \r, \t and \uxxxx according to JSON spec.
    while (stream->good()) {
        stream->get(ch);
        if (ch == '\\') {
            if (backslash) {
                s += '\\';
                backslash = false;
            } else {
                backslash = true;
                continue;
            }
        } else if (backslash) {
            backslash = false;
            if (ch == '"') {
                s += '"';
            } else if (ch == '/') {
                s += '/';
            } else if (ch == 'b') {
                s += '\b';
            } else if (ch == 'f') {
                s += '\f';
            } else if (ch == 'n') {
                s += '\n';
            } else if (ch == 'r') {
                s += '\r';
            } else if (ch == 't') {
                s += '\t';
            } else if (ch == 'u') {
                // get the next four characters as hexadecimal
                stream->get(unihex, 5);
                // insert the appropriate unicode character in utf8
                // TODO: verify that unihex is in fact 4 hex digits.
                char** endptr = 0;
                unsigned u = (unsigned)strtoul(unihex, endptr, 16);
                s += utf32_to_utf8(u);
            } else {
                // for anything else, just add the character, i suppose
                s += ch;
            }
        } else if (ch == '"') {
            // end of the string
            end_value();
            return s;
        } else if (strict && (ch == '\r' || ch == '\n')) {
            error("reached end of line without closing string", -1);
        } else if (strict && (unsigned char)ch < 0x20) {
            error("invalid character inside string", -1);
        } else {
            s += ch;
        }
    }
    // if we get to here, probably hit a premature EOF?
    if (stream->eof()) {
        stream->clear();
        seek(startpos);
        error("couldn't find end of string, reached EOF.");
    } else if (stream->fail()) {
        throw (std::string)"stream failure while reading string.";
    }
    throw (std::string)"something went wrong D:";
}

int JsonIn::get_int()
{
    // get float value and then convert to int,
    // because "1.359e3" is technically a valid integer.
    return (int)get_float();
}

double JsonIn::get_float()
{
    // this could maybe be prettier?
    char ch;
    bool neg = false;
    int i = 0;
    int e = 0;
    int mod_e = 0;
    eat_whitespace();
    stream->get(ch);
    if (ch == '-') {
        neg = true;
        stream->get(ch);
    } else if (ch != '.' && (ch < '0' || ch > '9')) {
        // not a valid float
        std::stringstream err;
        err << "expecting number but found '" << ch << "'";
        error(err.str(), -1);
    }
    if (strict && ch == '0') {
        // allow a single leading zero in front of a '.' or 'e'/'E'
        stream->get(ch);
        if (ch >= '0' && ch <= '9') {
            error("leading zeros not strictly allowed", -1);
        }
    }
    while (ch >= '0' && ch <= '9') {
        i *= 10;
        i += (ch - '0');
        stream->get(ch);
    }
    if (ch == '.') {
        stream->get(ch);
        while (ch >= '0' && ch <= '9') {
            i *= 10;
            i += (ch - '0');
            mod_e -= 1;
            stream->get(ch);
        }
    }
    if (neg) {
        i *= -1;
    }
    if (ch == 'e' || ch == 'E') {
        stream->get(ch);
        neg = false;
        if (ch == '-') {
            neg = true;
            stream->get(ch);
        } else if (ch == '+') {
            stream->get(ch);
        }
        while (ch >= '0' && ch <= '9') {
            e *= 10;
            e += (ch - '0');
            stream->get(ch);
        }
        if (neg) {
            e *= -1;
        }
    }
    // unget the final non-number character (probably a separator)
    stream->unget();
    end_value();
    // now put it all together!
    return i * pow(10.0f, static_cast<float>(e + mod_e));
}

bool JsonIn::get_bool()
{
    char ch;
    char text[5];
    std::stringstream err;
    eat_whitespace();
    stream->get(ch);
    if (ch == 't') {
        stream->get(text, 4);
        if (strcmp(text, "rue") == 0) {
            end_value();
            return true;
        } else {
            err << "not a boolean. expected \"true\", but got \"";
            err << ch << text << "\"";
            error(err.str(), -4);
        }
    } else if (ch == 'f') {
        stream->get(text, 5);
        if (strcmp(text, "alse") == 0) {
            end_value();
            return false;
        } else {
            err << "not a boolean. expected \"false\", but got \"";
            err << ch << text << "\"";
            error(err.str(), -5);
        }
    }
    err << "not a boolean value! expected 't' or 'f' but got '" << ch << "'";
    error(err.str(), -1);
    throw (std::string)"warnings are silly";
}

void JsonIn::start_array()
{
    eat_whitespace();
    if (peek() == '[') {
        stream->get();
        ate_separator = false;
        return;
    } else {
        // expecting an array, so this is an error
        std::stringstream err;
        err << "tried to start array, but found '";
        err << peek() << "', not '['";
        error(err.str());
    }
}

bool JsonIn::end_array()
{
    eat_whitespace();
    if (peek() == ']') {
        if (strict && ate_separator) {
            uneat_whitespace();
            error("separator not strictly allowed at end of array");
        }
        stream->get();
        end_value();
        return true;
    } else {
        // not the end yet, so just return false?
        return false;
    }
}

void JsonIn::start_object()
{
    eat_whitespace();
    if (peek() == '{') {
        stream->get();
        ate_separator = false; // not that we want to
        return;
    } else {
        // expecting an object, so fail loudly
        std::stringstream err;
        err << "tried to start object, but found '";
        err << peek() << "', not '{'";
        error(err.str());
    }
}

bool JsonIn::end_object()
{
    eat_whitespace();
    if (peek() == '}') {
        if (strict && ate_separator) {
            uneat_whitespace();
            error("separator not strictly allowed at end of object");
        }
        stream->get();
        end_value();
        return true;
    } else {
        // not the end yet, so just return false?
        return false;
    }
}

bool JsonIn::test_null()
{
    eat_whitespace();
    if (peek() == 'n') {
        return true;
    }
    return false;
}

bool JsonIn::test_bool()
{
    eat_whitespace();
    const char ch = peek();
    if (ch == 't' || ch == 'f') {
        return true;
    }
    return false;
}

bool JsonIn::test_int()
{
    // we have to parse it to know if it's an integer or not.
    // well...
    // technically we could test for the existance of a decimal point,
    // then add the number of digits after it,
    // then subtract this from the exponent,
    // and test if the exponent is greater than -1,
    // but then we'd /still/ have to adjust for silly cases like 0.000300e4,
    // which, technically, is an integer.
    // feel free to implement.
    eat_whitespace();
    const int startpos = tell();
    const char ch = peek();
    if (ch != '-' && ch != '+' && ch != '.' && (ch < '0' || ch > '9')) {
        return false;
    }
    const double f = get_float();
    seek(startpos);
    if (int(f) == f) {
        return true;
    }
    return false;
}

bool JsonIn::test_float()
{
    // considering all numbers to be valid floats.
    // note that this is thus much easier than testing for an int.
    eat_whitespace();
    const char ch = peek();
    if (ch != '-' && ch != '+' && ch != '.' && (ch < '0' || ch > '9')) {
        return false;
    }
    return true;
}

bool JsonIn::test_string()
{
    eat_whitespace();
    if (peek() == '"') {
        return true;
    }
    return false;
}

bool JsonIn::test_array()
{
    eat_whitespace();
    if (peek() == '[') {
        return true;
    }
    return false;
}

bool JsonIn::test_object()
{
    eat_whitespace();
    if (peek() == '{') {
        return true;
    }
    return false;
}

/* error display */

// WARNING: for occasional use only.
std::string JsonIn::line_number(int offset_modifier)
{
    if (stream->eof()) {
        return "EOF";
    } else if (stream->fail()) {
        return "???";
    } // else stream is fine
    int pos = tell();
    int line = 1;
    int offset = 1;
    char ch;
    seek(0);
    for (int i=0; i < pos; ++i) {
        stream->get(ch);
        if (ch == '\r') {
            offset = 1;
            ++line;
            if (peek() == '\n') {
                stream->get();
                ++i;
            }
        } else if (ch == '\n') {
            offset = 1;
            ++line;
        } else {
            ++offset;
        }
    }
    std::stringstream ret;
    ret << "line " << line << ":" << (offset + offset_modifier);
    return ret.str();
}

void JsonIn::error(std::string message, int offset)
{
    std::ostringstream err;
    err << line_number(offset) << ": " << message;
    // if we can't get more info from the stream don't try
    if (!stream->good()) {
        throw err.str();
    }
    // also print surrounding few lines of context, if not too large
    err << "\n\n";
    stream->seekg(offset, std::istream::cur);
    size_t pos = tell();
    rewind(3, 240);
    size_t startpos = tell();
    char buffer[241];
    stream->read(&buffer[0], pos-startpos);
    buffer[pos-startpos] = '\0';
    err << buffer;
    if (!is_whitespace(peek())) {
        err << peek();
    }
    // display a pointer to the position
    rewind(1, 240);
    startpos = tell();
    err << '\n';
    for (int i=0; i < pos-startpos-1; ++i) {
        err << ' ';
    }
    err << "^\n";
    seek(pos);
    // if that wasn't the end of the line, continue underneath pointer
    char ch = stream->get();
    if (ch == '\r') {
        if (peek() == '\n') {
            stream->get();
        }
    } else if (ch == '\n') {
        // pass
    } else if (peek() != '\r' && peek() != '\n') {
        for (int i=0; i < pos-startpos; ++i) {
            err << ' ';
        }
    }
    // print the next couple lines as well
    int line_count = 0;
    for (int i=0; i < 240; ++i) {
        stream->get(ch);
        err << ch;
        if (ch == '\r') {
            ++line_count;
            if (peek() == '\n') {
                err << stream->get();
            }
        } else if (ch == '\n') {
            ++line_count;
        }
        if (line_count > 2) {
            break;
        }
    }
    throw err.str();
}

void JsonIn::rewind(int max_lines, int max_chars)
{
    if (max_lines < 0 && max_chars < 0) {
        // just rewind to the beginning i guess
        seek(0);
        return;
    }
    if (tell() == 0) {
        return;
    }
    int lines_found = 0;
    stream->seekg(-1, std::istream::cur);
    for (int i=0; i < max_chars; ++i) {
        size_t tellpos = tell();
        if (peek() == '\n') {
            ++lines_found;
            if (tellpos > 0) {
                stream->seekg(-1, std::istream::cur);
                // note: does not update tellpos or count a character
                if (peek() != '\r') {
                    continue;
                }
            }
        } else if (peek() == '\r') {
            ++lines_found;
        }
        if (tellpos == 0) {
            break;
        } else if (lines_found == max_lines) {
            // don't include the last \n or \r
            stream->seekg(1, std::istream::cur);
            break;
        }
        stream->seekg(-1, std::istream::cur);
    }
}

std::string JsonIn::substr(size_t pos, size_t len)
{
    std::string ret;
    if (len == std::string::npos) {
        stream->seekg(0, std::istream::end);
        size_t end = tell();
        len = end - pos;
    }
    ret.resize(len);
    stream->seekg(pos);
    stream->read(&ret[0],len);
    return ret;
}


/* class JsonOut
 * represents an ostream of JSON data,
 * allowing easy serialization of c++ datatypes.
 */
JsonOut::JsonOut(std::ostream *s)
{
    stream = s;
    need_separator = false;
    // ensure user's locale doesn't interfere with number format
    stream->imbue(std::locale::classic());
    // scientific format for floating-point numbers
    stream->setf(std::ostream::scientific, std::ostream::floatfield);
    // it's already decimal, but set it anyway
    stream->setf(std::ostream::dec, std::ostream::basefield);
    // could also set showbase and showpoint,
    // but it currently doesn't matter.
}

void JsonOut::write_separator()
{
    stream->put(',');
    need_separator = false;
}

void JsonOut::write_member_separator()
{
    stream->put(':');
    need_separator = false;
}

void JsonOut::start_object()
{
    if (need_separator) {
        write_separator();
    }
    stream->put('{');
    need_separator = false;
}

void JsonOut::end_object()
{
    stream->put('}');
    need_separator = true;
}

void JsonOut::start_array()
{
    if (need_separator) {
        write_separator();
    }
    stream->put('[');
    need_separator = false;
}

void JsonOut::end_array()
{
    stream->put(']');
    need_separator = true;
}

void JsonOut::write_null()
{
    if (need_separator) {
        write_separator();
    }
    stream->write("null", 4);
    need_separator = true;
}

void JsonOut::write(const bool &b)
{
    if (need_separator) {
        write_separator();
    }
    if (b) {
        stream->write("true", 4);
    } else {
        stream->write("false", 5);
    }
    need_separator = true;
}

void JsonOut::write(const int &i)
{
    if (need_separator) {
        write_separator();
    }
    // format specified in constructor, let's hope it hasn't changed
    *stream << i;
    need_separator = true;
}

void JsonOut::write(const unsigned &u)
{
    if (need_separator) {
        write_separator();
    }
    // format specified in constructor, let's hope it hasn't changed
    *stream << u;
    need_separator = true;
}

void JsonOut::write(const double &f)
{
    if (need_separator) {
        write_separator();
    }
    // format specified in constructor, let's hope it hasn't changed
    *stream << f;
    need_separator = true;
}

void JsonOut::write(const std::string &s)
{
    if (need_separator) {
        write_separator();
    }
    unsigned char ch;
    stream->put('"');
    for (int i = 0; i < s.size(); ++i) {
        ch = s[i];
        if (ch == '"') {
            stream->write("\\\"", 2);
        } else if (ch == '\\') {
            stream->write("\\\\", 2);
        } else if (ch == '/') {
            // don't technically need to escape this
            stream->put('/');
        } else if (ch == '\b') {
            stream->write("\\b", 2);
        } else if (ch == '\f') {
            stream->write("\\f", 2);
        } else if (ch == '\n') {
            stream->write("\\n", 2);
        } else if (ch == '\r') {
            stream->write("\\r", 2);
        } else if (ch == '\t') {
            stream->write("\\t", 2);
        } else if (ch < 0x20) {
            // convert to "\uxxxx" unicode escape
            stream->write("\\u00", 4);
            stream->put((ch < 0x10) ? '0' : '1');
            char remainder = ch & 0x0F;
            if (remainder < 0x0A) {
                stream->put('0' + remainder);
            } else {
                stream->put('A' + (remainder - 0x0A));
            }
        } else {
            stream->put(ch);
        }
    }
    stream->put('"');
    need_separator = true;
}

void JsonOut::write(const JsonSerializer &thing)
{
    if (need_separator) {
        write_separator();
    }
    thing.serialize(*this);
    need_separator = true;
}

void JsonOut::member(const std::string &name)
{
    write(name);
    write_member_separator();
}

void JsonOut::null_member(const std::string &name)
{
    member(name);
    write_null();
}

