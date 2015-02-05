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
    (void)startx; (void)starty; // FIXME: unused
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

bool internal::format_data::fix_bindings(const char c)
{
    if( bindings.find(c) != bindings.end() ) {
        return false;
    }
    bindings[c].reset( new statically_determine_terrain() );
    return true;
}

void formatted_set_simple(map* m, const int startx, const int starty, const char* cstr,
                       std::shared_ptr<internal::format_effect> ter_b, std::shared_ptr<internal::format_effect> furn_b,
                       const bool empty_toilets)
{
    internal::format_data tdata;
    internal::format_data fdata;
    ter_b->execute(tdata);
    furn_b->execute(fdata);

    tdata.fix_bindings(' ');
    fdata.fix_bindings(' ');

    const char* p = cstr;
    int x = startx;
    int y = starty;
    while (*p != 0) {
        if (*p == '\n') {
            y++;
            x = startx;
        } else {
            tdata.fix_bindings(*p);
            fdata.fix_bindings(*p);
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
}

std::shared_ptr<internal::format_effect> basic_bind(std::string characters, ...)
{
    std::string temp;
    for( auto &character : characters ) {
        if( character != ' ' ) {
            temp += character;
        }
    }
    characters = temp;

    std::vector<std::shared_ptr<internal::determine_terrain> > determiners;
    va_list vl;
    va_start(vl,characters);
    determiners.resize(characters.size());
    for( size_t i = 0; i < characters.size(); ++i ) {
        determiners[i].reset( new internal::statically_determine_terrain( (ter_id)va_arg(vl,int) ));
    }
    va_end(vl);
    return std::shared_ptr<internal::format_effect>(new internal::format_effect(characters, determiners));
}

std::shared_ptr<internal::format_effect> ter_str_bind(std::string characters, ...)
{
    std::string temp;
    for( auto &character : characters ) {
        if( character != ' ' ) {
            temp += character;
        }
    }
    characters = temp;

    std::vector<std::shared_ptr<internal::determine_terrain> > determiners;
    va_list vl;
    va_start(vl,characters);
    determiners.resize(characters.size());
    for( size_t i = 0; i < characters.size(); ++i ) {
        const std::string sid = va_arg(vl,char *);
        const int iid = ( termap.find( sid ) != termap.end() ? termap[ sid ].loadid : 0 );
        determiners[i].reset( new internal::statically_determine_terrain( (ter_id)iid ) );
    }
    va_end(vl);
    return std::shared_ptr<internal::format_effect>(new internal::format_effect(characters, determiners));
}

std::shared_ptr<internal::format_effect> furn_str_bind(std::string characters, ...)
{
    std::string temp;
    for( auto &character : characters ) {
        if( character != ' ' ) {
            temp += character;
        }
    }
    characters = temp;

    std::vector<std::shared_ptr<internal::determine_terrain> > determiners;
    va_list vl;
    va_start(vl,characters);
    determiners.resize(characters.size());
    for( size_t i = 0; i < characters.size(); ++i ) {
        const std::string sid = va_arg(vl,char *);
        const int iid = ( furnmap.find( sid ) != furnmap.end() ? furnmap[ sid ].loadid : 0 );
        determiners[i].reset( new internal::statically_determine_terrain( (ter_id)iid ) );
    }
    va_end(vl);
    return std::shared_ptr<internal::format_effect>(new internal::format_effect(characters, determiners));
}



std::shared_ptr<internal::format_effect> simple_method_bind(std::string characters, ...)
{
    std::string temp;
    for( auto &character : characters ) {
        if( character != ' ' ) {
            temp += character;
        }
    }
    characters = temp;

    std::vector<std::shared_ptr<internal::determine_terrain> > determiners;
    va_list vl;
    va_start(vl,characters);
    determiners.resize(characters.size());
    for( size_t i = 0; i < characters.size(); ++i ) {
        determiners[i].reset( new internal::determine_terrain_with_simple_method( va_arg(vl, internal::determine_terrain_with_simple_method::ter_id_func ) ));
    }
    va_end(vl);
    return std::shared_ptr<internal::format_effect>(new internal::format_effect(characters, determiners));
}

namespace internal
{
    format_effect::format_effect(std::string characters, std::vector<std::shared_ptr<determine_terrain> > &determiners)
        : characters( characters ), determiners( determiners )
    {
    }

    format_effect::~format_effect() {}

    void format_effect::execute(format_data& data)
    {
        for( size_t i = 0; i < characters.size(); ++i ) {
            data.bindings[characters[i]] = determiners[i];
        }
    }
}

}//END NAMESPACE mapf
