#ifndef _INIT_H_
#define _INIT_H_

#include "json.h"

#include <string>
#include <vector>

void load_json_dir(std::string const &dirname);
std::vector<std::string> listfiles(std::string const &dirname);

void load_all_from_json(Jsin &jsin) throw (std::string);
bool load_object_from_json(std::string const &type, Jsin &jsin) throw (std::string);

#endif // _INIT_H_
