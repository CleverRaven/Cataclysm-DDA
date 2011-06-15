#include <string>
#include <vector>

struct text_hash
{
 std::vector<std::string> keys;
 std::vector<std::string> values;

 bool add(std::string key, std::string value);

 std::string value(std::string key);
};
