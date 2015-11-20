#ifndef CATA_UTILITY_H
#define CATA_UTILITY_H

#include "enums.h"
#include "creature.h"
#include "item.h"

#include <utility>
#include <string>
#include <vector>

typedef int nc_color;

struct pair_greater_cmp
{
    bool operator()( const std::pair<int, tripoint> &a, const std::pair<int, tripoint> &b )
    {
        return a.first > b.first;
    }
};

// TODO: Put this into a header (which one?) and maybe move the implementation somewhere else.
/** Comparator object to sort creatures according to their attitude from "u",
 * and (on same attitude) according to their distance to "u".
 */
struct compare_by_dist_attitude {
    const Creature &u;
    bool operator()(Creature *a, Creature *b) const;
};

bool isBetween( int test, int down, int up );

bool list_items_match(const item *item, std::string sPattern);

std::vector<map_item_stack> filter_item_stacks(std::vector<map_item_stack> stack,
                                               std::string filter);
int list_filter_high_priority(std::vector<map_item_stack> &stack, std::string priorities);
int list_filter_low_priority(std::vector<map_item_stack> &stack, int start,
                             std::string priorities);

int calculate_drop_cost(std::vector<item> &dropped, const std::vector<item> &dropped_worn,
                              int freed_volume_capacity);
nc_color sev(int a);

double logarithmic(double t);
double logarithmic_range(int min, int max, int pos);

int bound_mod_to_vals(int val, int mod, int max, int min);
double convert_weight(int weight);

#endif // CAT_UTILITY_H
