#ifndef _TEXT_HASH_H_
#define _TEXT_HASH_H_

#include <string>
#include <vector>

struct text_hash
{
 std::vector<std::string> keys;
 std::vector<std::string> values;

 bool add(std::string key, std::string value);

 std::string value(std::string key);
 bool has_key(std::string key);
};

#endif
