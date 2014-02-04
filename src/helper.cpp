#include "helper.h"
namespace helper
{
    int to_int(std::string str)
    {
        std::istringstream buffer(str);
        int value;
        buffer >> value;
        if(str != to_string(value)) //if the converted value is different then return 0 (similar to atoi)
        {
            return 0;
        }
        return value;
    }
    std::string to_string(int i)
    {
        std::stringstream sstr;
        sstr << i;
        std::string str1 = sstr.str();
        return str1;
    }
}
