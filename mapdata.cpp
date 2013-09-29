#include "mapdata.h"
#include "color.h"

#include <ostream>

std::vector<ter_t> terlist;
std::vector<furn_t> furnlist;

std::ostream & operator<<(std::ostream & out, const submap * sm)
{
 out << "submap(";
 if( !sm )
 {
  out << "NULL)";
  return out;
 }

 out << "\n\tter:";
 for(int x = 0; x < SEEX; ++x)
 {
  out << "\n\t" << x << ": ";
  for(int y = 0; y < SEEY; ++y)
   out << sm->ter[x][y] << ", ";
 }

 out << "\n\titm:";
 for(int x = 0; x < SEEX; ++x)
 {
  for(int y = 0; y < SEEY; ++y)
  {
   if( !sm->itm[x][y].empty() )
   {
    for( std::vector<item>::const_iterator it = sm->itm[x][y].begin(),
      end = sm->itm[x][y].end(); it != end; ++it )
    {
     out << "\n\t("<<x<<","<<y<<") ";
     out << *it << ", ";
    }
   }
  }
 }

   out << "\n\t)";
 return out;
}

std::ostream & operator<<(std::ostream & out, const submap & sm)
{
 out << (&sm);
 return out;
}

void load_furniture(JsonObject &jsobj)
{
  furn_t new_furniture;

  new_furniture.name = _(jsobj.get_string("name").c_str());
  new_furniture.sym = jsobj.get_string("symbol").c_str()[0];

  bool has_color = jsobj.has_member("color");
  bool has_bgcolor = jsobj.has_member("bgcolor");
  if(has_color && has_bgcolor) {
    debugmsg("Found both color and bgcolor for %s, use only one of these.", new_furniture.name.c_str());
    new_furniture.color = c_white;
  } else if(has_color) {
    new_furniture.color = color_from_string(jsobj.get_string("color"));
  } else if(has_bgcolor) {
    new_furniture.color = bgcolor_from_string(jsobj.get_string("bgcolor"));
  } else {
    debugmsg("Furniture %s needs at least one of: color, bgcolor.", new_furniture.name.c_str());
  }

  new_furniture.movecost = jsobj.get_int("move_cost_mod");
  new_furniture.move_str_req = jsobj.get_int("required_str");

  JsonArray flags = jsobj.get_array("flags");
  while(flags.has_more()) {
    new_furniture.flags.insert(flags.next_string());
  }

  if(jsobj.has_member("examine_action")) {
    std::string function_name = jsobj.get_string("examine_action");
    new_furniture.examine = iexamine_function_from_string(function_name);
  } else {
    //If not specified, default to no action
    new_furniture.examine = iexamine_function_from_string("none");
  }

  furnlist.push_back(new_furniture);
}

void load_terrain(JsonObject &jsobj)
{
  ter_t new_terrain;

  new_terrain.name = _(jsobj.get_string("name").c_str());

  //Special case for the LINE_ symbols
  std::string symbol = jsobj.get_string("symbol");
  if("LINE_XOXO" == symbol) {
    new_terrain.sym = LINE_XOXO;
  } else if("LINE_OXOX" == symbol) {
    new_terrain.sym = LINE_OXOX;
  } else {
    new_terrain.sym = symbol.c_str()[0];
  }

  new_terrain.color = color_from_string(jsobj.get_string("color"));
  new_terrain.movecost = jsobj.get_int("move_cost");

  if(jsobj.has_member("trap")) {
    new_terrain.trap = trap_id_from_string(jsobj.get_string("trap"));
  } else {
    new_terrain.trap = tr_null;
  }

  JsonArray flags = jsobj.get_array("flags");
  while(flags.has_more()) {
    new_terrain.flags.insert(flags.next_string());
  }

  if(jsobj.has_member("examine_action")) {
    std::string function_name = jsobj.get_string("examine_action");
    new_terrain.examine = iexamine_function_from_string(function_name);
  } else {
    //If not specified, default to no action
    new_terrain.examine = iexamine_function_from_string("none");
  }

  terlist.push_back(new_terrain);

}
