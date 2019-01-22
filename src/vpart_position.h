#pragma once
#ifndef VPART_POSITION_H
#define VPART_POSITION_H

#include <functional>
#include <string>

#include "optional.h"

class vehicle;
enum vpart_bitflags : int;
class vpart_reference;
struct tripoint;
struct point;

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

        bool is_inside() const;

        /**
         * Sets the label at this part of the vehicle. Removes the label if @p text is empty.
         */
        void set_label( const std::string &text ) const;
        /**
         * @returns The label at this part of the vehicle, if there is any.
         */
        cata::optional<std::string> get_label() const;
        /// @see vehicle::part_with_feature
        cata::optional<vpart_reference> part_with_feature( const std::string &f, bool unbroken ) const;
        /// @see vehicle::part_with_feature
        cata::optional<vpart_reference> part_with_feature( vpart_bitflags f, bool unbroken ) const;
        /**
         * Returns the obstacle that exists at this point of the vehicle (if any).
         * Open doors don't count as obstacles, but closed one do.
         * Broken parts are also never obstacles.
         */
        cata::optional<vpart_reference> obstacle_at_part() const;
        /**
         * Returns the part displayed at this point of the vehicle.
         */
        cata::optional<vpart_reference> part_displayed() const;
        /**
         * Returns the position of this part in the coordinates system that @ref game::m uses.
         * Postcondition (if the vehicle cache of the map is correct and if there are un-removed
         * parts at this positions):
         * `g->m.veh_at( this->pos() )` (there is a vehicle there)
         * `g->m.veh_at( this->pos() )->vehicle() == this->vehicle()` (it's this one)
         */
        // Name chosen to match Creature::pos
        tripoint pos() const;
        /**
         * Returns the mount point: the point in the vehicles own coordinate system.
         * This system is independent of movement / rotation.
         */
        // @todo change to return tripoint.
        point mount() const;
};

/**
 * Simple wrapper to forward functions that may return a @ref cata::optional
 * to @ref vpart_position. They generally return an empty `optional`, or
 * forward to the same function in `vpart_position`.
 */
class optional_vpart_position : public cata::optional<vpart_position>
{
    public:
        optional_vpart_position( cata::optional<vpart_position> p ) : cata::optional<vpart_position>
            ( std::move( p ) ) { }

        cata::optional<std::string> get_label() const {
            return has_value() ? value().get_label() : cata::nullopt;
        }
        cata::optional<vpart_reference> part_with_feature( const std::string &f, bool unbroken ) const;
        cata::optional<vpart_reference> part_with_feature( vpart_bitflags f, bool unbroken ) const;
        cata::optional<vpart_reference> obstacle_at_part() const;
        cata::optional<vpart_reference> part_displayed() const;
};

// For legacy code, phase out, don't use in new code.
//@todo remove this
inline vehicle *veh_pointer_or_null( const optional_vpart_position &p )
{
    return p ? &p->vehicle() : nullptr;
}

#endif
