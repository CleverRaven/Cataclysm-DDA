#pragma once
#ifndef VPART_POSITION_H
#define VPART_POSITION_H

#include <functional>

class vehicle;

/**
 * Reference to a position (a point) of the @ref vehicle.
 * It does not refer to a specific vehicle part, but to a mount point of a
 * vehicle that contains one or more vehicle parts.
 *
 * It's supposed to be the basic vehicle interface for the @ref map.
 * Another class shall be used to get a reference to a specific part of the
 * vehicle.
 *
 * Note that it must be created with a valid vehicle reference and a valid
 * part index. An instance can become invalid when the referenced vehicle is
 * changed (parts added / removed or whole vehicle removed). There is no way
 * to detect this (it behaves like C++ references).
 */
class vpart_position
{
    private:
        std::reference_wrapper<::vehicle> vehicle_;
        size_t part_index_;

    public:
        vpart_position( ::vehicle &v, const size_t part ) : vehicle_( v ), part_index_( part ) { }
        vpart_position( const vpart_position & ) = default;

        ::vehicle &vehicle() const {
            return vehicle_.get();
        }
        //@todo remove this, add a vpart_reference class instead
        size_t part_index() const {
            return part_index_;
        }
};

namespace cata
{
template<typename T>
class optional;
} // namespace cata

// For legacy code, phase out, don't use in new code.
//@todo remove this
template<typename T>
inline vehicle *veh_pointer_or_null( const cata::optional<T> &p )
{
    return p ? &p->vehicle() : nullptr;
}

#endif
