#pragma once
#ifndef CATA_SRC_MDARRAY_H
#define CATA_SRC_MDARRAY_H

#include <array>
#include <cstddef>
#include <initializer_list>
#include <type_traits>

#include "coordinates.h"
#include "point_traits.h"

namespace cata
{

template<typename Point>
struct mdarray_default_size_impl;

template<typename Point, coords::scale Scale, bool InBounds>
struct mdarray_default_size_impl<coords::coord_point<Point, coords::origin::reality_bubble, Scale, InBounds>> {
    static constexpr size_t value = MAPSIZE_X / map_squares_per( Scale );
    static_assert( MAPSIZE_X % map_squares_per( Scale ) == 0, "Scale must be smaller than map" );
};

template<typename Point, coords::origin Origin, coords::scale Scale, bool InBounds>
struct mdarray_default_size_impl<coords::coord_point<Point, Origin, Scale, InBounds>> {
    static constexpr coords::scale outer_scale = coords::scale_from_origin( Origin );
    static constexpr size_t value = map_squares_per( outer_scale ) / map_squares_per( Scale );
    static_assert( value > 0, "Scale must be smaller origin" );
};

template<typename Point>
constexpr size_t mdarray_default_size = mdarray_default_size_impl<Point>::value;

template<typename T, typename Point, size_t DimX, size_t DimY>
// Suppressions due to a clang-tidy bug
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
class mdarray_impl_2d
{
    private:
        static_assert( Point::dimension == 2, "mdarray_impl_2d only for use with 2D point types" );
        using Traits = point_traits<Point>;
    public:
        using value_type = T;
        using column_type = std::array<T, DimY>;

        static constexpr size_t size_x = DimX;
        static constexpr size_t size_y = DimY;

        constexpr mdarray_impl_2d() = default;

        constexpr mdarray_impl_2d( const std::initializer_list<std::initializer_list<T>> &arr )
            : data_{} {
            cata_assert( arr.size() == DimX );
            for( size_t i = 0; i != DimX; ++i ) {
                const std::initializer_list<T> &col = arr.begin()[i];
                cata_assert( col.size() == DimY );
                for( size_t j = 0; j != DimY; ++j ) {
                    data_[i][j] = col.begin()[j];
                }
            }
        }

        column_type &operator[]( size_t x ) {
            return data_[x];
        }

        const column_type &operator[]( size_t x ) const {
            return data_[x];
        }

        value_type &operator[]( const Point &p ) {
            return data_[Traits::x( p )][Traits::y( p )];
        }

        const value_type &operator[]( const Point &p ) const {
            return data_[Traits::x( p )][Traits::y( p )];
        }

        void fill( const T &t_all ) {
            for( column_type &col : data_ ) {
                for( T &t : col ) {
                    t = t_all;
                }
            }
        }

        template<typename F>
        void fill_from_callable( const F &f ) {
            for( column_type &col : data_ ) {
                for( T &t : col ) {
                    t = f();
                }
            }
        }
    private:
        // TODO: in C++17 array::operator[] becomes constexpr and we can use
        // std::array here
        // NOLINTNEXTLINE(modernize-avoid-c-arrays)
        column_type data_[DimX];
};

template<typename T, typename Point, size_t DimX = mdarray_default_size<Point>,
         size_t DimY = mdarray_default_size<Point>>
class mdarray : public std::conditional_t <
// TODO: 3D version not yet implemented
    Point::dimension == 2, mdarray_impl_2d<T, Point, DimX, DimY>, void >
{
    private:
        using base = std::conditional_t <
                     Point::dimension == 2, mdarray_impl_2d<T, Point, DimX, DimY>, void >;
    public:
        mdarray() = default;
        explicit mdarray( const T &t ) {
            this->fill( t );
        }
        using base::base;
};

} // namespace cata

#endif // CATA_SRC_MDARRAY_H
