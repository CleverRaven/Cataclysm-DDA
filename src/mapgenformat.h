#ifndef MAPGENFORMAT_H
#define MAPGENFORMAT_H

#include <vector>
#include <memory>

#include "map.h"
/////

struct ter_furn_id;

/////
namespace mapf
{
namespace internal
{
template<typename ID>
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
                           internal::format_effect<ter_id> ter_b, internal::format_effect<furn_id> furn_b,
                           const bool empty_toilets = false );

internal::format_effect<ter_id> ter_bind( std::string characters, ... );
internal::format_effect<furn_id> furn_bind( std::string characters, ... );

// Anything specified in here isn't finalized
namespace internal
{
template<typename ID>
struct format_data {
    std::map<char, ID> bindings;
    bool fix_bindings( const char c );
};

// This class will become an interface in the future.
template<typename ID>
class format_effect
{
    private:
        std::string characters;
        std::vector<ID> determiners;

    public:
        format_effect( std::string characters,
                       std::vector<ID> &determiners );

        void execute( format_data<ID> &data );
};

} //END NAMESPACE mapf::internal

} //END NAMESPACE mapf

#endif
