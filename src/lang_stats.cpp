#include <initializer_list>
#include <string>

#include "lang_stats.h"

using namespace std::literals::string_view_literals;

static constexpr std::initializer_list<lang_stats> all_lang_stats = {
};

const lang_stats *lang_stats_for( std::string_view lang )
{
    for( const lang_stats &l : all_lang_stats ) {
        // If something went wrong with the update_stats.sh script then the
        // value will probably be -1, so ignore the entry in that case.
        if( l.name == lang && l.num_translated > 0 ) {
            return &l;
        }
    }

    return nullptr;
}
