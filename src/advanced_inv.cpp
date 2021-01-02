#include "advanced_inv.h"

#include <algorithm> // for max
#include <memory>    // for unique_ptr
#include <string>    // for operator==, basic_string, string
#include <tuple>     // for tie
#include <utility>   // for move
#include <vector>    // for vector

#include "advuilist.h"         // for advuilist<>::groupcont_t, advuili...
#include "advuilist_helpers.h" // for add_aim_sources, setup_for_aim, iloc_...
#include "advuilist_sourced.h" // for advuilist_sourced
#include "options.h"           // for get_option
#include "output.h"            // for TERMX, FULL_SCREEN_WIDTH, TERMY
#include "panels.h"            // for panel_manager
#include "point.h"             // for point
#include "transaction_ui.h"    // for transaction_ui<>::advuilist_t, transa...
#include "uistate.h"           // for uistate, uistatedata
#include "units.h"             // for mass, volume, operator""_kilogram

namespace
{
std::pair<point, point> AIM_size( bool full_screen )
{
    int const min_w_width = FULL_SCREEN_WIDTH;
    int const max_w_width = full_screen ? TERMX : std::max( 120,
                            TERMX - 2 * ( panel_manager::get_manager().get_width_right() +
                                          panel_manager::get_manager().get_width_left() ) );

    int const width = TERMX < min_w_width
                      ? min_w_width
                      : TERMX > max_w_width ? max_w_width : TERMX;
    int const originx = TERMX > width ? ( TERMX - width ) / 2 : 0;

    return { point{width, TERMY}, point{originx, 0} };
}
} // namespace

void create_advanced_inv( bool resume )
{
    using namespace advuilist_helpers;
    using mytrui_t = transaction_ui<aim_container_t>;

    static std::unique_ptr<mytrui_t> mytrui;
    static pane_mutex_t pane_mutex{};
    static aim_stats_t lstats{ 0_kilogram, 0_liter };
    static aim_stats_t rstats{ 0_kilogram, 0_liter };
    static bool full_screen{ get_option<bool>( "AIM_WIDTH" ) };
    bool const _fs = get_option<bool>( "AIM_WIDTH" );
    if( !mytrui ) {
        std::pair<point, point> size = AIM_size( full_screen );

        mytrui = std::make_unique<mytrui_t>( aimlayout, size.first, size.second,
                                             "ADVANCED_INVENTORY", point{3, 1} );
        mytrui->on_resize( [&]( mytrui_t *ui ) {
            std::pair<point, point> size = AIM_size( full_screen );
            ui->resize( size.first, size.second );
        } );

        setup_for_aim( mytrui->left(), &lstats );
        setup_for_aim( mytrui->right(), &rstats );
        add_aim_sources( mytrui->left(), &pane_mutex );
        add_aim_sources( mytrui->right(), &pane_mutex );
        mytrui->on_select( aim_transfer );
        mytrui->setctxthandler( [&]( aim_transaction_ui_t *ui, std::string const & action ) {
            aim_ctxthandler( ui, action, &pane_mutex );
        } );
        mytrui->loadstate( &uistate.transfer_save, false );

    } else if( full_screen != _fs ) {
        full_screen = _fs;
        std::pair<point, point> size = AIM_size( full_screen );
        mytrui->resize( size.first, size.second );

    }

    reset_mutex( &pane_mutex );
    if( !resume and get_option<bool>( "OPEN_DEFAULT_ADV_INV" ) ) {
        mytrui->loadstate( &uistate.transfer_default, false );
    } else {
        mytrui->loadstate( &uistate.transfer_save, false );
    }

    reset_mutex( &*mytrui, &pane_mutex );
    aim_rebuild( &*mytrui, &pane_mutex );

    mytrui->show();
    mytrui->savestate( &uistate.transfer_save );
}
