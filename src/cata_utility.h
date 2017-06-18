#pragma once
#ifndef CATA_UTILITY_H
#define CATA_UTILITY_H

#include <utility>
#include <string>
#include <vector>
#include <fstream>
#include <functional>

class item;
class Creature;
class map_item_stack;
struct tripoint;

/**
 * Greater-than comparison operator; required by the sort interface
 */
struct pair_greater_cmp {
    bool operator()( const std::pair<int, tripoint> &a, const std::pair<int, tripoint> &b ) const;
};

/**
 * Type of object that a measurement is taken on.  Used, for example, to display wind speed in m/s
 * while displaying vehicle speed in km/h.
 */
enum units_type {
    VU_VEHICLE,
    VU_WIND
};

/**
 * Round a floating point value down to the nearest integer
 *
 * Optimized floor function, similar to std::floor but faster.
 */
inline int fast_floor( double v )
{
    return static_cast<int>( v ) - ( v < static_cast<int>( v ) );
}

/**
 * Round a value up at a given decimal place.
 *
 * @param val Value to be rounded.
 * @param dp Decimal place to round the value at.
 * @return Rounded value.
 */
double round_up( double val, unsigned int dp );

/**
 * Determine whether a value is between two given boundries.
 *
 * @param test Value to be tested.
 * @param down Lower boundry for value.
 * @param up Upper boundry for value.
 *
 * @return True if test value is greater than lower boundry and less than upper
 *         boundry, otherwise returns false.
 */
bool isBetween( int test, int down, int up );

/**
 * Perform case sensitive search for a query string inside a subject string.
 *
 * Searchs for string given by qry inside a subject string given by str.
 *
 * @param str Subject to search for occurance of the query string.
 * @param qry Query string to search for in str
 *
 * @return true if the query string is found at least once within the subject
 *         string, otherwise returns false
 */
bool lcmatch( const std::string &str, const std::string &qry );

std::vector<map_item_stack> filter_item_stacks( std::vector<map_item_stack> stack,
        std::string filter );
int list_filter_high_priority( std::vector<map_item_stack> &stack, std::string priorities );
int list_filter_low_priority( std::vector<map_item_stack> &stack, int start,
                              std::string priorities );

/**
 * Basic logistic function.
 *
 * Calculates the value at a single point on a stanndard logistic curve.
 *
 * @param t Poiint on logistic curve to retrieve value for
 *
 * @return Value of the logistic curve at the given poinit
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
 * @return The value of the scaled logstic curve at point pos.
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
 * maximum boundry, respectively.
 *
 * @param val The base value that the modifier will be applied to
 * @param mod The desired modifier to be added to the base value
 * @param max The desired maximum value of the base value after modification, or zero.
 * @param min The desired manimum value of the base value after modification, or zero.
 *
 * @returns Value of mod, possibly altered to respect the min and max boundries
 */
int bound_mod_to_vals( int val, int mod, int max, int min );

/**
 * Create a units label for a velocity value.
 *
 * Gives the name of the velocity unit in the user selected unit system, either
 * "km/h", "ms/s" or "mph".  Used to add abbreviated unit labels to the output of
 * @ref convert_velocity.
 *
 * @param vel_units type of velocity desired (ie wind or vehicle)
 *
 * @return name of unit.
 */
const char *velocity_units( const units_type vel_units );

/**
 * Create a units label for a weight value.
 *
 * Gives the name of the weight unit in the user selected unit system, either
 * "kgs" or "lbs".  Used to add unit labels to the output of @ref convert_weight.
 *
 * @return name of unit
 */
const char *weight_units();

/**
 * Create an abbreviated units label for a volume value.
 *
 * Returns the abbreviated name for the volume unit for the user selected unit system,
 * ie "c", "L", or "qt". Used to add unit labels to the output of @ref convert_volume.
 *
 * @return name of unit.
 */
const char *volume_units_abbr();

/**
 * Create a units label for a volume value.
 *
 * Returns the abbreviated name for the volume unit for the user selected unit system,
 * ie "cup", "liter", or "quart". Used to add unit labels to the output of @ref convert_volume.
 *
 * @return name of unit.
 */
const char *volume_units_long();

/**
 * Convert internal velocity units to units defined by user.
 *
 * @param velocity A velocity value in internal units.
 * @param vel_units General type of item this velocity is for (eg vehicles or wind)
 *
 * @returns Velocity in the user selected measurement system and in appropriate
 *          units for the object being measured.
 */
double convert_velocity( int velocity, const units_type vel_units );

/**
 * Convert weight in grams to units defined by user (kg or lbs)
 *
 * @param weight to be converted.
 *
 * @returns Weight converted to user selected unit
 */
double convert_weight( int weight );

/**
 * Convert volume from ml to units defined by user.
 */
double convert_volume( int volume );

/**
 * Convert volume from ml to units defined by user,
 * optionally returning the units preferred scale.
 */
double convert_volume( int volume, int *out_scale );

/**
 * Convert a temperature from degrees fahrenheit to degrees celsius.
 *
 * @param fahrenheit Temperature in degrees F.
 *
 * @return Temperature in degrees C.
 */
double temp_to_celsius( double fahrenheit );

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
        list_circularizer( std::vector<T> &_list ) : _list( &_list ) {
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
            return ( *_list )[_index]; // list could be null, but it would be a design time mistake and really, the callers fault.
        }
};

/**
 * Wrapper around std::ofstream that handles error checking and throws on errors.
 *
 * Use like a normal ofstream: the stream is opened in the constructor and
 * closed via @ref close. Both functions check for success and throw std::exception
 * upon any error (e.g. when opening failed or when the stream is in an error state when
 * being closed).
 * Use @ref stream (or the implicit conversion) to access the output stream and to write
 * to it.
 *
 * @note: the stream is closed in the constructor, but no exception is throw from it. To
 * ensure all errors get reported correctly, you should always call `close` explicitly.
 */
class ofstream_wrapper
{
    private:
        std::ofstream file_stream;

    public:
        ofstream_wrapper( const std::string &path );
        ~ofstream_wrapper();

        std::ostream &stream() {
            return file_stream;
        }
        operator std::ostream &() {
            return file_stream;
        }

        void close();
};

/**
 * Open a file for writing, calls the writer on that stream.
 *
 * If the writer throws, or if the file could not be opened or if any I/O error
 * happens, the function shows a popup containing the
 * \p fail_message, the error text and the path.
 *
 * @return Whether saving succeeded (no error was caught).
 */
bool write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer,
                    const char *fail_message );
class JsonIn;
class JsonDeserializer;
/**
 * Try to open and read from given file using the given callback.
 *
 * The file is opened for reading (binary mode), given to the callback (which does the actual
 * reading) and closed.
 * Any exceptions from the callbacks are caught and reported as `debugmsg`.
 * If the stream is in a fail state (other than EOF) after the callback returns, it is handled as
 * error as well.
 *
 * The callback can either be a generic `std::istream`, a @ref JsonIn stream (which has been
 * initialized from the `std::istream`) or a @ref JsonDeserializer object (in case of the later,
 * it's `JsonDeserializer::deserialize` method will be invoked).
 *
 * The functions with the "_optional" prefix do not show a debug message when the file does not
 * exist. They simply ignore the call and return `false` immediately (without calling the callback).
 * They can be used for loading legacy files.
 *
 * @return `true` is the file was read without any errors, `false` upon any error.
 */
/**@{*/
bool read_from_file( const std::string &path, const std::function<void( std::istream & )> &reader );
bool read_from_file_json( const std::string &path, const std::function<void( JsonIn & )> &reader );
bool read_from_file( const std::string &path, JsonDeserializer &reader );

bool read_from_file_optional( const std::string &path,
                              const std::function<void( std::istream & )> &reader );
bool read_from_file_optional_json( const std::string &path,
                                   const std::function<void( JsonIn & )> &reader );
bool read_from_file_optional( const std::string &path, JsonDeserializer &reader );
/**@}*/
/**
 * Same as ofstream_wrapper, but uses exclusive I/O (@ref fopen_exclusive).
 * The interface intentionally matches ofstream_wrapper. One should be able to use
 * one instead of the other.
 */
class ofstream_wrapper_exclusive
{
    private:
        std::ofstream file_stream;
        std::string path;

    public:
        ofstream_wrapper_exclusive( const std::string &path );
        ~ofstream_wrapper_exclusive();

        std::ostream &stream() {
            return file_stream;
        }
        operator std::ostream &() {
            return file_stream;
        }

        void close();
};

/** See @ref write_to_file, but uses the exclusive I/O functions. */
bool write_to_file_exclusive( const std::string &path,
                              const std::function<void( std::ostream & )> &writer,  const char *fail_message );

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

std::string obscure_message( const std::string &str, std::function<char( void )> f );


#endif // CAT_UTILITY_H
