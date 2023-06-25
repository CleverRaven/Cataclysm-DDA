#pragma once
#ifndef CATA_SRC_SETS_INTERSECT_H
#define CATA_SRC_SETS_INTERSECT_H

namespace cata
{

template<typename SetL, typename SetR>
bool sets_intersect( const SetL &l, const SetR &r )
{
    // There ar two reasonable implementation strategies for ordered sets, but
    // only one for unordered sets.  We use the approach that works for both,
    // but if both sets were ordered you can instead step though them in
    // parallel looking for a dupe.
    if( l.size() > r.size() ) {
        return sets_intersect( r, l );
    }

    for( const auto &elem : l ) {
        if( r.count( elem ) ) {
            return true;
        }
    }

    return false;
}

} // namespace cata

#endif // CATA_SRC_SETS_INTERSECT_H
