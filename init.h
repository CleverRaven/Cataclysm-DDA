#ifndef _INIT_H_
#define _INIT_H_

#include "json.h"

#include <string>
#include <vector>

std::vector<std::string> listfiles(std::string const &dirname);
void load_object(JsonObject &jsobj);
void init_data_structures();

void load_json_dir(std::string const &dirname);
void load_all_from_json(JsonIn &jsin);

#endif // _INIT_H_
