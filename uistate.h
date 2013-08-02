#ifndef _UISTATE_H_
#define _UISTATE_H_
/*
  centralized depot for trivial ui data such as sorting, string_input_popup history, etc.
  To use this, see the ****notes**** below
*/
struct uistatedata {
/**** declare your variable here. It can be anything, really *****/
  int adv_inv_leftsort;
  int adv_inv_rightsort;
  int adv_inv_leftarea;
  int adv_inv_rightarea;
  int adv_inv_leftindex;
  int adv_inv_rightindex;
  int adv_inv_leftpage;
  int adv_inv_rightpage;
  int adv_inv_last_popup_dest;
  bool debug_ranged;
  point adv_inv_last_coords;
  int last_inv_start, last_inv_sel;
  /* to save input history and make accessible via 'up', you don't need to edit this file, just run:
     output = string_input_popup(str, int, str, str, std::string("set_a_unique_identifier_here") );
  */

  std::map<std::string, std::vector<std::string>*> input_history; 

  bool _testing_save;
  bool _really_testing_save;
  std::string errdump;

  uistatedata() {
/**** this will set a default value on startup, however to save, see below ****/
      adv_inv_leftsort = 1;
      adv_inv_rightsort = 1;
      adv_inv_leftarea = 5;
      adv_inv_rightarea = 0;
      adv_inv_leftindex = 0;
      adv_inv_rightindex=0;
      adv_inv_leftpage=0;
      adv_inv_rightpage=0;
      adv_inv_last_popup_dest=0;
      adv_inv_last_coords.x=-999;
      adv_inv_last_coords.y=-999;
      last_inv_start = -2;
      last_inv_sel = -2;
      debug_ranged = false;  
      // internal stuff
      _testing_save = true;        // internal: whine on json errors. set false if no complaints in 2 weeks.
      _really_testing_save = false; // internal: spammy
      errdump = "";                // also internal: log errors en masse
  };

  std::vector<std::string> * gethistory(std::string id) {
      std::map<std::string, std::vector<std::string>*>::iterator it=input_history.find(id);
      if(it == input_history.end() || it->second == NULL ) {
          input_history[id]=new std::vector<std::string>;
          it=input_history.find(id);
      }
      return it->second;
  }

  virtual picojson::value save_data() {
      const int input_history_save_max = 25;
      std::map<std::string, picojson::value> data;
      
/**** if you want to save whatever so it's whatever when the game is started next, declare here and.... ****/
      data[std::string("adv_inv_leftsort")] = picojson::value(adv_inv_leftsort);
      data[std::string("adv_inv_rightsort")] = picojson::value(adv_inv_rightsort);
      data[std::string("adv_inv_leftarea")] = picojson::value(adv_inv_leftarea);
      data[std::string("adv_inv_rightarea")] = picojson::value(adv_inv_rightarea);
      data[std::string("adv_inv_last_popup_dest")] = picojson::value(adv_inv_last_popup_dest);

      std::map<std::string, picojson::value> histmap;
      for(std::map<std::string, std::vector<std::string>*>::iterator it = input_history.begin(); it != input_history.end(); ++it ) {
          if(it->second != NULL ) {
              std::vector<picojson::value> hist;
              int save_start=0;
              if  ( it->second->size() > input_history_save_max ) {
                  save_start = it->second->size() - input_history_save_max;
              }
              for(std::vector<std::string>::iterator hit = it->second->begin()+save_start; hit != it->second->end(); ++hit ) {
                  hist.push_back(picojson::value( *hit ));
              }
              histmap[std::string(it->first)] = picojson::value(hist);
          }
      }
      data[std::string("input_history")] = picojson::value(histmap);
      return picojson::value(data);
  };

  bool load(picojson::value parsed) {
      std::string out="";
      if(parsed.is<picojson::object>() ) {
          const picojson::object& data = parsed.get<picojson::object>();

/**** here ****/
          picoint(data,"adv_inv_leftsort",               adv_inv_leftsort);
          picoint(data,"adv_inv_rightsort",              adv_inv_rightsort);
          picoint(data,"adv_inv_leftarea",               adv_inv_leftarea);
          picoint(data,"adv_inv_rightarea",              adv_inv_rightarea);
          picoint(data,"adv_inv_last_popup_dest",        adv_inv_last_popup_dest);

          picojson::object::const_iterator hmit = data.find("input_history");
          if ( ! picoverify() ) return false;
          if ( hmit != data.end() ) {
            if ( hmit->second.is<picojson::object>() ) {
               const picojson::object& hmdata = hmit->second.get<picojson::object>();
               for (picojson::object::const_iterator hit = hmdata.begin(); hit != hmdata.end(); ++hit) {
                  if ( hit->second.is<picojson::array>() ) {
                     std::string hkey=hit->first;
                     std::vector<std::string> * v = gethistory(hkey);
                     const picojson::array& ht = hit->second.get<picojson::array>();
                     if(!picoverify()) continue;
                     v->clear();
                     for (picojson::array::const_iterator hent=ht.begin(); hent != ht.end(); ++hent) {
                        if( (*hent).is<std::string>() ) {
                           std::string tmpstr=(*hent).get<std::string>();
                           if(!picoverify()) continue;
                           v->push_back(tmpstr);
                        }
                     }
                  }
               }
               return true;
            } else {
               errdump += "\ninput_history is not an object";
               
               return false;
            }
          }
          return true;
      }
      return false;
  };

  std::string save() {
      return this->save_data().serialize();
  };
  
  bool picoverify() {
      std::string err = picojson::get_last_error();
      if (err.empty()) return true;
      errdump += "\n";
      errdump += err;
      if ( _testing_save ) popup("json error: %s",err.c_str());
      return false;
  }

  bool picobool(const picojson::object & obj, std::string key, bool & var) {
      picojson::object::const_iterator it = obj.find(key); 
      if(!picoverify()) return false;
      if(it != obj.end()) {
          if ( it->second.is<double>() ) {
              if(!picoverify()) return false;
              int tmp=(int)it->second.get<double>();
              if(!picoverify()) return false;
              var = tmp;
              return true;
          }
          return false;
      } 
      return false;
  }

  bool picoint(const picojson::object & obj, std::string key, int & var) {
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

};
extern uistatedata uistate;
#endif
