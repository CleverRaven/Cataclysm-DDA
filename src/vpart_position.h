#pragma once
#ifndef CATA_SRC_VPART_POSITION_H
#define CATA_SRC_VPART_POSITION_H

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "coords_fwd.h"
#include "item.h"
#include "type_id.h"

class Character;
class inventory;
class map;
class vehicle;
class vehicle_stack;
class vpart_info;
class vpart_reference;
enum vpart_bitflags : int;
struct vehicle_part;

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

        ::vehicle &vehicle() const {
            return vehicle_.get();
        }
        // TODO: remove this, add a vpart_reference class instead
        size_t part_index() const {
            return part_index_;
        }

        bool is_inside() const;

        /**
         * @returns Movement difficulty. 0 for impassable.
         */
        int get_movecost() const;

        /**
         * Sets the label at this part of the vehicle. Removes the label if @p text is empty.
         */
        void set_label( const std::string &text ) const;
        /**
         * @returns The label at this part of the vehicle, if there is any.
         */
        std::optional<std::string> get_label() const;
        // @return reference to unbroken CARGO part at this position or std::nullopt
        std::optional<vpart_reference> cargo() const;
        /// @see vehicle::part_with_feature
        std::optional<vpart_reference> part_with_feature( const std::string &f, bool unbroken,
                bool include_fake = false ) const;
        /// @see vehicle::part_with_feature
        std::optional<vpart_reference> part_with_feature( vpart_bitflags f, bool unbroken,
                bool include_fake = false ) const;
        /// @see vehicle::part_with_feature
        std::optional<vpart_reference> avail_part_with_feature( const std::string &f ) const;
        /// @see vehicle::part_with_feature
        std::optional<vpart_reference> avail_part_with_feature( vpart_bitflags f ) const;
        /**
         * Returns the obstacle that exists at this point of the vehicle (if any).
         * Open doors don't count as obstacles, but closed one do.
         * Broken parts are also never obstacles.
         */
        std::optional<vpart_reference> obstacle_at_part() const;
        /**
         * Returns the part displayed at this point of the vehicle.
         */
        std::optional<vpart_reference> part_displayed() const;

        // Finds vpart_reference to inner part with specified tool
        std::optional<vpart_reference> part_with_tool( map &here, const itype_id &tool_type ) const;
        // Returns a list of all tools provided by vehicle and their hotkey
        std::map<item, int> get_tools( map &here ) const;
        // Forms inventory for inventory::form_from_map
        void form_inventory( map &here, inventory &inv ) const;

        tripoint_bub_ms pos_bub( const map &here ) const;
        tripoint_abs_ms pos_abs() const;
        /**
         * Returns the mount point: the point in the vehicles own coordinate system.
         * This system is independent of movement / rotation.
         */
        // TODO: change to return tripoint.
        point_rel_ms mount_pos() const;

        // implementation required for using as std::map key
        bool operator<( const vpart_position &other ) const;
};

/**
 * Simple wrapper to forward functions that may return a @ref std::optional
 * to @ref vpart_position. They generally return an empty `optional`, or
 * forward to the same function in `vpart_position`.
 */
class optional_vpart_position : public std::optional<vpart_position>
{
    public:
        explicit optional_vpart_position( std::optional<vpart_position> p ) :
            std::optional<vpart_position>( p ) { }

        std::optional<std::string> get_label() const {
            return has_value() ? value().get_label() : std::nullopt;
        }
        // @return reference to unbroken CARGO part at this position or std::nullopt
        std::optional<vpart_reference> cargo() const;
        std::optional<vpart_reference> part_with_feature( const std::string &f, bool unbroken ) const;
        std::optional<vpart_reference> part_with_feature( vpart_bitflags f, bool unbroken ) const;
        std::optional<vpart_reference> avail_part_with_feature( const std::string &f ) const;
        std::optional<vpart_reference> avail_part_with_feature( vpart_bitflags f ) const;
        std::optional<vpart_reference> obstacle_at_part() const;
        std::optional<vpart_reference> part_displayed() const;
        std::optional<vpart_reference> part_with_tool( map &here, const itype_id &tool_type ) const;
        std::vector<std::string> extended_description() const;
};

/**
 * This is a wrapper over a vehicle pointer and a reference to a part of it.
 *
 * The class does not support an "invalid" state, it is created from a
 * valid reference and the user must ensure it's still valid when used.
 * Most functions just forward to the equally named functions in the @ref vehicle
 * class, so see there for documentation.
 */
class vpart_reference : public vpart_position
{
    public:
        vpart_reference( ::vehicle &v, const size_t part ) : vpart_position( v, part ) { }
        vpart_reference( const vpart_reference & ) = default;
        vpart_reference &operator=( const vpart_reference & ) = default;

        using vpart_position::vehicle;

        /// Yields the \ref vehicle_part object referenced by this. @see vehicle::parts
        vehicle_part &part() const;
        /// See @ref vehicle_part::info
        const vpart_info &info() const;
        // @return vehicle_stack constructed from the part's cargo items
        vehicle_stack items() const;
        /**
         * Returns whether the part *type* has the given feature.
         * Note that this is different from part flags (which apply to part
         * instances).
         * For example a feature is "CARGO" (the part can store items).
         */
        /**@{*/
        bool has_feature( const std::string &f ) const;
        bool has_feature( vpart_bitflags f ) const;
        /**@}*/

        /// Returns the passenger in this part, or nullptr if no passenger.
        Character *get_passenger() const;
};

// For legacy code, phase out, don't use in new code.
// TODO: remove this
inline vehicle *veh_pointer_or_null( const optional_vpart_position &p )
{
    return p ? &p->vehicle() : nullptr;
}

#endif // CATA_SRC_VPART_POSITION_H
