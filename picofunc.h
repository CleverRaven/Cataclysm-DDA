#ifndef _PICOFUNC_H_
#define _PICOFUNC_H_
#include "picojson.h"
#include "enums.h"


#define pv(x) picojson::value(x)

bool picoverify();
bool picobool(const picojson::object & obj, std::string key, bool & var);
bool picoint(const picojson::object & obj, std::string key, int & var);
bool picostring(const picojson::object & obj, std::string key, std::string & var);
bool picouint(const picojson::object & obj, std::string key, unsigned int & var);
bool picopoint(const picojson::object & obj, std::string key, point & var);
bool picovector(picojson::object & obj, std::string key, std::vector<int> & var);

picojson::array * pgetarray(picojson::object & obj, std::string key);
picojson::object * pgetmap(picojson::object & obj, std::string key);

picojson::value json_wrapped_vector ( std::vector<int> val );
picojson::value json_wrapped_vector ( std::vector<std::string> val );

#endif
