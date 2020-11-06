#include "advanced_inv.h"

#include <memory>  // for unique_ptr
#include <string>  // for operator==, basic_string, string
#include <utility> // for move
#include <vector>  // for vector

#include "activity_type.h"     // for activity_id
#include "advuilist_helpers.h" // for add_aim_sources, setup_for_aim, iloc_...
#include "debug.h"             // for debugmsg
#include "options.h"           // for get_option
#include "panels.h"            // for panel_manager
#include "transaction_ui.h"    // for transaction_ui<>::advuilist_t, transa...

void create_advanced_inv()
{
    using namespace advuilist_helpers;
    using mytrui_t = transaction_ui<aim_container_t>;

    static std::unique_ptr<mytrui_t> mytrui;
    static pane_mutex_t pane_mutex{};
    static aim_stats_t lstats{ 0_kilogram, 0_liter };
    static aim_stats_t rstats{ 0_kilogram, 0_liter };
    static bool full_screen{ get_option<bool>( "AIM_WIDTH" ) };
    bool const _fs = get_option<bool>( "AIM_WIDTH" );
    if( !mytrui or full_screen != _fs ) {
        full_screen = _fs;
        int const min_w_width = FULL_SCREEN_WIDTH;
        int const max_w_width = full_screen ? TERMX : std::max( 120,
                                TERMX - 2 * ( panel_manager::get_manager().get_width_right() +
                                              panel_manager::get_manager().get_width_left() ) );

        int const width = TERMX < min_w_width
                          ? min_w_width
                          : TERMX > max_w_width ? max_w_width : TERMX;
        int const originx = TERMX > width ? ( TERMX - width ) / 2 : 0;

        mytrui = std::make_unique<mytrui_t>( aimlayout, point{ width, TERMY }, point{ originx, 0 },
                                             "ADVANCED_INVENTORY" );
        setup_for_aim( mytrui->left(), &lstats );
        setup_for_aim( mytrui->right(), &rstats );
        add_aim_sources( mytrui->left(), &pane_mutex );
        add_aim_sources( mytrui->right(), &pane_mutex );
        mytrui->on_select( aim_transfer );
        mytrui->setctxthandler( [&]( aim_transaction_ui_t *ui, std::string const & action ) {
            aim_ctxthandler( ui, action, &pane_mutex );
        } );
        mytrui->loadstate( &uistate.transfer_save );
    } else {
        std::size_t const lidx = mytrui->left()->getSource().first;
        std::size_t const ridx = mytrui->right()->getSource().first;
        pane_mutex[lidx] = false;
        mytrui->left()->rebuild();
        pane_mutex[lidx] = true;
        pane_mutex[ridx] = false;
        mytrui->right()->rebuild();
    }
    reset_mutex( &*mytrui, &pane_mutex );
    mytrui->show();
    mytrui->savestate( &uistate.transfer_save );
}
