#include <initializer_list>
#include <lang_stats.h>

using namespace std::literals::string_view_literals;

static constexpr std::initializer_list<lang_stats> all_lang_stats = {
#include <lang_stats.inc>
};

const lang_stats *lang_stats_for( std::string_view lang )
{
    for( const lang_stats &l : all_lang_stats ) {
        if( l.name == lang ) {
            return &l;
        }
    }

    return nullptr;
}
