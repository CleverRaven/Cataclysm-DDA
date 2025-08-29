#pragma once
#ifndef CATA_SRC_MAPGENFORMAT_H
#define CATA_SRC_MAPGENFORMAT_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"

class map;

namespace mapf
{
template<typename ID>
class format_effect;

/**
 * Set terrain and furniture on the supplied map.
 * @param m The supplied map
 * @param ter_b,furn_b The lookup table for placing terrain / furniture
 *   (result of @ref ter_bind / @ref furn_bind).
 * @param cstr Contains the ASCII representation of the map. Each character in it represents
 *   one tile on the map. It will be looked up in \p ter_b and \p furn_b to get the terrain/
 *   furniture to place there (if that lookup returns a null id, nothing is set on the map).
 *   A newline character continues on the next line (resets `x` to \p startx and increments `y`).
 * @param start Coordinates in the map where to start drawing \p cstr.
 */
void formatted_set_simple( map *m, const point_bub_ms &start, const char *cstr,
                           const format_effect<ter_id> &ter_b, const format_effect<furn_id> &furn_b );

template<typename ID>
class format_effect
{
    private:
        std::string characters;
        std::vector<ID> determiners;

    public:
        format_effect( const std::string &chars,
                       std::vector<ID> dets );

        ID translate( char c ) const;
};

/**
 * The functions create a mapping of characters to ids, usable with @ref formatted_set_simple.
 * The first parameter must a string literal, containing the mapped characters.
 * Only every second character of it is mapped:
 * `"a b c"` maps `a` to the first id, `b` to the second and `c` to the third id.
 * The further parameters form an array of suitable size with ids to map to.
 *
 * \code
 * ter_bind( "a", t_dirt );
 * ter_bind( "a b", t_dirt, t_wall );
 * // This does not work (t_wall is not mapped to any character):
 * ter_bind( "a", t_dirt, t_wall );
 * // This does not work (character b is not mapped to any id):
 * ter_bind( "a b", t_dirt );
 * \endcode
 */
/**@{*/
template<size_t N, typename ...Args>
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
inline format_effect<ter_id> ter_bind( const char ( &characters )[N], Args... ids )
{
    // Note to self: N contains the 0-char at the end of a string literal!
    static_assert( N % 2 == 0, "list of characters to bind to must be odd, e.g. \"a b c\"" );
    static_assert( N / 2 == sizeof...( Args ),
                   "list of characters to bind to must match the size of the remaining arguments" );
    return format_effect<ter_id>( characters, { std::forward<Args>( ids )... } );
}

template<size_t N, typename ...Args>
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
inline format_effect<furn_id> furn_bind( const char ( &characters )[N], Args... ids )
{
    // Note to self: N contains the 0-char at the end of a string literal!
    static_assert( N % 2 == 0, "list of characters to bind to must be odd, e.g. \"a b c\"" );
    static_assert( N / 2 == sizeof...( Args ),
                   "list of characters to bind to must match the size of the remaining arguments" );
    return format_effect<furn_id>( characters, { std::forward<Args>( ids )... } );
}
/**@}*/

} //END NAMESPACE mapf

#endif // CATA_SRC_MAPGENFORMAT_H
