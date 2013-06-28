#include <iostream>
#include <map>

#include <string>

#include <cassert>
#include <stdarg.h>

#include "output.h"

#include "mapgenformat.h"

namespace mapf
{

void formatted_set_simple(map* m, const int startx, const int starty, const char* cstr,
                       internal::format_effect* ter_b, internal::format_effect* furn_b)
{
 internal::format_data tdata;
 internal::format_data fdata;
 ter_b->execute(tdata);
 furn_b->execute(fdata);

 if (tdata.bindings.find(' ') == tdata.bindings.end())
  tdata.bindings[' '] = new internal::statically_determine_terrain();
 if (fdata.bindings.find(' ') == fdata.bindings.end())
  fdata.bindings[' '] = new internal::statically_determine_terrain();

 const char* p = cstr;
 int x = startx;
 int y = starty;
 while (*p != 0) {
  if (*p == '\n') {
   y++;
   x = startx;
  }
  else {
   if (tdata.bindings.find(*p) == tdata.bindings.end())
    tdata.bindings[*p] = new internal::statically_determine_terrain();
   if (fdata.bindings.find(*p) == fdata.bindings.end())
    fdata.bindings[*p] = new internal::statically_determine_terrain();
   ter_id ter = (ter_id)(*tdata.bindings[*p])(m, x, y);
   furn_id furn = (furn_id)(*fdata.bindings[*p])(m, x, y);
   if (ter != t_null)
    m->ter_set(x, y, ter);
   if (furn != f_null)
    m->furn_set(x, y, furn);
   x++;
  }
  p++;
 }
 for(std::map<char, internal::determine_terrain*>::iterator it = fdata.bindings.begin(); it != fdata.bindings.end(); ++it)
 {
  delete it->second;
 }
}

void formatted_set_terrain(map* m, const int startx, const int starty, const char* cstr, ...)
{
 internal::format_data fdata;
 va_list vl;
 va_start(vl,cstr);
 internal::format_effect* temp;
 while((temp = va_arg(vl,internal::format_effect*)))
 {
  temp->execute(fdata);
  delete temp;
 }

 if(fdata.bindings.find(' ') == fdata.bindings.end())
  fdata.bindings[' '] = new internal::statically_determine_terrain();

 va_end(vl);

 const char* p = cstr;
 int x = startx;
 int y = starty;
 while(*p != 0) {
  if(*p == '\n') {
   y++;
   x = startx;
  }
  else {
   if(fdata.bindings.find(*p) == fdata.bindings.end())
   {
    debugmsg("No binding for \'%c.\'", *p);
    fdata.bindings[*p] = new internal::statically_determine_terrain();
   }
   ter_id id = (ter_id)(*fdata.bindings[*p])(m, x, y);
   if(id != t_null)
    m->ter_set(x, y, id);
   x++;
  }
  p++;
 }
 for(std::map<char, internal::determine_terrain*>::iterator it = fdata.bindings.begin(); it != fdata.bindings.end(); ++it)
 {
  delete it->second;
 }
}

internal::format_effect* basic_bind(std::string characters, ...)
{
 std::string temp;
 for(int i = 0; i < characters.size(); i++)
  if(characters[i] != ' ')
   temp += characters[i];
 characters = temp;

 std::vector<internal::determine_terrain*> determiners;
 va_list vl;
 va_start(vl,characters);
 for(int i = 0; i < characters.size(); i++)
  determiners.push_back( new internal::statically_determine_terrain( (ter_id)va_arg(vl,int) ));
 va_end(vl);
 return new internal::format_effect(characters, determiners);
}

internal::format_effect* simple_method_bind(std::string characters, ...)
{
 std::string temp;
 for(int i = 0; i < characters.size(); i++)
  if(characters[i] != ' ')
   temp += characters[i];
 characters = temp;

 std::vector<internal::determine_terrain*> determiners;
 va_list vl;
 va_start(vl,characters);
 for(int i = 0; i < characters.size(); i++)
  determiners.push_back( new internal::determine_terrain_with_simple_method( va_arg(vl, internal::determine_terrain_with_simple_method::ter_id_func ) ));
 va_end(vl);
 return new internal::format_effect(characters, determiners);
}

internal::format_effect* end()
{
 return NULL;
}

namespace internal
{
 format_effect::format_effect(std::string characters, std::vector<determine_terrain*> determiners)
 {
  this->characters = characters;
  this->determiners = determiners;
 }

 format_effect::~format_effect() {}

 void format_effect::execute(format_data& data)
 {
  for(int i = 0; i < characters.size(); i++)
   data.bindings[characters[i]] = determiners[i];
 }
}

}//END NAMESPACE mapf
