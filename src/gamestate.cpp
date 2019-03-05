#include "gamestate.h"
#include "cata_utility.h"
#include "path_info.h"
#include "json.h"
#include "input.h"


void gamestate_integration::init()
{
}

void gamestate_integration::update_input_context()
{
        if (!input_context::input_context_stack.empty()) {
            gsi_input_context = *--input_context::input_context_stack.end();
        }
}

void gamestate_integration::write_out()
{
	write_to_file(FILENAMES["gsi_output"], [&](std::ostream & data_file) {
	JsonOut jsout(data_file, true);

	jsout.start_array();
	for (t_action_contexts::const_iterator a = action_contexts.begin(); a != action_contexts.end();
		++a) {
		const t_actions &actions = a->second;
		for (const auto &action : actions) {
			const t_input_event_list &events = action.second.input_events;
			jsout.start_object();

			jsout.member("id", action.first);
			jsout.member("category", a->first);
			bool is_user_created = action.second.is_user_created;
			if (is_user_created) {
				jsout.member("is_user_created", is_user_created);
			}

			jsout.member("bindings");
			jsout.start_array();
			for (const auto &event : events) {
				jsout.start_object();
				switch (event.type) {
				case CATA_INPUT_KEYBOARD:
					jsout.member("input_method", "keyboard");
					break;
				case CATA_INPUT_GAMEPAD:
					jsout.member("input_method", "gamepad");
					break;
				case CATA_INPUT_MOUSE:
					jsout.member("input_method", "mouse");
					break;
				default:
					throw std::runtime_error("unknown input_event_t");
				}
				jsout.member("key");
				jsout.start_array();
				for (size_t i = 0; i < event.sequence.size(); i++) {
					jsout.write(get_keyname(event.sequence[i], event.type, true));
				}
				jsout.end_array();
				jsout.end_object();
			}
			jsout.end_array();

			jsout.end_object();
		}
	}
	jsout.end_array();
}, _("game state information"));
}
