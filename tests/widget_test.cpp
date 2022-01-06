#include "catch/catch.hpp"

#include "player_helpers.h"
#include "morale.h"
#include "options_helpers.h"
#include "weather.h"
#include "weather_type.h"
#include "widget.h"

// Needed for screen scraping
#if !(defined(TILES) || defined(_WIN32))
namespace cata_curses_test
{
#define NCURSES_NOMACROS
#if defined(__CYGWIN__)
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif
}
#else
#include "cursesport.h"
#endif

static const itype_id itype_rad_badge( "rad_badge" );

static const weather_type_id weather_acid_rain( "acid_rain" );
static const weather_type_id weather_cloudy( "cloudy" );
static const weather_type_id weather_drizzle( "drizzle" );
static const weather_type_id weather_portal_storm( "portal_storm" );
static const weather_type_id weather_snowing( "snowing" );
static const weather_type_id weather_sunny( "sunny" );

// test widgets defined in data/json/sidebar.json and data/mods/TEST_DATA/widgets.json
static const widget_id widget_test_2_column_layout( "test_2_column_layout" );
static const widget_id widget_test_3_column_layout( "test_3_column_layout" );
static const widget_id widget_test_4_column_layout( "test_4_column_layout" );
static const widget_id widget_test_bp_wetness_head_num( "test_bp_wetness_head_num" );
static const widget_id widget_test_bp_wetness_torso_num( "test_bp_wetness_torso_num" );
static const widget_id widget_test_bucket_graph( "test_bucket_graph" );
static const widget_id widget_test_color_graph_10k_widget( "test_color_graph_10k_widget" );
static const widget_id widget_test_color_graph_widget( "test_color_graph_widget" );
static const widget_id widget_test_color_number_widget( "test_color_number_widget" );
static const widget_id widget_test_dex_num( "test_dex_num" );
static const widget_id widget_test_focus_num( "test_focus_num" );
static const widget_id widget_test_hp_head_graph( "test_hp_head_graph" );
static const widget_id widget_test_hp_head_num( "test_hp_head_num" );
static const widget_id widget_test_int_num( "test_int_num" );
static const widget_id widget_test_mana_num( "test_mana_num" );
static const widget_id widget_test_morale_num( "test_morale_num" );
static const widget_id widget_test_move_num( "test_move_num" );
static const widget_id widget_test_per_num( "test_per_num" );
static const widget_id widget_test_pool_graph( "test_pool_graph" );
static const widget_id widget_test_rad_badge_text( "test_rad_badge_text" );
static const widget_id widget_test_speed_num( "test_speed_num" );
static const widget_id widget_test_stamina_graph( "test_stamina_graph" );
static const widget_id widget_test_stamina_num( "test_stamina_num" );
static const widget_id widget_test_stat_panel( "test_stat_panel" );
static const widget_id widget_test_str_num( "test_str_num" );
static const widget_id widget_test_text_widget( "test_text_widget" );
static const widget_id widget_test_weariness_num( "test_weariness_num" );
static const widget_id widget_test_weather_text( "test_weather_text" );
static const widget_id widget_test_weather_text_height5( "test_weather_text_height5" );

// dseguin 2022 - Ugly hack to scrape content from the window object.
// Scrapes the window w at origin, reading the number of cols and rows.
static std::vector<std::string> scrape_win_at(
    catacurses::window &w, const point &origin, int cols, int rows )
{
    std::vector<std::string> lines;

#if defined(TILES) || defined(_WIN32)
    cata_cursesport::WINDOW *win = static_cast<cata_cursesport::WINDOW *>( w.get() );

    for( int i = origin.y; i < rows && static_cast<size_t>( i ) < win->line.size(); i++ ) {
        lines.emplace_back( std::string() );
        for( int j = origin.x; j < cols && static_cast<size_t>( j ) < win->line[i].chars.size(); j++ ) {
            lines[i] += win->line[i].chars[j].ch;
        }
    }
#else
    cata_curses_test::WINDOW *win = static_cast<cata_curses_test::WINDOW *>( w.get() );

    int max_y = catacurses::getmaxy( w );
    for( int i = origin.y; i < rows && i < max_y; i++ ) {
        wchar_t *buf = static_cast<wchar_t *>( ::malloc( sizeof( *buf ) * ( cols + 1 ) ) );
        cata_curses_test::mvwinnwstr( win, i, origin.x, buf, cols );
        std::wstring line( buf, static_cast<size_t>( cols ), std::allocator<wchar_t>() );
        lines.emplace_back( std::string( line.begin(), line.end() ) );
        ::free( buf );
    }
#endif

    return lines;
}

TEST_CASE( "widget value strings", "[widget][value][string]" )
{
    SECTION( "numeric values" ) {
        widget focus = widget_test_focus_num.obj();
        REQUIRE( focus._style == "number" );
        CHECK( focus.value_string( 0 ) == "0" );
        CHECK( focus.value_string( 50 ) == "50" );
        CHECK( focus.value_string( 100 ) == "100" );
    }

    SECTION( "graph values with bucket fill" ) {
        widget head = widget_test_hp_head_graph.obj();
        REQUIRE( head._style == "graph" );
        REQUIRE( head._fill == "bucket" );
        // Buckets of width 5 with 2 nonzero symbols can show 10 values
        REQUIRE( head._width == 5 );
        // Uses comma instead of period to avoid clang-tidy complaining about ellipses
        REQUIRE( head._symbols == ",\\|" );
        // Each integer step cycles the graph symbol by one
        CHECK( head.value_string( 0 ) == ",,,,," );
        CHECK( head.value_string( 1 ) == "\\,,,," );
        CHECK( head.value_string( 2 ) == "|,,,," );
        CHECK( head.value_string( 3 ) == "|\\,,," );
        CHECK( head.value_string( 4 ) == "||,,," );
        CHECK( head.value_string( 5 ) == "||\\,," );
        CHECK( head.value_string( 6 ) == "|||,," );
        CHECK( head.value_string( 7 ) == "|||\\," );
        CHECK( head.value_string( 8 ) == "||||," );
        CHECK( head.value_string( 9 ) == "||||\\" );
        CHECK( head.value_string( 10 ) == "|||||" );
        // Above max displayable value, show max
        CHECK( head.value_string( 11 ) == "|||||" );
        CHECK( head.value_string( 12 ) == "|||||" );
        // Below minimum, show min
        CHECK( head.value_string( -1 ) == ",,,,," );
        CHECK( head.value_string( -2 ) == ",,,,," );
    }

    SECTION( "graph values with pool fill" ) {
        widget stamina = widget_test_stamina_graph.obj();
        REQUIRE( stamina._style == "graph" );
        REQUIRE( stamina._fill == "pool" );
        // Pool of width 20 with 2 nonzero symbols can show 20 values
        REQUIRE( stamina._width == 10 );
        REQUIRE( stamina._symbols == "-=#" );
        // Each integer step increases the graph symbol by one
        CHECK( stamina.value_string( 0 ) == "----------" );
        CHECK( stamina.value_string( 1 ) == "=---------" );
        CHECK( stamina.value_string( 2 ) == "==--------" );
        CHECK( stamina.value_string( 3 ) == "===-------" );
        CHECK( stamina.value_string( 4 ) == "====------" );
        CHECK( stamina.value_string( 6 ) == "======----" );
        CHECK( stamina.value_string( 8 ) == "========--" );
        CHECK( stamina.value_string( 10 ) == "==========" );
        CHECK( stamina.value_string( 12 ) == "##========" );
        CHECK( stamina.value_string( 14 ) == "####======" );
        CHECK( stamina.value_string( 16 ) == "######====" );
        CHECK( stamina.value_string( 18 ) == "########==" );
        CHECK( stamina.value_string( 20 ) == "##########" );
        // Above max displayable value, show max
        CHECK( stamina.value_string( 21 ) == "##########" );
        CHECK( stamina.value_string( 22 ) == "##########" );
        // Below minimum, show min
        CHECK( stamina.value_string( -1 ) == "----------" );
        CHECK( stamina.value_string( -2 ) == "----------" );
    }
}

TEST_CASE( "widgets", "[widget][graph][color]" )
{
    SECTION( "text widgets" ) {
        widget words = widget_test_text_widget.obj();
        REQUIRE( words._style == "text" );

        CHECK( words.text( 0 ) == "Zero" );
        CHECK( words.text( 1 ) == "One" );
        CHECK( words.text( 2 ) == "Two" );
        CHECK( words.text( 3 ) == "Three" );
        CHECK( words.text( 4 ) == "Four" );
        CHECK( words.text( 5 ) == "Five" );
        CHECK( words.text( 6 ) == "Six" );
        CHECK( words.text( 7 ) == "Seven" );
        CHECK( words.text( 8 ) == "Eight" );
        CHECK( words.text( 9 ) == "Nine" );
        CHECK( words.text( 10 ) == "Ten" );
    }

    SECTION( "number widget with color" ) {
        widget colornum = widget_test_color_number_widget.obj();
        REQUIRE( colornum._style == "number" );
        REQUIRE( colornum._colors.size() == 3 );
        REQUIRE( colornum._var_max == 2 );

        CHECK( colornum.color_value_string( 0 ) == "<color_c_red>0</color>" );
        CHECK( colornum.color_value_string( 1 ) == "<color_c_yellow>1</color>" );
        CHECK( colornum.color_value_string( 2 ) == "<color_c_green>2</color>" );
        // Beyond var_max, stays at max color
        CHECK( colornum.color_value_string( 3 ) == "<color_c_green>3</color>" );
    }

    SECTION( "graph widget with color" ) {
        widget colornum = widget_test_color_graph_widget.obj();
        REQUIRE( colornum._style == "graph" );
        REQUIRE( colornum._colors.size() == 4 );
        REQUIRE( colornum._var_max == 10 );

        // with +0.5: 2r, 3y, 4lg, 2g
        CHECK( colornum.color_value_string( 0 ) == "<color_c_red>-----</color>" );
        CHECK( colornum.color_value_string( 1 ) == "<color_c_red>=----</color>" );
        CHECK( colornum.color_value_string( 2 ) == "<color_c_yellow>#----</color>" );
        CHECK( colornum.color_value_string( 3 ) == "<color_c_yellow>#=---</color>" );
        CHECK( colornum.color_value_string( 4 ) == "<color_c_yellow>##---</color>" );
        CHECK( colornum.color_value_string( 5 ) == "<color_c_light_green>##=--</color>" );
        CHECK( colornum.color_value_string( 6 ) == "<color_c_light_green>###--</color>" );
        CHECK( colornum.color_value_string( 7 ) == "<color_c_light_green>###=-</color>" );
        CHECK( colornum.color_value_string( 8 ) == "<color_c_light_green>####-</color>" );
        CHECK( colornum.color_value_string( 9 ) == "<color_c_green>####=</color>" );
        CHECK( colornum.color_value_string( 10 ) == "<color_c_green>#####</color>" );
        // Beyond var_max, stays at max color
        CHECK( colornum.color_value_string( 11 ) == "<color_c_green>#####</color>" );

        // Long / large var graph
        widget graph10k = widget_test_color_graph_10k_widget.obj();
        REQUIRE( graph10k._style == "graph" );
        REQUIRE( graph10k._colors.size() == 5 );
        REQUIRE( graph10k._var_max == 10000 );

        CHECK( graph10k.color_value_string( 0 ) == "<color_c_red>----------</color>" );
        CHECK( graph10k.color_value_string( 2500 ) == "<color_c_light_red>=====-----</color>" );
        CHECK( graph10k.color_value_string( 5000 ) == "<color_c_yellow>==========</color>" );
        CHECK( graph10k.color_value_string( 7500 ) == "<color_c_light_green>#####=====</color>" );
        CHECK( graph10k.color_value_string( 10000 ) == "<color_c_green>##########</color>" );
    }

    SECTION( "graph widgets" ) {
        SECTION( "bucket fill" ) {
            widget wid = widget_test_bucket_graph.obj();
            REQUIRE( wid._style == "graph" );
            REQUIRE( wid._fill == "bucket" );

            CHECK( wid.graph( 0 ) == "0000" );
            CHECK( wid.graph( 1 ) == "1000" );
            CHECK( wid.graph( 2 ) == "2000" );
            CHECK( wid.graph( 3 ) == "3000" );
            CHECK( wid.graph( 4 ) == "3100" );
            CHECK( wid.graph( 5 ) == "3200" );
            CHECK( wid.graph( 6 ) == "3300" );
            CHECK( wid.graph( 7 ) == "3310" );
            CHECK( wid.graph( 8 ) == "3320" );
            CHECK( wid.graph( 9 ) == "3330" );
            CHECK( wid.graph( 10 ) == "3331" );
            CHECK( wid.graph( 11 ) == "3332" );
            CHECK( wid.graph( 12 ) == "3333" );
        }
        SECTION( "pool fill" ) {
            widget wid = widget_test_pool_graph.obj();
            REQUIRE( wid._style == "graph" );
            REQUIRE( wid._fill == "pool" );

            CHECK( wid.graph( 0 ) == "0000" );
            CHECK( wid.graph( 1 ) == "1000" );
            CHECK( wid.graph( 2 ) == "1100" );
            CHECK( wid.graph( 3 ) == "1110" );
            CHECK( wid.graph( 4 ) == "1111" );
            CHECK( wid.graph( 5 ) == "2111" );
            CHECK( wid.graph( 6 ) == "2211" );
            CHECK( wid.graph( 7 ) == "2221" );
            CHECK( wid.graph( 8 ) == "2222" );
            CHECK( wid.graph( 9 ) == "3222" );
            CHECK( wid.graph( 10 ) == "3322" );
            CHECK( wid.graph( 11 ) == "3332" );
            CHECK( wid.graph( 12 ) == "3333" );
        }
    }

    SECTION( "graph hit points" ) {
        widget wid = widget_test_hp_head_graph.obj();
        REQUIRE( wid._fill == "bucket" );

        CHECK( wid._label.translated() == "HEAD" );
        CHECK( wid.graph( 0 ) == ",,,,," );
        CHECK( wid.graph( 1 ) == "\\,,,," );
        CHECK( wid.graph( 2 ) == "|,,,," );
        CHECK( wid.graph( 3 ) == "|\\,,," );
        CHECK( wid.graph( 4 ) == "||,,," );
        CHECK( wid.graph( 5 ) == "||\\,," );
        CHECK( wid.graph( 6 ) == "|||,," );
        CHECK( wid.graph( 7 ) == "|||\\," );
        CHECK( wid.graph( 8 ) == "||||," );
        CHECK( wid.graph( 9 ) == "||||\\" );
        CHECK( wid.graph( 10 ) == "|||||" );
    }
}

TEST_CASE( "widgets showing avatar attributes", "[widget][avatar]" )
{
    avatar &ava = get_avatar();
    clear_avatar();

    SECTION( "base stats str / dex / int / per" ) {
        widget str_w = widget_test_str_num.obj();
        widget dex_w = widget_test_dex_num.obj();
        widget int_w = widget_test_int_num.obj();
        widget per_w = widget_test_per_num.obj();

        ava.str_max = 8;
        ava.dex_max = 10;
        ava.int_max = 7;
        ava.per_max = 13;

        CHECK( str_w.layout( ava ) == "STR: 8" );
        CHECK( dex_w.layout( ava ) == "DEX: 10" );
        CHECK( int_w.layout( ava ) == "INT: 7" );
        CHECK( per_w.layout( ava ) == "PER: 13" );
    }

    SECTION( "stamina" ) {
        widget stamina_num_w = widget_test_stamina_num.obj();
        widget stamina_graph_w = widget_test_stamina_graph.obj();
        REQUIRE( stamina_graph_w._fill == "pool" );
        REQUIRE( stamina_graph_w._symbols == "-=#" );

        ava.set_stamina( 0 );
        CHECK( stamina_num_w.layout( ava ) == "STAMINA: 0" );
        CHECK( stamina_graph_w.layout( ava ) == "STAMINA: ----------" );
        ava.set_stamina( 2500 );
        CHECK( stamina_num_w.layout( ava ) == "STAMINA: 2500" );
        CHECK( stamina_graph_w.layout( ava ) == "STAMINA: =====-----" );
        ava.set_stamina( 5000 );
        CHECK( stamina_num_w.layout( ava ) == "STAMINA: 5000" );
        CHECK( stamina_graph_w.layout( ava ) == "STAMINA: ==========" );
        ava.set_stamina( 7500 );
        CHECK( stamina_num_w.layout( ava ) == "STAMINA: 7500" );
        CHECK( stamina_graph_w.layout( ava ) == "STAMINA: #####=====" );
        ava.set_stamina( 10000 );
        CHECK( stamina_num_w.layout( ava ) == "STAMINA: 10000" );
        CHECK( stamina_graph_w.layout( ava ) == "STAMINA: ##########" );
    }

    SECTION( "speed pool" ) {
        widget speed_w = widget_test_speed_num.obj();

        ava.set_speed_base( 90 );
        CHECK( speed_w.layout( ava ) == "SPEED: 90" );
        ava.set_speed_base( 240 );
        CHECK( speed_w.layout( ava ) == "SPEED: 240" );
    }

    SECTION( "focus pool" ) {
        widget focus_w = widget_test_focus_num.obj();

        ava.set_focus( 75 );
        CHECK( focus_w.layout( ava ) == "FOCUS: 75" );
        ava.set_focus( 120 );
        CHECK( focus_w.layout( ava ) == "FOCUS: 120" );
    }

    SECTION( "mana pool" ) {
        widget mana_w = widget_test_mana_num.obj();

        ava.magic->set_mana( 150 );
        CHECK( mana_w.layout( ava ) == "MANA: 150" );
        ava.magic->set_mana( 450 );
        CHECK( mana_w.layout( ava ) == "MANA: 450" );
    }

    SECTION( "morale" ) {
        widget morale_w = widget_test_morale_num.obj();

        ava.clear_morale();
        CHECK( morale_w.layout( ava ) == "MORALE: 0" );
        ava.add_morale( MORALE_FOOD_GOOD, 20 );
        CHECK( morale_w.layout( ava ) == "MORALE: 20" );

        ava.clear_morale();
        ava.add_morale( MORALE_KILLED_INNOCENT, -100 );
        CHECK( morale_w.layout( ava ) == "MORALE: -100" );
    }

    SECTION( "move counter" ) {
        widget move_w = widget_test_move_num.obj();

        ava.movecounter = 80;
        CHECK( move_w.layout( ava ) == "MOVE: 80" );
        ava.movecounter = 150;
        CHECK( move_w.layout( ava ) == "MOVE: 150" );
    }

    SECTION( "hit points" ) {
        bodypart_id head( "head" );
        widget head_num_w = widget_test_hp_head_num.obj();
        widget head_graph_w = widget_test_hp_head_graph.obj();
        REQUIRE( ava.get_part_hp_max( head ) == 84 );
        REQUIRE( ava.get_part_hp_cur( head ) == 84 );

        ava.set_part_hp_cur( head, 84 );
        CHECK( head_num_w.layout( ava ) == "HEAD: 84" );
        CHECK( head_graph_w.layout( ava ) == "HEAD: |||||" );
        ava.set_part_hp_cur( head, 42 );
        CHECK( head_num_w.layout( ava ) == "HEAD: 42" );
        CHECK( head_graph_w.layout( ava ) == "HEAD: ||\\,," );
        ava.set_part_hp_cur( head, 17 );
        CHECK( head_num_w.layout( ava ) == "HEAD: 17" );
        CHECK( head_graph_w.layout( ava ) == "HEAD: |,,,," );
        ava.set_part_hp_cur( head, 0 );
        CHECK( head_num_w.layout( ava ) == "HEAD: 0" );
        // NOLINTNEXTLINE(cata-text-style): suppress "unnecessary space" warning before commas
        CHECK( head_graph_w.layout( ava ) == "HEAD: ,,,,," );
    }

    SECTION( "weariness" ) {
        widget weariness_w = widget_test_weariness_num.obj();

        CHECK( weariness_w.layout( ava ) == "WEARINESS: 0" );
        // TODO: Check weariness set to other levels
    }

    SECTION( "wetness" ) {
        widget head_wetness_w = widget_test_bp_wetness_head_num.obj();
        widget torso_wetness_w = widget_test_bp_wetness_torso_num.obj();

        CHECK( head_wetness_w.layout( ava ) == "HEAD WET: 0" );
        CHECK( torso_wetness_w.layout( ava ) == "TORSO WET: 0" );
        ava.drench( 100, { body_part_head, body_part_torso }, false );
        CHECK( head_wetness_w.layout( ava ) == "HEAD WET: 2" );
        CHECK( torso_wetness_w.layout( ava ) == "TORSO WET: 2" );
    }
}

TEST_CASE( "radiation badge widget", "[widget][radiation]" )
{
    widget rads_w = widget_test_rad_badge_text.obj();

    avatar &ava = get_avatar();
    clear_avatar();

    // No indicator when character has no radiation badge
    CHECK( rads_w.layout( ava ) == "RADIATION: <color_c_light_gray>Unknown</color>" );

    // Acquire and wear a radiation badge
    item &rad_badge = ava.i_add( item( itype_rad_badge ) );
    ava.worn.emplace_back( rad_badge );

    // Color indicator is shown when character has radiation badge
    rad_badge.irradiation = 0;
    CHECK( rads_w.layout( ava ) == "RADIATION: <color_c_white_green> green </color>" );
    // Any positive value turns it blue
    rad_badge.irradiation = 1;
    CHECK( rads_w.layout( ava ) == "RADIATION: <color_h_white> blue </color>" );
    rad_badge.irradiation = 29;
    CHECK( rads_w.layout( ava ) == "RADIATION: <color_h_white> blue </color>" );
    rad_badge.irradiation = 31;
    CHECK( rads_w.layout( ava ) == "RADIATION: <color_i_yellow> yellow </color>" );
    rad_badge.irradiation = 61;
    CHECK( rads_w.layout( ava ) == "RADIATION: <color_c_red_yellow> orange </color>" );
    rad_badge.irradiation = 121;
    CHECK( rads_w.layout( ava ) == "RADIATION: <color_c_red_red> red </color>" );
    rad_badge.irradiation = 241;
    CHECK( rads_w.layout( ava ) == "RADIATION: <color_c_pink> black </color>" );
}

// Widgets with "layout" style can combine other widgets in columns or rows.
//
// Using "arrange": "columns", width is divided as equally as possible among widgets.
// With C columns, (C-1)*2 characters are allotted for space between columns (__):
//
//     C=2: FIRST__SECOND
//     C=3: FIRST__SECOND__THIRD
//     C=4: FIRST__SECOND__THIRD__FOURTH
//
// So total width available to each column is:
//
//     (W - (C-1)*2) / C
//
// At 24 width, 2 columns, each column gets (24 - (2-1)*2) / 2 == 11 characters.
// At 36 width, 2 columns, each column gets (36 - (2-1)*2) / 2 == 17 characters.
// At 36 width, 3 columns, each column gets (36 - (3-1)*2) / 3 == 10 characters.
//
// This test case calls layout() at different widths for 2-, 3-, and 4-column layouts,
// to verify and demonstrate how the space is distributed among widgets in columns.
//
TEST_CASE( "layout widgets in columns", "[widget][layout][columns]" )
{
    widget stat_w = widget_test_stat_panel.obj();
    widget two_w = widget_test_2_column_layout.obj();
    widget three_w = widget_test_3_column_layout.obj();
    widget four_w = widget_test_4_column_layout.obj();

    avatar &ava = get_avatar();
    clear_avatar();

    ava.str_max = 8;
    ava.dex_max = 8;
    ava.int_max = 8;
    ava.per_max = 8;
    ava.movecounter = 50;
    ava.set_focus( 120 );
    ava.set_speed_base( 100 );
    ava.magic->set_mana( 1000 );

    // Two columns
    // string ruler:                   123456789012345678901234567890123456
    CHECK( two_w.layout( ava, 24 ) == "MOVE:    50  SPEED:  100" );
    CHECK( two_w.layout( ava, 25 ) == "MOVE:     50  SPEED:  100" );
    CHECK( two_w.layout( ava, 26 ) == "MOVE:     50  SPEED:   100" );
    CHECK( two_w.layout( ava, 27 ) == "MOVE:      50  SPEED:   100" );
    CHECK( two_w.layout( ava, 28 ) == "MOVE:      50  SPEED:    100" );
    CHECK( two_w.layout( ava, 29 ) == "MOVE:       50  SPEED:    100" );
    CHECK( two_w.layout( ava, 30 ) == "MOVE:       50  SPEED:     100" );
    CHECK( two_w.layout( ava, 31 ) == "MOVE:        50  SPEED:     100" );
    CHECK( two_w.layout( ava, 32 ) == "MOVE:        50  SPEED:      100" );
    CHECK( two_w.layout( ava, 33 ) == "MOVE:         50  SPEED:      100" );
    CHECK( two_w.layout( ava, 34 ) == "MOVE:         50  SPEED:       100" );
    CHECK( two_w.layout( ava, 35 ) == "MOVE:          50  SPEED:       100" );
    CHECK( two_w.layout( ava, 36 ) == "MOVE:          50  SPEED:        100" );
    // string ruler:                   123456789012345678901234567890123456

    // Three columns
    // string ruler:                     1234567890123456789012345678901234567890
    CHECK( three_w.layout( ava, 36 ) == "MOVE:    50  SPEED:  100  FOCUS: 120" );
    CHECK( three_w.layout( ava, 37 ) == "MOVE:    50  SPEED:  100  FOCUS:  120" );
    CHECK( three_w.layout( ava, 38 ) == "MOVE:     50  SPEED:  100  FOCUS:  120" );
    CHECK( three_w.layout( ava, 39 ) == "MOVE:     50  SPEED:   100  FOCUS:  120" );
    CHECK( three_w.layout( ava, 40 ) == "MOVE:     50  SPEED:   100  FOCUS:   120" );
    CHECK( three_w.layout( ava, 41 ) == "MOVE:      50  SPEED:   100  FOCUS:   120" );
    CHECK( three_w.layout( ava, 42 ) == "MOVE:      50  SPEED:    100  FOCUS:   120" );
    CHECK( three_w.layout( ava, 43 ) == "MOVE:      50  SPEED:    100  FOCUS:    120" );
    CHECK( three_w.layout( ava, 44 ) == "MOVE:       50  SPEED:    100  FOCUS:    120" );
    CHECK( three_w.layout( ava, 45 ) == "MOVE:       50  SPEED:     100  FOCUS:    120" );
    CHECK( three_w.layout( ava, 46 ) == "MOVE:       50  SPEED:     100  FOCUS:     120" );
    // string ruler:                     1234567890123456789012345678901234567890123456

    // Four columns
    // string ruler:                    123456789012345678901234567890123456789012
    CHECK( stat_w.layout( ava, 32 ) == "STR:  8  DEX:  8  INT: 8  PER: 8" );
    CHECK( stat_w.layout( ava, 33 ) == "STR:  8  DEX:  8  INT:  8  PER: 8" );
    CHECK( stat_w.layout( ava, 34 ) == "STR:  8  DEX:  8  INT:  8  PER:  8" );
    CHECK( stat_w.layout( ava, 35 ) == "STR:   8  DEX:  8  INT:  8  PER:  8" );
    CHECK( stat_w.layout( ava, 36 ) == "STR:   8  DEX:   8  INT:  8  PER:  8" );
    CHECK( stat_w.layout( ava, 37 ) == "STR:   8  DEX:   8  INT:   8  PER:  8" );
    CHECK( stat_w.layout( ava, 38 ) == "STR:   8  DEX:   8  INT:   8  PER:   8" );
    CHECK( stat_w.layout( ava, 39 ) == "STR:    8  DEX:   8  INT:   8  PER:   8" );
    CHECK( stat_w.layout( ava, 40 ) == "STR:    8  DEX:    8  INT:   8  PER:   8" );
    CHECK( stat_w.layout( ava, 41 ) == "STR:    8  DEX:    8  INT:    8  PER:   8" );
    CHECK( stat_w.layout( ava, 42 ) == "STR:    8  DEX:    8  INT:    8  PER:    8" );
    CHECK( stat_w.layout( ava, 43 ) == "STR:     8  DEX:    8  INT:    8  PER:    8" );
    CHECK( stat_w.layout( ava, 44 ) == "STR:     8  DEX:     8  INT:    8  PER:    8" );
    CHECK( stat_w.layout( ava, 45 ) == "STR:     8  DEX:     8  INT:     8  PER:    8" );
    CHECK( stat_w.layout( ava, 46 ) == "STR:     8  DEX:     8  INT:     8  PER:     8" );
    // string ruler:                    1234567890123456789012345678901234567890123456

    // Column alignment
    // Layout keeps labels vertically aligned for layouts with the same number of widgets
    // string ruler:                    123456789012345678901234567890123456789012345678
    CHECK( stat_w.layout( ava, 48 ) == "STR:      8  DEX:      8  INT:     8  PER:     8" );
    CHECK( four_w.layout( ava, 48 ) == "MOVE:    50  SPEED:  100  FOCUS: 120  MANA: 1000" );

    // string ruler:                    1234567890123456789012345678901234567890123456789012
    CHECK( stat_w.layout( ava, 52 ) == "STR:       8  DEX:       8  INT:      8  PER:      8" );
    CHECK( four_w.layout( ava, 52 ) == "MOVE:     50  SPEED:   100  FOCUS:  120  MANA:  1000" );

    // string ruler:                    12345678901234567890123456789012345678901234567890123456
    CHECK( stat_w.layout( ava, 56 ) == "STR:        8  DEX:        8  INT:       8  PER:       8" );
    CHECK( four_w.layout( ava, 56 ) == "MOVE:      50  SPEED:    100  FOCUS:   120  MANA:   1000" );

    // string ruler:                    123456789012345678901234567890123456789012345678901234567890
    CHECK( stat_w.layout( ava, 60 ) == "STR:         8  DEX:         8  INT:        8  PER:        8" );
    CHECK( four_w.layout( ava, 60 ) == "MOVE:       50  SPEED:     100  FOCUS:    120  MANA:    1000" );

    // TODO: Consider re-distributing space so values are closer to labels, like this:
    // 48 width
    //     "STR: 8      DEX: 8      INT: 8      PER: 8      "
    //     "MOVE: 0     SPEED: 100  FOCUS: 100  MANA: 1000  "
    // 60 width
    //     "STR: 8         DEX: 8         INT: 8         PER: 8         "
    //     "MOVE: 0        SPEED: 100     FOCUS: 100     MANA: 1000     "
}

TEST_CASE( "widgets showing weather conditions", "[widget][weather]" )
{
    widget weather_w = widget_test_weather_text.obj();

    avatar &ava = get_avatar();
    clear_avatar();

    SECTION( "weather conditions" ) {
        SECTION( "sunny" ) {
            scoped_weather_override forecast( weather_sunny );
            REQUIRE( get_weather().weather_id->name.translated() == "Sunny" );
            CHECK( weather_w.layout( ava ) == "Weather: <color_c_light_cyan>Sunny</color>" );
        }

        SECTION( "cloudy" ) {
            scoped_weather_override forecast( weather_cloudy );
            CHECK( weather_w.layout( ava ) == "Weather: <color_c_light_gray>Cloudy</color>" );
        }

        SECTION( "drizzle" ) {
            scoped_weather_override forecast( weather_drizzle );
            CHECK( weather_w.layout( ava ) == "Weather: <color_c_light_blue>Drizzle</color>" );
        }

        SECTION( "snowing" ) {
            scoped_weather_override forecast( weather_snowing );
            CHECK( weather_w.layout( ava ) == "Weather: <color_c_white>Snowing</color>" );
        }

        SECTION( "acid rain" ) {
            scoped_weather_override forecast( weather_acid_rain );
            CHECK( weather_w.layout( ava ) == "Weather: <color_c_green>Acid Rain</color>" );
        }

        SECTION( "portal storm" ) {
            scoped_weather_override forecast( weather_portal_storm );
            CHECK( weather_w.layout( ava ) == "Weather: <color_c_red>Portal Storm</color>" );
        }

        SECTION( "cannot see weather when underground" ) {
            ava.setpos( tripoint_below );
            CHECK( weather_w.layout( ava ) == "Weather: <color_c_light_gray>Underground</color>" );
        }
    }
}

TEST_CASE( "Custom widget height and multiline formatting", "[widget]" )
{
    const int cols = 32;
    const int rows = 5;
    widget height1 = widget_test_weather_text.obj();
    widget height5 = widget_test_weather_text_height5.obj();

    avatar &ava = get_avatar();
    clear_avatar();
    scoped_weather_override forcast( weather_sunny );

    SECTION( "Height field does not impact text content" ) {
        std::string layout1 = height1.layout( ava );
        std::string layout5 = height5.layout( ava );
        CHECK( height1._height == 1 );
        CHECK( height5._height == 5 );
        CHECK( layout1 == "Weather: <color_c_light_cyan>Sunny</color>" );
        CHECK( layout5 == "Weather: <color_c_light_cyan>Sunny</color>" );
    }

    SECTION( "Multiline drawing splits newlines correctly" ) {
#if !(defined(TILES) || defined(_WIN32))
        // Running the tests in a developer environment works fine, but
        // the CI env has no interactive shell, so we skip the screen scraping.
        const char *term_env = ::getenv( "TERM" );
        // The tests don't initialize the curses window, so initialize it here...
        if( term_env != nullptr && std::string( term_env ) != "unknown" &&
            cata_curses_test::initscr() != nullptr ) {
#endif
            catacurses::window w = catacurses::newwin( rows, cols, point_zero );

            werase( w );
            SECTION( "Single-line layout" ) {
                std::string layout1 = "abcd efgh ijkl mnop qrst";
                CHECK( widget::custom_draw_multiline( layout1, w, 1, 30, 0 ) == 1 );
                std::vector<std::string> lines = scrape_win_at( w, point_zero, cols, rows );
                CHECK( lines[0] == " abcd efgh ijkl mnop qrst       " );
                CHECK( lines[1] == "                                " );
                CHECK( lines[2] == "                                " );
                CHECK( lines[3] == "                                " );
                CHECK( lines[4] == "                                " );
            }

            werase( w );
            SECTION( "Single-line layout" ) {
                std::string layout5 = "abcd\nefgh\nijkl\nmnop\nqrst";
                CHECK( widget::custom_draw_multiline( layout5, w, 1, 30, 0 ) == 5 );
                std::vector<std::string> lines = scrape_win_at( w, point_zero, cols, rows );
                CHECK( lines[0] == " abcd                           " );
                CHECK( lines[1] == " efgh                           " );
                CHECK( lines[2] == " ijkl                           " );
                CHECK( lines[3] == " mnop                           " );
                CHECK( lines[4] == " qrst                           " );
            }

#if !(defined(TILES) || defined(_WIN32))
            // ... and free it here
            cata_curses_test::endwin();
        }
#endif
    }
}
