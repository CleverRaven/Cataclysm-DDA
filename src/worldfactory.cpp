#include "worldfactory.h"

#include <cstdio>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "cata_utility.h"
#include "catacharset.h"
#include "char_validity_check.h"
#include "cursesdef.h"
#include "cursesport.h"
#include "debug.h"
#include "enums.h"
#include "filesystem.h"
#include "ime.h"
#include "input.h"
#include "json.h"
#include "mod_manager.h"
#include "name.h"
#include "output.h"
#include "path_info.h"
#include "string_formatter.h"
#include "translations.h"
#include "color.h"
#include "game.h"
#include "string_id.h"

using namespace std::placeholders;

#define dbg(x) DebugLog((x), D_MAIN) << __FILE__ << ":" << __LINE__ << ": "

static const std::string SAVE_MASTER( "master.gsav" );
static const std::string SAVE_EXTENSION( ".sav" );

// single instance of world generator
std::unique_ptr<worldfactory> world_generator;

save_t::save_t( const std::string &name )
    : name( name )
{
}

std::string save_t::player_name() const
{
    return name;
}

std::string save_t::base_path() const
{
    return base64_encode( name );
}

save_t save_t::from_player_name( const std::string &name )
{
    return save_t( name );
}

save_t save_t::from_base_path( const std::string &base_path )
{
    return save_t( base64_decode( base_path ) );
}

static std::string get_next_valid_worldname()
{
    std::string worldname = Name::get( nameIsWorldName );

    return worldname;
}

WORLD::WORLD()
{
    world_name = get_next_valid_worldname();
    WORLD_OPTIONS = get_options().get_world_defaults();

    world_saves.clear();
    active_mod_order = world_generator->get_mod_manager().get_default_mods();
}

void WORLD::COPY_WORLD( const WORLD *world_to_copy )
{
    world_name = world_to_copy->world_name + "_copy";
    WORLD_OPTIONS = world_to_copy->WORLD_OPTIONS;
    active_mod_order = world_to_copy->active_mod_order;
}

std::string WORLD::folder_path() const
{
    return FILENAMES["savedir"] + utf8_to_native( world_name );
}

bool WORLD::save_exists( const save_t &name ) const
{
    return std::find( world_saves.begin(), world_saves.end(), name ) != world_saves.end();
}

void WORLD::add_save( const save_t &name )
{
    if( !save_exists( name ) ) {
        world_saves.push_back( name );
    }
}

worldfactory::worldfactory()
    : active_world( nullptr )
    , mman_ui( *mman )
{
    // prepare tab display order
    tabs.push_back( std::bind( &worldfactory::show_worldgen_tab_modselection, this, _1, _2 ) );
    tabs.push_back( std::bind( &worldfactory::show_worldgen_tab_options, this, _1, _2 ) );
    tabs.push_back( std::bind( &worldfactory::show_worldgen_tab_confirm, this, _1, _2 ) );
}

worldfactory::~worldfactory() = default;

WORLDPTR worldfactory::add_world( std::unique_ptr<WORLD> retworld )
{
    if( !retworld->save() ) {
        return nullptr;
    }
    return ( all_worlds[ retworld->world_name ] = std::move( retworld ) ).get();
}

WORLDPTR worldfactory::make_new_world( const std::vector<mod_id> &mods )
{
    std::unique_ptr<WORLD> retworld = std::make_unique<WORLD>();
    retworld->active_mod_order = mods;
    return add_world( std::move( retworld ) );
}

WORLDPTR worldfactory::make_new_world( bool show_prompt, const std::string &world_to_copy )
{
    // World to return after generating
    std::unique_ptr<WORLD> retworld = std::make_unique<WORLD>();

    if( !world_to_copy.empty() ) {
        retworld->COPY_WORLD( world_generator->get_world( world_to_copy ) );
    }

    if( show_prompt ) {
        // set up window
        catacurses::window wf_win = catacurses::newwin( TERMY, TERMX / 2, point( TERMX / 4, 0 ) );

        int curtab = 0;
        int lasttab = 0; // give placement memory to menus, sorta.
        const size_t numtabs = tabs.size();
        while( static_cast<size_t>( curtab ) < numtabs ) {
            lasttab = curtab;
            draw_worldgen_tabs( wf_win, static_cast<size_t>( curtab ) );
            curtab += tabs[curtab]( wf_win, retworld.get() );

            // If it is -1, or for unsigned size_t, it would be max.
            if( curtab < 0 ) {
                if( !query_yn( _( "Do you want to abort World Generation?" ) ) ) {
                    curtab = lasttab;
                }
            }
        }
        if( curtab < 0 ) {
            handle_redraw();
            return nullptr;
        }
    }

    return add_world( std::move( retworld ) );
}

WORLDPTR worldfactory::make_new_world( special_game_id special_type )
{
    std::string worldname;
    switch( special_type ) {
        case SGAME_TUTORIAL:
            worldname = "TUTORIAL";
            break;
        case SGAME_DEFENSE:
            worldname = "DEFENSE";
            break;
        default:
            return nullptr;
    }

    // Look through all worlds and see if a world named worldname already exists. If so, then just return it instead of
    // making a new world.
    if( has_world( worldname ) ) {
        return all_worlds[worldname].get();
    }

    std::unique_ptr<WORLD> special_world = std::make_unique<WORLD>();
    special_world->world_name = worldname;

    special_world->WORLD_OPTIONS["WORLD_END"].setValue( "delete" );

    if( !special_world->save() ) {
        return nullptr;
    }

    return ( all_worlds[worldname] = std::move( special_world ) ).get();
}

void worldfactory::set_active_world( WORLDPTR world )
{
    world_generator->active_world = world;
    if( world ) {
        get_options().set_world_options( &world->WORLD_OPTIONS );
    } else {
        get_options().set_world_options( nullptr );
    }
}

bool WORLD::save( const bool is_conversion ) const
{
    if( !assure_dir_exist( folder_path() ) ) {
        DebugLog( D_ERROR, DC_ALL ) << "Unable to create or open world[" << world_name <<
                                    "] directory for saving";
        return false;
    }

    if( !is_conversion ) {
        const auto savefile = folder_path() + "/" + FILENAMES["worldoptions"];
        const bool saved = write_to_file( savefile, [&]( std::ostream & fout ) {
            JsonOut jout( fout );

            jout.start_array();

            for( auto &elem : WORLD_OPTIONS ) {
                // Skip hidden option because it is set by mod and should not be saved
                if( !elem.second.getDefaultText().empty() ) {
                    jout.start_object();

                    jout.member( "info", elem.second.getTooltip() );
                    jout.member( "default", elem.second.getDefaultText( false ) );
                    jout.member( "name", elem.first );
                    jout.member( "value", elem.second.getValue( true ) );

                    jout.end_object();
                }
            }

            jout.end_array();
        }, _( "world data" ) );
        if( !saved ) {
            return false;
        }
    }

    world_generator->get_mod_manager().save_mods_list( const_cast<WORLDPTR>( this ) );
    return true;
}

void worldfactory::init()
{
    load_last_world_info();

    std::vector<std::string> qualifiers;
    qualifiers.push_back( FILENAMES["worldoptions"] );
    qualifiers.push_back( FILENAMES["legacy_worldoptions"] );
    qualifiers.push_back( SAVE_MASTER );

    all_worlds.clear();

    // get the master files. These determine the validity of a world
    // worlds exist by having an option file
    // create worlds
    for( const auto &world_dir : get_directories_with( qualifiers, FILENAMES["savedir"], true ) ) {
        // get the save files
        auto world_sav_files = get_files_from_path( SAVE_EXTENSION, world_dir, false );
        // split the save file names between the directory and the extension
        for( auto &world_sav_file : world_sav_files ) {
            size_t save_index = world_sav_file.find( SAVE_EXTENSION );
            world_sav_file = world_sav_file.substr( world_dir.size() + 1,
                                                    save_index - ( world_dir.size() + 1 ) );
        }
        // the directory name is the name of the world
        std::string worldname;
        size_t name_index = world_dir.find_last_of( "/\\" );
        worldname = native_to_utf8( world_dir.substr( name_index + 1 ) );

        // create and store the world
        all_worlds[worldname] = std::make_unique<WORLD>();
        // give the world a name
        all_worlds[worldname]->world_name = worldname;
        // add sav files
        for( auto &world_sav_file : world_sav_files ) {
            all_worlds[worldname]->world_saves.push_back( save_t::from_base_path( world_sav_file ) );
        }
        mman->load_mods_list( all_worlds[worldname].get() );

        // load options into the world
        if( !all_worlds[worldname]->load_options() ) {
            all_worlds[worldname]->WORLD_OPTIONS = get_options().get_world_defaults();
            all_worlds[worldname]->WORLD_OPTIONS["WORLD_END"].setValue( "delete" );
            all_worlds[worldname]->save();
        }
    }

    // check to see if there exists a worldname "save" which denotes that a world exists in the save
    // directory and not in a sub-world directory
    if( has_world( "save" ) ) {
        const WORLD &old_world = *all_worlds["save"];

        std::unique_ptr<WORLD> newworld = std::make_unique<WORLD>();
        newworld->world_name = get_next_valid_worldname();

        // save world as conversion world
        if( newworld->save( true ) ) {
            const std::string origin_path = old_world.folder_path();
            // move files from origin_path into new world path
            for( auto &origin_file : get_files_from_path( ".", origin_path, false ) ) {
                std::string filename = origin_file.substr( origin_file.find_last_of( "/\\" ) );

                rename( origin_file.c_str(), ( newworld->folder_path() + filename ).c_str() );
            }
            newworld->world_saves = old_world.world_saves;
            newworld->WORLD_OPTIONS = old_world.WORLD_OPTIONS;

            all_worlds.erase( "save" );

            all_worlds[newworld->world_name] = std::move( newworld );
        } else {
            debugmsg( "worldfactory::convert_to_world -- World Conversion Failed!" );
        }
    }
}

bool worldfactory::has_world( const std::string &name ) const
{
    return all_worlds.count( name ) > 0;
}

std::vector<std::string> worldfactory::all_worldnames() const
{
    std::vector<std::string> result;
    for( auto &elem : all_worlds ) {
        result.push_back( elem.first );
    }
    return result;
}

WORLDPTR worldfactory::pick_world( bool show_prompt )
{
    std::vector<std::string> world_names = all_worldnames();

    // Filter out special worlds (TUTORIAL | DEFENSE) from world_names.
    for( std::vector<std::string>::iterator it = world_names.begin(); it != world_names.end(); ) {
        if( *it == "TUTORIAL" || *it == "DEFENSE" ) {
            it = world_names.erase( it );
        } else {
            ++it;
        }
    }
    // If there is only one world to pick from, autoreturn it.
    if( world_names.size() == 1 ) {
        return get_world( world_names[0] );
    }
    // If there are no worlds to pick from, immediately try to make one.
    else if( world_names.empty() ) {
        return make_new_world( show_prompt );
    }
    // If we're skipping prompts, return the world with 0 save if there is one
    else if( !show_prompt ) {
        for( const auto &name : world_names ) {
            if( get_world( name )->world_saves.empty() ) {
                return get_world( name );
            }
        }
        // if there isn't any, adhere to old logic: return the alphabetically first one
        return get_world( world_names[0] );
    }

    const int iTooltipHeight = 3;
    const int iContentHeight = TERMY - 3 - iTooltipHeight;
    const size_t num_pages = world_names.size() / iContentHeight + 1; // at least 1 page

    std::map<int, bool> mapLines;
    mapLines[3] = true;

    std::map<int, std::vector<std::string> > world_pages;
    unsigned int worldnum = 0;
    for( unsigned int i = 0; i < num_pages; ++i ) {
        for( int j = 0; j < iContentHeight && worldnum < world_names.size(); ++j ) {
            world_pages[i].push_back( world_names[ worldnum++ ] );
        }
    }
    size_t sel = 0, selpage = 0;

    catacurses::window w_worlds_border = catacurses::newwin( TERMY, TERMX / 2, point( TERMX / 4, 0 ) );
    catacurses::window w_worlds_tooltip = catacurses::newwin( iTooltipHeight, TERMX / 2 - 2,
                                          point( 1 + TERMX / 4, 1 ) );
    catacurses::window w_worlds_header = catacurses::newwin( 1, TERMX / 2 - 2,
                                         point( 1 + TERMX / 4, 1 + iTooltipHeight ) );
    catacurses::window w_worlds = catacurses::newwin( iContentHeight, TERMX / 2 - 2,
                                  point( 1 + TERMX / 4, iTooltipHeight + 2 ) );

    draw_border( w_worlds_border, BORDER_COLOR, _( " WORLD SELECTION " ) );
    mvwputch( w_worlds_border, point( 0, 4 ), BORDER_COLOR, LINE_XXXO ); // |-
    mvwputch( w_worlds_border, point( TERMX / 2 - 1, 4 ), BORDER_COLOR, LINE_XOXX ); // -|

    for( auto &mapLine : mapLines ) {
        mvwputch( w_worlds_border, point( mapLine.first + 1, TERMY - 1 ), BORDER_COLOR,
                  LINE_XXOX ); // _|_
    }

    wrefresh( w_worlds_border );

    for( int i = 0; i < getmaxx( w_worlds_border ); i++ ) {
        if( mapLines[i] ) {
            mvwputch( w_worlds_header, point( i, 0 ), BORDER_COLOR, LINE_OXXX );
        } else {
            mvwputch( w_worlds_header, point( i, 0 ), BORDER_COLOR, LINE_OXOX ); // Draw header line
        }
    }

    wrefresh( w_worlds_header );

    input_context ctxt( "PICK_WORLD_DIALOG" );
    ctxt.register_updown();
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );

    std::ostringstream sTemp;

    while( true ) {
        //Clear the lines
        for( int i = 0; i < iContentHeight; i++ ) {
            for( int j = 0; j < getmaxx( w_worlds ); j++ ) {
                if( mapLines[j] ) {
                    mvwputch( w_worlds, point( j, i ), BORDER_COLOR, LINE_XOXO );
                } else {
                    mvwputch( w_worlds, point( j, i ), c_black, ' ' );
                }

                if( i < iTooltipHeight ) {
                    mvwputch( w_worlds_tooltip, point( j, i ), c_black, ' ' );
                }
            }
        }

        //Draw World Names
        for( size_t i = 0; i < world_pages[selpage].size(); ++i ) {
            sTemp.str( "" );
            sTemp << i + 1;
            mvwprintz( w_worlds, point( 0, static_cast<int>( i ) ), c_white, sTemp.str() );
            wmove( w_worlds, point( 4, static_cast<int>( i ) ) );

            std::string world_name = ( world_pages[selpage] )[i];
            size_t saves_num = get_world( world_name )->world_saves.size();

            if( i == sel ) {
                wprintz( w_worlds, c_yellow, ">> " );
            } else {
                wprintz( w_worlds, c_yellow, "   " );
            }

            wprintz( w_worlds, c_white, "%s (%lu)", world_name, saves_num );
        }

        //Draw Tabs
        wmove( w_worlds_header, point( 7, 0 ) );

        for( size_t i = 0; i < num_pages; ++i ) {
            if( !world_pages[i].empty() ) { //skip empty pages
                nc_color tabcolor = ( selpage == i ) ? hilite( c_white ) : c_white;
                wprintz( w_worlds_header, c_white, "[" );
                wprintz( w_worlds_header, tabcolor, _( "Page %lu" ), i + 1 ) ;
                wprintz( w_worlds_header, c_white, "]" );
                wputch( w_worlds_header, BORDER_COLOR, LINE_OXOX );
            }
        }

        wrefresh( w_worlds_header );

        fold_and_print( w_worlds_tooltip, point_zero, 78, c_white, _( "Pick a world to enter game" ) );
        wrefresh( w_worlds_tooltip );

        wrefresh( w_worlds );

        const std::string action = ctxt.handle_input();

        if( action == "QUIT" ) {
            break;
        } else if( !world_pages[selpage].empty() && action == "DOWN" ) {
            sel++;
            if( sel >= world_pages[selpage].size() ) {
                sel = 0;
            }
        } else if( !world_pages[selpage].empty() && action == "UP" ) {
            if( sel == 0 ) {
                sel = world_pages[selpage].size() - 1;
            } else {
                sel--;
            }
        } else if( action == "NEXT_TAB" ) {
            sel = 0;
            do { //skip empty pages
                selpage++;
                if( selpage >= world_pages.size() ) {
                    selpage = 0;
                }
            } while( world_pages[selpage].empty() );
        } else if( action == "PREV_TAB" ) {
            sel = 0;
            do { //skip empty pages
                if( selpage != 0 ) {
                    selpage--;
                } else {
                    selpage = world_pages.size() - 1;
                }
            } while( world_pages[selpage].empty() );
        } else if( action == "CONFIRM" ) {
            werase( w_worlds );
            werase( w_worlds_border );
            werase( w_worlds_header );
            werase( w_worlds_tooltip );
            return get_world( world_pages[selpage][sel] );
        }
    }

    werase( w_worlds );
    werase( w_worlds_border );
    werase( w_worlds_header );
    werase( w_worlds_tooltip );

    return nullptr;
}

void worldfactory::remove_world( const std::string &worldname )
{
    auto it = all_worlds.find( worldname );
    if( it != all_worlds.end() ) {
        WORLDPTR wptr = it->second.get();
        if( active_world == wptr ) {
            get_options().set_world_options( nullptr );
            active_world = nullptr;
        }
        all_worlds.erase( it );
    }
}

void worldfactory::load_last_world_info()
{
    std::ifstream file( FILENAMES["lastworld"], std::ifstream::in | std::ifstream::binary );
    if( !file.good() ) {
        return;
    }

    JsonIn jsin( file );
    JsonObject data = jsin.get_object();
    last_world_name = data.get_string( "world_name" );
    last_character_name = data.get_string( "character_name" );
}

void worldfactory::save_last_world_info()
{
    write_to_file( FILENAMES["lastworld"], [&]( std::ostream & file ) {
        JsonOut jsout( file, true );
        jsout.start_object();
        jsout.member( "world_name", last_world_name );
        jsout.member( "character_name", last_character_name );
        jsout.end_object();
    }, _( "last world info" ) );
}

std::string worldfactory::pick_random_name()
{
    // TODO: add some random worldname parameters to name generator
    return get_next_valid_worldname();
}

int worldfactory::show_worldgen_tab_options( const catacurses::window &/*win*/, WORLDPTR world )
{
    get_options().set_world_options( &world->WORLD_OPTIONS );
    const std::string action = get_options().show( false, true );
    get_options().set_world_options( nullptr );
    if( action == "PREV_TAB" ) {
        return -1;

    } else if( action == "NEXT_TAB" ) {
        return 1;

    } else if( action == "QUIT" ) {
        return -999;
    }

    return 0;
}

void worldfactory::draw_mod_list( const catacurses::window &w, int &start, size_t cursor,
                                  const std::vector<mod_id> &mods, bool is_active_list,
                                  const std::string &text_if_empty, const catacurses::window &w_shift )
{
    werase( w );
    werase( w_shift );

    const int iMaxRows = getmaxy( w );
    size_t iModNum = mods.size();
    size_t iActive = cursor;
    bool first_line_is_category = false;

    if( mods.empty() ) {
        center_print( w, 0, c_red, text_if_empty );
    } else {
        int iCatSortNum = 0;
        std::string sLastCategoryName;
        std::map<int, std::string> mSortCategory;
        mSortCategory[0] = sLastCategoryName;

        for( size_t i = 0; i < mods.size(); ++i ) {
            const std::string category_name = _( mods[i]->category.second );
            if( sLastCategoryName != category_name ) {
                sLastCategoryName = category_name;
                mSortCategory[ i + iCatSortNum++ ] = sLastCategoryName;
                iModNum++;
                if( i == 0 ) {
                    first_line_is_category = true;
                }
            }
        }

        const int wwidth = getmaxx( w ) - 1 - 3; // border (1) + ">> " (3)

        unsigned int iNum = 0;
        int index = 0;
        bool bKeepIter = false;
        int iCatBeforeCursor = 0;

        for( size_t i = 0; i <= iActive; i++ ) {
            if( !mSortCategory[i].empty() ) {
                iActive++;
                iCatBeforeCursor++;
            }
        }

        calcStartPos( start, iActive, iMaxRows, iModNum );

        for( int i = 0; i < start; i++ ) {
            if( !mSortCategory[i].empty() ) {
                iNum++;
            }
        }

        int larger = ( iMaxRows > static_cast<int>( iModNum ) ) ? static_cast<int>( iModNum ) : iMaxRows;
        for( auto iter = mods.begin(); iter != mods.end(); ++index ) {
            if( iNum >= static_cast<size_t>( start ) && iNum < static_cast<size_t>( start + larger ) ) {
                if( !mSortCategory[iNum].empty() ) {
                    bKeepIter = true;
                    trim_and_print( w, point( 1, iNum - start ), wwidth, c_magenta, mSortCategory[iNum] );

                } else {
                    if( iNum == iActive ) {
                        //mvwprintw( w, iNum - start + iCatSortOffset, 1, "   " );
                        if( is_active_list ) {
                            mvwprintz( w, point( 1, iNum - start ), c_yellow, ">> " );
                        } else {
                            mvwprintz( w, point( 1, iNum - start ), c_blue, ">> " );
                        }
                    }

                    const MOD_INFORMATION &mod = **iter;
                    trim_and_print( w, point( 4, iNum - start ), wwidth, c_white, mod.name() );

                    if( w_shift ) {
                        // get shift information for the active item
                        std::string shift_display;
                        const size_t iPos = std::distance( mods.begin(), iter );

                        if( mman_ui->can_shift_up( iPos, mods ) ) {
                            shift_display += "<color_blue>+</color> ";
                        } else {
                            shift_display += "<color_dark_gray>+</color> ";
                        }

                        if( mman_ui->can_shift_down( iPos, mods ) ) {
                            shift_display += "<color_blue>-</color>";
                        } else {
                            shift_display += "<color_dark_gray>-</color>";
                        }

                        trim_and_print( w_shift, point( 1, 2 + iNum - start ), 3, c_white, shift_display );
                    }
                }
            }

            if( bKeepIter ) {
                bKeepIter = false;
            } else {
                ++iter;
            }

            iNum++;
        }
    }

    if( first_line_is_category && iActive == 1 ) {  // Ensure that the scrollbar starts at zero position
        draw_scrollbar( w, 0, iMaxRows, static_cast<int>( iModNum ), point_zero );
    } else {
        draw_scrollbar( w, static_cast<int>( iActive ), iMaxRows, static_cast<int>( iModNum ), point_zero );
    }

    wrefresh( w );
    wrefresh( w_shift );
}

void worldfactory::show_active_world_mods( const std::vector<mod_id> &world_mods )
{
    catacurses::window w_border = catacurses::newwin( TERMY - 11, TERMX / 4 - 3, point( TERMX / 4, 4 ) );
    catacurses::window w_mods = catacurses::newwin( TERMY - 13, TERMX / 4 - 4, point( TERMX / 4, 5 ) );

    int start = 0;
    int cursor = 0;
    const size_t num_mods = world_mods.size();

    draw_border( w_border, BORDER_COLOR, _( " ACTIVE WORLD MODS " ) );
    wrefresh( w_border );

    while( true ) {
        draw_mod_list( w_mods, start, static_cast<size_t>( cursor ), world_mods,
                       true, _( "--NO ACTIVE MODS--" ), catacurses::window() );
        wrefresh( w_mods );

        input_context ctxt( "DEFAULT" );
        ctxt.register_updown();
        ctxt.register_action( "QUIT" );
        ctxt.register_action( "CONFIRM" );
        const std::string action = ctxt.handle_input();

        if( action == "UP" ) {
            cursor--;
            // If it went under 0, loop back to the end of the list.
            if( cursor < 0 ) {
                cursor = static_cast<int>( num_mods - 1 );
            }

        } else if( action == "DOWN" ) {
            cursor++;
            // If it went over the end of the list, loop back to the start of the list.
            if( cursor > static_cast<int>( num_mods - 1 ) ) {
                cursor = 0;
            }

        } else if( action == "QUIT" || action == "CONFIRM" ) {
            handle_redraw();
            break;
        }
    }
}

int worldfactory::show_worldgen_tab_modselection( const catacurses::window &win, WORLDPTR world )
{
    // Use active_mod_order of the world,
    // saves us from writing 'world->active_mod_order' all the time.
    std::vector<mod_id> &active_mod_order = world->active_mod_order;
    {
        std::vector<mod_id> tmp_mod_order;
        // clear active_mod_order and re-add all the mods, his ensures
        // that changes (like changing dependencies) get updated
        tmp_mod_order.swap( active_mod_order );
        for( auto &elem : tmp_mod_order ) {
            mman_ui->try_add( elem, active_mod_order );
        }
    }

    input_context ctxt( "MODMANAGER_DIALOG" );
    ctxt.register_updown();
    ctxt.register_action( "LEFT", translate_marker( "Switch to other list" ) );
    ctxt.register_action( "RIGHT", translate_marker( "Switch to other list" ) );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "NEXT_CATEGORY_TAB" );
    ctxt.register_action( "PREV_CATEGORY_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM", translate_marker( "Activate / deactivate mod" ) );
    ctxt.register_action( "ADD_MOD" );
    ctxt.register_action( "REMOVE_MOD" );
    ctxt.register_action( "SAVE_DEFAULT_MODS" );
    ctxt.register_action( "VIEW_MOD_DESCRIPTION" );

    // lots of small windows so that each section can be drawn to independently of the others as necessary
    catacurses::window w_header1 = catacurses::newwin( 1, TERMX / 4 - 5, point( 1 + TERMX / 4, 3 ) );
    catacurses::window w_header2 = catacurses::newwin( 1, TERMX / 4 - 4,
                                   point( TERMX / 4 + 3 + TERMX / 4, 3 ) );
    catacurses::window w_shift   = catacurses::newwin( TERMY - 11, 5, point( TERMX / 4 - 3 + TERMX / 4,
                                   3 ) );
    catacurses::window w_list    = catacurses::newwin( TERMY - 13, TERMX / 4 - 4, point( TERMX / 4,
                                   5 ) );
    catacurses::window w_active  = catacurses::newwin( TERMY - 13, TERMX / 4 - 4,
                                   point( TERMX / 4 + 2 + TERMX / 4, 5 ) );
    catacurses::window w_description = catacurses::newwin( 4, TERMX / 2 - 4, point( 1 + TERMX / 4,
                                       TERMY - 5 ) );


    draw_modselection_borders( win, ctxt );
    std::vector<std::string> headers;
    headers.push_back( _( "Mod List" ) );
    headers.push_back( _( "Mod Load Order" ) );
    std::vector<catacurses::window> header_windows;
    header_windows.push_back( w_header1 );
    header_windows.push_back( w_header2 );

    int tab_output = 0;
    size_t last_active_header = 0;
    size_t active_header = 0;
    size_t useable_mod_count = mman->get_usable_mods().size();
    int startsel[2] = {0, 0};
    size_t cursel[2] = {0, 0};
    size_t iCurrentTab = 0;
    std::vector<mod_id> current_tab_mods;

    bool redraw_headers = true;
    bool redraw_description = true;
    bool redraw_list = true;
    bool redraw_active = true;
    bool selection_changed = false;
    bool recalc_tabs = true;

    // Helper function for determining the currently selected mod
    const auto get_selected_mod = [&]() -> const MOD_INFORMATION* {
        if( current_tab_mods.empty() )
        {
            return nullptr;
        } else if( active_header == 0 )
        {
            return &current_tab_mods[cursel[0]].obj();
        } else if( !active_mod_order.empty() )
        {
            return &active_mod_order[cursel[1]].obj();
        }
        return nullptr;
    };

    // Helper function to trigger full redraw on mod selection screen
    const auto redraw_all = [&]() {
        redraw_headers = true;
        redraw_list = true;
        redraw_active = true;
        redraw_description = true;
        draw_worldgen_tabs( win, 0 );
        draw_modselection_borders( win, ctxt );
    };

    while( tab_output == 0 ) {
        if( redraw_headers ) {
            for( size_t i = 0; i < headers.size(); ++i ) {
                werase( header_windows[i] );
                const int header_x = ( getmaxx( header_windows[i] ) - utf8_width( headers[i] ) ) / 2;
                mvwprintz( header_windows[i], point( header_x, 0 ), c_cyan, headers[i] );

                if( active_header == i ) {
                    mvwputch( header_windows[i], point( header_x - 3, 0 ), c_red, '<' );
                    mvwputch( header_windows[i], point( header_x + utf8_width( headers[i] ) + 2, 0 ),
                              c_red, '>' );
                }
                wrefresh( header_windows[i] );
            }
            redraw_list = true;
            redraw_active = true;
            redraw_headers = false;
        }

        if( recalc_tabs ) {
            current_tab_mods.clear();

            for( const auto &item : mman->get_usable_mods() ) {
                const auto &iter = get_mod_list_cat_tab().find(
                                       get_mod_list_categories()[item->category.first].first );

                std::string sCatTab = "tab_default";
                if( iter != get_mod_list_cat_tab().end() ) {
                    sCatTab = _( iter->second );
                }

                if( sCatTab == get_mod_list_tabs()[iCurrentTab].first ) {
                    current_tab_mods.push_back( item );
                }

                useable_mod_count = current_tab_mods.size();
            }

            recalc_tabs = false;
        }

        if( selection_changed ) {
            if( active_header == 0 ) {
                redraw_list = true;
            }
            if( active_header == 1 ) {
                redraw_active = true;
            }
            selection_changed = false;
            redraw_description = true;
        }

        if( redraw_description ) {
            werase( w_description );

            if( const MOD_INFORMATION *selmod = get_selected_mod() ) {
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                int num_lines = fold_and_print( w_description, point( 1, 0 ),
                                                getmaxx( w_description ) - 1,
                                                c_white, mman_ui->get_information( selmod ) );
                auto window_height = catacurses::getmaxy( w_description );
                auto window_width = catacurses::getmaxx( w_description );
                if( num_lines > window_height ) {
                    // The description didn't fit in the window, so provide a
                    // hint for how to see the whole thing
                    std::string message = string_format( _( "…%s = View full description " ),
                                                         ctxt.get_desc( "VIEW_MOD_DESCRIPTION" ) );
                    nc_color color = c_green;
                    print_colored_text( w_description, point( window_width - utf8_width( message ), window_height - 1 ),
                                        color, color, message );
                }
            }

            //redraw tabs
            wmove( win, point( 2, 4 ) );
            for( size_t i = 0; i < get_mod_list_tabs().size(); i++ ) {
                wprintz( win, c_white, "[" );
                wprintz( win, ( iCurrentTab == i ) ? hilite( c_light_green ) : c_light_green,
                         _( get_mod_list_tabs()[i].second ) );
                wprintz( win, c_white, "]" );
                wputch( win, BORDER_COLOR, LINE_OXOX );
            }

            redraw_description = false;
            wrefresh( w_description );
            wrefresh( win );
        }

        if( redraw_list ) {
            draw_mod_list( w_list, startsel[0], cursel[0], current_tab_mods, active_header == 0,
                           _( "--NO AVAILABLE MODS--" ), catacurses::window() );
        }
        if( redraw_active ) {
            draw_mod_list( w_active, startsel[1], cursel[1], active_mod_order, active_header == 1,
                           _( "--NO ACTIVE MODS--" ), w_shift );
        }
        catacurses::refresh();

        last_active_header = active_header;
        const int next_header = ( active_header == 1 ) ? 0 : 1;
        const int prev_header = ( active_header == 0 ) ? 1 : 0;

        size_t selection = ( active_header == 0 ) ? cursel[0] : cursel[1];
        size_t last_selection = selection;
        size_t next_selection = selection + 1;
        size_t prev_selection = selection - 1;
        if( active_header == 0 ) {
            next_selection = ( next_selection >= useable_mod_count ) ? 0 : next_selection;
            prev_selection = ( prev_selection > useable_mod_count ) ? useable_mod_count - 1 : prev_selection;
        } else {
            next_selection = ( next_selection >= active_mod_order.size() ) ? 0 : next_selection;
            prev_selection = ( prev_selection > active_mod_order.size() ) ? active_mod_order.size() - 1 :
                             prev_selection;
        }

        const std::string action = ctxt.handle_input();

        if( action == "DOWN" ) {
            selection = next_selection;
        } else if( action == "UP" ) {
            selection = prev_selection;
        } else if( action == "RIGHT" ) {
            active_header = next_header;
        } else if( action == "LEFT" ) {
            active_header = prev_header;
        } else if( action == "CONFIRM" ) {
            if( active_header == 0 && !current_tab_mods.empty() ) {
                // try-add
                mman_ui->try_add( current_tab_mods[cursel[0]], active_mod_order );
                redraw_active = true;
            } else if( active_header == 1 && !active_mod_order.empty() ) {
                // try-rem
                mman_ui->try_rem( cursel[1], active_mod_order );
                redraw_active = true;
                redraw_description = true;
                if( active_mod_order.empty() ) {
                    // switch back to other list, we can't change
                    // anything in the empty active mods list.
                    active_header = 0;
                }
            }
        } else if( action == "ADD_MOD" ) {
            if( active_header == 1 && active_mod_order.size() > 1 ) {
                mman_ui->try_shift( '+', cursel[1], active_mod_order );
                redraw_active = true;
            }
        } else if( action == "REMOVE_MOD" ) {
            if( active_header == 1 && active_mod_order.size() > 1 ) {
                mman_ui->try_shift( '-', cursel[1], active_mod_order );
                redraw_active = true;
                redraw_description = true;
            }
        } else if( action == "NEXT_CATEGORY_TAB" ) {
            if( active_header == 0 ) {
                if( ++iCurrentTab >= get_mod_list_tabs().size() ) {
                    iCurrentTab = 0;
                }

                startsel[0] = 0;
                cursel[0] = 0;

                recalc_tabs = true;
                redraw_description = true;
            }

        } else if( action == "PREV_CATEGORY_TAB" ) {
            if( active_header == 0 ) {
                if( --iCurrentTab > get_mod_list_tabs().size() ) {
                    iCurrentTab = get_mod_list_tabs().size() - 1;
                }

                startsel[0] = 0;
                cursel[0] = 0;

                recalc_tabs = true;
                redraw_description = true;
            }
        } else if( action == "NEXT_TAB" ) {
            tab_output = 1;
        } else if( action == "PREV_TAB" ) {
            tab_output = -1;
        } else if( action == "SAVE_DEFAULT_MODS" ) {
            if( mman->set_default_mods( active_mod_order ) ) {
                popup( _( "Saved list of active mods as default" ) );
                draw_modselection_borders( win, ctxt );
                redraw_description = true;
                redraw_headers = true;
            }
        } else if( action == "VIEW_MOD_DESCRIPTION" ) {
            if( const MOD_INFORMATION *selmod = get_selected_mod() ) {
                popup( "%s", mman_ui->get_information( selmod ) );
                redraw_all();
            }
        } else if( action == "HELP_KEYBINDINGS" ) {
            redraw_all();
        } else if( action == "QUIT" ) {
            tab_output = -999;
        }
        // RESOLVE INPUTS
        if( last_active_header != active_header ) {
            redraw_headers = true;
            redraw_description = true;
        }
        if( last_selection != selection ) {
            if( active_header == 0 ) {
                redraw_list = true;
                cursel[0] = selection;
            } else {
                redraw_active = true;
                cursel[1] = selection;
            }
            redraw_description = true;
        }
        if( active_mod_order.empty() ) {
            redraw_active = true;
            cursel[1] = 0;
        }

        if( active_header == 1 ) {
            if( active_mod_order.empty() ) {
                cursel[1] = 0;
            } else {
                // If it goes below 0, it'll loop back to max (or at least, greater than AMO size*10.
                if( cursel[1] > active_mod_order.size() * 10 ) {
                    cursel[1] = 0;
                }
                // If it goes above AMO.size(), cap to size.
                else if( cursel[1] >= active_mod_order.size() ) {
                    cursel[1] = active_mod_order.size() - 1;
                }
            }
        }
        // end RESOLVE INPUTS
    }
    return tab_output;
}

int worldfactory::show_worldgen_tab_confirm( const catacurses::window &win, WORLDPTR world )
{
    const int iTooltipHeight = 1;
    const int iContentHeight = TERMY - 3 - iTooltipHeight;

    const char *line_of_32_underscores = "________________________________";

    catacurses::window w_confirmation = catacurses::newwin( iContentHeight, TERMX / 2 - 2,
                                        point( 1 + TERMX / 4, iTooltipHeight + 2 ) );

    int namebar_y = 1;
    int namebar_x = 3 + utf8_width( _( "World Name:" ) );

    bool noname = false;
    input_context ctxt( "WORLDGEN_CONFIRM_DIALOG" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "PICK_RANDOM_WORLDNAME" );

    std::string worldname = world->world_name;

    // do not switch IME mode now, but restore previous mode on return
    ime_sentry sentry( ime_sentry::keep );
    do {
        mvwprintz( w_confirmation, point( 2, namebar_y ), c_white, _( "World Name:" ) );
        mvwprintz( w_confirmation, point( namebar_x, namebar_y ), c_light_gray, line_of_32_underscores );
        fold_and_print( w_confirmation, point( 2, 3 ), 76, c_light_gray,
                        _( "Press <color_yellow>%s</color> to pick a random name for your world." ),
                        ctxt.get_desc( "PICK_RANDOM_WORLDNAME" ) );
        fold_and_print( w_confirmation, point( 2, TERMY / 2 - 2 ), 76, c_light_gray,
                        _( "Press <color_yellow>%s</color> when you are satisfied with the world as it is and are ready "
                           "to continue, or <color_yellow>%s</color> to go back and review your world." ),
                        ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "PREV_TAB" ) );
        if( !noname ) {
            mvwprintz( w_confirmation, point( namebar_x, namebar_y ), c_light_gray, worldname );
            wprintz( w_confirmation, h_light_gray, "_" );
        }
        if( noname ) {
            mvwprintz( w_confirmation, point( namebar_x, namebar_y ), c_light_gray, line_of_32_underscores );
            noname = false;
        }

        wrefresh( win );
        wrefresh( w_confirmation );
        catacurses::refresh();

        const std::string action = ctxt.handle_input();
        if( action == "NEXT_TAB" ) {
            if( worldname.empty() ) {
                mvwprintz( w_confirmation, point( namebar_x, namebar_y ), h_light_gray,
                           _( "________NO NAME ENTERED!________" ) );
                noname = true;
                wrefresh( w_confirmation );
                if( !query_yn( _( "Are you SURE you're finished?  World name will be randomly generated." ) ) ) {
                    werase( w_confirmation );
                    continue;
                } else {
                    world->world_name = pick_random_name();
                    if( !valid_worldname( world->world_name ) ) {
                        continue;
                    }
                    return 1;
                }
            } else if( query_yn( _( "Are you SURE you're finished?" ) ) ) {
                // erase entire window to avoid overlapping of query with possible popup about invalid worldname
                werase( w_confirmation );
                wrefresh( w_confirmation );
                catacurses::refresh();

                if( valid_worldname( worldname ) ) {
                    world->world_name = worldname;
                    return 1;
                } else {
                    continue;
                }
            } else {
                werase( w_confirmation );
                continue;
            }
        } else if( action == "PREV_TAB" ) {
            world->world_name = worldname;
            return -1;
        } else if( action == "PICK_RANDOM_WORLDNAME" ) {
            mvwprintz( w_confirmation, point( namebar_x, namebar_y ), c_light_gray, line_of_32_underscores );
            world->world_name = worldname = pick_random_name();
        } else if( action == "QUIT" ) {
            // Cache the current name just in case they say No to the exit query.
            world->world_name = worldname;
            return -999;
        } else if( action == "ANY_INPUT" ) {
            const input_event ev = ctxt.get_raw_input();
            const int ch = ev.get_first_input();
            utf8_wrapper wrap( worldname );
            utf8_wrapper newtext( ev.text );
            if( ch == KEY_BACKSPACE ) {
                if( !wrap.empty() ) {
                    wrap.erase( wrap.length() - 1, 1 );
                    worldname = wrap.str();
                }
            } else if( ch == KEY_F( 2 ) ) {
                std::string tmp = get_input_string_from_file();
                int tmplen = utf8_width( tmp );
                if( tmplen > 0 && tmplen + utf8_width( worldname ) < 30 ) {
                    worldname.append( tmp );
                }
            } else if( !newtext.empty() && is_char_allowed( newtext.at( 0 ) ) ) {
                // No empty string, no slash, no backslash, no control sequence
                wrap.append( newtext );
                worldname = wrap.str();
            }
            mvwprintz( w_confirmation, point( namebar_x, namebar_y ), c_light_gray, line_of_32_underscores );
            mvwprintz( w_confirmation, point( namebar_x, namebar_y ), c_light_gray, worldname );
            wprintz( w_confirmation, h_light_gray, "_" );
        }
    } while( true );

    return 0;
}

void worldfactory::draw_modselection_borders( const catacurses::window &win,
        const input_context &ctxtp )
{
    // make appropriate lines: X & Y coordinate of starting point, length, horizontal/vertical type
    std::array<int, 5> xs = {{1, 1, TERMX / 4 + 2, TERMX / 4 - 4, TERMX / 4 + 2}};
    std::array<int, 5> ys = {{TERMY - 8, 4, 4, 3, 3}};
    std::array<int, 5> ls = {{TERMX / 2 - 2, TERMX / 4 - 4, TERMX / 4 - 2, TERMY - 11, 1}};
    std::array<bool, 5> hv = {{true, true, true, false, false}}; // horizontal line = true, vertical line = false

    for( int i = 0; i < 5; ++i ) {
        int x = xs[i];
        int y = ys[i];
        int l = ls[i];
        if( hv[i] ) {
            for( int j = 0; j < l; ++j ) {
                mvwputch( win, point( x + j, y ), BORDER_COLOR, LINE_OXOX ); // -
            }
        } else {
            for( int j = 0; j < l; ++j ) {
                mvwputch( win, point( x, y + j ), BORDER_COLOR, LINE_XOXO ); // |
            }
        }
    }

    // Add in connective characters
    mvwputch( win, point( 0, 4 ), BORDER_COLOR, LINE_XXXO );
    mvwputch( win, point( 0, TERMY - 8 ), BORDER_COLOR, LINE_XXXO );
    mvwputch( win, point( TERMX / 4 + 2, 4 ), BORDER_COLOR, LINE_XXXO );

    mvwputch( win, point( TERMX / 2 - 1, 4 ), BORDER_COLOR, LINE_XOXX );
    mvwputch( win, point( TERMX / 2 - 1, TERMY - 8 ), BORDER_COLOR, LINE_XOXX );
    mvwputch( win, point( TERMX / 4 - 4, 4 ), BORDER_COLOR, LINE_XOXX );

    mvwputch( win, point( TERMX / 4 - 4, 2 ), BORDER_COLOR, LINE_OXXX ); // -.-
    mvwputch( win, point( TERMX / 4 + 2, 2 ), BORDER_COLOR, LINE_OXXX ); // -.-

    mvwputch( win, point( TERMX / 4 - 4, TERMY - 8 ), BORDER_COLOR,
              LINE_XXOX ); // _|_
    mvwputch( win, point( TERMX / 4 + 2, TERMY - 8 ), BORDER_COLOR,
              LINE_XXOX ); // _|_

    // Add tips & hints
    fold_and_print( win, point( 2, TERMY - 7 ), getmaxx( win ) - 4, c_green,
                    _( "%s = Save Load Order as default.  %s = Controls %s/%s = Prev/Next Option.  %s/%s = Prev/Next Tab." ),
                    ctxtp.get_desc( "SAVE_DEFAULT_MODS" ),
                    ctxtp.get_desc( "HELP_KEYBINDINGS" ),
                    ctxtp.get_desc( "PREV_CATEGORY_TAB" ),
                    ctxtp.get_desc( "NEXT_CATEGORY_TAB" ),
                    ctxtp.get_desc( "PREV_TAB" ),
                    ctxtp.get_desc( "NEXT_TAB" )
                  );
    wrefresh( win );
    catacurses::refresh();
}

void worldfactory::draw_worldgen_tabs( const catacurses::window &w, size_t current )
{
    werase( w );

    static const std::vector<std::string> tab_strings = { {
            translate_marker( "World Mods" ),
            translate_marker( "World Options" ),
            translate_marker( "Finalize World" )
        }
    };

    draw_tabs( w, tab_strings, current );
    draw_border_below_tabs( w );
}

bool worldfactory::valid_worldname( const std::string &name, bool automated )
{
    std::string msg;

    if( name == "save" || name == "TUTORIAL" || name == "DEFENSE" ) {
        msg = string_format( _( "%s is a reserved name!" ), name );
    } else if( !has_world( name ) ) {
        return true;
    } else {
        msg = string_format( _( "A world named %s already exists!" ), name );
    }
    if( !automated ) {
        popup( msg, PF_GET_KEY );
    }
    return false;
}

void WORLD::load_options( JsonIn &jsin )
{
    // if core data version isn't specified then assume version 1
    int version = 1;

    auto &opts = get_options();

    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo = jsin.get_object();
        const std::string name = opts.migrateOptionName( jo.get_string( "name" ) );
        const std::string value = opts.migrateOptionValue( jo.get_string( "name" ),
                                  jo.get_string( "value" ) );

        // Verify format of options file
        if( !jo.has_string( "info" ) || !jo.has_string( "default" ) ) {
            dbg( D_ERROR ) << "options object " << name << " was missing info or default";
        }

        if( name == "CORE_VERSION" ) {
            version = std::max( std::atoi( value.c_str() ), 0 );
            continue;
        }

        if( opts.has_option( name ) && opts.get_option( name ).getPage() == "world_default" ) {
            WORLD_OPTIONS[ name ].setValue( value );
        }
    }
    // for legacy saves, try to simulate old city_size based density
    if( WORLD_OPTIONS.count( "CITY_SPACING" ) == 0 ) {
        WORLD_OPTIONS["CITY_SPACING"].setValue( 5 - get_option<int>( "CITY_SIZE" ) / 3 );
    }

    WORLD_OPTIONS[ "CORE_VERSION" ].setValue( version );
}

void WORLD::load_legacy_options( std::istream &fin )
{
    auto &opts = get_options();

    //load legacy txt
    std::string sLine;
    while( !fin.eof() ) {
        getline( fin, sLine );
        if( !sLine.empty() && sLine[0] != '#' && std::count( sLine.begin(), sLine.end(), ' ' ) == 1 ) {
            size_t ipos = sLine.find( ' ' );
            // make sure that the option being loaded is part of the world_default page in OPTIONS
            // In 0.C some lines consisted of a space and nothing else
            const std::string name = opts.migrateOptionName( sLine.substr( 0, ipos ) );
            const std::string value = opts.migrateOptionValue( sLine.substr( 0, ipos ), sLine.substr( ipos + 1,
                                      sLine.length() ) );

            if( ipos != 0 && opts.get_option( name ).getPage() == "world_default" ) {
                WORLD_OPTIONS[name].setValue( value );
            }
        }
    }
}

bool WORLD::load_options()
{
    WORLD_OPTIONS = get_options().get_world_defaults();

    using namespace std::placeholders;
    const auto path = folder_path() + "/" + FILENAMES["worldoptions"];
    if( read_from_file_optional_json( path, [&]( JsonIn & jsin ) {
    load_options( jsin );
    } ) ) {
        return true;
    }

    const auto legacy_path = folder_path() + "/" + FILENAMES["legacy_worldoptions"];
    if( read_from_file_optional( legacy_path, std::bind( &WORLD::load_legacy_options, this, _1 ) ) ) {
        if( save() ) {
            // Remove old file as the options have been saved to the new file.
            remove_file( legacy_path );
        }
        return true;
    }

    return false;
}

void load_world_option( JsonObject &jo )
{
    auto arr = jo.get_array( "options" );
    if( arr.empty() ) {
        jo.throw_error( "no options specified", "options" );
    }
    while( arr.has_more() ) {
        get_options().get_option( arr.next_string() ).setValue( "true" );
    }
}

//load external option from json
void load_external_option( JsonObject &jo )
{
    auto name = jo.get_string( "name" );
    auto stype = jo.get_string( "stype" );
    options_manager &opts = get_options();
    if( !opts.has_option( name ) ) {
        auto sinfo = jo.get_string( "info" );
        opts.add_external( name, "world_default", stype, sinfo, sinfo );
    }
    options_manager::cOpt &opt = opts.get_option( name );
    if( stype == "float" ) {
        opt.setValue( static_cast<float>( jo.get_float( "value" ) ) );
    } else if( stype == "int" ) {
        opt.setValue( jo.get_int( "value" ) );
    } else if( stype == "bool" ) {
        if( jo.get_bool( "value" ) ) {
            opt.setValue( "true" );
        } else {
            opt.setValue( "false" );
        }
    } else if( stype == "string" ) {
        opt.setValue( jo.get_string( "value" ) );
    } else {
        jo.throw_error( "Unknown or unsupported stype for external option", "stype" );
    }
}

mod_manager &worldfactory::get_mod_manager()
{
    return *mman;
}

WORLDPTR worldfactory::get_world( const std::string &name )
{
    const auto iter = all_worlds.find( name );
    if( iter == all_worlds.end() ) {
        debugmsg( "Requested non-existing world %s, prepare for crash", name );
        return nullptr;
    }
    return iter->second.get();
}

// Helper predicate to exclude files from deletion when resetting a world directory.
static bool isForbidden( const std::string &candidate )
{
    return candidate.find( FILENAMES["worldoptions"] ) != std::string::npos ||
           candidate.find( "mods.json" ) != std::string::npos;
}

void worldfactory::delete_world( const std::string &worldname, const bool delete_folder )
{
    std::string worldpath = get_world( worldname )->folder_path();
    std::set<std::string> directory_paths;

    auto file_paths = get_files_from_path( "", worldpath, true, true );
    if( !delete_folder ) {
        std::vector<std::string>::iterator forbidden = find_if( file_paths.begin(), file_paths.end(),
                isForbidden );
        while( forbidden != file_paths.end() ) {
            file_paths.erase( forbidden );
            forbidden = find_if( file_paths.begin(), file_paths.end(), isForbidden );
        }
    }
    for( auto &file_path : file_paths ) {
        // strip to path and remove worldpath from it
        std::string part = file_path.substr( worldpath.size(),
                                             file_path.find_last_of( "/\\" ) - worldpath.size() );
        size_t last_separator = part.find_last_of( "/\\" );
        while( last_separator != std::string::npos && part.size() > 1 ) {
            directory_paths.insert( part );
            part = part.substr( 0, last_separator );
            last_separator = part.find_last_of( "/\\" );
        }
    }

    for( auto &file : file_paths ) {
        remove_file( file );
    }
    // Trying to remove a non-empty parent directory before a child
    // directory will fail.  Removing directories in reverse order
    // will prevent this situation from arising.
    for( auto it = directory_paths.rbegin(); it != directory_paths.rend(); ++it ) {
        remove_directory( worldpath + *it );
    }
    if( delete_folder ) {
        remove_directory( worldpath );
        remove_world( worldname );
    } else {
        get_world( worldname )->world_saves.clear();
    }
}
