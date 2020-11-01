#ifndef CATA_SRC_ADVUILIST_HELPERS_H
#define CATA_SRC_ADVUILIST_HELPERS_H

#include <cstddef> // for size_t
#include <string>  // for allocator, basic_string, string
#include <vector>  // for vector

#include "advuilist.h"         // for advuilist
#include "advuilist_sourced.h" // for advuilist_sourced
#include "item_location.h"     // for item_location
#include "string_formatter.h"  // for string_format

class map_cursor;

namespace advuilist_helpers
{
/// entry type for advuilist based on item_location
struct iloc_entry {
    // entries are stacks of items
    using stack_t = std::vector<item_location>;
    stack_t stack;
};

using iloc_stack_t = std::vector<iloc_entry>;
using aim_container_t = std::vector<iloc_entry>;
using aim_advuilist_t = advuilist<aim_container_t, iloc_entry>;
using aim_advuilist_sourced_t = advuilist_sourced<aim_container_t, iloc_entry>;

iloc_stack_t get_stacks( map_cursor &cursor );

std::size_t iloc_entry_counter( iloc_entry const &it );

template <class Container = aim_container_t>
void setup_for_aim( advuilist<Container, iloc_entry> *myadvuilist );

template <class Container = aim_container_t>
Container source_all( int radius = 1 );

template <class Container = aim_container_t>
void add_aim_sources( advuilist_sourced<Container, iloc_entry> *myadvuilist );

extern template void setup_for_aim( aim_advuilist_t *myadvuilist );
extern template aim_container_t source_all( int radius );
extern template void add_aim_sources( aim_advuilist_sourced_t *myadvuilist );

} // namespace advuilist_helpers

#endif
