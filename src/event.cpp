#include "event.h"

namespace event_detail
{

static_assert( static_cast<int>( event_type::num_event_types ) == 2,
               "This static_assert is a reminder to add a definition below when you add a new "
               "event_type" );

constexpr std::array<std::pair<const char *, cata_variant_type>, 3>
event_spec<event_type::character_kills_character>::fields;

constexpr std::array<std::pair<const char *, cata_variant_type>, 2>
event_spec<event_type::character_kills_monster>::fields;

} // namespace event_detail
