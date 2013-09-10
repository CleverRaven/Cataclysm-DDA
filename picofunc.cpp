#include "picofunc.h"
#include "output.h"
#include <stdlib.h>
#include <map>
#include <string>
#include <vector>
#include "debug.h"

static bool _testing_save=true;
static bool _really_testing_save=false;
/*
 * automagically whine on parsing errors
 */
static bool _log_failures=false;//true;
#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

bool picoverify() {
      std::string err = picojson::get_last_error();
      if (err.empty()) return true;
      if ( _testing_save ) popup("json error: %s",err.c_str());
      return false;
  }

/////////////////////////////////////
/*
 * helper functions for extracting data from picojson::object structures.
 *
 * These do -not- return the values found; they return false if not found or
 * if it's the wrong type. If found, they will set the 'var' reference argument. Ex:
 *
 * json: {foo:"bar","variablename":23} -> decoded to 'data'
 *
 * int variablename = -1; int anothervariablename = 12345;
 * bool return1 = picoint(data, "variablename", variablename);
 * bool return2 = picoint(data, "anothervariablename", anothervariablename);
 *
 * return1: true, variablename: 23, return2: false, anothervariablename: 12345
 */
bool picobool(const picojson::object & obj, std::string key, bool & var) {
      picojson::object::const_iterator it = obj.find(key); 
      if(it != obj.end()) {
          if ( it->second.is<bool>() ) {
              bool tmp=(int)it->second.get<bool>();
              var = tmp;
              return true;
          }
      } 
      if (_log_failures==true) dbg(D_INFO) << "json find falure: "<<key<<"]";
      return false;
}

/*
 * int
 */
bool picoint(const picojson::object & obj, std::string key, int & var) {
      picojson::object::const_iterator it = obj.find(key); 
      if(it != obj.end()) {
          if ( it->second.is<double>() ) {
              int tmp=(int)it->second.get<double>();
              if (tmp != it->second.get<double>() ) {
                  if(_testing_save) {
                      popup("key %s %d != %f",key.c_str(),tmp,it->second.get<double>());
                  }
              } else {
                  var = tmp;
                  if(_really_testing_save) {
                      popup("OK %s %d == %f == %d",key.c_str(),tmp,it->second.get<double>(),tmp);
                  }
                  return true;
              }
          }
      } 
      if (_log_failures==true) dbg(D_INFO) << "json find falure: "<<key<<"]";
      return false;
  }

/*
 * std::string
 */
bool picostring(const picojson::object & obj, std::string key, std::string & var) {
      picojson::object::const_iterator it = obj.find(key); 
      if(it != obj.end()) {
          if ( it->second.is<std::string>() ) {
              std::string tmp=(std::string)it->second.get<std::string>();
              var = tmp;
              return true;
          }
      } 
      if (_log_failures==true) dbg(D_INFO) << "json find falure: "<<key<<"]";
      return false;
}

/*
 * unsigned int
 */
bool picouint(const picojson::object & obj, std::string key, unsigned int & var) {
      picojson::object::const_iterator it = obj.find(key);
      if(it != obj.end()) {
          if ( it->second.is<double>() ) {
              unsigned int tmp=it->second.get<double>();
              if (tmp != it->second.get<double>() ) {
                  if(_testing_save) {
                      popup("key %s %d != %f",key.c_str(),tmp,it->second.get<double>());
                  }
              } else {
                  var = tmp;
                  if(_really_testing_save) {
                      popup("OK %s %d == %f == %d",key.c_str(),tmp,it->second.get<double>(),tmp);
                  }
                  return true;
              }
          }
      }
      if (_log_failures==true) dbg(D_INFO) << "json find falure: "<<key<<"]";
      return false;
  }

/*
 * point
 */
bool picopoint(const picojson::object & obj, std::string key, point & var) {
    picojson::object::const_iterator it = obj.find(key);
    if ( it != obj.end() && it->second.is<picojson::array>() && it->second.get<picojson::array>().size() == 2 ) {
        if ( it->second.get<picojson::array>()[0].is<double>() &&
            it->second.get<picojson::array>()[1].is<double>()
        ) {
             var.x = int ( it->second.get<picojson::array>()[0].get<double>() );
             var.y = int ( it->second.get<picojson::array>()[1].get<double>() );
             return true;
        }
    }
    if (_log_failures==true) dbg(D_INFO) << "json find falure: "<<key<<"]";
    return false;
}

/*
 * std::vector<int>
 */
bool picovector(picojson::object & obj, std::string key, std::vector<int> & var) {
    picojson::object::iterator pfind = obj.find(key);
    if ( pfind != obj.end() && pfind->second.is<picojson::array>()) {
        picojson::array & parray = pfind->second.get<picojson::array>();
        var.clear();
        for( picojson::array::iterator pit = parray.begin(); pit != parray.end(); ++pit) {
            if( (*pit).is<double>() ) {
                var.push_back( (*pit).get<double>() );
            }
        }
    }
    return false;
}
/*
 * ptr to wrapped array
 */
picojson::array * pgetarray(picojson::object & obj, std::string key) {
    picojson::object::iterator pfind = obj.find(key);
    if ( pfind != obj.end() && pfind->second.is<picojson::array>()) {
        return &pfind->second.get<picojson::array>();
    }
    return NULL;
}

/*
 * ptr to wrapped object
 */
picojson::object * pgetmap(picojson::object & obj, std::string key) {
    picojson::object::iterator pfind = obj.find(key);
    if ( pfind != obj.end() && pfind->second.is<picojson::object>()) {
        return &pfind->second.get<picojson::object>();
    }
    return NULL;
}

/////////////////////////////////////
///// helpers for saving. Since picojson requires implicit re-casting unless improved upon

/*
 * Take a vector<int> and spit out picojson::value ( vector<picojson::value> ), which
 * becomes "[1, 2, 3]" after serialize()
 */
picojson::value json_wrapped_vector ( std::vector<int> val ) {
    std::vector<picojson::value> ptmpvect;
    for (std::vector<int>::iterator iter = val.begin(); iter != val.end(); ++iter) {
        ptmpvect.push_back( pv ( *iter ) );
    }
    return pv( ptmpvect );
}

picojson::value json_wrapped_vector ( std::vector<std::string> val ) {
    std::vector<picojson::value> ptmpvect;
    for (std::vector<std::string>::iterator iter = val.begin(); iter != val.end(); ++iter) {
        ptmpvect.push_back( pv ( *iter ) );
    }
    return pv( ptmpvect );
}

