#include "path_manager.h"

#include <algorithm>
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
#include "color.h"
#include "coordinates.h"
#include "coords_fwd.h"
#include "debug.h"
#include "enums.h"
#include "filesystem.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "input_popup.h"
#include "json.h"
#include "json_error.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "string_formatter.h"
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
        void set_avatar_path() const;
        /**
         * Avatar stands in the middle of the path, so go towards the end or start selected by `towards_end`.
         */
        void set_avatar_path( bool towards_end ) const;
        void set_name_start();
        void set_name_end();
        /**
         * Reverse path and swap start, end names.
         */
        void swap_start_end();
        /**
         * Avatar stands at the start of the `recorded_path`.
         */
        bool is_avatar_at_start() const;
        /**
         * Avatar stands at the end of the `recorded_path`.
         */
        bool is_avatar_at_end() const;
        /**
         * Return the index of the closest tile of `recorded_path` to the avatar.
         *
         * More precise the closer the closest tile is to the avatar.
         * Always correctly finds, if the closest tile is under or next to the avatar.
         */
        int avatar_closest_i_approximate() const;

        void serialize( JsonOut &jsout );
        void deserialize( const JsonObject &jsin );
    private:
        std::vector<tripoint_abs_ms> recorded_path;
        std::string name_start;
        std::string name_end;
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
         * Find the first path the avatar stands at the start or end of. Set it to avatar's auto route.
         * Crash on fail <==> when avatar_at_what_start_or_end returns -1.
         */
        void auto_route_from_path() const;
        /**
         * Let the player choose on what path to walk and in what direction.
         * Set avatars auto route to the selected path.
         *
         * Return true if path was selected, false otherwise.
         */
        bool auto_route_from_path_middle() const;
        /**
         * Start recording a new path.
         */
        void start_recording();
        /**
         * Continue recording the first path the avatar stands at the start or end of.
         */
        void continue_recording();
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
        /**
         * Return the index of the first path the avatar stands at start or end of.
         * -1 if the avatar stands at no start or end.
         */
        int avatar_at_what_start_or_end() const;
        /**
         * Return indexes of paths the avatar stands on.
         */
        std::vector<int> avatar_at_what_paths() const;
    private:
        bool is_recording_path() const;
        /**
         * Set recording_path_index to p_index.
         * -1 means not recording.
         */
        void set_recording_path( int p_index );
        void swap_selected( int index_a, int index_b );
        int recording_path_index = -1;
        // Used when continuing a recording from start
        bool swap_after_recording = false;
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

static std::string avatar_distance_from_tile( const tripoint_abs_ms &tile )
{
    const tripoint_abs_ms &avatar_pos = get_map().getglobal( get_avatar().pos_bub() );
    if( avatar_pos == tile ) {
        return colorize( _( "It's here." ), c_light_green );
    } else {
        return direction_suffix( avatar_pos, tile );
    }
}

void path::record_step( const tripoint_abs_ms &new_pos )
{
    // early return on a huge step, like an elevator teleport
    if( recorded_path.size() >= 1 ) {
        tripoint diff = ( recorded_path.back() - new_pos ).raw().abs();
        if( std::max( { diff.x, diff.y, diff.z } ) > 1 ) {
            popup( _( string_format(
                          "Character moved by %s, expected 1 in each direction at most.  Recording stopped.",
                          diff.to_string_writable() ) ) );
            get_avatar().get_path_manager()->pimpl->stop_recording();
            return;
        }
    }
    // if a loop exists find it and remove it
    // it + 1 so that we don't try to optimize from last tile to last tile (nothing)
    for( auto it = recorded_path.begin(); it + 1 < recorded_path.end(); ) {
        tripoint point_diff = ( *it - new_pos ).raw().abs();
        if( point_diff.z == 0 && ( point_diff.x <= 1 && point_diff.y <= 1 ) ) {
            const size_t old_path_len = recorded_path.size();
            recorded_path.erase( it + 1, recorded_path.end() );
            // special case: character stepped at the first tile so recorded_path = { new_pos }
            if( recorded_path.back() != new_pos ) {
                recorded_path.emplace_back( new_pos );
            }
            add_msg( m_info, _( "Auto path: Cut a corner!  Old path length: %s, new path length: %s." ),
                     old_path_len,
                     recorded_path.size() );
            return;
        }
        // Asuming the path tiles are at most (1, 1, 1) apart,
        // we can skip as many tiles, as the curent tile is far from the `new_pos`.
        int diff = std::max( { point_diff.x, point_diff.y, point_diff.z } );
        // Move 1 less than that for the corner optimization. Move at least 1.
        it += std::max( 1, diff - 1 );
    }
    recorded_path.emplace_back( new_pos );
}

void path::set_avatar_path() const
{
    if( is_avatar_at_start() || is_avatar_at_end() ) {
        set_avatar_path( is_avatar_at_start() );
    } else {
        debugmsg( "Tried to set auto route but the player character doesn't stand at path start or end." );
        return;
    }
}

void path::set_avatar_path( bool towards_end ) const
{
    avatar &player_character = get_avatar();
    std::vector<tripoint_bub_ms> route;
    std::string from;
    if( is_avatar_at_start() ) {
        from = name_start;
    } else if( is_avatar_at_end() ) {
        from = name_end;
    } else {
        from = _( "the middle of path" );
    }

    int avatar_at = avatar_closest_i_approximate();
    if( towards_end ) {
        add_msg( m_info, _( string_format( "Auto path: Go from %s to %s.", from, name_end ) ) );
        for( auto it = std::next( recorded_path.begin() + avatar_at ); it != recorded_path.end(); ++it ) {
            route.emplace_back( get_map().bub_from_abs( *it ) );
        }
    } else {
        add_msg( m_info, _( string_format( "Auto path: Go from %s to %s.", from, name_start ) ) );
        for( auto it = std::next( recorded_path.rbegin() + recorded_path.size() - avatar_at - 1 );
             it != recorded_path.rend(); ++it ) {
            route.emplace_back( get_map().bub_from_abs( *it ) );
        }
    }
    player_character.set_destination( route );
}

void path::set_name_start()
{
    name_start = string_input_popup_imgui( 34, name_start, _( "Name path start:" ) ).query();
}

void path::set_name_end()
{
    name_end = string_input_popup_imgui( 34, name_end, _( "Name path end:" ) ).query();
}

void path::swap_start_end()
{
    std::reverse( recorded_path.begin(), recorded_path.end() );
    std::swap( name_start, name_end );
}

bool path::is_avatar_at_start() const
{
    return get_avatar().pos_bub() == get_map().bub_from_abs( recorded_path.front() );
}

bool path::is_avatar_at_end() const
{
    return get_avatar().pos_bub() == get_map().bub_from_abs( recorded_path.back() );
}

int path::avatar_closest_i_approximate() const
{
    cata_assert( !recorded_path.empty() );
    const tripoint_abs_ms &avatar_pos = get_map().getglobal( get_avatar().pos_bub() );
    // Check start and end so that the path is not further away than either of those.
    int closest_i = recorded_path.size() - 1;
    int closest_dist = square_dist( recorded_path.back(), avatar_pos );
    if( closest_dist == 0 ) {
        return closest_i;
    }
    for( auto it = recorded_path.begin(); it < recorded_path.end(); ) {
        // Asuming the path tiles are at most (1, 1, 1) apart,
        // we can skip as many tiles, as the curent tile is far from the `avatar_pos`.
        int diff = square_dist( *it, avatar_pos );
        // TODO north is closer than northeast && north is closer than z-1
        if( diff < closest_dist ) {
            closest_i = it - recorded_path.begin();
            if( diff == 0 ) {
                return closest_i;
            }
            closest_dist = diff;
        }
        it += diff;
    }
    return closest_i;
    // TODO remove start and end distance?
    /*
    | name        | distance              | length |
    | start | end | start | end | closest |        |
    */
}

void path::serialize( JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "recorded_path", recorded_path );
    jsout.member( "name_start", name_start );
    jsout.member( "name_end", name_end );
    jsout.end_object();
}

void path::deserialize( const JsonObject &jsin )
{
    jsin.read( "recorded_path", recorded_path );
    jsin.read( "name_start", name_start );
    jsin.read( "name_end", name_end );
}

void path_manager_impl::start_recording()
{
    path &p = paths.emplace_back();
    set_recording_path( paths.size() - 1 );
    p.record_step( get_avatar().get_location() );
    p.set_name_start();
}

void path_manager_impl::continue_recording()
{
    int p_index = avatar_at_what_start_or_end();
    cata_assert( p_index != -1 );
    set_recording_path( p_index );
    swap_after_recording = paths[recording_path_index].is_avatar_at_start();
    if( swap_after_recording ) {
        paths[recording_path_index].swap_start_end();
    }
}

void path_manager_impl::stop_recording()
{
    path &current_path = paths[recording_path_index];
    if( current_path.recorded_path.size() <= 1 ) {
        paths.erase( paths.begin() + recording_path_index );
        popup( _( "Recorded path has no length.  Path erased." ) );
        set_recording_path( -1 );
        return;
    }
    if( swap_after_recording ) {
        current_path.swap_start_end();
        current_path.set_name_start();
    } else {
        current_path.set_name_end();
    }
    popup( _( "Path saved." ) );

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

int path_manager_impl::avatar_at_what_start_or_end() const
{
    for( auto it = paths.begin(); it != paths.end(); ++it ) {
        if( it->is_avatar_at_start() || it->is_avatar_at_end() ) {
            return it - paths.begin();
        }
    }
    return -1;
}

std::vector<int> path_manager_impl::avatar_at_what_paths() const
{
    std::vector<int> ret;
    const tripoint_abs_ms &avatar_pos = get_map().getglobal( get_avatar().pos_bub() );
    for( auto it = paths.begin(); it != paths.end(); ++it ) {
        if( avatar_pos == it->recorded_path[it->avatar_closest_i_approximate()] ) {
            ret.emplace_back( static_cast<int>( it - paths.begin() ) );
        }
    }
    return ret;
}

bool path_manager_impl::is_recording_path() const
{
    return recording_path_index != -1;
}

void path_manager_impl::auto_route_from_path() const
{
    int p_index = avatar_at_what_start_or_end();
    cata_assert( p_index != -1 );
    paths[p_index].set_avatar_path();
}

bool path_manager_impl::auto_route_from_path_middle() const
{
    uilist path_selection;
    path_selection.text = _( "Select path and direction to walk in." );
    for( int i : avatar_at_what_paths() ) {
        const path &curr_path = paths[i];
        const std::string from_to = string_format( "from %s (%s) to %s (%s)",
                                    curr_path.name_start,
                                    avatar_distance_from_tile( curr_path.recorded_path.front() ),
                                    curr_path.name_end,
                                    avatar_distance_from_tile( curr_path.recorded_path.back() ) );
        const int avatar_at_i = curr_path.avatar_closest_i_approximate();
        const int start_steps = avatar_at_i;
        const int end_steps = curr_path.recorded_path.size() - avatar_at_i - 1;
        const std::string start = string_format( _( "%s%s (%d steps)" ), start_steps == 0 ? "    " : "",
                                  curr_path.name_start, avatar_at_i );
        const std::string end = string_format( _( "%s%s (%d steps)" ), end_steps == 0 ? "    " : "",
                                               curr_path.name_end, end_steps );
        path_selection.addentry( -1, false, MENU_AUTOASSIGN, from_to );
        path_selection.addentry( i << 1, start_steps > 0, MENU_AUTOASSIGN, start );
        path_selection.addentry( ( i << 1 ) + 1, end_steps > 0, MENU_AUTOASSIGN, end );
    }
    path_selection.query();
    if( path_selection.ret < 0 ) {
        return false;
    }
    paths[path_selection.ret >> 1].set_avatar_path( path_selection.ret % 2 );
    return true;
}

void path_manager_impl::set_recording_path( int p_index )
{
    cata_assert( -1 <= p_index && p_index < static_cast<int>( paths.size() ) );
    if( p_index == -1 ) {
        swap_after_recording = false;
    }
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
    // walk buttons
    cataimgui::draw_colored_text( _( "Walk:" ) );
    ImGui::SameLine();
    enabled_active_button( "WALK_PATH", pimpl->avatar_at_what_start_or_end() != -1 );
    ImGui::SameLine();
    enabled_active_button( "WALK_PATH_FROM_MIDDLE", !pimpl->avatar_at_what_paths().empty() );

    // recording buttons
    cataimgui::draw_colored_text( _( "Recording:" ) );
    ImGui::SameLine();
    enabled_active_button( "START_RECORDING", !pimpl->is_recording_path() );
    ImGui::SameLine();
    enabled_active_button( "CONTINUE_RECORDING", !pimpl->is_recording_path()
                           && pimpl->avatar_at_what_start_or_end() != -1 );
    ImGui::SameLine();
    enabled_active_button( "STOP_RECORDING", pimpl->is_recording_path() );

    // manage buttons
    cataimgui::draw_colored_text( _( "Manage:" ) );
    ImGui::SameLine();
    enabled_active_button( "DELETE_PATH", pimpl->selected_id != -1 );
    ImGui::SameLine();
    enabled_active_button( "MOVE_PATH_UP", pimpl->selected_id > 0 );
    ImGui::SameLine();
    enabled_active_button( "MOVE_PATH_DOWN", pimpl->selected_id != -1 &&
                           pimpl->selected_id + 1 < static_cast<int>( pimpl->paths.size() ) );

    // name buttons
    cataimgui::draw_colored_text( _( "Rename:" ) );
    ImGui::SameLine();
    enabled_active_button( "RENAME_START", pimpl->selected_id != -1 );
    ImGui::SameLine();
    enabled_active_button( "RENAME_END", pimpl->selected_id != -1 );
    ImGui::SameLine();
    enabled_active_button( "SWAP_START_END", pimpl->selected_id != -1 );

    ImGui::BeginChild( "table" );
    if( ! ImGui::BeginTable( "PATH_MANAGER", 6, ImGuiTableFlags_Resizable ) ) {
        return;
    }
    // TODO invlet
    ImGui::TableSetupColumn( _( "start name" ) );
    ImGui::TableSetupColumn( _( "start distance" ) );
    ImGui::TableSetupColumn( _( "end name" ) );
    ImGui::TableSetupColumn( _( "end distance" ) );
    ImGui::TableSetupColumn( _( "closest tile" ) );
    ImGui::TableSetupColumn( _( "total length" ) );
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
            cataimgui::draw_colored_text( curr_path.name_start );

            ImGui::TableNextColumn();
            cataimgui::draw_colored_text( avatar_distance_from_tile( curr_path.recorded_path.front() ) );

            ImGui::TableNextColumn();
            cataimgui::draw_colored_text( curr_path.name_end );

            ImGui::TableNextColumn();
            cataimgui::draw_colored_text( avatar_distance_from_tile( curr_path.recorded_path.back() ) );

            ImGui::TableNextColumn();
            cataimgui::draw_colored_text( avatar_distance_from_tile(
                                              curr_path.recorded_path[curr_path.avatar_closest_i_approximate()] ) );

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
    ctxt.register_action( "WALK_PATH_FROM_MIDDLE" );
    ctxt.register_action( "START_RECORDING" );
    ctxt.register_action( "STOP_RECORDING" );
    ctxt.register_action( "CONTINUE_RECORDING" );
    ctxt.register_action( "DELETE_PATH" );
    ctxt.register_action( "MOVE_PATH_UP" );
    ctxt.register_action( "MOVE_PATH_DOWN" );
    ctxt.register_action( "RENAME_START" );
    ctxt.register_action( "RENAME_END" );
    ctxt.register_action( "SWAP_START_END" );
    std::string action;

    ui_manager::redraw();

    while( is_open ) {
        ui_manager::redraw();
        if( has_button_action() ) {
            action = get_button_action();
        } else {
            action = ctxt.handle_input( 17 );
        }

        if( action == "WALK_PATH" && pimpl->avatar_at_what_start_or_end() != -1 ) {
            pimpl->auto_route_from_path();
            break;
        } else if( action == "WALK_PATH_FROM_MIDDLE" && !pimpl->avatar_at_what_paths().empty() ) {
            if( pimpl->auto_route_from_path_middle() ) {
                break;
            }
        } else if( action == "START_RECORDING" && !pimpl->is_recording_path() ) {
            pimpl->start_recording();
            break;
        } else if( action == "CONTINUE_RECORDING" && !pimpl->is_recording_path()
                   && pimpl->avatar_at_what_start_or_end() != -1 ) {
            pimpl->continue_recording();
        } else if( action == "STOP_RECORDING" && pimpl->is_recording_path() ) {
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
        } else if( action == "RENAME_START" && pimpl->selected_id != -1 ) {
            pimpl->paths[pimpl->selected_id].set_name_start();
        } else if( action == "RENAME_END" && pimpl->selected_id != -1 ) {
            pimpl->paths[pimpl->selected_id].set_name_end();
        } else if( action == "SWAP_START_END" && pimpl->selected_id != -1 ) {
            pimpl->paths[pimpl->selected_id].swap_start_end();
        } else if( action == "QUIT" ) {
            break;
        }
    }
}

// These need to be here so that pimpl works with unique ptr
path_manager::path_manager() : pimpl( std::make_unique<path_manager_impl>() ) {}
path_manager::~path_manager() = default;

void path_manager::record_step( const tripoint_abs_ms &new_pos )
{
    if( !pimpl->is_recording_path() ) {
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
    JsonObject data = jsin.get_object();
    data.read( "recording_path_index", pimpl->recording_path_index );
    data.read( "swap_after_recording", pimpl->swap_after_recording );

    JsonArray data_paths = data.get_array( "paths" );
    for( int i = 0; data_paths.has_object( i ); ++i ) {
        JsonObject obj = data_paths.next_object();
        path p;
        p.deserialize( obj );
        pimpl->paths.emplace_back( p );
    }
}

void path_manager::serialize( JsonOut &jsout )
{
    jsout.member( "recording_path_index", pimpl->recording_path_index );
    jsout.member( "swap_after_recording", pimpl->swap_after_recording );
    jsout.member( "paths" );
    jsout.start_array();
    for( path &p : pimpl->paths ) {
        p.serialize( jsout );
    }
    jsout.end_array();
}

void path_manager::show()
{
    path_manager_ui( pimpl.get() ).run();
    // todo activity title and progress
    // player_character.assign_activity( workout_activity_actor( player_character.pos() ) );
}
