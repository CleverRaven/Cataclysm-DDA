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
    private:
        bool recording_path = false;
        // todo int or size_t?
        /// Set recording to true/false on current path, current path must be valid
        void set_recording_path( bool set_to );
        /// Set current path to p_index and recording_path to true
        void set_recording_path( int p_index );
        int current_path_index = -1;
        std::vector<path> paths;
};

class path_manager_ui : public cataimgui::window
{
    public:
        explicit path_manager_ui( path_manager_impl *pimpl_in );
        void run();

    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;
    private:
        std::string msg;
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
    current_path_index = paths.size();  // future_size - 1
    paths.emplace_back();
    set_recording_path( current_path_index );
    paths.back().record_step( get_avatar().get_location() );
}

void path_manager_impl::stop_recording()
{
    set_recording_path( false );

    const path &current_path = paths[current_path_index];
    const std::vector<tripoint_abs_ms> &curr_path = current_path.recorded_path;
    if( curr_path.size() <= 1 ) {
        add_msg( m_info, _( "Auto path: Recorded path has no lenght.  Path erased." ) );
        paths.erase( paths.begin() + current_path_index );
    } else {
        add_msg( m_info, _( "Auto path: Path saved." ) );
    }

    current_path_index = -1;
    // TODO error when starts or stops at the same tile as another path ??
    // or just prefer the higher path - this allows
    // more flexibility, but it needs to be documented
}

void path_manager_impl::auto_route_from_path()
{
    avatar &player_character = get_avatar();
    for( int path_index = 0; path_index < static_cast<int>( paths.size() ); ++path_index ) {
        const std::vector<tripoint_abs_ms> &p = paths[path_index].recorded_path;
        if( player_character.pos_bub() == get_map().bub_from_abs( p.front() )
            || player_character.pos_bub() == get_map().bub_from_abs( p.back() )
          ) {
            current_path_index = path_index;
            paths[path_index].set_avatar_path();
            return;
        }
    }
    // Didn't find any, set index to invalid.
    current_path_index = -1;
    add_msg( m_info,
             _( "Auto path: Player doesn't stand at start or end of existing path.  Recording new path." ) );
    start_recording();
}

void path_manager_impl::set_recording_path( bool set_to )
{
    if( set_to ) {
        cata_assert( 0 <= current_path_index && current_path_index < static_cast<int>( paths.size() ) );
    }
    recording_path = set_to;
}

void path_manager_impl::set_recording_path( int p_index )
{
    cata_assert( 0 <= p_index && p_index < static_cast<int>( paths.size() ) );
    current_path_index = p_index;
    recording_path = true;
}

path_manager_ui::path_manager_ui( path_manager_impl *pimpl_in )
    : cataimgui::window( _( "Path Manager" ) ), pimpl( pimpl_in ) {}

cataimgui::bounds path_manager_ui::get_bounds()
{
    return { -1.f, -1.f, float( str_width_to_pixels( TERMX ) ), float( str_height_to_pixels( TERMY ) ) };
}

void path_manager_ui::draw_controls()
{
    if( ! ImGui::BeginTable( "PATH_MANAGER", 4,
                             ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable
                             | ImGuiTableFlags_BordersOuter ) ) {
        return;
    }
    // TODO invlet
    ImGui::TableSetupColumn( "start name" );
    ImGui::TableSetupColumn( "start distance" );
    ImGui::TableSetupColumn( "end name" );
    ImGui::TableSetupColumn( "end distance" );
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin( pimpl->paths.size() );
    int selected_id = -1;
    while( clipper.Step() ) {
        for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
            const path &curr_path = pimpl->paths[i];
            ImGui::TableNextColumn();
            if( ImGui::Selectable( ( "##" + std::to_string( i ) ).c_str(), selected_id == i,
                                   ImGuiSelectableFlags_SpanAllColumns )
              ) {
                //set_selected_id( i );
            }
            ImGui::SameLine();

            ImGui::Text( "%s", "start" );

            ImGui::TableNextColumn();
            std::string dist = direction_suffix( get_avatar().get_location(), curr_path.recorded_path.front() );
            ImGui::Text( "%s", dist == "" ? _( "It's under your feet." ) : dist );

            ImGui::TableNextColumn();
            ImGui::Text( "%s", "end" );

            ImGui::TableNextColumn();
            dist = direction_suffix( get_avatar().get_location(), curr_path.recorded_path.back() );
            ImGui::Text( "%s", dist == "" ? _( "It's under your feet." ) : dist );
        }
    }
    ImGui::EndTable();
}

void path_manager_ui::run()
{
    input_context ctxt( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    std::string action;

    ui_manager::redraw();

    while( is_open ) {
        ui_manager::redraw();
        action = ctxt.handle_input( 17 );
        if( action == "QUIT" ) {
            break;
        }
    }
}

// These need to be here so that pimpl works with unique ptr
path_manager::path_manager() = default;
path_manager::~path_manager() = default;

void path_manager::record_step( const tripoint_abs_ms &new_pos )
{
    if( !pimpl->recording_path ) {
        return;
    }
    pimpl->paths[pimpl->current_path_index].record_step( new_pos );
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

        data.read( "recording_path", pimpl->recording_path );
        data.read( "current_path_index", pimpl->current_path_index );
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
    jsout.member( "recording_path", pimpl->recording_path );
    jsout.member( "current_path_index", pimpl->current_path_index );
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
    // todo move to GUI
    if( pimpl->recording_path ) {
        pimpl->stop_recording();
        return;
    }

    pimpl->auto_route_from_path();

    // todo activity title and progress
    // player_character.assign_activity( workout_activity_actor( player_character.pos() ) );
}
