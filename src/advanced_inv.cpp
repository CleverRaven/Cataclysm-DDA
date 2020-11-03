#include "advanced_inv.h"

#include <memory>  // for unique_ptr
#include <string>  // for operator==, basic_string, string
#include <utility> // for move
#include <vector>  // for vector

#include "activity_type.h"     // for activity_id
#include "advuilist_helpers.h" // for add_aim_sources, setup_for_aim, iloc_...
#include "debug.h"             // for debugmsg
#include "transaction_ui.h"    // for transaction_ui<>::advuilist_t, transa...

void create_advanced_inv()
{
    using namespace advuilist_helpers;
    using mytrui_t = transaction_ui<aim_container_t>;

    static std::unique_ptr<mytrui_t> mytrui;
    static aim_stats_t lstats{ 0_kilogram, 0_liter };
    static aim_stats_t rstats{ 0_kilogram, 0_liter };
    if( !mytrui ) {
        mytrui = std::make_unique<mytrui_t>( aimlayout );
        setup_for_aim( mytrui->left(), &lstats );
        setup_for_aim( mytrui->right(), &rstats );
        add_aim_sources( mytrui->left() );
        add_aim_sources( mytrui->right() );
        mytrui->on_select( aim_transfer );
        mytrui->loadstate( &uistate.transfer_save );
    } else {
        mytrui->left()->rebuild();
        mytrui->right()->rebuild();
    }
    mytrui->show();
    mytrui->savestate( &uistate.transfer_save );
}

void cancel_aim_processing()
{
    // uistate.transfer_save.re_enter_move_all = aim_entry::START;
}
