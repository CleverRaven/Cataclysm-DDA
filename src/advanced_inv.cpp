#include "advanced_inv.h"

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
    
    mytrui_t mytrui( aimlayout );
    aim_stats_t stats;
    setup_for_aim( mytrui.left(), &stats );
    setup_for_aim( mytrui.right(), &stats );
    add_aim_sources( mytrui.left() );
    add_aim_sources( mytrui.right() );
    mytrui.on_select( aim_transfer );
    mytrui.show();
}

void cancel_aim_processing()
{
    // uistate.transfer_save.re_enter_move_all = aim_entry::START;
}
