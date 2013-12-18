#include <iostream>
#include <map>

#include <string>

#include <cassert>
#include <stdarg.h>

#include "output.h"

#include "mapgenformat.h"
/*
 * Take array of struct { short; short } and spaw it on a map
 */

void formatted_set_incredibly_simple( map * m, const ter_furn_id data[], const int width, const int height, const int startx, const int starty, const int defter ) {
    for ( int y = 0; y < height; y++ ) {
        const int mul = y * height;
        for( int x = 0; x < width; x++ ) {
            const ter_furn_id tdata = data[ mul + x ];
            if ( tdata.furn != f_null ) {
                if ( tdata.ter != t_null ) {
                    m->set(x, y, tdata.ter, tdata.furn);
                } else if ( defter != -1 ) {
                    m->set(x, y, defter, tdata.furn);
                } else {
                    m->furn_set(x, y, tdata.furn);
                }
            } else if ( tdata.ter != t_null ) {
                m->ter_set(x, y, tdata.ter);
            } else if ( defter != -1 ) {
                m->ter_set(x, y, defter);
            }

        }
    }
}
/////////////////

namespace mapf
{

bool fix_bindings(internal::format_data &data, const char c)
{
    bool fixed = false;
    if (data.bindings.find(c) == data.bindings.end()) {
        data.bindings[c] = new internal::statically_determine_terrain();
        fixed = true;
    }
    return fixed;
}

void formatted_set_simple(map* m, const int startx, const int starty, const char* cstr,
                       internal::format_effect* ter_b, internal::format_effect* furn_b,
                       const bool empty_toilets)
{
    internal::format_data tdata;
    internal::format_data fdata;
    ter_b->execute(tdata);
    furn_b->execute(fdata);

    fix_bindings(tdata, ' ');
    fix_bindings(fdata, ' ');

    const char* p = cstr;
    int x = startx;
    int y = starty;
    while (*p != 0) {
        if (*p == '\n') {
            y++;
            x = startx;
        } else {
            fix_bindings(tdata, *p);
            fix_bindings(fdata, *p);
            ter_id ter = (ter_id)(*tdata.bindings[*p])(m, x, y);
            furn_id furn = (furn_id)(*fdata.bindings[*p])(m, x, y);
            if (ter != t_null) {
                m->ter_set(x, y, ter);
            }
            if (furn != f_null) {
                if (furn == f_toilet && !empty_toilets) {
                    m->place_toilet(x, y);
                } else {
                    m->furn_set(x, y, furn);
                }
            }
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

 fix_bindings(fdata, ' ');

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
   if (fix_bindings(fdata, *p))
   {
    debugmsg("No binding for \'%c.\'", *p);
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

internal::format_effect* ter_str_bind(std::string characters, ...)
{
 std::string temp;
 for(int i = 0; i < characters.size(); i++)
  if(characters[i] != ' ')
   temp += characters[i];
 characters = temp;

 std::vector<internal::determine_terrain*> determiners;
 va_list vl;
 va_start(vl,characters);
 for(int i = 0; i < characters.size(); i++) {
    const std::string sid = va_arg(vl,char *);
    const int iid = ( termap.find( sid ) != termap.end() ? termap[ sid ].loadid : 0 );
    determiners.push_back( new internal::statically_determine_terrain( (ter_id)iid ) );
 }
 va_end(vl);
 return new internal::format_effect(characters, determiners);
}

internal::format_effect* furn_str_bind(std::string characters, ...)
{
 std::string temp;
 for(int i = 0; i < characters.size(); i++)
  if(characters[i] != ' ')
   temp += characters[i];
 characters = temp;

 std::vector<internal::determine_terrain*> determiners;
 va_list vl;
 va_start(vl,characters);
 for(int i = 0; i < characters.size(); i++) {
    const std::string sid = va_arg(vl,char *);
    const int iid = ( furnmap.find( sid ) != furnmap.end() ? furnmap[ sid ].loadid : 0 );
    determiners.push_back( new internal::statically_determine_terrain( (ter_id)iid ) );
 }
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
