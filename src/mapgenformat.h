#ifndef MAPGENFORMAT_H
#define MAPGENFORMAT_H

#include "int_id.h"

#include <vector>
#include <string>

struct ter_t;
using ter_id = int_id<ter_t>;
struct furn_t;
using furn_id = int_id<furn_t>;
class map;

namespace mapf
{
template<typename ID>
class format_effect;

/** The return statement for this method is not finalized.
 * The following things will be acces from this return statements.
 * - the region of points set by given character
 * - the region of points set to a given terrian id
 * - possibly some other stuff
 * You will have specify the values you want to track with a parameter.
 */
void formatted_set_simple( map *m, const int startx, const int starty, const char *cstr,
                           format_effect<ter_id> ter_b, format_effect<furn_id> furn_b,
                           const bool empty_toilets = false );

// This class will become an interface in the future.
template<typename ID>
class format_effect
{
    private:
        std::string characters;
        std::vector<ID> determiners;

    public:
        format_effect( std::string characters,
                       std::vector<ID> determiners );

        ID translate( char c ) const;
};

template<size_t N, typename ...Args>
inline format_effect<ter_id> ter_bind( const char ( &characters )[N], Args... ids )
{
    // Note to self: N contains the 0-char at the end of a string literal!
    static_assert( N % 2 == 0, "list of characters to bind to must be odd, e.g. \"a b c\"" );
    static_assert( N / 2 == sizeof...( Args ),
                   "list of characters to bind to must match the size of the remaining arguments" );
    return format_effect<ter_id>( characters, { std::forward<Args>( ids )... } );
}

template<size_t N, typename ...Args>
inline format_effect<furn_id> furn_bind( const char ( &characters )[N], Args... ids )
{
    // Note to self: N contains the 0-char at the end of a string literal!
    static_assert( N % 2 == 0, "list of characters to bind to must be odd, e.g. \"a b c\"" );
    static_assert( N / 2 == sizeof...( Args ),
                   "list of characters to bind to must match the size of the remaining arguments" );
    return format_effect<furn_id>( characters, { std::forward<Args>( ids )... } );
}

} //END NAMESPACE mapf

#endif
