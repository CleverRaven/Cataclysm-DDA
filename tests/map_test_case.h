#ifndef CATA_TESTS_MAP_TEST_CASE_H
#define CATA_TESTS_MAP_TEST_CASE_H

#include <functional>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "coordinates.h"
#include "point.h"
#include "type_id.h"

/**
 * Framework that simplifies writing tests that involve setting up 2d (or limited 3d) maps.
 * Features:
 *  * set up map using 2d array of characters by specifying configurable behavior for character/coords
 *  * anchor input 2d array to the desired `map` coords
 *  * test with all basic transformations of the source 2d array
 *  * log per-tile info (formatted into table)
 *  * validate results against another 2d array of characters
 *
 *  Usage:
 *   (see vision_test.cpp for examples)
 *   * create instance of the `map_test_case`
 *   * initialize `setup` and `expected_results` (must have same dimensions)
 *   * set `anchor_char` (or use `set_anchor_char_from`), char must be uniquely present in the `setup`
 *   * set `anchor_map_pos` to the desired map coordinates to be aligned with `anchor_char` position
 *   * (optionally) apply transformations (`transpose`, `reflect_*`) can be done at any point after
 *        `setup` and `expected_results` initialization. However, note, transformations might change
 *        the results of getter methods.
 *        Alternatively, use `generate_transform_combinations` inside `catch2` test to automatically
 *        generate and test all transformations.
 *   * use `map_tiles` variants to execute a function for each tile
 *   * use function builders from `map_test_case_common` namespace to construct desired behavior
 *        to use with `for_each_tile`
 *   * use `map_test_case_common::printers` to print common debug information about the map area where
 *        `map_test_case` is mapped to
 */
class map_test_case
{
    public:
        // struct that is supplied as an argument to the per-tile functions (see below)
        struct tile {
            // current char from the `setup`
            char setup_c;
            // current char from `expected_results`
            char expect_c;
            // current point in map coords
            tripoint_bub_ms p;
            // current point in local coords [0..width) [0..height)
            point p_local;
        };

        /**
         *  Specifies the input 2d layout as a char matrix.
         */
        std::vector<std::string> setup;
        /**
         *  Specifies the expected results as a char matrix
         */
        std::vector<std::string> expected_results;

        /**
         * Char (must be uniquely present in the map_test_case::setup) that defines how the
         * `map_test_case` coordinates are mapped onto map coordinates.
         * When `anchor_char` is not set, top left corner (0,0) of the `setup` is used as an anchor.
         *
         * When using any `for_each_tile` variant, calculated (map) coordinates (`tile::p`) are such that
         * `anchor_char` point within `setup` is aligned with the map coords stored in `anchor_map_pos`.
         */
        std::optional<char> anchor_char = std::nullopt;

        /**
         *  The 'anchor' coordinates on the map (in map coordinates).
         *  For example, it can be the center of the map or player's coordinates.
         *  This test will align `anchor_char` (in `setup`) with given map coordinates.
         */
        tripoint_bub_ms anchor_map_pos = tripoint_bub_ms::zero;

        /**
         * Invokes callback for each tile of the input map.
         * @param callback
         */
        void for_each_tile( const std::function<void( tile )> &callback );

        /**
         * Invokes callback for each tile of the input map.
         * Gathers info written in out stream and arranges it into 2d grid which is returned.
         */
        std::vector<std::vector<std::string>> map_tiles_str( const
                                           std::function<void( tile, std::ostringstream &out )> &callback );

        /**
         * Invokes callback for each tile of the input map.
         * Collects results of the invocation in 2d vector.
         */
        template<typename R>
        std::vector<std::vector<R>> map_tiles( const std::function<R( tile )> &callback );

        // input major (first) dimension
        int get_height() const;

        // input minor dimension
        int get_width() const;

        // returns calculated "origin" point, i.e. shift relative to the map
        tripoint_bub_ms get_origin();

        // automatically set `anchor_char` from the list of given chars to the one that is present in the `setup`
        void set_anchor_char_from( const std::set<char> &chars );

        // WARNING: transformations might change later results of getters (`get_origin`, `get_height`, etc.)

        // transposes `setup` and `expected_results`
        void transpose();

        // reverses rows in `setup` and `expected_results`
        void reflect_x();

        // reverses columns in `setup` and `expected_results`
        void reflect_y();

        /**
        * Generates (using catch2 GENERATE) every combination of three transformations and applies them to the test.
        * See https://github.com/catchorg/Catch2/blob/master/docs/generators.md
        * Test will be invoked for each combination of transformation.
        * @param t test case to apply transformations to
        * @return string info about applied transformations
        */
        std::string generate_transform_combinations();

        /**
         * Ensures that `map_test_case` is set up correctly by
         * validating given point in map coordinates against `map_test_case::anchor_map_pos`
         * (and performing other sanity checks)
         * @param p tripoint in map coordinates that is expected to align with the `anchor_map_pos`
         */
        void validate_anchor_point( const tripoint_bub_ms &p );

    private:
        // origin (0,0) of this `map_test_case` in `map` coordinates
        // based on `anchor_map_pos` and `anchor_char`, lazily calculated when needed, reset on transformations
        std::optional<tripoint_bub_ms> origin = std::nullopt;

        // flag that internal sanity checks are completed, resets on transformations
        bool checks_complete = false;

        void do_internal_checks();

        void for_each_tile( const tripoint_bub_ms &tmp_origin,
                            const std::function<void( tile & )> &callback ) const;

};

// common helpers, used together with map_test_case
namespace map_test_case_common
{
using tile_predicate = std::function<bool( map_test_case::tile )>;

/**
 * Combines two functions `f` and `g` into a single one.
 * `f` and `g` must have same signature.
 * Returns tile_predicate that always returns "true" for easier chaning.
 */
tile_predicate operator+(
    const std::function<void( map_test_case::tile )> &f,
    const std::function<void( map_test_case::tile )> &g );

/**
 * Combines two boolean functions `f` and `g` into a single function that returns `f(tile) && g(tile)`.
 * Note, if `f` returns false, `g` is not invoked!  (&& semantics)
 */
tile_predicate operator&&( const tile_predicate &f, const tile_predicate &g );

/**
 * Combines two boolean functions `f` and `g` into a single function that returns `f(tile) || g(tile)`.
 * Note, if `f` returns true, `g` is not invoked!  (|| semantics)
 */
tile_predicate operator||( const tile_predicate &f, const tile_predicate &g );

namespace tiles
{
/**
 * Wraps the give function in another function that stores a char and compares input `tile::setup_c` to this char.
 * If `tile::setup_c` matches the char, underlying function is called (with forwarded arguments) and `true` is returned.
 * Otherwise underlying function is not called and `false` is returned.
 *
 * Intended usage:
 * ```auto set_up_tile = ifchar('#', brick_wall + roof_above) || ifchar('.', floor); ```
 *
 * @param c char that is tested against `tile::setup_c`
 * @param f function to call when char matches
 * @return function that returns true when char is matched and false otherwise
 */
tile_predicate ifchar( char c, const tile_predicate &f );

/**
 * Returns the function that sets the map `ter` at the `map_test_case::tile` coords to the given ter_id
 * @param ter ter id to set
 * @param shift shift from the current tile coordinates (e.g. to have the ability to set tiles above)
 * @return function that sets tiles
 */
tile_predicate ter_set(
    ter_str_id ter,
    tripoint shift = tripoint::zero
);

/**
 * Function that prints encountered char and fails.
 * Use at the end of the chain of char handlers.
 */
inline const tile_predicate fail = []( map_test_case::tile t )
{
    FAIL( "Setup char is not handled: " << t.setup_c );
    return false;
};

inline const tile_predicate noop = []( map_test_case::tile )
{
    return true;
};

} // namespace tiles

namespace printers
{
// converts 2d string vector into formatted (justified into the 2d table) string
std::string format_2d_array( const std::vector<std::vector<std::string>> &info );

// below are methods that print 2d table of the values that correspond to the given map_test_case map location
std::string fields( map_test_case &t, int zshift = 0 );

std::string transparency( map_test_case &t, int zshift = 0 );

std::string seen( map_test_case &t, int zshift = 0 );

std::string lm( map_test_case &t, int zshift = 0 );

std::string apparent_light( map_test_case &t, int zshift = 0 );

std::string obstructed( map_test_case &t, int zshift = 0 );

std::string floor( map_test_case &t, int zshift = 0 );

std::string expected( map_test_case &t );

} // namespace printers

} // namespace map_test_case_common

// definitions of template functions
template<typename R>
std::vector<std::vector<R>> map_test_case::map_tiles(
                             const std::function<R( map_test_case::tile )> &callback )
{
    std::vector<std::vector<R>> ret;

    for_each_tile( [&]( tile t ) -> void {
        if( t.p_local.x == 0 )
        {
            ret.push_back( std::vector<R>() );
        }
        ret[t.p_local.y].push_back( callback( t ) );
    } );

    return ret;
}

#endif // CATA_TESTS_MAP_TEST_CASE_H
