#ifndef MAPGENFORMAT_H
#define MAPGENFORMAT_H

#include <vector>
#include <memory>

#include "map.h"
/////

struct ter_furn_id;

void formatted_set_incredibly_simple(
    map *m, const ter_furn_id data[], int width, int height, int startx, int starty, ter_id defter
);

/////
namespace mapf
{
namespace internal
{
class format_effect;
}
/** The return statement for this method is not finalized.
 * The following things will be acces from this return statements.
 * - the region of points set by given character
 * - the region of points set to a given terrian id
 * - possibly some other stuff
 * You will have specify the values you want to track with a parameter.
 */
void formatted_set_simple( map *m, const int startx, const int starty, const char *cstr,
                           std::shared_ptr<internal::format_effect> ter_b, std::shared_ptr<internal::format_effect> furn_b,
                           const bool empty_toilets = false );

std::shared_ptr<internal::format_effect> basic_bind( std::string characters, ... );
std::shared_ptr<internal::format_effect> ter_str_bind( std::string characters, ... );
std::shared_ptr<internal::format_effect> furn_str_bind( std::string characters, ... );

// Anything specified in here isn't finalized
namespace internal
{
class statically_determine_terrain;
using determine_terrain = statically_determine_terrain;
struct format_data {
    std::map<char, std::shared_ptr<determine_terrain> > bindings;
    bool fix_bindings( const char c );
};

// This class will become an interface in the future.
class format_effect
{
    private:
        std::string characters;
        std::vector< std::shared_ptr<determine_terrain> > determiners;

    public:
        format_effect( std::string characters,
                       std::vector<std::shared_ptr<determine_terrain> > &determiners );

        void execute( format_data &data );
};

class statically_determine_terrain
{
    private:
        int id;
    public:
        statically_determine_terrain() : id( 0 ) {}
        statically_determine_terrain( int pid ) : id( pid ) {}
        ~statically_determine_terrain() {}
        int operator()( map *, const int /*x*/, const int /*y*/ ) {
            return id;
        }
};

} //END NAMESPACE mapf::internal

} //END NAMESPACE mapf

#endif
