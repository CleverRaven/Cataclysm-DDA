#pragma once
#ifndef VPART_REFERENCE_H
#define VPART_REFERENCE_H

#include <functional>

class vehicle;

/**
 * This is a wrapper over a vehicle pointer and a reference to a part of it.
 *
 * The class does not support an "invalid" state, it is created from a
 * valid reference and the user must ensure it's still valid when used.
 * Most functions just forward to the equally named functions in the @ref vehicle
 * class, so see there for documentation.
 */
class vpart_reference
{
    private:
        std::reference_wrapper<::vehicle> vehicle_;
        size_t part_index_;

    public:
        vpart_reference( ::vehicle &v, const size_t part ) : vehicle_( v ), part_index_( part ) { }
        vpart_reference( const vpart_reference & ) = default;

        ::vehicle &vehicle() const {
            return vehicle_.get();
        }
        //@todo remove this
        size_t part_index() const {
            return part_index_;
        }
};

#endif
