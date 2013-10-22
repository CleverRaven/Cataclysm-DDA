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
#define STRICT_JSON true

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
    // TODO: this is not an exhaustive list of valid whitespace characters.
    // it should probably be with the other utf8 processing functions as well.
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
    strict = STRICT_JSON;
    jsin = j;
    start = jsin->tell();
    // cache the position of the value for each member
    jsin->start_object();
    bool gotsep = true;
    bool lastsep = false;
    while (!jsin->end_object()) {
        std::string n = jsin->get_member_name();
        int p = jsin->tell();
        positions[n] = p;
        if ( strict && ! gotsep ) {
            end = jsin->tell();
            jsin->seek(start);
            throw jsin->line_number() + ": missing comma: " + dump_input();
        }
        gotsep = jsin->skip_value();
        lastsep = gotsep;
    }
    end = jsin->tell();
    if ( strict && lastsep ) {
        jsin->seek(start);
        throw jsin->line_number() + ": trailing comma: " + dump_input();
    }
}

JsonObject::JsonObject(const JsonObject &jo)
{
    jsin = jo.jsin;
    start = jo.start;
    positions = jo.positions;
    end = jo.end;
    strict = jo.strict;
}

void JsonObject::finish()
{
    jsin->seek(end);
}

int JsonObject::verify_position(const std::string &name,
                                const bool throw_exception)
{
    int pos = positions[name]; // initialized to 0 if it doesn't exist
    if (pos > start) {
        return pos;
    } else if (throw_exception) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    // 0 is always the opening brace,
    // so it will never indicate a valid member position
    return 0;
}

bool JsonObject::has_member(const std::string &name)
{
    return (bool)verify_position(name, false);
}

std::string JsonObject::line_number()
{
    jsin->seek(start);
    return jsin->line_number();
}

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

JsonArray JsonObject::get_array(const std::string &name)
{
    int pos = positions[name];
    if (pos <= start) {
        return JsonArray(); // empty array
    }
    jsin->seek(pos);
    return JsonArray(jsin);
}

JsonObject JsonObject::get_object(const std::string &name)
{
    int pos = verify_position(name);
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

std::string JsonObject::dump_input() {
    int origpos = jsin->tell();
    jsin->seek(start);
    jsin->eat_whitespace();
    int len = end - jsin->tell();
    std::string ret;
    ret.resize(len);
    jsin->read(&ret[0],len);
    jsin->seek(origpos);
    return ret;
}
/* class JsonArray
 * represents a JSON array,
 * providing access to the underlying data.
 */
JsonArray::JsonArray(JsonIn *j) : positions()
{
    strict = STRICT_JSON;
    jsin = j;
    start = jsin->tell();
    index = 0;
    // cache the position of each element
    bool gotsep = true;
    bool lastsep = false;
    jsin->start_array();
    while (!jsin->end_array()) {
        positions.push_back(jsin->tell());
        if ( strict && ! gotsep ) {
            end = jsin->tell();
            jsin->seek(start);
            throw jsin->line_number() + ": missing comma: " + dump_input();
        }
        gotsep = jsin->skip_value();
        lastsep = gotsep;
    }
    end = jsin->tell();
    if ( strict && lastsep ) {
        jsin->seek(start);
        throw jsin->line_number() + ": trailing comma: " + dump_input();
    }
}

JsonArray::JsonArray(const JsonArray &ja)
{
    jsin = ja.jsin;
    start = ja.start;
    index = 0;
    positions = ja.positions;
    end = ja.end;
    strict = ja.strict;
}

bool JsonArray::has_more()
{
    return (index >= 0 && index < positions.size());
}

int JsonArray::size()
{
    return positions.size();
}

void JsonArray::verify_index(int i)
{
    if (!jsin) {
        throw (std::string)"tried to access empty array.";
    } else if (i < 0 || i >= positions.size()) {
        jsin->seek(start);
        std::stringstream err;
        err << jsin->line_number() << ": ";
        err << "bad index value: " << i;
        throw err.str();
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

std::string JsonArray::dump_input() {
    int origpos = jsin->tell();
    jsin->seek(start);
    jsin->eat_whitespace();
    int len = end - jsin->tell();
    std::string ret;
    ret.resize(len);
    jsin->read(&ret[0],len);
    jsin->seek(origpos);
    return ret;
}

/* class JsonIn
 * represents an istream of JSON data,
 * allowing easy extraction into c++ datatypes.
 */
JsonIn::JsonIn(std::istream *s)
{
    stream = s;
    strict = STRICT_JSON;
}

int JsonIn::tell() { return stream->tellg(); }
void JsonIn::seek(int pos) { stream->seekg(pos, std::istream::beg); }
char JsonIn::peek() { return (char)stream->peek(); }
bool JsonIn::good() { return stream->good(); }
void JsonIn::read(char * str, int len) { stream->read(str, len); }
void JsonIn::eat_whitespace()
{
    while (is_whitespace(peek())) {
        stream->get();
    }
}

bool JsonIn::skip_member()
{
    skip_string();
    skip_pair_separator();
    skip_value();
    return skip_separator();
}

bool JsonIn::skip_pair_separator()
{
    char ch;
    eat_whitespace();
    stream->get(ch);
    if (ch != ':') {
        std::stringstream err;
        err << line_number(-1) << ": expected pair separator ':', not '" << ch << "'";
        throw err.str();
    }
    return false;
}

bool JsonIn::skip_string()
{
    char ch;
    eat_whitespace();
    stream->get(ch);
    if (ch != '"') {
        std::stringstream err;
        err << line_number(-1) << ": expecting string but found '" << ch << "'";
        throw err.str();
    }
    while (stream->good()) {
        stream->get(ch);
        if (ch == '\\') {
            stream->get(ch);
            continue;
        } else if (ch == '"') {
            break;
        }
    }
    return skip_separator();
}

bool JsonIn::skip_value()
{
    char ch;
    bool foundsep;
    eat_whitespace();
    ch = peek();
    // it's either a string '"'
    if (ch == '"') {
        foundsep = skip_string();
    // or an object '{'
    } else if (ch == '{') {
        foundsep = skip_object();
    // or an array '['
    } else if (ch == '[') {
        foundsep = skip_array();
    // or a number (-0123456789)
    } else if (ch == '-' || (ch >= '0' && ch <= '9')) {
        foundsep = skip_number();
    // or "true", "false" or "null"
    } else if (ch == 't') {
        foundsep = skip_true();
    } else if (ch == 'f') {
        foundsep = skip_false();
    } else if (ch == 'n') {
        foundsep = skip_null();
    // or an error.
    } else {
        std::stringstream err;
        err << line_number() << ": expected JSON value but got '" << ch << "'";
        throw err.str();
    }
    return foundsep;//b( foundsep || skip_separator() );
}

bool JsonIn::skip_object()
{
    char ch;
    bool lastsep = false;
    int brackets = 1;
    eat_whitespace();
    int startpos = tell();
    stream->get(ch);
    if (ch != '{') {
        std::stringstream err;
        err << line_number(-1) << ": expected object but found '" << ch << "'";
        throw err.str();
    }
    while (brackets && stream->good()) {
        stream->get(ch);
        // ignore everything inside strings
        if (ch == '"') {
            stream->unget();
            lastsep = skip_string();
        // otherwise count opening and closing brackets until they all match
        } else if (ch == '{') {
            brackets += 1;
            lastsep = false;
        } else if (ch == '}') {
            brackets -= 1;
            if ( strict && lastsep ) {
                std::stringstream err;
                std::string txt;
                int errpos = tell();
                err << line_number(-1) << ": trailing comma: ";
                stream->seekg(startpos);
                stream->read(&txt[0],errpos-startpos);
                err << txt;
                throw err.str();                 
            }
            lastsep = false;
        } else if (!is_whitespace(ch)) {
            lastsep = false;
        }
    }
    if (brackets != 0) {
        // something messed up!
        std::stringstream err;
        if (stream->fail()) {
            throw (std::string)"stream failure while reading object.";
        } else if (stream->eof()) {
            stream->clear();
            seek(startpos);
            err << line_number() << ": ";
            err << "couldn't find end of object, reached EOF with ";
            err << brackets << " bracket(s) left.";
            throw err.str();
        } else { // this should be impossible
            err << line_number() << " ";
            seek(startpos);
            err << "(" << line_number() << "): ";
            err << "object brackets didn't match?";
            err << " " << brackets << " bracket(s) left.";
            throw err.str();
        }
    }
    return skip_separator();
}

bool JsonIn::skip_array()
{
    char ch;
    bool lastsep = false;
    int brackets = 1;
    eat_whitespace();
    int startpos = tell();
    stream->get(ch);
    if (ch != '[') {
        std::stringstream err;
        err << line_number(-1) << ": expected array but found '" << ch << "'";
        throw err.str();
    }
    while (brackets && stream->good()) {
        stream->get(ch);
        // ignore everything inside strings
        if (ch == '"') {
            stream->unget();
            lastsep = skip_string();
        // otherwise count opening and closing brackets until they all match
        } else if (ch == '[') {
            brackets += 1;
            lastsep = false;
        } else if (ch == ']') {
            brackets -= 1;
            if ( strict && lastsep ) {
                std::stringstream err;
                std::string txt;
                int errpos = tell();
                err << line_number(-1) << ": trailing comma: ";
                stream->seekg(startpos);
                stream->read(&txt[0],errpos-startpos);
                err << txt;
                throw err.str();                 
            }
            lastsep = false;
        } else if (!is_whitespace(ch)) {
            lastsep = false;
        }

    }
    if (brackets != 0) {
        // something messed up!
        std::stringstream err;
        if (stream->fail()) {
            throw (std::string)"stream failure while reading array.";
        } else if (stream->eof()) {
            stream->clear();
            seek(startpos);
            err << line_number() << ": ";
            err << "couldn't find end of array, reached EOF with ";
            err << brackets << " bracket(s) left.";
            throw err.str();
        } else { // this should be impossible
            err << line_number() << " ";
            seek(startpos);
            err << "(" << line_number() << "): ";
            err << "array brackets didn't match?";
            err << " " << brackets << " bracket(s) left.";
            throw err.str();
        }
    }
    return skip_separator();
}

bool JsonIn::skip_true()
{
    char text[5];
    eat_whitespace();
    stream->get(text, 5);
    if (strcmp(text, "true") != 0) {
        std::stringstream err;
        err << line_number(-4) << ": expected \"true\", but found \"" << text << "\"";
        throw err.str();
    }
    return skip_separator();
}

bool JsonIn::skip_false()
{
    char text[6];
    eat_whitespace();
    stream->get(text, 6);
    if (strcmp(text, "false") != 0) {
        std::stringstream err;
        err << line_number(-5) << ": expected \"false\", but found \"" << text << "\"";
        throw err.str();
    }
    return skip_separator();
}

bool JsonIn::skip_null()
{
    char text[5];
    eat_whitespace();
    stream->get(text, 5);
    if (strcmp(text, "null") != 0) {
        std::stringstream err;
        err << line_number(-4) << ": expected \"null\", but found \"" << text << "\"";
        throw err.str();
    }
    return skip_separator();
}

bool JsonIn::skip_number()
{
    char ch;
    eat_whitespace();
    // skip all of (+-0123456789.eE)
    while (stream->good()) {
        stream->get(ch);
        if (ch != '+' && ch != '-' && (ch < '0' || ch > '9') &&
                ch != 'e' && ch != 'E' && ch != '.') {
            stream->unget();
            return (ch==',');
        }
    }
    return skip_separator();
}

bool JsonIn::skip_separator()
{
    char ch;
    eat_whitespace();
    ch = peek();
    if (ch == ',') {
        stream->get();
        return true;
    }
    return false;
}

std::string JsonIn::get_member_name()
{
    // get the name
    std::string s = get_string();
    // eat any whitespace
    eat_whitespace();
    // skip the separator
    skip_pair_separator();
    // done
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
        err << line_number(-1) << ": expecting string but got '" << ch << "'";
        throw err.str();
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
            skip_separator();
            return s;
        } else {
            s += ch;
        }
    }
    // if we get to here, probably hit a premature EOF?
    if (stream->fail()) {
        throw (std::string)"stream failure while reading string.";
    } else if (stream->eof()) {
        stream->clear();
        seek(startpos);
        std::stringstream err;
        err << line_number() << ": ";
        err << "couldn't find end of string, reached EOF.";
        throw err.str();
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
    skip_separator();
    // now put it all together!
    return i * pow(10.0f, static_cast<float>(e + mod_e));
}

bool JsonIn::get_bool()
{
    char ch;
    char text[5];
    int pos = tell();
    std::stringstream err;
    eat_whitespace();
    stream->get(ch);
    if (ch == 't') {
        stream->get(text, 4);
        if (strcmp(text, "rue") == 0) {
            skip_separator();
            return true;
        } else {
            err << line_number(-4) << ": ";
            err << "not a boolean. expected \"true\", but got \"";
            err << ch << text << "\"";
            seek(pos);
            throw err.str();
        }
    } else if (ch == 'f') {
        stream->get(text, 5);
        if (strcmp(text, "alse") == 0) {
            skip_separator();
            return false;
        } else {
            err << line_number(-5) << ": ";
            err << "not a boolean. expected \"false\", but got \"";
            err << ch << text << "\"";
            seek(pos);
            throw err.str();
        }
    }
    err << line_number(-1) << ": ";
    err << "not a boolean value! expected 't' or 'f' but got '" << ch << "'";
    seek(pos);
    throw err.str();
}

void JsonIn::start_array()
{
    eat_whitespace();
    if (peek() == '[') {
        stream->get();
        return;
    } else {
        // expecting an array, so this is an error
        std::stringstream err;
        err << line_number() << ": ";
        err << "tried to start array, but found '";
        err << peek() << "', not '['";
        throw err.str();
    }
}

bool JsonIn::end_array()
{
    eat_whitespace();
    if (peek() == ']') {
        stream->get();
        return true;
    } else if (peek() == ',') {
        // also eat separators, makes iterating easy
        stream->get();
        return false;
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
        return;
    } else {
        // expecting an object, so fail loudly
        std::stringstream err;
        err << line_number() << ": ";
        err << "tried to start object, but found '";
        err << peek() << "', not '{'";
        throw err.str();
    }
}

bool JsonIn::end_object()
{
    eat_whitespace();
    if (peek() == '}') {
        stream->get();
        return true;
    } else if (peek() == ',') {
        // also eat separators, makes iterating easy
        stream->get();
        return false;
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
    // but then we'd /still/ have to adjust for silly cases like 0.0003e4,
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

// WARNING: intended for occasional use only
// TODO: track character/line offset while parsing so this can be fast.
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
