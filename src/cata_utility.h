#pragma once
#ifndef CATA_SRC_CATA_UTILITY_H
#define CATA_SRC_CATA_UTILITY_H

#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string> // IWYU pragma: keep
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "enums.h"

class JsonOut;
class JsonValue;
class cata_path;
class translation;

/**
 * Greater-than comparison operator; required by the sort interface
 */
struct pair_greater_cmp_first {
    template< class T, class U >
    bool operator()( const std::pair<T, U> &a, const std::pair<T, U> &b ) const {
        return a.first > b.first;
    }

};

/**
 * For use with smart pointers when you don't actually want the deleter to do
 * anything.
 */
struct null_deleter {
    template<typename T>
    void operator()( T * ) const {}
};

/**
 * Type of object that a measurement is taken on.  Used, for example, to display wind speed in m/s
 * while displaying vehicle speed in km/h.
 */
enum units_type {
    VU_VEHICLE,
    VU_WIND
};

constexpr bool float_equals( double l, double r )
{
    constexpr double epsilon = std::numeric_limits<double>::epsilon() * 100;
    return l + epsilon >= r && r + epsilon >= l;
}

/**
 * Round a value up at a given decimal place.
 *
 * @param val Value to be rounded.
 * @param dp Decimal place to round the value at.
 * @return Rounded value.
 */
double round_up( double val, unsigned int dp );

/** Divide @p num by @p den, rounding up
*
* @p num must be non-negative, @p den must be positive, and @c num+den must not overflow.
*/
template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T divide_round_up( T num, T den )
{
    return ( num + den - 1 ) / den;
}

int divide_round_down( int a, int b );

/**
 * Determine whether a value is between two given boundaries.
 *
 * @param test Value to be tested.
 * @param down Lower boundary for value.
 * @param up Upper boundary for value.
 *
 * @return True if test value is greater than lower boundary and less than upper
 *         boundary, otherwise returns false.
 */
bool isBetween( int test, int down, int up );

/**
 * Perform case insensitive search for a query string inside a subject string.
 *
 * Searches for string given by qry inside a subject string given by str.
 *
 * Supports searching for accented letters with a non-accented search key, for example,
 * search key 'bo' matches 'Bō', but search key 'bö' should not match with 'Bō' and only match with 'Bö' or 'BÖ'.
 *
 * @param str Subject to search for occurrence of the query string.
 * @param qry Query string to search for in str
 *
 * @return true if the query string is found at least once within the subject
 *         string, otherwise returns false
 */
bool lcmatch( std::string_view str, std::string_view qry );
bool lcmatch( const translation &str, std::string_view qry );

/**
 * Matches text case insensitive with the include/exclude rules of the filter
 *
 * Multiple includes/excludes are possible
 *
 * Examle: bank,-house,tank,-car
 * Will match text containing tank or bank while not containing house or car
 *
 * @param text String to be matched
 * @param filter String with include/exclude rules
 *
 * @return true if include/exclude rules pass. See Example.
 */
bool match_include_exclude( std::string_view text, std::string filter );

/**
 * Basic logistic function.
 *
 * Calculates the value at a single point on a standard logistic curve.
 *
 * @param t Point on logistic curve to retrieve value for
 *
 * @return Value of the logistic curve at the given point
 */
double logarithmic( double t );

/**
 * Normalized logistic function
 *
 * Generates a logistic curve on the domain [-6,6], then normalizes such that
 * the value ranges from 1 to 0.  A single point is then calculated on this curve.
 *
 * @param min t-value that should yield an output of 1 on the scaled curve.
 * @param max t-value that should yield an output of 0 on the scaled curve.
 * @param pos t-value to calculate the output for.
 *
 * @return The value of the scaled logistic curve at point pos.
 */
double logarithmic_range( int min, int max, int pos );

/**
 * Clamp the value of a modifier in order to bound the resulting value
 *
 * Ensures that a modifier value will not cause a base value to exceed given
 * bounds when applied.  If necessary, the given modifier value is increased or
 * reduced to meet this constraint.
 *
 * Giving a value of zero for min or max indicates that there is no minimum or
 * maximum boundary, respectively.
 *
 * @param val The base value that the modifier will be applied to
 * @param mod The desired modifier to be added to the base value
 * @param max The desired maximum value of the base value after modification, or zero.
 * @param min The desired minimum value of the base value after modification, or zero.
 *
 * @returns Value of mod, possibly altered to respect the min and max boundaries
 */
int bound_mod_to_vals( int val, int mod, int max, int min );

/**
 * Create a units label for a velocity value.
 *
 * Gives the name of the velocity unit in the user selected unit system, either
 * "km/h", "ms/s" or "mph".  Used to add abbreviated unit labels to the output of
 * @ref convert_velocity.
 *
 * @param vel_units type of velocity desired (i.e. wind or vehicle)
 *
 * @return name of unit.
 */
const char *velocity_units( units_type vel_units );

/**
 * Convert internal velocity units to units defined by user.
 *
 * @param velocity A velocity value in internal units.
 * @param vel_units General type of item this velocity is for (e.g. vehicles or wind)
 *
 * @returns Velocity in the user selected measurement system and in appropriate
 *          units for the object being measured.
 */
double convert_velocity( int velocity, units_type vel_units );

/**
 * Convert a temperature from Kelvin to degrees Fahrenheit.
 *
 * @return Temperature in degrees C.
 */
double kelvin_to_fahrenheit( double kelvin );

/**
 * Clamp (number and space wise) value to with,
 * taking into account the specified preferred scale,
 * returning the adjusted (shortened) scale that best fit the width,
 * optionally returning a flag that indicate if the value was truncated to fit the width
 */
/**@{*/
double clamp_to_width( double value, int width, int &scale );
double clamp_to_width( double value, int width, int &scale, bool *out_truncated );
/**@}*/

/**
 * Clamp first argument so that it is no lower than second and no higher than third.
 * Does not check if min is lower than max.
 */
template<typename T>
constexpr T clamp( const T &val, const T &min, const T &max )
{
    return std::max( min, std::min( max, val ) );
}

/**
 * Linear interpolation: returns first argument if t is 0, second if t is 1, otherwise proportional to t.
 * Does not clamp t, meaning it can return values lower than min (if t<0) or higher than max (if t>1).
 */
template<typename T>
constexpr T lerp( const T &min, const T &max, float t )
{
    return ( 1.0f - t ) * min + t * max;
}

/** Linear interpolation with t clamped to [0, 1] */
template<typename T>
constexpr T lerp_clamped( const T &min, const T &max, float t )
{
    return lerp( min, max, clamp( t, 0.0f, 1.0f ) );
}

// Inverse of \p lerp, unbounded so it may extrapolate, returns 0.0f if min == max
// @returns linear factor for interpolating between \p min and \p max to reach \p value
template<typename T>
constexpr float inverse_lerp( const T &min, const T &max, const T &value )
{
    if( max == min ) {
        return 0.0f; // avoids a NaN
    }
    return ( value - min ) / ( max - min );
}

// Remaps \p value from range of \p i_min to \p i_max to a range between \p o_min and \p o_max
// uses unclamped linear interpolation, so output value may be beyond output range if value is
// outside input range.
template<typename Tin, typename Tout>
constexpr Tout linear_remap( const Tin &i_min, const Tin &i_max,
                             const Tout &o_min, const Tout &o_max, Tin value )
{
    const float t = inverse_lerp( i_min, i_max, value );
    return lerp( o_min, o_max, t );
}

/**
 * From `points`, finds p1 and p2 such that p1.first < x < p2.first
 * Then linearly interpolates between p1.second and p2.second and returns the result.
 * `points` should be sorted by first elements of the pairs.
 * If x is outside range, returns second value of the first (if x < points[0].first) or last point.
 */
float multi_lerp( const std::vector<std::pair<float, float>> &points, float x );

/**
 * @brief Class used to access a list as if it were circular.
 *
 * Some times you just want to have a list loop around on itself.
 * This wrapper class allows you to do that. It requires the list to exist
 * separately, but that also means any changes to the list get propagated (both ways).
 */
template<typename T>
class list_circularizer
{
    private:
        unsigned int _index = 0;
        std::vector<T> *_list;
    public:
        /** Construct list_circularizer from an existing std::vector. */
        explicit list_circularizer( std::vector<T> &_list ) : _list( &_list ) {
        }

        /** Advance list to next item, wrapping back to 0 at end of list */
        void next() {
            _index = ( _index == _list->size() - 1 ? 0 : _index + 1 );
        }

        /** Advance list to previous item, wrapping back to end at zero */
        void prev() {
            _index = ( _index == 0 ? _list->size() - 1 : _index - 1 );
        }

        /** Return list element at the current location */
        T &cur() const {
            // list could be null, but it would be a design time mistake and really, the callers fault.
            return ( *_list )[_index];
        }
};

/**
 * Open a file for writing, calls the writer on that stream.
 *
 * If the writer throws, or if the file could not be opened or if any I/O error
 * happens, the function shows a popup containing the
 * \p fail_message, the error text and the path.
 *
 * @return Whether saving succeeded (no error was caught).
 * @throw The void function throws when writing fails or when the @p writer throws.
 * The other function catches all exceptions and returns false.
 */
///@{
bool write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer,
                    const char *fail_message );
void write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer );
bool write_to_file( const cata_path &path, const std::function<void( std::ostream & )> &writer,
                    const char *fail_message );
void write_to_file( const cata_path &path, const std::function<void( std::ostream & )> &writer );
///@}

/**
 * Try to open and read from given file using the given callback.
 *
 * The file is opened for reading (binary mode), given to the callback (which does the actual
 * reading) and closed.
 * Any exceptions from the callbacks are caught and reported as `debugmsg`.
 * If the stream is in a fail state (other than EOF) after the callback returns, it is handled as
 * error as well.
 *
 * The callback can either be a generic `std::istream` or a @ref JsonIn stream (which has been
 * initialized from the `std::istream`) or a @ref JsonValue object.
 *
 * The functions with the "_optional" prefix do not show a debug message when the file does not
 * exist. They simply ignore the call and return `false` immediately (without calling the callback).
 * They can be used for loading legacy files.
 *
 * @return `true` is the file was read without any errors, `false` upon any error.
 */
/**@{*/
bool read_from_file( const std::string &path, const std::function<void( std::istream & )> &reader );
bool read_from_file( const std::filesystem::path &path,
                     const std::function<void( std::istream & )> &reader );
bool read_from_file( const cata_path &path, const std::function<void( std::istream & )> &reader );
bool read_from_file_json( const cata_path &path,
                          const std::function<void( const JsonValue & )> &reader );

bool read_from_file_optional( const std::string &path,
                              const std::function<void( std::istream & )> &reader );
bool read_from_file_optional( const std::filesystem::path &path,
                              const std::function<void( std::istream & )> &reader );
bool read_from_file_optional( const cata_path &path,
                              const std::function<void( std::istream & )> &reader );
bool read_from_file_optional_json( const cata_path &path,
                                   const std::function<void( const JsonValue & )> &reader );
/**@}*/

/**
 * Try to open and provide a std::istream for the given, possibly, gzipped, file.
 *
 * The file is opened for reading (binary mode) and tested to see if it is compressed.
 * Compressed files are decompressed into a std::stringstream and returned.
 * Uncompressed files are returned as normal lazy ifstreams.
 * Any exceptions during reading, including failing to open the file, are reported as dbgmsg.
 *
 * @return A unique_ptr of the appropriate istream, or nullptr on failure.
 */
/**@{*/
std::unique_ptr<std::istream> read_maybe_compressed_file( const std::string &path );
std::unique_ptr<std::istream> read_maybe_compressed_file( const std::filesystem::path &path );
std::unique_ptr<std::istream> read_maybe_compressed_file( const cata_path &path );
/**@}*/

/**
 * Try to open and read the entire file to a string.
 *
 * The file is opened for reading (binary mode), read into a string, and then closed.
 * Any exceptions during reading, including failing to open the file, are reported as dbgmsg.
 *
 * @return A nonempty optional with the contents of the file on success, or std::nullopt on failure.
 */
/**@{*/
std::optional<std::string> read_whole_file( const std::string &path );
std::optional<std::string> read_whole_file( const std::filesystem::path &path );
std::optional<std::string> read_whole_file( const cata_path &path );
/**@}*/

std::istream &safe_getline( std::istream &ins, std::string &str );

/** Apply fuzzy effect to a string like:
 * Hello, world! --> H##lo, wurl#!
 *
 * @param str the original string to be processed
 * @param f the function that guides how to mess the message
 * f() will be called for each character (lingual, not byte):
 * [-] f() == -1 : nothing will be done
 * [-] f() == 0  : the said character will be replace by a random character
 * [-] f() == ch : the said character will be replace by ch
 *
 * @return The processed string
 *
 */

std::string obscure_message( const std::string &str, const std::function<char()> &f );

/**
 * @group JSON (de)serialization wrappers.
 *
 * The functions here provide a way to (de)serialize objects without actually
 * including "json.h". The `*_wrapper` function create the JSON stream instances
 * and therefore require "json.h", but the caller doesn't. Callers should just
 * forward the stream reference to the actual (de)serialization function.
 *
 * The inline function do this by calling `T::(de)serialize` (which is assumed
 * to exist with the proper signature).
 *
 * @throws std::exception Deserialization functions may throw upon malformed
 * JSON or unexpected/invalid content.
 */
/**@{*/
std::string serialize_wrapper( const std::function<void( JsonOut & )> &callback );
void deserialize_wrapper( const std::function<void( const JsonValue & )> &callback,
                          const std::string &data );

template<typename T>
inline std::string serialize( const T &obj )
{
    return serialize_wrapper( [&obj]( JsonOut & jsout ) {
        obj.serialize( jsout );
    } );
}

template<typename T>
inline void deserialize_from_string( T &obj, const std::string &data )
{
    deserialize_wrapper( [&obj]( const JsonValue & jsin ) {
        obj.deserialize( jsin );
    }, data );
}
/**@}*/

/**
 * \brief Returns true iff s1 starts with s2
 */
inline bool string_starts_with( std::string_view s1, std::string_view s2 )
{
    return s1.compare( 0, s2.size(), s2 ) == 0;
}

/**
 * \brief Returns true iff s1 ends with s2
 */
inline bool string_ends_with( std::string_view s1, std::string_view s2 )
{
    return s1.size() >= s2.size() &&
           s1.compare( s1.size() - s2.size(), s2.size(), s2 ) == 0;
}

bool string_empty_or_whitespace( const std::string &s );

/** strcmp, but for string_view objects.  In C++20 this can be replaced with
 * operator<=> */
int string_view_cmp( std::string_view, std::string_view );

template<typename Integer>
Integer svto( std::string_view );

extern template int svto<int>( std::string_view );

/** Used as a default filter in various functions */
template<typename T>
bool return_true( const T & )
{
    return true;
}

template<typename T>
bool return_false( const T & )
{
    return false;
}

/**
 * Joins an iterable (class implementing begin() and end()) of elements into a single
 * string with specified delimiter by using `<<` ostream operator on each element
 *
 * keyword: implode
 */
template<typename Container>
std::string string_join( const Container &iterable, const std::string &joiner )
{
    std::ostringstream buffer;

    for( auto a = iterable.begin(); a != iterable.end(); ++a ) {
        if( a != iterable.begin() ) {
            buffer << joiner;
        }
        buffer << *a;
    }
    return buffer.str();
}

/**
* Splits a string by delimiter into a vector of strings
*
* keyword: explode
*/
std::vector<std::string> string_split( std::string_view string, char delim );

/**
 * Append all arguments after the first to the first.
 *
 * This provides a way to append several strings to a single root string
 * in a single line without an expression like 'a += b + c' which can cause an
 * unnecessary allocation in the 'b + c' expression.
 */
template<typename... T>
std::string &str_append( std::string &root, T &&...a )
{
    // Using initializer list as a poor man's fold expression until C++17.
    static_cast<void>(
    std::array<bool, sizeof...( T )> { {
            ( root.append( std::forward<T>( a ) ), false )...
        }
    } );
    return root;
}

/**
 * Concatenates a bunch of strings with append, to minimize unnecessary
 * allocations
 */
template<typename T0, typename... T>
std::string str_cat( T0 &&a0, T &&...a )
{
    std::string result( std::forward<T0>( a0 ) );
    return str_append( result, std::forward<T>( a )... );
}

/**
 * Erases elements from a set that match given predicate function.
 * Will work on vector, albeit not optimally performance-wise.
 * @return true if set was changed
 */
//bool erase_if( const std::function<bool( const value_type & )> &predicate ) {
template<typename Col, class Pred>
bool erase_if( Col &set, Pred predicate )
{
    bool ret = false;
    auto iter = set.begin();
    for( ; iter != set.end(); ) {
        if( predicate( *iter ) ) {
            iter = set.erase( iter );
            ret = true;
        } else {
            ++iter;
        }
    }
    return ret;
}

/**
 * Checks if two sets are equal, ignoring specified elements.
 * Works as if `ignored_elements` were temporarily erased from both sets before comparison.
 * @tparam Set type of the set (must be ordered, i.e. std::set, cata::flat_set)
 * @param set first set to compare
 * @param set2 second set to compare
 * @param ignored_elements elements from both sets to ignore
 * @return true, if sets without ignored elements are equal, false otherwise
 */
template<typename Set, typename T = std::decay_t<decltype( *std::declval<const Set &>().begin() )>>
bool equal_ignoring_elements( const Set &set, const Set &set2, const Set &ignored_elements )
{
    // general idea: splits both sets into the ranges bounded by elements from `ignored_elements`
    // and checks that these ranges are equal

    // traverses ignored elements in
    if( ignored_elements.empty() ) {
        return set == set2;
    }

    using Iter = typename Set::iterator;
    Iter end = ignored_elements.end();
    Iter cur = ignored_elements.begin();
    Iter prev = cur;
    cur++;

    // first comparing the sets range [set.begin() .. ignored_elements.begin()]
    if( !std::equal( set.begin(), set.lower_bound( *prev ),
                     set2.begin(), set2.lower_bound( *prev ) ) ) {
        return false;
    }

    // compare ranges bounded by two elements: [`prev` .. `cur`]
    while( cur != end ) {
        if( !std::equal( set.upper_bound( *prev ), set.lower_bound( *cur ),
                         set2.upper_bound( *prev ), set2.lower_bound( *cur ) ) ) {
            return false;
        }
        prev = cur;
        cur++;
    }

    // compare the range after the last element of ignored_elements: [ignored_elements.back() .. set.end()]
    return static_cast<bool>( std::equal( set.upper_bound( *prev ), set.end(),
                                          set2.upper_bound( *prev ), set2.end() ) );
}

/**
 * Return a copy of a std::map with some keys removed.
 */
template<typename K, typename V>
std::map<K, V> map_without_keys( const std::map<K, V> &original, const std::vector<K> &remove_keys )
{
    std::map<K, V> filtered( original );
    for( const K &key : remove_keys ) {
        filtered.erase( key );
    }
    return filtered;
}

template<typename Map, typename Set>
bool map_equal_ignoring_keys( const Map &lhs, const Map &rhs, const Set &ignore_keys )
{
    // Since map and set are sorted, we can do this as a single pass with only conditional checks into remove_keys
    if( ignore_keys.empty() ) {
        return lhs == rhs;
    }

    auto lbegin = lhs.begin();
    auto lend = lhs.end();
    auto rbegin = rhs.begin();
    auto rend = rhs.end();

    for( ; lbegin != lend && rbegin != rend; ++lbegin, ++rbegin ) {
        // Sanity check keys
        if( lbegin->first != rbegin->first ) {
            while( lbegin != lend && ignore_keys.count( lbegin->first ) == 1 ) {
                ++lbegin;
            }
            if( lbegin == lend ) {
                break;
            }
            if( rbegin->first != lbegin->first ) {
                while( rbegin != rend && ignore_keys.count( rbegin->first ) == 1 ) {
                    ++rbegin;
                }
                if( rbegin == rend ) {
                    break;
                }
            }
            // If we've skipped ignored keys and the keys still don't match,
            // then the maps are unequal.
            if( lbegin->first != rbegin->first ) {
                return false;
            }
        }
        if( lbegin->second != rbegin->second && ignore_keys.count( lbegin->first ) != 1 ) {
            return false;
        }
        // Either the values were equal, or the key was ignored.
    }
    // At least one map ran out of keys. The other may still have ignored keys in it.
    while( lbegin != lend && ignore_keys.count( lbegin->first ) ) {
        ++lbegin;
    }
    while( rbegin != rend && ignore_keys.count( rbegin->first ) ) {
        ++rbegin;
    }
    return lbegin == lend && rbegin == rend;
}

int modulo( int v, int m );

/** Add elements from one set to another */
template <typename T>
std::unordered_set<T> &operator<<( std::unordered_set<T> &lhv, const std::unordered_set<T> &rhv )
{
    lhv.insert( rhv.begin(), rhv.end() );
    return lhv;
}

/** Move elements from one set to another */
template <typename T>
std::unordered_set<T> &operator<<( std::unordered_set<T> &lhv, std::unordered_set<T> &&rhv )
{
    for( const T &value : rhv ) {
        lhv.insert( std::move( value ) );
    }
    rhv.clear();
    return lhv;
}

/**
 * Get the current holiday based on the given time, or based on current time if time = 0
 * @param time The timestampt to assess
 * @param force_refresh Force recalculation of current holiday, otherwise use cached value
*/
holiday get_holiday_from_time( std::time_t time = 0, bool force_refresh = false );

/**
 * Returns a random (weighted) bucket index from a list of weights
 * @param weights vector with a list of int weights
 * @return random bucket index
 */
int bucket_index_from_weight_list( const std::vector<int> &weights );

/**
 * Set the game window title.
 * Implemented in `stdtiles.cpp`, `wincurse.cpp`, and `ncurses_def.cpp`.
 */
void set_title( const std::string &title );

/**
 * Convenience function to get the aggregate value for a list of values.
 */
template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T aggregate( const std::vector<T> &values, aggregate_type agg_func )
{
    if( values.empty() ) {
        return T();
    }
    switch( agg_func ) {
        case aggregate_type::FIRST:
            return values.front();
        case aggregate_type::LAST:
            return *values.rbegin();
        case aggregate_type::MIN:
            return *std::min_element( values.begin(), values.end() );
        case aggregate_type::MAX:
            return *std::max_element( values.begin(), values.end() );
        case aggregate_type::AVERAGE:
        case aggregate_type::SUM: {
            T agg_sum = std::accumulate( values.begin(), values.end(), 0 );
            return agg_func == aggregate_type::SUM ? agg_sum : agg_sum / values.size();
        }
        default:
            return T();
    }
}

// overload pattern for std::variant from https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
explicit overloaded( Ts... ) -> overloaded<Ts...>;

std::optional<double> svtod( std::string_view token );

#endif // CATA_SRC_CATA_UTILITY_H
