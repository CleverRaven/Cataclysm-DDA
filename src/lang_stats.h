#ifndef CATA_SRC_LANG_STATS_H
#define CATA_SRC_LANG_STATS_H

#include <string_view>

struct lang_stats {
    std::string_view name;
    int num_translated;
    int num_untranslated;

    float percent_translated() const {
        return 100.0 * num_translated / ( num_translated + num_untranslated );
    }
};

const lang_stats *lang_stats_for( std::string_view lang );

#endif // CATA_SRC_LANG_STATS_H
