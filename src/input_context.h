#pragma once

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#if defined(__ANDROID__)
#include <list>
#endif

#include "action.h"
#include "input_enums.h"
#include "point.h"
#include "translation.h"

enum class kb_menu_status;

class hotkey_queue;
class keybindings_ui;
namespace catacurses
{
class window;
} // namespace catacurses

/**
 * Represents a context in which a set of actions can be performed.
 *
 * This class is responsible for registering possible actions
 * (traditionally keypresses), handling input, and yielding the correct
 * action string descriptors for given input.
 *
 * This turns this class into an abstraction method between actual
 * input(keyboard, gamepad etc.) and game.
 */
class input_context
{
        friend class keybindings_ui;
    public:
#if defined(__ANDROID__)
        // Whatever's on top is our current input context.
        static std::list<input_context *> input_context_stack;
#endif

        input_context() : registered_any_input( false ), category( "default" ),
            coordinate_input_received( false ), handling_coordinate_input( false ) {
#if defined(__ANDROID__)
            input_context_stack.push_back( this );
            allow_text_entry = false;
#endif
            register_action( "toggle_language_to_en" );
        }
        // TODO: consider making the curses WINDOW an argument to the constructor, so that mouse input
        // outside that window can be ignored
        explicit input_context( const std::string &category,
                                const keyboard_mode preferred_keyboard_mode = keyboard_mode::keycode )
            : registered_any_input( false ), category( category ),
              coordinate_input_received( false ), handling_coordinate_input( false ),
              preferred_keyboard_mode( preferred_keyboard_mode ) {
#if defined(__ANDROID__)
            input_context_stack.push_back( this );
            allow_text_entry = false;
#endif
            register_action( "toggle_language_to_en" );
        }

#if defined(__ANDROID__)
        virtual ~input_context() {
            input_context_stack.remove( this );
        }

        // HACK: hack to allow creating manual keybindings for getch() instances, uilists etc. that don't use an input_context outside of the Android version
        struct manual_key {
            manual_key( int _key, const std::string &_text ) : key( _key ), text( _text ) {}
            int key;
            std::string text;
            bool operator==( const manual_key &other ) const {
                return key == other.key && text == other.text;
            }
        };

        std::vector<manual_key> registered_manual_keys;

        // If true, prevent virtual keyboard from dismissing after a key press while this input context is active.
        // NOTE: This won't auto-bring up the virtual keyboard, for that use sdltiles.cpp is_string_input()
        bool allow_text_entry;

        void register_manual_key( manual_key mk );
        void register_manual_key( int key, const std::string text = "" );

        std::string get_action_name_for_manual_key( int key ) {
            for( const auto &manual_key : registered_manual_keys ) {
                if( manual_key.key == key ) {
                    return manual_key.text;
                }
            }
            return std::string();
        }
        std::vector<manual_key> &get_registered_manual_keys() {
            return registered_manual_keys;
        }

        std::string &get_category() {
            return category;
        }
        std::vector<std::string> &get_registered_actions() {
            return registered_actions;
        }
        bool is_action_registered( const std::string &action_descriptor ) const {
            return std::find( registered_actions.begin(), registered_actions.end(),
                              action_descriptor ) != registered_actions.end();
        }

        input_context &operator=( const input_context &other ) {
            registered_actions = other.registered_actions;
            registered_manual_keys = other.registered_manual_keys;
            allow_text_entry = other.allow_text_entry;
            registered_any_input = other.registered_any_input;
            category = other.category;
            coordinate = other.coordinate;
            coordinate_input_received = other.coordinate_input_received;
            handling_coordinate_input = other.handling_coordinate_input;
            next_action = other.next_action;
            iso_mode = other.iso_mode;
            action_name_overrides = other.action_name_overrides;
            timeout = other.timeout;
            return *this;
        }

        bool operator==( const input_context &other ) const {
            return category == other.category &&
                   registered_actions == other.registered_actions &&
                   registered_manual_keys == other.registered_manual_keys &&
                   allow_text_entry == other.allow_text_entry &&
                   registered_any_input == other.registered_any_input &&
                   coordinate == other.coordinate &&
                   coordinate_input_received == other.coordinate_input_received &&
                   handling_coordinate_input == other.handling_coordinate_input &&
                   next_action == other.next_action &&
                   iso_mode == other.iso_mode &&
                   action_name_overrides == other.action_name_overrides &&
                   timeout == other.timeout;
        }
        bool operator!=( const input_context &other ) const {
            return !( *this == other );
        }
#endif

        /**
         * Register an action with this input context.
         *
         * Only registered actions will be returned by `handle_input()`, it's
         * thus possible to have multiple actions associated with the same keypress,
         * as long as they don't ever occur in the same input context.
         *
         * If `action_descriptor` is the special "ANY_INPUT", instead of ignoring
         * unregistered keys, those keys will all be linked to this "ANY_INPUT"
         * action.
         *
         * If `action_descriptor` is the special "COORDINATE", coordinate input will be processed
         * and the specified coordinates can be retrieved using `get_coordinates()`. Currently the
         * only form of coordinate input is mouse input(you can directly click coordinates on
         * the screen).
         *
         * @param action_descriptor String of action id.
         */
        void register_action( const std::string &action_descriptor );

        /**
         * Same as other @ref register_action function but allows a context specific
         * action name. The given name is displayed instead of the name taken from
         * the @ref input_manager.
         *
         * @param action_descriptor String of action id.
         * @param name Name of the action, displayed to the user. If empty use the
         * name reported by the input_manager.
         */
        void register_action( const std::string &action_descriptor, const translation &name );

        /**
         * Get the set of available single character keyboard keys that do not
         * conflict with any registered hotkeys.  The result will only include
         * characters from the requested_keys parameter that have no conflicts
         * i.e. the set difference requested_keys - conflicts.
         *
         * @param requested_keys The set of single character hotkeys to
         *                       potentially use. Defaults to all printable ascii.
         *
         * @return Returns the set of non-conflicting, single character keyboard
         *         keys suitable for use as hotkeys.
         */
        std::string get_available_single_char_hotkeys(
            std::string requested_keys =
                "abcdefghijkpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-=:;'\",./<>?!@#$%^&*()_+[]\\{}|`~" );

        using input_event_filter = std::function<bool( const input_event & )>;

        // Helper functions to be used as @ref input_event_filter
        static bool disallow_lower_case_or_non_modified_letters( const input_event &evt );
        static bool allow_all_keys( const input_event &evt );

        /**
         * Get a description text for the key/other input method associated
         * with the given action. If there are multiple bound keys, no more
         * than max_limit will be described in the result. In addition, only
         * keys satisfying evt_filter will be described.
         *
         * @param action_descriptor The action descriptor for which to return
         *                          a description of the bound keys.
         *
         * @param max_limit No more than max_limit bound keys will be
         *                  described in the returned description. A value of
         *                  0 indicates no limit.
         *
         * @param evt_filter Only keys satisfying this function will be
         *                   described.
         */
        std::string get_desc( const std::string &action_descriptor,
                              unsigned int max_limit = 0,
                              const input_event_filter &evt_filter = allow_all_keys ) const;

        /**
         * Get a description for the action parameter along with a printable hotkey
         * for the description in the format [%s] %s
         *
         * @param action_descriptor The action descriptor for which to return
         *                          a description of the bound keys.
         */
        std::string get_button_text( const std::string &action_descriptor ) const;

        /**
         * Get a description for the action parameter along with a printable hotkey
         * for the description in the format [%s] %s
         *
         * @param action_descriptor The action descriptor for which to return
         *                          a description of the bound keys.
         *
         * @param action_text The human readable description of the action.
         */
        std::string get_button_text( const std::string &action_descriptor,
                                     const std::string &action_text ) const;

        /**
         * Get a description based on `text`. If a bound key for `action_descriptor`
         * satisfying `evt_filter` is contained in `text`, surround the key with
         * brackets and change the case if necessary (e.g. "(Y)es"). Otherwise
         * prefix `text` with description of the first bound key satisfying
         * `evt_filter`, surrounded in square brackets (e.g "[RETURN] Yes").
         *
         * @param action_descriptor The action descriptor for which to return
         *                          a description of the bound keys.
         *
         * @param text The base text for action description
         *
         * @param evt_filter Only keys satisfying this function will be considered
         * @param inline_fmt Action description format when a key is found in the
         *                   text (for example "(a)ctive")
         * @param separate_fmt Action description format when a key is not found
         *                     in the text (for example "[X] active" or "[N/A] active")
         */
        std::string get_desc( const std::string &action_descriptor,
                              const std::string &text,
                              const input_event_filter &evt_filter,
                              const translation &inline_fmt,
                              const translation &separate_fmt ) const;
        std::string get_desc( const std::string &action_descriptor,
                              const std::string &text,
                              const input_event_filter &evt_filter = allow_all_keys ) const;

        /**
         * Equivalent to get_desc( act, get_action_name( act ), filter )
         **/
        std::string describe_key_and_name( const std::string &action_descriptor,
                                           const input_event_filter &evt_filter = allow_all_keys ) const;

        /**
         * Handles input and returns the next action in the queue.
         *
         * This internally calls getch() or whatever other input method
         * is available(e.g. gamepad).
         *
         * If the action is mouse input, returns "MOUSE".
         *
         * @param timeout in milliseconds.
         * @return One of the input actions formerly registered with
         *         `register_action()`, or "ERROR" if an error happened.
         *
         */
        const std::string &handle_input();
        const std::string &handle_input( int timeout );

        /**
         * Convert a direction action (UP, DOWN etc) to a delta vector.
         *
         * @return If the action is a movement action (UP, DOWN, ...),
         * the delta vector associated with it. Otherwise returns an empty value.
         * The returned vector will always have a z component of 0.
         */
        // TODO: Get rid of untyped version and change name of the typed one.
        std::optional<tripoint> get_direction( const std::string &action ) const;
        std::optional<tripoint_rel_ms> get_direction_rel_ms( const std::string &action ) const;

        /**
         * Get the coordinates associated with the last mouse click (if any).
         */
        std::optional<tripoint> get_coordinates( const catacurses::window &capture_win_,
                const point &offset = point_zero, bool center_cursor = false ) const;

        // Below here are shortcuts for registering common key combinations.
        void register_directions();
        void register_updown();
        void register_leftright();
        void register_cardinal();
        void register_navigate_ui_list();

        /**
         * Display the possible actions in the current context and their
         * keybindings.
         * @param permit_execute_action If `true` the function allows the user to specify an action to execute
         * @return action_id of any action the user specified to execute, or ACTION_NULL if none
         */
        action_id display_menu( bool permit_execute_action = false );
    private:
        /**
         * Reset action to default keybindings.
         * Prompt the user to resolve conflicts if they arise.
         * @return true if keybinding changed
         */
        bool action_reset( const std::string &action_id );
        /**
         * Prompt the user to add an event to an action.
         * Prompt the user to resolve conflicts if they arise.
         * @return true if keybinding changed
         */
        bool action_add( const std::string &name, const std::string &action_id, bool is_local,
                         kb_menu_status status );
        /**
         * Remove an action.
         * Prompt the user to resolve conflicts if they arise.
         * @return true if keybinding changed
         */
        bool action_remove( const std::string &name, const std::string &action_id, bool is_local,
                            bool is_empty );
    public:

        /**
         * Temporary method to retrieve the raw input received, so that input_contexts
         * can be used in screens where not all possible actions have been defined in
         * keybindings.json yet.
         */
        input_event get_raw_input();

        /**
         * Get coordinate of text level from mouse input, difference between this and get_coordinates is that one is getting pixel level coordinate.
         */
        std::optional<point> get_coordinates_text( const catacurses::window &capture_win ) const;

        /**
         * Get the human-readable name for an action.
         */
        std::string get_action_name( const std::string &action_id ) const;

        /* For the future, something like this might be nice:
         * const std::string register_action(const std::string& action_descriptor, x, y, width, height);
         * (x, y, width, height) would describe an area on the visible window that, if clicked, triggers the action.
         */

        // (Press X (or Y)|Try) to Z
        std::string press_x( const std::string &action_id ) const;
        std::string press_x( const std::string &action_id, const std::string &key_bound,
                             const std::string &key_unbound ) const;
        std::string press_x( const std::string &action_id, const std::string &key_bound_pre,
                             const std::string &key_bound_suf, const std::string &key_unbound ) const;

        /**
         * Keys (and only keys, other input types are not included) that
         * trigger the given action.
         * @param action_descriptor The action descriptor for which to get the bound keys.
         * @param maximum_modifier_count Maximum number of modifiers allowed for
         *        the returned action. <0 means any number is allowed.
         * @param restrict_to_printable If `true`, the function returns the bound
         *        keys only if they are printable (space counts as non-printable
         *        here). If `false`, all keys (whether they are printable or not)
         *        are returned.
         * @param restrict_to_keyboard If `true`, the function returns the bound keys only
         *        if they are keyboard inputs. If `false`, all inputs, such as mouse
         *        inputs are included.
         * @returns All keys bound to the given action descriptor.
         */
        std::vector<input_event> keys_bound_to( const std::string &action_descriptor,
                                                int maximum_modifier_count = -1,
                                                bool restrict_to_printable = true,
                                                bool restrict_to_keyboard = true ) const;

        /**
        * Get/Set edittext to display IME unspecified string.
        */
        void set_edittext( const std::string &s );
        std::string get_edittext();

        void set_iso( bool mode = true );

        /**
         * Sets input polling timeout as appropriate for the current interface system.
         * Use this method to set timeouts when using input_context, rather than calling
         * the old timeout() method or using input_manager::(re)set_timeout, as using
         * this method will cause input_event_t::timeout events to be generated correctly,
         * and will reset timeout correctly when a new input context is entered.
         */
        void set_timeout( int val );
        void reset_timeout();

        input_event first_unassigned_hotkey( const hotkey_queue &queue ) const;
        input_event next_unassigned_hotkey( const hotkey_queue &queue, const input_event &prev ) const;
    private:

        std::vector<std::string> registered_actions;
        std::string edittext;
    public:
        const std::string &input_to_action( const input_event &inp ) const;
        bool is_event_type_enabled( input_event_t type ) const;
        bool is_registered_action( const std::string &action_name ) const;
    private:
        bool registered_any_input;
        std::string category; // The input category this context uses.
        point coordinate;
        bool coordinate_input_received;
        bool handling_coordinate_input;
        input_event next_action;
        bool iso_mode = false; // should this context follow the game's isometric settings?
        int timeout = -1;
        keyboard_mode preferred_keyboard_mode = keyboard_mode::keycode;

        /**
         * When registering for actions within an input_context, callers can
         * specify a custom action name that will override the action's normal
         * name. This map stores those overrides. The key is the action ID and the
         * value is the user-visible name.
         */
        std::map<std::string, translation> action_name_overrides;

        /**
         * Returns whether action uses the specified input
         */
        bool action_uses_input( const std::string &action_id, const input_event &event ) const;
        /**
         * Return a user presentable list of actions that conflict with the
         * proposed keybinding. Returns an empty string if nothing conflicts.
         */
        std::string get_conflicts( const input_event &event, const std::string &ignore_action ) const;
        /**
         * Clear an input_event from all conflicting keybindings (excluding conflicts for `ignore_action`) that are
         * registered by this input_context in default and current context (see `category`).
         *
         * @param event The input event to be cleared from conflicting
         * keybindings.
         */
        void clear_conflicting_keybindings( const input_event &event, const std::string &ignore_action );
        /**
         * Find all conflicts for all `events` (excluding conflicts for `ignore_action`).
         * If there are any, prompt the user "Should they be cleared?".
         * Then, clear all input_events from all conflicting keybindings (actions)
         * in both the default and current context (see `category`).
         *
         * @param events The input events to be cleared from conflicting actions
         * @return true if cleared (user agreed) or if no conflicts found
         */
        bool resolve_conflicts( const std::vector<input_event> &events, const std::string &ignore_action );
        /**
         * Filter a vector of strings by a phrase, returning only strings that contain the phrase.
         *
         * @param strings The vector of strings to filter
         * @param phrase  The phrase to search within each of the given strings
         * @return A vector of the filtered strings
         */
        std::vector<std::string> filter_strings_by_phrase( const std::vector<std::string> &strings,
                std::string_view phrase ) const;
};

class hotkey_queue
{
    public:
        // ctxt is only used for determining hotkey input type
        // use input_context::first_unassigned_hotkey() instead to skip assigned actions
        input_event first( const input_context &ctxt ) const;
        // use input_context::next_unassigned_hotkey() instead to skip assigned actions
        input_event next( const input_event &prev ) const;

        /**
         * In keychar mode:
         *   a-z, A-Z
         * In keycode mode:
         *   a-z, shift a-z
         */
        static const hotkey_queue &alphabets();

        /**
         * In keychar mode:
         *   1-0, a-z, A-Z
         * In keycode mode:
         *   1-0, a-z, shift 1-0, shift a-z
         */
        static const hotkey_queue &alpha_digits();

    private:
        std::vector<int> codes_keychar;
        std::vector<int> codes_keycode;
        std::vector<std::set<keymod_t>> modifiers_keycode;
};
