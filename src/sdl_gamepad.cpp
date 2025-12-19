#if defined(TILES)
#include "sdl_gamepad.h"

#include <array>
#include <cmath>
#include <ostream>

#include "debug.h"
#include "input.h"
#include "input_enums.h"
#include "point.h"
#include "ui_manager.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace gamepad
{

static constexpr int max_triggers = 2;
static constexpr int max_sticks = 2;
static constexpr int max_buttons = 30;

static std::array<int, max_triggers> triggers_axis = {
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT
};
static std::array<std::array<int, 2>, max_sticks> sticks_axis = { {
        { {SDL_CONTROLLER_AXIS_LEFTX,  SDL_CONTROLLER_AXIS_LEFTY}  },
        { {SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY} }
    }
};

static std::array<int, max_triggers> triggers_state = {0, 0};

static int triggers_threshold = 16000;
static int sticks_threshold = 16000;
static int error_margin = 2000;

struct task_t {
    Uint32 when;
    int button;
    int counter;
    int state;
    input_event_t type;
};

static constexpr int max_tasks           = max_buttons + max_sticks + max_triggers + 1;
static constexpr int sticks_task_index   = max_buttons;
static constexpr int triggers_task_index = max_buttons + max_sticks;

static std::array<task_t, max_tasks> all_tasks;

static int repeat_delay = 400;
static int repeat_interval = 100;

// SDL related stuff
static SDL_TimerID timer_id;
static SDL_GameController *controller = nullptr;
static SDL_JoystickID controller_id = -1;

static direction left_stick_dir = direction::NONE;
static direction right_stick_dir = direction::NONE;
static direction radial_left_last_dir = direction::NONE;
static direction radial_right_last_dir = direction::NONE;
static bool radial_left_open = false;
static bool radial_right_open = false;
static bool alt_modifier_held = false;
static constexpr Uint32 fast_move_hold_ms = 200;

static Uint32 timer_func( Uint32 interval, void * )
{
    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_GAMEPAD_SCHEDULER;
    userevent.code = 0;
    userevent.data1 = nullptr;
    userevent.data2 = nullptr;

    event.type = SDL_GAMEPAD_SCHEDULER;
    event.user = userevent;

    SDL_PushEvent( &event );
    return interval;
}

void init()
{
    for( gamepad::task_t &task : all_tasks ) {
        task.counter = 0;
    }

    int ret = SDL_Init( SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER );
    if( ret < 0 ) {
        printErrorIf( ret != 0, "Init gamecontroller+timer failed" );
        return;
    }

    if( SDL_NumJoysticks() > 0 ) {
        controller = SDL_GameControllerOpen( 0 );
        if( controller ) {
            controller_id = SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( controller ) );
            SDL_GameControllerEventState( SDL_ENABLE );
        }
    }

    timer_id = SDL_AddTimer( 50, timer_func, nullptr );
    printErrorIf( timer_id == 0, "SDL_AddTimer failed" );
}

void quit()
{
    if( timer_id ) {
        SDL_RemoveTimer( timer_id );
        timer_id = 0;
    }
    if( controller ) {
        SDL_GameControllerClose( controller );
        controller = nullptr;
    }
}

SDL_GameController *get_controller()
{
    return controller;
}

static int one_of_two( const std::array<int, 2> &arr, int val )
{
    if( arr[0] == val ) {
        return 0;
    }
    if( arr[1] == val ) {
        return 1;
    }
    return -1;
}

static void schedule_task( task_t &task, Uint32 when, int button, int state,
                           input_event_t type = input_event_t::gamepad )
{
    task.when = when;
    task.button = button;
    task.counter = 1;
    task.state = state;
    task.type = type;
}

static void cancel_task( task_t &task )
{
    task.counter = 0;
}

static void send_input( int ibtn, input_event_t itype = input_event_t::gamepad )
{
    last_input = input_event( ibtn, itype );
}


// Helper to get ALT variant of a button code
static int get_alt_button( int base_code )
{
    switch( base_code ) {
        case JOY_A:  return JOY_ALT_A;
        case JOY_B:  return JOY_ALT_B;
        case JOY_X:  return JOY_ALT_X;
        case JOY_Y:  return JOY_ALT_Y;
        case JOY_LB: return JOY_ALT_LB;
        case JOY_RB: return JOY_ALT_RB;
        case JOY_RT: return JOY_ALT_RT;
        case JOY_LS: return JOY_ALT_LS;
        case JOY_RS: return JOY_ALT_RS;
        case JOY_DPAD_UP:    return JOY_ALT_DPAD_UP;
        case JOY_DPAD_DOWN:  return JOY_ALT_DPAD_DOWN;
        case JOY_DPAD_LEFT:  return JOY_ALT_DPAD_LEFT;
        case JOY_DPAD_RIGHT: return JOY_ALT_DPAD_RIGHT;
        case JOY_START: return JOY_ALT_START;
        case JOY_BACK:  return JOY_ALT_BACK;
        default: return base_code;
    }
}

// Convert direction to numpad key ('1'-'9')
static int direction_to_num_key( direction dir )
{
    switch( dir ) {
        case direction::N:  return '8';
        case direction::NE: return '9';
        case direction::E:  return '6';
        case direction::SE: return '3';
        case direction::S:  return '2';
        case direction::SW: return '1';
        case direction::W:  return '4';
        case direction::NW: return '7';
        default: return -1;
    }
}

// Convert direction to radial joy event
int direction_to_radial_joy( direction dir, int stick_idx )
{
    if( dir == direction::NONE ) {
        return -1;
    }
    int base = ( stick_idx == 0 ) ? JOY_RADIAL_L_N : JOY_RADIAL_R_N;
    switch( dir ) {
        case direction::N:  return base + 0;
        case direction::NE: return base + 1;
        case direction::E:  return base + 2;
        case direction::SE: return base + 3;
        case direction::S:  return base + 4;
        case direction::SW: return base + 5;
        case direction::W:  return base + 6;
        case direction::NW: return base + 7;
        default: return -1;
    }
}

// Calculate 8-way direction from raw axis values using angle
static direction angle_to_direction( int x, int y )
{
    // atan2 returns angle in radians, with 0 pointing right (East)
    // We want 0 to be North, so we use atan2(x, -y)
    double angle = std::atan2( static_cast<double>( x ), static_cast<double>( -y ) );
    
    // Convert to degrees (0-360, with 0 = North, clockwise)
    double degrees = angle * 180.0 / M_PI;
    if( degrees < 0 ) {
        degrees += 360.0;
    }
    
    // Each direction covers 45 degrees, centered on its cardinal/diagonal
    // N: 337.5 - 22.5, NE: 22.5 - 67.5, E: 67.5 - 112.5, etc.
    if( degrees >= 337.5 || degrees < 22.5 ) {
        return direction::N;
    } else if( degrees >= 22.5 && degrees < 67.5 ) {
        return direction::NE;
    } else if( degrees >= 67.5 && degrees < 112.5 ) {
        return direction::E;
    } else if( degrees >= 112.5 && degrees < 157.5 ) {
        return direction::SE;
    } else if( degrees >= 157.5 && degrees < 202.5 ) {
        return direction::S;
    } else if( degrees >= 202.5 && degrees < 247.5 ) {
        return direction::SW;
    } else if( degrees >= 247.5 && degrees < 292.5 ) {
        return direction::W;
    } else {
        return direction::NW;
    }
}

// Helper to send directional movement input
static void send_direction_movement()
{
    if( left_stick_dir == direction::NONE ) {
        return;
    }
    
    // Convert direction to old-style JOY movement for keybinding lookup
    switch( left_stick_dir ) {
        case direction::N:  send_input( JOY_L_UP ); break;
        case direction::NE: send_input( JOY_L_RIGHTUP ); break;
        case direction::E:  send_input( JOY_L_RIGHT ); break;
        case direction::SE: send_input( JOY_L_RIGHTDOWN ); break;
        case direction::S:  send_input( JOY_L_DOWN ); break;
        case direction::SW: send_input( JOY_L_LEFTDOWN ); break;
        case direction::W:  send_input( JOY_L_LEFT ); break;
        case direction::NW: send_input( JOY_L_LEFTUP ); break;
        default: break;
    }
}

bool handle_axis_event( SDL_Event &event )
{
    if( event.type != SDL_CONTROLLERAXISMOTION ) {
        return false;
    }

    int axis = event.caxis.axis;
    int value = event.caxis.value;
    Uint32 now = event.caxis.timestamp;
    bool direction_changed = false;

    // check triggers
    int idx = one_of_two( triggers_axis, axis );
    if( idx >= 0 ) {
        int state = triggers_state[idx];
        task_t &task = all_tasks[triggers_task_index + idx];
        bool trigger_pressed = value > triggers_threshold + error_margin;
        bool trigger_released = value < triggers_threshold - error_margin;
        
            if( idx == 1 ) {
                // Right trigger (RT)
                if( !state && trigger_pressed ) {
                    triggers_state[idx] = 1;
                    // Check if direction is selected
                    if( left_stick_dir != direction::NONE ) {
                        // Move in selected direction
                        send_direction_movement();
                        // Schedule repeat for continuous movement
                        schedule_task( task, now + fast_move_hold_ms, -1, 1 );
                    } else {
                        // No direction selected - send JOY_RT event (with ALT if held) and schedule repeat
                        int joy_code = alt_modifier_held ? get_alt_button( JOY_RT ) : JOY_RT;
                        send_input( joy_code );
                        schedule_task( task, now + repeat_delay, joy_code, 1 );
                    }
                }
                if( state && trigger_released ) {
                    triggers_state[idx] = 0;
                    cancel_task( task );
                }
            } else {
                // Left trigger (LT)
                if( !state && trigger_pressed ) {
                    alt_modifier_held = true;
                    triggers_state[idx] = 1;
                }
                if( state && trigger_released ) {
                    triggers_state[idx] = 0;
                    alt_modifier_held = false;

                    // Trigger radial select on Alt release
                    if( radial_left_open && radial_left_last_dir != direction::NONE ) {
                        int joy_code = direction_to_radial_joy( radial_left_last_dir, 0 );
                        if( joy_code != -1 ) {
                            send_input( joy_code );
                        }
                    }
                    if( radial_right_open && radial_right_last_dir != direction::NONE ) {
                        int joy_code = direction_to_radial_joy( radial_right_last_dir, 1 );
                        if( joy_code != -1 ) {
                            send_input( joy_code );
                        }
                    }

                    // Reset radial menu states when LT is released
                    radial_left_open = false;
                    radial_right_open = false;
                    radial_left_last_dir = direction::NONE;
                    radial_right_last_dir = direction::NONE;
                }
            }
            return false;
        }

        // check sticks
        for( int i = 0; i < max_sticks; ++i ) {
            int axis_idx = one_of_two( sticks_axis[i], axis );
            if( axis_idx >= 0 ) {
                // Get current values for both axes of this stick
                int x_val = SDL_GameControllerGetAxis( controller,
                            static_cast<SDL_GameControllerAxis>( sticks_axis[i][0] ) );
                int y_val = SDL_GameControllerGetAxis( controller,
                            static_cast<SDL_GameControllerAxis>( sticks_axis[i][1] ) );

                // Calculate magnitude (distance from center)
                double magnitude = std::sqrt( static_cast<double>( x_val ) * x_val +
                                              static_cast<double>( y_val ) * y_val );

                direction dir = ( magnitude > sticks_threshold ) ? angle_to_direction( x_val,
                                y_val ) : direction::NONE;

                task_t &stick_task = all_tasks[sticks_task_index + i];
                direction old_dir = ( i == 0 ) ? left_stick_dir : right_stick_dir;

                if( i == 0 ) {
                    // Left stick (i=0): Update selected direction
                    left_stick_dir = dir;
                    // Signal UI refresh needed if direction changed and not in menu
                    if( old_dir != left_stick_dir && !is_in_menu() ) {
                        direction_changed = true;
                    }
                } else {
                    right_stick_dir = dir;
                }

                if( alt_modifier_held ) {
                    // When LT is held, sticks control radial menu state
                    direction &radial_last = ( i == 0 ) ? radial_left_last_dir : radial_right_last_dir;
                    bool &radial_open = ( i == 0 ) ? radial_left_open : radial_right_open;
                    bool &radial_other = ( i == 0 ) ? radial_right_open : radial_left_open;

                    // Only update direction when stick is deflected beyond threshold
                    if( dir != direction::NONE ) {
                        radial_open = true;
                        radial_other = false; // Mutually exclusive
                        if( dir != radial_last ) {
                            radial_last = dir;
                        }
                    }
                    // When stick is in deadzone (released), do nothing - keep last direction

                    // Cancel any existing repeat tasks when entering radial mode
                    cancel_task( stick_task );
                } else if( i == 1 ) {
                    // Right stick: Output num keys in menu, or joy directions in gameplay
                    if( is_in_menu() ) {
                        int num_key = direction_to_num_key( dir );
                        if( num_key != -1 ) {
                            send_input( num_key, input_event_t::keyboard_char );
                            schedule_task( stick_task, now + repeat_delay, num_key, 1, input_event_t::keyboard_char );
                        } else {
                            cancel_task( stick_task );
                        }
                    } else {
                        // Right stick (i=1) in gameplay: Map to stick JOY_R_* input code and schedule repeat
                        int joy_code = -1;

                        switch( dir ) {
                            case direction::N:
                                joy_code = JOY_R_UP;
                                break;
                            case direction::E:
                                joy_code = JOY_R_RIGHT;
                                break;
                            case direction::S:
                                joy_code = JOY_R_DOWN;
                                break;
                            case direction::W:
                                joy_code = JOY_R_LEFT;
                                break;
                            case direction::NE:
                                joy_code = JOY_R_RIGHTUP;
                                break;
                            case direction::SE:
                                joy_code = JOY_R_RIGHTDOWN;
                                break;
                            case direction::SW:
                                joy_code = JOY_R_LEFTDOWN;
                                break;
                            case direction::NW:
                                joy_code = JOY_R_LEFTUP;
                                break;
                            case direction::NONE:
                                // Cancel repeat when stick returns to neutral
                                cancel_task( stick_task );
                                break;
                        }

                        if( joy_code != -1 ) {
                            send_input( joy_code );
                            // Schedule repeat for held direction
                            schedule_task( stick_task, now + repeat_delay, joy_code, 1 );
                        }
                    }
                } else {
                    // Left stick (i == 0) and not in radial mode: No direct input or repeat.
                    // It only updates left_stick_dir (handled above).
                    cancel_task( stick_task );
                }
            }
        }
        return direction_changed;
    }


void handle_button_event( SDL_Event &event )
{
    int button = event.cbutton.button;
    Uint32 now = event.cbutton.timestamp;

    if( event.type == SDL_CONTROLLERBUTTONUP ) {
        // Button released - only handle cleanup/task cancellation
        if( button < max_buttons ) {
            cancel_task( all_tasks[button] );
        }
        return;
    }

    // SDL_CONTROLLERBUTTONDOWN:
    // Button pressed - determine which input to send
    int joy_code = -1;
    input_event_t input_type = input_event_t::gamepad;

    switch( button ) {
        case SDL_CONTROLLER_BUTTON_A:
            if( is_in_menu() ) {
                joy_code = '\n';
                input_type = input_event_t::keyboard_char;
            } else {
                joy_code = JOY_A;
            }
            break;
        case SDL_CONTROLLER_BUTTON_B:
            if( is_in_menu() ) {
                joy_code = KEY_ESCAPE;
                input_type = input_event_t::keyboard_code;
            } else {
                joy_code = JOY_B;
            }
            break;
        case SDL_CONTROLLER_BUTTON_X:
            joy_code = JOY_X;
            break;
        case SDL_CONTROLLER_BUTTON_Y:
            joy_code = JOY_Y;
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            if( is_in_menu() ) {
                joy_code = '\t';
                input_type = input_event_t::keyboard_char;
            } else {
                joy_code = JOY_RB;
            }
            break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            if( is_in_menu() ) {
                joy_code = KEY_BTAB;
                input_type = input_event_t::keyboard_char;
            } else {
                joy_code = JOY_LB;
            }
            break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            joy_code = JOY_LS;
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            joy_code = JOY_RS;
            break;
        case SDL_CONTROLLER_BUTTON_START:
            if( is_in_menu() ) {
                joy_code = KEY_ESCAPE;
                input_type = input_event_t::keyboard_code;
            } else {
                joy_code = JOY_START;
            }
            break;
        case SDL_CONTROLLER_BUTTON_BACK:
            if( is_in_menu() ) {
                joy_code = KEY_ESCAPE;
                input_type = input_event_t::keyboard_code;
            } else {
                joy_code = JOY_BACK;
            }
            break;

        // D-pad handling
        // If in a menu, send keyboard arrow keys for navigation
        // If in gameplay, send JOY_DPAD_* codes for mapped actions
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            if( is_in_menu() ) {
                joy_code = KEY_UP;
                input_type = input_event_t::keyboard_char;
            } else {
                joy_code = JOY_DPAD_UP;
            }
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            if( is_in_menu() ) {
                joy_code = KEY_DOWN;
                input_type = input_event_t::keyboard_char;
            } else {
                joy_code = JOY_DPAD_DOWN;
            }
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            if( is_in_menu() ) {
                joy_code = KEY_LEFT;
                input_type = input_event_t::keyboard_char;
            } else {
                joy_code = JOY_DPAD_LEFT;
            }
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            if( is_in_menu() ) {
                joy_code = KEY_RIGHT;
                input_type = input_event_t::keyboard_char;
            } else {
                joy_code = JOY_DPAD_RIGHT;
            }
            break;

        default:
            break;
    }

    // Apply ALT modifier if held (only for non-keyboard inputs)
    if( joy_code != -1 && alt_modifier_held && input_type == input_event_t::gamepad ) {
        joy_code = get_alt_button( joy_code );
    }

    if( joy_code != -1 ) {
        send_input( joy_code, input_type );
        // Only D-pad buttons repeat
        if( button >= SDL_CONTROLLER_BUTTON_DPAD_UP && button <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT ) {
            schedule_task( all_tasks[button], now + repeat_delay, joy_code, 1, input_type );
        }
    }
}

void handle_device_event( SDL_Event &event )
{
    if( event.type == SDL_CONTROLLERDEVICEADDED ) {
        if( controller == nullptr ) {
            controller = SDL_GameControllerOpen( event.cdevice.which );
            if( controller ) {
                controller_id = SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( controller ) );
            }
        }
    } else if( event.type == SDL_CONTROLLERDEVICEREMOVED ) {
        if( controller != nullptr && event.cdevice.which == controller_id ) {
            SDL_GameControllerClose( controller );
            controller = nullptr;
            controller_id = -1;
        }
    }
}

void handle_scheduler_event( SDL_Event &event )
{
    Uint32 now = event.user.timestamp;
    for( size_t i = 0; i < all_tasks.size(); ++i ) {
        gamepad::task_t &task = all_tasks[i];
        if( task.counter && task.when <= now ) {
            // Check if this is the RT repeat task
            if( i == triggers_task_index + 1 && left_stick_dir != direction::NONE ) {
                // RT continuous movement - send direction
                send_direction_movement();
            } else if( i >= sticks_task_index && i < triggers_task_index ) {
                // Stick repeat (sticks_task_index + i where i is stick index)
                if( task.button != -1 ) {
                    send_input( task.button, task.type );
                }
            } else if( task.button != -1 ) {
                // Regular button repeat (including RT when no direction is held)
                send_input( task.button, task.type );
            }
            task.counter += 1;
            task.when = now + repeat_interval;
        }
    }
}

direction get_left_stick_direction()
{
    return left_stick_dir;
}

direction get_right_stick_direction()
{
    return right_stick_dir;
}

direction get_radial_left_direction()
{
    return radial_left_last_dir;
}

direction get_radial_right_direction()
{
    return radial_right_last_dir;
}

bool is_radial_left_open()
{
    return radial_left_open;
}

bool is_radial_right_open()
{
    return radial_right_open;
}

bool is_alt_held()
{
    return alt_modifier_held;
}

bool is_in_menu()
{
    // If the UI stack has more than 1 element, it usually means a menu is open
    // on top of the main game UI.
    return ui_adaptor::ui_stack_size() > 1;
}

bool is_active()
{
    return controller != nullptr;
}

tripoint direction_to_offset( direction dir )
{
    switch( dir ) {
        case direction::N:
            return tripoint( 0, -1, 0 );
        case direction::NE:
            return tripoint( 1, -1, 0 );
        case direction::E:
            return tripoint( 1, 0, 0 );
        case direction::SE:
            return tripoint( 1, 1, 0 );
        case direction::S:
            return tripoint( 0, 1, 0 );
        case direction::SW:
            return tripoint( -1, 1, 0 );
        case direction::W:
            return tripoint( -1, 0, 0 );
        case direction::NW:
            return tripoint( -1, -1, 0 );
        case direction::NONE:
        default:
            return tripoint::zero;
    }
}

} // namespace gamepad

#endif // TILES
