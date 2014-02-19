#ifndef _UISTATE_H_
#define _UISTATE_H_
#include "json.h"
/*
  centralized depot for trivial ui data such as sorting, string_input_popup history, etc.
  To use this, see the ****notes**** below
*/
class uistatedata : public JsonSerializer, public JsonDeserializer
{
public:
/**** declare your variable here. It can be anything, really *****/
  int wishitem_selected;
  int wishmutate_selected;
  int wishmonster_selected;
  int iuse_knife_selected;
  int iexamine_atm_selected;
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

  uistatedata() {
/**** this will set a default value on startup, however to save, see below ****/
      wishitem_selected = 0;
      wishmutate_selected = 0;
      wishmonster_selected = 0;
      iuse_knife_selected = 0;
      iexamine_atm_selected = 0;
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
  };

  std::vector<std::string> * gethistory(std::string id) {
      std::map<std::string, std::vector<std::string>*>::iterator it=input_history.find(id);
      if(it == input_history.end() || it->second == NULL ) {
          input_history[id]=new std::vector<std::string>;
          it=input_history.find(id);
      }
      return it->second;
  }

    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const
    {
        const int input_history_save_max = 25;
        json.start_object();

/**** if you want to save whatever so it's whatever when the game is started next, declare here and.... ****/
        json.member("adv_inv_leftsort", adv_inv_leftsort);
        json.member("adv_inv_rightsort", adv_inv_rightsort);
        json.member("adv_inv_leftarea", adv_inv_leftarea);
        json.member("adv_inv_rightarea", adv_inv_rightarea);
        json.member("adv_inv_last_popup_dest", adv_inv_last_popup_dest);
        json.member("editmap_nsa_viewmode", editmap_nsa_viewmode);
        json.member("list_item_mon", list_item_mon);

        json.member("input_history");
        json.start_object();
        std::map<std::string,std::vector<std::string>*>::const_iterator it;
        for (it = input_history.begin(); it != input_history.end(); ++it) {
            if (it->second == NULL) {
                continue;
            }
            json.member(it->first);
            json.start_array();
            int save_start = 0;
            if (it->second->size() > input_history_save_max) {
                save_start = it->second->size() - input_history_save_max;
            }
            for (std::vector<std::string>::const_iterator hit = it->second->begin()+save_start; hit != it->second->end(); ++hit ) {
                json.write(*hit);
            }
            json.end_array();
        }
        json.end_object(); // input_history

        json.end_object();
    };

    void deserialize(JsonIn &jsin)
    {
        JsonObject jo = jsin.get_object();
/**** here ****/
        jo.read("adv_inv_leftsort", adv_inv_leftsort);
        jo.read("adv_inv_rightsort", adv_inv_rightsort);
        jo.read("adv_inv_leftarea", adv_inv_leftarea);
        jo.read("adv_inv_rightarea", adv_inv_rightarea);
        jo.read("adv_inv_last_popup_dest", adv_inv_last_popup_dest);
        jo.read("editmap_nsa_viewmode", editmap_nsa_viewmode);
        jo.read("list_item_mon", list_item_mon);

        JsonObject inhist = jo.get_object("input_history");
        std::set<std::string> inhist_members = inhist.get_member_names();
        for (std::set<std::string>::iterator it = inhist_members.begin();
                it != inhist_members.end(); ++it) {
            JsonArray ja = inhist.get_array(*it);
            std::vector<std::string> *v = gethistory(*it);
            v->clear();
            while (ja.has_more()) {
                v->push_back(ja.next_string());
            }
        }
    };
};
extern uistatedata uistate;
#endif
