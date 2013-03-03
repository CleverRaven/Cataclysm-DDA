#include "texthash.h"
#include <sstream>

bool text_hash::add(std::string key, std::string value)
{
 for (int i = 0; i < keys.size(); i++) {
  if (keys[i] == key)
   return false;	// Fail if key exists
 }
 keys.push_back(key);
 values.push_back(value);
 return true;		// Success
}

std::string text_hash::value(std::string key)
{
 for (int i = 0; i < keys.size(); i++) {
  if (keys[i] == key)
   return values[i];
 }
 std::stringstream fail;
 fail << "Key '" << key << "' not found.";
 return fail.str();
}

bool text_hash::has_key(std::string key)
{
 return true;
/*
 for (int i = 0; i < keys.size(); i++) {
  if (keys[i] == key)
   return true;
 }
 return false;
*/
}
