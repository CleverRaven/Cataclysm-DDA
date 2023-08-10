#ifndef CATA_OBJECT_CREATOR_CREATOR_MAIN_WINDOW_H
#define CATA_OBJECT_CREATOR_CREATOR_MAIN_WINDOW_H

#include "enum_traits.h"

class QApplication;

namespace creator
{
class main_window
{
    public:
        int execute( QApplication &app );
    private:
};

enum class jsobj_type {
    SPELL,
    item_group,
    LAST
};
}

template<>
struct enum_traits<creator::jsobj_type> {
    static constexpr auto last = creator::jsobj_type::LAST;
};

#endif
