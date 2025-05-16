#pragma once
#ifndef CATA_SRC_VPART_RANGE_H
#define CATA_SRC_VPART_RANGE_H

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <utility>

#include "vpart_position.h"

class vehicle;
enum class part_status_flag : int;

/**
 * Exposes (multiple) parts of one vehicle as @ref vpart_reference.
 * Uses an arbitrary @ref range_type to supply
 * - the vehicle reference,
 * - information which parts should be iterated over (and which
 *   should be skipped).
 */
template<typename range_type>
class vehicle_part_iterator
{
    private:
        std::reference_wrapper<const range_type> range_;
        std::optional<vpart_reference> vp_;

        const range_type &range() const {
            return range_.get();
        }
        void skip_to_next_valid( size_t i ) {
            while( i < range().part_count() &&
                   !range().matches( i ) ) {
                ++i;
            }
            if( i < range().part_count() ) {
                vp_.emplace( range().vehicle(), i );
            } else {
                vp_.reset();
            }
        }

    public:
        vehicle_part_iterator( const range_type &r, size_t i ) : range_( r ) {
            cata_assert( i <= range().part_count() );
            skip_to_next_valid( i );
        }

        const vpart_reference &operator*() const {
            cata_assert( vp_ );
            return *vp_;
        }
        const vpart_reference *operator->() const {
            cata_assert( vp_ );
            return &*vp_;
        }

        vehicle_part_iterator &operator++() {
            cata_assert( vp_ );
            skip_to_next_valid( vp_->part_index() + 1 );
            return *this;
        }

        bool operator==( const vehicle_part_iterator &rhs ) const {
            if( vp_.has_value() != rhs.vp_.has_value() ) {
                return false;
            }
            if( !vp_.has_value() ) {
                return true;
            }
            return &vp_->vehicle() == &rhs.vp_->vehicle() && vp_->part_index() == rhs.vp_->part_index();
        }
        bool operator!=( const vehicle_part_iterator &rhs ) const {
            return !operator==( rhs );
        }
};

template<class T>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::iterator_traits<vehicle_part_iterator<T>> {
    using difference_type = size_t;
    using value_type = vpart_reference;
    // TODO: maybe change into random access iterator? This requires adding
    // more operators to the iterator, which may not be efficient.
    using reference = const vpart_reference &;
    using pointer = const vpart_reference *;
    using iterator_category = std::forward_iterator_tag;
};

/**
 * The generic range, it misses the `bool contained(size_t)` function that is
 * required by the iterator class. You need to derive from it and implement
 * that function. It uses the curiously recurring template pattern (CRTP),
 * so use your derived range class as @ref range_type.
 */
template<typename range_type>
class generic_vehicle_part_range
{
    private:
        std::reference_wrapper<::vehicle> vehicle_;
        bool with_fake_;

    public:
        explicit generic_vehicle_part_range( ::vehicle &v, bool with_fake = false ) : vehicle_( v ),
            with_fake_( with_fake ) { }

        // Templated because see top of file.
        template<typename T = ::vehicle>
        size_t part_count() const {
            if( with_fake_ ) {
                return static_cast<const T &>( vehicle_.get() ).part_count();
            } else {
                return static_cast<const T &>( vehicle_.get() ).part_count_real_cached();
            }

        }

        using iterator = vehicle_part_iterator<range_type>;
        iterator begin() const {
            return iterator( const_cast<range_type &>( static_cast<const range_type &>( *this ) ), 0 );
        }
        iterator end() const {
            return iterator( const_cast<range_type &>( static_cast<const range_type &>( *this ) ),
                             part_count() );
        }

        friend bool empty( const generic_vehicle_part_range<range_type> &range ) {
            return range.begin() == range.end();
        }
        friend size_t size( const generic_vehicle_part_range<range_type> &range ) {
            return std::distance( range.begin(), range.end() );
        }
        friend iterator begin( const generic_vehicle_part_range<range_type> &range ) {
            return range.begin();
        }
        friend iterator end( const generic_vehicle_part_range<range_type> &range ) {
            return range.end();
        }

        ::vehicle &vehicle() const {
            return vehicle_.get();
        }
};

/** A range that contains all parts of the vehicle. */
class vehicle_part_range : public generic_vehicle_part_range<vehicle_part_range>
{
    public:
        explicit vehicle_part_range( ::vehicle &v ) : generic_vehicle_part_range( v ) { }

        bool matches( size_t part ) const;
};

class vehicle_part_with_fakes_range : public
    generic_vehicle_part_range<vehicle_part_with_fakes_range>
{
    private:
        bool with_inactive_fakes_;
    public:
        vehicle_part_with_fakes_range( ::vehicle &v, bool with_inactive ) :
            generic_vehicle_part_range( v, true ),
            with_inactive_fakes_( with_inactive ) { }

        bool matches( size_t part ) const;
};

/** A range that contains parts that have a given feature and (optionally) are not broken. */
template<typename feature_type>
class vehicle_part_with_feature_range : public
    generic_vehicle_part_range<vehicle_part_with_feature_range<feature_type>>
{
    private:
        feature_type feature_;
        part_status_flag required_;

    public:
        vehicle_part_with_feature_range( ::vehicle &v, feature_type f, part_status_flag r ) :
            generic_vehicle_part_range<vehicle_part_with_feature_range<feature_type>>( v ),
                    feature_( std::move( f ) ), required_( r ) { }

        bool matches( size_t part ) const;
};

#endif // CATA_SRC_VPART_RANGE_H
