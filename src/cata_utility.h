#ifndef CATA_UTILITY_H
#define CATA_UTILITY_H

#include <utility>
#include <string>
#include <vector>

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
std::string velocity_units( bool to_alternative_units = false );
std::string weight_units();
double convert_velocity( int velocity, bool to_alternative_units = false );
double convert_weight( int weight );

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

#endif // CAT_UTILITY_H
