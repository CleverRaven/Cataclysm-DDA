#include "event.h"

namespace event_detail
{

constexpr const char *event_spec<event_type::kill_monster>::name;
constexpr std::array<std::pair<const char *, cata_variant_type>, 2>
event_spec<event_type::kill_monster>::fields;

} // namespace event_detail
