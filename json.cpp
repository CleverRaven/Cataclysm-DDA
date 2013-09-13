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
 * A central dispatcher will load each .json file, read the "type",
 * and send a Jsin instance to the appropriate constructor.
 * ~
 * Object constructors can use the Jsin class to construct from JSON.
 * The type of each member should be inferrable from the object "type",
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


/* class Jsobj
 * represents a JSON object,
 * providing access to the underlying data.
 */
Jsobj::Jsobj(Jsin *j)
{
    jsin = j;
    start = jsin->tell();
    jsin->start_object();
    while (!jsin->end_object()) {
        std::string n = jsin->get_member_name();
        int p = jsin->tell();
        positions[n] = p;
        jsin->skip_value();
    }
    end = jsin->tell();
}

void Jsobj::finish()
{
    jsin->seek(end);
}

bool Jsobj::get_bool(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    jsin->seek(pos);
    return jsin->get_bool();
}

bool Jsobj::get_bool(std::string name, bool fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_bool();
}

int Jsobj::get_int(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    jsin->seek(pos);
    return jsin->get_int();
}

int Jsobj::get_int(std::string name, int fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_int();
}

double Jsobj::get_float(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    jsin->seek(pos);
    return jsin->get_float();
}

double Jsobj::get_float(std::string name, double fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_float();
}

std::string Jsobj::get_string(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        jsin->seek(start);
        throw jsin->line_number() + ": member not found: " + name;
    }
    jsin->seek(pos);
    return jsin->get_string();
}

std::string Jsobj::get_string(std::string name, std::string fallback)
{
    int pos = positions[name];
    if (pos <= start) {
        return fallback;
    }
    jsin->seek(pos);
    return jsin->get_string();
}

Jsarr Jsobj::get_array(std::string name)
{
    int pos = positions[name];
    if (pos <= start) {
        return Jsarr(); // empty array
    }
    jsin->seek(pos);
    return Jsarr(jsin);
}


/* class Jsarr
 * represents a JSON array,
 * providing access to the underlying data.
 */
Jsarr::Jsarr(Jsin *j)
{
    jsin = j;
    start = jsin->tell();
    pos = 0;
    jsin->start_array();
    while (!jsin->end_array()) {
        positions.push_back(jsin->tell());
        jsin->skip_value();
    }
}

bool Jsarr::has_more()
{
    return (pos >= 0 && pos < positions.size());
}

bool Jsarr::next_bool()
{
    jsin->seek(positions[pos++]);
    return jsin->get_bool();
}

int Jsarr::next_int()
{
    jsin->seek(positions[pos++]);
    return jsin->get_int();
}

double Jsarr::next_float()
{
    jsin->seek(positions[pos++]);
    return jsin->get_float();
}

std::string Jsarr::next_string()
{
    jsin->seek(positions[pos++]);
    return jsin->get_string();
}

Jsarr Jsarr::next_array()
{
    jsin->seek(positions[pos++]);
    return jsin->get_array();
}

Jsobj Jsarr::next_object()
{
    jsin->seek(positions[pos++]);
    return jsin->get_object();
}


/* class Jsin
 * represents an istream of JSON data,
 * allowing easy extraction into c++ datatypes.
 */
Jsin::Jsin(std::istream *s)
{
    stream = s;
}

int Jsin::tell() { return stream->tellg(); }
void Jsin::seek(int pos) { stream->seekg(pos); }
char Jsin::peek() { return (char)stream->peek(); }
bool Jsin::good() { return stream->good(); }

void Jsin::eat_whitespace()
{
    while (is_whitespace((char)stream->peek())) {
        stream->get();
    }
}

void Jsin::skip_member()
{
    skip_string();
    skip_pair_separator();
    skip_value();
    skip_separator();
}

void Jsin::skip_pair_separator()
{
    char ch;
    eat_whitespace();
    stream->get(ch);
    if (ch != ':') {
        std::stringstream err;
        err << line_number() << ": expected pair separator ':', not '" << ch << "'";
        throw err.str();
    }
}

void Jsin::skip_string()
{
    char ch;
    eat_whitespace();
    stream->get(ch);
    if (ch != '"') {
        std::stringstream err;
        err << line_number() << ": expecting string but found '" << ch << "'";
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

void Jsin::skip_value()
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

void Jsin::skip_object()
{
    char ch;
    int brackets = 1;
    eat_whitespace();
    stream->get(ch);
    if (ch != '{') {
        std::stringstream err;
        err << line_number() << ": expected object but found '" << ch << "'";
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
        err << line_number() << ": couldn't find end of object!";
        err << " " << brackets << " bracket(s) left.";
        throw err.str();
    }
    skip_separator();
}

void Jsin::skip_array()
{
    char ch;
    int brackets = 1;
    eat_whitespace();
    stream->get(ch);
    if (ch != '[') {
        std::stringstream err;
        err << line_number() << ": expected array but found '" << ch << "'";
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
        err << line_number() << ": couldn't find end of array!";
        err << " " << brackets << " bracket(s) left.";
        throw err.str();
    }
    skip_separator();
}

void Jsin::skip_true()
{
    char ch[5];
    eat_whitespace();
    stream->get(ch, 5);
    if (strcmp(ch, "true") != 0) {
        std::stringstream err;
        err << line_number() << ": expected \"true\", but found \"" << ch << "\"";
        throw err.str();
    }
    skip_separator();
}

void Jsin::skip_false()
{
    char ch[6];
    eat_whitespace();
    stream->get(ch, 6);
    if (strcmp(ch, "false") != 0) {
        std::stringstream err;
        err << line_number() << ": expected \"false\", but found \"" << ch << "\"";
        throw err.str();
    }
    skip_separator();
}

void Jsin::skip_null()
{
    char ch[5];
    eat_whitespace();
    stream->get(ch, 5);
    if (strcmp(ch, "null") != 0) {
        std::stringstream err;
        err << line_number() << ": expected \"null\", but found \"" << ch << "\"";
        throw err.str();
    }
    skip_separator();
}

void Jsin::skip_number()
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

void Jsin::skip_separator()
{
    char ch;
    eat_whitespace();
    ch = peek();
    if (ch == ',') {
        stream->get();
    }
}

std::string Jsin::get_member_name()
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

std::string Jsin::get_string()
{
    std::string s = "";
    char ch;
    bool backslash = false;
    char unihex[5] = "0000";
    eat_whitespace();
    // the first character had better be a '"'
    stream->get(ch);
    if (ch != '"') {
        std::stringstream err;
        err << line_number() << ": expecting string but got '" << ch << "'";
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
    throw (std::string)"something went wrong D:";
}

int Jsin::get_int()
{
    // get float value and then convert to int,
    // because "1.359e3" is technically a valid integer.
    return (int)get_float();
}

double Jsin::get_float()
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

bool Jsin::get_bool()
{
    char ch;
    char text[5];
    int pos = stream->tellg();
    std::stringstream err;
    eat_whitespace();
    stream->get(ch);
    if (ch == 't') {
        stream->get(text, 4);
        if (strcmp(text, "rue") == 0) {
            skip_separator();
            return true;
        } else {
            err << line_number() << ": ";
            err << "not a boolean. expected \"true\", but got \"";
            err << ch << text << "\"";
            stream->seekg(pos);
            throw err.str();
        }
    } else if (ch == 'f') {
        stream->get(text, 5);
        if (strcmp(text, "alse") == 0) {
            skip_separator();
            return false;
        } else {
            err << line_number() << ": ";
            err << "not a boolean. expected \"false\", but got \"";
            err << ch << text << "\"";
            stream->seekg(pos);
            throw err.str();
        }
    }
    err << line_number() << ": ";
    err << "not a boolean value! expected 't' or 'f' but got '" << ch << "'";
    stream->seekg(pos);
    throw err.str();
}

void Jsin::start_array()
{
    eat_whitespace();
    if (stream->peek() == (int)'[') {
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

bool Jsin::end_array()
{
    eat_whitespace();
    if (stream->peek() == (int)']') {
        stream->get();
        return true;
    } else if (stream->peek() == (int)',') {
        // also eat separators, makes iterating easy
        stream->get();
        return false;
    } else {
        // not the end yet, so just return false?
        return false;
    }
}

void Jsin::start_object()
{
    eat_whitespace();
    if (stream->peek() == (int)'{') {
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

bool Jsin::end_object()
{
    eat_whitespace();
    if (stream->peek() == (int)'}') {
        stream->get();
        return true;
    } else if (stream->peek() == (int)',') {
        // also eat separators, makes iterating easy
        stream->get();
        return false;
    } else {
        // not the end yet, so just return false?
        return false;
    }
}

// intended for occasional use only
std::string Jsin::line_number()
{
    int pos = stream->tellg();
    int line = 1;
    int offset = 1;
    stream->seekg(0);
    for (int i=0; i < pos; ++i) {
        if (stream->get() == '\n') {
            offset = 1;
            ++line;
        } else {
            ++offset;
        }
    }
    std::stringstream ret;
    ret << "line " << line << ":" << offset;
    return ret.str();
}
