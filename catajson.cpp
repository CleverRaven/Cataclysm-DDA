#include "catajson.h"
#include "output.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

catajson::catajson()
{
    path_msg = "";
}

catajson::catajson(std::string path, bool is_static)
{
    std::ifstream file;
    file.open(path.c_str());
    file >> val;
    file.close();

    path_msg = path;

    std::string err = picojson::get_last_error();
    if (! err.empty()) {
        if(is_static)
        {
            std::cout << "Parse error in " << path << std::endl << "ERROR: " << err;
            exit(1);
        }
        else
            debugmsg("Parse error in %s.\n\nERROR: %s", path.c_str(), err.c_str());
    }
}

catajson::catajson(picojson::value val_in, std::string path_msg_in)
{
    val = val_in;
    path_msg = path_msg_in;
}

char catajson::as_char() const
{
    std::string temp = as_string();
    if (temp.size() != 1)
    {
        debugmsg("JSON warning at %s: value requested as char, string length is not 1", path_msg.c_str());
    }
    return temp[0];
}

std::string catajson::as_string() const
{
    if (val.is<std::string>())
    {
        return val.get<std::string>();
    }
    else
    {
        debugmsg("JSON error at %s: value is not a string", path_msg.c_str());
    }
    return "";
}

int catajson::as_int() const
{
    double temp = as_double();
    int ret = static_cast<int>(temp);
    if (ret != temp)
    {
        debugmsg("JSON warning at %s: value was requested as int, provided as double", path_msg.c_str());
    }
    return ret;
}

bool catajson::as_bool() const
{
    if (val.is<bool>())
    {
        return val.get<bool>();
    }
    else
    {
        debugmsg("JSON error at %s: value is not a boolean", path_msg.c_str());
    }
    return val.evaluate_as_boolean();
}

double catajson::as_double() const
{
    if (val.is<double>())
    {
        return val.get<double>();
    }
    else
    {
        debugmsg("JSON error at %s: value is not numeric", path_msg.c_str());
    }
    return 0;
}

std::set<std::string> catajson::as_tags()
{
    std::set<std::string> tags;
    if (is_array())
    {
        for (set_begin(); has_curr(); next())
        {
            tags.insert( curr().as_string() );
        }
    }
    else
    {
        // We should have gotten a string, if not an array.
        tags.insert( as_string() );
    }
    return tags;
}

catajson catajson::get(std::string key) const
{
    if (val.is<picojson::object>())
    {
        if (has(key))
        {
            return catajson(val.get(key), path_msg + "[" + key + "]");
        }
        else
        {
            debugmsg("JSON error at %s[%s]: value does not exist", path_msg.c_str(), key.c_str());
        }
    }
    else
    {
        debugmsg("JSON error at %s: value is not an object", path_msg.c_str());
    }
    return catajson();
}

catajson catajson::get(int index) const
{
    if (val.is<picojson::array>())
    {
        if (has(index))
        {
            std::stringstream text;
            text << path_msg << "[" << index << "]";
            return catajson(val.get(index), text.str());
        }
        else
        {
            debugmsg("JSON error at %s[%d]: value does not exist", path_msg.c_str(), index);
        }
    }
    else
    {
        debugmsg("JSON error at %s: value is not an array", path_msg.c_str());
    }
    return catajson();
}

bool catajson::has(std::string key) const
{
    if (val.is<picojson::object>())
    {
        return val.contains(key);
    }
    else
    {
        debugmsg("JSON error at %s: value is not an object", path_msg.c_str());
    }
    return false;
}

bool catajson::has(int index) const
{
    if (val.is<picojson::array>())
    {
        return val.contains(index);
    }
    else
    {
        debugmsg("JSON error at %s: value is not an array", path_msg.c_str());
    }
    return false;
}

bool catajson::is_char() const
{
    return is_string() && as_string().size() == 1;
}

bool catajson::is_string() const
{
    return val.is<std::string>();
}

bool catajson::is_number() const
{
    return val.is<double>();
}

bool catajson::is_bool() const
{
    return val.is<bool>();
}

bool catajson::is_object() const
{
    return val.is<picojson::object>();
}

bool catajson::is_array() const
{
    return val.is<picojson::array>();
}

void catajson::set_begin()
{
    if (val.is<picojson::array>())
    {
        ar_iter = val.get<picojson::array>().begin();
    }
    else if (val.is<picojson::object>())
    {
        obj_iter = val.get<picojson::object>().begin();
    }
    else
    {
        debugmsg("JSON error at %s: cannot call set_begin(), value is not an array or object", path_msg.c_str());
    }
}

void catajson::set_end()
{
    if (val.is<picojson::array>())
    {
        ar_iter = val.get<picojson::array>().end();
    }
    else if (val.is<picojson::object>())
    {
        obj_iter = val.get<picojson::object>().end();
    }
    else
    {
        debugmsg("JSON error at %s: cannot call set_end(), value is not an array or object", path_msg.c_str());
    }
}

void catajson::next()
{
    if (val.is<picojson::array>())
    {
        ++ar_iter;
    }
    else if (val.is<picojson::object>())
    {
        ++obj_iter;
    }
    else
    {
        debugmsg("JSON error at %s: tried to iterate an invalid value", path_msg.c_str());
    }
}

void catajson::prev()
{
    if (val.is<picojson::array>())
    {
        --ar_iter;
    }
    else if (val.is<picojson::object>())
    {
        --obj_iter;
    }
    else
    {
        debugmsg("JSON error at %s: tried to iterate an invalid value", path_msg.c_str());
    }
}

bool catajson::has_curr() const
{
    if (val.is<picojson::array>())
    {
        if (ar_iter >= val.get<picojson::array>().begin() && ar_iter < val.get<picojson::array>().end())
        {
            return true;
        }
    }
    else if (val.is<picojson::object>())
    {
        return has(obj_iter->first);
    }
    else
    {
        debugmsg("JSON error at %s: not an array or object", path_msg.c_str());
    }
    return false;
}

catajson catajson::curr() const
{
    if (val.is<picojson::array>())
    {
        const int count = val.get<picojson::array>().size();
        for (int i = 0; i < count; ++i)
        {
            if (val.get(i) == *ar_iter)
            {
                return get(i);
            }
        }
    }
    else if (val.is<picojson::object>())
    {
        return get(obj_iter->first);
    }
    else
    {
        debugmsg("JSON error at %s: not an array or object", path_msg.c_str());
    }
    return catajson();
}
