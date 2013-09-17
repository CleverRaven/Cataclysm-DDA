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
JsonObject::JsonObject(JsonIn *j)
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
}

void JsonObject::finish()
{
    jsin->seek(end);
}

bool JsonObject::has_member(std::string name)
{
    int pos = positions[name]; // zero if the member isn't there
    if (pos > start) {
        return true;
    }
    return false;
}

std::string JsonObject::line_number()
{
    jsin->seek(start);
    return jsin->line_number();
}

bool JsonObject::get_bool(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    jsin->seek(pos);
    return jsin->get_bool();
}

bool JsonObject::get_bool(std::string name, bool fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_bool();
}

int JsonObject::get_int(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    jsin->seek(pos);
    return jsin->get_int();
}

int JsonObject::get_int(std::string name, int fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_int();
}

double JsonObject::get_float(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    jsin->seek(pos);
    return jsin->get_float();
}

double JsonObject::get_float(std::string name, double fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_float();
}

std::string JsonObject::get_string(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    jsin->seek(pos);
    return jsin->get_string();
}

std::string JsonObject::get_string(std::string name, std::string fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_string();
}

JsonArray JsonObject::get_array(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        return JsonArray(); // empty array
    }
    jsin->seek(pos);
    return JsonArray(jsin);
}


/* class JsonArray
 * represents a JSON array,
 * providing access to the underlying data.
 */
JsonArray::JsonArray(JsonIn *j)
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


/* class JsonIn
 * represents an istream of JSON data,
 * allowing easy extraction into c++ datatypes.
 */
JsonIn::JsonIn(std::istream *s)
{
    stream = s;
}

int JsonIn::tell() { return stream->tellg(); }
void JsonIn::seek(int pos) { stream->seekg(pos, std::istream::beg); }
char JsonIn::peek() { return (char)stream->peek(); }
bool JsonIn::good() { return stream->good(); }

void JsonIn::eat_whitespace()
{
    while (is_whitespace(peek())) {
        stream->get();
    }
}

void JsonIn::skip_member()
{
    skip_string();
    skip_pair_separator();
    skip_value();
    skip_separator();
}

void JsonIn::skip_pair_separator()
{
    char ch;
    eat_whitespace();
    stream->get(ch);
    if (ch != ':') {
        std::stringstream err;
        err << line_number(-1) << ": expected pair separator ':', not '" << ch << "'";
        throw err.str();
    }
}

void JsonIn::skip_string()
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
    skip_separator();
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
        err << line_number() << ": expected JSON value but got '" << ch << "'";
        throw err.str();
    }
    skip_separator();
}

void JsonIn::skip_object()
{
    char ch;
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
            skip_string();
        // otherwise count opening and closing brackets until they all match
        } else if (ch == '{') {
            brackets += 1;
        } else if (ch == '}') {
            brackets -= 1;
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
    skip_separator();
}

void JsonIn::skip_array()
{
    char ch;
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
            skip_string();
        // otherwise count opening and closing brackets until they all match
        } else if (ch == '[') {
            brackets += 1;
        } else if (ch == ']') {
            brackets -= 1;
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
    skip_separator();
}

void JsonIn::skip_true()
{
    char text[5];
    eat_whitespace();
    stream->get(text, 5);
    if (strcmp(text, "true") != 0) {
        std::stringstream err;
        err << line_number(-4) << ": expected \"true\", but found \"" << text << "\"";
        throw err.str();
    }
    skip_separator();
}

void JsonIn::skip_false()
{
    char text[6];
    eat_whitespace();
    stream->get(text, 6);
    if (strcmp(text, "false") != 0) {
        std::stringstream err;
        err << line_number(-5) << ": expected \"false\", but found \"" << text << "\"";
        throw err.str();
    }
    skip_separator();
}

void JsonIn::skip_null()
{
    char text[5];
    eat_whitespace();
    stream->get(text, 5);
    if (strcmp(text, "null") != 0) {
        std::stringstream err;
        err << line_number(-4) << ": expected \"null\", but found \"" << text << "\"";
        throw err.str();
    }
    skip_separator();
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
            return;
        }
    }
    skip_separator();
}

void JsonIn::skip_separator()
{
    char ch;
    eat_whitespace();
    ch = peek();
    if (ch == ',') {
        stream->get();
    }
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
    return i * pow(10, e + mod_e);
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

// intended for occasional use only
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
