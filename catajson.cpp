#include "catajson.h"
#include "output.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef SAVEFILE_COMPRESSION
extern "C" {
    #include <bzlib.h>
}
#endif

catajson::catajson()
{
    path_msg = "";
}

catajson::catajson(std::string path, bool is_static)
{
    load(path, false);
}

catajson::catajson(picojson::value val_in, std::string path_msg_in)
{
    val = val_in;
    path_msg = path_msg_in;
}


int catajson::load(std::string path, bool compressed)
{
    #ifdef SAVEFILE_COMPRESSION
        if(compressed) {
            path = path + ".bz2";
        }
    #endif

    // The stringstream we'll read the file into.
    std::stringstream reader;

    #ifdef SAVEFILE_COMPRESSION
        if(compressed) {
            // bzip2 is a C library, so load the file using the C API.
            FILE* file = fopen(path.c_str() , "rb");
                    
            int bzError;
            BZFILE *bzFile = BZ2_bzReadOpen(&bzError, file, 0, 0, NULL, 0);
            if(bzError != BZ_OK) {
                debugmsg("Unable to bzip2 uncompress file %s\n", path.c_str());
                BZ2_bzReadClose(&bzError, bzFile);
                fclose(file);
                return 0;
            }

            // Slow and painful read of the file, that's the C API for you!
            static const int READ_SIZE = 1024;
            char readbuffer[READ_SIZE];
            while(1) {
                int number_bytes = BZ2_bzRead (&bzError, bzFile, readbuffer, READ_SIZE);
                if(bzError != BZ_OK && bzError != BZ_STREAM_END) {
                    debugmsg("Unable to bzip2 uncompress file %s\n", path.c_str());
                    BZ2_bzReadClose(&bzError, bzFile);
                    fclose(file);
                    return 0;
                }
                reader.write(readbuffer, number_bytes);
                if(number_bytes < READ_SIZE) {
                    break;
                }
            }

            BZ2_bzReadClose(&bzError, bzFile);
            fclose(file);
        } else {
    #endif

    // The stuff here only happens either if compressed == false, or SAVEFILE_COMPRESSION is off
    std::ifstream file;
    file.open(path.c_str());
    reader << file.rdbuf();
    file.close();

    #ifdef SAVEFILE_COMPRESSION
        }
    #endif

    // We have our file data in reader, now parse JSON.
    reader >> val;

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

    return 1;
}

int catajson::save(std::string path, bool compress)
{
    #ifdef SAVEFILE_COMPRESSION
        if(compress) {
            path = path + ".bz2";
        }
    #endif

    // First serialize the data into a string stream.
    std::stringstream data; data << val;

    // Prepare data for reading.
    data.seekg(0);

    // Now write to a file, possibly with compression.
    #ifdef SAVEFILE_COMPRESSION
        if(compress) {
            // bzip2 is a C library, so open the file using the C API.
            FILE* file = fopen(path.c_str() , "wb");
                    
            int bzError;
            BZFILE *bzFile = BZ2_bzWriteOpen(&bzError, file, 7, 0, 30);
            if(bzError != BZ_OK) {
                debugmsg("Unable to bzip2 compress file %s\n", path.c_str());
                BZ2_bzWriteClose(&bzError, bzFile, 0, NULL, NULL);
                fclose(file);
                return 0;
            }

            // Slow and painful read of the file, that's the C API for you!
            static const int READ_SIZE = 1024;
            char writebuffer[READ_SIZE];
            while(1) {
                data.read(writebuffer, READ_SIZE);
                int number_bytes = data.gcount();
                
                BZ2_bzWrite(&bzError, bzFile, writebuffer, number_bytes);
                if(bzError != BZ_OK) {
                    debugmsg("Unable to bzip2 compress file %s\n", path.c_str());
                    BZ2_bzWriteClose(&bzError, bzFile, 0, NULL, NULL);
                    fclose(file);
                    return 0;
                }
                if(number_bytes < READ_SIZE) {
                    break;
                }
            }

            BZ2_bzWriteClose(&bzError, bzFile, 0, NULL, NULL);
            fclose(file);
        } else {
    #endif

    // The stuff here only happens either if compressed == false, or SAVEFILE_COMPRESSION is off
    std::ofstream file;
    file.open(path.c_str());
    file << data.str();
    file.close();

    #ifdef SAVEFILE_COMPRESSION
        }
    #endif

    return 1;
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
