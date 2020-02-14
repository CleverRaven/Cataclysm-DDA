#include "move_mode.h"

#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "int_id.h"
#include "color.h"

namespace
{

    generic_factory<move_mode> all_move_modes("move modes");

} // namespace

template<>
bool int_id<move_mode>::is_valid() const
{
    return all_move_modes.is_valid(*this);
}

/** @relates int_id */
template<>
const move_mode& int_id<move_mode>::obj() const
{
    return all_move_modes.obj(*this);
}

/** @relates int_id */
template<>
const string_id<move_mode>& int_id<move_mode>::id() const
{
    return all_move_modes.convert(*this);
}

/** @relates string_id */
template<>
bool string_id<move_mode>::is_valid() const
{
    return all_move_modes.is_valid(*this);
}

/** @relates string_id */
template<>
const move_mode& string_id<move_mode>::obj() const
{
    return all_move_modes.obj(*this);
}

/** @relates string_id */
template<>
int_id<move_mode> string_id<move_mode>::id() const
{
    return all_move_modes.convert(*this, MM_NULL);
}

/** @relates int_id */
template<>
int_id<move_mode>::int_id(const string_id<move_mode>& id) : _id(id.id())
{
}

void move_mode::load(const JsonObject& jo, const std::string&)
{
    mandatory(jo, was_loaded, "name", name);

    optional(jo, was_loaded, "flavor_text", flavor_text, to_translation("You change stance."));
    optional(jo, was_loaded, "flavor_text_mount", flavor_text_mount, to_translation("You change stance."));
    optional(jo, was_loaded, "flavor_text_mech", flavor_text_mech, to_translation("You change stance."));

    optional(jo, was_loaded, "cycle_index", cycle_index, -1);
    optional(jo, was_loaded, "can_be_cycled_to", can_be_cycled_to, true);
    optional(jo, was_loaded, "can_haul", can_haul, true);
    optional(jo, was_loaded, "usable_while_mounted", usable_while_mounted, false);
    optional(jo, was_loaded, "speed", speed, 1.0f);
    optional(jo, was_loaded, "display_character", display_character, "M");
    optional(jo, was_loaded, "minimum_required_legs", minimum_required_legs, 0);
    optional(jo, was_loaded, "cost_to_enter_or_leave_stance", cost_to_enter_or_leave_stance, 0);
    optional(jo, was_loaded, "cancels_activities", cancels_activities, false);
    optional(jo, was_loaded, "stamina_burn_multiplier", stamina_burn_multiplier, 1.0f);
    optional(jo, was_loaded, "volume_multiplier", volume_multiplier, 1.0f);
    optional(jo, was_loaded, "cancels_activities", cancels_activities, false);
    optional(jo, was_loaded, "display_colour", display_colour, "light_gray");

}

void move_mode::finalize()
{
    nc_display_colour = color_from_string(display_colour);
    display_character_ptr = display_character.c_str();
}

void move_mode::check() const
{
    if (display_character.length() < 1) {
        debugmsg("Attempt to load move mode %s failed because it did not have a valid display_character", id.c_str());
    }
}

size_t move_mode::count()
{
    return all_move_modes.size();
}

void move_modes::load(const JsonObject& jo, const std::string& src)
{
    all_move_modes.load(jo, src);
}

void move_modes::finalize_all()
{
    set_move_mode_ids();
    all_move_modes.finalize();
    for (const move_mode& mm : all_move_modes.get_all()) {
        const_cast<move_mode&>(mm).finalize();
    }
}

void move_modes::check_consistency()
{
    all_move_modes.check();
}

void move_modes::reset()
{
    all_move_modes.reset();
}

const std::vector<move_mode>& move_modes::get_all()
{
    return all_move_modes.get_all();
}

const move_mode_id move_modes::get_by_cycle_id(int cycle_id) {
    for (const move_mode& mm : all_move_modes.get_all()) {
        /*if (mm.default_mode) {
            default_move_mode = mm;
        }*/

        if (!mm.can_be_cycled_to) continue;
        if (mm.cycle_index == cycle_id) {
            return mm.id;
        }
    }
    return MM_WALK;
}

move_mode_id MM_NULL,
MM_WALK,
MM_RUN,
MM_CROUCH;

void move_modes::set_move_mode_ids()
{
    

    MM_NULL = move_mode_id("MM_NULL");
    MM_WALK = move_mode_id("MM_WALK");
    MM_RUN = move_mode_id("MM_RUN");
    MM_CROUCH = move_mode_id("MM_CROUCH");
}
