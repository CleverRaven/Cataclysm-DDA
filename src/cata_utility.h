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

struct pair_greater_cmp {
    bool operator()( const std::pair<int, tripoint> &a, const std::pair<int, tripoint> &b );
};

// TODO: Put this into a header (which one?) and maybe move the implementation somewhere else.
/** Comparator object to sort creatures according to their attitude from "u",
 * and (on same attitude) according to their distance to "u".
 */
struct compare_by_dist_attitude {
    const Creature &u;
    bool operator()( Creature *a, Creature *b ) const;
};

enum units_type {
    VU_VEHICLE,
    VU_WIND
};

bool isBetween( int test, int down, int up );

bool list_items_match( const item *item, std::string sPattern );

std::vector<map_item_stack> filter_item_stacks( std::vector<map_item_stack> stack,
        std::string filter );
int list_filter_high_priority( std::vector<map_item_stack> &stack, std::string priorities );
int list_filter_low_priority( std::vector<map_item_stack> &stack, int start,
                              std::string priorities );

int calculate_drop_cost( std::vector<item> &dropped, const std::vector<item> &dropped_worn,
                         int freed_volume_capacity );

double logarithmic( double t );
double logarithmic_range( int min, int max, int pos );

int bound_mod_to_vals( int val, int mod, int max, int min );
const char *velocity_units( const units_type vel_units );
const char *weight_units();
double convert_velocity( int velocity, const units_type vel_units );
double convert_weight( int weight );
double temp_to_celsius( double fahrenheit );

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
        list_circularizer( std::vector<T> &_list ) : _list( &_list ) {
        }

        void next() {
            _index = ( _index == _list->size() - 1 ? 0 : _index + 1 );
        }

        void prev() {
            _index = ( _index == 0 ? _list->size() - 1 : _index - 1 );
        }

        T &cur() const {
            return ( *_list )[_index]; // list could be null, but it would be a design time mistake and really, the callers fault.
        }
};

/**
 * Wrapper around @ref std::ofstream that handles error checking and throws on
 * errors. Use like a normal ofstream: the stream is opened in the constructor and
 * closed via @ref close. Both functions check for success and throw @ref std::exception
 * upon any error (e.g. when opening failed or when the stream is in an error state when
 * being closed).
 * Use @ref stream (or the implicit conversion) to access the output stream and to write
 * to it.
 * Note: the stream is closed in the constructor, but no exception is throw from it. To
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
 * Open a file for writing, calls the writer on that stream. If the writer throws, or if the file
 * could not be opened or if any I/O error happens, the function shows a popup containing the
 * \p fail_message, the error text and the path.
 * @return Whether saving succeeded (no error was caught).
 */
bool write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer,
                    const char *fail_message );

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

#endif // CAT_UTILITY_H
