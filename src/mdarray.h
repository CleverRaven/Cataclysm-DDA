#pragma once
#ifndef CATA_SRC_MDARRAY_H
#define CATA_SRC_MDARRAY_H

#include <array>
#include <cstddef>
#include <type_traits>

namespace cata
{

template<typename T, typename Point, size_t DimX, size_t DimY>
class mdarray_impl_2d
{
    public:
        static_assert( Point::dimension == 2, "mdarray_impl_2d only for use with 2D point types" );

        using value_type = T;
        using column_type = std::array<T, DimY>;

        static constexpr size_t size_x = DimX;
        static constexpr size_t size_y = DimY;

        column_type &operator[]( size_t x ) {
            return data_[x];
        }

        const column_type &operator[]( size_t x ) const {
            return data_[x];
        }

        value_type &operator[]( const Point &p ) {
            return data_[p.x()][p.y()];
        }

        const value_type &operator[]( const Point &p ) const {
            return data_[p.x()][p.y()];
        }

        void fill( const T &t_all ) {
            for( column_type &col : data_ ) {
                for( T &t : col ) {
                    t = t_all;
                }
            }
        }
    private:
        std::array<column_type, DimX> data_;
};

template<typename T, typename Point, size_t DimX, size_t DimY>
class mdarray : public std::conditional_t <
// TODO: 3D version not yet implemented
    Point::dimension == 2, mdarray_impl_2d<T, Point, DimX, DimY>, void >
{
    public:
        mdarray() = default;
        explicit mdarray( const T &t ) {
            this->fill( t );
        }
};

} // namespace cata

#endif // CATA_SRC_MDARRAY_H
