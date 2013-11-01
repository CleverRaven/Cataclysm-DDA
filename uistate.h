#ifndef _UISTATE_H_
#define _UISTATE_H_
#include "picofunc.h"
/*
  centralized depot for trivial ui data such as sorting, string_input_popup history, etc.
  To use this, see the ****notes**** below
*/
struct uistatedata {
/**** declare your variable here. It can be anything, really *****/
  int wishitem_selected;
  int wishmutate_selected;
  int wishmonster_selected;
  int iuse_knife_selected;
  int adv_inv_leftsort;
  int adv_inv_rightsort;
  int adv_inv_leftarea;
  int adv_inv_rightarea;
  int adv_inv_leftindex;
  int adv_inv_rightindex;
  int adv_inv_leftpage;
  int adv_inv_rightpage;
  int adv_inv_last_popup_dest;
  std::string adv_inv_leftfilter;
  std::string adv_inv_rightfilter;

  bool editmap_nsa_viewmode;        // true: ignore LOS and lighting
  bool debug_ranged;
  point adv_inv_last_coords;
  int last_inv_start, last_inv_sel;
  int list_item_mon;
  /* to save input history and make accessible via 'up', you don't need to edit this file, just run:
     output = string_input_popup(str, int, str, str, std::string("set_a_unique_identifier_here") );
  */

  std::map<std::string, std::vector<std::string>*> input_history; 

  std::map<std::string, std::string> lastreload; // last typeid used when reloading ammotype

  bool _testing_save;
  bool _really_testing_save;
  std::string errdump;

  uistatedata() {
/**** this will set a default value on startup, however to save, see below ****/
      wishitem_selected = 0;
      wishmutate_selected = 0;
      wishmonster_selected = 0;
      iuse_knife_selected = 0;
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
      adv_inv_leftfilter="";
      adv_inv_rightfilter="";
      editmap_nsa_viewmode = false;
      last_inv_start = -2;
      last_inv_sel = -2;
      list_item_mon = 1;
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
      data[std::string("editmap_nsa_viewmode")] = picojson::value(editmap_nsa_viewmode);
      data[std::string("list_item_mon")] = picojson::value(list_item_mon);

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
      if(parsed.is<picojson::object>() ) {
          const picojson::object& data = parsed.get<picojson::object>();

/**** here ****/
          picoint(data,"adv_inv_leftsort",               adv_inv_leftsort);
          picoint(data,"adv_inv_rightsort",              adv_inv_rightsort);
          picoint(data,"adv_inv_leftarea",               adv_inv_leftarea);
          picoint(data,"adv_inv_rightarea",              adv_inv_rightarea);
          picoint(data,"adv_inv_last_popup_dest",        adv_inv_last_popup_dest);
          picobool(data,"editmap_nsa_viewmode",          editmap_nsa_viewmode);
          picoint(data,"list_item_mon",                  list_item_mon);

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

};
extern uistatedata uistate;
#endif
