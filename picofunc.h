#ifndef _PICOFUNC_H_
#define _PICOFUNC_H_
static bool _testing_save=true;
static bool _really_testing_save=false;
static std::string errdump;

#define pv(x) picojson::value(x)

inline  bool picoverify() {
      std::string err = picojson::get_last_error();
      if (err.empty()) return true;
      if ( _testing_save ) popup("json error: %s",err.c_str());
      return false;
  }

inline  bool picobool(const picojson::object & obj, std::string key, bool & var) {
      picojson::object::const_iterator it = obj.find(key); 
      if(!picoverify()) return false;
      if(it != obj.end()) {
          if ( it->second.is<bool>() ) {
              if(!picoverify()) return false;
              bool tmp=(int)it->second.get<bool>();
              if(!picoverify()) return false;
              var = tmp;
              return true;
          }
          return false;
      } 
      return false;
  }

inline  bool picoint(const picojson::object & obj, std::string key, int & var) {
      picojson::object::const_iterator it = obj.find(key); 
      if(!picoverify()) return false;
      if(it != obj.end()) {
          if ( it->second.is<double>() ) {
              if(!picoverify()) return false;
              int tmp=(int)it->second.get<double>();
              if(!picoverify()) return false;
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
          return false;
      } 
      return false;
  }

inline bool picostring(const picojson::object & obj, std::string key, std::string & var) {
      picojson::object::const_iterator it = obj.find(key); 
      if(!picoverify()) return false;
      if(it != obj.end()) {
          if ( it->second.is<std::string>() ) {
              if(!picoverify()) return false;
              std::string tmp=(std::string)it->second.get<std::string>();
              if(!picoverify()) return false;
              var = tmp;
              return true;
          }
          return false;
      } 
      return false;
}

#endif
