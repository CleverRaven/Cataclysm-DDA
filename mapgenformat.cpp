#include <iostream>
#include <map>

#include <string>

#include <cassert>
#include <stdarg.h>

#include "output.h"

#include "mapgenformat.h"

namespace mapf
{

void formatted_set_terrain(map* m, const int startx, const int starty, const char* cstr, ...)
{
 internal::format_data fdata;
 va_list vl;
 va_start(vl,cstr);
 internal::format_effect* temp;
 while(temp = va_arg(vl,internal::format_effect*))
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
   ter_id id = (*fdata.bindings[*p])(m, x, y);
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
  determiners.push_back( new internal::determine_terrain_with_simple_method( va_arg(vl, ter_id (*)() ) ));
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
