#ifndef _HELPER_H_
#define _HELPER_H_
#include <string>
#include <sstream>
namespace helper
{
    int to_int(std::string str);
    std::string to_string_int(int i);
    std::string to_string_long(long i);
}
#endif
