#include "path_manager.h"

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cata_assert.h"
#include "cata_imgui.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "coordinates.h"
#include "coords_fwd.h"
#include "debug.h"
#include "enums.h"
#include "filesystem.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "json.h"
#include "json_error.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "output.h"
#include "path_info.h"
#include "translations.h"
#include "ui_manager.h"

class path
{
        friend path_manager;
        friend path_manager_impl;
        friend class path_manager_ui;
    public:
        path() = default;
        path( const path &other ) = default;
        explicit path( std::vector<tripoint_abs_ms> &&recorded_path_in )
            : recorded_path( std::move( recorded_path_in ) ) {}
        path &operator=( const path &other ) = default;
        path &operator=( path &&other ) = default;
        ~path() = default;
        /**
         * Record a single step of path.
         * If this step makes a loop, remove the whole loop.
         */
        void record_step( const tripoint_abs_ms &new_pos );
        /**
         * Set avatar to auto route to this paths other end.
         * Avatar must be at one of the ends of the path.
         */
        void set_avatar_path();
    private:
        std::vector<tripoint_abs_ms> recorded_path;
        // std::string name_start;
        // std::string name_end;
        /// There are no 2 same tiles on the path
        // bool optimize_loops;
        /// A Character walking this path could never go from i-th tile to i+k-th tile, where k > 1
        // bool optimize_nearby;
};

class path_manager_impl
{
        friend path_manager;
        friend class path_manager_ui;
    public:
        /**
         * Try to find a path then walk on it.
         * Start new path if player doesn't stand at start or end of any path.
         */
        void auto_route_from_path();
        /**
         * Start recording a new path.
         */
        void start_recording();
        /**
         * Stop recording path.
         */
        void stop_recording();
        /**
         * Delete path selected by `select_id`.
         */
        void delete_selected();
        /**
         * Move path selected by `select_id` up.
         */
        void move_selected_up();
        /**
         * Move path selected by `select_id` down.
         */
        void move_selected_down();
    private:
        bool recording_path();
        // todo int or size_t?
        /**
         * Set recording_path_index to p_index.
         * -1 means not recording.
         */
        void set_recording_path( int p_index );
        void swap_selected( int index_a, int index_b );
        int recording_path_index = -1;
        int selected_id = -1;
        std::vector<path> paths;
};

class path_manager_ui : public cataimgui::window
{
    public:
        explicit path_manager_ui( path_manager_impl *pimpl_in );
        void run();

        input_context ctxt = input_context( "PATH_MANAGER" );
        void enabled_active_button( const std::string action, bool enabled );
    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;
    private:
        path_manager_impl *pimpl;
};


void path::record_step( const tripoint_abs_ms &new_pos )
{
    // if a loop exists find it and remove it
    // todo optimize, probably with unordered set
    for( auto it = recorded_path.begin(); it != recorded_path.end(); ++it ) {
        if( *it == new_pos ) {
            const size_t old_path_len = recorded_path.size();
            recorded_path.erase( it + 1, recorded_path.end() );
            add_msg( m_info, _( "Auto path: Made a loop!  Old path len: %s, new path len: %s." ),
                     old_path_len,
                     recorded_path.size() );
            return;
        }
    }
    recorded_path.emplace_back( new_pos );
}

void path::set_avatar_path()
{
    avatar &player_character = get_avatar();
    std::vector<tripoint_bub_ms> route;
    if( player_character.pos_bub() == get_map().bub_from_abs( recorded_path.front() ) ) {
        add_msg( m_info, _( "Auto path: Go from start." ) );
        for( auto it = std::next( recorded_path.begin() ); it != recorded_path.end(); ++it ) {
            route.emplace_back( get_map().bub_from_abs( *it ) );
        }
    } else if( player_character.pos_bub() == get_map().bub_from_abs( recorded_path.back() ) ) {
        add_msg( m_info, _( "Auto path: Go from end." ) );
        for( auto it = std::next( recorded_path.rbegin() ); it != recorded_path.rend(); ++it ) {
            route.emplace_back( get_map().bub_from_abs( *it ) );
        }
    } else {
        debugmsg( "Tried to set auto route but the player character isn't standing at path start or end." );
        return;
    }
    player_character.set_destination( route );
}

void path_manager_impl::start_recording()
{
    paths.emplace_back();
    set_recording_path( paths.size() - 1 );
    paths.back().record_step( get_avatar().get_location() );
}

void path_manager_impl::stop_recording()
{
    const path &current_path = paths[recording_path_index];
    const std::vector<tripoint_abs_ms> &curr_path = current_path.recorded_path;
    if( curr_path.size() <= 1 ) {
        paths.erase( paths.begin() + recording_path_index );
        popup( _( "Recorded path has no lenght.  Path erased." ) );
    } else {
        popup( _( "Path saved." ) );
    }

    set_recording_path( -1 );
    // TODO error when starts or stops at the same tile as another path ??
    // or just prefer the higher path - this allows
    // more flexibility, but it needs to be documented
}

void path_manager_impl::delete_selected()
{
    cata_assert( 0 <= selected_id && selected_id < static_cast<int>( paths.size() ) );
    if( selected_id == recording_path_index ) {
        set_recording_path( -1 );
    } else if( selected_id < recording_path_index ) {
        set_recording_path( recording_path_index - 1 );
    }
    paths.erase( paths.begin() + selected_id );
    selected_id = std::min( selected_id, static_cast<int>( paths.size() ) - 1 );
}

void path_manager_impl::swap_selected( int index_a, int index_b )
{
    cata_assert( 0 <= index_a && index_a < static_cast<int>( paths.size() ) );
    cata_assert( 0 <= index_b && index_b < static_cast<int>( paths.size() ) );
    if( index_a == recording_path_index ) {
        set_recording_path( index_b );
    } else if( index_b == recording_path_index ) {
        set_recording_path( index_a );
    }
    std::swap( paths[index_a], paths[index_b] );
}

void path_manager_impl::move_selected_up()
{
    swap_selected( selected_id, selected_id - 1 );
    --selected_id;
}

void path_manager_impl::move_selected_down()
{
    swap_selected( selected_id, selected_id + 1 );
    ++selected_id;
}

bool path_manager_impl::recording_path()
{
    return recording_path_index != -1;
}

void path_manager_impl::auto_route_from_path()
{
    avatar &player_character = get_avatar();
    for( int path_index = 0; path_index < static_cast<int>( paths.size() ); ++path_index ) {
        const std::vector<tripoint_abs_ms> &p = paths[path_index].recorded_path;
        if( player_character.pos_bub() == get_map().bub_from_abs( p.front() )
            || player_character.pos_bub() == get_map().bub_from_abs( p.back() )
          ) {
            paths[path_index].set_avatar_path();
            return;
        }
    }
    popup( _( "Player doesn't stand at start or end of existing path." ) );
}

void path_manager_impl::set_recording_path( int p_index )
{
    cata_assert( -1 <= p_index && p_index < static_cast<int>( paths.size() ) );
    recording_path_index = p_index;
}

path_manager_ui::path_manager_ui( path_manager_impl *pimpl_in )
    : cataimgui::window( _( "Path Manager" ) ), pimpl( pimpl_in ) {}

cataimgui::bounds path_manager_ui::get_bounds()
{
    return { -1.f, -1.f, float( str_width_to_pixels( TERMX ) ), float( str_height_to_pixels( TERMY ) ) };
}

void path_manager_ui::enabled_active_button( const std::string action, bool enabled )
{
    ImGui::BeginDisabled( !enabled );
    action_button( action, ctxt.get_button_text( action ) );
    ImGui::EndDisabled();
}

void path_manager_ui::draw_controls()
{
    // general buttons
    enabled_active_button( "WALK_PATH", true );
    ImGui::SameLine();
    enabled_active_button( "START_RECORDING", !pimpl->recording_path() );
    ImGui::SameLine();
    enabled_active_button( "STOP_RECORDING", pimpl->recording_path() );

    // buttons related to selected path
    draw_colored_text( _( "Selected path:" ), c_white );
    ImGui::SameLine();
    enabled_active_button( "DELETE_PATH", pimpl->selected_id != -1 );
    ImGui::SameLine();
    enabled_active_button( "MOVE_PATH_UP", pimpl->selected_id > 0 );
    ImGui::SameLine();
    enabled_active_button( "MOVE_PATH_DOWN", pimpl->selected_id != -1 &&
                           pimpl->selected_id + 1 < static_cast<int>( pimpl->paths.size() ) );

    ImGui::BeginChild( "table" );
    if( ! ImGui::BeginTable( "PATH_MANAGER", 5, ImGuiTableFlags_Resizable ) ) {
        return;
    }
    // TODO invlet
    ImGui::TableSetupColumn( "start name" );
    ImGui::TableSetupColumn( "start distance" );
    ImGui::TableSetupColumn( "end name" );
    ImGui::TableSetupColumn( "end distance" );
    ImGui::TableSetupColumn( "total lenght" );
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin( pimpl->paths.size() );
    while( clipper.Step() ) {
        for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
            const path &curr_path = pimpl->paths[i];
            ImGui::TableNextColumn();
            if( ImGui::Selectable( ( "##" + std::to_string( i ) ).c_str(), pimpl->selected_id == i,
                                   ImGuiSelectableFlags_SpanAllColumns )
              ) {
                pimpl->selected_id = i;
            }
            ImGui::SameLine();
            draw_colored_text( "start", c_white );

            ImGui::TableNextColumn();
            std::string dist = direction_suffix( get_avatar().get_location(), curr_path.recorded_path.front() );
            dist = dist == "" ? _( "It's under your feet." ) : dist;
            draw_colored_text( dist, c_white );

            ImGui::TableNextColumn();
            draw_colored_text( "end", c_white );

            ImGui::TableNextColumn();
            dist = direction_suffix( get_avatar().get_location(), curr_path.recorded_path.back() );
            dist = dist == "" ? _( "It's under your feet." ) : dist;
            draw_colored_text( dist, c_white );

            ImGui::TableNextColumn();
            ImGui::Text( "%zu", curr_path.recorded_path.size() );
        }
    }
    ImGui::EndTable();
    ImGui::EndChild();
}

void path_manager_ui::run()
{
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ctxt.register_action( "WALK_PATH" );
    ctxt.register_action( "START_RECORDING" );
    ctxt.register_action( "STOP_RECORDING" );
    ctxt.register_action( "DELETE_PATH" );
    ctxt.register_action( "MOVE_PATH_UP" );
    ctxt.register_action( "MOVE_PATH_DOWN" );
    std::string action;

    ui_manager::redraw();

    while( is_open ) {
        ui_manager::redraw();
        if( has_button_action() ) {
            action = get_button_action();
        } else {
            action = ctxt.handle_input( 17 );
        }

        if( action == "WALK_PATH" ) {
            pimpl->auto_route_from_path();
            break;
        } else if( action == "START_RECORDING" && !pimpl->recording_path() ) {
            pimpl->start_recording();
            break;
        } else if( action == "STOP_RECORDING" && pimpl->recording_path() ) {
            pimpl->stop_recording();
            break;
        } else if( action == "DELETE_PATH" && pimpl->selected_id != -1 ) {
            pimpl->delete_selected();
        } else if( action == "MOVE_PATH_UP" && pimpl->selected_id > 0 ) {
            pimpl->move_selected_up();
        } else if( action == "MOVE_PATH_DOWN" && pimpl->selected_id != -1 &&
                   pimpl->selected_id + 1 < static_cast<int>( pimpl->paths.size() )
                 ) {
            pimpl->move_selected_down();
        } else if( action == "QUIT" ) {
            break;
        }
    }
}

// These need to be here so that pimpl works with unique ptr
path_manager::path_manager() = default;
path_manager::~path_manager() = default;

void path_manager::record_step( const tripoint_abs_ms &new_pos )
{
    if( !pimpl->recording_path() ) {
        return;
    }
    pimpl->paths[pimpl->recording_path_index].record_step( new_pos );
}

bool path_manager::store()
{
    const std::string name = base64_encode( get_avatar().get_save_id() + "_path_manager" );
    cata_path path = PATH_INFO::world_base_save_path() +  "/" + name + ".json";
    const bool iswriten = write_to_file( path, [&]( std::ostream & fout ) {
        serialize( fout );
    }, _( "path_manager data" ) );
    return iswriten;
}

void path_manager::load()
{
    pimpl = std::make_unique<path_manager_impl>();

    const std::string name = base64_encode( get_avatar().get_save_id() + "_path_manager" );
    cata_path path = PATH_INFO::world_base_save_path() / ( name + ".json" );
    if( file_exist( path ) ) {
        read_from_file_json( path, [&]( const JsonValue & jv ) {
            deserialize( jv );
        } );
    }
}

void path_manager::serialize( std::ostream &fout )
{
    JsonOut jsout( fout, true );
    jsout.start_object();
    serialize( jsout );
    jsout.end_object();
}

void path_manager::deserialize( const JsonValue &jsin )
{
    try {
        JsonObject data = jsin.get_object();

        data.read( "recording_path_index", pimpl->recording_path_index );
        // from `path` load only recorded_path
        std::vector<std::vector<tripoint_abs_ms>> recorded_paths;
        data.read( "recorded_paths", recorded_paths );
        for( std::vector<tripoint_abs_ms> &p : recorded_paths ) {
            pimpl->paths.emplace_back( std::move( p ) );
        }
    } catch( const JsonError &e ) {
    }
}

void path_manager::serialize( JsonOut &jsout )
{
    jsout.member( "recording_path_index", pimpl->recording_path_index );
    // from `path` save only recorded_path
    std::vector<std::vector<tripoint_abs_ms>> recorded_paths;
    for( const path &p : pimpl->paths ) {
        recorded_paths.emplace_back( p.recorded_path );
    }
    jsout.member( "recorded_paths", recorded_paths );
}

void path_manager::show()
{
    path_manager_ui( pimpl.get() ).run();
    // todo activity title and progress
    // player_character.assign_activity( workout_activity_actor( player_character.pos() ) );
}
